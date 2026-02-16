#ifndef __SEG_DISPLAY_H
#define __SEG_DISPLAY_H

#include "stm32f10x.h"
#include "string.h"
#include "stdlib.h"

/*************************** 引脚配置（PB3开始）***************************/
// 段脚定义（a-g + dp）
#define SEG_PORT        GPIOB
#define SEG_PIN_A       GPIO_Pin_3
#define SEG_PIN_B       GPIO_Pin_4
#define SEG_PIN_C       GPIO_Pin_5
#define SEG_PIN_D       GPIO_Pin_6
#define SEG_PIN_E       GPIO_Pin_7
#define SEG_PIN_F       GPIO_Pin_8
#define SEG_PIN_G       GPIO_Pin_9
#define SEG_PIN_DP      GPIO_Pin_10

// 位选定义（四位数码管：千位-百位-十位-个位）
#define BIT_PORT        GPIOB
#define BIT_PIN_UNIT    GPIO_Pin_11  // 个位
#define BIT_PIN_TEN     GPIO_Pin_12  // 十位
#define BIT_PIN_HUND    GPIO_Pin_13  // 百位
#define BIT_PIN_THOU    GPIO_Pin_14  // 千位

// 所有段脚/位选脚合集
#define ALL_SEG_PINS    (SEG_PIN_A | SEG_PIN_B | SEG_PIN_C | SEG_PIN_D | \
                         SEG_PIN_E | SEG_PIN_F | SEG_PIN_G | SEG_PIN_DP)
#define ALL_BIT_PINS    (BIT_PIN_UNIT | BIT_PIN_TEN | BIT_PIN_HUND | BIT_PIN_THOU)

/*************************** 计时器参数配置 ***************************/
#define TIMER_MAX_VALUE 999.9f       // 计时器最大值（四位有效数字上限）
#define TIMER_REFRESH_MS 10          // 计时器刷新周期（10ms=0.01s精度）
#define TIMER_PRECISION 0.01f        // 计时器精度（0.01秒）

/*************************** 函数声明 ***************************/
/**
 * @brief  四位数码管+计时器初始化
 * @param  无
 * @retval 无
 */
void Seg_Init(void);

/**
 * @brief  计时器核心刷新函数（需定时调用，建议10ms一次）
 * @note   包含数码管动态扫描+计时器数值更新
 * @param  无
 * @retval 无
 */
void Seg_Timer_Refresh(void);

/**
 * @brief  控制计时器启停
 * @param  start: 1=启动计时，0=停止计时
 * @retval 无
 */
void Seg_Timer_Control(uint8_t start);

/**
 * @brief  重置计时器（清零）
 * @param  无
 * @retval 无
 */
void Seg_Timer_Reset(void);

/**
 * @brief  获取当前计时值
 * @param  无
 * @retval 当前计时值（单位：秒，保留两位小数）
 */
float Seg_Timer_GetValue(void);

/**
 * @brief  熄灭四位数码管
 * @param  无
 * @retval 无
 */
void Seg_Off(void);

/**
 * @brief  直接控制某一位数码管显示某一数字
 * @param  pos：位数，取值0~3
			num: 数字，取值0~9
 * @retval 无
 */
void Seg_ShowSingleNum(uint8_t pos, uint8_t num, uint8_t show_dp);

/**
 * @brief  直接控制数码管显示数字
 * @param  num: 数字，范围（0.000~1000)，保证四位有效即可
 * @retval 无
 */
void Seg_ShowString(const char *num_str);

void Seg_Refresh(void);

void Timer_Ms2Str(uint32_t ms, char *buf);

void Seg_ShowREADY(void);

void Seg_DispTime(uint32_t Timer_ARR);

#endif /* __SEG_DISPLAY_H */
