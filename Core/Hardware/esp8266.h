#ifndef __ESP8266_H
#define __ESP8266_H

#include "main.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>

/*********************** 硬件连接 **************************
* ESP-01S <-> STM32F407
* ESP8266 TX  -> PD6(USART2_RX)
* ESP8266 RX  -> PD5(USART2_TX)
* ESP8266 VCC -> 3.3V 
* ESP8266 GND -> GND
************************************************************/

/*********************** WiFi配置 **************************/
#define WIFI_SSID       "2580"
#define WIFI_PASSWORD   "88888888"

/************************* OneNET MQTT配置 *************************/
/* OneNET新版物联网平台 - 物模型接入 */
#define MQTT_BROKER_HOST        "mqtts.heclouds.com"
#define MQTT_BROKER_PORT        1883
#define MQTT_CLIENT_ID          "test1"
#define MQTT_USERNAME           "usEZau433A"
#define MQTT_PASSWORD           "version=2018-10-31&res=products%2FusEZau433A%2Fdevices%2Ftest1&et=1805693871&method=md5&sign=wWXazOQ9ZtFRryawxcKRCA%3D%3D"

/* 物模型属性上报主题 */
#define MQTT_PUBLISH_TOPIC      "$sys/usEZau433A/test1/thing/property/post"
/* 上报回复主题（订阅用，用于确认上报是否成功） */
#define MQTT_SUB_REPLY_TOPIC    "$sys/usEZau433A/test1/thing/property/post/reply"

/************************* 函数声明 *************************/
void ESP8266_Init(void);
uint8_t ESP8266_ConnectWiFi(void);
uint8_t ESP8266_CheckWiFi(void);
uint8_t ESP8266_MQTT_Connect(void);
uint8_t ESP8266_MQTT_IsConnected(void);
uint8_t ESP8266_MQTT_Publish(const char *topic, const char *payload);
void ESP8266_SendCmd(const char *cmd);
void ESP8266_Reset(void);
void ESP8266_DumpRx(uint32_t wait_ms);

#endif
