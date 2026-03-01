#include "main_logic.h"
#include "Key.h"
#include "Timer.h"
#include "seg_display.h"
#include "Buzzer.h"
#include "LightSensor.h"
#include "NRF24L01.h"         
#include "NRF24L01_Define.h"  
#include "Delay.h"
#include <string.h>
#include <stdio.h>

// --- 系统状态 (System States) ---
typedef enum {
    SYS_INIT_HANDSHAKE, 
    SYS_IDLE,           
    SYS_RUNNING,        
    SYS_PAUSE           
} SysState_t;

static SysState_t sys_state = SYS_INIT_HANDSHAKE;

// 握手重试计数
static uint32_t handshake_timer = 0;
static uint8_t handshake_retry = 0;

extern void Seg_ShowString(const char *str); 

/**
 * @brief  辅助函数：发送指令
 */
static void Send_Cmd(const char *cmd) {
    // 发送前清空底层 FIFO 缓存
    NRF24L01_FlushTx(); 
    NRF24L01_FlushRx(); 
    
    memcpy(NRF24L01_TxPacket, cmd, 4);      
    NRF24L01_Send();  // 阻塞发送
    NRF24L01_Rx();    // 切换接收
}

// --- 接口实现 ---

void MainLogic_Init(void)
{
    sys_state = SYS_INIT_HANDSHAKE;
    handshake_retry = 0;
    
    Timer_Reset();
    NRF24L01_Init(); 
    
    uint8_t check_val = NRF24L01_ReadReg(NRF24L01_CONFIG);
    if(check_val == 0x00) { while(1) { Seg_ShowString("0002"); } }
    if(check_val == 0xFF) { while(1) { Seg_ShowString("0004"); } }
    
    NRF24L01_Rx();
}

/**
 * @brief 无线检查
 */
void MainLogic_CheckWireless(void)
{
    if(NRF24L01_Receive() == 1) 
    {
        // 拷贝指令后，立刻清零接收数组，防止硬件误触发导致重复读取同一包
        char cmd_buf[5] = {0};
        memcpy(cmd_buf, NRF24L01_RxPacket, 4);
        memset(NRF24L01_RxPacket, 0, 4);

        // 1. 收到开始指令 "STAR"
        if(strncmp(cmd_buf, "STAR", 4) == 0) {
            if(sys_state == SYS_IDLE || sys_state == SYS_PAUSE) {
                Timer_Reset();
                // 启动前清除 STM32 的硬件挂起中断，防止秒数突变
                TIM_ClearFlag(TIM2, TIM_FLAG_Update); 
                Timer_Resume(); 
                sys_state = SYS_RUNNING;
            }
        }
        // 2. 收到暂停/停止指令 "STOP"
        else if(strncmp(cmd_buf, "STOP", 4) == 0) {
            // 给无线接收也加上盲区保护,防止对方一开跑就发来错乱的STOP信号
            if(sys_state == SYS_RUNNING && Timer_GetTotalTimeMs() > 1000) {
                Timer_Pause();
                sys_state = SYS_PAUSE;
            }
        }
        // 3. 收到复位指令 "RSET"
        else if(strncmp(cmd_buf, "RSET", 4) == 0) {
            Timer_Reset();
            sys_state = SYS_IDLE;
        }
        // 4. 收到握手请求 "PING"
        else if(strncmp(cmd_buf, "PING", 4) == 0) {
            if(sys_state == SYS_INIT_HANDSHAKE) {
                sys_state = SYS_IDLE; 
            }
        }
    }
}

/**
 * @brief 状态机与事件处理
 */
void Handle_StateMachine(uint8_t event)
{
    if(sys_state == SYS_INIT_HANDSHAKE) {
        handshake_timer++;
        if(handshake_timer > 300) { 
            handshake_timer = 0;
            if(handshake_retry < 5) {
                memcpy(NRF24L01_TxPacket, "PING", 4);
                if(NRF24L01_Send() == 1) { 
                    sys_state = SYS_IDLE;
                }
                NRF24L01_Rx(); 
                handshake_retry++;
            } else {
                sys_state = SYS_IDLE; 
            }
        }
        return;
    }

    // --- 终点逻辑：光敏传感器触发 ---
    if(sys_state == SYS_RUNNING) {
        // 传感器盲区保护
        if(LightSensor_Get() == 1 && Timer_GetTotalTimeMs() > 1000) { 
            Timer_Pause();       
            Send_Cmd("STOP");    
            sys_state = SYS_PAUSE;
        }
    }

    // --- 按键逻辑 ---
    switch (sys_state)
    {
        case SYS_IDLE:
        case SYS_PAUSE:
            if (event == KEY1_PRESS_SHORT) {
                Send_Cmd("STAR"); 
                Timer_Reset();
                TIM_ClearFlag(TIM2, TIM_FLAG_Update); // 同样强制清除中断挂起
                Timer_Resume(); 
                sys_state = SYS_RUNNING;
            }
            else if (event == KEY1_PRESS_LONG) {
                Send_Cmd("RSET");
                Timer_Reset();
                sys_state = SYS_IDLE;
            }
            break;

        case SYS_RUNNING:
            if (event == KEY1_PRESS_SHORT) {
                // 【启用按键盲区】：强制拦截前 1000ms 内的所有按键，绝对防止双击/抖动导致的误停！
                if(Timer_GetTotalTimeMs() < 1000) {
					Seg_ShowWAIT();
					break;
				} 
                
                Timer_Pause();
                Send_Cmd("STOP");
                sys_state = SYS_PAUSE;
            }
            else if (event == KEY1_PRESS_LONG) {
                Timer_Reset();
                Send_Cmd("RSET");
                sys_state = SYS_IDLE;
            }
            break;
    }
}

/**
 * @brief 显示刷新逻辑
 */
void Handle_Display(void)
{
    uint32_t total_ms = 0;

    switch (sys_state)
    {
        case SYS_INIT_HANDSHAKE:
            if(handshake_retry < 5) Seg_ShowCONN();
            else Seg_ShowWAIT();
            break;

        case SYS_IDLE:
            Seg_ShowREADY(); 
            break;

        case SYS_RUNNING:
            total_ms = Timer_GetTotalTimeMs(); 
            Seg_DispTime(total_ms * 10);
            break;

        case SYS_PAUSE:
            total_ms = Timer_GetTotalTimeMs();
            Seg_DispTime(total_ms * 10);
            break;
    }
}
