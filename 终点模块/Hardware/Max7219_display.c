#include "Max7219_display.h"
#include "Delay.h"
#include <stdio.h>
#include <string.h>

// 显存缓冲区
static uint8_t disp_buf[Max7219_MODULES * 8] = {0};

// ======================= 字库数据 (新增全套A-Z) =======================

static const uint8_t font_5x7[][5] = {
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 9
    {0x00, 0x00, 0x24, 0x00, 0x00}, // 10: ':' 
    {0x00, 0x00, 0x40, 0x00, 0x00}, // 11: '.' 
    {0x00, 0x08, 0x08, 0x08, 0x00}, // 12: '-' 
    {0x00, 0x00, 0x00, 0x00, 0x00}, // 13: ' ' (空格)
    // --- 完整大写字母 (14 ~ 39) ---
    {0x7C, 0x12, 0x11, 0x12, 0x7C}, // A (14)
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // F
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // G
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // M
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
    {0x46, 0x49, 0x49, 0x49, 0x31}, // S
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // T
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, // W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // X
    {0x07, 0x08, 0x70, 0x08, 0x07}, // Y
    {0x61, 0x51, 0x49, 0x45, 0x43}  // Z (39)
};

// ======================= 底层SPI驱动 =======================

static void Max7219_SendByte(uint8_t data)
{
    for(uint8_t i = 0; i < 8; i++)
    {
        Max7219_CLK_0();
        if(data & 0x80) Max7219_DIN_1();
        else            Max7219_DIN_0();
        data <<= 1;
        for(volatile int d=0; d<2; d++); 
        Max7219_CLK_1();
        for(volatile int d=0; d<2; d++);
    }
}

static void Max7219_WriteCmdAll(uint8_t addr, uint8_t data)
{
    Max7219_CS_0();
    for(uint8_t i = 0; i < Max7219_MODULES; i++)
    {
        Max7219_SendByte(addr);
        Max7219_SendByte(data);
    }
    Max7219_CS_1();
}

// ======================= 核心功能 =======================

void Max7219_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    if(Max7219_PORT == GPIOA)      RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    else if(Max7219_PORT == GPIOB) RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    else if(Max7219_PORT == GPIOC) RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
    
    GPIO_InitStructure.GPIO_Pin = Max7219_CLK_PIN | Max7219_CS_PIN | Max7219_DIN_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(Max7219_PORT, &GPIO_InitStructure);
    
    Max7219_CS_1();
    Max7219_CLK_0();
    Max7219_DIN_0();
    Delay_ms(50);
    
    // 发送空指令冲刷垃圾数据，防止错位
    Max7219_CS_0();
    for(int i = 0; i < 8; i++) Max7219_SendByte(0x00);
    Max7219_CS_1(); 
    
    Max7219_WriteCmdAll(0x0F, 0x00); // 关测试
    Max7219_WriteCmdAll(0x09, 0x00); // 取消译码
    Max7219_WriteCmdAll(0x0A, 0x03); // 亮度 (0x00~0x0F)
    Max7219_WriteCmdAll(0x0B, 0x07); // 扫描全开
    Max7219_WriteCmdAll(0x0C, 0x01); // 正常工作
    
    Max7219_Clear();
}

void Max7219_Clear(void)
{
    memset(disp_buf, 0, sizeof(disp_buf));
    Max7219_Refresh();
}

// 刷新显存
void Max7219_Refresh(void)
{
    for(uint8_t row = 0; row < 8; row++)
    {
        Max7219_CS_0();
        // 【重要修复】：将递减改成正向递增，完美解决第1块跑到第4块的反序问题
        for(uint8_t m = 0; m < Max7219_MODULES; m++)
        {
            Max7219_SendByte(row + 1); 
            uint8_t row_data = 0;
            for(uint8_t col = 0; col < 8; col++)
            {
                if(disp_buf[m * 8 + col] & (1 << row)) 
                    row_data |= (0x80 >> col);
            }
            Max7219_SendByte(row_data);
        }
        Max7219_CS_1();
    }
}

// 往指定位置(0~3)写入单个绝对居中的 8x8 字符区块
static void Max7219_ShowChar8x8(uint8_t pos, char c, uint8_t show_dp)
{
    if (pos >= Max7219_MODULES) return;
    
    uint8_t offset = pos * 8;
    uint8_t index = 13; // 默认空格
    
    // ASCII 映射
    if (c >= '0' && c <= '9') index = c - '0';
    else if (c == ':') index = 10;
    else if (c == '.') index = 11;
    else if (c == '-') index = 12;
    else if (c >= 'A' && c <= 'Z') index = c - 'A' + 14;
    else if (c >= 'a' && c <= 'z') index = c - 'a' + 14; // 支持小写自动转大写

    // 【完美8x8排版】：0位空 + 1~5位字符 + 6位小数点 + 7位空
    disp_buf[offset + 0] = 0x00;
    for(uint8_t i = 0; i < 5; i++)
    {
        disp_buf[offset + 1 + i] = font_5x7[index][i];
    }
    disp_buf[offset + 6] = show_dp ? 0x80 : 0x00; // 0x80对应屏幕右下角最底部的点
    disp_buf[offset + 7] = 0x00;
}

// ======================= 应用层功能 =======================

// 你点名要求的函数：指定屏幕位置显示数字并附带小数点
void Max7219_ShowSingleNum(uint8_t pos, uint8_t num, uint8_t show_dp)
{
    // num+'0' 将数字 1 转为 ASCII 字符 '1'
    Max7219_ShowChar8x8(pos, num + '0', show_dp);
    Max7219_Refresh();
}

// 智能字符串显示（会自动将 "12.34" 压缩融合，不会让小数点多占一个屏幕）
void Max7219_ShowString(const char *str)
{
    Max7219_Clear();
    uint8_t pos = 0;
    while(*str && pos < Max7219_MODULES)
    {
        uint8_t show_dp = 0;
        // 如果下一个字符是小数点，把它揉进当前格子里
        if (*(str + 1) == '.') {
            show_dp = 1;
        }
        
        Max7219_ShowChar8x8(pos, *str, show_dp);
        pos++;
        str++;
        if (show_dp) str++; // 跳过那个已经被融合的小数点
    }
    Max7219_Refresh();
}

// 显示文本
void Max7219_ShowREADY(void) // Ready
{
    Max7219_Clear(); // 先清空显存
    const char *str = "READY";
    
    // (32列 - 29列) / 2 = 1，从第1列起步
    uint8_t x_offset = 1; 
    
    while(*str)
    {
        uint8_t index = *str - 'A' + 14; // 在 font_5x7 中的索引
        for(uint8_t i = 0; i < 5; i++)
        {
            disp_buf[x_offset++] = font_5x7[index][i];
        }
        x_offset++; // 字母之间留1列的空格
        str++;
    }
    Max7219_Refresh();
	// 延时确保显示
	Delay_ms(2);
}
void Max7219_ShowWAIT(void)  { Max7219_ShowString("WAIT"); } // Wait
void Max7219_ShowCONN(void)  { Max7219_ShowString("CONN"); } // Connected

// 计时显示功能
void Max7219_DispTime(uint32_t Timer_ARR)
{
    uint32_t ms = Timer_ARR / 10;
    uint32_t Seconds = ms / 1000;
    int mins = Seconds / 60;
    char str_buf[10];

    if (Seconds < 60) {
        // 输出秒与毫秒，如 "12.34" (利用内置的融合小数点机制)
        sprintf(str_buf, "%02d.%02d", Seconds, (ms % 1000) / 10);
        Max7219_ShowString(str_buf); 
    } 
    else if (Seconds < 3600) {
        // 核心修改：生成去掉冒号的纯数字 "1234"
        sprintf(str_buf, "%02d%02d", mins, (Seconds - 60 * mins));
        Max7219_ShowString(str_buf);
        
        // 【魔法操作】：在第2个屏幕和第3个屏幕的交界处（第15列）强行画上冒号
        // 0x24 转二进制是 00100100，正好对应冒号的上下两个点
        disp_buf[15] = 0x24; 
        Max7219_Refresh(); // 重新推送到屏幕
    }
    else {
        Max7219_ShowREADY();
        return; 
    }
}

// 带计次功能的高阶时间显示
// count: 左上角的计次数字 (1~9)
// Timer_ARR: 要显示的时间 (单位：10ms)
void Max7219_DispTimeWithCount(uint8_t count, uint32_t Timer_ARR)
{
    Max7219_Clear();

    // 1. 3x5 微型字库
    const uint8_t tiny_font[10][3] = {
        {0x1F, 0x11, 0x1F}, // 0
        {0x00, 0x1F, 0x00}, // 1 (极简竖线)
        {0x1D, 0x15, 0x17}, // 2
        {0x15, 0x15, 0x1F}, // 3
        {0x07, 0x04, 0x1F}, // 4
        {0x17, 0x15, 0x1D}, // 5
        {0x1F, 0x15, 0x1D}, // 6
        {0x01, 0x01, 0x1F}, // 7
        {0x1F, 0x15, 0x1F}, // 8
        {0x17, 0x15, 0x1F}  // 9
    };

    // 在屏幕最左侧 (0, 1, 2 列) 渲染微型数字
    uint8_t safe_count = count % 10; // 越界保护
    disp_buf[0] = tiny_font[safe_count][0];
    disp_buf[1] = tiny_font[safe_count][1];
    disp_buf[2] = tiny_font[safe_count][2];
    
    // 第 3, 4, 5, 6 列保持为空白 (0x00)，用来明确隔开计次与时间

    // 2. 解析时间
    uint32_t ms = Timer_ARR / 10;
    uint32_t Seconds = ms / 1000;
    int mins = Seconds / 60;
    
    uint8_t d1, d2, d3, d4;
    uint8_t show_colon = 0; // 0=底边小数点, 1=居中冒号

    if (Seconds < 60) {
        // 格式：SS.ms
        d1 = Seconds / 10;
        d2 = Seconds % 10;
        uint32_t frac = (ms % 1000) / 10;
        d3 = frac / 10;
        d4 = frac % 10;
        show_colon = 0; 
    } else if (Seconds < 3600) {
        // 格式：MM:SS
        d1 = mins / 10;
        d2 = mins % 10;
        uint32_t secs = Seconds - 60 * mins;
        d3 = secs / 10;
        d4 = secs % 10;
        show_colon = 1; 
    } else {
        Max7219_ShowREADY();
        return;
    }

    // 3. 紧凑型时间渲染
    uint8_t x = 6; 
    
    // 画第一位数字
    for(int i=0; i<5; i++) disp_buf[x++] = font_5x7[d1][i];
    x++; // 仅留 1 列作为数字间隔
    
    // 画第二位数字
    for(int i=0; i<5; i++) disp_buf[x++] = font_5x7[d2][i];
    x++; // 仅留 1 列作为符号间隔
    
    // 巧妙注入小数点或冒号 (不占额外宽度，仅用 1 列)
    disp_buf[x++] = show_colon ? 0x24 : 0x40; // 0x24=冒号, 0x40=小数点
    x++; // 仅留 1 列
    
    // 画第三位数字
    for(int i=0; i<5; i++) disp_buf[x++] = font_5x7[d3][i];
    x++; // 仅留 1 列
    
    // 画第四位数字
    for(int i=0; i<5; i++) disp_buf[x++] = font_5x7[d4][i];

    // 推送到物理屏幕
    Max7219_Refresh();
}

// ======================= 动画与测试功能 =======================

// 测试：全屏点亮所有像素 (用于检查是否有 LED 坏点)
void Max7219_TestAll(void)
{
    memset(disp_buf, 0xFF, sizeof(disp_buf));
    Max7219_Refresh();
}

// 动画：基于用户手绘 8x8 原稿的丝滑火柴人跑动 (方向完美修复)
void Max7219_AnimLoading(uint8_t loops)
{
    // ================= 经典 2 帧跑动设计 (宽 6 列) =================
    
    // 【向右跑】
    const uint8_t run_right[2][6] = {
        // 帧1：大步跨开 (重心高，手臂向上扬起，双腿前后大跨步)
        {0x00, 0x88, 0x50, 0x3F, 0x53, 0x88}, 
        
        // 帧2：收腿下蹲 (重心整体下沉1像素，手臂向下摆到腰部，双腿收拢)
        {0x00, 0x20, 0x90, 0x7E, 0x96, 0x20}  
    };

    // 【向左跑】 (将右跑的数据完美水平镜像，头部朝左)
    const uint8_t run_left[2][6] = {
        {0x88, 0x53, 0x3F, 0x50, 0x88, 0x00},
        {0x20, 0x96, 0x7E, 0x90, 0x20, 0x00}
    };

	uint8_t FPS_Loading = 12; // 帧率 
	
    for(uint8_t t = 0; t < loops; t++) 
    {
        // 1. 向右跑 (从屏幕最左侧外冲向最右侧外)
        for(int pos = -6; pos <= 32; pos++) 
        {
            memset(disp_buf, 0, sizeof(disp_buf)); 
            
            // 关键优化：abs(pos) % 2 保证每向前位移 1 个像素点，动作就切换一次。
            // 配合帧2的重心下沉，形成完美的 "落地->弹起->落地->弹起" 的奔跑重力感！
            uint8_t frame_idx = abs(pos) % 2;
            const uint8_t *frame = run_right[frame_idx];
            
            // 写入 6 列像素
            for(int i = 0; i < 6; i++) 
            {
                int x = pos + i;
                if(x >= 0 && x < 32) disp_buf[x] = frame[i];
            }
            Max7219_Refresh();
            Delay_ms(1000 / FPS_Loading); // 稍微放缓一点点速度，让肉眼能看清上下弹跳的起伏
        }
        
        // 2. 转身向左跑
        for(int pos = 32; pos >= -6; pos--) 
        {
            memset(disp_buf, 0, sizeof(disp_buf));
            
            uint8_t frame_idx = abs(pos) % 2;
            const uint8_t *frame = run_left[frame_idx];
            
            for(int i = 0; i < 6; i++) 
            {
                int x = pos + i;
                if(x >= 0 && x < 32) disp_buf[x] = frame[i];
            }
            Max7219_Refresh();
            Delay_ms(700 / FPS_Loading);
        }
    }
    
    // 跑完清空屏幕，避免残留
    memset(disp_buf, 0, sizeof(disp_buf));
    Max7219_Refresh();
}

// 动画：kunkun跑动
void Max7219_AnimLoading_kunkun(uint8_t loops)
{
    // ================= 经典 2 帧跑动设计 (宽 6 列) =================
    
    // 【向右跑】
	// 铁山靠
	const uint8_t run_right[9][6] = {
    {0xD8,0x3C,0x5E,0xDA,0x00,0x00},
    {0x80,0x58,0x3C,0x5E,0xDA,0x00},
    {0x80,0x40,0x58,0x3C,0x5E,0xDA},
    {0xD8,0x3C,0x5E,0xDA,0x00,0x00},
    {0xCC,0x3E,0x2F,0xED,0x00,0x00},
    {0xC0,0x2C,0x1E,0x2F,0xED,0x00},
    {0x80,0x40,0x2C,0x3E,0x2F,0xED},
    {0xC0,0x2C,0x1E,0x2F,0xED,0x00},
    {0xCC,0x3E,0x2F,0xED,0x00,0x00}
};
//	const uint8_t run_right[6][6] = {
//		{0x98,0x7C,0x3E,0x2A,0xC0,0x00},
//		{0x80,0x40,0x38,0x38,0xDE,0x1A},
//		{0x98,0x7C,0x3E,0x2A,0xC0,0x00},
//		{0xC4,0x3E,0x1F,0x6D,0x80,0x00},
//		{0x80,0x40,0x24,0x1E,0xFF,0x0D},
//		{0xC4,0x3E,0x1F,0x6D,0x80,0x00}
//	};

    // 往左靠
    const uint8_t run_left[9][8] = {
		{0x00,0x00,0xDA,0x5E,0x3C,0xD8,0x00,0x00},
		{0x00,0xDA,0x5E,0x3C,0x58,0x80,0x00,0x00},
		{0xDA,0x5E,0x3C,0x58,0x40,0x80,0x00,0x00},
		{0x00,0x00,0xDA,0x5E,0x3C,0xD8,0x00,0x00},
		{0x00,0x00,0xED,0x2F,0x3E,0xCC,0x00,0x00},
		{0x00,0xED,0x2F,0x1E,0x2C,0xC0,0x00,0x00},
		{0xED,0x2F,0x3E,0x2C,0x40,0x80,0x00,0x00},
		{0x00,0xED,0x2F,0x1E,0x2C,0xC0,0x00,0x00},
		{0x00,0x00,0xED,0x2F,0x3E,0xCC,0x00,0x00}
	};

	uint8_t FPS_Loading = 5; // 帧率
	int FPS_all_move = sizeof(run_right) / 6;
	
    for(uint8_t t = 0; t < loops; t++) 
    {
        // 向右靠
        for(int pos = -6; pos <= 32; pos++) 
        {
            memset(disp_buf, 0, sizeof(disp_buf)); 
            
            uint8_t frame_idx = abs(pos) % FPS_all_move;
            const uint8_t *frame = run_right[frame_idx];
            
            // 写入 6 列像素
            for(int i = 0; i < 6; i++) 
            {
                int x = pos + i;
                if(frame_idx == 1 || frame_idx == 2 || frame_idx == 4) x-=1; // 在前几帧之间暂停一步，为了让鸡脚不要溜冰 
				if(x >= 0 && x < 32) disp_buf[x] = frame[i];
            }
            Max7219_Refresh();
            Delay_ms(1000 / FPS_Loading);
        }
		/*
		// 铁山靠不该向左！
        for(int pos = 32; pos >= -6; pos--) 
        {
            memset(disp_buf, 0, sizeof(disp_buf));
            
            uint8_t frame_idx = abs(pos) % FPS_all_move;
            const uint8_t *frame = run_left[frame_idx];
            
            for(int i = 0; i < 6; i++) 
            {
                int x = pos + i;
                if(x >= 0 && x < 32) disp_buf[x] = frame[i];
            }
            Max7219_Refresh();
            Delay_ms(700 / FPS_Loading);
        }
		*/
        
		for(int times = 0; times < 5; times++)
		{
			// 逐帧播放（总共 9 帧）
			for(uint8_t frame = 0; frame < 9; frame++)
			{
				// 每画新的一帧前，先清空内存画布
				memset(disp_buf, 0, sizeof(disp_buf));

				// 遍历 4 个模块，将同一帧数据同时写入它们
				for(uint8_t m = 0; m < Max7219_MODULES; m++)
				{
					// 每个模块占用 8 列 (m * 8)
					// 为了让 6 列的画面居中，我们从第 1 列开始写 (偏移量 + 1)
					// 这样就形成了: 1列空 + 6列画面 + 1列空 = 8列
					uint8_t offset = m * 8 + 1; 
					
					for(uint8_t col = 0; col < 6; col++)
					{
						disp_buf[offset + col] = run_right[frame][col];
					}
				}
				
				// 4个屏幕的内存都写好后，一次性推送到物理屏幕
				Max7219_Refresh();
				
				Delay_ms(140); 
			}
		}
        
    }
    
    // 跑完清空屏幕，避免残留
    memset(disp_buf, 0, sizeof(disp_buf));
    Max7219_Refresh();
}
