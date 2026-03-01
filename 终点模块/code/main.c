#include "stm32f10x.h"
#include "Delay.h"
#include "Buzzer.h"
#include "LightSensor.h"
#include "LED.h"
#include "seg_display.h"
#include "Key.h"
#include "Timer.h"
#include "main_logic.h"

// 声明外部函数，确保编译器不报错
extern void MainLogic_CheckWireless(void);

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
    
    // 如果你有 Delay_Init 请在这里调用
    // Delay_Init(); 

    /* 2. 业务逻辑层初始化 (App Layer) */
    MainLogic_Init();

    /* 3. 主循环 */
    while(1)
    {
        // 1. 获取输入 (键盘)
        uint8_t key_event = Key_GetEvent();

        // 2. 处理逻辑 (状态机 + 事件处理)
        Handle_StateMachine(key_event);
        
        // 3. 【新增】检查无线数据 (必须在此处轮询)
        // 包含了接收远程 START 信号和 PING 握手信号
        MainLogic_CheckWireless();

        // 4. 处理输出 (显示/LED/蜂鸣器)
        // 这里必须高频调用，不能被阻塞
        Handle_Display();
        
        // 5. 如果需要控制循环频率，可以使用极短的延时
        // 但为了数码管刷新效果，建议不要加长延时
		// Delay_us(500); 
    }
}
