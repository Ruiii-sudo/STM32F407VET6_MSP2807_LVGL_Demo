#ifndef __SENSOR_DATA_H
#define __SENSOR_DATA_H

#include "stm32f4xx_hal.h"

/* 传感器类型枚举 */
typedef enum {
    SENSOR_TEMP = 0,    /* 温度 */
    SENSOR_HUMI,        /* 湿度 */
    SENSOR_LIGHT,       /* 光照 */
    SENSOR_GAS,         /* 气体 */
    SENSOR_COUNT
} SensorType_t;

/* 传感器数据结构 */
typedef struct {
    float    temp;      /* 温度 ℃ */
    float    humi;      /* 湿度 %RH */
    uint16_t light;     /* 光照 lx */
    uint16_t gas;       /* 气体 ADC值 */
} EnvData_t;

/* 历史数据点数量 */
#define HISTORY_MAX  60

/* 初始化传感器数据管理 */
void SensorData_Init(void);

/* 更新一帧传感器数据（同时写入历史缓冲区） */
void SensorData_Update(const EnvData_t *data);

/* 获取当前最新数据 */
EnvData_t SensorData_GetCurrent(void);

/* 获取指定传感器的历史数据（返回有效数据点数） */
/* buf: 调用者提供的float数组，至少HISTORY_MAX个元素 */
/* 返回实际写入的点数 */
uint16_t SensorData_GetHistory(SensorType_t type, float *buf, uint16_t max_len);

/* 获取传感器名称 */
const char *SensorData_GetName(SensorType_t type);

/* 获取传感器单位 */
const char *SensorData_GetUnit(SensorType_t type);

/* 获取传感器当前值（转成float统一格式） */
float SensorData_GetValue(SensorType_t type);

#endif /* __SENSOR_DATA_H */
