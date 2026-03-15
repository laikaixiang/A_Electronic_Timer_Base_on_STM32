#ifndef __VL53L0_I2C_H
#define __VL53L0_I2C_H

#include "stm32f10x.h" // 使用标准库头文件

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;

// 状态
#define STATUS_OK       0x00
#define STATUS_FAIL     0x01

// IIC操作函数声明
void VL53L0X_i2c_init(void);

u8 VL53L0X_write_byte(u8 address, u8 index, u8 data);
u8 VL53L0X_write_word(u8 address, u8 index, u16 data);
u8 VL53L0X_write_dword(u8 address, u8 index, u32 data);
u8 VL53L0X_write_multi(u8 address, u8 index, u8 *pdata, u16 count);

u8 VL53L0X_read_byte(u8 address, u8 index, u8 *pdata);
u8 VL53L0X_read_word(u8 address, u8 index, u16 *pdata);
u8 VL53L0X_read_dword(u8 address, u8 index, u32 *pdata);
u8 VL53L0X_read_multi(u8 address, u8 index, u8 *pdata, u16 count);

#endif
