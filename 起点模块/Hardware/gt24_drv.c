#include "gt24_drv.h"

// ================= 寄存器地址 =================
#define NRF_WRITE_REG   0x20
#define RD_RX_PLOAD     0x61
#define WR_TX_PLOAD     0xA0
#define FLUSH_TX        0xE1
#define FLUSH_RX        0xE2
#define STATUS          0x07
#define TX_ADDR         0x10
#define RX_ADDR_P0      0x0A

// 统一通信地址 (收发必须一致)
const uint8_t ADDRESS[5] = {0x34, 0x43, 0x10, 0x10, 0x01};

// ================= GPIO 操作宏 =================
#define CE_H()   GPIO_SetBits(GT24_PORT, GT24_PIN_CE)
#define CE_L()   GPIO_ResetBits(GT24_PORT, GT24_PIN_CE)
#define CSN_H()  GPIO_SetBits(GT24_PORT, GT24_PIN_CSN)
#define CSN_L()  GPIO_ResetBits(GT24_PORT, GT24_PIN_CSN)
#define SCK_H()  GPIO_SetBits(GT24_PORT, GT24_PIN_SCK)
#define SCK_L()  GPIO_ResetBits(GT24_PORT, GT24_PIN_SCK)
#define MOSI_H() GPIO_SetBits(GT24_PORT, GT24_PIN_MOSI)
#define MOSI_L() GPIO_ResetBits(GT24_PORT, GT24_PIN_MOSI)
#define MISO_READ() GPIO_ReadInputDataBit(GT24_PORT, GT24_PIN_MISO)
#define IRQ_READ()  GPIO_ReadInputDataBit(GT24_PORT, GT24_PIN_IRQ)

// 软件SPI字节读写 (Mode 0)
static uint8_t SPI_RW(uint8_t byte) {
    uint8_t i;
    for(i=0; i<8; i++) {
        if(byte & 0x80) MOSI_H(); else MOSI_L();
        byte <<= 1;
        SCK_H(); 
        if(MISO_READ()) byte |= 0x01;
        SCK_L();
    }
    return byte;
}

// 写寄存器
static uint8_t GT24_Write_Reg(uint8_t reg, uint8_t value) {
    uint8_t status;
    CSN_L();
    status = SPI_RW(reg);
    SPI_RW(value);
    CSN_H();
    return status;
}

// 读寄存器
static uint8_t GT24_Read_Reg(uint8_t reg) {
    uint8_t value;
    CSN_L();
    SPI_RW(reg);
    value = SPI_RW(0xFF);
    CSN_H();
    return value;
}

// 缓冲区读写
static void GT24_Write_Buf(uint8_t reg, uint8_t *pBuf, uint8_t len) {
    CSN_L();
    SPI_RW(reg);
    while(len--) SPI_RW(*pBuf++);
    CSN_H();
}
static void GT24_Read_Buf(uint8_t reg, uint8_t *pBuf, uint8_t len) {
    CSN_L();
    SPI_RW(reg);
    while(len--) *pBuf++ = SPI_RW(0xFF);
    CSN_H();
}

// ================= 核心功能实现 =================

void GT24_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(GT24_RCC, ENABLE);

    // 输出: CE, CSN, SCK, MOSI
    GPIO_InitStructure.GPIO_Pin = GT24_PIN_CE | GT24_PIN_CSN | GT24_PIN_SCK | GT24_PIN_MOSI;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GT24_PORT, &GPIO_InitStructure);

    // 输入: MISO, IRQ
    GPIO_InitStructure.GPIO_Pin = GT24_PIN_MISO | GT24_PIN_IRQ;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // 上拉输入
    GPIO_Init(GT24_PORT, &GPIO_InitStructure);

    CE_L(); CSN_H(); SCK_L();

    // 参考 RF24L01.c 的初始化逻辑，适配 GT-24
    GT24_Write_Reg(NRF_WRITE_REG + 0x04, 0x1a); // 自动重发: 500us, 10次
    GT24_Write_Reg(NRF_WRITE_REG + 0x01, 0x01); // 开启自动应答 (EN_AA)
    GT24_Write_Reg(NRF_WRITE_REG + 0x02, 0x01); // 允许通道0
    GT24_Write_Reg(NRF_WRITE_REG + 0x03, 0x03); // 地址宽度5字节
    GT24_Write_Reg(NRF_WRITE_REG + 0x05, 40);   // 射频通道 2440MHz
    // 0x0F = 2Mbps (低延迟), 0dBm (最大功率)
    GT24_Write_Reg(NRF_WRITE_REG + 0x06, 0x0F); 

    // 写地址
    GT24_Write_Buf(NRF_WRITE_REG + TX_ADDR, (uint8_t*)ADDRESS, 5);
    GT24_Write_Buf(NRF_WRITE_REG + RX_ADDR_P0, (uint8_t*)ADDRESS, 5);
}

// 硬件自检
uint8_t GT24_Check(void) {
    uint8_t buf[5] = {0xA5,0xA5,0xA5,0xA5,0xA5};
    uint8_t i;
    GT24_Write_Buf(NRF_WRITE_REG+TX_ADDR, buf, 5);
    GT24_Read_Buf(TX_ADDR, buf, 5);
    for(i=0; i<5; i++) if(buf[i]!=0xA5) return 1; 
    return 0; 
}

void GT24_SetRxMode(void) {
    CE_L();
    GT24_Write_Reg(NRF_WRITE_REG + 0x00, 0x0F); // RX, PWR_UP
    GT24_Write_Reg(NRF_WRITE_REG + 0x11, 32);   // 固定Payload长度32
    CE_H();
}

void GT24_SetTxMode(void) {
    CE_L();
    GT24_Write_Reg(NRF_WRITE_REG + 0x00, 0x0E); // TX, PWR_UP
    CE_H();
}

uint8_t GT24_TxPacket(uint8_t *txbuf) {
    uint8_t status;
    CE_L();
    GT24_Write_Buf(WR_TX_PLOAD, txbuf, 32);
    GT24_Write_Reg(NRF_WRITE_REG + 0x00, 0x0E);
    CE_H(); // 启动发送
    
    // 稍微延时，给模块反应时间
    { volatile int x=0; for(;x<100;x++); } 
    CE_L();

    // 等待IRQ拉低 (发送完成或达到最大重发)
    // 增加超时保护，防止死循环卡死主程序
    uint32_t timeout = 0xFFFFF; 
    while(IRQ_READ() != 0 && timeout--);

    // 读取并清除中断
    status = GT24_Read_Reg(STATUS);
    GT24_Write_Reg(NRF_WRITE_REG + STATUS, status); 

    if(status & 0x20) return 1; // TX_DS: 成功收到ACK
    if(status & 0x10) {
        GT24_Write_Reg(FLUSH_TX, 0xFF); // MAX_RT: 失败，清除FIFO
        return 0; 
    }
    return 0;
}

uint8_t GT24_RxPacket(uint8_t *rxbuf) {
    // 优先检查IRQ引脚，速度快
    if (IRQ_READ() == 0) { 
        uint8_t status = GT24_Read_Reg(STATUS);
        if(status & 0x40) { // RX_DR: 收到数据
            GT24_Read_Buf(RD_RX_PLOAD, rxbuf, 32);
            GT24_Write_Reg(NRF_WRITE_REG + STATUS, status); // 清除中断
            return 1;
        }
    }
    return 0;
}
