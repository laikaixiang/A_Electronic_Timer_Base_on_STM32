#ifndef __KEY_H
#define __KEY_H
#include "stm32f10x.h"

// --- 1. 定义按键事件返回值 ---
// 0:无, 1-9:短按, 10-19:长按
typedef enum {
    KEY_NONE = 0,
    
    KEY1_PRESS_SHORT = 1,
    KEY1_PRESS_LONG = 11,
    
//    KEY2_PRESS_SHORT,
//    KEY2_PRESS_LONG,
//    
//    // 如果有更多，继续往下加...
//    KEY3_PRESS_SHORT,
//    KEY3_PRESS_LONG,
} KeyEvent_t;

void Key_Init(void);
uint8_t Key_GetEvent(void);

#endif