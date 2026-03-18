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
    Dev->I2cDevAddr = 0x52; 
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
    
    // A. 极限降低信号强度要求 (接收远距离微弱反光，0.1 MCPS)
    VL53L0X_SetLimitCheckValue(Dev, VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE, 
                               (FixPoint1616_t)(0.1 * 65536));

    // B. 极限放宽噪声容忍度 (允许较大误差，保证远距离能触发)
    VL53L0X_SetLimitCheckValue(Dev, VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE, 
                               (FixPoint1616_t)(100 * 65536));

    // C. 修改激光发射脉冲周期
    VL53L0X_SetVcselPulsePeriod(Dev, VL53L0X_VCSEL_PERIOD_PRE_RANGE, 18);
    VL53L0X_SetVcselPulsePeriod(Dev, VL53L0X_VCSEL_PERIOD_FINAL_RANGE, 14);

    // D. 压缩时间预算，拉满扫描速度 (30ms = 约33Hz刷新率)
    // 第二个参数MeasurementTimingBudgetMicroSeconds>20000,20ms/次
    VL53L0X_SetMeasurementTimingBudgetMicroSeconds(Dev, 30000);

    // ==========================================================

    // 7. 设置为连续测量模式 (Continuous Ranging) 以实现无阻塞极速轮询
    Status = VL53L0X_SetDeviceMode(Dev, VL53L0X_DEVICEMODE_CONTINUOUS_RANGING);
    if(Status != VL53L0X_ERROR_NONE) return 5;
    
    // 8. 立刻启动测量，让传感器在后台自己跑起来
    Status = VL53L0X_StartMeasurement(Dev);
    if(Status != VL53L0X_ERROR_NONE) return 6;

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

/**
 * @brief  快速检测是否有物体进入指定阈值范围 (非阻塞式)
 * @param  threshold_mm 报警距离阈值 (例如 1250 代表 1.25米)
 * @retval 1: 阈值内有物体 ; 0: 安全
 */
uint8_t VL53L0X_Check_Object_Presence(uint16_t threshold_mm) 
{
    static uint16_t last_valid_distance = 8190; // 静态变量，保存上一次的有效距离
    uint8_t dataReady = 0;
    VL53L0X_RangingMeasurementData_t RangingData;

    // 1. 询问传感器：这 30ms 的光子收集完了吗？(耗时不到 10us)
    VL53L0X_GetMeasurementDataReady(Dev, &dataReady);

    // 2. 只有数据准备好了，才去读 I2C (避免阻塞等待)
    if(dataReady == 1) 
    {
        VL53L0X_GetRangingMeasurementData(Dev, &RangingData);
        uint16_t current_dist = RangingData.RangeMilliMeter;
        
        // 3. 过滤官方的错误码 (8190, 8191, 65535 代表测不到东西 / 超出量程)
        // RangeStatus != 0 通常代表测距有轻微异常，但为了快速触发，我们放宽要求，只要不是完全失败(Status 4)就采纳
        if(current_dist != 8190 && current_dist != 8191 && current_dist != 65535 && RangingData.RangeStatus != 4) 
        {
            last_valid_distance = current_dist; // 更新为最新有效距离
        } 
        else 
        {
            last_valid_distance = 8190; // 没测到东西，视为远于量程
        }
        
        // 4. 清除中断，让传感器立刻无缝开始下一轮 30ms 的曝光
        VL53L0X_ClearInterruptMask(Dev, VL53L0X_REG_SYSTEM_INTERRUPT_GPIO_NEW_SAMPLE_READY);
    }

    // 5. 核心逻辑判断：是否在阈值范围内？(忽略 10mm 以内的自身外壳干扰)
    if(last_valid_distance > 10 && last_valid_distance <= threshold_mm) 
    {
        return 1; // 有
    } 
    else 
    {
        return 0; // 无
    }
}
