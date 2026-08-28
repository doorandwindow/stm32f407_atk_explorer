/**
  ******************************************************************************
  * @file    gt9147.c
  * @brief   GT9147/GT917S 电容触摸驱动（正点原子探索者 V2.2 4.3寸屏）
  *
  * 模拟 I2C: CT_SCL=PB0 / CT_SDA=PF11 / CT_RST=PC13 / CT_INT=PB1
  * 寄存器: 0x8040 控制 / 0x8047 配置 / 0x8140 产品ID / 0x814E 状态 / 0x8150 坐标
  * [实测] 本屏 IC 为 GT917S (0x8140="917S"), 寄存器兼容 GT9147, 驱动通用
  * 参考: 正点原子探索者例程《实验28 触摸屏实验》GT9147 部分
  ******************************************************************************
  */
#include "gt9147.h"
#include "uart_dbg.h"

/* ---- 模拟 I2C 引脚操作 ---- */
#define GT_SCL_H()  HAL_GPIO_WritePin(GT_SCL_PORT, GT_SCL_PIN, GPIO_PIN_SET)
#define GT_SCL_L()  HAL_GPIO_WritePin(GT_SCL_PORT, GT_SCL_PIN, GPIO_PIN_RESET)
#define GT_SDA_H()  HAL_GPIO_WritePin(GT_SDA_PORT, GT_SDA_PIN, GPIO_PIN_SET)
#define GT_SDA_L()  HAL_GPIO_WritePin(GT_SDA_PORT, GT_SDA_PIN, GPIO_PIN_RESET)
#define GT_SDA_READ()  HAL_GPIO_ReadPin(GT_SDA_PORT, GT_SDA_PIN)

static void GT_IIC_Delay(void)
{
    /* GT9147 I2C 上限 400kHz: 168MHz 下空转 8 次≈1.4MHz 严重超标,
       上拉电阻来不及拉起 SDA → 偶发读坏字节(坐标跳变)。60 次≈250kHz */
    volatile uint32_t i = 60;
    while (i--);
}

static void GT_SDA_Output(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GT_SDA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GT_SDA_PORT, &GPIO_InitStruct);
}

static void GT_SDA_Input(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GT_SDA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GT_SDA_PORT, &GPIO_InitStruct);
}

/* ---- I2C 时序 ---- */
static void GT_IIC_Start(void)
{
    GT_SDA_Output();
    GT_SDA_H();
    GT_SCL_H();
    GT_IIC_Delay();
    GT_SDA_L();
    GT_IIC_Delay();
    GT_SCL_L();
}

static void GT_IIC_Stop(void)
{
    GT_SDA_Output();
    GT_SDA_L();
    GT_SCL_H();
    GT_IIC_Delay();
    GT_SDA_H();
    GT_IIC_Delay();
}

static uint8_t GT_IIC_WaitAck(void)
{
    /* 对齐官方 ctiic.c: 每次轮询带延时, 总超时 ~500us。
       GT9147 保存配置到内部 flash 期间会持续 NACK, 空转轮询的 ~40us 超时误判率高 */
    uint16_t timeout = 250;
    GT_SDA_Input();
    GT_IIC_Delay();
    GT_SCL_H();
    GT_IIC_Delay();
    while (GT_SDA_READ())
    {
        GT_IIC_Delay();
        if (--timeout == 0)
        {
            GT_SCL_L();
            return 1;   /* 无 ACK */
        }
    }
    GT_SCL_L();
    return 0;
}

static void GT_IIC_Ack(void)
{
    GT_SDA_Output();
    GT_SDA_L();
    GT_IIC_Delay();
    GT_SCL_H();
    GT_IIC_Delay();
    GT_SCL_L();
}

static void GT_IIC_NAck(void)
{
    GT_SDA_Output();
    GT_SDA_H();
    GT_IIC_Delay();
    GT_SCL_H();
    GT_IIC_Delay();
    GT_SCL_L();
}

static void GT_IIC_SendByte(uint8_t data)
{
    uint8_t i;
    GT_SDA_Output();
    GT_SCL_L();
    for (i = 0; i < 8; i++)
    {
        if (data & 0x80)
            GT_SDA_H();
        else
            GT_SDA_L();
        data <<= 1;
        GT_IIC_Delay();
        GT_SCL_H();
        GT_IIC_Delay();
        GT_SCL_L();
        GT_IIC_Delay();
    }
}

static uint8_t GT_IIC_RecvByte(void)
{
    uint8_t i, data = 0;
    GT_SDA_Input();
    for (i = 0; i < 8; i++)
    {
        GT_SCL_L();
        GT_IIC_Delay();
        GT_SCL_H();
        GT_IIC_Delay();
        data <<= 1;
        if (GT_SDA_READ())
            data |= 0x01;
    }
    GT_SCL_L();
    return data;
}

/* ---- GT9147 寄存器读写（16 位寄存器地址） ---- */
static uint8_t GT9147_WR_Reg(uint16_t reg, uint8_t val)
{
    uint8_t buf[3];
    buf[0] = reg >> 8;
    buf[1] = reg & 0xFF;
    buf[2] = val;

    GT_IIC_Start();
    GT_IIC_SendByte(GT_ADDR_W);
    if (GT_IIC_WaitAck()) goto fail;
    GT_IIC_SendByte(buf[0]);
    if (GT_IIC_WaitAck()) goto fail;
    GT_IIC_SendByte(buf[1]);
    if (GT_IIC_WaitAck()) goto fail;
    GT_IIC_SendByte(buf[2]);
    if (GT_IIC_WaitAck()) goto fail;
    GT_IIC_Stop();
    return 0;

fail:
    GT_IIC_Stop();
    return 1;
}

static uint8_t GT9147_RD_Reg(uint16_t reg, uint8_t *val)
{
    if (val == NULL) return 1;

    GT_IIC_Start();
    GT_IIC_SendByte(GT_ADDR_W);
    if (GT_IIC_WaitAck()) goto fail;
    GT_IIC_SendByte(reg >> 8);
    if (GT_IIC_WaitAck()) goto fail;
    GT_IIC_SendByte(reg & 0xFF);
    if (GT_IIC_WaitAck()) goto fail;
    GT_IIC_Start();
    GT_IIC_SendByte(GT_ADDR_R);
    if (GT_IIC_WaitAck()) goto fail;
    *val = GT_IIC_RecvByte();
    GT_IIC_NAck();
    GT_IIC_Stop();
    return 0;

fail:
    GT_IIC_Stop();
    return 1;
}

static uint8_t GT9147_WR_Regs(uint16_t reg, const uint8_t *buf, uint16_t len)
{
    uint16_t i;

    if (buf == NULL || len == 0) return 1;

    GT_IIC_Start();
    GT_IIC_SendByte(GT_ADDR_W);
    if (GT_IIC_WaitAck()) goto fail;
    GT_IIC_SendByte(reg >> 8);
    if (GT_IIC_WaitAck()) goto fail;
    GT_IIC_SendByte(reg & 0xFF);
    if (GT_IIC_WaitAck()) goto fail;
    for (i = 0; i < len; i++)
    {
        GT_IIC_SendByte(buf[i]);
        if (GT_IIC_WaitAck()) goto fail;
    }
    GT_IIC_Stop();
    return 0;

fail:
    GT_IIC_Stop();
    return 1;
}

static uint8_t GT9147_RD_Regs(uint16_t reg, uint8_t *buf, uint16_t len)
{
    uint16_t i;

    if (buf == NULL || len == 0) return 1;

    GT_IIC_Start();
    GT_IIC_SendByte(GT_ADDR_W);
    if (GT_IIC_WaitAck()) goto fail;
    GT_IIC_SendByte(reg >> 8);
    if (GT_IIC_WaitAck()) goto fail;
    GT_IIC_SendByte(reg & 0xFF);
    if (GT_IIC_WaitAck()) goto fail;
    GT_IIC_Start();
    GT_IIC_SendByte(GT_ADDR_R);
    if (GT_IIC_WaitAck()) goto fail;
    for (i = 0; i < len; i++)
    {
        buf[i] = GT_IIC_RecvByte();
        if (i == len - 1)
            GT_IIC_NAck();
        else
            GT_IIC_Ack();
    }
    GT_IIC_Stop();
    return 0;

fail:
    GT_IIC_Stop();
    return 1;
}

/**
  * @brief GT9147 初始化
  * @retval 1=成功 0=失败
  */
uint8_t GT9147_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    uint8_t pid[4] = {0};
    /* 总线在复位释放后 ~120ms 即可正常应答 (2026-08-28 实测首次尝试即通),
       3 次重试兜底足够; 失败会跳过量程修正, 不能省 */
    uint8_t retry = 3;

    /* ---- GPIO 时钟（CubeMX 已使能 GPIOC/F/B, 这里兜底） ---- */
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* CT_SCL: 推挽输出 */
    GPIO_InitStruct.Pin = GT_SCL_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GT_SCL_PORT, &GPIO_InitStruct);

    /* CT_RST: 推挽输出 */
    GPIO_InitStruct.Pin = GT_RST_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GT_RST_PORT, &GPIO_InitStruct);

    /* CT_INT: 输入上拉, 全程不驱动 (对齐官方例程)。
       [坑] 2026-08-28 上板实测: strap 后保持推挽输出高哪怕 ~10ms, 也会与 GT9147
       复位释放后的 INT 输出驱动顶驱, 模块被打成亚健康 (I2C NACK 持续 0.4~0.7s+,
       PID 重试窗口全错过)。地址 0x14 只靠内部上拉的高电平即可选出, 无需驱动 INT */
    GPIO_InitStruct.Pin = GT_INT_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GT_INT_PORT, &GPIO_InitStruct);

    /* CT_SDA: 开漏输出初始 */
    GT_SDA_Output();

    /* ---- 复位时序 (对齐官方): INT 保持输入上拉(高)=> 地址 0x14, 只操作 RST ---- */
    HAL_GPIO_WritePin(GT_RST_PORT, GT_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(GT_RST_PORT, GT_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(10);

    /* 复位释放后等 100ms 再访问 I2C (Goodix 手册 ~50ms; 过早访问必 NACK) */
    HAL_Delay(100);

    /* ---- 读产品 ID 验证通信 ----
       [实测 2026-08-28] 本屏触摸 IC 实际为 GT917S (0x8140 读回 "917S"),
       寄存器与 GT9147 兼容; 官方例程 strcmp("9147") 在这批屏上同样匹配不到。
       两种 ID 都接受 */
    {
        uint8_t pid_ok = 0;
        while (retry-- && !pid_ok)
        {
            pid_ok = (GT9147_RD_Regs(GT_REG_PID, pid, 4) == 0 &&
                      ((pid[0] == '9' && pid[1] == '1' &&
                        pid[2] == '4' && pid[3] == '7') ||
                       (pid[0] == '9' && pid[1] == '1' &&
                        pid[2] == '7' && pid[3] == 'S')));
            if (!pid_ok)
                HAL_Delay(50);
        }
        if (!pid_ok)
        {
            return 0;   /* 通信失败: 重试窗口内仍读不到合法 ID */
        }
        dbg_printf("[dbg] GT9147 CTP ID: %c%c%c%c\r\n",
                   pid[0], pid[1], pid[2], pid[3]);
    }

    /* ---- 软复位 + 结束复位 ---- */
    if (GT9147_WR_Reg(GT_REG_CMD, 2) != 0)
        return 0;
    HAL_Delay(10);
    if (GT9147_WR_Reg(GT_REG_CMD, 0) != 0)
        return 0;

    /* ---- 修正坐标几何: 模块出厂配置量程/轴向不规范, 实测坐标高低字节随机混叠 ±256 跳变。
       读出配置页 → 强制 X_Output_Max=480 / Y_Output_Max=800 (与竖屏显示一致, 低字节在前)
       → 重算校验和写回 → 置 config_fresh 让 GT9147 重载 (对齐正点原子例程做法) ---- */
    {
        uint8_t cfg[184];   /* 0x8047 ~ 0x80FE */
        uint8_t sum = 0;
        uint16_t i;
        if (GT9147_RD_Regs(GT_REG_CFG, cfg, 184) != 0)
            return 0;
        if (cfg[1] == (480 & 0xFF) && cfg[2] == (480 >> 8) &&
            cfg[3] == (800 & 0xFF) && cfg[4] == (800 >> 8))
        {
            /* 量程已正确 (config_fresh=1 会存入片内 EEPROM, 只需写一次):
               跳过写入, 避免每次上电磨损配置 EEPROM */
        }
        else
        {
            cfg[1] = 480 & 0xFF;   cfg[2] = 480 >> 8;   /* X_Output_Max @0x8048/49 */
            cfg[3] = 800 & 0xFF;   cfg[4] = 800 >> 8;   /* Y_Output_Max @0x804A/4B */
            for (i = 0; i < 184; i++) sum += cfg[i];
            if (GT9147_WR_Regs(GT_REG_CFG, cfg, 184) != 0)
                return 0;
            if (GT9147_WR_Reg(0x80FF, (uint8_t)(0u - sum)) != 0) /* 校验和独立寄存器 */
                return 0;
            if (GT9147_WR_Reg(0x8100, 1) != 0)                   /* config_fresh */
                return 0;
            HAL_Delay(200);
        }
    }

    return 1;
}

/**
  * @brief 扫描触摸点（单点, LVGL 使用）
  * @param point 输出坐标
  * @retval 1=有触摸 0=无
  */
GT9147_ScanResult_t GT9147_Scan(GT_Point_t *point)
{
    uint8_t status;
    uint8_t buf[6] = {0};
    uint8_t points;
    GT9147_ScanResult_t result;

    if (GT9147_RD_Reg(GT_REG_STATUS, &status) != 0)
        return GT9147_SCAN_ERROR;
    if ((status & 0x80) == 0)          /* buffer 无新数据: 保持上次状态 */
    {
        return GT9147_SCAN_NO_DATA;
    }

    points = status & 0x0F;
    if (points > 0 && points < 6)      /* 对齐官方: 只认 1~5 点 */
    {
        if (point == NULL || GT9147_RD_Regs(GT_REG_POINT1, buf, 6) != 0)
        {
            /* 先清状态再报错: bit7 不清会卡死在 ERROR 循环, 丢一帧好过死循环 */
            GT9147_WR_Reg(GT_REG_STATUS, 0);
            return GT9147_SCAN_ERROR;
        }
        /* 本模块固件坐标布局 (2026-08-28 字节流实测确认, 与常见 GT9 参考不同——无 track id):
           0x8150=XL, 0x8151=XH, 0x8152=YL, 0x8153=YH, 0x8154=size, 0x8155=保留
           实测轨迹: x 191→352 连续, y 720→790 连续 */
        point->x = ((uint16_t)buf[1] << 8) | buf[0];
        point->y = ((uint16_t)buf[3] << 8) | buf[2];
        result = GT9147_SCAN_PRESSED;
    }
    else if (points == 0)
    {
        result = GT9147_SCAN_RELEASED; /* 抬起帧: 仅此一帧带 0x80 标志, 必须捕获 */
    }
    else
    {
        result = GT9147_SCAN_NO_DATA;  /* >5 点: 状态字节受干扰, 不产生事件 */
    }

    if (GT9147_WR_Reg(GT_REG_STATUS, 0) != 0) /* 读后必须清状态 */
        return GT9147_SCAN_ERROR;
    return result;
}
