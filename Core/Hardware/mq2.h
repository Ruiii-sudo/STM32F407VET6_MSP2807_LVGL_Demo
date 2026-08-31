#ifndef __MQ2_H
#define __MQ2_H

#include "stm32f4xx_hal.h"

/************************* 硬件连接 *************************
 * MQ2 <-> STM32F407
 * MQ2 AO -> PC0 (ADC1_IN10)
 ************************************************************/

uint16_t MQ2_ReadAdc(void);

#endif
