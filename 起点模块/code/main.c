#include "stm32f10x.h"
#include "Delay.h"
#include "Buzzer.h"
#include "LightSensor.h"
#include "LED.h"
#include "seg_display.h"
#include "Key.h"
#include "Timer.h"

// 系统状态定义
typedef enum {
    SYS_IDLE,    // 空闲（显示RDY，计时器归零）
    SYS_RUNNING, // 运行中（显示跳动的时间）
    SYS_PAUSE    // 暂停（显示静止的时间）
} Sys_State;

int main(void)
{
    /* 1. 模块初始化 */
    SystemInit();       // 系统时钟配置
    Buzzer_Init();
    LightSensor_Init();
    LED_Init();
    Key_Init();
    Seg_Init();
    Timer_Init();       // 定时器初始化（默认处于暂停状态）

    /* 2. 局部变量定义 */
    Sys_State sys_state = SYS_IDLE; // 初始状态为空闲
    uint8_t key_event = 0;          // 按键事件存储
    uint32_t current_time_ms = 0;   // 存储当前计时值

    while(1)
    {
        /* --- 核心逻辑环 --- */
        
        // 1. 获取按键事件 (非阻塞，瞬间返回)
        key_event = Key_GetEvent();

        // 2. 状态机处理
        switch (sys_state)
        {
            case SYS_IDLE:
                // 显示 READY
                Seg_ShowREADY();
                
                // 事件：短按 -> 开始计时
                if (key_event == KEY_SHORT_PRESS) 
                {
                    Timer_Reset();      // 确保清零
                    Timer_Resume();     // 启动定时器
                    sys_state = SYS_RUNNING;
                }
                break;

            case SYS_RUNNING:
                // 获取时间并显示
                current_time_ms = Timer_GetTotalTimeMs();
                Seg_DispTime(current_time_ms * 10); // Seg_DispTime输入单位是0.1ms，所以乘以10
                
                // 事件：短按 -> 暂停
                if (key_event == KEY_SHORT_PRESS)
                {
                    Timer_Pause();
                    sys_state = SYS_PAUSE;
                }
                // 运行状态下长按通常忽略，或者也可以定义为暂停
                break;

            case SYS_PAUSE:
                // 获取时间（时间已静止）并显示
                current_time_ms = Timer_GetTotalTimeMs();
                Seg_DispTime(current_time_ms * 10);
                
                // 事件：短按 -> 继续运行
                if (key_event == KEY_SHORT_PRESS)
                {
                    Timer_Resume();
                    sys_state = SYS_RUNNING;
                }
                // 事件：长按 -> 清除归零
                else if (key_event == KEY_LONG_PRESS)
                {
                    Timer_Reset(); // 归零并保持暂停
                    sys_state = SYS_IDLE;
                }
                break;
        }
    }
}

/**
  * 函    数：TIM2中断函数
  * 说    明：由Timer.c中的逻辑管理，此处保留以防链接错误，
  * 但建议将中断服务函数移至Timer.c中统一管理。
  * 如果Timer.c里已经有IRQHandler，这里需要删掉，否则报错。
  * (根据下文Timer.c的修改，这里不需要了，因为移到了Timer.c中)
  */