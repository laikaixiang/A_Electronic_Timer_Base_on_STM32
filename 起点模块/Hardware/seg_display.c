#include "seg_display.h"
#include "Delay.h"
#include <stdio.h>

// 共阴极数码管段码表（0-9）：dp g f e d c b a（高位到低位）
static const uint8_t seg_code_table[10] = {0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f}; // 共阴极管,转为二进制后，高位是dp，低位是a

// 全局变量：存储四位要显示的数字+小数点状态
static uint8_t disp_buf[4] = {0};    // 四位数字：[0]=个位,[1]=十位,[2]=百位,[3]=千位
static uint8_t dp_flag[4] = {0};     // 小数点标志：[0]=个位小数点,...[3]=千位小数点（1=显示）

// ************************** 计时功能核心变量 **************************
typedef enum {
    TIMER_STOP = 0,   // 停止/重置状态
    TIMER_RUNNING,    // 运行状态
    TIMER_PAUSE       // 暂停状态
} Timer_State;

static Timer_State timer_state = TIMER_STOP; // 计时状态
static uint64_t timer_ms = 0;                // 累计计时毫秒数（0 ~ 9999ms → 对应00.00~99.99秒）

// ************************** 数码管基础函数**************************
void Seg_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 1. 开启AFIO时钟，禁用JTAG释放PB3/PB4
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
    
    // 2. 开启GPIOB时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    
    // 3. 配置段脚为推挽输出
    GPIO_InitStructure.GPIO_Pin = ALL_SEG_PINS;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SEG_PORT, &GPIO_InitStructure);
    
    // 4. 配置位选脚为推挽输出
    GPIO_InitStructure.GPIO_Pin = ALL_BIT_PINS;
    GPIO_Init(BIT_PORT, &GPIO_InitStructure);
    
    // 5. 初始熄灭所有数码管
    Seg_Off();
}

void Seg_ShowSingleNum(uint8_t pos, uint8_t num, uint8_t show_dp)
{
    uint8_t seg_code;
    
    // 1. 先熄灭所有位选（防止重影）
    GPIO_SetBits(BIT_PORT, ALL_BIT_PINS);
    
    // 2. 熄灭所有段脚
    GPIO_ResetBits(SEG_PORT, ALL_SEG_PINS);
    
    // 3. 数字超出范围则直接返回（熄灭）
    if(num > 9) return;
	
    // 4. 位选
	switch(pos){
		case 0:
			GPIO_WriteBit(GPIOB,BIT_PIN_UNIT,Bit_SET); 	// 个位置1：显示
			GPIO_WriteBit(GPIOB,BIT_PIN_TEN,Bit_RESET);	// 十位置0：不显示
			GPIO_WriteBit(GPIOB,BIT_PIN_HUND,Bit_RESET);// 百位置0：不显示		
			GPIO_WriteBit(GPIOB,BIT_PIN_THOU,Bit_RESET);// 千位置0：不显示	
			break;
		case 1:
			GPIO_WriteBit(GPIOB,BIT_PIN_TEN,Bit_SET);
			GPIO_WriteBit(GPIOB,BIT_PIN_UNIT,Bit_RESET);
			GPIO_WriteBit(GPIOB,BIT_PIN_HUND,Bit_RESET);
			GPIO_WriteBit(GPIOB,BIT_PIN_THOU,Bit_RESET);
			break;
		case 2:
			GPIO_WriteBit(GPIOB,BIT_PIN_HUND,Bit_SET);
			GPIO_WriteBit(GPIOB,BIT_PIN_TEN,Bit_RESET);		
			GPIO_WriteBit(GPIOB,BIT_PIN_UNIT,Bit_RESET);			
			GPIO_WriteBit(GPIOB,BIT_PIN_THOU,Bit_RESET);	
			break;
		case 3:
			GPIO_WriteBit(GPIOB,BIT_PIN_THOU,Bit_SET);
			GPIO_WriteBit(GPIOB,BIT_PIN_TEN,Bit_RESET);			
			GPIO_WriteBit(GPIOB,BIT_PIN_HUND,Bit_RESET);			
			GPIO_WriteBit(GPIOB,BIT_PIN_UNIT,Bit_RESET);	
			break;
	}
	
	// 5. 获取段码，按需添加小数点
	//	GPIO_Write(GPIOA,seg_code_table[num]);
	seg_code = seg_code_table[num];
//	seg_code = ~seg_code_table[num];
	if((seg_code & 0x01) == 0) GPIO_SetBits(SEG_PORT, SEG_PIN_A); // a段亮
    if((seg_code & 0x02) == 0) GPIO_SetBits(SEG_PORT, SEG_PIN_B); // b段亮
    if((seg_code & 0x04) == 0) GPIO_SetBits(SEG_PORT, SEG_PIN_C); // c段亮
    if((seg_code & 0x08) == 0) GPIO_SetBits(SEG_PORT, SEG_PIN_D); // d段亮
    if((seg_code & 0x10) == 0) GPIO_SetBits(SEG_PORT, SEG_PIN_E); // e段亮
    if((seg_code & 0x20) == 0) GPIO_SetBits(SEG_PORT, SEG_PIN_F); // f段亮
    if((seg_code & 0x40) == 0) GPIO_SetBits(SEG_PORT, SEG_PIN_G); // g段亮
    if(show_dp == 1) {GPIO_ResetBits(SEG_PORT, SEG_PIN_DP);}
	else{GPIO_SetBits(SEG_PORT, SEG_PIN_DP);}
	// dp段亮
	Delay_ms(2);
}


void Seg_Refresh(void)
{
    static uint8_t scan_pos = 1; // 扫描位置：1-4循环
    
    // 显示当前位置的数字+小数点
    Seg_ShowSingleNum(scan_pos, disp_buf[scan_pos-1], dp_flag[scan_pos-1]);
    
    // 切换到下一个位置
    scan_pos++;
    if(scan_pos > 4) scan_pos = 1;
}

void Seg_Off(void)
{
    // 熄灭所有段脚
    GPIO_ResetBits(SEG_PORT, ALL_SEG_PINS);
    // 熄灭所有位选（高电平）
    GPIO_SetBits(BIT_PORT, ALL_BIT_PINS);
}

// 计时显示
void Seg_ShowString(const char *num_str)
{
    uint8_t i = 0, dot_pos = 0xFF; // 初始化为0xFF（无小数点）
    uint8_t str_len = strlen(num_str);
    uint8_t num_buf[4] = {0};      // 存储四位数字
    uint8_t dp_bit = 0;            // 小数点要显示的位置（1-4）
    
    // 1. 清空显示缓冲区和小数点标志
    memset(disp_buf, 0, sizeof(disp_buf));
    memset(dp_flag, 0, sizeof(dp_flag));
    
    // 2. 查找小数点位置
    for(i = 0; i < str_len; i++)
    {
        if(num_str[i] == '.')
        {
            dot_pos = i;
            break;
        }
    }
    
    // 3. 解析数字（处理如"49.03"、"1234"、"0.123"等情况）
    if(dot_pos == 0xFF) // 无小数点，纯四位数字
    {
        // 从右往左填充（如"1234" → 个位=4,十位=3,百位=2,千位=1）
        int j = str_len - 1;
        for(i = 0; i < 4 && j >= 0; i++, j--)
        {
            if(num_str[j] >= '0' && num_str[j] <= '9')
                num_buf[i] = num_str[j] - '0';
        }
    }
    else // 有小数点（如"49.03" → dot_pos=2）
    {
		for (int j = str_len - 1, i = 0; j >= 0; j--) 
		{
        if (j != dot_pos) {
            num_buf[i++] = num_str[j] - '0';
        }
		}
    }
    
    // 4. 填充到显示缓冲区并显示
	for(i=0; i < 4 ; i++ ){
		if (i != 4 - dot_pos){
			Seg_ShowSingleNum(i, num_buf[i], 0);
		}
		else{
			Seg_ShowSingleNum(i, num_buf[i], 1);
		}
	}
	
    disp_buf[0] = num_buf[0]; // 个位
    disp_buf[1] = num_buf[1]; // 十位
    disp_buf[2] = num_buf[2]; // 百位
    disp_buf[3] = num_buf[3]; // 千位
}

void Seg_ShowREADY(void)
{
	// 共阳极数码管段码表：R(0位)、D(1位)、Y(2位)
    const uint8_t ready_seg_code[] = {0x50, 0x5e, 0x6e};
    const uint8_t pos_list[] = {2, 1, 0}; // 要显示的位置

    // 遍历每个位置显示对应字符
    for(int i=0; i<3; i++)
    {
        // 1. 熄灭所有位选和段脚（防重影）
        GPIO_SetBits(BIT_PORT, ALL_BIT_PINS);
        GPIO_ResetBits(SEG_PORT, ALL_SEG_PINS);

        // 2. 位选控制
        uint8_t pos = pos_list[i];
        switch(pos){
            case 0:
                GPIO_WriteBit(GPIOB,BIT_PIN_UNIT,Bit_SET);
                GPIO_WriteBit(GPIOB,BIT_PIN_TEN,Bit_RESET);
                GPIO_WriteBit(GPIOB,BIT_PIN_HUND,Bit_RESET);
                GPIO_WriteBit(GPIOB,BIT_PIN_THOU,Bit_RESET);
                break;
            case 1:
                GPIO_WriteBit(GPIOB,BIT_PIN_TEN,Bit_SET);
                GPIO_WriteBit(GPIOB,BIT_PIN_UNIT,Bit_RESET);
                GPIO_WriteBit(GPIOB,BIT_PIN_HUND,Bit_RESET);
                GPIO_WriteBit(GPIOB,BIT_PIN_THOU,Bit_RESET);
                break;
            case 2:
                GPIO_WriteBit(GPIOB,BIT_PIN_HUND,Bit_SET);
                GPIO_WriteBit(GPIOB,BIT_PIN_TEN,Bit_RESET);
                GPIO_WriteBit(GPIOB,BIT_PIN_UNIT,Bit_RESET);
                GPIO_WriteBit(GPIOB,BIT_PIN_THOU,Bit_RESET);
                break;
        }

        // 3. 输出对应字符的段码
        uint8_t seg_code = ready_seg_code[i];
        if((seg_code & 0x01) == 0) GPIO_SetBits(SEG_PORT, SEG_PIN_A);
        if((seg_code & 0x02) == 0) GPIO_SetBits(SEG_PORT, SEG_PIN_B);
        if((seg_code & 0x04) == 0) GPIO_SetBits(SEG_PORT, SEG_PIN_C);
        if((seg_code & 0x08) == 0) GPIO_SetBits(SEG_PORT, SEG_PIN_D);
        if((seg_code & 0x10) == 0) GPIO_SetBits(SEG_PORT, SEG_PIN_E);
        if((seg_code & 0x20) == 0) GPIO_SetBits(SEG_PORT, SEG_PIN_F);
        if((seg_code & 0x40) == 0) GPIO_SetBits(SEG_PORT, SEG_PIN_G);
        GPIO_SetBits(SEG_PORT, SEG_PIN_DP); // 小数点始终熄灭

        // 4. 延时保持显示
        Delay_ms(2);
    }
}

// 显示时间，输入的单位为0.1ms
// ms: 0.1ms长度的时间计数，1ARR = 0.1ms
void Seg_DispTime(uint32_t Timer_ARR){
	uint32_t ms = Timer_ARR * 0.1;
	uint32_t Seconds = ms/1000; 
	int mins = Seconds / 60;
	if (Seconds >= 0 && Seconds < 10){
		Seg_ShowSingleNum(0, ms/10%10, 0);
		Seg_ShowSingleNum(1, ms/100%10, 0);
		Seg_ShowSingleNum(2, Seconds%10, 1);
	}
	else if(Seconds >= 10 && Seconds < 60){
		Seg_ShowSingleNum(0, ms/10%10, 0);
		Seg_ShowSingleNum(1, ms/100%10, 0);
		Seg_ShowSingleNum(2, Seconds%10, 1);
		Seg_ShowSingleNum(3, Seconds/10%10, 0);
	}
	else if(Seconds >= 60 && Seconds < 600){
		Seg_ShowSingleNum(0, ms/100%10, 0);
		Seg_ShowSingleNum(1, (Seconds-60*mins)%10, 1);
		Seg_ShowSingleNum(2, (Seconds-60*mins)/10%10, 0);
		Seg_ShowSingleNum(3, mins, 1);
	}
	else if(Seconds >= 600 && Seconds < 3600){
		Seg_ShowSingleNum(0, (Seconds-60*mins)%10, 0);		//秒，个位
		Seg_ShowSingleNum(1, (Seconds-60*mins)/10%10, 0);	//秒，十位
		Seg_ShowSingleNum(2, mins % 10, 1);					//分，个位
		Seg_ShowSingleNum(3, mins / 10 % 10, 0);			//分，十位
	}
	else{
		Seg_ShowREADY();
	}
}
