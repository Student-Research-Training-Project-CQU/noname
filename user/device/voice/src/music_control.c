
#include "obstacle_detect.h"
#include "music_control.h"
#include <stdint.h>
#include "main.h"
#include "usart.h"
#include "gpio.h"
#include "stm32f1xx_hal_gpio.h"
//宏定义距离阈值
#define DISTANCE_MIN 100
#define DISTANCE_MAX 1000


//定义距离变量
uint16_t find_min_distance(const ObstacleDistance_t *dist) 
{
    uint16_t min = dist->front;
    
    if (dist->right < min) min = dist->right;
    if (dist->back  < min) min = dist->back;
    if (dist->left  < min) min = dist->left;
    
    return min;
}


//usart发送
void Music_Start(uint8_t a[])
{
    HAL_Delay(10); // 确保模块准备好接收数据
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET); // 拉高控制引脚（用于测试时看是否运行此函数）
    HAL_UART_Transmit(&huart2, a, 7, 100);
}

//播放音乐
void Music_Play(void)
{
    uint8_t playCmd[] = {0xFE, 0x07, 0xFF, 0xFF, 0x02, 0x05, 0xBE};  
    Music_Start(playCmd);
}
//发送控制音量指令
void Music_Control_volume(uint8_t volume)
{
    uint8_t volumeSet[] = {0xFE,0x08,0xFF,0xFF,0x13,volume,0xBE};
    Music_Start(volumeSet);
}
int n=0;
//根据距离控制音乐音量(100~1000mm--0~30音量)
void Music_Control_By_Distance(uint16_t distance)
{
      static uint8_t has_started = 0;
    static uint32_t start_time = 0;
    static uint32_t last_time = 0;

    // ===== 第一次启动播放 =====
    if (!has_started)
    {
        Music_Play();
        start_time = HAL_GetTick();
        has_started = 1;
        return;  // 直接返回，不做音量控制
    }

    // ===== 播放后500ms内禁止任何控制=====
    if (HAL_GetTick() - start_time < 500)
        return;

    // ===== 控制发送频率 =====
    if (HAL_GetTick() - last_time < 200)
        return;

    last_time = HAL_GetTick();
    
    if (distance == 0 || distance >= DISTANCE_MAX)
    {
        Music_Control_volume(VOLUME_MIN);
    }
    else if (distance < DISTANCE_MAX && distance >= DISTANCE_MIN)
    {
        uint8_t volume = (uint8_t)((float)(DISTANCE_MIN - distance) / (float)(DISTANCE_MAX - DISTANCE_MIN) * (VOLUME_MAX - VOLUME_MIN) + VOLUME_MAX);
        Music_Control_volume(volume);
    }
    else
    {
        Music_Control_volume(VOLUME_MAX);
    }
}
