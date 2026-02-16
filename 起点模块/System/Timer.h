#ifndef __TIMER_H
#define __TIMER_H

void Timer_Init(void);
uint16_t Timer_GetCounter(void);
void Timer_Pause(void);
void Timer_Resume(void);
void Timer_Reset(uint32_t *p_Seconds);
uint32_t Timer_GetTotalTimeMs(uint32_t *p_Seconds);
uint8_t Timer_GetState(void);
uint16_t Timer_GetCounter(void);
uint32_t Timer_Trans2ARRCounter(uint32_t Seconds, uint32_t ARR_Counter);


#endif
