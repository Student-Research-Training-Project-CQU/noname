#ifndef __LED_CONTROL_H
#define __LED_CONTROL_H

#include "../../../Core/Inc/main.h"
#include <stdint.h>
#include <stdbool.h>
// 单位：毫米 (mm)
// 这些值会被上位机配置更新（见 uart_cmd.c）
extern uint16_t g_safe_distance_max;   // 大于等于这个值 → 完全安全（暗绿）
extern uint16_t g_gradient_start;      // 渐变开始点（从安全绿开始变黄）
extern uint16_t g_yellow_point;        // 到达纯黄色的距离
extern uint16_t g_orange_point;        // 到达橙色的距离
extern uint16_t g_red_point;           // 到达纯红色的距离（最大危险）
extern uint16_t g_critical_threshold;  // 小于这个值可用于额外强调（如闪烁）

#define NUM_DIRECTIONS 4//方向数量（前、右、后、左）
#define LEDS_PER_DIRECTION 20//每个方向的LED数量


void LED_Update_By_Lidar(void);
bool LED_Thresholds_Update(uint16_t safe, uint16_t caution, uint16_t warning);


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
