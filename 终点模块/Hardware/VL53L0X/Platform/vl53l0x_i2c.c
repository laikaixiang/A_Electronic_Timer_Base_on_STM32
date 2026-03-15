#include "vl53l0x_i2c.h"
#include "Delay.h" // 软件I2C必须用到微秒延时函数。
#include "VL53L0X.h"

// ==================== 底层 GPIO 控制 ====================
// 配置 SDA 为输入
static void VL_SDA_IN(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = VL_IIC_SDA_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // 上拉输入
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

// 配置 SDA 为输出
static void VL_SDA_OUT(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = VL_IIC_SDA_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; // 推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

// 控制 SCL 电平
static void VL_IIC_SCL(u8 Data) {
    if(Data) GPIO_SetBits(GPIOA, VL_IIC_SCL_PIN);
    else     GPIO_ResetBits(GPIOA, VL_IIC_SCL_PIN);
}

// 控制 SDA 电平
static void VL_IIC_SDA(u8 Data) {
    if(Data) GPIO_SetBits(GPIOA, VL_IIC_SDA_PIN);
    else     GPIO_ResetBits(GPIOA, VL_IIC_SDA_PIN);
}

// 读取 SDA 电平
static u8 VL_IIC_SDA_READ(void) {
    return GPIO_ReadInputDataBit(GPIOA, VL_IIC_SDA_PIN);
}

// ==================== I2C 基础时序 ====================
// 初始化 I2C 引脚
void VL53L0X_i2c_init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 使能 GPIOA 和 GPIOB 时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);
    
    // 初始化 SCL(PA11) 和 SDA(PA12)
    GPIO_InitStructure.GPIO_Pin = VL_IIC_SCL_PIN | VL_IIC_SDA_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // 初始化 Xshut(PB7)
    GPIO_InitStructure.GPIO_Pin = VL_XSHUT_PIN;
    GPIO_Init(VL_XSHUT_PORT, &GPIO_InitStructure);
    
    // 默认拉高 SCL 和 SDA，释放总线
    GPIO_SetBits(GPIOA, VL_IIC_SCL_PIN | VL_IIC_SDA_PIN);
    
    // 拉高 Xshut 引脚，使能 VL53L0X 传感器工作
    GPIO_SetBits(VL_XSHUT_PORT, VL_XSHUT_PIN);
}

// 产生 IIC 起始信号
static void VL_IIC_Start(void) {
    VL_SDA_OUT();
    VL_IIC_SDA(1);
    VL_IIC_SCL(1);
    Delay_us(4);
    VL_IIC_SDA(0); // SCL为高时，SDA由高变低
    Delay_us(4);
    VL_IIC_SCL(0); // 钳住I2C总线，准备发送或接收数据
}

// 产生 IIC 停止信号
static void VL_IIC_Stop(void) {
    VL_SDA_OUT();
    VL_IIC_SCL(0);
    VL_IIC_SDA(0); // SCL为高时，SDA由低变高
    Delay_us(4);
    VL_IIC_SCL(1);
    VL_IIC_SDA(1);
    Delay_us(4);
}

// 等待应答信号
static u8 VL_IIC_Wait_Ack(void) {
    u8 ucErrTime = 0;
    VL_SDA_IN(); // SDA设置为输入
    VL_IIC_SDA(1); Delay_us(1);
    VL_IIC_SCL(1); Delay_us(1);
    while(VL_IIC_SDA_READ()) {
        ucErrTime++;
        if(ucErrTime > 250) {
            VL_IIC_Stop();
            return 1; // 接收应答失败
        }
    }
    VL_IIC_SCL(0); // 时钟输出0
    return 0; // 成功
}

// 产生 ACK 应答
static void VL_IIC_Ack(void) {
    VL_IIC_SCL(0);
    VL_SDA_OUT();
    VL_IIC_SDA(0);
    Delay_us(2);
    VL_IIC_SCL(1);
    Delay_us(2);
    VL_IIC_SCL(0);
}

// 不产生 ACK 应答
static void VL_IIC_NAck(void) {
    VL_IIC_SCL(0);
    VL_SDA_OUT();
    VL_IIC_SDA(1);
    Delay_us(2);
    VL_IIC_SCL(1);
    Delay_us(2);
    VL_IIC_SCL(0);
}

// IIC 发送一个字节
static void VL_IIC_Send_Byte(u8 txd) {                        
    u8 t;   
    VL_SDA_OUT();
    VL_IIC_SCL(0); 
    for(t=0; t<8; t++) {
        VL_IIC_SDA((txd & 0x80) >> 7);
        txd <<= 1;
        Delay_us(2);
        VL_IIC_SCL(1);
        Delay_us(2); 
        VL_IIC_SCL(0);
        Delay_us(2);
    }
}

// IIC 读取一个字节
static u8 VL_IIC_Read_Byte(u8 ack) {
    u8 i, receive = 0;
    VL_SDA_IN();
    for(i=0; i<8; i++ ) {
        VL_IIC_SCL(0); 
        Delay_us(2);
        VL_IIC_SCL(1);
        receive <<= 1;
        if(VL_IIC_SDA_READ()) receive++;
        Delay_us(1); 
    }
    if (!ack) VL_IIC_NAck();
    else VL_IIC_Ack();
    return receive;
}

// ==================== VL53L0X 读写接口 ====================
u8 VL53L0X_write_multi(u8 address, u8 index, u8 *pdata, u16 count) {
    u8 status = STATUS_OK;
    VL_IIC_Start();
    VL_IIC_Send_Byte(address); // 写地址
    if(VL_IIC_Wait_Ack()) status = STATUS_FAIL;
    VL_IIC_Send_Byte(index);   // 寄存器地址
    if(VL_IIC_Wait_Ack()) status = STATUS_FAIL;
    while(count--) {
        VL_IIC_Send_Byte(*pdata++);
        if(VL_IIC_Wait_Ack()) status = STATUS_FAIL;
    }
    VL_IIC_Stop();
    return status;
}

u8 VL53L0X_read_multi(u8 address, u8 index, u8 *pdata, u16 count) {
    u8 status = STATUS_OK;
    VL_IIC_Start();
    VL_IIC_Send_Byte(address); // 写地址
    if(VL_IIC_Wait_Ack()) status = STATUS_FAIL;
    VL_IIC_Send_Byte(index);   // 寄存器地址
    if(VL_IIC_Wait_Ack()) status = STATUS_FAIL;
    
    VL_IIC_Start();
    VL_IIC_Send_Byte(address | 0x01); // 读地址
    if(VL_IIC_Wait_Ack()) status = STATUS_FAIL;
    while(count--) {
        if(count == 0) *pdata = VL_IIC_Read_Byte(0); // 最后一个字节NAck
        else {
            *pdata = VL_IIC_Read_Byte(1); // Ack
            pdata++;
        }
    }
    VL_IIC_Stop();
    return status;
}

u8 VL53L0X_write_byte(u8 address, u8 index, u8 data) {
    return VL53L0X_write_multi(address, index, &data, 1);
}

u8 VL53L0X_write_word(u8 address, u8 index, u16 data) {
    u8 buf[2];
    buf[0] = data >> 8;
    buf[1] = data & 0xFF;
    return VL53L0X_write_multi(address, index, buf, 2);
}

u8 VL53L0X_write_dword(u8 address, u8 index, u32 data) {
    u8 buf[4];
    buf[0] = (data >> 24) & 0xFF;
    buf[1] = (data >> 16) & 0xFF;
    buf[2] = (data >> 8)  & 0xFF;
    buf[3] = data & 0xFF;
    return VL53L0X_write_multi(address, index, buf, 4);
}

u8 VL53L0X_read_byte(u8 address, u8 index, u8 *pdata) {
    return VL53L0X_read_multi(address, index, pdata, 1);
}

u8 VL53L0X_read_word(u8 address, u8 index, u16 *pdata) {
    u8 buf[2];
    u8 status = VL53L0X_read_multi(address, index, buf, 2);
    *pdata = ((u16)buf[0] << 8) | buf[1];
    return status;
}

u8 VL53L0X_read_dword(u8 address, u8 index, u32 *pdata) {
    u8 buf[4];
    u8 status = VL53L0X_read_multi(address, index, buf, 4);
    *pdata = ((u32)buf[0] << 24) | ((u32)buf[1] << 16) | ((u32)buf[2] << 8) | buf[3];
    return status;
}
