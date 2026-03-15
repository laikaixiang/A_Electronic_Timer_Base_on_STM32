#include "vl53l0x_platform.h"
#include "Delay.h"

/**
 * @brief  写入多个字节
 */
VL53L0X_Error VL53L0X_WriteMulti(VL53L0X_DEV Dev, uint8_t index, uint8_t *pdata, uint32_t count) {
    uint8_t status = VL53L0X_write_multi(Dev->I2cDevAddr, index, pdata, (uint16_t)count);
    return (status == STATUS_OK) ? VL53L0X_ERROR_NONE : VL53L0X_ERROR_CONTROL_INTERFACE;
}

/**
 * @brief  读取多个字节
 */
VL53L0X_Error VL53L0X_ReadMulti(VL53L0X_DEV Dev, uint8_t index, uint8_t *pdata, uint32_t count) {
    uint8_t status = VL53L0X_read_multi(Dev->I2cDevAddr, index, pdata, (uint16_t)count);
    return (status == STATUS_OK) ? VL53L0X_ERROR_NONE : VL53L0X_ERROR_CONTROL_INTERFACE;
}

/**
 * @brief  写入单个字节
 */
VL53L0X_Error VL53L0X_WrByte(VL53L0X_DEV Dev, uint8_t index, uint8_t data) {
    uint8_t status = VL53L0X_write_byte(Dev->I2cDevAddr, index, data);
    return (status == STATUS_OK) ? VL53L0X_ERROR_NONE : VL53L0X_ERROR_CONTROL_INTERFACE;
}

/**
 * @brief  写入双字节 (字)
 */
VL53L0X_Error VL53L0X_WrWord(VL53L0X_DEV Dev, uint8_t index, uint16_t data) {
    uint8_t status = VL53L0X_write_word(Dev->I2cDevAddr, index, data);
    return (status == STATUS_OK) ? VL53L0X_ERROR_NONE : VL53L0X_ERROR_CONTROL_INTERFACE;
}

/**
 * @brief  写入四字节 (双字)
 */
VL53L0X_Error VL53L0X_WrDWord(VL53L0X_DEV Dev, uint8_t index, uint32_t data) {
    uint8_t status = VL53L0X_write_dword(Dev->I2cDevAddr, index, data);
    return (status == STATUS_OK) ? VL53L0X_ERROR_NONE : VL53L0X_ERROR_CONTROL_INTERFACE;
}

/**
 * @brief  读取单个字节
 */
VL53L0X_Error VL53L0X_RdByte(VL53L0X_DEV Dev, uint8_t index, uint8_t *data) {
    uint8_t status = VL53L0X_read_byte(Dev->I2cDevAddr, index, data);
    return (status == STATUS_OK) ? VL53L0X_ERROR_NONE : VL53L0X_ERROR_CONTROL_INTERFACE;
}

/**
 * @brief  读取双字节 (字)
 */
VL53L0X_Error VL53L0X_RdWord(VL53L0X_DEV Dev, uint8_t index, uint16_t *data) {
    uint8_t status = VL53L0X_read_word(Dev->I2cDevAddr, index, data);
    return (status == STATUS_OK) ? VL53L0X_ERROR_NONE : VL53L0X_ERROR_CONTROL_INTERFACE;
}

/**
 * @brief  读取四字节 (双字)
 */
VL53L0X_Error VL53L0X_RdDWord(VL53L0X_DEV Dev, uint8_t index, uint32_t *data) {
    uint8_t status = VL53L0X_read_dword(Dev->I2cDevAddr, index, data);
    return (status == STATUS_OK) ? VL53L0X_ERROR_NONE : VL53L0X_ERROR_CONTROL_INTERFACE;
}

/**
 * @brief  更新指定寄存器的位 (先读出，进行与或操作后，再写入)
 */
VL53L0X_Error VL53L0X_UpdateByte(VL53L0X_DEV Dev, uint8_t index, uint8_t AndData, uint8_t OrData) {
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    uint8_t data;

    // 先读取
    Status = VL53L0X_RdByte(Dev, index, &data);
    if (Status != VL53L0X_ERROR_NONE) {
        return Status;
    }

    // 修改数据
    data = (data & AndData) | OrData;

    // 再写入
    Status = VL53L0X_WrByte(Dev, index, data);
    return Status;
}

/**
 * @brief  传感器状态轮询延时
 * @note   通常等待几毫秒即可
 */
VL53L0X_Error VL53L0X_PollingDelay(VL53L0X_DEV Dev) {
    // 调用系统毫秒延时函数，具体名称根据你的工程修改
    Delay_ms(2); 
    return VL53L0X_ERROR_NONE;
}
