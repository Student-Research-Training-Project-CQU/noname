#ifndef WS2812_H
#define WS2812_H

#include "main.h"

extern uint8_t g_num_leds;
extern uint8_t g_led_r;
extern uint8_t g_led_g;
extern uint8_t g_led_b;
extern volatile uint8_t ws2812_dma_busy;

#define WS2812_MAX_LEDS 255


void ws2812_SetPixel(uint16_t index, uint8_t red, uint8_t green, uint8_t blue);
void ws2812_SetAll(uint8_t red, uint8_t green, uint8_t blue);
void ws2812_Clear(void);
void ws2812_Show(void);
void ws2812_Update0();
void ws2812_RunningLightR(uint8_t red, uint8_t green, uint8_t blue, uint16_t delay_ms);
void ws2812_RunningLightG(uint8_t red, uint8_t green, uint8_t blue, uint16_t delay_ms);
void ws2812_RunningLightW(uint8_t red, uint8_t green, uint8_t blue, uint16_t delay_ms);
void ws2812_RainbowRunningLight(uint16_t delay_ms);

#endif