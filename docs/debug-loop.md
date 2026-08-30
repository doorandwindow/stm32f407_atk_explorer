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
  - LVGL mem pool: 0x68000000 起 128 KB (lv_conf.h LV_MEM_ADR)
  - LVGL 渲染缓冲: 已迁至 CCM 0x10000000 (单缓冲 480×60×2B=57.6KB, 提速), 见 app/ports/lv_port_disp.c

## 串口参数
- 端口: COM18 (CH340)
- 波特率: 115200
- 数据位: 8, 停止: 1, 校验: none, 流控: none
- 默认拉低 DTR/RTS (复位不接管)

## 已知启动输出（dbg_printf 走 USART1）

`app/core/main.c` 在 main 开头打印复位源 (RCC->CSR) 并清标志。

预期正常启动输出 (2026-08-30 small_smart 移植 P4 起):
```
[dbg] RST cause 0x04000003: PIN      (烧录复位; IWDG 标志出现即说明发生过喂狗失败)
[smart] alarm started (5s patrol)
[ui] LCD ID=0x5510                   (NT35510)
[dbg] GT9147 CTP ID: 917S
[ui] GT9147 OK
[ui] pages ready, main created       (环境总览页创建完成)
[smart] tick started (1s beat)
[smart] periodic started (10ms loop, hw-stubbed)
[alm] #N ON/OFF @Ns                  (报警沿触发时; 稳态无输出)
[ui] go page N                       (触摸切页时: 1=Setup 2=报警 3=目标温度 4=关于)
```

串口周期输出只剩 `[mon]` 每 10s 四行 (cpu+heap / stk 逐任务栈水位 / iram+ccm / eram)。
历史任务输出 (keyLed/keyBright/dash/lvglTest) 已随 2026-08-30 P1 清场退役, 从 git 历史找回。

系统状态监控任务 (每 10s 四行, 含逐任务栈水位):
```
[mon] monitorTask started, period 10000ms
[mon] cpu 10.5% | heap(kb) total 40.0 used 16.7 free 23.3 peak_used 16.7 min_free 23.3
[mon] stk(b) default 924 monitor 1208 tick 456 periodic 1480 alarm 1472 ui 6356 idle 428
[mon] iram(kb) total 128.0 static 47.5 isr_stack 0.1 free 80.4 | ccm(kb) total 64.0 buf 56.3 free 7.8
[mon] eram(kb) total 1024.0 pool 128.0 used 16.5 lvgl_max 8.7
      cpu=整机忙占比; 内存字段单位 KB(0.1 定点, 全整数换算)
      heap = FreeRTOS 堆(heap_4); peak_used/min_free 是开机以来历史极值, 顶穿 40.0 会触发 [HOOK] 复位
      stk  = 逐任务栈高水位(字节): 剩余未写过空间, 裁栈/加任务以 ×2 峰值为准 (P1 起新增)
      iram = 主 SRAM 128KB: static=data+bss(含 40KB 堆池 ucHeap), isr_stack=中断栈水印
             (图案填充+从栈底向上扫描, 深于 1KB 保留区即危险), free=total-static-isr_stack
      ccm  = CCM 64KB: LVGL 渲染单缓冲 57.6KB 固定占用 (800x36 行, 横屏化后行数 60->36)
      eram = 外部 SRAM 1MB: LVGL 对象池 128KB@0x68000000 是唯一占用者; used=TLSF 池实时占用,
             lvgl_max=LVGL 分配记账峰值(仅请求字节), 口径不同 max<used 属正常
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
4. ~~MX_LWIP_Init 在 defaultTask 里同步调用~~ **已退役 (2026-08-30 P2)**: LwIP+ETH 整体出构建
   (外科手术式, 见 `cmake/stm32cubemx/CMakeLists.txt` 头部注释)。defaultTask 现为纯喂狗(1KB 栈)。
   ⚠ ioc 中 LwIP/ETH 配置仍保留: 若将来 CubeMX 重新生成, LwIP 会回到编译(无调用点, 仅 RAM 回退),
   需重做手术或在 CubeMX GUI 取消勾选 LwIP 中间件
5. HAL 时基走 TIM7, SysTick 给 FreeRTOS — 不冲突
6. ~~configCHECK_FOR_STACK_OVERFLOW=2 会破坏启动~~ **已证伪**: 之前"开检测就不能启动"是因为钩子
   尚未就位/配置半途而废; 现状检测全开 + 2KB 栈, 启动干净 (2026-08-27 两次 8-10s 抓串口验证)
7. **触摸 IC 实为 GT917S, 不是 GT9147** (2026-08-28 串口逐次诊断实测: 0x8140 读回 "917S")
   - 寄存器/时序与 GT9147 兼容, 驱动通用; 但 ID 校验必须同时接受 "9147"/"917S",
     否则 init 报 FAIL (扫描通路不查 ID, 触摸仍能用, 但会跳过软复位+量程修正)
   - **CT_INT(PB1) 在复位释放后不可推挽驱动** (哪怕 ~10ms): 会与 GT9147 的 INT 输出顶驱,
     模块亚健康。正确做法: INT 全程输入上拉, 只操作 RST, 地址 0x14 靠上拉高电平选出
   - 修复后量程修正带幂等保护 (已对则跳过写), 避免每次上电写 GT 配置 EEPROM 磨损
8. **FreeRTOS 堆 32KB→40KB (0xA000)** (2026-08-29): 加了 dashboardTask(4KB 栈)后 5 任务栈
   30KB(default 2 + lvgl 16 + keyLed 4 + keyBright 4 + dashboard 4) + idle/timer 栈 1.5KB + 7 个 TCB ≈ 32.1KB
   **超过 32KB 堆** → 最后分配(idle/timer 任务)失败 → 调度器不喂狗 → IWDG 每 ~2.1s 复位。
   现象: 串口只见 `[dash] dashbo...` 刷屏、lvgl/其它任务不打印、`RST cause 0x24000003: IWDG`。
   修复: `configTOTAL_HEAP_SIZE` 升到 0xA000(Inc/FreeRTOSConfig.h + config/FreeRTOSConfig.h 两处都要改)。
   ⚠ IWDG 复位后第一时间怀疑: 新加任务的栈 + TCB 是否把 32KB 堆顶穿。
9. ~~板端网络~~ **随 P2 拔除 LwIP 一并退役 (2026-08-30)**; 恢复网络时连同坑 4 一起回补。
10. **rtstats 时钟回卷窗口倒退竞态** (2026-08-30 P2 发现并修复): TIM11 16 位计数器 65.5ms 回卷一次,
    CNT 已回卷但溢出 ISR(优先级 15, 可被 BASEPRI 屏蔽延迟)尚未执行时读时钟, 读数倒退最多 65.5ms;
    FreeRTOS 用它算任务运行差值会下溢, `[mon] cpu` 打出 42936.6% 一类荒谬值。
    修复: `rtstats_rtclock_get()` 检测 UIF 挂起且 CNT<0x8000 时补偿 +1 个高位 (见 bsp/components/rtstats/)。
11. **横屏 800x480 的方向配套** (P4.0): `lcd.h LCD_SCAN_MODE`(0x60 候选A / 0xA0 候选B, 倒 180° 时互换)
    与 `lv_port_indev.c TP_MAP_FLIP`(1/0) 必须成组切换; 只换一个会出现"画面正但触摸镜像错位"。
    CCM 渲染缓冲横屏后为 800x36 行(仍 57.6KB), 行数在 `lv_port_disp.c DISP_BUF_LINES`。
12. **中文字库再生成** (P4 起): 文案字符集真源是 `app/ui/smart_pages/smart_text.h`(75 字),
    新增中文文案后需重新生成字库, 否则新字符显示为空:
    ```bash
    node -e "..."   # 从 smart_text.h 提取唯一非 ASCII 字符 -> build/font_charset.txt
    npx --yes lv_font_conv --no-compress --bpp 4 --size 16 --format lvgl --lv-include lvgl.h \
      --lv-fallback lv_font_montserrat_20 --font C:/Windows/Fonts/simhei.ttf \
      -r 0x20-0x7E -r 0xB0 --symbols "$(cat build/font_charset.txt)" -o app/ui/fonts/smart_cn_16.c
    # 24px 同理 (smart_cn_24.c); 注意 -r 必须放在 --font 之后; LV_SYMBOL_* 走 montserrat_20 回退
    ```

## RAM 布局速查 (build/Debug/CubeMX_Config.map, 2026-08-30 P2 后, -O2)

```
ucHeap        0x20001004  0xA000  (FreeRTOS 堆 40KB, 任务栈从这里分配; [mon] 实时水位)
_sdata        0x20000000          (data+bss 起点, P2 后 static≈48.6KB 由 [mon] 实时打印)
_ebss         ≈0x2000C1xx         (静态区结束; 主 SRAM 剩余 ~80KB)
_sstack       0x2001FC00          (MSP 中断栈底, 保留 1KB; [mon] isr_stack 实时水印)
_estack       0x20020000          (MSP 顶; CM4F 端口切换后 MSP 常驻 _estack-0x20 属正常)
CCM           0x10000000  0x10000 (LVGL 渲染单缓冲 57.6KB=800x36 行, [mon] ccm 实时打印)
外部 SRAM     0x68000000  0x100000 (LVGL 对象池 128KB@基址, [mon] eram 实时水位)
```

P1/P2 清场账 (2026-08-30): 静态区 114.8→47.5KB (退役 LwIP ~64KB + demo/服务 ~3KB),
堆占用 35.9→16.7KB (6 任务栈 17KB + TCB); 当前余量: 主 SRAM ~80KB / 堆 min_free ~23KB / Flash ~75%。

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
