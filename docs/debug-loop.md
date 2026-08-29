# STM32F407 debug loop 事实卡

> 上板闭环脚本约定。所有 `devloop.py`/`serial_capture.py`/`flash.py` 调用都用这张卡的参数。

## 硬件平台
- 主控: STM32F407ZGT6 (Cortex-M4F @ 168 MHz)
- 开发板: 正点原子探索者 V2.2
- 烧录器: ST-Link (SWD)
- 调试串口: USART1 (板载 CH340, USB-SERIAL CH340 (COM18))
- TFTLCD: NT35510, 480×800, FSMC Bank4 NE4 (0x6C000000), 背光 PB15
- 触摸: GT9147 (模拟 I2C, 0x14)
- 外部 SRAM: IS62WV51216, FSMC Bank3 NE3 (0x68000000), 1 MB
  - LVGL mem pool: 0x68000000 起 128 KB
  - LVGL disp buf:  0x68020000 起约 192 KB (双缓冲，每个 480×100)

## 串口参数
- 端口: COM18 (CH340)
- 波特率: 115200
- 数据位: 8, 停止: 1, 校验: none, 流控: none
- 默认拉低 DTR/RTS (复位不接管)

## 已知启动输出（dbg_printf 走 USART1）

`Src/main.c` 在 main 开头打印复位源 (RCC->CSR) 并清标志。
`Src/freertos.c` 打印 `lvglTestTask started` / `LCD_Init` / `GT9147` / `LVGL ready` 四阶段。

预期正常启动输出:
```
[dbg] RST cause 0x04000003: PIN      (烧录复位; IWDG 标志出现即说明发生过喂狗失败)
[dbg] lvglTestTask started
[dbg] LCD_Init done, ID=0x5510
[dbg] GT9147 CTP ID: 917S
[dbg] GT9147 OK
[dbg] LVGL ready
[dbg] alive N s ...
```

按键控制 LED 任务 (keyLedTask) 追加输出 (2026-08-29 新增, 已上板验证):
```
[key] keyLedTask started, KEY0=PE4 LED1=PF10 (active-low)
[key] KEY0 press -> LED1 ON      (按 KEY0(PE4) 时出现, 严格 ON/OFF 交替, 消抖 20ms)
```

长按调光任务 (keyBrightTask) 输出 (2026-08-29 新增, 已上板验证; 亮度的 LED 对换方案):
```
[bright] keyBrightTask started, KEY1=PE3 LED0=PF9 (HW-PWM, 1000 levels)
[bright] KEY1 hold -> start ramping
[bright] duty 5%               (按住后每跨 5% 打印一次; 0%->100%->0% 往返)
[bright] KEY1 release, duty=NN%
[bright] KEY1 tap(short) -> no change   (按住 <400ms, 不调光)
```

## 已知坑 / 风险点

1. **IWDG 超时约 2s** (LSI 32kHz /16, Reload 4095; Prescaler/Reload 已回写 `CubeMX_Config.ioc`, 重新生成不会再回退 /4≈0.5s)
   - defaultTask (每 1ms) + lvglTestTask (每 5ms) 喂狗; 调度器没跑起来之前狗已在计数
2. **HardFault_Handler 已带诊断打印** (`Src/stm32f4xx_it.c`): 8 槽栈帧 + HFSR/CFSR/BFAR
   - BusFault/MemManage/UsageFault 单独 handler 仍是静默 while(1)——现场会被升级或吞掉
3. **FreeRTOSConfig.h 的两个检测宏是手工加的, CubeMX 重新生成会丢** (2026-08-27)
   - `configCHECK_FOR_STACK_OVERFLOW=2` + `configUSE_MALLOC_FAILED_HOOK=1`
   - 对应钩子在 `Src/freertos.c` USER CODE Application 区末尾 (`[HOOK] STACK OVERFLOW task=...`)
   - CubeMX GUI 里也可勾选: FreeRTOS 页 → Advanced settings → USE_MALLOC_FAILED_HOOK / CHECK_FOR_STACK_OVERFLOW
4. **MX_LWIP_Init 在 defaultTask 里同步调用** (`Src/freertos.c`), PHY 失联时阻塞——该任务栈给 2KB 起步
5. HAL 时基走 TIM7, SysTick 给 FreeRTOS — 不冲突
6. ~~configCHECK_FOR_STACK_OVERFLOW=2 会破坏启动~~ **已证伪**: 之前"开检测就不能启动"是因为钩子
   尚未就位/配置半途而废; 现状检测全开 + 2KB 栈, 启动干净 (2026-08-27 两次 8-10s 抓串口验证)
7. **触摸 IC 实为 GT917S, 不是 GT9147** (2026-08-28 串口逐次诊断实测: 0x8140 读回 "917S")
   - 寄存器/时序与 GT9147 兼容, 驱动通用; 但 ID 校验必须同时接受 "9147"/"917S",
     否则 init 报 FAIL (扫描通路不查 ID, 触摸仍能用, 但会跳过软复位+量程修正)
   - **CT_INT(PB1) 在复位释放后不可推挽驱动** (哪怕 ~10ms): 会与 GT9147 的 INT 输出顶驱,
     模块亚健康。正确做法: INT 全程输入上拉, 只操作 RST, 地址 0x14 靠上拉高电平选出
   - 修复后量程修正带幂等保护 (已对则跳过写), 避免每次上电写 GT 配置 EEPROM 磨损

## RAM 布局速查 (build/Debug/CubeMX_Config.map, -O0)

```
Idle_TCB      0x20000810  0x5C
Idle_Stack    0x2000086C  0x200   (configMINIMAL_STACK_SIZE=128 字)
Timer_TCB     0x20000A6C  0x5C
Timer_Stack   0x20000AC8  0x400   (configTIMER_TASK_STACK_DEPTH=256 字)
ucHeap        0x20000EC8  0x8000  (FreeRTOS 堆, 任务栈从这里分配)
xStart/pxEnd  0x20008EC8       (heap_4 元数据)
BSS 结束      0x20019C74       (_ebss, RAM 128KB 还剩 ~25KB — 堆不越界, 别再怀疑这个)
MSP 顶        0x20020000       (_estack, 保留区仅 _Min_Stack_Size=1KB)
```

## 常用命令

### 抓串口 (N 秒)
```bash
python "C:\Users\syclx\.claude\skills\stm32-debug-loop\scripts\serial_capture.py" \
  --port COM18 --baud 115200 --seconds <N> --ts \
  --out "D:\video_my\stm32_ai\stm32f407\build\devloop_last\uart.log"
```

### 一把梭 (build + flash + capture)
```bash
python "C:\Users\syclx\.claude\skills\stm32-debug-loop\scripts\devloop.py" \
  --project "D:/video_my/stm32_ai/stm32f407" --seconds 8 --ts
```

### 仅烧录
```bash
python "C:\Users\syclx\.claude\skills\stm32-debug-loop\scripts\flash.py" \
  --elf "D:\video_my\stm32_ai\stm32f407\build\Debug\CubeMX_Config.elf"
```

## 调试经验沉淀

### 2026-08-27: 板子一直重启 — ✅ 已修复 (defaultTask 栈 512B → 2KB)

**现象**: 每隔 ~2.86s 重启一次。串口: `LVGL ready` → 第一次 `lv_task_handler` 结束 → ~70ms 后
HardFault → 卡死 → 2s 后 IWDG 复位, 无限循环。

**真实根因 (单变量二分, 双向可复现)**:
- `defaultTask` 栈仅 512B (128*4), 却同步跑 `MX_LWIP_Init()`: .su 实测调用链
  `MX_LWIP_Init(24)+netif_add(32)+ethernetif_init(16)+low_level_init(176)+HAL_ETH_Init(24)+StartDefaultTask`
  ≈ 300B+, 加上阻塞/恢复帧, 512B 必然溢出
- 任务栈从 ucHeap 分配, 溢出向下踩坏 heap 空闲链/相邻调度结构 → PendSV 恢复出坏上下文
- `BX LR` (LR=0x0000005C) → UsageFault INVPC (CFSR=0x00020000) → HardFault 卡死 → IWDG 复位
- 512B+检测开时死得更快: 调度早期即 lockup, 连 lvglTestTask 第一行打印都没有

**故障帧垃圾值的解码 (定位关键)**: 打印出的"异常帧"根本不是帧——
`R1=R3=0x20000AC8`=Timer_Stack, `R2=0x20000A6C`=Timer_TCB, `LR=0x5C`=TCB 大小,
全是 map 里 cmsis_os2.c 静态区的调度器结构 → CPU 把调度器数据当代码/上下文恢复了。

**排除的错误假设 (勿再走回头路)**:
- ~~FreeRTOS 堆 32KB 越界 SRAM~~: map 证明 ucHeap 完全在 RAM 内, BSS 后还剩 ~25KB
- ~~LVGL/外部 SRAM/FSMC 问题~~: LCD ID=0x5510 读取正常, LVGL 渲染链路完好
- ~~IWDG 喂狗时序问题~~: 两任务都喂, 狗只是死后的收尸人不是凶手

**修复清单 (已全部落地并上板验证)**:
1. `Src/freertos.c`: defaultTask `.stack_size = 512 * 4` (2KB), 注释写明原因
2. `Inc/FreeRTOSConfig.h`: `configCHECK_FOR_STACK_OVERFLOW=2` + `configUSE_MALLOC_FAILED_HOOK=1`
   ⚠ CubeMX 重新生成会删, 需重加 (见已知坑 #3)
3. `Src/freertos.c` USER CODE Application 末尾: 两个钩子打印任务名后卡死等 IWDG, 常备"行车记录仪"

**验证**: 2KB 版连续 8-10s 串口零 HF/零 HOOK/零 IWDG 标志, alive 心跳正常;
512B 版必死。修复后烧录复位标志只有 PIN, 无 IWDG。

**遗留观察 (非故障)**:
- lvglTestTask 每 5ms 打印 `th enter/exit` 把 115200 串口打满, 循环被拖慢到 ~11ms,
  alive 间隔变成 ~2.3s——调试结束后建议删掉这两行
- 触摸/屏幕显示内容尚需人工目视确认 (串口只能证明软件链路活着)
