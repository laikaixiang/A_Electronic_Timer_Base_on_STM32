#ifndef __KEY_H
#define __KEY_H
#include "stm32f10x.h"

// 定义按键引脚
#define KEY_GPIO_PORT   GPIOA
#define KEY_PIN         GPIO_Pin_1 // 根据你原来的定义修改，假设用PA1

// 按键事件类型
#define KEY_NONE        0
#define KEY_SHORT_PRESS 1
#define KEY_LONG_PRESS  2

void Key_Init(void);
uint8_t Key_GetEvent(void); // 获取按键事件

#endif