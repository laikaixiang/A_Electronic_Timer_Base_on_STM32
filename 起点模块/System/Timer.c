#include "Timer.h"

// 计时变量
static uint32_t Total_Overflows = 0; // 溢出次数记录
static uint8_t  Timer_Is_Running = 0; // 运行标志位

// 硬件配置参数：10kHz频率 (0.1ms周期)，计数10000次溢出(1秒)
#define TIMER_PSC   7200
#define TIMER_ARR   10000 

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

// 暂停
void Timer_Pause(void)
{
    TIM_Cmd(TIM2, DISABLE);
    Timer_Is_Running = 0;
}

// 恢复/开始
void Timer_Resume(void)
{
    TIM_Cmd(TIM2, ENABLE);
    Timer_Is_Running = 1;
}

// 复位归零
void Timer_Reset(void)
{
    TIM_Cmd(TIM2, DISABLE);      // 停止
    TIM_SetCounter(TIM2, 0);     // 清空当前计数值
    Total_Overflows = 0;         // 清空溢出次数
    TIM_ClearFlag(TIM2, TIM_FLAG_Update); // 清除标志位防止误触发
    Timer_Is_Running = 0;
}

// 获取总毫秒数
uint32_t Timer_GetTotalTimeMs(void)
{
    // 计算公式：溢出次数 * 1000ms + (当前CNT * 0.1ms) / 10
    // 假设 ARR=10000 (1秒), PSC=7200 (10kHz, 0.1ms)
    uint32_t time_ms = (Total_Overflows * 1000) + (TIM_GetCounter(TIM2) / 10);
    return time_ms;
}

// 中断服务函数
void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
    {
        Total_Overflows++;
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    }
}