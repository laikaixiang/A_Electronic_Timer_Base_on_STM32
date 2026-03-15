#include "myiic.h"

// 简单的软件微秒级延时，适用于 72MHz 的 STM32F103
// 控制 I2C 的通信速率在 100KHz - 400KHz 左右
static void IIC_Delay(void) {
    uint8_t i = 30; // 适当调整这个值可以改变 I2C 速率
    while(i--);
}

// 初始化 I2C 引脚
void IIC_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;

    // 1. 开启 GPIO 时钟
    RCC_APB2PeriphClockCmd(IIC_CLK, ENABLE);

    // 2. 配置 SCL 和 SDA 为开漏输出 (Out_OD)
    // 关键：开漏输出模式下，输出1代表释放总线，可以直接读取外部电平，免去了切换方向的麻烦
    GPIO_InitStructure.GPIO_Pin = IIC_SCL_PIN | IIC_SDA_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD; 
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(IIC_PORT, &GPIO_InitStructure);

    // 3. 初始状态总线拉高（空闲状态）
    IIC_SCL_HIGH();
    IIC_SDA_HIGH();
}

// 产生 IIC 起始信号
void IIC_Start(void) {
    IIC_SDA_HIGH();
    IIC_SCL_HIGH();
    IIC_Delay();
    IIC_SDA_LOW(); // 在 SCL 高电平期间，SDA 产生下降沿
    IIC_Delay();
    IIC_SCL_LOW(); // 钳住 I2C 总线，准备发送或接收数据
}

// 产生 IIC 停止信号
void IIC_Stop(void) {
    IIC_SCL_LOW();
    IIC_SDA_LOW();
    IIC_Delay();
    IIC_SCL_HIGH();
    IIC_Delay();
    IIC_SDA_HIGH(); // 在 SCL 高电平期间，SDA 产生上升沿
    IIC_Delay();
}

// 等待应答信号到来
// 返回值：1，接收应答失败；0，接收应答成功
uint8_t IIC_Wait_Ack(void) {
    uint8_t ucErrTime = 0;
    
    IIC_SDA_HIGH(); // 主机释放 SDA 线，允许从机控制
    IIC_Delay();
    IIC_SCL_HIGH();
    IIC_Delay();
    
    // 等待从机把 SDA 拉低
    while (IIC_SDA_READ()) {
        ucErrTime++;
        if (ucErrTime > 250) {
            IIC_Stop();
            return 1; // 超时无应答
        }
    }
    IIC_SCL_LOW(); // 时钟输出0
    return 0;      // 成功收到应答
}

// 产生 ACK 应答
void IIC_Ack(void) {
    IIC_SCL_LOW();
    IIC_SDA_LOW(); // SDA 拉低表示应答
    IIC_Delay();
    IIC_SCL_HIGH();
    IIC_Delay();
    IIC_SCL_LOW();
}

// 不产生 ACK 应答
void IIC_NAck(void) {
    IIC_SCL_LOW();
    IIC_SDA_HIGH(); // SDA 拉高表示非应答
    IIC_Delay();
    IIC_SCL_HIGH();
    IIC_Delay();
    IIC_SCL_LOW();
}

// IIC 发送一个字节
void IIC_Send_Byte(uint8_t txd) {
    uint8_t t;   
    IIC_SCL_LOW(); // 拉低时钟开始数据传输
    for (t = 0; t < 8; t++) {
        // 先发送高位
        if ((txd & 0x80) >> 7) {
            IIC_SDA_HIGH();
        } else {
            IIC_SDA_LOW();
        }
        txd <<= 1; // 左移一位
        IIC_Delay();
        
        IIC_SCL_HIGH();
        IIC_Delay();
        IIC_SCL_LOW();
        IIC_Delay();
    }
}

// IIC 读取一个字节
// 参数 ack: 1代表发送 ACK 应答，0代表发送 NACK 非应答
uint8_t IIC_Read_Byte(unsigned char ack) {
    uint8_t i, receive = 0;
    
    IIC_SDA_HIGH(); // 释放总线准备读取
    
    for (i = 0; i < 8; i++) {
        IIC_SCL_LOW();
        IIC_Delay();
        IIC_SCL_HIGH();
        receive <<= 1; // 左移空出最低位
        
        if (IIC_SDA_READ()) {
            receive++; // 读到高电平则最低位+1
        }
        IIC_Delay(); 
    }
    
    if (!ack) {
        IIC_NAck(); // 读完最后一个字节发送 NACK
    } else {
        IIC_Ack();  // 还需要继续读则发送 ACK
    }
    return receive;
}
