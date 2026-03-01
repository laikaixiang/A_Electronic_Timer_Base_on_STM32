#include "Timer.h"

// 计时变量
static uint32_t Total_Overflows = 0; // 溢出次数记录 数值上等于秒
static uint8_t  Timer_Is_Running = 0; // 运行标志位

// 硬件配置参数：10kHz频率 (0.1ms周期)，计数10000次溢出(1秒)
#define TIMER_PSC   7200
#define TIMER_ARR   10000 

/**
  * 函    数：定时中断初始化
  * 参    数：无
  * 返 回 值：无
  */
void Timer_Init(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    TIM_InternalClockConfig(TIM2);

    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInitStructure.TIM_Period = TIMER_ARR - 1;
    TIM_TimeBaseInitStructure.TIM_Prescaler = TIMER_PSC - 1;
    TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);

    TIM_ClearFlag(TIM2, TIM_FLAG_Update);
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_Init(&NVIC_InitStructure);

    TIM_Cmd(TIM2, DISABLE); // 初始化后暂不启动
    Timer_Is_Running = 0;
}

/**
  * 函    数：暂停定时器
  * 参    数：无
  * 返 回 值：无
  * 说    明：暂停后CNT值保持不变，中断也会暂停
  */
void Timer_Pause(void)
{
    TIM_Cmd(TIM2, DISABLE);
    Timer_Is_Running = 0;
}

/**
  * 函    数：暂停后重新开始定时器
  * 参    数：无
  * 返 回 值：无
  * 说    明：从暂停时的CNT值继续计数
  */
void Timer_Resume(void)
{
    TIM_Cmd(TIM2, ENABLE);
    Timer_Is_Running = 1;
}

/**
  * 函    数：定时器归零
  * 参    数：无
  * 返 回 值：无
  * 说    明：将CNT值清零，同时清空溢出计数，归零后保持当前运行/暂停状态
  */
void Timer_Reset(void)
{
    TIM_Cmd(TIM2, DISABLE);      // 停止
    TIM_SetCounter(TIM2, 0);     // 清空当前计数值
    Total_Overflows = 0;         // 清空溢出次数
    TIM_ClearFlag(TIM2, TIM_FLAG_Update); // 清除标志位防止误触发
    Timer_Is_Running = 0;
}

/**
  * 函    数：获取定时器总计时时间(个ARR，1ARR=0.1ms)
  * 参    数：无
  * 返 回 值：总计时时间，单位ms (基于72MHz主频，10kHz计数频率)
  * 说    明：结合溢出次数+当前CNT值，扩展计时范围
  */
uint32_t Timer_GetTotalTimeMs(void)
{
    // 计算公式：溢出次数 * 1000ms + (当前CNT 0.1ms/个) / 10
    // 假设 ARR=10000 (1秒), PSC=7200 (10kHz, 0.1ms)
    uint32_t time_ms = (Total_Overflows * 1000) + (TIM_GetCounter(TIM2) / 10);
    return time_ms;
}

/**
  * 函    数：获取定时器当前状态
  * 参    数：无
  * 返 回 值：1-运行 0-暂停
  */
void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
    {
        Total_Overflows++;
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    }
}