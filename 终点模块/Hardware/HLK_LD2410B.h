#ifndef __HLK_LD2410B_H_
#define __HLK_LD2410B_H_	 
#include "stm32f10x.h"
#include "stdbool.h"

/************************ LD2410B 硬件引脚/外设宏定义 ************************/
// 串口通信外设定义
#define LD2410B_USART                   USART2
#define LD2410B_USART_CLK               RCC_APB1Periph_USART2
#define LD2410B_USART_BAUDRATE          256000  // 【注意】LD2410B出厂默认波特率通常为256000
#define LD2410B_USART_IRQn              USART2_IRQn
#define LD2410B_USART_PREEMPT_PRIO      1       // 串口中断抢占优先级
#define LD2410B_USART_SUB_PRIO          1       // 串口中断子优先级

// 串口GPIO引脚定义
#define LD2410B_GPIO_PORT               GPIOA
#define LD2410B_GPIO_CLK                RCC_APB2Periph_GPIOA
#define LD2410B_TX_PIN                  GPIO_Pin_2  // 模块RX <-- 单片机TX
#define LD2410B_RX_PIN                  GPIO_Pin_3  // 模块TX --> 单片机RX

/************************ LD2410B 帧格式宏定义 ************************/
#define LD2410B_FRAME_HEADER            0xFAFBFCFD  // 帧头
#define LD2410B_FRAME_TAIL              0x01020304  // 帧尾
#define LD2410B_FRAME_LEN               32          // 单帧数据长度（不含帧头帧尾）
#define BUFF_MAX_LEN_LD2410B            300         // 短接收缓冲区长度
#define RECEIVE_MAX_LEN_LD2410B         1024        // 总接收缓冲区长度

/************************ LD2410B 数据结构体（严格保留原有字段） ************************/
/* 短接收数据缓存结构体 */
typedef struct
{
    uint8_t rx_buff[BUFF_MAX_LEN_LD2410B]; 
    bool rx_ok;                            
    uint16_t rx_len;                       
} _RX_Data_LD2410B;

/* 总接收数据缓存结构体 */
typedef struct
{
    uint8_t RECEIVE_BUF[RECEIVE_MAX_LEN_LD2410B]; 
    uint16_t Receive_len;                         
} _Receive_Data_LD2410B;

/* 雷达检测数据结构体 */
typedef struct
{
    uint8_t STATE_target;                // 目标存在状态：0-无 1-有
    uint8_t MOTION_target_distance;      // 动态目标距离(cm)
    uint8_t MOTION_target_energy;        // 动态目标能量(0-100)
    uint8_t STATIC_target_distance;      // 静态目标距离(cm)
    uint8_t STATIC_target_energy;        // 静态目标能量(0-100)
    uint8_t Detection_target_distance;   // 综合检测距离(cm)
    bool rx_ok;                          // 解析完成标志 1:完成 0:未完成
    uint16_t len;                        // 帧数据长度
} _Detection_Target_LD2410B;

/************************ 全局变量声明（严格保留） ************************/
extern _RX_Data_LD2410B          RX_Data_LD2410B;
extern _Receive_Data_LD2410B     Receive_Data_LD2410B;
extern _Detection_Target_LD2410B Detection_Target_LD2410B;

/************************ 接口函数声明（严格保留） ************************/
void LD2410B_Init(void);                // 初始化
void LD2410B_ParseData(void);           // 解析
void DataGet_LD2410B(void);             // 数据获取入口
_Detection_Target_LD2410B LD2410B_GetLatestData(void); // 获取结构体

#endif /* __HLK_LD2410B_H_ */
