#include "main_logic.h"
#include "Key.h"
#include "Timer.h"
#include "seg_display.h"
#include "Buzzer.h"
#include "LightSensor.h"
#include "NRF24L01.h"         // 替换为 NRF24L01 驱动头文件
#include "NRF24L01_Define.h"  // 包含寄存器定义以便进行硬件检查
#include "Delay.h"
#include <string.h>
#include <stdio.h>

// --- 系统状态 (System States) ---
typedef enum {
    SYS_INIT_HANDSHAKE, // 上电握手阶段 (Power-on handshake phase)
    SYS_IDLE,           // 就绪态 (Idle/Ready state)
    SYS_RUNNING,        // 计时中 (Running/Timing state)
    SYS_PAUSE           // 结束/暂停 (Finished/Paused state)
} SysState_t;

static SysState_t sys_state = SYS_INIT_HANDSHAKE;

// 握手重试计数 (Handshake retry counters)
static uint32_t handshake_timer = 0;
static uint8_t handshake_retry = 0;

// 外部显示接口声明 (External display interface declaration)
extern void Seg_ShowString(const char *str); 

/**
 * @brief  辅助函数：发送指令
 * @note   使用 memcpy 防止 '\0' 越界溢出4字节数组。发送完成后立刻切回接收模式
 */
static void Send_Cmd(const char *cmd) {
    // 强制只拷贝4个有效字符，不拷贝结束符'\0'
    memcpy(NRF24L01_TxPacket, cmd, 4);      
    NRF24L01_Send();  // 阻塞发送，直到收到接收端的ACK应答才返回 (同步关键点)
    NRF24L01_Rx();    // 发送完毕后重新进入接收模式
}

// --- 接口实现 (Interface Implementation) ---

void MainLogic_Init(void)
{
    sys_state = SYS_INIT_HANDSHAKE;
    handshake_retry = 0;
    
    Timer_Reset();
    NRF24L01_Init(); 
	Timer_Init();
    
    // 硬件检查：如果模块没插好，读取出的值通常全0或全FF，卡死显示 Err
    uint8_t check_val = NRF24L01_ReadReg(NRF24L01_CONFIG);
    if(check_val == 0x00) {
        while(1) { Seg_ShowString("0002"); }
    }
    if(check_val == 0xFF) {
        while(1) { Seg_ShowString("0004"); }
    }
    
    // 初始化完成，切入接收模式准备握手
    NRF24L01_Rx();
}

/**
 * @brief 主循环中调用的无线检查 (Wireless signal polling)
 */
void MainLogic_CheckWireless(void)
{
    // NRF24L01_Receive 返回1代表成功收到数据包
    if(NRF24L01_Receive() == 1) 
    {
        // 1. 收到开始指令 "STAR"
        if(strncmp((char*)NRF24L01_RxPacket, "STAR", 4) == 0) {
            if(sys_state == SYS_IDLE || sys_state == SYS_PAUSE) {
                Timer_Reset();
                Timer_Resume(); // 接收端立刻启动定时器 (与发送端ACK同步)
                sys_state = SYS_RUNNING;
            }
        }
        // 2. 收到暂停/停止指令 "STOP"
        else if(strncmp((char*)NRF24L01_RxPacket, "STOP", 4) == 0) {
            if(sys_state == SYS_RUNNING) {
                Timer_Pause();
                sys_state = SYS_PAUSE;
            }
        }
        // 3. 收到复位指令 "RSET"
        else if(strncmp((char*)NRF24L01_RxPacket, "RSET", 4) == 0) {
            Timer_Reset();
            sys_state = SYS_IDLE;
        }
        // 4. 收到握手请求 "PING"
        else if(strncmp((char*)NRF24L01_RxPacket, "PING", 4) == 0) {
            if(sys_state == SYS_INIT_HANDSHAKE) {
                sys_state = SYS_IDLE; 
            }
        }
    }
}

/**
 * @brief 状态机与事件处理 (State Machine & Event Handling)
 */
void Handle_StateMachine(uint8_t event)
{
    // --- 1. 初始化握手逻辑 ---
    if(sys_state == SYS_INIT_HANDSHAKE) {
        handshake_timer++;
        if(handshake_timer > 300) { 
            handshake_timer = 0;
            if(handshake_retry < 5) {
                // 主动发 PING，不使用 strcpy
                memcpy(NRF24L01_TxPacket, "PING", 4);
                if(NRF24L01_Send() == 1) { // 1代表发送成功并收到ACK
                    sys_state = SYS_IDLE;
                }
                NRF24L01_Rx(); 
                handshake_retry++;
            } else {
                sys_state = SYS_IDLE; // 重试失败强行进入IDLE
            }
        }
        return;
    }

    // --- 2. 终点逻辑：光敏传感器触发 (End point trigger) ---
    if(sys_state == SYS_RUNNING) {
        if(LightSensor_Get() == 1) { 
            Timer_Pause();       // 先停本地定时器
            Send_Cmd("STOP");    // 马上通知起点端停止
            sys_state = SYS_PAUSE;
        }
    }

    // --- 3. 按键逻辑 (Key logic: supports start/stop/reset at BOTH ends) ---
    switch (sys_state)
    {
        case SYS_IDLE:
			if (event == KEY1_PRESS_SHORT){
				Send_Cmd("STAR"); 
                Timer_Reset();      // 清零
                Timer_Resume();     // 开始
                sys_state = SYS_RUNNING;				
            }
			break;
		
        case SYS_PAUSE:
            if (event == KEY1_PRESS_SHORT) {
                // 短按启动：先发无线指令，阻塞等待ACK返回后再本地启动定时器
                Send_Cmd("STAR"); 
                Timer_Reset();
                Timer_Resume(); // 发送端在收到ACK后立刻启动 (与接收端同步)
                sys_state = SYS_RUNNING;
            }
            else if (event == KEY1_PRESS_LONG) {
                // 长按复位
                Send_Cmd("RSET");
                Timer_Reset();
                sys_state = SYS_IDLE;
            }
            break;

        case SYS_RUNNING:
            if (event == KEY1_PRESS_SHORT) {
                // 短按暂停
                Timer_Pause();
                Send_Cmd("STOP");
                sys_state = SYS_PAUSE;
            }
            else if (event == KEY1_PRESS_LONG) {
                // 长按复位
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
            Seg_ShowREADY(); // 显示 READY (此处保留原有宏定义格式)
            break;

        case SYS_RUNNING:
            // 实时显示
			total_ms = Timer_GetTotalTimeMs(); // 返回值为ms
            Seg_DispTime(total_ms * 10);
            break;

        case SYS_PAUSE:
            // 显示定格时间
            total_ms = Timer_GetTotalTimeMs();
            Seg_DispTime(total_ms * 10);
            break;
    }
}
