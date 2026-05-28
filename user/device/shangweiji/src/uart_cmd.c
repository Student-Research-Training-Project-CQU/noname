#include "uart_cmd.h"
#include "usart.h"
#include "led_control.h"
#include "ws2812.h"
#include "main.h"
#define FLASH_PAGE_SIZE 1024U
#define CONFIG_MAGIC 0x534F4C43U

#define UART1_RX_DMA_BUF_LEN 14
#define CMD_FRAME_LEN 14
#define CMD_HEADER 0xAA
#define CMD_SET_THRESHOLDS 0x10

static uint8_t s_uart1_dma_buf[UART1_RX_DMA_BUF_LEN];
static uint8_t s_rx_cache[UART1_RX_DMA_BUF_LEN];
static uint16_t s_rx_cache_len = 0;
static uint8_t s_ok_reply[4] = { 'O', 'K', '\r', '\n' };
static volatile uint8_t s_ok_pending = 0;

typedef struct {
    uint32_t magic;
    uint16_t safe_distance_max;
    uint16_t gradient_start;
    uint16_t yellow_point;
    uint16_t orange_point;
    uint16_t red_point;
    uint16_t critical_threshold;
    uint8_t num_leds;
    uint8_t reserved[3];
    uint32_t checksum;
} FlashConfig_t;

static uint32_t config_checksum(const uint8_t *data, uint32_t length)
{
    uint32_t sum = 0;
    for (uint32_t i = 0; i < length; i++) {
        sum += data[i];
    }
    return sum;
}

static uint32_t config_flash_address(void)
{
    uint32_t flash_kb = (uint32_t)(*(uint16_t *)FLASHSIZE_BASE);
    uint32_t flash_bytes = flash_kb * 1024U;
    return FLASH_BASE + flash_bytes - FLASH_PAGE_SIZE;
}

static bool config_is_valid(const FlashConfig_t *cfg)
{
    if (cfg->magic != CONFIG_MAGIC) {
        return false;
    }
    uint32_t calc = config_checksum((const uint8_t *)cfg, sizeof(FlashConfig_t) - sizeof(uint32_t));
    if (calc != cfg->checksum) {
        return false;
    }
    if (cfg->safe_distance_max == 0U || cfg->yellow_point == 0U || cfg->orange_point == 0U) {
        return false;
    }
    if (cfg->safe_distance_max <= cfg->yellow_point || cfg->yellow_point <= cfg->orange_point) {
        return false;
    }
    if (cfg->red_point == 0U || cfg->critical_threshold == 0U) {
        return false;
    }
    if (cfg->red_point <= cfg->critical_threshold) {
        return false;
    }
    if (cfg->num_leds > WS2812_MAX_LEDS) {
        return false;
    }
    return true;
}

static void apply_config(const FlashConfig_t *cfg)
{
    g_safe_distance_max = cfg->safe_distance_max;
    g_gradient_start = cfg->gradient_start;
    g_yellow_point = cfg->yellow_point;
    g_orange_point = cfg->orange_point;
    g_red_point = cfg->red_point;
    g_critical_threshold = cfg->critical_threshold;
    if (cfg->num_leds != 0U) {
        g_num_leds = cfg->num_leds;
    }
}

void UART1_Cmd_LoadConfig(void)
{
    const FlashConfig_t *cfg = (const FlashConfig_t *)config_flash_address();
    if (config_is_valid(cfg)) {
        apply_config(cfg);
    }
}

bool UART1_Cmd_SaveConfig(void)
{
    FlashConfig_t cfg = {0};
    cfg.magic = CONFIG_MAGIC;
    cfg.safe_distance_max = g_safe_distance_max;
    cfg.gradient_start = g_gradient_start;
    cfg.yellow_point = g_yellow_point;
    cfg.orange_point = g_orange_point;
    cfg.red_point = g_red_point;
    cfg.critical_threshold = g_critical_threshold;
    cfg.num_leds = g_num_leds;
    cfg.checksum = config_checksum((const uint8_t *)&cfg, sizeof(FlashConfig_t) - sizeof(uint32_t));

    uint32_t address = config_flash_address();

    HAL_FLASH_Unlock();

    FLASH_EraseInitTypeDef erase = {0};
    uint32_t error_page = 0;
    erase.TypeErase = FLASH_TYPEERASE_PAGES;
    erase.PageAddress = address;
    erase.NbPages = 1;
    if (HAL_FLASHEx_Erase(&erase, &error_page) != HAL_OK) {
        HAL_FLASH_Lock();
        return false;
    }

    const uint32_t *data = (const uint32_t *)&cfg;
    uint32_t word_count = (uint32_t)(sizeof(FlashConfig_t) / sizeof(uint32_t));
    for (uint32_t i = 0; i < word_count; i++) {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address + (i * 4U), data[i]) != HAL_OK) {
            HAL_FLASH_Lock();
            return false;
        }
    }

    HAL_FLASH_Lock();
    return true;
}

static void uart1_send_ok_now(void)
{
    (void)HAL_UART_Transmit_DMA(&huart1, s_ok_reply, (uint16_t)sizeof(s_ok_reply));
}

static void uart1_send_ok(void)
{
    if (HAL_UART_GetState(&huart1) == HAL_UART_STATE_READY) {
        uart1_send_ok_now();
    } else if (s_ok_pending < 255U) {
        s_ok_pending++;
    }
}

static bool uart1_parse_frame(const uint8_t *frame)
{
    if (frame[0] != CMD_HEADER || frame[1] != CMD_SET_THRESHOLDS) {
        return false;
    }
    for (uint8_t i = 9; i < CMD_FRAME_LEN; i++) {
        if (frame[i] != 0x00) {
            return false;
        }
    }

    uint16_t t1 = (uint16_t)(frame[2] | ((uint16_t)frame[3] << 8));
    uint16_t t2 = (uint16_t)(frame[4] | ((uint16_t)frame[5] << 8));
    uint16_t t3 = (uint16_t)(frame[6] | ((uint16_t)frame[7] << 8));
    uint8_t new_num_leds = frame[8];

    bool thresholds_ok = LED_Thresholds_Update(t1, t2, t3);
    bool leds_updated = false;

    // 灯珠数量更新独立处理，不依赖阈值更新的结果
    if (new_num_leds != 0U) {
        if (new_num_leds > WS2812_MAX_LEDS) {
            new_num_leds = WS2812_MAX_LEDS;
        }
        if (new_num_leds != g_num_leds) {
            // 等待上一次DMA完成
            uint32_t timeout = 50;  // 最多等待50ms
            while (ws2812_dma_busy && timeout--) {
                HAL_Delay(1);
            }
            
            g_num_leds = new_num_leds;
            ws2812_Clear();
            ws2812_SetAll(g_led_r, g_led_g, g_led_b);
            ws2812_Show();
            leds_updated = true;
        }
    }

    // 如果阈值或灯珠数量有任何更新，就保存配置并回复OK
    if (thresholds_ok || leds_updated) {
        UART1_Cmd_SaveConfig();
        uart1_send_ok();
        return true;
    }

    return false;
}

void UART1_Cmd_Init(void)
{
    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, s_uart1_dma_buf, UART1_RX_DMA_BUF_LEN);
    if (huart1.hdmarx != NULL) {
        __HAL_DMA_DISABLE_IT(huart1.hdmarx, DMA_IT_HT);
    }
}

uint8_t *UART1_Cmd_GetRxBuffer(void)
{
    return s_uart1_dma_buf;
}

uint16_t UART1_Cmd_GetRxBufferSize(void)
{
    return UART1_RX_DMA_BUF_LEN;
}

void UART1_Cmd_OnTxDone(void)
{
    if (s_ok_pending > 0U) {
        s_ok_pending--;
        uart1_send_ok_now();
    }
}

void UART1_Cmd_OnRxData(const uint8_t *data, uint16_t len)
{

    if (data == NULL || len == 0) {
        return;
    }

    if (len > UART1_RX_DMA_BUF_LEN) {
        data += (len - UART1_RX_DMA_BUF_LEN);
        len = UART1_RX_DMA_BUF_LEN;
    }

    uint16_t space = UART1_RX_DMA_BUF_LEN - s_rx_cache_len;
    if (len > space) {
        s_rx_cache_len = 0;
    }

    for (uint16_t i = 0; i < len; i++) {
        s_rx_cache[s_rx_cache_len++] = data[i];
    }

    while (s_rx_cache_len >= CMD_FRAME_LEN) {
        if (s_rx_cache[0] != CMD_HEADER) {
            for (uint16_t i = 1; i < s_rx_cache_len; i++) {
                if (s_rx_cache[i] == CMD_HEADER) {
                    for (uint16_t j = 0; j + i < s_rx_cache_len; j++) {
                        s_rx_cache[j] = s_rx_cache[j + i];
                    }
                    s_rx_cache_len -= i;
                    break;
                }
            }
            if (s_rx_cache[0] != CMD_HEADER) {
                s_rx_cache_len = 0;
                break;
            }
        }

        if (s_rx_cache_len < CMD_FRAME_LEN) {
            break;
        }


        (void)uart1_parse_frame(s_rx_cache);




        for (uint16_t i = CMD_FRAME_LEN; i < s_rx_cache_len; i++) {
            s_rx_cache[i - CMD_FRAME_LEN] = s_rx_cache[i];
        }
        s_rx_cache_len -= CMD_FRAME_LEN;


    }
}
