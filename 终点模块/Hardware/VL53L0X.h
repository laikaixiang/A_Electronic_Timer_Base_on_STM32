#ifndef __VL53L0X_H
#define __VL53L0X_H

#include "stm32f10x.h"
#include "vl53l0x_api.h" 

// I2C 通信引脚配置
#define VL_IIC_PORT      GPIOA
#define VL_IIC_CLK       RCC_APB2Periph_GPIOA
#define VL_IIC_SCL_PIN   GPIO_Pin_9
#define VL_IIC_SDA_PIN   GPIO_Pin_10

// XSHUT (传感器使能) 引脚配置
#define VL_XSHUT_PORT    GPIOA
#define VL_XSHUT_CLK     RCC_APB2Periph_GPIOA
#define VL_XSHUT_PIN     GPIO_Pin_11

// 函数声明
uint8_t VL53L0X_Init(void);
uint16_t VL53L0X_ReadDistance_mm(void);

#endif /* __VL53L0X_H */
