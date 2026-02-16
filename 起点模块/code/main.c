#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "Buzzer.h"
#include "LightSensor.h"
#include "LED.h"
#include "seg_display.h"
#include "Key.h"
#include "Timer.h"
#include <stdio.h>

// 系统状态枚举（局部枚举，仅main.c使用）
typedef enum {
    SYS_IDLE,    // 空闲（显示RDY）
    SYS_RUNNING, // 运行计时
    SYS_PAUSE    // 暂停（记忆时间）
} Sys_State;

// 定时器运行状态标志（0=暂停/未启动，1=运行中）
static uint8_t timer_running = 0;
// 按键计时相关变量
static uint32_t key1_press_time = 0;  // 按键按下时长
static uint8_t key1_long_press_flag = 0; // 长按标志
//定时器相关变量
static uint32_t Seconds;	// 秒数 or 中断计数器

// 测试函数
// 函数声明（main.c内部函数）
static void Sys_Key_Process(Sys_State *p_state, uint32_t *p_Seconds, uint32_t *p_pause_ms);
static void Sys_Display_Process(Sys_State state, uint32_t *p_Seconds, uint32_t pause_ms);

int main(void)
{
	/*模块初始化*/
	Buzzer_Init();			//蜂鸣器初始化
	LightSensor_Init();		//光敏传感器初始化
	LED_Init();             //LED初始化
	Key_Init();				//key初始化
	Seg_Init();				//数码管初始化
	
    // 系统时钟初始化（STM32标准库必须）
    SystemInit();
	
    // 计时模块初始化
    Timer_Init();
	// 局部变量（替代全局变量，仅在main中定义）
	Sys_State sys_state = SYS_IDLE;    // 系统状态
	uint32_t pause_total_ms = 0;       // 暂停时的计时值（记忆）
	
	while(1)
	{
		// 按键处理（传指针修改状态/计时值）
		Sys_Key_Process(&sys_state, &Seconds, &pause_total_ms);
		// 显示处理（传状态/指针/值）
		Sys_Display_Process(sys_state, &Seconds, pause_total_ms);
	}
}


// 按键处理逻辑（传指针修改状态/计时值，无全局变量）
static void Sys_Key_Process(Sys_State *p_state, uint32_t *p_Seconds, uint32_t *p_pause_ms)
{
    uint8_t key_num = Key_GetNum();
    if (key_num == 0) return; // 无按键

    // 长按：重置所有状态
    if (Key_GetEvent()==2)
    {	
		Seg_ShowREADY();
        Timer_Reset(p_Seconds);    // 重置定时器（清空Seconds和CNT）
		Seconds = 0;
        *p_pause_ms = 0;           // 清空暂停记忆值
        *p_state = SYS_IDLE;       // 切回空闲状态
        return;
    }
	
    // 短按：状态切换
    switch (*p_state)
    {
        case SYS_IDLE:
            // 空闲→运行：启动定时器
            *p_state = SYS_RUNNING;
            Timer_Resume();
            break;

        case SYS_RUNNING:
            // 运行→暂停：记忆当前时间，暂停定时器
            *p_state = SYS_PAUSE;
            Timer_Pause();
            *p_pause_ms = Timer_GetTotalTimeMs(p_Seconds); // 记录暂停时的计时值
            break;

        case SYS_PAUSE:
            // 暂停→运行：恢复定时器
            *p_state = SYS_RUNNING;
            Timer_Resume();
            break;

        default:
            *p_state = SYS_IDLE;
            break;
    }
}

// 显示处理逻辑（无全局变量，传参驱动显示）
static void Sys_Display_Process(Sys_State state, uint32_t *p_Seconds, uint32_t pause_ms)
{
    uint32_t current_ms;

    switch (state)
    {
        case SYS_IDLE:
            Seg_ShowREADY(); // 空闲显示RDY
            break;

        case SYS_RUNNING:
            // 运行中：实时获取计时值并显示
            current_ms = Timer_GetTotalTimeMs(p_Seconds);
            Seg_DispTime(current_ms);
            break;

        case SYS_PAUSE:
            // 暂停：显示记忆的暂停时间
            Seg_DispTime(pause_ms);
            break;

        default:
            Seg_ShowREADY();
            break;
    }
}

/**
  * 函    数：TIM2中断函数
  * 参    数：无
  * 返 回 值：无
  * 注意事项：此函数为中断函数，无需调用，中断触发后自动执行
  *           函数名为预留的指定名称，可以从启动文件复制
  *           请确保函数名正确，不能有任何差异，否则中断函数将不能进入
  */
void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)		//判断是否是TIM2的更新事件触发的中断
	{
		Seconds ++;												//Num变量自增，用于测试定时中断
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);			//清除TIM2更新事件的中断标志位
															//中断标志位必须清除
															//否则中断将连续不断地触发，导致主程序卡死
	}
}
