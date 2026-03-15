#ifndef __MYIIC_H
#define __MYIIC_H

#include "stm32f10x.h"

// -------------------------------------------------------------
// 【引脚定义】如果你改了引脚，只需要在这里修改对应的端口和 Pin
// -------------------------------------------------------------
#define IIC_PORT      GPIOB
#define IIC_SCL_PIN   GPIO_Pin_6
#define IIC_SDA_PIN   GPIO_Pin_7
#define IIC_CLK       RCC_APB2Periph_GPIOB

// -------------------------------------------------------------
// 【底层控制宏】利用标准库的位带操作进行高低电平控制
// -------------------------------------------------------------
#define IIC_SCL_HIGH()  GPIO_SetBits(IIC_PORT, IIC_SCL_PIN)
#define IIC_SCL_LOW()   GPIO_ResetBits(IIC_PORT, IIC_SCL_PIN)

#define IIC_SDA_HIGH()  GPIO_SetBits(IIC_PORT, IIC_SDA_PIN)
#define IIC_SDA_LOW()   GPIO_ResetBits(IIC_PORT, IIC_SDA_PIN)

#define IIC_SDA_READ()  GPIO_ReadInputDataBit(IIC_PORT, IIC_SDA_PIN)

// -------------------------------------------------------------
// 【接口函数声明】（与之前 VL53L0X 驱动里需要的完全对应）
// -------------------------------------------------------------
void IIC_Init(void);
void IIC_Start(void);
void IIC_Stop(void);
uint8_t IIC_Wait_Ack(void);
void IIC_Ack(void);
void IIC_NAck(void);
void IIC_Send_Byte(uint8_t txd);
uint8_t IIC_Read_Byte(unsigned char ack);

#endif