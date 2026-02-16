#ifndef __KEY_H
#define __KEY_H

// 按键事件定义
//#define KEY_NONE          0
//#define KEY1_SHORT_PRESS  1
//#define KEY1_LONG_PRESS   2
//#define KEY2_SHORT_PRESS  3
//#define KEY2_LONG_PRESS   4

//按键定义
#define Key_1 	GPIO_Pin_1
#define Key_2 	GPIO_Pin_2

void Key_Init(void);
uint8_t Key_GetNum(void);
uint8_t Key_GetEvent(void);

#endif
