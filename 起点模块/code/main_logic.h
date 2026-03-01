#ifndef __MAIN_LOGIC_H
#define __MAIN_LOGIC_H

#include "stm32f10x.h"

// 逻辑模块初始化
void MainLogic_Init(void);

// 1. 获取输入 (键盘)
uint8_t Key_GetEvent(void);

// 2. 处理逻辑 (状态机 + 事件处理)
void Handle_StateMachine(uint8_t event);

// 3. 处理输出 (显示/LED/蜂鸣器)
void Handle_Display(void);

// 4. 无线信号轮询查询
void MainLogic_CheckWireless(void);

#endif
