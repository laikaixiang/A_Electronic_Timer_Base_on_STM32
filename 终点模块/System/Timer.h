#ifndef __TIMER_H
#define __TIMER_H
#include "stm32f10x.h"

void Timer_Init(void);
void Timer_Pause(void);
void Timer_Resume(void);
void Timer_Reset(void);
uint32_t Timer_GetTotalTimeMs(void);

#endif
