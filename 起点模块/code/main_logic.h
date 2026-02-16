#ifndef __MAIN_LOGIC_H
#define __MAIN_LOGIC_H

#include "stm32f10x.h"


void MainLogic_Init(void);// 逻辑模块初始化
void Handle_Display(void);//处理输出 (显示/LED/蜂鸣器)
void Handle_StateMachine(uint8_t event)//处理逻辑 (状态机 + 事件处理)

#endif