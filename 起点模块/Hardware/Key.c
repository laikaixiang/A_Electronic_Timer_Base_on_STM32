#include "Key.h"

// --- 参数配置 ---
#define CNT_DEBOUNCE    4   // 消抖 (4*10ms = 40ms)
#define CNT_LONG_PRESS  80  // 长按 (80*10ms = 800ms)

// --- 1. 定义按键对象结构体 ---
typedef struct {
    GPIO_TypeDef* GPIOx;    // 端口
    uint16_t GPIO_Pin;      // 引脚
    uint8_t  State;         // 状态机状态 (0:空闲, 1:按下, 2:长按保持)
    uint32_t PressCnt;      // 按下计时
    uint8_t  ShortEventID;  // 短按对应的事件码
    uint8_t  LongEventID;   // 长按对应的事件码
} Key_t;

// --- 2. 按键列表 (在这里添加更多按键!) ---
// 假设：Key1在 PA1, Key2在 PA2, Key3在 PB5
static Key_t Key_List[] = {
    // 端口,   引脚,       初始状态, 计数,  短按ID,            长按ID
    {GPIOA, GPIO_Pin_1,   0,       0,   KEY1_PRESS_SHORT, KEY1_PRESS_LONG}, 
//    {GPIOA, GPIO_Pin_2,   0,       0,   KEY2_PRESS_SHORT, KEY2_PRESS_LONG},
    // {GPIOB, GPIO_Pin_5,   0,       0,   KEY3_PRESS_SHORT, KEY3_PRESS_LONG}, // 扩展示例
};

// 计算按键数量
#define KEY_COUNT (sizeof(Key_List) / sizeof(Key_List[0]))

// --- 3. 初始化所有按键 ---
/**
  * 函    数：按键初始化
  * 参    数：无
  * 返 回 值：无
  */
void Key_Init(void)
{
    // 开启常用端口时钟 (根据实际情况开启，多开无害)
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // 上拉输入
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    // 循环初始化列表中的所有GPIO
    for(int i = 0; i < KEY_COUNT; i++)
    {
        GPIO_InitStructure.GPIO_Pin = Key_List[i].GPIO_Pin;
        GPIO_Init(Key_List[i].GPIOx, &GPIO_InitStructure);
    }
}

// --- 4. 扫描所有按键 (非阻塞) ---
/**
  * 函    数：获取按键事件（支持长短按）
  * 参    数：无
  * 返 回 值：0:无, 1-9:1-9短按, 10-19:1-9长按
  * 说    明：非阻塞式改造（也可根据需求改为阻塞式）
  */
uint8_t Key_GetEvent(void)
{
    // 遍历每一个按键
    for(int i = 0; i < KEY_COUNT; i++)
    {
        // 获取物理状态 (0=按下, 1=松开)
        uint8_t pin_level = GPIO_ReadInputDataBit(Key_List[i].GPIOx, Key_List[i].GPIO_Pin);
        
        // --- 状态机逻辑 ---
        if (pin_level == 0) // 按下
        {
            if (Key_List[i].State == 0 || Key_List[i].State == 1)
            {
                Key_List[i].PressCnt++;
                Key_List[i].State = 1; // 确认按下状态

                // 长按判定
                if (Key_List[i].PressCnt >= CNT_LONG_PRESS)
                {
                    Key_List[i].State = 2; // 锁定为长按状态
                    return Key_List[i].LongEventID; // 【立即返回事件】
                }
            }
        }
        else // 松开
        {
            if (Key_List[i].State == 1) // 之前是按下，现在松开了
            {
                // 短按判定
                if (Key_List[i].PressCnt >= CNT_DEBOUNCE)
                {
                    // 复位并返回
                    Key_List[i].State = 0;
                    Key_List[i].PressCnt = 0;
                    return Key_List[i].ShortEventID; // 【立即返回事件】
                }
            }
            
            // 无论长按还是短按，松手后都要复位
            Key_List[i].State = 0;
            Key_List[i].PressCnt = 0;
        }
    }
    
    return KEY_NONE; // 本轮扫描没有有效事件
}