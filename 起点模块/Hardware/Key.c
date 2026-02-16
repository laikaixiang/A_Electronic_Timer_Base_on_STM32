#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "Key.h"
#include "seg_display.h"
#include "Timer.h"

// 按键状态枚举
typedef enum
{
    KEY_NONE = 0,       // 0：无按键
    KEY1_SHORT_PRESS,  	// 1：按键1短按
	KEY1_LONG_PRESS,   	// 2：按键1长按
	KEY2_SHORT_PRESS,  	// 3：按键2短按
	KEY2_LONG_PRESS   	// 4：按键2长按
} Key_StatusTypeDef;

// 长按判定阈值（可根据需求调整，单位ms）
#define KEY_LONG_TIME    1000    
// 消抖阈值
#define KEY_DEBOUNCE_TIME 20  

/**
  * 函    数：按键初始化
  * 参    数：无
  * 返 回 值：无
  */
void Key_Init(void)
{
	/*开启时钟*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);		//开启GPIOA的时钟
	
	/*GPIO初始化*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = Key_1 | Key_1;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);						//将PA0和PA1引脚初始化为上拉输入
}

/**
  * 函    数：按键获取键码
  * 参    数：无
  * 返 回 值：按下按键的键码值，返回0则代表没有按键按下
  * 注意事项：此函数是阻塞式操作，当按键按住不放时，函数会卡住，直到按键松手
  */
uint8_t Key_GetNum(void)
{
	uint8_t KeyNum = 0;		//定义变量，默认键码值为0
	
	if (GPIO_ReadInputDataBit(GPIOA, Key_1) == 0)			//读PA0输入寄存器的状态，如果为0，则代表按键1按下
	{
		Delay_ms(20);											//延时消抖
		while (GPIO_ReadInputDataBit(GPIOA, Key_1) == 0);	//等待按键松手
		Delay_ms(20);											//延时消抖
		KeyNum = 1;												//置键码为1
	}
	
	if (GPIO_ReadInputDataBit(GPIOA, Key_1) == 0)			//读PA1输入寄存器的状态，如果为0，则代表按键2按下
	{
		Delay_ms(20);											//延时消抖
		while (GPIO_ReadInputDataBit(GPIOA, Key_1) == 0);	//等待按键松手
		Delay_ms(20);											//延时消抖
		KeyNum = 2;												//置键码为2
	}
	
	return KeyNum;			//返回键码值，如果没有按键按下，所有if都不成立，则键码为默认值0
}

/**
  * 函    数：获取按键事件（支持长短按）
  * 参    数：无
  * 返 回 值：
  *         0：无按键
  *         1：按键1短按
  *         2：按键1长按
  *         3：按键2短按
  *         4：按键2长按
  * 说    明：非阻塞式改造（也可根据需求改为阻塞式）
  */
uint8_t Key_GetEvent(void)
{
    uint8_t KeyEvent = 0;
    uint32_t PressTime = 0;

    // 检测按键1（PA1）
    if(Key_GetNum() == 1)
    {
        // 记录按下时长
        while(GPIO_ReadInputDataBit(GPIOA, Key_1) == 0)
        {
            Delay_ms(5);
            PressTime += 5;
            Seg_ShowREADY();
            // 防止死循环：超过长按阈值后退出计数
            if(PressTime >= KEY_LONG_TIME)
            {
                break;
            }
        }

        // 判断长短按
        if(PressTime >= KEY_LONG_TIME)
        {
            KeyEvent = 2;  // 按键1长按
        }
        else if(PressTime >= KEY_DEBOUNCE_TIME)  // 有效短按（至少超过消抖时间）
        {
            KeyEvent = 1;  // 按键1短按
        }
        
        // 松手消抖
        Delay_ms(KEY_DEBOUNCE_TIME);
    }

    // 检测按键2（PA2）
    if(Key_GetNum() == 2)
    {
        PressTime = 0;
        // 记录按下时长
        while(GPIO_ReadInputDataBit(GPIOA, Key_2) == 0)
        {
            Delay_ms(5);
            PressTime += 5;
            
            if(PressTime >= KEY_LONG_TIME)
            {
                break;
            }
        }

        // 判断长短按
        if(PressTime >= KEY_LONG_TIME)
        {
            KeyEvent = 4;  // 按键2长按
        }
        else if(PressTime >= KEY_DEBOUNCE_TIME)
        {
            KeyEvent = 3;  // 按键2短按
        }
        
        // 松手消抖
        Delay_ms(KEY_DEBOUNCE_TIME);
    }

    return KeyEvent;
}
