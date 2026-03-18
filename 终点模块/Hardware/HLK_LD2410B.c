#include "HLK_LD2410B.h"
#include "string.h"

/************************ 全局变量定义 ************************/
_RX_Data_LD2410B          RX_Data_LD2410B = {0};
_Receive_Data_LD2410B     Receive_Data_LD2410B = {0};
_Detection_Target_LD2410B Detection_Target_LD2410B = {0};

/************************ 私有函数声明 ************************/
static void LD2410B_USART_Config(void);  

/************************ 函数实现 ************************/

void LD2410B_Init(void)
{
    memset(&RX_Data_LD2410B, 0, sizeof(RX_Data_LD2410B));
    memset(&Receive_Data_LD2410B, 0, sizeof(Receive_Data_LD2410B));
    memset(&Detection_Target_LD2410B, 0, sizeof(Detection_Target_LD2410B));
    
    LD2410B_USART_Config();
}

static void LD2410B_USART_Config(void)
{
    GPIO_InitTypeDef  GPIO_InitStruct;
    USART_InitTypeDef USART_InitStruct;
    NVIC_InitTypeDef  NVIC_InitStruct;

	// 1. 使能时钟（引用头文件宏）
    RCC_APB2PeriphClockCmd(LD2410B_GPIO_CLK, ENABLE);
    RCC_APB1PeriphClockCmd(LD2410B_USART_CLK, ENABLE);

	// 2. 配置GPIO引脚（TX:推挽复用，RX:浮空输入）
    GPIO_InitStruct.GPIO_Pin = LD2410B_TX_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(LD2410B_GPIO_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.GPIO_Pin = LD2410B_RX_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(LD2410B_GPIO_PORT, &GPIO_InitStruct);

	// 3. 配置USART参数（引用头文件宏）
    USART_InitStruct.USART_BaudRate = LD2410B_USART_BAUDRATE;
    USART_InitStruct.USART_WordLength = USART_WordLength_8b;
    USART_InitStruct.USART_StopBits = USART_StopBits_1;
    USART_InitStruct.USART_Parity = USART_Parity_No;
    USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStruct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(LD2410B_USART, &USART_InitStruct);

	// 4. 配置串口中断（引用头文件宏）
    NVIC_InitStruct.NVIC_IRQChannel = LD2410B_USART_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = LD2410B_USART_PREEMPT_PRIO;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = LD2410B_USART_SUB_PRIO;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

	// 5. 使能串口及接收中断
    USART_ITConfig(LD2410B_USART, USART_IT_RXNE, ENABLE);
    USART_Cmd(LD2410B_USART, ENABLE);
}

/**
 * @brief  雷达数据解析（核心函数）
 * @param  无
 * @retval 无
 */
void LD2410B_ParseData(void)
{
    uint16_t i = 0;
    uint8_t *rx_buf = Receive_Data_LD2410B.RECEIVE_BUF;
    uint16_t rx_len = Receive_Data_LD2410B.Receive_len;

    while (i < rx_len - 4 - LD2410B_FRAME_LEN - 4)
    {
        if ((rx_buf[i] == 0xFD) && (rx_buf[i+1] == 0xFC) && 
            (rx_buf[i+2] == 0xFB) && (rx_buf[i+3] == 0xFA))
        {
            uint16_t data_start = i + 4; 

            if ((rx_buf[data_start + LD2410B_FRAME_LEN] == 0x04) && 
                (rx_buf[data_start + LD2410B_FRAME_LEN + 1] == 0x03) && 
                (rx_buf[data_start + LD2410B_FRAME_LEN + 2] == 0x02) && 
                (rx_buf[data_start + LD2410B_FRAME_LEN + 3] == 0x01))
            {
                // 赋值给原有结构体，保证外部调用完全兼容
                Detection_Target_LD2410B.STATE_target = rx_buf[data_start + 0];
                Detection_Target_LD2410B.MOTION_target_distance = rx_buf[data_start + 1];
                Detection_Target_LD2410B.MOTION_target_energy = rx_buf[data_start + 3];
                Detection_Target_LD2410B.STATIC_target_distance = rx_buf[data_start + 5];
                Detection_Target_LD2410B.STATIC_target_energy = rx_buf[data_start + 7];
                Detection_Target_LD2410B.Detection_target_distance = 
                    (rx_buf[data_start + 2] << 8) | rx_buf[data_start + 1];
                Detection_Target_LD2410B.rx_ok = true;

                // 使用 memmove 只清除已处理的这一帧数据。
                uint16_t frame_total_len = i + 4 + LD2410B_FRAME_LEN + 4;
                uint16_t remain_len = rx_len - frame_total_len;
                if (remain_len > 0) {
                    memmove(rx_buf, &rx_buf[frame_total_len], remain_len);
                }
                Receive_Data_LD2410B.Receive_len = remain_len;
                RX_Data_LD2410B.rx_ok = false;
                
                return; // 解析成功一帧即退出，等待下次调用
            }
        }
        i++;
    }

    // 【安全机制】：如果由于误码导致接收区快满了还没找到帧头，清空缓存防死机
    if (Receive_Data_LD2410B.Receive_len >= RECEIVE_MAX_LEN_LD2410B - 50) {
        memset(rx_buf, 0, RECEIVE_MAX_LEN_LD2410B);
        Receive_Data_LD2410B.Receive_len = 0;
    }
}

/**
 * @brief  数据获取封装（兼容用户原有函数名）
 * @param  无
 * @retval 无
 */
void DataGet_LD2410B(void)
{
    if (Receive_Data_LD2410B.Receive_len >= (4 + LD2410B_FRAME_LEN + 4))
    {
        LD2410B_ParseData();
    }
}

/**
 * @brief  获取最新检测数据
 * @param  无
 * @retval 检测数据结构体
 */
_Detection_Target_LD2410B LD2410B_GetLatestData(void)
{
    return Detection_Target_LD2410B;
}

void USART2_IRQHandler(void)
{
    uint8_t rx_data = 0;
    
    if (USART_GetITStatus(LD2410B_USART, USART_IT_RXNE) != RESET)
    {
        rx_data = USART_ReceiveData(LD2410B_USART); 
        
        if (Receive_Data_LD2410B.Receive_len < RECEIVE_MAX_LEN_LD2410B)
        {
            Receive_Data_LD2410B.RECEIVE_BUF[Receive_Data_LD2410B.Receive_len++] = rx_data;
            RX_Data_LD2410B.rx_ok = true;
        }
        
        USART_ClearITPendingBit(LD2410B_USART, USART_IT_RXNE); 
    }
}
