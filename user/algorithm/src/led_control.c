#include "led_control.h"
#include "ws2812.h"
#include "obstacle_detect.h"
/**
  * @brief      根据距离计算RGB颜色，距离越近颜色越红，距离越远颜色越绿，中间平滑过渡
  * @paragraph distance: 距离值，单位毫米
  * @paragraph r, g, b: 输出的RGB颜色值指针
  * @paragraph brightness: 亮度缩放（0-255），可以用于调暗或闪烁效果，目前默认为255（全亮）
  * @details    该函数根据输入的距离值计算对应的RGB颜色。距离为0或大于等于SAFE_DISTANCE_MAX时，返回暗绿色（非常安全）。在RED_POINT到GRADIENT_START之间，颜色从绿逐渐过渡到黄；在GRADIENT_START到RED_POINT之间，颜色从黄逐渐过渡到橙；在RED_POINT以下，颜色从橙逐渐过渡到红。通过线性插值实现平滑过渡，并且可以通过brightness参数调整整体亮度。
  * @note       这种基于距离的颜色计算方法比简单的分级显示更直观，可以让用户更清晰地感知距离的变化，尤其是在临界点附近。同时，保留了亮度缩放的接口，方便未来添加闪烁或呼吸灯效果来强调紧急情况。
  * @example    如果distance=900mm，函数会计算出一个介于绿和黄之间的颜色；如果distance=300mm，函数会计算出一个介于橙和红之间的颜色；如果distance=0mm，函数会返回暗绿色。
  * @warning    确保输入的距离值在合理范围内，否则可能会得到不预期的颜色结果。建议在调用该函数前先进行距离的有效性检查。
  * @see        get_distance_level() // 如果需要分级显示，可以先调用该函数获取级别，再根据级别设置颜色，但线性渐变通常更美观且更直观。
  */
static void get_rgb_by_distance(uint16_t distance, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t brightness)
{
    if (distance == 0 || distance >= SAFE_DISTANCE_MAX)
    {
        // 非常安全 → 暗绿
        *r = 0;
        *g = 40;
        *b = 0;
        return;
    }

    // 计算危险系数 0.0（安全） ~ 1.0（最危险）
    float danger = 0.0f;
    if (distance <= RED_POINT)
    {
        danger = 1.0f;
    }
    else if (distance < GRADIENT_START)
    {
        danger = (float)(GRADIENT_START - distance) / (float)DANGER_RANGE;
    }
    // danger 现在是 0.0 ~ 1.0

    // 多段线性插值
    uint8_t r1, g1, b1, r2, g2, b2;
    float t;  // 当前段的比例 0.0 ~ 1.0

    if (danger <= 0.333f)
    {
        // 段1：绿 → 黄
        t = danger / 0.333f;
        r1 = 0;   g1 = 255; b1 = 0;
        r2 = 255; g2 = 255; b2 = 0;
    }
    else if (danger <= 0.666f)
    {
        // 段2：黄 → 橙
        t = (danger - 0.333f) / 0.333f;
        r1 = 255; g1 = 255; b1 = 0;
        r2 = 255; g2 = 140; b2 = 0;
    }
    else
    {
        // 段3：橙 → 红
        t = (danger - 0.666f) / 0.334f;
        r1 = 255; g1 = 140; b1 = 0;
        r2 = 255; g2 = 0;   b2 = 0;
    }

    // 线性插值计算当前颜色
    *r = (uint8_t)(r1 + t * (r2 - r1));
    *g = (uint8_t)(g1 + t * (g2 - g1));
    *b = (uint8_t)(b1 + t * (b2 - b1));

    // 应用亮度缩放（如果以后需要调暗或闪烁）
    *r = (uint8_t)((uint16_t)*r * brightness / 255);
    *g = (uint8_t)((uint16_t)*g * brightness / 255);
    *b = (uint8_t)((uint16_t)*b * brightness / 255);
}

/**
 * @param dir_index 方向索引：0=前，1=右，2=后，3=左
 * @param distance 距离值，单位毫米
 * @details 根据距离值计算对应的RGB颜色，并设置对应方向的LED颜色。如果距离为0或大于等于安全距离，则LED显示暗绿色；如果距离在渐变范围内，则LED颜色根据距离线性过渡，从绿到黄再到红。通过调用get_rgb_by_distance函数实现颜色计算，并将结果应用到对应方向的LED上。每个方向有两个LED，确保它们显示相同的颜色以增强视觉效果。
 * @note 该函数假设LED索引按照方向顺序排列（前2个LED为前方，接着是右、后、左），并且每个方向的LED数量由LEDS_PER_DIRECTION定义。调用该函数前应确保输入的距离值在合理范围内，以获得正确的颜色显示。
 * @example 如果dir_index=0（前方）且distance=300mm，函数会计算出一个介于橙和红之间的颜色，并将前方的两个LED设置为该颜色；如果dir_index=1（右侧）且distance=900mm，函数会计算出一个介于绿和黄之间的颜色，并将右侧的两个LED设置为该颜色；如果dir_index=2（后方）且distance=0mm，函数会返回暗绿色，并将后方的两个LED设置为暗绿色。
 * @warning 确保dir_index在0到3之间，否则可能会访问越界的LED索引。建议在调用该函数前先验证dir_index的有效性，以避免潜在的内存访问错误。同时，确保输入的距离值在合理范围内，以获得正确的颜色显示。
 */
static void LED_Show_Direction(uint8_t dir_index, uint16_t distance)
{
    if (distance <= 0) return;

    uint8_t r, g, b;
    uint8_t brightness = 255;  // 默认满亮

    get_rgb_by_distance(distance, &r, &g, &b, brightness);
    uint8_t start_idx = dir_index * LEDS_PER_DIRECTION;
    ws2812_SetPixel(start_idx, r, g, b);
    ws2812_SetPixel(start_idx + 1, r, g, b);
}

/**
 * @details 该函数获取当前的障碍物距离信息，并根据每个方向的距离值更新对应方向的LED颜色。它调用Obstacle_Get()函数获取前、右、后、左四个方向的距离值，并依次调用LED_Show_Direction()函数为每个方向设置LED颜色。最后，通过调用ws2812_Show()函数将更新后的LED状态显示出来。确保在调用该函数前，相关的障碍物检测逻辑已经正确更新了距离信息，以便LED能够准确反映当前的环境状况。
 * @note 该函数假设LED索引按照方向顺序排列（前2个LED为前方，接着是右、后、左），并且每个方向的LED数量由LEDS_PER_DIRECTION定义。调用该函数前应确保障碍物距离信息已经更新，以获得正确的颜色显示。
 * @example 如果当前障碍物距离信息为前方300mm、右侧900mm、后方0mm、左侧1500mm，函数会将前方的LED设置为介于橙和红之间的颜色，右侧的LED设置为介于绿和黄之间的颜色，后方的LED设置为暗绿色，左侧的LED设置为暗绿色，并最终显示出来。
 * @warning 确保障碍物距离信息已经更新，否则LED显示可能不准确。建议在调用该函数前先调用相关的障碍物检测更新函数，以确保获取到最新的距离信息。同时，确保LED显示逻辑正确处理了距离值为0的情况，以避免显示错误的颜色。
 */
void LED_Update_By_Lidar(void)
{
    ObstacleDistance_t obs = Obstacle_Get();

    LED_Show_Direction(0, obs.front);
    LED_Show_Direction(1, obs.right);
    LED_Show_Direction(2, obs.back);
    LED_Show_Direction(3, obs.left);

    ws2812_Show();
}


/*此部分为距离阈值控制灯带的颜色，原本设计了分级显示，但实际测试后发现线性渐变更美观且更直观，所以改为根据距离直接计算颜色。以下是原来的分级设计，保留以备参考：
// 阈值（mm）  SAFE → CAUTION → WARNING → DANGER → CRITICAL
static const uint16_t level_thresholds[] = {600, 500, 400, 300, 222};

static DistanceAlertLevel_t get_distance_level(uint16_t dist)
{
    if (dist == 0 || dist > level_thresholds[0]) return DISTANCE_SAFE;
    if (dist > level_thresholds[1]) return DISTANCE_CAUTION;
    if (dist > level_thresholds[2]) return DISTANCE_WARNING;
    if (dist > level_thresholds[3]) return DISTANCE_DANGER;
    return DISTANCE_CRITICAL;
}

static void get_rgb_by_level(DistanceAlertLevel_t level, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t brightness_scale)
{
    uint8_t base_r = 0, base_g = 0, base_b = 0;

    switch(level)
    {
        case DISTANCE_SAFE:     base_r=0;   base_g=200; base_b=0;   break;  // 绿
        case DISTANCE_CAUTION:  base_r=220; base_g=220; base_b=0;   break;  // 黄
        case DISTANCE_WARNING:  base_r=255; base_g=140; base_b=0;   break;  // 橙
        case DISTANCE_DANGER:   base_r=255; base_g=60;  base_b=0;   break;  // 红橙
        case DISTANCE_CRITICAL: base_r=255; base_g=0;   base_b=0;   break;  // 红
        default:                base_r=60;  base_g=60;  base_b=60;  break;
    }

    *r = (uint8_t)((uint16_t)base_r * brightness_scale / 255);
    *g = (uint8_t)((uint16_t)base_g * brightness_scale / 255);
    *b = (uint8_t)((uint16_t)base_b * brightness_scale / 255);
}*/