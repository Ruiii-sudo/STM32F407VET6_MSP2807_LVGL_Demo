/**
  ******************************************************************************
  * @file    ui_demo_01.h
  * @brief   传感器数据监控UI - 实时数据卡片 + 历史折线图
  ******************************************************************************
  */
#ifndef __UI_DEMO_01_H
#define __UI_DEMO_01_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
  * @brief  UI初始化入口（在LVGL任务中调用）
  */
void ui_demo_01_init(void);

/**
  * @brief  刷新传感器数据显示（在LVGL任务中定时调用）
  */
void ui_refresh_sensor_data(void);

/**
  * @brief  手动锁定屏幕（设置页按钮可调用）
  */
void ui_lock_screen_now(void);
	
/**
  * @brief  查询数据上传开关是否开启
  * @retval true-开启上传, false-关闭上传
  */
bool ui_is_upload_enabled(void);

#ifdef __cplusplus
}
#endif

#endif /* __UI_DEMO_01_H */
