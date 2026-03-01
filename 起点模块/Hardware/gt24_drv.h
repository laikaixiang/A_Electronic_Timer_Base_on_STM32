#ifndef __GT24_DRV_H
#define __GT24_DRV_H

#include "stm32f10x.h"

// ================= 硬件引脚定义 (PA7 - PA12) =================
// 参考了你提供的引脚分配
#define GT24_PORT       GPIOA
#define GT24_RCC        RCC_APB2Periph_GPIOA

#define GT24_PIN_IRQ    GPIO_Pin_7   // 输入: 中断 (低电平有效)
#define GT24_PIN_CE     GPIO_Pin_8   // 输出: 模式控制 (高=工作, 低=待机)
#define GT24_PIN_CSN    GPIO_Pin_9   // 输出: 片选 (低=选中)
#define GT24_PIN_SCK    GPIO_Pin_10  // 输出: 时钟
#define GT24_PIN_MOSI   GPIO_Pin_11  // 输出: 主出从入
#define GT24_PIN_MISO   GPIO_Pin_12  // 输入: 主入从出

// ================= 接口函数声明 =================
void GT24_Init(void);              // 初始化
uint8_t GT24_Check(void);          // 自检: 0=成功, 1=失败
void GT24_SetRxMode(void);         // 切换接收模式
void GT24_SetTxMode(void);         // 切换发送模式

// 发送数据 (带有限超时，不阻塞主循环太久)
// 返回: 1=发送成功(收到ACK), 0=失败
uint8_t GT24_TxPacket(uint8_t *txbuf);

// 接收数据 (查询模式，高效)
// 返回: 1=收到数据, 0=无数据
uint8_t GT24_RxPacket(uint8_t *rxbuf);

#endif
