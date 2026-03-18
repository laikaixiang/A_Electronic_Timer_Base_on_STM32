#include "usart1.h"
#include <string.h>

/************************ 静态全局变量（仅本文件可见） ************************/
static uint8_t USART1_RxBuf[USART1_RX_BUF_LEN] = {0};  // 接收缓冲区
static uint16_t USART1_RxLen = 0;                      // 接收数据长度

/**
 * @brief  USART1初始化（标准库实现）
 * @param  baudrate: 波特率（如9600/115200/19200）
 * @retval 无
 */
void USART1_Init(uint32_t baudrate)
{
    GPIO_InitTypeDef  GPIO_InitStruct;
    USART_InitTypeDef USART_InitStruct;
    NVIC_InitTypeDef  NVIC_InitStruct;

    // 1. 使能时钟：USART1 + GPIOA
    RCC_APB2PeriphClockCmd(USART1_USART_CLK | USART1_GPIO_CLK, ENABLE);

    // 2. 配置GPIO引脚：TX(PA9)推挽复用，RX(PA10)浮空输入
    GPIO_InitStruct.GPIO_Pin = USART1_TX_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(USART1_GPIO_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.GPIO_Pin = USART1_RX_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(USART1_GPIO_PORT, &GPIO_InitStruct);

    // 3. 配置USART1参数
    USART_InitStruct.USART_BaudRate = baudrate;
    USART_InitStruct.USART_WordLength = USART_WordLength_8b;
    USART_InitStruct.USART_StopBits = USART_StopBits_1;
    USART_InitStruct.USART_Parity = USART_Parity_No;
    USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStruct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &USART_InitStruct);

    // 4. 配置串口中断（可选，如需中断接收）
    NVIC_InitStruct.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = USART1_PREEMPT_PRIO;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = USART1_SUB_PRIO;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    // 5. 使能USART1及接收中断
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    USART_Cmd(USART1, ENABLE);

    // 6. 清空接收缓冲区
    USART1_ClearRxBuf();
}

/**
 * @brief  USART1发送单个字节
 * @param  data: 要发送的字节
 * @retval 无
 */
void USART1_SendByte(uint8_t data)
{
    USART_SendData(USART1, data);
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET); // 等待发送完成
}

/**
 * @brief  USART1发送字符串
 * @param  str: 字符串指针（以'\0'结尾）
 * @retval 无
 */
void USART1_SendString(uint8_t *str)
{
    while (*str != '\0')
    {
        USART1_SendByte(*str);
        str++;
    }
}

/**
 * @brief  获取USART1接收缓冲区数据
 * @param  buf: 输出缓冲区（用户提供）
 * @param  len: 输出数据长度
 * @retval 无
 */
void USART1_GetRxData(uint8_t *buf, uint16_t *len)
{
    if (buf == NULL || len == NULL) return;
    
    memcpy(buf, USART1_RxBuf, USART1_RxLen);
    *len = USART1_RxLen;
}

/**
 * @brief  清空USART1接收缓冲区
 * @param  无
 * @retval 无
 */
void USART1_ClearRxBuf(void)
{
    memset(USART1_RxBuf, 0, USART1_RX_BUF_LEN);
    USART1_RxLen = 0;
}

/**
 * @brief  USART1中断服务函数
 * @note   仅处理接收中断，存入缓冲区
 * @retval 无
 */
void USART1_IRQHandler(void)
{
    uint8_t rx_data = 0;
    
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        rx_data = USART_ReceiveData(USART1); // 读取接收数据
        
        // 防止缓冲区溢出
        if (USART1_RxLen < USART1_RX_BUF_LEN - 1)
        {
            USART1_RxBuf[USART1_RxLen++] = rx_data;
        }
        
        USART_ClearITPendingBit(USART1, USART_IT_RXNE); // 清除中断标志
    }
}

/**
 * @brief  重定向printf到USART1（标准库需要）
 * @note   包含stdio.h后，printf自动使用USART1输出
 * @param  ch: 输出字符
 * @param  f: 文件指针（无实际意义）
 * @retval 输出字符
 */
#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif
PUTCHAR_PROTOTYPE
{
    USART1_SendByte((uint8_t)ch);
    return ch;
}
