#include "user_task.h"
#include "FreeRTOS.h"
#include "task.h"

#include "LCD.h"
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "lv_demos.h"
#include "touch.h"
#include "XPT2046.h"
#include "ui_demo_01.h"

/*启动任务配置*/
#define START_TASK_STACK_SIZE 128
#define START_TASK_PRIORITY 2
TaskHandle_t start_task_handle;
void start_task(void *pvParameters);  //start_task函数声明

/*LVGL图形任务配置*/
#define LVGL_Task_STACK_SIZE 128
#define LVGL_Task_PRIORITY 3
TaskHandle_t LVGL_Task_handle;
void LVGL_Task(void *pvParameters);  //LVGL_Task函数声明

/*触摸扫描任务配置*/
#define XPT2046_ScanTask_STACK_SIZE 128
#define XPT2046_ScanTask_PRIORITY 4
TaskHandle_t XPT2046_ScanTask_handle;
void XPT2046_ScanTask(void *pvParameters);  //XPT2046_ScanTask函数声明


/**
 * @brief  启动FreeRTOS
 * @retval None
 */
void freertos_start(void)
{
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
  
    USART1_Printf("System init OK, bare metal version\r\n");

    for (;;)
    {
        lv_task_handler();  // LVGL任务处理
        vTaskDelay(5); 
    }
}


void XPT2046_ScanTask(void *pvParameters)
{
    TP_Init();  // 触摸硬件初始化
    uint32_t debug_cnt = 0;
    for (;;)
    {
        tp_dev.scan(); // 轮询扫描触摸点（XPT2046无中断，使用轮询）
        /* 调试输出（校准完成后可注释掉，避免影响触摸性能） */
        debug_cnt++;
        if (g_xpt2046_pressed && (debug_cnt % 10 == 0))
        {
             USART1_Printf("[TOUCH] x=%d y=%d raw_x=%d raw_y=%d\r\n",
                            tp_dev.x[0], tp_dev.y[0], g_xpt2046_raw_x, g_xpt2046_raw_y);
        }
        vTaskDelay(10); 
    }
}




