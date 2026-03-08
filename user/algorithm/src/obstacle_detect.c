// obstacle_detect.c
// -----------------------------------------------------------------------------
// 文件说明：
//  这个模块从雷达点云数据（Dataprocess 数组）中提取四个方向（前、右、后、左）的最小障碍物距离，并对结果做简单的指数平滑滤波。
//  目标：为上层控制或避障逻辑提供稳定的四向障碍物距离信息（单位：毫米）。
//  设计要点：
//   - 将一圈点划分为四个扇区（每个扇区中心在 0°, 90°, 180°, 270°），每个扇区覆盖 ±SECTOR_HALF 度。
//   - 忽略距离为 0（无回波）或置信度低于阈值的点，以减少误报。
//   - 在每个扇区内取最小距离作为该方向的障碍物距离；只有当该扇区的有效采样点数达到最小要求时才更新距离。
//   - 使用简单的低通滤波（指数平滑）对测得的最小距离进行平滑，避免由于单帧噪声导致的剧烈波动。
//
//  说明：Dataprocess 数组、ObstacleDistance_t 类型和相关头文件在本模块的头文件中声明（见 obstacle_detect.h 和 lidar.h）。
// -----------------------------------------------------------------------------

#include "obstacle_detect.h"
#include "lidar.h"

//  定义滤波系数
#define FILTER_ALPHA 0.9f

// 使用四个 90° 扇区的中心角（度）
#define FRONT_CENTER  0.0f
#define RIGHT_CENTER  90.0f
#define BACK_CENTER  180.0f
#define LEFT_CENTER  270.0f

// 每个方向检测 ±SECTOR_HALF° 范围（例如 SECTOR_HALF=50 则每扇区宽度约 100°）
#define SECTOR_HALF   50.0f

/**
 * filter - 对距离值做简单的指数平滑滤波
 * @param old: 上一帧/滤波后的距离值（mm）
 * @param new_val: 本次测得的新距离值（mm），0 表示无效读数（忽略）
 *
 * 返回：滤波后的距离（mm）。
 * 说明：如果 new_val 为 0（无效），直接返回 old，避免把无回波覆盖掉已知安全距离。
 *       否则使用 alpha*old + (1-alpha)*new 的形式，并四舍五入到整数。
 */
static uint16_t filter(uint16_t old, uint16_t new_val) {
    if (new_val == 0) return old; // 忽略无效读数
    // 使用显式类型转换避免从整型到浮点的隐式窄化转换告警
    return (uint16_t)(((float)old) * FILTER_ALPHA + ((float)new_val) * (1.0f - FILTER_ALPHA) + 0.5f);
}

// 当前四个方向的滤波后障碍物距离，初始值设为较大的安全距离（单位 mm）
static ObstacleDistance_t obstacle = {1500, 1500, 1500, 1500};  // 初始值设为安全距离

/**
 * Obstacle_Detect_Update - 从全角点云中统计每个方向的最小距离并更新滤波值
 *
 * 工作流程：
 *  1. 遍历 Dataprocess 点云（假设一圈约 720 点）。
 *  2. 过滤掉距离为 0 或置信度小于阈值（此处使用 40）的点。
 *  3. 将角度统一映射到 [0, 360) 区间。
 *  4. 根据角度判断点属于哪个扇区（前/右/后/左），在对应扇区中记录最小距离并计数。
 *  5. 扇区内若有效点数达到最小采样（此处 >=3）则用 filter() 更新对应方向的 obstacle 值。
 *
 *  注意点/理由：
 *   - 使用最小距离（min）而不是均值，是为了对潜在的最近障碍物更敏感（避障更及时）。
 *   - 使用置信度阈值可以排除低质量点（可能是噪声或失真）。
 *   - 最小采样点数检查可以避免单点噪声导致的误报。
 */
void Obstacle_Detect_Update(void)
{
    uint16_t front_min = 4000, right_min = 4000, back_min = 4000, left_min = 4000; // 初始为较大值（代表未检测到）
    uint8_t front_cnt = 0, right_cnt = 0, back_cnt = 0, left_cnt = 0; // 每个扇区的有效点计数

    for (int i = 0; i < 720; i++) {   // 假设一圈约720点
        if (Dataprocess[i].distance == 0 || Dataprocess[i].confidence < 40) {
            continue;
        }

        float a = Dataprocess[i].angle; // 点的角度（度）
        uint16_t d = Dataprocess[i].distance; // 点的距离（mm）

        // 统一转到 [0,360) 方便后续分区判断
        if (a < 0) a += 360.0f;
        if (a >= 360) a -= 360.0f;

        // 前（围绕 0°）: 角度接近 0 或接近 360
        if (a <= SECTOR_HALF || a >= (360.0f - SECTOR_HALF)) {
            if (d < front_min) front_min = d; // 取最小距离
            front_cnt++;
        }
        // 右（围绕 90°）
        else if (a >= (RIGHT_CENTER - SECTOR_HALF) && a <= (RIGHT_CENTER + SECTOR_HALF)) {
            if (d < right_min) right_min = d;
            right_cnt++;
        }
        // 后（围绕 180°）
        else if (a >= (BACK_CENTER - SECTOR_HALF) && a <= (BACK_CENTER + SECTOR_HALF)) {
            if (d < back_min) back_min = d;
            back_cnt++;
        }
        // 左（围绕 270°）
        else if (a >= (LEFT_CENTER - SECTOR_HALF) && a <= (LEFT_CENTER + SECTOR_HALF)) {
            if (d < left_min) left_min = d;
            left_cnt++;
        }
    }

    // 只有当扇区内有足够的有效采样点（3个）时才更新对应方向的距离，防止单帧噪声误触发
    if (front_cnt >= 3) obstacle.front = filter(obstacle.front, front_min);
    if (right_cnt >= 3) obstacle.right = filter(obstacle.right, right_min);
    if (back_cnt  >= 3) obstacle.back  = filter(obstacle.back,  back_min);
    if (left_cnt  >= 3) obstacle.left  = filter(obstacle.left,  left_min);
}

/**
 * Obstacle_Get - 获取当前四向的滤波后障碍物距离
 *
 * 返回：ObstacleDistance_t 结构，包含 front/right/back/left 四个方向的距离（mm）
 */
ObstacleDistance_t Obstacle_Get(void) {
    return obstacle;
}