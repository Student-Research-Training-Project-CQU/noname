//
// Created by FuShenzhe on 2026/4/28.
//
#include "host_cmd.h"
#include "ws2812.h"
#include "led_control.h"
#include "usart.h"
#include <string.h>

// 当前LED模式
// 0=自动(雷达控制)  1=手动纯色  2=彩虹  3=流水
uint8_t g_led_mode = 0;
uint8_t g_brightness = 255;

// 预警距离阈值变量（对应上位机的三个阈值）
uint16_t g_threshold_safe = 820;     // 安全阈值 (t1)
uint16_t g_threshold_caution = 600;  // 注意阈值 (t2)
uint16_t g_threshold_warning = 200;  // 警告阈值 (t3)
uint8_t g_led_count = 16;           // LED数量（默认16个）

// CRC8校验函数
uint8_t calcCRC8(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0xFF;
    for (uint8_t i = 0; i < len; i++)
    {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++)
        {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x07;
            else
                crc <<= 1;
        }
    }
    return crc;
}

// 发送应答帧给上位机
// 格式：0xBB + CMD + STATUS + DATA_H + DATA_L + CHECKSUM
static void send_ack(uint8_t cmd, uint8_t status, uint16_t data)
{
    uint8_t frame[6];
    frame[0] = 0xBB;
    frame[1] = cmd;
    frame[2] = status;
    frame[3] = (data >> 8) & 0xFF;
    frame[4] = data & 0xFF;
    frame[5] = frame[1] ^ frame[2] ^ frame[3] ^ frame[4];
    HAL_UART_Transmit(&huart1, frame, 6, 100);
}

// 解析阈值设置命令（17字节帧）
static void parse_threshold_cmd(const uint8_t *frame)
{
    // 校验CRC8
    uint8_t crc = calcCRC8(frame, THRESHOLD_FRAME_LEN - 1);
    if (crc != frame[THRESHOLD_FRAME_LEN - 1])
    {
        return; // CRC校验失败，忽略
    }

    // 解析三个阈值（小端格式）
    g_threshold_safe = (uint16_t)frame[2] | ((uint16_t)frame[3] << 8);
    g_threshold_caution = (uint16_t)frame[4] | ((uint16_t)frame[5] << 8);
    g_threshold_warning = (uint16_t)frame[6] | ((uint16_t)frame[7] << 8);
    
    // 解析LED数量
    g_led_count = frame[8];

    // 更新LED控制模块的阈值
    LED_Set_Safe(g_threshold_safe);
    LED_Set_Caution(g_threshold_caution);
    LED_Set_Warning(g_threshold_warning);
    
    // 更新LED数量
    LED_Set_Count(g_led_count);

    // 发送确认应答
    send_ack(THRESHOLD_CMD_SET, 0x00, 0);
}

void HostCmd_Parse(uint8_t *buf, uint16_t len)
{
    // 首先检查是否为阈值设置命令（15字节帧）
    if (len >= THRESHOLD_FRAME_LEN)
    {
        for (int i = 0; i <= (int)len - THRESHOLD_FRAME_LEN; i++)
        {
            if (buf[i] == THRESHOLD_FRAME_HEADER && buf[i+1] == THRESHOLD_CMD_SET)
            {
                parse_threshold_cmd(&buf[i]);
                i += THRESHOLD_FRAME_LEN - 1;
                return;
            }
        }
    }

    // 解析旧格式的5字节命令帧
    for (int i = 0; i <= (int)len - CMD_FRAME_LEN; i++)
    {
        if (buf[i] != CMD_FRAME_HEAD) continue;

        uint8_t cmd      = buf[i+1];
        uint8_t param_h  = buf[i+2];
        uint8_t param_l  = buf[i+3];
        uint8_t checksum = buf[i+4];

        // 校验
        if ((cmd ^ param_h ^ param_l) != checksum) continue;

        // 执行命令
        switch (cmd)
        {
            case CMD_LED_OFF:
                ws2812_Clear();
                ws2812_Show();
                g_led_mode = 1;  // 切换到手动模式，防止雷达覆盖
                send_ack(cmd, 0x00, 0);
                break;

            case CMD_LED_ON:
                ws2812_SetAll(255, 255, 255);
                ws2812_Show();
                g_led_mode = 1;
                send_ack(cmd, 0x00, 0);
                break;

            case CMD_LED_COLOR:
                // PARAM_H = R, PARAM_L = G, 蓝色固定为0（可以扩展）
                ws2812_SetAll(param_h, param_l, 0);
                ws2812_Show();
                g_led_mode = 1;
                send_ack(cmd, 0x00, 0);
                break;

            case CMD_LED_MODE:
                g_led_mode = param_l;
                send_ack(cmd, 0x00, g_led_mode);
                break;

            case CMD_LED_BRIGHT:
                g_brightness = param_l;
                send_ack(cmd, 0x00, g_brightness);
                break;

            case CMD_QUERY_STATUS:
                // 回复当前模式（data高字节=模式，低字节=亮度）
                send_ack(cmd, 0x00, ((uint16_t)g_led_mode << 8) | g_brightness);
                break;

            default:
                send_ack(cmd, 0xFF, 0);  // 未知命令
                break;
        }
        i += CMD_FRAME_LEN - 1;  // 跳过已处理的帧
    }
}
