#include "sensor_data.h"
#include <string.h>

/* 当前最新数据 */
static EnvData_t s_current_data;

/* 历史数据环形缓冲区 */
static float s_history[SENSOR_COUNT][HISTORY_MAX];
static uint16_t s_history_head[SENSOR_COUNT];  /* 下一个写入位置 */
static uint16_t s_history_count[SENSOR_COUNT]; /* 已写入数据点数 */

/* 传感器名称 */
static const char *s_sensor_names[SENSOR_COUNT] = {
    "Temp",
    "Humi",
    "Light",
    "Gas"
};

/* 传感器单位 */
static const char *s_sensor_units[SENSOR_COUNT] = {
    "C",
    "%RH",
    "lx",
    "ADC"
};

/**
 * @brief  初始化传感器数据管理
 * @retval None
 */
void SensorData_Init(void)
{
    memset(&s_current_data, 0, sizeof(s_current_data));
    memset(s_history, 0, sizeof(s_history));
    memset(s_history_head, 0, sizeof(s_history_head));
    memset(s_history_count, 0, sizeof(s_history_count));
}

/**
 * @brief  更新传感器数据
 * @param  data: 新的传感器数据
 * @retval None
 */
void SensorData_Update(const EnvData_t *data)
{
    if (data == NULL) return;

    /* 保存当前值 */
    s_current_data = *data;

    /* 写入各传感器历史缓冲区 */
    float values[SENSOR_COUNT];
    values[SENSOR_TEMP]  = data->temp;
    values[SENSOR_HUMI]  = data->humi;
    values[SENSOR_LIGHT] = (float)data->light;
    values[SENSOR_GAS]   = (float)data->gas;

    for (uint8_t i = 0; i < SENSOR_COUNT; i++) {
        s_history[i][s_history_head[i]] = values[i];
        s_history_head[i] = (s_history_head[i] + 1) % HISTORY_MAX;
        if (s_history_count[i] < HISTORY_MAX) {
            s_history_count[i]++;
        }
    }
}

/**
 * @brief  获取当前传感器数据
 * @retval 当前传感器数据
 */
EnvData_t SensorData_GetCurrent(void)
{
    return s_current_data;
}

/**
 * @brief  获取传感器历史数据
 * @param  type: 传感器类型
 * @param  buf: 存储历史数据的缓冲区
 * @param  max_len: 缓冲区最大长度
 * @retval 实际获取的历史数据点数
 */
uint16_t SensorData_GetHistory(SensorType_t type, float *buf, uint16_t max_len)
{
    if (buf == NULL || type >= SENSOR_COUNT) return 0;

    uint16_t count = s_history_count[type];
    if (count > max_len) count = max_len;
    if (count == 0) return 0;

    /* 从最旧的数据开始输出，保证时间顺序 */
    uint16_t start = (s_history_head[type] + HISTORY_MAX - count) % HISTORY_MAX;
    for (uint16_t i = 0; i < count; i++) {
        buf[i] = s_history[type][(start + i) % HISTORY_MAX];
    }
    return count;
}

/**
 * @brief  获取传感器名称
 * @param  type: 传感器类型
 * @retval 传感器名称
 */
const char *SensorData_GetName(SensorType_t type)
{
    if (type >= SENSOR_COUNT) return "";
    return s_sensor_names[type];
}

/**
 * @brief  获取传感器单位
 * @param  type: 传感器类型
 * @retval 传感器单位
 */
const char *SensorData_GetUnit(SensorType_t type)
{
    if (type >= SENSOR_COUNT) return "";
    return s_sensor_units[type];
}

/**
 * @brief  获取传感器值
 * @param  type: 传感器类型
 * @retval 传感器值
 */
float SensorData_GetValue(SensorType_t type)
{
    switch (type) {
        case SENSOR_TEMP:  return s_current_data.temp;
        case SENSOR_HUMI:  return s_current_data.humi;
        case SENSOR_LIGHT: return (float)s_current_data.light;
        case SENSOR_GAS:   return (float)s_current_data.gas;
        default:            return 0.0f;
    }
}
