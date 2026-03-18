//#include "stm32f10x.h"
//#include "Delay.h"
//#include "Buzzer.h"
//#include "LightSensor.h"
//#include "LED.h"
//#include "seg_display.h"
//#include "Key.h"
//#include "Timer.h"
//#include "main_logic.h"

//// 声明外部函数，确保编译器不报错
//extern void MainLogic_CheckWireless(void);

//int main(void)
//{
//    /* 1. 硬件驱动层初始化 (BSP Layer) */
//    SystemInit();       
//    Buzzer_Init();
//    LightSensor_Init();
//    LED_Init();
//    Key_Init();
//    Seg_Init();
//    Timer_Init();
//    
//    // 如果你有 Delay_Init 请在这里调用
//    // Delay_Init(); 

//    /* 2. 业务逻辑层初始化 (App Layer) */
//    MainLogic_Init();

//    /* 3. 主循环 */
//    while(1)
//    {
//        // 1. 获取输入 (键盘)
//        uint8_t key_event = Key_GetEvent();

//        // 2. 处理逻辑 (状态机 + 事件处理)
//        Handle_StateMachine(key_event);
//        
//        // 3. 【新增】检查无线数据 (必须在此处轮询)
//        // 包含了接收远程 START 信号和 PING 握手信号
//        MainLogic_CheckWireless();

//        // 4. 处理输出 (显示/LED/蜂鸣器)
//        // 这里必须高频调用，不能被阻塞
//        Handle_Display();
//        
//        // 5. 如果需要控制循环频率，可以使用极短的延时
//        // 但为了数码管刷新效果，建议不要加长延时
//		// Delay_us(500); 
//    }
//}

#include "stm32f10x.h"
#include "VL53L0x.h"

// 外部数码管与延时函数声明
extern void Seg_ShowSingleNum(uint8_t pos, uint8_t num, uint8_t show_dp);
extern void Delay_ms(uint16_t ms);
extern void Seg_Init(void); 

/**
 * @brief 将距离值拆分并显示在4位数码管上
 * @param dist_mm 距离(毫米)
 */
void Display_Distance(uint16_t dist_mm) {
    // 如果大于 9999 毫米，超出了4位数码管的显示范围，显示 9999 封顶
    if(dist_mm > 9999) {
        dist_mm = 9999;
    }
    
    // 千位
    Seg_ShowSingleNum(3, (dist_mm / 1000) % 10, 0); 
    // 百位
    Seg_ShowSingleNum(2, (dist_mm / 100) % 10, 0);  
    // 十位
    Seg_ShowSingleNum(1, (dist_mm / 10) % 10, 0);   
    // 个位
    Seg_ShowSingleNum(0, dist_mm % 10, 0);          
}

int main(void) {
    uint16_t current_distance = 0;  // 保存当前测量的距离
    uint8_t refresh_counter = 0;    // 刷新计数器
    
    // 1. 系统及外设初始化
    // SystemInit(); // 根据STM32标准库启动文件，通常已在main之前调用
    Seg_Init();      // 初始化数码管的引脚
    
    // 2. 初始化 VL53L0X 传感器
    // 如果初始化失败，在数码管上显示 8888 提示错误
    uint8_t init_status = VL53L0X_Init(); // 只执行一次，把结果存起来
    
    if(init_status != 0) {
        while(1) {
            // 根据存下来的结果判断并刷新显示
            if(init_status == 1) Display_Distance(1111); // DataInit 失败(I2C不通)
            else if(init_status == 2) Display_Distance(2222); // StaticInit 失败
            else if(init_status == 3) Display_Distance(3333); // 参比校准失败
            else if(init_status == 4) Display_Distance(4444); // SPAD校准失败
            else if(init_status == 5) Display_Distance(5555); // 模式设置失败
            else Display_Distance(8888); // 其他未知错误
        }
    }
    
    // 3. 初始读取一次数据
    current_distance = VL53L0X_ReadDistance_mm();
    
    // 4. 主循环
    while(1) {
        // 步骤A：不断刷新数码管，维持视觉暂留
        Display_Distance(current_distance);
        
        // 步骤B：计数器累加，不要每次循环都去测距，避免数码管闪烁卡顿
        refresh_counter++;
        
        // 当数码管完整刷新了 10 次后，再测一次距 (假设每次刷新约8ms，则测量周期约为80ms)
        if(refresh_counter >= 10) {
            uint16_t new_dist = VL53L0X_ReadDistance_mm();
            
            // 过滤掉错误数据 (65535代表读取超时或异常)
            if(new_dist != 65535) {
                current_distance = new_dist;
            }
            
            // 计数器清零，重新开始下一轮数码管刷新周期
            refresh_counter = 0;
        }
    }
//	while(1){
//		if(VL53L0X_Check_Object_Presence(1250)==0){Display_Distance(1234);}
//		else{Display_Distance(0000);}
//	}
}
