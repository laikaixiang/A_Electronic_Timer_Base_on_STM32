#ifndef __MAX7219_DISPLAY_H
#define __MAX7219_DISPLAY_H

#include "stm32f10x.h"

// ------------------------- 引脚配置 -------------------------
#define Max7219_PORT        GPIOB
#define Max7219_CLK_PIN     GPIO_Pin_5
#define Max7219_CS_PIN      GPIO_Pin_6
#define Max7219_DIN_PIN     GPIO_Pin_7

#define Max7219_CLK_0()     GPIO_ResetBits(Max7219_PORT, Max7219_CLK_PIN)
#define Max7219_CLK_1()     GPIO_SetBits(Max7219_PORT, Max7219_CLK_PIN)

#define Max7219_CS_0()      GPIO_ResetBits(Max7219_PORT, Max7219_CS_PIN)
#define Max7219_CS_1()      GPIO_SetBits(Max7219_PORT, Max7219_CS_PIN)

#define Max7219_DIN_0()     GPIO_ResetBits(Max7219_PORT, Max7219_DIN_PIN)
#define Max7219_DIN_1()     GPIO_SetBits(Max7219_PORT, Max7219_DIN_PIN)

// ------------------------- 模块数量 -------------------------
#define Max7219_MODULES     4  

// ------------------------- 基础控制 -------------------------
void Max7219_Init(void);
void Max7219_Clear(void);
void Max7219_Refresh(void);

// ------------------------- 核心显示 -------------------------
// 严格按照 8x8 排版显示英文字符串 (支持小数点)
void Max7219_ShowString(const char *str);

// 你要求的单数字/单字符控制函数
void Max7219_ShowSingleNum(uint8_t pos, uint8_t num, uint8_t show_dp);

// 状态机平替 (纯英文)
void Max7219_ShowREADY(void);     // 显示 "RDY "
void Max7219_ShowCONN(void);      // 显示 "CONN"
void Max7219_ShowWAIT(void);      // 显示 "WAIT"

// 计时显示
void Max7219_DispTime(uint32_t Timer_ARR);

// 计次计时器 (左上角显示 1-9 的小字次号，右侧紧凑显示时间)
void Max7219_DispTimeWithCount(uint8_t count, uint32_t Timer_ARR);

// ------------------------- 动画与测试 -------------------------
void Max7219_AnimLoading(uint8_t loops);  // 加载过场动画 (扫描条)
void Max7219_AnimLoading_kunkun(uint8_t loops); // kunkun过场动画
void Max7219_TestAll(void);               // 全屏爆亮测试 (检查坏点)

#endif
