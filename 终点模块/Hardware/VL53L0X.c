#include "VL53l0X.h"
#include "vl53l0x_i2c.h"

// 定义一个设备实例，ST官方API需要用到它来保存设备状态和I2C地址
VL53L0X_Dev_t MyDevice;
VL53L0X_DEV   Dev = &MyDevice;

/**
 * @brief  VL53L0X 初始化
 * @retval 0: 成功, 其他值: 失败代码
 */
uint8_t VL53L0X_Init(void) {
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    uint32_t refSpadCount;
    uint8_t isApertureSpads;
    uint8_t VhvSettings;
    uint8_t PhaseCal;

    // 1. 初始化底层的 I2C 引脚和外设
    VL53L0X_i2c_init();

    // 2. 配置设备的默认 I2C 地址 (VL53L0X默认8位读写地址为0x52)
    Dev->I2cDevAddr = 0x52; // 具体的地址定义取决于你的 vl53l0x_platform 平台层实现
    Dev->comms_type = 1;    // 选择I2C通信
    Dev->comms_speed_khz = 400;

    // 3. 基础数据初始化
    Status = VL53L0X_DataInit(Dev);
    if(Status != VL53L0X_ERROR_NONE) return 1;

    // 4. 静态初始化
    Status = VL53L0X_StaticInit(Dev);
    if(Status != VL53L0X_ERROR_NONE) return 2;

    // 5. 参考校准 (温度和相位)
    Status = VL53L0X_PerformRefCalibration(Dev, &VhvSettings, &PhaseCal);
    if(Status != VL53L0X_ERROR_NONE) return 3;

    // 6. SPAD 校准
    Status = VL53L0X_PerformRefSpadManagement(Dev, &refSpadCount, &isApertureSpads);
    if(Status != VL53L0X_ERROR_NONE) return 4;

    // 7. 设置为单次测量模式 (Single Ranging)
    Status = VL53L0X_SetDeviceMode(Dev, VL53L0X_DEVICEMODE_SINGLE_RANGING);
    if(Status != VL53L0X_ERROR_NONE) return 5;

    return 0; // 初始化成功返回0
}

/**
 * @brief  读取距离
 * @retval 距离(mm), 返回 65535(0xFFFF) 表示测量失败或数据无效
 */
uint16_t VL53L0X_ReadDistance_mm(void) {
    VL53L0X_RangingMeasurementData_t RangingMeasurementData;
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    uint16_t distance = 65535; // 默认返回错误码

    // 触发单次测量并获取数据
    Status = VL53L0X_PerformSingleRangingMeasurement(Dev, &RangingMeasurementData);

    if (Status == VL53L0X_ERROR_NONE) {
        // RangeStatus 为 0 代表测量有效；如果是其他值说明出现了错误(如超出量程、信号太弱等)
        if (RangingMeasurementData.RangeStatus == 0) {
            distance = RangingMeasurementData.RangeMilliMeter;
        } else {
            // 你也可以根据需求在信号弱时仍然返回 RangingMeasurementData.RangeMilliMeter
            distance = 65535; 
        }
    }
    
    return distance;
}
