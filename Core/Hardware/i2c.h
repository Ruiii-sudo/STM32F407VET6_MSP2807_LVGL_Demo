#ifndef __I2C_H
#define __I2C_H

#include "stm32f4xx_hal.h"

#define I2C_GPIO_Port GPIOB
#define I2C_SCL_Pin GPIO_PIN_6
#define I2C_SDA_Pin GPIO_PIN_7

void I2C_Start(void);
void I2C_Stop(void);
uint8_t I2C_WaitAck(void);
void I2C_SendByte(uint8_t byte);
uint8_t I2C_ReadByte(uint8_t ack);

#endif
