#include "esp8266.h"
#include "usart.h"
#include "FreeRTOS.h"
#include "task.h"

extern UART_HandleTypeDef huart2;
static uint8_t rx_buf[512]; // 接收缓冲区
static uint16_t rx_len = 0; // 接收数据长度

/**
 * @brief  通过USART2发送数据到ESP8266
 * @param  data: 数据指针
 * @param  len: 数据长度
 */
static void ESP8266_USART2_Send(uint8_t *data, uint16_t len)
{
    HAL_UART_Transmit(&huart2, data, len, 1000);
}

/**
 * @brief  清空接收缓冲区
 */
static void ESP8266_ClearRx(void)
{
    rx_len = 0;
    memset(rx_buf, 0, sizeof(rx_buf));
}

/**
 * @brief  等待指定响应，超时返回0，成功返回1
 * @param  expect: 期望的响应字符串
 * @param  timeout_ms: 超时时间（毫秒）
 * @return: 1表示成功，0表示失败
 */
static uint8_t ESP8266_WaitResponse(const char *expect, uint32_t timeout_ms)
{
    uint32_t start = HAL_GetTick();
    uint8_t ch;
    while (HAL_GetTick() - start < timeout_ms)
    {
        if (HAL_UART_Receive(&huart2, &ch, 1, 10) == HAL_OK)
        {
            if (rx_len < sizeof(rx_buf) - 1)
            {
                rx_buf[rx_len++] = ch;
                rx_buf[rx_len] = '\0';
            }
            if (strstr((char *)rx_buf, expect) != NULL)
            {
                return 1;
            }
			if (strstr((char *)rx_buf, "ERROR") != NULL)
            {
                return 0;
            }
            if (strstr((char *)rx_buf, "FAIL") != NULL)
            {
                return 0;
            }
        }
    }
    return 0;
}


/**
 * @brief  发送AT指令到ESP8266
 * @param  cmd: 指令字符串
 */
void ESP8266_SendCmd(const char *cmd)
{
    ESP8266_ClearRx();
    ESP8266_USART2_Send((uint8_t *)cmd, strlen(cmd));
    ESP8266_USART2_Send((uint8_t *)"\r\n", 2);
}

/**
 * @brief  发送AT指令并等待指定响应，超时返回0，成功返回1
 * @param  cmd: 指令字符串
 * @param  expect: 期望的响应字符串
 * @param  timeout_ms: 超时时间（毫秒）
 * @return: 1表示成功，0表示失败
 */
static uint8_t ESP8266_SendCmdWait(const char *cmd, const char *expect, uint32_t timeout_ms)
{
    ESP8266_SendCmd(cmd);
    return ESP8266_WaitResponse(expect, timeout_ms);
}

/**
 * @brief  初始化ESP8266
 */
void ESP8266_Init(void)
{
    /* 测试AT响应 */
    for (uint8_t i = 0; i < 3; i++)
    {
        if (ESP8266_SendCmdWait("AT", "OK", 1000))
        {
            USART1_Printf("[ESP8266] AT OK\r\n");
            break;
        }
        vTaskDelay(500);
    }
	
	/* 关闭回显 —— 不加这行，PUBRAW的>会被回显淹没 */
    if (ESP8266_SendCmdWait("ATE0", "OK", 1000))
    {
        USART1_Printf("[ESP8266] Echo OFF\r\n");
    }

    /* 设置WiFi模式为Station模式 */
    if (ESP8266_SendCmdWait("AT+CWMODE=1", "OK", 1000))
    {
        USART1_Printf("[ESP8266] Set STA mode OK\r\n");
    }
    USART1_Printf("[ESP8266] Init done\r\n");
}

/**
 * @brief  连接WiFi
 * @return: 1表示成功，0表示失败
 */
uint8_t ESP8266_ConnectWiFi(void)
{
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"",
             WIFI_SSID, WIFI_PASSWORD);
    USART1_Printf("[ESP8266] Connecting WiFi: %s ...\r\n", WIFI_SSID);
    if (ESP8266_SendCmdWait(cmd, "WIFI GOT IP", 15000))
    {
        USART1_Printf("[ESP8266] WiFi connected\r\n");
        return 1;
    }
    USART1_Printf("[ESP8266] WiFi connect FAILED\r\n");
    return 0;
}

uint8_t ESP8266_CheckWiFi(void)
{
    /* 发送AT+CWJAP?查询当前连接的AP信息
    * 已连接返回：+CWJAP:"SSID",... OK
    * 未连接返回：No AP OK
    */
    if (ESP8266_SendCmdWait("AT+CWJAP?", "+CWJAP:", 2000))
    {
        return 1;  /* 已连接到AP */
    }
    return 0;  /* 未连接 */
}

/**
 * @brief  连接OneNET MQTT服务器
 * @return: 1表示成功，0表示失败
 */
uint8_t ESP8266_MQTT_Connect(void)
{
    char buf[256];

    /* 第1步：MQTT用户配置 */
    snprintf(buf, sizeof(buf),
             "AT+MQTTUSERCFG=0,1,\"%s\",\"%s\",\"%s\",0,0,\"\"",
             MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD);
    USART1_Printf("[MQTT] User config...\r\n");
    if (!ESP8266_SendCmdWait(buf, "OK", 3000))
    {
        USART1_Printf("[MQTT] User config FAILED\r\n");
        return 0;
    }
    USART1_Printf("[MQTT] User config OK\r\n");
       
    /* 第2步：连接OneNET MQTT服务器（reconnect=1开启自动重连） */
    snprintf(buf, sizeof(buf), "AT+MQTTCONN=0,\"%s\",%d,1",
             MQTT_BROKER_HOST, MQTT_BROKER_PORT);
    USART1_Printf("[MQTT] Connecting to %s:%d ...\r\n", MQTT_BROKER_HOST, MQTT_BROKER_PORT);
    if (!ESP8266_SendCmdWait(buf, "OK", 8000))
    {
        USART1_Printf("[MQTT] MQTT connect FAILED\r\n");
        return 0;
    }
    USART1_Printf("[MQTT] MQTT connect OK\r\n");

    /* 第3步：订阅上报回复主题 */
    snprintf(buf, sizeof(buf), "AT+MQTTSUB=0,\"%s\",0", MQTT_SUB_REPLY_TOPIC);
    USART1_Printf("[MQTT] Subscribing to %s ...\r\n", MQTT_SUB_REPLY_TOPIC);
    if (!ESP8266_SendCmdWait(buf, "OK", 2000))
    {
        USART1_Printf("[MQTT] MQTT subscribe FAILED\r\n");
        return 0;
    }
    USART1_Printf("[MQTT] MQTT subscribe OK\r\n");
    return 1;
}


/**
 * @brief 查询MQTT连接状态
 * @return 1表示已连接，0表示未连接
 */
uint8_t ESP8266_MQTT_IsConnected(void)
{
    if (ESP8266_SendCmdWait("AT+MQTTCONN?", "+MQTTCONN:0,1", 2000))
    {
        return 1;  // 已连接
    }
    return 0;  // 未连接
}



/**
 * @brief  发布MQTT消息
 * @param  topic: 发布主题
 * @param  payload: 消息内容
 * @return: 1表示成功，0表示失败
 */
uint8_t ESP8266_MQTT_Publish(const char *topic, const char *payload)
{
    char buf[256]; // 发送AT指令缓冲区
    uint16_t payload_len = strlen(payload);
	
	if (payload_len == 0 || payload_len > 2048)
    {
        USART1_Printf("[MQTT] Invalid payload len: %d\r\n", payload_len);
        return 0;
    }
	
    // 发送AT+MQTTPUBRAW指令，指定主题和消息长度
    snprintf(buf, sizeof(buf), "AT+MQTTPUBRAW=0,\"%s\",%d,0,0",
             topic, payload_len);
    ESP8266_SendCmd(buf);
    // 等待ESP8266返回">"提示符，表示可以发送消息内容
    if (!ESP8266_WaitResponse(">", 5000))
    {
        USART1_Printf("[MQTT] PUBRAW no >, rx: %s\r\n", rx_buf);
		/* 关键：发送一个\r\n尝试跳出RAW模式，避免状态机锁死 */
        ESP8266_USART2_Send((uint8_t *)"\r\n", 2);
        ESP8266_ClearRx();
        return 0;
    }
    // 发送消息内容
    ESP8266_USART2_Send((uint8_t *)payload, payload_len);
    // 等待ESP8266返回"OK"表示发布成功
    if (!ESP8266_WaitResponse("OK", 5000))
    {
        USART1_Printf("[MQTT] Publish FAILED, rx: %s\r\n", rx_buf);
        return 0;
    }
    USART1_Printf("[MQTT] Publish OK\r\n");
    return 1;
}

/**
 * @brief 软复位ESP8266
 */
void ESP8266_Reset(void)
{
    USART1_Printf("[ESP8266] Software reset...\r\n");
    ESP8266_SendCmd("AT+RST");
    vTaskDelay(3000);  // 等待重启完成
    ESP8266_ClearRx();
    /* 重启后重新关闭回显 */
    ESP8266_SendCmdWait("ATE0", "OK", 2000);
    USART1_Printf("[ESP8266] Reset done\r\n");
}

/**
 * @brief 打印平台 reply
 */
void ESP8266_DumpRx(uint32_t wait_ms)
{
    uint32_t start = HAL_GetTick();
    uint8_t ch;
    ESP8266_ClearRx();
    while (HAL_GetTick() - start < wait_ms)
    {
        if (HAL_UART_Receive(&huart2, &ch, 1, 10) == HAL_OK)
        {
            if (rx_len < sizeof(rx_buf) - 1)
            {
                rx_buf[rx_len++] = ch;
                rx_buf[rx_len] = '\0';
            }
        }
    }
    if (rx_len > 0)
    {
        USART1_Printf("[MQTT] Platform reply: %s\r\n", rx_buf);
    }
}


