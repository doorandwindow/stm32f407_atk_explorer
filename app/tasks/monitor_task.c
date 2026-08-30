/**
 ******************************************************************************
 * @file    monitor_task.c
 * @brief   系统状态监控任务 (10s 周期打印内存 + CPU 占用率)
 * @note    内存三层水位(KB 显示, 0.1KB 定点换算): ① FreeRTOS 堆(heap_4,
 *          xPortGetFreeHeapSize / xPortGetMinimumEverFreeHeapSize, 峰值=总量-历史
 *          最小剩余, 内核天然记录);
 *          ② 芯片内部主 SRAM 128KB(静态区 = 链接符号 _sdata~_ebss, 含 40KB 堆池)
 *             + CCM 64KB(LVGL 渲染单缓冲) + 中断栈水印(图案填充+扫描);
 *          ③ 外部 SRAM 1MB(LVGL 对象池 128KB@0x68000000, lv_mem_monitor 查动态水位)。
 *          CPU: 依赖 configGENERATE_RUN_TIME_STATS (TIM11 1MHz 统计时钟, 见
 *          bsp/components/rtstats/), 忙占比 = 100% - 空闲任务运行占比, 0.1% 整数精度。
 *          单行输出走 dbg_printf 的 160B 栈缓冲, 注意字段总量勿超。
 ******************************************************************************
 */

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"

#include "main.h"
#include "iwdg.h"
#include "uart_dbg.h"
#include "board.h"
#include "lvgl.h"

/* ---- 打印周期 ---- */
#define MONITOR_PERIOD_MS   10000U

/* ---- 芯片 RAM 布局常量 ---- */
#define CCM_RAM_SIZE        (64U * 1024U)      /* CCM 64KB @0x10000000 */
#define ISR_STACK_PATTERN   0xA5A5A5A5UL       /* MSP(中断栈)水印填充图案 */

/* 链接脚本符号(取地址使用): 内部主 SRAM 布局 */
extern uint32_t _sstack;    /* MSP 栈底 = _estack - _Min_Stack_Size(0x400) */
extern uint32_t _estack;    /* 内部主 SRAM 顶 0x20020000 */
extern uint32_t _sdata;     /* .data 起始(RAM 侧) */
extern uint32_t _ebss;      /* .bss 结束(静态占用 = data+bss, 含 40KB FreeRTOS 堆池 ucHeap) */

extern uint32_t lv_port_disp_buf_bytes(void);   /* CCM 渲染单缓冲字节数 */

/**
 * @brief  字节 -> 0.1KB 定点数(四舍五入), 打印时拆成 x.y
 * @note   全整数运算(FPU 未开), 精度 ~103 字节, 足够看水位
 */
static unsigned kb_x10(uint32_t bytes)
{
  return (unsigned)((bytes * 10U + 512U) / 1024U);
}

/**
 * @brief  MSP(中断栈)水印填充: [_sstack, _estack) 全部填图案
 * @note   关中断期间填充, 避免与 ISR 压栈互踩。该区间是链接脚本保留的中断栈,
 *         任务线程模式下 MSP 停在栈顶、区间全是已弹出的死内存, 覆写安全;
 *         之后 ISR 每次压栈都会"刻"下印记, 扫描即得开机以来最深占用。
 */
static void isr_stack_paint(void)
{
  uint32_t *p;
  taskENTER_CRITICAL();
  for (p = (uint32_t *)&_sstack; p < (uint32_t *)&_estack; p++)
  {
    *p = ISR_STACK_PATTERN;
  }
  taskEXIT_CRITICAL();
}

/**
 * @brief  扫描 MSP 栈水印, 返回自填充以来 ISR 栈的最深占用字节数
 * @note   ISR 的 C 帧从 MSP 常驻点(_estack 下方约 32B, CM4F 端口切换残留)连续
 *         向下压栈且弹出不清内存, 故最深的被写字 = 从栈底向上第一个非图案字,
 *         占用 = 栈顶 - 该字; 全是图案则返回 0。注意不能从栈顶向下找——那只会
 *         命中最浅残留, 深层使用被上方未触碰的图案掩蔽。
 */
static uint32_t isr_stack_used(void)
{
  uint32_t top  = (uint32_t)&_estack;
  uint32_t base = (uint32_t)&_sstack;
  const uint32_t *p;

  for (p = (const uint32_t *)base; p < (const uint32_t *)top; p++)
  {
    if (*p != ISR_STACK_PATTERN)
    {
      return top - (uint32_t)p;
    }
  }
  return 0U;
}

/**
 * @brief  系统状态监控任务
 * @param  argument: Not used
 */
void StartMonitorTask(void *argument)
{
  (void)argument;

  TaskStatus_t idle_st;
  uint32_t last_rt = 0U;        /* 上次采样的运行时钟总计数 (us) */
  uint32_t last_idle_rt = 0U;   /* 上次采样的空闲任务运行计数 (us) */

  /* 芯片级 RAM 快照(固定项只算一次) */
  uint32_t iram_total  = (uint32_t)&_estack - 0x20000000UL;  /* 主 SRAM 128KB */
  uint32_t iram_static = (uint32_t)&_ebss - (uint32_t)&_sdata;
  uint32_t ccm_buf     = lv_port_disp_buf_bytes();

  isr_stack_paint();   /* 刻中断栈水印基线 */

  dbg_printf("[mon] monitorTask started, period %ums\r\n", (unsigned)MONITOR_PERIOD_MS);

  uint32_t next_wake = osKernelGetTickCount();
  for (;;)
  {
    next_wake += MONITOR_PERIOD_MS;
    osDelayUntil(next_wake);

    HAL_IWDG_Refresh(&hiwdg);   /* 喂狗: IWDG 超时 ~2s */

    /* ---- CPU 占用率: 本窗口内空闲任务运行时间的补数 ---- */
    uint32_t rt_now = portGET_RUN_TIME_COUNTER_VALUE();
    vTaskGetInfo(xTaskGetIdleTaskHandle(), &idle_st, pdFALSE, eRunning);
    uint32_t idle_rt = idle_st.ulRunTimeCounter;

    uint32_t dt      = rt_now - last_rt;        /* 窗口总运行时间 (us) */
    uint32_t d_idle  = idle_rt - last_idle_rt;  /* 窗口空闲时间 (us) */
    last_rt     = rt_now;
    last_idle_rt = idle_rt;

    /* 千分比整数运算, 打印时再拆成 x.y% (FPU 未开, 避开浮点) */
    uint32_t busy_pm = 0U;
    if (dt > 0U)
    {
      busy_pm = (uint32_t)(((uint64_t)(dt - d_idle) * 1000U) / dt);
    }

    /* ---- FreeRTOS 堆: 当前 + 开机以来历史极值 ---- */
    uint32_t heap_total    = (uint32_t)configTOTAL_HEAP_SIZE;
    uint32_t heap_free     = (uint32_t)xPortGetFreeHeapSize();
    uint32_t heap_min_free = (uint32_t)xPortGetMinimumEverFreeHeapSize();

    unsigned ht = kb_x10(heap_total);
    unsigned hu = kb_x10(heap_total - heap_free);
    unsigned hf = kb_x10(heap_free);
    unsigned hp = kb_x10(heap_total - heap_min_free);
    unsigned hm = kb_x10(heap_min_free);
    dbg_printf("[mon] cpu %u.%u%% | heap(kb) total %u.%u used %u.%u free %u.%u peak_used %u.%u min_free %u.%u\r\n",
               (unsigned)(busy_pm / 10U), (unsigned)(busy_pm % 10U),
               ht / 10U, ht % 10U, hu / 10U, hu % 10U, hf / 10U, hf % 10U,
               hp / 10U, hp % 10U, hm / 10U, hm % 10U);

    /* ---- 芯片内部 RAM: 主 SRAM 静态区 + 中断栈水印; CCM 渲染缓冲 ---- */
    uint32_t isr_stack = isr_stack_used();
    unsigned it = kb_x10(iram_total);
    unsigned is = kb_x10(iram_static);
    unsigned iw = kb_x10(isr_stack);
    unsigned ifr = kb_x10(iram_total - iram_static - isr_stack);
    unsigned ct = kb_x10(CCM_RAM_SIZE);
    unsigned cb = kb_x10(ccm_buf);
    unsigned cf = kb_x10(CCM_RAM_SIZE - ccm_buf);
    dbg_printf("[mon] iram(kb) total %u.%u static %u.%u isr_stack %u.%u free %u.%u | ccm(kb) total %u.%u buf %u.%u free %u.%u\r\n",
               it / 10U, it % 10U, is / 10U, is % 10U, iw / 10U, iw % 10U,
               ifr / 10U, ifr % 10U, ct / 10U, ct % 10U, cb / 10U, cb % 10U,
               cf / 10U, cf % 10U);

    /* ---- 芯片外部 RAM 1MB: 目前 LVGL 对象池(128KB@0x68000000)是唯一占用者 ----
       used = TLSF 池实时占用(含分配器块头开销); lvgl_max = LVGL 分配记账的
       历史峰值(lv_mem.c 按请求字节累计), 两者口径不同, max < used 属正常 */
    lv_mem_monitor_t lv_mem;
    lv_mem_monitor(&lv_mem);
    unsigned eu = kb_x10(lv_mem.total_size - lv_mem.free_size);
    unsigned em = kb_x10(lv_mem.max_used);
    dbg_printf("[mon] eram(kb) total %u.%u pool %u.%u used %u.%u lvgl_max %u.%u\r\n",
               kb_x10(BOARD_EXT_SRAM_SIZE) / 10U, kb_x10(BOARD_EXT_SRAM_SIZE) % 10U,
               kb_x10(lv_mem.total_size) / 10U, kb_x10(lv_mem.total_size) % 10U,
               eu / 10U, eu % 10U, em / 10U, em % 10U);
  }
}
