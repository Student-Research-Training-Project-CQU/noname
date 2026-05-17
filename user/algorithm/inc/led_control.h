#ifndef __LED_CONTROL_H
#define __LED_CONTROL_H

#include "../../../Core/Inc/main.h"
#include <stdint.h>
#include <stdbool.h>
// 单位：毫米 (mm)
// 修改这些值就能全局改变渐变范围
#define SAFE_DISTANCE_MAX     820    // 大于等于这个值 → 完全安全（暗绿）
#define GRADIENT_START        800    // 渐变开始点（从安全绿开始变黄）
#define YELLOW_POINT          600     // 到达纯黄色的距离
#define ORANGE_POINT          400     // 到达橙色的距离
#define RED_POINT             200     // 到达纯红色的距离（最大危险）
#define CRITICAL_THRESHOLD    100     // 小于这个值可用于额外强调（如闪烁）
#define NUM_DIRECTIONS 4//方向数量（前、右、后、左）
#define LEDS_PER_DIRECTION 2//每个方向的LED数量

// 定义分母
#define DANGER_RANGE          (GRADIENT_START - RED_POINT)   //


void LED_Update_By_Lidar(void);

// 新的三个阈值接口（对应上位机的t1, t2, t3）
void LED_Set_Safe(uint16_t value);
void LED_Set_Caution(uint16_t value);
void LED_Set_Warning(uint16_t value);
uint16_t LED_Get_Safe(void);
uint16_t LED_Get_Caution(void);
uint16_t LED_Get_Warning(void);

// 保留原来的函数用于向后兼容
void LED_Set_Safe_Max(uint16_t value);
void LED_Set_Gradient_Start(uint16_t value);
void LED_Set_Yellow_Point(uint16_t value);
void LED_Set_Orange_Point(uint16_t value);
void LED_Set_Red_Point(uint16_t value);
void LED_Set_Critical(uint16_t value);
uint16_t LED_Get_Safe_Max(void);
uint16_t LED_Get_Gradient_Start(void);
uint16_t LED_Get_Yellow_Point(void);
uint16_t LED_Get_Orange_Point(void);
uint16_t LED_Get_Red_Point(void);
uint16_t LED_Get_Critical(void);


/*这些是曾经的代码，暂时留作参考
// LED警示模式
typedef enum {
    LED_MODE_OFF = 0,       // 关闭
    LED_MODE_STANDBY,       // 待机（慢呼吸蓝）
    LED_MODE_CAUTION,       // 注意（慢闪黄）
    LED_MODE_WARNING,       // 警告（快闪橙）
    LED_MODE_DANGER,        // 危险（常亮红）
    LED_MODE_ALARM,         // 紧急（快速闪烁红）
    LED_MODE_INIT,          // 初始化（绿色呼吸）
    LED_MODE_ERROR          // 错误（红蓝交替）

} LedAlertMode_t;

// 距离警示级别
typedef enum {
    DISTANCE_SAFE = 0,      // 安全（>1500mm）
    DISTANCE_CAUTION,       // 注意（1000-1500mm）
    DISTANCE_WARNING,       // 警告（500-1000mm）
    DISTANCE_DANGER,        // 危险（200-500mm）
    DISTANCE_CRITICAL       // 紧急（<200mm）

} DistanceAlertLevel_t;
void LED_Control_Init(void);
void LED_Control_Update(bool obstacle_detected, uint16_t distance_mm);
void LED_Control_Run(void);
void LED_Control_Set_Mode(LedAlertMode_t mode);
void LED_Control_Test_Pattern(void);
void LED_Control_Distance_Gradient(uint16_t distance_mm);
*/
#endif /* __LED_CONTROL_H */