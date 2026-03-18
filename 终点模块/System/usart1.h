#ifndef __USART1_H_
#define __USART1_H_	 
#include "stm32f10x.h"
#include <stdio.h>

/************************ USART1 硬件宏定义 ************************/
#define USART1_BAUDRATE                115200  // 默认波特率
#define USART1_GPIO_PORT               GPIOA
#define USART1_GPIO_CLK                RCC_APB2Periph_GPIOA
#define USART1_TX_PIN                  GPIO_Pin_9
#define USART1_RX_PIN                  GPIO_Pin_10
#define USART1_USART_CLK               RCC_APB2Periph_USART1
#define USART1_IRQn                    USART1_IRQn
#define USART1_PREEMPT_PRIO            2       // 抢占优先级
#define USART1_SUB_PRIO                0       // 子优先级

/************************ USART1 接收缓存配置 ************************/
#define USART1_RX_BUF_LEN              256     // 接收缓冲区长度

/************************ 函数声明 ************************/
void USART1_Init(uint32_t baudrate);          // USART1初始化（可自定义波特率）
void USART1_SendByte(uint8_t data);           // 发送单个字节
void USART1_SendString(uint8_t *str);         // 发送字符串
void USART1_GetRxData(uint8_t *buf, uint16_t *len); // 获取接收缓冲区数据
void USART1_ClearRxBuf(void);                 // 清空接收缓冲区

#endif /* __USART1_H_ */
