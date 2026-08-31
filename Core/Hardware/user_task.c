#include "user_task.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

#include "LCD.h"
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "lv_demos.h"
#include "touch.h"
#include "XPT2046.h"
#include "ui_demo_01.h"
#include "sht30.h"
#include "bh1750.h"
#include "mq2.h"
#include "sensor_data.h"
#include "esp8266.h"

/* 低功耗第一档：LCD 背光自动熄灭 */
#define BACKLIGHT_TIMEOUT_MS  60000   // 无触摸60s灭背光
static uint32_t g_backlight_last_tick = 0;  // 上次触摸时的系统 tick，用于精确计时
static uint8_t  g_backlight_is_on  = 1;

/* 低功耗第二档：触摸 EXTI 中断唤醒 */
static TaskHandle_t  xTouchTaskHandle = NULL;  // 触摸任务句柄，中断唤醒用


/*队列句柄*/
QueueHandle_t DataQueue;  // 环境数据队列句柄

/*启动任务配置*/
#define START_TASK_STACK_SIZE 128
#define START_TASK_PRIORITY 2
TaskHandle_t start_task_handle;
void start_task(void *pvParameters);  //start_task函数声明

/*LVGL图形任务配置*/
#define LVGL_Task_STACK_SIZE 512
#define LVGL_Task_PRIORITY 3
TaskHandle_t LVGL_Task_handle;
void LVGL_Task(void *pvParameters);  //LVGL_Task函数声明

/*触摸扫描任务配置*/
#define XPT2046_ScanTask_STACK_SIZE 128
#define XPT2046_ScanTask_PRIORITY 4
TaskHandle_t XPT2046_ScanTask_handle;
void XPT2046_ScanTask(void *pvParameters);  //XPT2046_ScanTask函数声明

/*传感器采集任务配置*/
#define SensorCollect_Task_STACK_SIZE 128
#define SensorCollect_Task_PRIORITY 2
TaskHandle_t SensorCollect_Task_handle;
void SensorCollect_Task(void *pvParameters);  //SensorCollect_Task函数声明

/*MQTT上云任务配置*/
#define MQTT_Upload_Task_STACK_SIZE 512
#define MQTT_Upload_Task_PRIORITY 2
TaskHandle_t MQTT_Upload_Task_handle;
void MQTT_Upload_Task(void *pvParameters);  //MQTT_Upload_Task函数声明



/**
 * @brief  启动FreeRTOS
 * @retval None
 */
void freertos_start(void)
{
    /*在创建任务前，先创建好需要的队列*/
    //创建用于传递环境数据的队列
    DataQueue = xQueueCreate(10, sizeof(EnvData_t));
	
    //1.创建一个启动任务
    xTaskCreate((TaskFunction_t) start_task,                      //任务函数的地址
                (char *) "Start Task",                            //任务名字
                (configSTACK_DEPTH_TYPE) START_TASK_STACK_SIZE,   //任务栈大小，单位为字
                (void *)  NULL,                                   //传递给任务函数的参数
                (UBaseType_t) START_TASK_PRIORITY,                //任务优先级
                (TaskHandle_t *) &start_task_handle);             //任务句柄的地址

    //2.启动调度器
    vTaskStartScheduler();
}

/**
 * @brief  启动任务:用来创建其他任务
 * @param  pvParameters: 任务参数
 * @retval None
 */
void start_task(void *pvParameters)
{
    taskENTER_CRITICAL(); //进入临界区:保护临界区中的代码不会被打断

    //创建LVGL图形任务
    xTaskCreate((TaskFunction_t) LVGL_Task,                    
                (char *) "LVGL_Task",                          
                (configSTACK_DEPTH_TYPE) LVGL_Task_STACK_SIZE,  
                (void *)  NULL,                             
                (UBaseType_t) LVGL_Task_PRIORITY,               
                (TaskHandle_t *) &LVGL_Task_handle);      

    //创建触摸扫描任务
    xTaskCreate((TaskFunction_t) XPT2046_ScanTask,                    
                (char *) "XPT2046_ScanTask",                          
                (configSTACK_DEPTH_TYPE) XPT2046_ScanTask_STACK_SIZE,  
                (void *)  NULL,                             
                (UBaseType_t) XPT2046_ScanTask_PRIORITY,               
                (TaskHandle_t *) &XPT2046_ScanTask_handle);  
                
    //创建传感器采集任务
    xTaskCreate((TaskFunction_t) SensorCollect_Task,                    
                (char *) "SensorCollect_Task",                          
                (configSTACK_DEPTH_TYPE) SensorCollect_Task_STACK_SIZE,  
                (void *)  NULL,                             
                (UBaseType_t) SensorCollect_Task_PRIORITY,               
                (TaskHandle_t *) &SensorCollect_Task_handle);
    
    //创建MQTT上云任务
    xTaskCreate((TaskFunction_t) MQTT_Upload_Task,                    
                (char *) "MQTT_Upload_Task",                          
                (configSTACK_DEPTH_TYPE) MQTT_Upload_Task_STACK_SIZE,  
                (void *)  NULL,                             
                (UBaseType_t) MQTT_Upload_Task_PRIORITY,               
                (TaskHandle_t *) &MQTT_Upload_Task_handle);


    //删除启动任务(启动任务只需要执行一次，创建完其他任务后就可以删除自己)
    vTaskDelete(NULL);

    taskEXIT_CRITICAL(); //退出临界区
}


void LVGL_Task(void *pvParameters)
{
    // LVGL初始化（必须按顺序）
    USART1_Printf("STEP1: 开始LVGL初始化\r\n");
    lv_init();
    USART1_Printf("STEP2: 开始LVGL显示初始化\r\n");
    lv_port_disp_init(); // 显示初始化   
    USART1_Printf("STEP3: 开始LVGL输入设备初始化\r\n");
    lv_port_indev_init(); // 输入设备初始化（触摸）
    
    // 创建LVGL demo
    //lv_demo_widgets();
    ui_demo_01_init();
    uint32_t refresh_cnt = 0; // 用于定时刷新传感器数据到UI界面
    USART1_Printf("System init OK\r\n");

    for (;;)
    {
        lv_task_handler();  // LVGL任务处理

        /*使用队列的原因：
        *LVGL任务接收队列中传感器采集的数据后，再由LVGL任务进行写全局，
        *那么LVGL读全局时不会被写打断，因为它们在一个任务中 顺序执行（LVGL写全局结束才读全局）
        *但是MQTT任务读全局时是可能会被写打断的（出现读写冲突） 解决方法：可以用临界区
        *如果不使用队列，则传感器采集任务写全局，LVGL和MQTT任务读全局，二者读全局时都可能会被写打断（读写冲突）导致数据不一致
        *所以经过队列优化后，从两处读写冲突变成了只有一处读写冲突
        * 
        *LVGL和MQTT任务不能读同一个队列，因为队列是先进先出，读一次就会被移除，另一个任务就读不到了（单生产者双消费者）
        */
        EnvData_t rec_data; // 用于接收队列中的传感器数据
        if (xQueueReceive(DataQueue, &rec_data, 0) == pdPASS) {
            // 从队列中接收到传感器数据后，写全局
            SensorData_Update(&rec_data);
        }

        if (++refresh_cnt >= 100) // 每100次刷新一次传感器数据到UI界面（约500ms）
        {
            refresh_cnt = 0; // 重置计数器
            ui_refresh_sensor_data(); // 刷新传感器数据到UI界面（读取全局数据）
        }
		
		
		/* 低功耗：背光精确计时（用系统 tick，不受任务周期影响）*/
        if (g_backlight_is_on)
        {
            if ((xTaskGetTickCount() - g_backlight_last_tick) >= BACKLIGHT_TIMEOUT_MS)
            {
                LCD_LED_OFF();
                g_backlight_is_on = 0;
				ui_lock_screen_now(); // 背光熄灭同时进入锁屏页
            }
        }

		
        vTaskDelay(5); 
    }
}


void XPT2046_ScanTask(void *pvParameters)
{
    TP_Init();  // 触摸硬件初始化
    g_backlight_last_tick = xTaskGetTickCount();  // 开机时开始计时
    g_backlight_is_on  = 1;

    xTouchTaskHandle = xTaskGetCurrentTaskHandle();  // 保存句柄供中断唤醒

    for (;;)
    {
        /* 无限等待中断通知，没触摸时任务完全挂起，CPU 深度睡眠 */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		
	
        /* 被唤醒 → 连续扫描直到松手（支持拖动） */
        uint8_t release_cnt = 0;
        while (1)
        {
            tp_dev.scan();

            if (g_xpt2046_pressed)
                release_cnt = 0;       // 还按着
            else
            {
                release_cnt++;
                if (release_cnt >= 3)   // 连续3次未按下，确认松手
                    break;
            }
            vTaskDelay(15);             // 拖动跟踪间隔 
        }

        /* 松手：重新使能 EXTI 中断，等待下一次按下 */
        //__HAL_GPIO_EXTI_ENABLE_IT(GPIO_PIN_8);
		EXTI->IMR |= GPIO_PIN_8; 


    }
}

void SensorCollect_Task(void *pvParameters)
{
    EnvData_t env_data;
    BH1750_Init();  //初始化BH1750
    for (;;)
    {
        SHT30_ReadData(&env_data.temp, &env_data.humi);
        env_data.light = BH1750_ReadLight();
        env_data.gas = MQ2_ReadAdc(); 
        
        //将采集到的数据发送到队列中
        xQueueSend(DataQueue, &env_data, 0);
        vTaskDelay(1000); // 每隔1秒收集一次数据
    }
}

void MQTT_Upload_Task(void *pvParameters)
{
    char json_buf[256];
    static uint32_t msg_id = 1;
    uint8_t fail_cnt = 0;       // 当前状态内的失败计数

    typedef enum {
        MQTT_STATE_CONNECTED = 0,
        MQTT_STATE_RECONNECT_MQTT,
        MQTT_STATE_RECONNECT_WIFI,
    } MQTT_State_t;

    MQTT_State_t mqtt_state = MQTT_STATE_RECONNECT_WIFI;
	
	ESP8266_Reset();
    ESP8266_Init();

    for (;;) //死循环
    {
        switch (mqtt_state)
        {
            /* 状态1：正常连接，上报数据 */
            case MQTT_STATE_CONNECTED:
            {
                taskENTER_CRITICAL();
                EnvData_t data = SensorData_GetCurrent();
                taskEXIT_CRITICAL();
				
				/* 检查数据上传开关：关闭时跳过本次发布，保持MQTT连接不断开 */
                if (!ui_is_upload_enabled())
                {
                    break;
                }

                snprintf(json_buf, sizeof(json_buf),
                         "{\"id\":\"%lu\",\"params\":{\"temp\":{\"value\":%.1f},\"humi\":{\"value\":%.1f},\"light\":{\"value\":%d},\"gas\":{\"value\":%d}}}",
                         (unsigned long)msg_id, data.temp, data.humi, data.light, data.gas);

                if (ESP8266_MQTT_Publish(MQTT_PUBLISH_TOPIC, json_buf))
                {
                    USART1_Printf("[MQTT] Publish OK, id=%lu\r\n", (unsigned long)msg_id);
                    msg_id++;
                    fail_cnt = 0;
                    ESP8266_DumpRx(1000);  // 收平台回复
                }
                else
                {
                    fail_cnt++;
                    USART1_Printf("[MQTT] Publish FAILED (%d/3)\r\n", fail_cnt);

                    if (fail_cnt >= 3)
                    {
                        /* 连续失败3次，先查MQTT到底还连不连着 */
                        if (ESP8266_MQTT_IsConnected())
                        {
                            /* 还连着！说明只是偶发发布失败，不用重连，继续发 */
                            USART1_Printf("[MQTT] MQTT still connected, retry publish...\r\n");
                            fail_cnt = 0;
                        }
                        else
                        {
                            /* 真断了，检查WiFi决定降级到哪一级 */
                            if (ESP8266_CheckWiFi())
                            {
                                USART1_Printf("[MQTT] MQTT lost, WiFi OK, reconnect MQTT...\r\n");
                                mqtt_state = MQTT_STATE_RECONNECT_MQTT;
                            }
                            else
                            {
                                USART1_Printf("[MQTT] MQTT lost, WiFi down, reconnect WiFi...\r\n");
                                mqtt_state = MQTT_STATE_RECONNECT_WIFI;
                            }
                            fail_cnt = 0;
                        }
                    }
                }
                break;
            }

            /* 状态2：重连MQTT */
            case MQTT_STATE_RECONNECT_MQTT:
            {
                USART1_Printf("[MQTT] Reconnecting MQTT...\r\n");

                if (ESP8266_MQTT_Connect())
                {
                    USART1_Printf("[MQTT] MQTT Reconnected\r\n");
                    mqtt_state = MQTT_STATE_CONNECTED;
                    fail_cnt = 0;
                }
                else
                {
                    fail_cnt++;
                    USART1_Printf("[MQTT] MQTT reconnect FAILED (%d/3)\r\n", fail_cnt);

                    if (fail_cnt >= 3)
                    {
						if (ESP8266_CheckWiFi())
						{
							/* WiFi 还连着, MQTT连不上3次 → ESP8266可能假死，软复位 */
							USART1_Printf ("[MQTT] WiFi still connected, MQTT dead 3x, reset ESP8266...\r\n");
							ESP8266_Reset();
							mqtt_state = MQTT_STATE_RECONNECT_WIFI;
							fail_cnt = 0;
						}
                        else
						{
							/* WiFi 已经断了，降级到重连 WiFi */
							USART1_Printf ("[MQTT] WiFi disconnected, reconnect WiFi...\r\n");
							mqtt_state = MQTT_STATE_RECONNECT_WIFI;
							fail_cnt = 0;
						}
                    }
                    else
                    {
                        vTaskDelay(3000);
                    }
                }
                break;
            }

            /* 状态3：重连WiFi */
            case MQTT_STATE_RECONNECT_WIFI:
            {
                USART1_Printf("[MQTT] Reconnecting WiFi...\r\n");

                if (ESP8266_ConnectWiFi())
                {
                    USART1_Printf("[MQTT] WiFi Reconnected\r\n");
                    mqtt_state = MQTT_STATE_RECONNECT_MQTT;
                    fail_cnt = 0;
                }
                else
                {
                    fail_cnt++;
                    USART1_Printf("[MQTT] WiFi reconnect FAILED (%d/3)\r\n", fail_cnt);

                    if (fail_cnt >= 3)
                    {
                        /* WiFi连不上3次 → ESP8266可能假死，软复位 */
                        USART1_Printf("[MQTT] WiFi dead 3x, reset ESP8266...\r\n");
                        ESP8266_Reset();
                        fail_cnt = 0;
                        /* 复位后继续在本状态重连WiFi */
                    }
                    else
                    {
                        vTaskDelay(3000);
                    }
                }
                break;
            }

            default:
            {
                mqtt_state = MQTT_STATE_RECONNECT_WIFI;
                fail_cnt = 0;
                break;
            }
        }

        vTaskDelay(10000);  // 每隔10秒上传一次数据
    }
}

/**
 * @brief HAL GPIO EXTI 中断回调
 *        PC8(T_IRQ) 触摸按下时触发，唤醒触摸任务 + 点亮背光
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_8)
    {
        /* 禁用 EXTI 线8，防止扫描过程中 T_IRQ 抖动反复触发 */
        //__HAL_GPIO_EXTI_DISABLE_IT(GPIO_PIN_8);
		EXTI->IMR &= ~GPIO_PIN_8;  


        /* 唤醒挂起的触摸任务 */
        if (xTouchTaskHandle != NULL)
        {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            vTaskNotifyGiveFromISR(xTouchTaskHandle, &xHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }

        /* 触摸唤醒背光 */
        if (!g_backlight_is_on)
        {
            LCD_LED_ON();
            g_backlight_is_on = 1;
            //USART1_Printf("[LOWPOWER] Backlight ON (touch wake)\r\n");
        }
        g_backlight_last_tick = xTaskGetTickCountFromISR();  // 记录触摸时刻的系统 tick 
    }
}

/**
 * @brief 重置背光倒计时（解锁屏幕后调用）
 */
void backlight_reset(void)
{
    g_backlight_last_tick = xTaskGetTickCount();
    if (!g_backlight_is_on)
    {
        LCD_LED_ON();
        g_backlight_is_on = 1;
    }
}

