#include "mq2.h"

extern ADC_HandleTypeDef hadc1;

uint16_t MQ2_ReadAdc(void)
{
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 100);
    uint16_t adc_val = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
    return adc_val;
}

