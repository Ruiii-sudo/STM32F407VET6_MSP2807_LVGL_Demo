#include "bh1750.h"
#include "i2c.h"
#include "FreeRTOS.h"
#include "task.h"

/**
 * @brief  BH1750 初始化（上电 + 设置连续高分辨率模式）
 * @retval 0=成功
 */
uint8_t BH1750_Init(void)
{
    /* 1. 上电 */
    I2C_Start();
    I2C_SendByte(BH1750_ADDR << 1);      /* 写地址 */
    I2C_SendByte(BH1750_POWER_ON);       /* 上电命令 */
    I2C_Stop();
    vTaskDelay(10);

    /* 2. 设置连续高分辨率模式（1lx 精度，120ms 转换时间） */
    I2C_Start();
    I2C_SendByte(BH1750_ADDR << 1);
    I2C_SendByte(BH1750_CONT_H_RES);
    I2C_Stop();
    vTaskDelay(120);  /* 等待第一次转换完成 */

    return 0;
}

/**
 * @brief  读取光照强度
 * @param  light: 输出光照值，单位 lux（整数）
 * @retval 0=成功
 * @note   BH1750 输出公式：光照 = 原始值 / 1.2
 */
uint16_t BH1750_ReadLight(void)
{
    uint8_t  buf[2];

    /* 连续模式下直接读取 2 字节数据（高字节在前） */
    I2C_Start();
    I2C_SendByte((BH1750_ADDR << 1) | 0x01);  /* 读地址 */
    buf[0] = I2C_ReadByte(1);   /* 高字节，主机应答 ACK */
    buf[1] = I2C_ReadByte(0);   /* 低字节，主机应答 NACK */
    I2C_Stop();

    uint16_t raw = ((uint16_t)buf[0] << 8) | buf[1];
    uint16_t light = (uint16_t)((float)raw / 1.2f);

    return light;
}

