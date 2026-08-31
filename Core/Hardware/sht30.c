#include "sht30.h"
#include "i2c.h"
#include "FreeRTOS.h"
#include "task.h"

uint8_t SHT30_ReadData(float *temp, float *humi)
{
    uint8_t buf[6];
    uint16_t t_raw, h_raw;

    //1.发送测量命令
    I2C_Start();
    I2C_SendByte(SHT30_ADDR << 1);   //地址+写(0)
    I2C_SendByte(0x24);   //命令：无时钟拉伸 低速
    I2C_SendByte(0x00);
    I2C_Stop();

    vTaskDelay(20);  //等待转换

    //2.读取数据
    I2C_Start();
    I2C_SendByte(SHT30_ADDR << 1 | 0x01);   //地址+读(1)

    buf[0] = I2C_ReadByte(1);
    buf[1] = I2C_ReadByte(1);
    buf[2] = I2C_ReadByte(1);  //校验和
    buf[3] = I2C_ReadByte(1);
    buf[4] = I2C_ReadByte(1);
    buf[5] = I2C_ReadByte(0);
    I2C_Stop();

    //3.计算真实值
    t_raw = (buf[0] << 8) | buf[1];
    h_raw = (buf[3] << 8) | buf[4];

    *temp = -45.0f + (175.0f * t_raw) / 65535.0f;
    *humi = 100.0f * h_raw / 65535.0f;

    return 0;
} 

