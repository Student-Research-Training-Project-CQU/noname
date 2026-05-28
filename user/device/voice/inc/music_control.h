#ifndef __MUSIC_CONTROL_H
#define __MUSIC_CONTROL_H

#include <stdint.h>
#include <stdbool.h>
#include <main.h>
#include <obstacle_detect.h>
#include <usart.h>
#include <gpio.h>
#include <stm32f1xx_hal_gpio.h>


//音乐变量宏定义
#define VOLUME_MAX 30
#define VOLUME_MIN 0

//函数
void Music_Control_volume(uint8_t volume)  ;     //设置音量，范围0-30
void Music_Start(uint8_t a[]);                         //开始播放
void Music_Play(void);
void Music_Control_By_Distance(uint16_t distance);  //根据距离控制音量
uint16_t find_min_distance(const ObstacleDistance_t *dist); //寻找最小距离

#endif /* __MUSIC_CONTROL_H */