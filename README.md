# 基于 FreeRTOS+LVGL 的物联网环境监测系统

基于 STM32F407VET6 设计的多传感器环境监测终端，集成温湿度、光照、气体三类传感器，通过 ESP8266 接入 OneNET 物联网平台实现数据云端上报。终端配备 2.8 寸 TFT 触摸屏，基于 LVGL 构建图形交互界面，软件采用 FreeRTOS 进行多任务调度，实现传感器驱动、MQTT 通信、显示刷新与低功耗管理之间的高效协同。

## 硬件清单

| 模块 | 型号 | 接口 | 说明 |
|------|------|------|------|
| 主控 | STM32F407VET6 | — | Cortex-M4 @ 168MHz，512KB Flash / 192KB RAM |
| 显示屏 | MSP2807 (ILI9341) | SPI | 2.8 寸 TFT，240×320 分辨率 |
| 触摸 | XPT2046 | SPI + EXTI | 电阻式触摸，中断唤醒 |
| WiFi | ESP8266 (ESP-01S) | USART2 | AT 指令驱动，MQTT 通信 |
| 温湿度 | SHT30 | I2C（软件模拟） | 温度 / 湿度 |
| 光照 | BH1750 | I2C（软件模拟） | 环境光强度 |
| 气体 | MQ2 | ADC | 烟雾 / 可燃气体检测 |

## 功能特性

### 多传感器数据采集
- 每秒采集一次温度、湿度、光照、气体数据
- 采集数据经消息队列传递至 LVGL 任务，写入全局当前值与环形缓冲区，供界面刷新与 MQTT 上传共享读取

### LVGL 图形交互界面
- **滑动解锁锁屏**：设备上电进入锁屏界面，滑块拖动解锁，未达阈值松手自动回弹，背光超时自动锁屏
  
<img width="250" alt="lock_screen" src="https://github.com/user-attachments/assets/05dc9d49-dced-4dee-a379-29cd10fdeb09" />

- **三页式布局**：数据展示页（2×2 传感器卡片）、系统设置页（自动刷新开关 / 数据上传开关、锁屏按钮）、设备信息页

| 数据展示页 | 系统设置页 | 设备信息页 |
|:---:|:---:|:---:|
| <img width="300" alt="display" src="https://github.com/user-attachments/assets/93d4d958-138e-4507-96a5-8ebb3ce4de8e" /> | <img width="300" alt="settings" src="https://github.com/user-attachments/assets/b09dca35-c400-4bef-a70d-6d3a96a154fc" /> | <img width="300" alt="device" src="https://github.com/user-attachments/assets/f2d804a5-497f-48e2-8031-ba8745118b31" />

- **传感器详情页**：点击卡片进入，折线图展示 60 点历史数据，底部统计面板显示当前值 / 最大值 / 最小值 / 平均值，支持单传感器暂停 / 继续

| 温度 | 湿度 | 光照 | 空气质量 |
|:---:|:---:|:---:|:---:|
| <img width="300" alt="temp" src="https://github.com/user-attachments/assets/23c053c2-1462-44eb-82c1-382b8f53ea5c" /> | <img width="300" alt="humi" src="https://github.com/user-attachments/assets/a4f584ea-c1db-4909-a9b8-d7b6ba1b6846" /> | <img width="300" alt="light" src="https://github.com/user-attachments/assets/5c9c94c0-4f6c-4b7c-833a-aaaabc14e9c2" /> | <img width="300" alt="gas" src="https://github.com/user-attachments/assets/62b8955b-ba5d-48d6-996d-9c2d2c34e0fd" /> |

### 数据可视化算法
- **环形缓冲区**：固定容量 60 点，新数据覆盖最旧数据，无需动态内存分配
- **线性插值**：将 60 个采样点插值至 120 个显示点，曲线平滑无阶梯感
- **动态 Y 轴量程**：根据数据极值自动计算绘图范围，上下各留 10% 边距，数据波动小时不会挤成直线
- **自适应小数位数**：Y 轴刻度根据量程自动选择 0/1/2 位小数

### MQTT 云端上报
- 接入 OneNET 物联网平台物模型，JSON 格式上报温度 / 湿度 / 光照 / 气体数据
- 每 10 秒上传一次，订阅平台回复主题确认投递结果
- **三级断连自愈状态机**：正常上报 → MQTT 重连 → WiFi 重连，连续 3 次失败自动检测链路层级并触发 ESP8266 软复位，确保网络异常后系统可自动恢复

### 低功耗管理
- **FreeRTOS Tickless Idle**：系统空闲时自动进入 WFI 睡眠，通过 PRE_SLEEP / POST_SLEEP 钩子挂起 / 恢复 HAL 时基，消除空闲节拍中断功耗
- **背光智能管理**：60 秒无触摸操作自动关闭背光并锁屏，触摸 EXTI 中断实时唤醒
- 配置数据上传开关，关闭时跳过 MQTT 发布，减少无线通信功耗

## 系统架构

### FreeRTOS 任务划分

| 任务 | 优先级 | 栈大小 | 说明 |
|------|--------|--------|------|
| LVGL 界面刷新 | 3 | 512 字 | lv_task_handler 调度、队列数据接收、UI 定时刷新 |
| 触摸扫描 | 4 | 128 字 | EXTI 中断唤醒后扫描 XPT2046，支持滑动手势 |
| 传感器采集 | 2 | 128 字 | 每秒采集 SHT30/BH1750/MQ2，写入消息队列 |
| MQTT 上传 | 2 | 512 字 | 三级状态机管理 WiFi/MQTT 连接与数据上报 |
| 启动任务 | 2 | 128 字 | 创建以上任务后自删 |

### 任务间通信
- **消息队列**：传感器采集任务 → LVGL 任务，传递 EnvData_t 结构体，容量 10，解耦数据采集与界面处理
- **全局共享数据**：LVGL 任务将队列数据写入全局当前值与环形缓冲区，供界面刷新与 MQTT 上传共同读取；MQTT 读取时使用临界区保护，防止高优先级 LVGL 任务抢占写入导致数据不一致
- **任务通知**：触摸 EXTI 中断 → 触摸扫描任务，零开销唤醒，替代信号量

### DMA 加速
- **SPI1 TX DMA**：LCD 像素数据分块异步传输，配合传输完成中断回调，避免 CPU 阻塞等待
- **USART1 TX DMA**：调试日志非阻塞输出

## 项目结构

```
├── Core/
│   ├── Inc/                # HAL 外设初始化头文件
│   ├── Src/                # HAL 外设初始化源码
│   └── Hardware/           # 应用层硬件驱动
│       ├── sht30.c/h       # SHT30 温湿度驱动 (I2C)
│       ├── bh1750.c/h      # BH1750 光照驱动 (I2C)
│       ├── mq2.c/h         # MQ2 气体驱动 (ADC)
│       ├── i2c.c/h         # 软件 I2C
│       ├── LCD.c/h         # ILI9341 TFT 驱动 (SPI + DMA)
│       ├── XPT2046.c/h     # XPT2046 触摸驱动
│       ├── touch.c/h       # 触摸扫描与校准
│       ├── esp8266.c/h     # ESP8266 WiFi + MQTT 驱动 (AT 指令)
│       ├── sensor_data.c/h  # 环形缓冲区与传感器数据管理
│       └── user_task.c/h   # FreeRTOS 任务定义与调度
├── FreeRTOS/                # FreeRTOS V202212.01 源码与配置
│   └── FreeRTOSConfig.h     # 系统配置 (抢占式调度 / Tickless Idle / 28KB 堆)
├── LVGL/
│   ├── GUI/lvgl/            # LVGL v8.3.x 源码
│   └── GUI_APP/my_demos/
│       └── ui_demo_01.c/h   # 应用界面 (锁屏/ 三页布局 / 折线图)
├── Drivers/                  # STM32 HAL 驱动与 CMSIS
└── MDK-ARM/                  # Keil MDK-ARM 工程文件
```

## 快速开始

### 开发环境
- Keil MDK-ARM 5
- STM32CubeMX
- ST-Link 仿真器

### 编译与烧录
1. 打开 `MDK-ARM/project.uvprojx`
2. 编译工程
3. 通过 ST-Link 烧录至 STM32F407VET6

### WiFi 与云平台配置
在 `Core/Hardware/esp8266.h` 中修改以下配置：

```c
// WiFi
#define WIFI_SSID       "你的WiFi名称"
#define WIFI_PASSWORD   "你的WiFi密码"

// OneNET MQTT
#define MQTT_BROKER_HOST    "mqtts.heclouds.com"
#define MQTT_BROKER_PORT    1883
#define MQTT_CLIENT_ID      "你的设备名称"
#define MQTT_USERNAME       "你的产品ID"
#define MQTT_PASSWORD       "你的设备令牌(MD5签名)"
#define MQTT_PUBLISH_TOPIC  "$sys/产品ID/设备名称/thing/property/post"

```
### 操作说明
- **锁屏**：向右滑动滑块解锁，60 秒无操作自动锁屏
- **展示页**：查看四路传感器实时数值，点击卡片进入历史折线图详情
- **设置页**：开关自动刷新 / 数据上传，手动锁屏
- **设备页**：查看硬件与固件版本信息
- **底部 Tab 栏**：切换三个页面

## 许可证

本项目仅供学习交流使用。
