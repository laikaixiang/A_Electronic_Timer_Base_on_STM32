#ifndef __MAIN_LOGIC_H
#define __MAIN_LOGIC_H

#include "stm32f10x.h"

// 逻辑模块初始化
void MainLogic_Init(void);

// 获取按键输入
uint8_t Key_GetEvent(void);

// 处理主逻辑，包含状态机和收发动作
void Handle_StateMachine(uint8_t event);

// 处理数码管时间显示刷新
void Handle_Display(void);

// 无线信号轮询接收与处理
void MainLogic_CheckWireless(void);

#endif
