#ifndef __SHT30_H
#define __SHT30_H

#include "stm32f4xx_hal.h"

/************************* 硬件连接 *************************
 * SHT30 <-> STM32F407
 * SHT30 SDA -> PB7 (I2C1_SDA)
 * SHT30 SCL -> PB6 (I2C1_SCL)
 ************************************************************/

#define SHT30_ADDR          0x44  

uint8_t SHT30_ReadData(float *temp, float *humi);

#endif
