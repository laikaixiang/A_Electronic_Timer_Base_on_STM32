#include "Key.h"

// 阈值设定（基于主循环一次大约10ms的估算）
// 短按消抖：约40ms (4 * 10ms)
// 长按判定：约800ms (80 * 10ms)
#define CNT_DEBOUNCE    4
#define CNT_LONG_PRESS  80 

void Key_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // 上拉输入
    GPIO_InitStructure.GPIO_Pin = KEY_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(KEY_GPIO_PORT, &GPIO_InitStructure);
}

/**
  * 函    数：获取按键事件（支持长短按）
  * 参    数：无
  * 返 回 值：event
  *         0：无按键
  *         1：按键短按
  *         2：按键长按
  * 说    明：非阻塞式改造（也可根据需求改为阻塞式）
  */
uint8_t Key_GetEvent(void)
{
    static uint8_t key_state = 0;       // 0:无按键, 1:按下确认中, 2:等待释放(已触发长按)
    static uint32_t press_cnt = 0;      // 按下计数器

    uint8_t current_level = GPIO_ReadInputDataBit(KEY_GPIO_PORT, KEY_PIN);
    uint8_t event = KEY_NONE;

    // 检测到低电平（按下）
    if (current_level == 0)
    {
        if (key_state == 0 || key_state == 1)
        {
            press_cnt++; // 持续按下，计数增加
            key_state = 1;

            // 达到长按阈值
            if (press_cnt >= CNT_LONG_PRESS)
            {
                event = KEY_LONG_PRESS; // 触发长按事件
                key_state = 2;          // 进入等待释放状态，避免重复触发
            }
        }
    }
	
    // 检测到高电平（松开）
    else
    {
        if (key_state == 1) // 之前是按下状态
        {
            // 如果按下时间超过消抖阈值，且未达到长按
            if (press_cnt >= CNT_DEBOUNCE && press_cnt < CNT_LONG_PRESS)
            {
                event = KEY_SHORT_PRESS; // 触发短按事件
            }
        }
        
        // 复位状态
        key_state = 0;
        press_cnt = 0;
    }

    return event;
}