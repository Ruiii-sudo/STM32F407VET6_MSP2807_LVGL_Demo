#include "i2c.h"

static void I2C_Delay(void)
{
    uint8_t i = 10;
    while (i--)
    {
        __NOP();
    }
}

//I2C开始
void I2C_Start(void)
{   //先将SDA、SCL都置为高电平，处于空闲状态
    HAL_GPIO_WritePin(I2C_GPIO_Port, I2C_SDA_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(I2C_GPIO_Port, I2C_SCL_Pin, GPIO_PIN_SET);
    I2C_Delay();
    HAL_GPIO_WritePin(I2C_GPIO_Port, I2C_SDA_Pin, GPIO_PIN_RESET); //SCL保持高电平时，SDA拉低，产生起始信号
    I2C_Delay();
    HAL_GPIO_WritePin(I2C_GPIO_Port, I2C_SCL_Pin, GPIO_PIN_RESET);  //SCL拉低，准备传输数据
}

//I2C停止
void I2C_Stop(void)
{   
    HAL_GPIO_WritePin(I2C_GPIO_Port, I2C_SDA_Pin, GPIO_PIN_RESET); //SDA先拉低，为产生停止信号做准备
    HAL_GPIO_WritePin(I2C_GPIO_Port, I2C_SCL_Pin, GPIO_PIN_SET);   //SCL拉高
    I2C_Delay();
    HAL_GPIO_WritePin(I2C_GPIO_Port, I2C_SDA_Pin, GPIO_PIN_SET);  //SCL高电平时，SDA拉高，产生停止信号
    I2C_Delay();
}

//I2C等待ACK应答
uint8_t I2C_WaitAck(void)
{
    uint8_t ack = 0;
    HAL_GPIO_WritePin(I2C_GPIO_Port, I2C_SDA_Pin, GPIO_PIN_SET);  //主机释放SDA，等待从机应答
    I2C_Delay();
    HAL_GPIO_WritePin(I2C_GPIO_Port, I2C_SCL_Pin, GPIO_PIN_SET); //SCL拉高，读取SDA电平
    I2C_Delay();
    ack = HAL_GPIO_ReadPin(I2C_GPIO_Port, I2C_SDA_Pin);   //读取SDA状态：低电平=从机应答(0)，高电平=无应答(1)
    HAL_GPIO_WritePin(I2C_GPIO_Port, I2C_SCL_Pin, GPIO_PIN_RESET);  //SCL拉低，结束应答检测
    I2C_Delay();
    return ack; //0=有应答 1=无应答
}

//I2C发送一个字节
void I2C_SendByte(uint8_t byte)
{
    uint8_t i;
    for(i=0;i<8;i++) //循环发送8位数据
    {  
		//判断当前最高位是1还是0，控制SDA电平
        if(byte & 0x80)  //0x80= 1000 0000，取最高位
            HAL_GPIO_WritePin(I2C_GPIO_Port, I2C_SDA_Pin, GPIO_PIN_SET);
        else
            HAL_GPIO_WritePin(I2C_GPIO_Port, I2C_SDA_Pin, GPIO_PIN_RESET);
       
		byte <<= 1;  //数据左移一位，准备发送下一位
        I2C_Delay();
		
		//SCL产生上升沿，从机锁存数据
        HAL_GPIO_WritePin(I2C_GPIO_Port, I2C_SCL_Pin, GPIO_PIN_SET);
        I2C_Delay();
        HAL_GPIO_WritePin(I2C_GPIO_Port, I2C_SCL_Pin, GPIO_PIN_RESET); //SCL拉低，准备下一位数据
    }
    I2C_WaitAck(); //发送完1字节后，等待从机应答
}

//I2C读取一个字节
uint8_t I2C_ReadByte(uint8_t ack)
{
    uint8_t i, dat=0;
    for(i=0;i<8;i++)
    {
        dat <<= 1;  //数据左移，准备接收新位
		//SCL拉高，读取SDA电平
        HAL_GPIO_WritePin(I2C_GPIO_Port, I2C_SCL_Pin, GPIO_PIN_SET);
        I2C_Delay();
		//如果SDA为高，数据最低位置1
        if(HAL_GPIO_ReadPin(I2C_GPIO_Port, I2C_SDA_Pin))
            dat |= 0x01;
		//SCL拉低，结束一位数据读取
        HAL_GPIO_WritePin(I2C_GPIO_Port, I2C_SCL_Pin, GPIO_PIN_RESET);
        I2C_Delay();
    }

    //主机发送ACK或NACK信号
    if(ack)
        HAL_GPIO_WritePin(I2C_GPIO_Port, I2C_SDA_Pin, GPIO_PIN_RESET); //拉低=应答ACK
    else
        HAL_GPIO_WritePin(I2C_GPIO_Port, I2C_SDA_Pin, GPIO_PIN_SET);  //拉高=非应答NACK
    I2C_Delay();
	//产生时钟脉冲锁存应答信号
    HAL_GPIO_WritePin(I2C_GPIO_Port, I2C_SCL_Pin, GPIO_PIN_SET);
    I2C_Delay();
    HAL_GPIO_WritePin(I2C_GPIO_Port, I2C_SCL_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(I2C_GPIO_Port, I2C_SDA_Pin, GPIO_PIN_SET);

    return dat; //返回读取到的字节
}
