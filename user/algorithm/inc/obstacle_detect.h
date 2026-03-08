//
// Created by Administrator on 2026/3/8.
//

#ifndef LIDARMODULE_OBSTACLE_DETECT_H
#define LIDARMODULE_OBSTACLE_DETECT_H
#include "stdint.h"
// 定义一个结构体来存储四个方向的障碍物距离
typedef struct
{
    uint16_t front;
    uint16_t right;
    uint16_t back;
    uint16_t left;
}ObstacleDistance_t;

void Obstacle_Detect_Update(void);
ObstacleDistance_t Obstacle_Get(void);
#endif //LIDARMODULE_OBSTACLE_DETECT_H