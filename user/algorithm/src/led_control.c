#include "led_control.h"
#include "ws2812.h"
#include "obstacle_detect.h"

// 动态阈值变量（对应上位机的三个阈值）
static uint16_t threshold_safe = 820;     // 安全阈值 (t1)
static uint16_t threshold_caution = 600;  // 注意阈值 (t2)
static uint16_t threshold_warning = 200;  // 警告阈值 (t3)
static uint8_t led_count = 16;          // LED数量

// 阈值设置函数
void LED_Set_Safe(uint16_t value) {
    threshold_safe = value;
}

void LED_Set_Caution(uint16_t value) {
    threshold_caution = value;
}

void LED_Set_Warning(uint16_t value) {
    threshold_warning = value;
}

// 阈值获取函数
uint16_t LED_Get_Safe(void) {
    return threshold_safe;
}

uint16_t LED_Get_Caution(void) {
    return threshold_caution;
}

uint16_t LED_Get_Warning(void) {
    return threshold_warning;
}

// LED数量设置和获取函数
void LED_Set_Count(uint8_t count) {
    // 限制LED数量在合理范围（4-64个）
    if (count >= 4 && count <= 64) {
        led_count = count;
    }
}

uint8_t LED_Get_Count(void) {
    return led_count;
}

// 保留原来的函数用于向后兼容
void LED_Set_Safe_Max(uint16_t value) {
    threshold_safe = value - 20;
}

void LED_Set_Gradient_Start(uint16_t value) {
    threshold_safe = value;
}

void LED_Set_Yellow_Point(uint16_t value) {
    threshold_caution = value;
}

void LED_Set_Orange_Point(uint16_t value) {
    threshold_caution = (value * 3 - threshold_warning) / 2;
}

void LED_Set_Red_Point(uint16_t value) {
    threshold_warning = value;
}

void LED_Set_Critical(uint16_t value) {
    threshold_warning = value * 2;
}

uint16_t LED_Get_Safe_Max(void) {
    return threshold_safe + 20;
}

uint16_t LED_Get_Gradient_Start(void) {
    return threshold_safe;
}

uint16_t LED_Get_Yellow_Point(void) {
    return threshold_caution;
}

uint16_t LED_Get_Orange_Point(void) {
    return (threshold_caution + threshold_warning) / 2;
}

uint16_t LED_Get_Red_Point(void) {
    return threshold_warning;
}

uint16_t LED_Get_Critical(void) {
    return threshold_warning / 2;
}

/**
  * @brief      根据距离计算RGB颜色，距离越近颜色越红，距离越远颜色越绿，中间平滑过渡
  * @paragraph distance: 距离值，单位毫米
  * @paragraph r, g, b: 输出的RGB颜色值指针
  * @paragraph brightness: 亮度缩放（0-255），可以用于调暗或闪烁效果，目前默认为255（全亮）
  * @details    该函数根据输入的距离值计算对应的RGB颜色。使用三个阈值进行渐变。
  */
static void get_rgb_by_distance(uint16_t distance, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t brightness)
{
    if (distance == 0 || distance >= threshold_safe)
    {
        // 安全 → 暗绿
        *r = 0;
        *g = 40;
        *b = 0;
        return;
    }

    // 计算危险系数 0.0（安全） ~ 1.0（最危险）
    float danger = 0.0f;
    if (distance <= threshold_warning)
    {
        danger = 1.0f;
    }
    else if (distance < threshold_safe)
    {
        danger = (float)(threshold_safe - distance) / (float)(threshold_safe - threshold_warning);
    }

    // 多段线性插值
    uint8_t r1, g1, b1, r2, g2, b2;
    float t;  // 当前段的比例 0.0 ~ 1.0

    // 计算颜色过渡点（基于danger系数）
    float transition1 = 0.333f;  // 绿到黄的分界
    float transition2 = 0.666f;  // 黄到橙的分界

    if (danger <= transition1)
    {
        // 段1：绿 → 黄
        t = danger / transition1;
        r1 = 0;   g1 = 255; b1 = 0;
        r2 = 255; g2 = 255; b2 = 0;
    }
    else if (danger <= transition2)
    {
        // 段2：黄 → 橙
        t = (danger - transition1) / (transition2 - transition1);
        r1 = 255; g1 = 255; b1 = 0;
        r2 = 255; g2 = 140; b2 = 0;
    }
    else
    {
        // 段3：橙 → 红
        t = (danger - transition2) / (1.0f - transition2);
        r1 = 255; g1 = 140; b1 = 0;
        r2 = 255; g2 = 0;   b2 = 0;
    }

    // 线性插值计算当前颜色
    *r = (uint8_t)(r1 + t * (r2 - r1));
    *g = (uint8_t)(g1 + t * (g2 - g1));
    *b = (uint8_t)(b1 + t * (b2 - b1));

    // 应用亮度缩放
    *r = (uint8_t)((uint16_t)*r * brightness / 255);
    *g = (uint8_t)((uint16_t)*g * brightness / 255);
    *b = (uint8_t)((uint16_t)*b * brightness / 255);
}

/**
 * @param dir_index 方向索引：0=前，1=右，2=后，3=左
 * @param distance 距离值，单位毫米
 */
static void LED_Show_Direction(uint8_t dir_index, uint16_t distance)
{
    if (distance <= 0) return;

    uint8_t r, g, b;
    uint8_t brightness = 255;  // 默认满亮

    get_rgb_by_distance(distance, &r, &g, &b, brightness);
    
    // 动态计算每个方向的LED数量和起始位置
    uint8_t leds_per_dir = led_count / 4;  // 平均分配给四个方向
    uint8_t start_idx = dir_index * leds_per_dir;
    
    // 设置该方向的所有LED
    for (uint8_t i = 0; i < leds_per_dir; i++) {
        ws2812_SetPixel(start_idx + i, r, g, b);
    }
}

void LED_Update_By_Lidar(void)
{
    ObstacleDistance_t obs = Obstacle_Get();

    LED_Show_Direction(0, obs.front);
    LED_Show_Direction(1, obs.right);
    LED_Show_Direction(2, obs.back);
    LED_Show_Direction(3, obs.left);

    ws2812_Show();
}
