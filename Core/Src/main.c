/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "led_control.h"
#include <stdio.h>
#include <string.h>
#include "lidar.h"
#include "ws2812.h"
#include "obstacle_detect.h"

#include "../../user/device/shangweiji/inc/uart_cmd.h"
#include "oled.h"
#include "music_control.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

uint16_t rx_index = 0;
uint8_t rx_buffer[256];
uint8_t uart3_rx_buffer[512];

// 声明DMA句柄
extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_usart3_rx;

extern uint8_t g_num_leds;
uint16_t g_safe_distance_max = 820;
uint16_t g_gradient_start = 800;
uint16_t g_yellow_point = 600;
uint16_t g_orange_point = 400;
uint16_t g_red_point = 200;
uint16_t g_critical_threshold = 100;
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void print(void)
{
  const char *msg = "USART1 print test\r\n";
  HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
}
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_USART3_UART_Init();
  MX_I2C1_Init();
  MX_TIM3_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  UART1_Cmd_Init();
  UART1_Cmd_LoadConfig();
  HAL_Delay(20);
  OLED_Init();
  ws2812_SetAll(255,0,0);
/* 在需要显示的地方（例如循环内） */
char num_buf[4]; // 足够放 0-255 和终止符
  Music_Control_volume(30);
  Music_Play();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {


   // print();
      OLED_NewFrame();
      OLED_PrintString(0, 0, "小灯数量:", &font16x16, OLED_COLOR_NORMAL);
      snprintf(num_buf, sizeof(num_buf), "%u", (unsigned)g_num_leds);
      OLED_PrintASCIIString(80, 0, num_buf, &afont16x8, OLED_COLOR_NORMAL);
      OLED_PrintString(0, 16, "安全:", &font16x16, OLED_COLOR_NORMAL);
      snprintf(num_buf, sizeof(num_buf), "%u", (unsigned)g_safe_distance_max);
      OLED_PrintASCIIString(80, 16, num_buf, &afont16x8, OLED_COLOR_NORMAL);
      OLED_PrintString(0, 32, "注意:", &font16x16, OLED_COLOR_NORMAL);
      snprintf(num_buf, sizeof(num_buf), "%u", (unsigned)g_yellow_point);
      OLED_PrintASCIIString(80, 32, num_buf, &afont16x8, OLED_COLOR_NORMAL);
      OLED_PrintString(0, 48, "警告:", &font16x16, OLED_COLOR_NORMAL);
      snprintf(num_buf, sizeof(num_buf), "%u", (unsigned)g_orange_point);
      OLED_PrintASCIIString(80, 48, num_buf, &afont16x8, OLED_COLOR_NORMAL);
      //OLED_PrintASCIIString(80,0,g_num_leds, &font16x16, OLED_COLOR_NORMAL);
      //OLED_DrawImage((128 - (bilibiliImg.w)) / 2, 0, &bilibiliImg, OLED_COLOR_NORMAL);

      OLED_ShowFrame();
      // HAL_Delay(100);
    //ws2812_Update0();
    OLED_NewFrame();
    ws2812_Show();
    HAL_Delay(10);


   //ws2812_Update0();
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
  if (huart->Instance == USART1)
  {
    if (Size > 0) {
      UART1_Cmd_OnRxData(UART1_Cmd_GetRxBuffer(), Size);
    }
    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, UART1_Cmd_GetRxBuffer(), UART1_Cmd_GetRxBufferSize());
    if (huart1.hdmarx != NULL) {
      __HAL_DMA_DISABLE_IT(huart1.hdmarx, DMA_IT_HT);
    }
  }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1)
  {
    UART1_Cmd_OnTxDone();
  }
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1)
    {
    }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
