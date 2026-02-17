#include "stm32f10x.h"
#include "Delay.h"
#include "Buzzer.h"
#include "LightSensor.h"
#include "LED.h"
#include "seg_display.h"
#include "Key.h"
#include "Timer.h"
#include "main_logic.h"

int main(void)
{
    /* 1. 硬件驱动层初始化 (BSP Layer) */
    SystemInit();       
    Buzzer_Init();
    LightSensor_Init();
    LED_Init();
    Key_Init();
    Seg_Init();
    Timer_Init();

    /* 2. 业务逻辑层初始化 (App Layer) */
    MainLogic_Init();

    /* 3. 主循环 */
    while(1)
    {
        // 1. 获取输入 (键盘)
		uint8_t key_event = Key_GetEvent();

		// 2. 处理逻辑 (状态机 + 事件处理)
		Handle_StateMachine(key_event);

		// 3. 处理输出 (显示/LED/蜂鸣器)
		Handle_Display();
    }
}