#include "stm32f10x.h"                  // Device header
#include "seg_display.h"

// 定义全局变量记录定时器状态，方便管理
static uint8_t Timer2_State = 1;        // 1-运行 0-暂停
static uint16_t ARR_User = 10000;			// 定义ARR
static uint16_t PSC_User = 7200;				// 定义PSC
//static uint32_t Seconds = 0;  // 记录定时器溢出总次数（扩展计时范围）
//								//ps：还是不要在子文件里面声明新变量了

/**
  * 函    数：定时中断初始化
  * 参    数：无
  * 返 回 值：无
  */
void Timer_Init(void)
{
	/*开启时钟*/
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);			//开启TIM2的时钟
	
	/*配置时钟源*/
	TIM_InternalClockConfig(TIM2);		//选择TIM2为内部时钟，若不调用此函数，TIM默认也为内部时钟
	
	/*时基单元初始化*/
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;				//定义结构体变量
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;		//时钟分频，选择不分频，此参数用于配置滤波器时钟，不影响时基单元功能
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;	//计数器模式，选择向上计数
	TIM_TimeBaseInitStructure.TIM_Period = ARR_User - 1;				//计数周期，即ARR的值
	TIM_TimeBaseInitStructure.TIM_Prescaler = PSC_User - 1;				//预分频器，即PSC的值
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;			//重复计数器，高级定时器才会用到
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);				//将结构体变量交给TIM_TimeBaseInit，配置TIM2的时基单元	

	/*中断输出配置*/
	TIM_ClearFlag(TIM2, TIM_FLAG_Update);						//清除定时器更新标志位
																//TIM_TimeBaseInit函数末尾，手动产生了更新事件
																//若不清除此标志位，则开启中断后，会立刻进入一次中断
																//如果不介意此问题，则不清除此标志位也可
	
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);					//开启TIM2的更新中断
	
	/*NVIC中断分组*/
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);				//配置NVIC为分组2
																//即抢占优先级范围：0~3，响应优先级范围：0~3
																//此分组配置在整个工程中仅需调用一次
																//若有多个中断，可以把此代码放在main函数内，while循环之前
																//若调用多次配置分组的代码，则后执行的配置会覆盖先执行的配置
	
	/*NVIC配置*/
	NVIC_InitTypeDef NVIC_InitStructure;						//定义结构体变量
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;				//选择配置NVIC的TIM2线
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				//指定NVIC线路使能
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;	//指定NVIC线路的抢占优先级为2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;			//指定NVIC线路的响应优先级为1
	NVIC_Init(&NVIC_InitStructure);								//将结构体变量交给NVIC_Init，配置NVIC外设
	
	/*TIM暂时不使能*/
	TIM_Cmd(TIM2, DISABLE);			//使能TIM2，定时器开始运行
	Timer2_State = 0;
	Seg_ShowREADY();
}

/**
  * 函    数：返回定时器CNT的值
  * 参    数：无
  * 返 回 值：定时器CNT的值，范围：0~65535
  */
uint16_t Timer_GetCounter(void)
{
	return TIM_GetCounter(TIM2);	//返回定时器TIM2的CNT
}

/**
  * 函    数：暂停定时器
  * 参    数：无
  * 返 回 值：无
  * 说    明：暂停后CNT值保持不变，中断也会暂停
  */
void Timer_Pause(void)
{
	if(Timer2_State == 1)           // 只有运行状态才执行暂停
	{
		TIM_Cmd(TIM2, DISABLE);     // 关闭定时器计数
		Timer2_State = 0;           // 标记为暂停状态
	}
}

/**
  * 函    数：暂停后重新开始定时器
  * 参    数：无
  * 返 回 值：无
  * 说    明：从暂停时的CNT值继续计数
  */
void Timer_Resume(void)
{
	if(Timer2_State == 0)           // 只有暂停状态才执行恢复
	{
		TIM_Cmd(TIM2, ENABLE);      // 重新开启定时器计数
		Timer2_State = 1;           // 标记为运行状态
	}
}

/**
  * 函    数：定时器归零
  * 参    数：*p_Seconds主函数中秒变量（Seconds）的指针，注意在主函数中调用时要取址&
  * 返 回 值：无
  * 说    明：将CNT值清零，同时清空溢出计数，归零后保持当前运行/暂停状态
  */
void Timer_Reset(uint32_t *p_Seconds)
{
	uint8_t temp_state = Timer2_State;  // 保存当前状态
    Timer_Pause();                      // 暂停计数，避免清零过程中CNT变化
    
    TIM_SetCounter(TIM2, 0);           // 清零CNT寄存器（Timer_GetCounter()返回值）
    TIM_ClearFlag(TIM2, TIM_FLAG_Update); // 清除更新标志位，避免残留中断
    
	Timer_Pause();                      // 先暂停防止清零过程中计数变化
	TIM_SetCounter(TIM2, 0);           // 将CNT寄存器清零
	*p_Seconds = 0;             // 清空溢出总次数
	if(temp_state == 1)
	{
		Timer_Resume();                // 如果原本是运行状态，恢复运行
	}
}

/**
  * 函    数：获取定时器总计时时间(个ARR，1ARR=0.1ms)
  * 参    数：*p_Seconds主函数中秒变量（Seconds）的指针，注意在主函数中调用时要取址&
  * 返 回 值：总计时时间，单位ms (基于72MHz主频，10kHz计数频率)
  * 说    明：结合溢出次数+当前CNT值，扩展计时范围
  */
uint32_t Timer_GetTotalTimeMs(uint32_t *p_Seconds)
{
	// 溢出次数*1000ms + 当前CNT值/10 (10kHz计数频率，1个CNT=0.1ms)
	return (*p_Seconds * ARR_User + (uint32_t)TIM_GetCounter(TIM2));
	// return (*p_Seconds * ARR_User + (uint32_t)TIM_GetCounter(TIM2)) / 10; // 这里返回的是ms
}

/**
  * 函    数：获取定时器当前状态
  * 参    数：无
  * 返 回 值：1-运行 0-暂停
  */
uint8_t Timer_GetState(void)
{
	return Timer2_State;
}

/**
  * 函    数：时间转换函数
  * 参    数：Seconds：秒数， ARR_Counter：ARRCounter未溢出的数，以函数Timer_GetCounter()来获取
  * 返 回 值：ARR_Counter总数
  */
uint32_t Timer_Trans2ARRCounter(uint32_t Seconds, uint32_t ARR_Counter){
	return (Seconds*ARR_User + ARR_Counter);
}

/* 定时器中断函数，可以复制到使用它的地方
void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
	{
		Seconds ++;												//Num变量自增，用于测试定时中断
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	}
}
*/
