//
// Created by FuShenzhe on 2026/4/28.
//
#ifndef HOST_CMD_H
#define HOST_CMD_H

#include <stdint.h>

/*
 * 命令帧格式（17字节）：
 * [0] 0xAA       帧头
 * [1] CMD        命令字 (0x10=设置阈值)
 * [2] 阈值1低字节（安全阈值）
 * [3] 阈值1高字节
 * [4] 阈值2低字节（注意阈值）
 * [5] 阈值2高字节
 * [6] 阈值3低字节（警告阈值）
 * [7] 阈值3高字节
 * [8] LED数量     灯珠数量
 * [9-15] 0x00   预留
 * [16] CRC8     校验
 */

#define THRESHOLD_FRAME_LEN    17
#define THRESHOLD_FRAME_HEADER 0xAA
#define THRESHOLD_CMD_SET      0x10

#define CMD_FRAME_HEAD   0xAA
#define CMD_FRAME_LEN    5

// 命令字定义
#define CMD_LED_OFF      0x01   // 关闭所有LED
#define CMD_LED_ON       0x02   // 打开所有LED（白色全亮）
#define CMD_LED_COLOR    0x03   // 设置全部LED颜色（PARAM_H=色调0~255, PARAM_L=亮度）
#define CMD_LED_MODE     0x04   // 切换模式（PARAM_L: 0=自动雷达, 1=手动, 2=彩虹, 3=流水）
#define CMD_LED_BRIGHT   0x05   // 设置亮度（PARAM_L: 0~255）
#define CMD_QUERY_STATUS 0x10   // 查询状态（STM32回复当前模式和障碍物距离）

extern uint8_t g_led_mode;
extern uint8_t g_brightness;
extern uint16_t g_threshold_safe;
extern uint16_t g_threshold_caution;
extern uint16_t g_threshold_warning;
extern uint8_t g_led_count;

void HostCmd_Parse(uint8_t *buf, uint16_t len);
uint8_t calcCRC8(const uint8_t *data, uint8_t len);

#endif
