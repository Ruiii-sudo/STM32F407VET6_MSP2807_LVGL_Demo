#ifndef __BH1750_H
#define __BH1750_H

#include "stm32f4xx_hal.h"

/************************* 硬件连接 *************************
 * BH1750 <-> STM32F407
 * BH1750 SDA -> PB7 (I2C1_SDA)
 * BH1750 SCL -> PB6 (I2C1_SCL)
 ************************************************************/

#define BH1750_ADDR          0x23

/* BH1750 I2C 地址
 * ADDR引脚接GND -> 0x23（默认）
 * ADDR引脚接VCC -> 0x5C
 */
#define BH1750_ADDR          0x23

/* BH1750 命令字 */
#define BH1750_POWER_DOWN     0x00   /* 断电 */
#define BH1750_POWER_ON       0x01   /* 上电 */
#define BH1750_RESET          0x07   /* 复位数据寄存器 */
#define BH1750_CONT_H_RES     0x10   /* 连续高分辨率模式（1lx，120ms） */
#define BH1750_CONT_H_RES2    0x11   /* 连续高分辨率模式2（0.5lx，120ms） */
#define BH1750_CONT_L_RES     0x13   /* 连续低分辨率模式（4lx，16ms） */
#define BH1750_ONCE_H_RES     0x20   /* 单次高分辨率模式 */
#define BH1750_ONCE_H_RES2    0x21   /* 单次高分辨率模式2 */
#define BH1750_ONCE_L_RES     0x23   /* 单次低分辨率模式 */

uint8_t BH1750_Init(void);
uint16_t BH1750_ReadLight(void);

#endif
