/**
  ******************************************************************************
  * @file    lcd.c
  * @brief   TFTLCD 驱动（正点原子探索者 V2.2 / NT35510 480x800 竖屏）
  *
  * 接口: FSMC Bank1 NE4(PG12) + A6(PF12, RS) + D[15:0], 16bit 8080 并口
  * 移植参考: 正点原子探索者官方例程 + RT-Thread stm32f407-atk-explorer BSP
  * 说明: NT35510 采用 16 位命令(如 0x2A00/0x2C00), 与 9341 等 8 位命令不同
  *
  * DMA 加速: 使用 DMA2_Stream0 (通道0, 内存到内存) 加速 FSMC 写入
  ******************************************************************************
  */
#include "lcd.h"

/* DMA 传输状态标志 */
static volatile uint8_t lcd_dma_busy = 0;
#define LCD_DMA_TIMEOUT_MS 50U
static uint16_t lcd_dma_x1;
static uint16_t lcd_dma_y1;
static uint16_t lcd_dma_x2;
static uint16_t lcd_dma_y2;
static const uint16_t *lcd_dma_buf;
static uint32_t lcd_dma_start_tick;
static uint32_t lcd_dma_fallback_count;

/* NT35510 初始化寄存器表 {reg, data}（381 对, 提取自正点原子探索者例程） */
static const struct { uint16_t reg; uint16_t data; } nt35510_init_table[] = {
#include "lcd_nt35510_init.inc"
};

/* ---- LCD 寄存器映射（FSMC NE4 + A6 做 RS） ---- */
#define LCD_BASE ((uint32_t)(0x6C000000 | 0x0000007E))
typedef struct
{
    volatile uint16_t LCD_REG;   /* RS=0: 命令 */
    volatile uint16_t LCD_RAM;   /* RS=1: 数据 */
} LCD_TypeDef;
#define LCD ((LCD_TypeDef *)LCD_BASE)

/* NT35510 命令（16 位） */
#define NT_CMD_SETX   0x2A00
#define NT_CMD_SETY   0x2B00
#define NT_CMD_WRAM   0x2C00

static uint16_t lcd_id = 0;

/* ---- 底层读写 ---- */
static void LCD_WR_REG(uint16_t reg)    { LCD->LCD_REG = reg; }
static void LCD_WR_DATA(uint16_t data)  { LCD->LCD_RAM = data; }
static uint16_t LCD_RD_DATA(void)       { return LCD->LCD_RAM; }
static void LCD_WriteReg(uint16_t reg, uint16_t val)
{
    LCD->LCD_REG = reg;
    LCD->LCD_RAM = val;
}

/**
  * @brief FSMC Bank4(NE4) + LCD 相关 GPIO 配置
  * @note  CubeMX 已配置外部 SRAM(Bank3 NE3), 这里补 LCD 的 Bank4 片选与背光
  */
static void LCD_FSMC_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_FSMC_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* PG12 = FSMC_NE4（LCD 片选） */
    GPIO_InitStruct.Pin = LCD_CS_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_FSMC;
    HAL_GPIO_Init(LCD_CS_GPIO_PORT, &GPIO_InitStruct);

    /* PB15 = LCD 背光（探索者上与 SPI2_MOSI 复用, 官方设计如此） */
    GPIO_InitStruct.Pin = LCD_BL_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(LCD_BL_GPIO_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(LCD_BL_GPIO_PORT, LCD_BL_PIN, GPIO_PIN_SET);   /* 背光亮 */

    /* FSMC Bank4 控制器（NOR/SRAM 模式, 16bit） */
    SRAM_HandleTypeDef hsram = {0};
    FSMC_NORSRAM_TimingTypeDef Timing = {0};

    hsram.Instance = FSMC_NORSRAM_DEVICE;
    hsram.Extended = FSMC_NORSRAM_EXTENDED_DEVICE;
    hsram.Init.NSBank = FSMC_NORSRAM_BANK4;
    hsram.Init.DataAddressMux = FSMC_DATA_ADDRESS_MUX_DISABLE;
    hsram.Init.MemoryType = FSMC_MEMORY_TYPE_SRAM;
    hsram.Init.MemoryDataWidth = FSMC_NORSRAM_MEM_BUS_WIDTH_16;
    hsram.Init.BurstAccessMode = FSMC_BURST_ACCESS_MODE_DISABLE;
    hsram.Init.WaitSignalPolarity = FSMC_WAIT_SIGNAL_POLARITY_LOW;
    hsram.Init.WrapMode = FSMC_WRAP_MODE_DISABLE;
    hsram.Init.WaitSignalActive = FSMC_WAIT_TIMING_BEFORE_WS;
    hsram.Init.WriteOperation = FSMC_WRITE_OPERATION_ENABLE;
    hsram.Init.WaitSignal = FSMC_WAIT_SIGNAL_DISABLE;
    hsram.Init.ExtendedMode = FSMC_EXTENDED_MODE_DISABLE;
    hsram.Init.AsynchronousWait = FSMC_ASYNCHRONOUS_WAIT_DISABLE;
    hsram.Init.WriteBurst = FSMC_WRITE_BURST_DISABLE;
    hsram.Init.PageSize = FSMC_PAGE_SIZE_NONE;

    /* NT35510 写周期 >=15ns, 读周期 >=45ns, 168MHz HCLK = 6ns. */
    Timing.AddressSetupTime = 3;      /* 地址建立: 3 HCLK = 18ns */
    Timing.AddressHoldTime = 0;       /* 地址保持: 0 HCLK (模式A不用) */
    Timing.DataSetupTime = 3;         /* 数据建立: 3 HCLK = 18ns */
    Timing.BusTurnAroundDuration = 0;
    Timing.CLKDivision = 2;
    Timing.DataLatency = 2;
    Timing.AccessMode = FSMC_ACCESS_MODE_A;

    if (HAL_SRAM_Init(&hsram, &Timing, &Timing) != HAL_OK)
    {
        Error_Handler();
    }

    /* ===== 初始化 DMA2_Stream0 用于 LCD 数据传输加速 ===== */
    __HAL_RCC_DMA2_CLK_ENABLE();

    DMA2_Stream0->CR = 0;  /* 复位 */
    while (DMA2_Stream0->CR & DMA_SxCR_EN);  /* 等待 DMA 禁用 */

    /*
     * FSMC LCD 没有可用的 DMA 外设请求，使用 DMA 内存到内存模式触发传输。
     * 在该模式下 PAR 作为递增源地址，M0AR 作为固定目标地址。
     */
    DMA2_Stream0->CR = (0U << DMA_SxCR_CHSEL_Pos)    /* 通道 0 */
                     | DMA_SxCR_DIR_1                /* 内存到内存: PAR -> M0AR */
                     | (1U << DMA_SxCR_PL_Pos)       /* 优先级: 中 */
                     | (1U << DMA_SxCR_MSIZE_Pos)    /* 内存数据宽度: 16bit */
                     | (1U << DMA_SxCR_PSIZE_Pos)    /* 外设数据宽度: 16bit */
                     | DMA_SxCR_PINC;                /* PAR(源)地址递增 */

    DMA2_Stream0->FCR = DMA_SxFCR_DMDIS;  /* 禁用直接模式，使用 FIFO */
    DMA2_Stream0->FCR |= (3U << DMA_SxFCR_FTH_Pos);  /* FIFO 阈值: 全满 */

    /* Completion is polled from the LVGL task.  An IRQ would clear TCIF
       before the task can observe it and force the slow CPU fallback. */
}

/**
  * @brief 读取 LCD 控制器 ID
  * @retval 0x5510: NT35510; 其他: 未识别
  */
uint16_t LCD_ReadID(void)
{
    uint16_t id = 0;

    /* NT35510 读 ID: 0xDA00->0x00, 0xDB00->0x80, 0xDC00->0x00 (RT-Thread 探索者 BSP 口径) */
    LCD_WR_REG(0xDA00);
    id = LCD_RD_DATA();               /* 0x00 */
    LCD_WR_REG(0xDB00);
    id = LCD_RD_DATA();               /* 0x80 */
    id <<= 8;
    LCD_WR_REG(0xDC00);
    id |= LCD_RD_DATA();              /* 0x00 */
    if (id == 0x8000)
    {
        id = 0x5510;                  /* NT35510 */
    }
    return id;
}

/**
  * @brief LCD 初始化: FSMC 配置 -> 读 ID -> NT35510 初始化序列
  */
void LCD_Init(void)
{
    uint16_t i;

    LCD_FSMC_Config();
    HAL_Delay(50);

    lcd_id = LCD_ReadID();

    /* NT35510 初始化序列（寄存器表在 lcd_nt35510_init.inc） */
    for (i = 0; i < (sizeof(nt35510_init_table) / sizeof(nt35510_init_table[0])); i++)
    {
        LCD_WriteReg(nt35510_init_table[i].reg, nt35510_init_table[i].data);
    }

    /* 扫描方向: 竖屏 L2R_U2D, NT35510 不需要 BGR 位 */
    LCD_WriteReg(0x3600, 0x00);

    /* 写窗口范围（全屏） */
    LCD_WR_REG(NT_CMD_SETX);
    LCD_WR_DATA(0);
    LCD_WR_REG(NT_CMD_SETX + 1);
    LCD_WR_DATA(0);
    LCD_WR_REG(NT_CMD_SETX + 2);
    LCD_WR_DATA((LCD_W - 1) >> 8);
    LCD_WR_REG(NT_CMD_SETX + 3);
    LCD_WR_DATA((LCD_W - 1) & 0xFF);
    LCD_WR_REG(NT_CMD_SETY);
    LCD_WR_DATA(0);
    LCD_WR_REG(NT_CMD_SETY + 1);
    LCD_WR_DATA(0);
    LCD_WR_REG(NT_CMD_SETY + 2);
    LCD_WR_DATA((LCD_H - 1) >> 8);
    LCD_WR_REG(NT_CMD_SETY + 3);
    LCD_WR_DATA((LCD_H - 1) & 0xFF);

    /* Sleep Out -> Display On */
    LCD_WR_REG(0x1100);
    HAL_Delay(120);
    LCD_WR_REG(0x2900);
}

/**
  * @brief 设置写窗口
  */
void LCD_SetWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    LCD_WR_REG(NT_CMD_SETX);
    LCD_WR_DATA(x1 >> 8);
    LCD_WR_REG(NT_CMD_SETX + 1);
    LCD_WR_DATA(x1 & 0xFF);
    LCD_WR_REG(NT_CMD_SETX + 2);
    LCD_WR_DATA(x2 >> 8);
    LCD_WR_REG(NT_CMD_SETX + 3);
    LCD_WR_DATA(x2 & 0xFF);

    LCD_WR_REG(NT_CMD_SETY);
    LCD_WR_DATA(y1 >> 8);
    LCD_WR_REG(NT_CMD_SETY + 1);
    LCD_WR_DATA(y1 & 0xFF);
    LCD_WR_REG(NT_CMD_SETY + 2);
    LCD_WR_DATA(y2 >> 8);
    LCD_WR_REG(NT_CMD_SETY + 3);
    LCD_WR_DATA(y2 & 0xFF);
}

void LCD_WriteRAMPrepare(void)
{
    LCD_WR_REG(NT_CMD_WRAM);
}

void LCD_WriteRAM(uint16_t color)
{
    LCD->LCD_RAM = color;
}

/**
  * @brief 清屏
  */
void LCD_Clear(uint16_t color)
{
    uint32_t total = (uint32_t)LCD_W * LCD_H;
    LCD_SetWindow(0, 0, LCD_W - 1, LCD_H - 1);
    LCD_WriteRAMPrepare();
    while (total--)
    {
        LCD->LCD_RAM = color;
    }
}

/**
  * @brief 批量填充区域（用于 LVGL flush 回调）
  * @param x1,y1,x2,y2 区域（含端点）
  * @param buf  RGB565 像素数据, 长度 = (x2-x1+1)*(y2-y1+1)
  */
void LCD_FillRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
                  const uint16_t *buf)
{
    uint32_t total = (uint32_t)(x2 - x1 + 1) * (y2 - y1 + 1);
    LCD_SetWindow(x1, y1, x2, y2);
    LCD_WriteRAMPrepare();
    while (total--)
    {
        LCD->LCD_RAM = *buf++;
    }
}

/**
  * @brief DMA 加速批量填充区域（性能提升 3-5 倍）
  * @param x1,y1,x2,y2 区域（含端点）
  * @param buf  RGB565 像素数据
  */
void LCD_FillRect_DMA(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
                      const uint16_t *buf)
{
    uint32_t total = (uint32_t)(x2 - x1 + 1) * (y2 - y1 + 1);

    /* 等待上次 DMA 完成 */
    while (lcd_dma_busy);

    LCD_SetWindow(x1, y1, x2, y2);
    LCD_WriteRAMPrepare();

    lcd_dma_x1 = x1;
    lcd_dma_y1 = y1;
    lcd_dma_x2 = x2;
    lcd_dma_y2 = y2;
    lcd_dma_buf = buf;
    lcd_dma_start_tick = HAL_GetTick();

    /* 配置并启动 DMA */
    lcd_dma_busy = 1;
    DMA2_Stream0->CR &= ~DMA_SxCR_EN;  /* 先禁用 */
    while (DMA2_Stream0->CR & DMA_SxCR_EN);

    DMA2->LIFCR = 0x3F;  /* 清除所有 Stream0 标志 */

    DMA2_Stream0->NDTR = total;                    /* 传输数量 */
    DMA2_Stream0->PAR = (uint32_t)buf;             /* 源: 像素缓冲区 */
    DMA2_Stream0->M0AR = (uint32_t)&(LCD->LCD_RAM);/* 目标: LCD 数据寄存器 */

    DMA2_Stream0->CR |= DMA_SxCR_EN;  /* 启动 DMA */
}

/**
  * @brief 检查 DMA 是否忙
  */
uint8_t LCD_DMA_IsBusy(void)
{
    return lcd_dma_busy;
}

uint8_t LCD_DMA_Poll(void)
{
    uint32_t flags;

    if (!lcd_dma_busy) return 1;
    flags = DMA2->LISR;
    if (flags & DMA_LISR_TCIF0)
    {
        DMA2_Stream0->CR &= ~DMA_SxCR_EN;
        DMA2->LIFCR = 0x3F;
        lcd_dma_busy = 0;
        return 1;
    }

    if ((flags & (DMA_LISR_TEIF0 | DMA_LISR_DMEIF0 | DMA_LISR_FEIF0)) ||
        (uint32_t)(HAL_GetTick() - lcd_dma_start_tick) > LCD_DMA_TIMEOUT_MS)
    {
        DMA2_Stream0->CR &= ~DMA_SxCR_EN;
        while (DMA2_Stream0->CR & DMA_SxCR_EN) { }
        DMA2->LIFCR = 0x3F;
        lcd_dma_busy = 0;
        lcd_dma_fallback_count++;
        LCD_FillRect(lcd_dma_x1, lcd_dma_y1, lcd_dma_x2, lcd_dma_y2,
                     lcd_dma_buf);
        return 1;
    }

    return 0;
}

uint32_t LCD_DMA_GetFallbackCount(void)
{
    return lcd_dma_fallback_count;
}

/**
  * @brief 等待 DMA 完成
  */
void LCD_DMA_Wait(void)
{
    uint32_t start = HAL_GetTick();

    while (lcd_dma_busy)
    {
        if (LCD_DMA_Poll()) break;
        if ((HAL_GetTick() - start) > LCD_DMA_TIMEOUT_MS)
        {
            (void)LCD_DMA_Poll();
            break;
        }
    }
}

/**
  * @brief DMA2_Stream0 中断处理（传输完成）
  */
void DMA2_Stream0_IRQHandler(void)
{
    if (DMA2->LISR & DMA_LISR_TCIF0)  /* 传输完成 */
    {
        DMA2->LIFCR = DMA_LIFCR_CTCIF0;  /* 清除标志 */
        /* Completion is consumed by LCD_DMA_Poll() in task context. */
    }
    if (DMA2->LISR & DMA_LISR_TEIF0)  /* 传输错误 */
    {
        DMA2->LIFCR = DMA_LIFCR_CTEIF0;
        /* Error is consumed by LCD_DMA_Poll() in task context. */
    }
}
