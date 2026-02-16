#include "main_logic.h"
#include "Key.h"
#include "Timer.h"
#include "seg_display.h"
#include "Buzzer.h"  // 假设你有蜂鸣器，用于扩展演示
#include "LED.h"     // 假设你有LED

// --- 1. 定义系统状态 ---
typedef enum {
    SYS_IDLE,    // 空闲/就绪态 (显示 READY)
    SYS_RUNNING, // 运行态 (计时中)
    SYS_PAUSE    // 暂停态 (显示停止的时间)
} SysState_t;

// --- 2. 内部全局变量 ---
static SysState_t sys_state = SYS_IDLE; // 当前状态

// --- 3. 内部辅助函数声明 ---
static void Handle_StateMachine(uint8_t event);
static void Handle_Display(void);

// --- 4. 接口实现 ---

// 初始化逻辑
void MainLogic_Init(void)
{
    sys_state = SYS_IDLE;
    Timer_Reset(); // 确保定时器归零
}

//// 主循环函数
//void MainLogic_Loop(void)
//{
//    // 1. 获取输入 (键盘)
//    uint8_t key_event = Key_GetEvent();

//    // 2. 处理逻辑 (状态机 + 事件处理)
//    Handle_StateMachine(key_event);

//    // 3. 处理输出 (显示/LED/蜂鸣器)
//    Handle_Display();
//}

// --- 5. 核心逻辑实现 ---

/**
 * @brief 状态机核心处理
 * @param event 从Key_GetEvent获取到的按键事件
 */
static void Handle_StateMachine(uint8_t event)
{
    switch (sys_state)
    {
        // ---------------- [状态：空闲] ----------------
        case SYS_IDLE:
            if (event == KEY1_PRESS_SHORT)
            {
                Timer_Reset();      // 清零
                Timer_Resume();     // 开始
                sys_state = SYS_RUNNING; 
            }
            // 可以在这里扩展其他按键，例如 KEY2 设置时间等
            break;

        // ---------------- [状态：运行中] ----------------
        case SYS_RUNNING:
            if (event == KEY1_PRESS_SHORT)
            {
                Timer_Pause();      // 暂停
                sys_state = SYS_PAUSE;
            }
            // 运行中通常忽略长按，或者你可以定义长按直接复位
            break;

        // ---------------- [状态：暂停] ----------------
        case SYS_PAUSE:
            if (event == KEY1_PRESS_SHORT)
            {
                Timer_Resume();     // 继续
                sys_state = SYS_RUNNING;
            }
            else if (event == KEY1_PRESS_LONG)
            {
                Timer_Reset();      // 复位
                sys_state = SYS_IDLE;
            }
            break;
    }

    // --- 全局通用按键 (无论什么状态都能响应) ---
    // 例如：KEY2 无论何时按下都切换蜂鸣器
    if (event == KEY2_PRESS_SHORT)
    {
        // Buzzer_Toggle(); // 你的蜂鸣器代码
    }
}

/**
 * @brief 显示刷新逻辑
 */
static void Handle_Display(void)
{
    uint32_t total_ms = 0;

    switch (sys_state)
    {
        case SYS_IDLE:
            Seg_ShowREADY(); // 显示 "rdy" 或 "0000"
            break;

        case SYS_RUNNING:
            // 实时获取时间
            total_ms = Timer_GetTotalTimeMs();
            // 注意：Seg_DispTime 输入单位如果是 0.1ms (取决于你的seg实现)，这里要 *10
            // 如果你的 Seg_DispTime 输入就是 ms，则不需要 *10
            Seg_DispTime(total_ms * 10); 
            break;

        case SYS_PAUSE:
            // 获取定格的时间
            total_ms = Timer_GetTotalTimeMs();
            Seg_DispTime(total_ms * 10);
            break;
    }
}