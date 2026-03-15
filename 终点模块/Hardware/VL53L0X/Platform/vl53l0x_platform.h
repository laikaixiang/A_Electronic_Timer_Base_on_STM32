#ifndef _VL53L0X_PLATFORM_H_
#define _VL53L0X_PLATFORM_H_

#include "vl53l0x_def.h"
#include "vl53l0x_i2c.h" 

// ==============================================================
// 1. 填补 ST API 需要的日志宏与字符串宏 (解决 String.c 报错)
// ==============================================================
#include <string.h>  
#define VL53L0X_COPYSTRING(str, ...) strcpy(str, ##__VA_ARGS__)

#define TRACE_MODULE_API 0
#define _LOG_FUNCTION_START(module, fmt, ...) (void)0
#define _LOG_FUNCTION_END(module, status, ...) (void)0
#define _LOG_FUNCTION_END_FMT(module, status, fmt, ...) (void)0

// ==============================================================
// 2. ST 官方 API 依赖的设备结构体定义
// ==============================================================
typedef struct {
    VL53L0X_DevData_t Data;      // ST API 保存参数和状态的内部数据区！
    uint8_t   I2cDevAddr;        // I2C 设备地址 (默认0x52)
    uint8_t   comms_type;        // 通信类型
    uint16_t  comms_speed_khz;   // 通信速度
} VL53L0X_Dev_t;

typedef VL53L0X_Dev_t* VL53L0X_DEV;

// ==============================================================
// 3. 读写设备内部 Data 结构体的宏映射
// ==============================================================
#define PALDevDataGet(Dev, field) (Dev->Data.field)
#define PALDevDataSet(Dev, field, data) (Dev->Data.field)=(data)

#define VL53L0X_MAX_I2C_XFER_SIZE  64 // 定义I2C写的最大字节数

// ==============================================================
// 4. ST 官方 API 必须实现的底层平台接口声明
// ==============================================================
VL53L0X_Error VL53L0X_WriteMulti(VL53L0X_DEV Dev, uint8_t index, uint8_t *pdata, uint32_t count);
VL53L0X_Error VL53L0X_ReadMulti(VL53L0X_DEV Dev, uint8_t index, uint8_t *pdata, uint32_t count);
VL53L0X_Error VL53L0X_WrByte(VL53L0X_DEV Dev, uint8_t index, uint8_t data);
VL53L0X_Error VL53L0X_WrWord(VL53L0X_DEV Dev, uint8_t index, uint16_t data);
VL53L0X_Error VL53L0X_WrDWord(VL53L0X_DEV Dev, uint8_t index, uint32_t data);
VL53L0X_Error VL53L0X_RdByte(VL53L0X_DEV Dev, uint8_t index, uint8_t *data);
VL53L0X_Error VL53L0X_RdWord(VL53L0X_DEV Dev, uint8_t index, uint16_t *data);
VL53L0X_Error VL53L0X_RdDWord(VL53L0X_DEV Dev, uint8_t index, uint32_t *data);
VL53L0X_Error VL53L0X_UpdateByte(VL53L0X_DEV Dev, uint8_t index, uint8_t AndData, uint8_t OrData);

VL53L0X_Error VL53L0X_PollingDelay(VL53L0X_DEV Dev); 

#endif /* _VL53L0X_PLATFORM_H_ */
