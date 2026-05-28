#include "tim.h"
#include "stdint.h"
#include "string.h"
#include "ws2812.h"

#define Code0       30
#define Code1       60
#define CodeReset   0
#define WS2812_DEFAULT_LEDS 64
#define WS2812_BITS_PER_LED 24
#define WS2812_RESET_SLOTS 80
volatile uint8_t ws2812_dma_busy = 0;
uint8_t g_num_leds = WS2812_DEFAULT_LEDS;
uint8_t g_led_r = 255;
uint8_t g_led_g = 0;
uint8_t g_led_b = 0;

// 在 DMA 完成回调里清标志（必须加这个回调！）
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    if (htim == &htim3) {  // 确认是你的定时器
        ws2812_dma_busy = 0;
        HAL_TIM_PWM_Stop_DMA(&htim3, TIM_CHANNEL_1);  // 推荐停止，防止漂移
    }
}
void ws2812_Update0()
{
    static uint16_t data[] = {
            Code1, Code1, Code1, Code1, Code1, Code1, Code1, Code1,
            Code0, Code0, Code0, Code0, Code0, Code0, Code0, Code0,
            Code0, Code0, Code0, Code0, Code0, Code0, Code0, Code0,
            // reset
                      0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,
                    0,0,0,0,0,0,0,0,0,0
        , 0,0,0,0,0,0,0,0,0,0,
                    0,0,0,0,0,0,0,0,0,0

    };
			HAL_TIM_PWM_Start_DMA(&htim3,TIM_CHANNEL_1,(uint32_t*)data,sizeof(data)/sizeof(uint16_t));
		
	}


static uint8_t led_buffer[WS2812_MAX_LEDS][3];
static uint16_t dma_buffer[WS2812_BITS_PER_LED * WS2812_MAX_LEDS + WS2812_RESET_SLOTS];



void ws2812_SetPixel(uint16_t index, uint8_t red, uint8_t green, uint8_t blue)
{
    if(index < g_num_leds && index < WS2812_MAX_LEDS)
    {
        led_buffer[index][0] = green; 
        led_buffer[index][1] = red;
        led_buffer[index][2] = blue;
    }
}


void ws2812_SetAll(uint8_t red, uint8_t green, uint8_t blue)
{
    g_led_r = red;
    g_led_g = green;
    g_led_b = blue;
    for(int i = 0; i < g_num_leds; i++)
    {
        ws2812_SetPixel(i, red, green, blue);
    }
}


void ws2812_Clear(void)
{
    memset(led_buffer, 0, sizeof(led_buffer));
}


void ws2812_Show(void)
{
    if (ws2812_dma_busy) {
        return;
    }

    uint8_t led_count = g_num_leds;
    if (led_count == 0) {
        led_count = WS2812_DEFAULT_LEDS;
    }
    if (led_count > WS2812_MAX_LEDS) {
        led_count = WS2812_MAX_LEDS;
        g_num_leds = WS2812_MAX_LEDS;
    }

    uint16_t idx = 0;
    for (uint8_t led = 0; led < led_count; led++) {
        uint8_t colors[3] = {
            led_buffer[led][0],
            led_buffer[led][1],
            led_buffer[led][2]
        };
        for (uint8_t c = 0; c < 3; c++) {
            uint8_t value = colors[c];
            for (uint8_t bit = 0; bit < 8; bit++) {
                dma_buffer[idx++] = (value & 0x80U) ? Code1 : Code0;
                value <<= 1;
            }
        }
    }

    for (uint16_t i = 0; i < WS2812_RESET_SLOTS; i++) {
        dma_buffer[idx++] = CodeReset;
    }

    ws2812_dma_busy = 1;
    if (HAL_TIM_PWM_Start_DMA(&htim3, TIM_CHANNEL_1,
                              (uint32_t*)dma_buffer,
                              idx) != HAL_OK) {
        ws2812_dma_busy = 0;
    }
}

void ws2812_RunningLight(uint8_t red, uint8_t green, uint8_t blue, uint16_t delay_ms)
{
    for(int i = 0; i < g_num_leds; i++)
    {
        ws2812_Clear();  
        ws2812_SetPixel(i, red, green, blue); 
        ws2812_Show(); 
        HAL_Delay(delay_ms);
    }
    
    for(int i = (int)g_num_leds - 1; i >= 0; i--)
    {
        ws2812_SetPixel(i, 0, 0, 0); 
        ws2812_Show(); 
        HAL_Delay(delay_ms); 
    }
}


void ws2812_RainbowRunningLight(uint16_t delay_ms)
{
    static uint8_t hue = 0;
    
    for(int i = 0; i < g_num_leds; i++)
    {
        uint8_t r, g, b;
        uint8_t h = (hue + i * 30) % 255; 
        
        if(h < 85)
        {
            r = 255 - h * 3;
            g = h * 3;
            b = 0;
        }
        else if(h < 170)
        {
            h -= 85;
            r = 0;
            g = 255 - h * 3;
            b = h * 3;
        }
        else
        {
            h -= 170;
            r = h * 3;
            g = 0;
            b = 255 - h * 3;
        }
        
        ws2812_Clear(); 
        ws2812_SetPixel(i, r, g, b); 
        ws2812_Show();
        HAL_Delay(delay_ms); 
    }
    
    hue += 10;
}
//彩虹
void test_Rainbow(void)
{
    for(int i = 0; i < 10; i++)
    {
        ws2812_RainbowRunningLight(80); 
    }
}
//流水灯红
void test_RunningLightsR(void)
{
    for(int i = 0; i < 3; i++)
    {
        ws2812_RunningLight(255, 0, 0, 100);  
    }
    
}
//流水灯绿
void test_RunningLightsG(void)
{
    
    for(int i = 0; i < 3; i++)
    {
        ws2812_RunningLight(0, 255, 0, 100);  
    }
    
}
//流水灯白
void test_RunningLightsW(void)
{
    for(int i = 0; i < 3; i++)
    {
        ws2812_RunningLight(255, 255, 255, 100); 
    }
}

/**
  * @brief  WS2812更新函数（兼容性）
  */
void ws2812_Update(void)
{
    ws2812_Show();
}

// 修改函数名以匹配头文件声明
void ws2812_RunningLightR(uint8_t red, uint8_t green, uint8_t blue, uint16_t delay_ms)
{
    ws2812_RunningLight(red, green, blue, delay_ms);
}

void ws2812_RunningLightG(uint8_t red, uint8_t green, uint8_t blue, uint16_t delay_ms)
{
    ws2812_RunningLight(red, green, blue, delay_ms);
}

void ws2812_RunningLightW(uint8_t red, uint8_t green, uint8_t blue, uint16_t delay_ms)
{
    ws2812_RunningLight(red, green, blue, delay_ms);
}
