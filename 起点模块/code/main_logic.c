#include "main_logic.h"
#include "Key.h"
#include "Timer.h"
#include "seg_display.h"
#include "Buzzer.h"
#include "LightSensor.h"
#include "gt24_drv.h"
#include <string.h>
#include <stdio.h>

// --- 系统状态 ---
typedef enum {
    SYS_INIT_HANDSHAKE, // 上电握手阶段
    SYS_IDLE,           // 就绪态
    SYS_RUNNING,        // 计时中
    SYS_PAUSE           // 结束/暂停
} SysState_t;

static SysState_t sys_state = SYS_INIT_HANDSHAKE;
static uint8_t rx_buf[32];
static uint8_t tx_buf[32];

// 握手重试计数
static uint32_t handshake_timer = 0;
static uint8_t handshake_retry = 0;

// 外部显示接口声明
extern void Seg_ShowString(const char *str); 

// 辅助：发送指令后立即切回接收
static void Send_Cmd(const char *cmd) {
    memset(tx_buf, 0, 32);
    strcpy((char*)tx_buf, cmd);
    GT24_TxPacket(tx_buf);
    GT24_SetRxMode();
}

// --- 接口实现 ---

void MainLogic_Init(void)
{
    sys_state = SYS_INIT_HANDSHAKE;
    handshake_retry = 0;
    
    Timer_Reset();
    GT24_Init();
    
    // 硬件检查：如果模块没插好，卡死显示 Err
    if(GT24_Check() != 0) {
        while(1) { Seg_ShowString("0001"); }
    }
    
    // 初始化完成，切入接收模式准备握手
    GT24_SetRxMode();
}

// 主循环中调用的无线检查
void MainLogic_CheckWireless(void)
{
    if(GT24_RxPacket(rx_buf)) 
    {
        // 1. 收到握手请求 "PING"
        if(strncmp((char*)rx_buf, "PING", 4) == 0) {
            if(sys_state == SYS_INIT_HANDSHAKE) {
                // 握手成功，发出提示音，进入就绪
                Buzzer_ON(); 
                for(int i=0;i<20000;i++); // 极短延时提示音
                Buzzer_OFF();
                sys_state = SYS_IDLE; 
            }
        }
        // 2. 收到开始指令 "STAR"
        else if(strncmp((char*)rx_buf, "STAR", 4) == 0) {
            // 只有在空闲或暂停时才允许远程启动
            if(sys_state == SYS_IDLE || sys_state == SYS_PAUSE) {
                Timer_Reset();
                Timer_Resume();
                sys_state = SYS_RUNNING;
            }
        }
    }
}

void Handle_StateMachine(uint8_t event)
{
    // --- 1. 初始化握手逻辑 (非阻塞) ---
    if(sys_state == SYS_INIT_HANDSHAKE) {
        handshake_timer++;
        // 利用主循环计数做非阻塞延时
        if(handshake_timer > 300) { 
            handshake_timer = 0;
            if(handshake_retry < 5) {
                // 主动发 PING
                memset(tx_buf, 0, 32);
                strcpy((char*)tx_buf, "PING");
                if(GT24_TxPacket(tx_buf) == 1) {
                    // 收到ACK，对方在线
                    sys_state = SYS_IDLE;
                    Buzzer_ON(); for(int i=0;i<20000;i++); Buzzer_OFF();
                }
                GT24_SetRxMode();
                handshake_retry++;
            } else {
                // 重试5次失败，放弃主动，显示等待 (Wait)
                // 用户可以按键强制进入IDLE，或者等待对方发PING
                // 这里为了方便，直接进IDLE但显示Wait
                sys_state = SYS_IDLE;
            }
        }
        return;
    }

    // --- 2. 终点逻辑：光敏传感器 ---
    if(sys_state == SYS_RUNNING) {
        if(LightSensor_Get() == 1) { // 假设1触发
            Timer_Pause();
            sys_state = SYS_PAUSE;
            Buzzer_ON(); for(int i=0;i<50000;i++); Buzzer_OFF();
        }
    }

    // --- 3. 按键逻辑 ---
    switch (sys_state)
    {
        case SYS_IDLE:
        case SYS_PAUSE:
            if (event == KEY1_PRESS_SHORT) {
                // 发令：先发无线，再跑本地
                Send_Cmd("STAR");
                Timer_Reset();
                Timer_Resume();
                sys_state = SYS_RUNNING;
            }
            else if (event == KEY1_PRESS_LONG) {
                Timer_Reset();
                sys_state = SYS_IDLE;
            }
            break;

        case SYS_RUNNING:
            if (event == KEY1_PRESS_SHORT) {
                Timer_Pause();
                sys_state = SYS_PAUSE;
            }
            break;
    }
}

/**
 * @brief 显示刷新逻辑
 * @note  完全保留了你之前的结构，只修改了具体的显示函数调用
 */
void Handle_Display(void)
{
    uint32_t total_ms = 0;
    char disp_buf[10];

    switch (sys_state)
    {
        case SYS_INIT_HANDSHAKE:
            if(handshake_retry < 5) Seg_ShowString("Conn");
            else Seg_ShowString("Wait");
            break;

        case SYS_IDLE:
            Seg_ShowREADY; // 显示 READY
            break;

        case SYS_RUNNING:
            // 实时显示
            total_ms = Timer_GetTotalTimeMs();
            // 假设Timer_GetTotalTimeMs返回的是毫秒，显示格式为 秒.毫秒
            sprintf(disp_buf, "%04d", (int)(total_ms / 10)); 
            Seg_ShowString(disp_buf);
            break;

        case SYS_PAUSE:
            // 显示定格时间
            total_ms = Timer_GetTotalTimeMs();
            sprintf(disp_buf, "%04d", (int)(total_ms / 10));
            Seg_ShowString(disp_buf);
            break;
    }
}
