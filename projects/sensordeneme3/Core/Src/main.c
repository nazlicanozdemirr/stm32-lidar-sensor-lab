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
#include "usart.h"
#include "gpio.h"
#include <stdio.h>
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/


/* USER CODE BEGIN PV */
#define RX_BUFFER_SIZE 9
uint8_t rxBuffer[RX_BUFFER_SIZE];
volatile uint8_t dataReceived = 0;

// Mesafe ve sıcaklık verileri
volatile uint16_t distance_cm = 0;
volatile uint16_t temperature_raw = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

/* USER CODE BEGIN PFP */
uint16_t Calculate_CRC16(uint8_t *data, uint8_t length);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint16_t Calculate_CRC16(uint8_t *data, uint8_t length)
{
    uint16_t crc = 0xFFFF;

    for (uint8_t i = 0; i < length; i++)
    {
        crc ^= (uint16_t)data[i];
        for (uint8_t j = 0; j < 8; j++)
        {
            if (crc & 0x0001)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc >>= 1;
        }
    }
    return crc;
}

// UART Receive Complete Callback
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        // LED'i toggle et (yanıp söndür)
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);

        // Veri doğrulama ve parse etme
        if (rxBuffer[0] == 0x01 && rxBuffer[1] == 0x04 && rxBuffer[2] == 0x04)
        {
            // Mesafe verisi (cm cinsinden)
            distance_cm = (rxBuffer[3] << 8) | rxBuffer[4];

            // Sıcaklık verisi (0.1°C cinsinden)
            temperature_raw = (rxBuffer[5] << 8) | rxBuffer[6];

            // CRC kontrolü (opsiyonel)
            uint16_t received_crc = (rxBuffer[8] << 8) | rxBuffer[7];
            uint16_t calculated_crc = Calculate_CRC16(rxBuffer, 7);

            if (received_crc == calculated_crc)
            {
                dataReceived = 1;
            }
        }

        // Bir sonraki veri için interrupt'ı tekrar başlat
        HAL_UART_Receive_IT(&huart1, rxBuffer, RX_BUFFER_SIZE);
    }
}
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
  MX_USART2_UART_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);

     // İlk UART receive interrupt'ını başlat
     HAL_UART_Receive_IT(&huart1, rxBuffer, RX_BUFFER_SIZE);

     HAL_Delay(500); // Sensör hazır olana kadar bekle

     // Sensörü aktif sürekli ölçüm moduna al
     // 1. Ölçüm periyodunu 50ms olarak ayarla
     uint8_t setMeasurementPeriod[] = {0x01, 0x06, 0x00, 0x42, 0x00, 0x32, 0xa8, 0x0b};
     HAL_UART_Transmit(&huart1, setMeasurementPeriod, 8, 100);
     HAL_Delay(100);

     // 2. Aktif raporlama modunu aç
     uint8_t setActiveReporting[] = {0x01, 0x06, 0x00, 0x3d, 0x00, 0x01, 0xd9, 0xc6};
     HAL_UART_Transmit(&huart1, setActiveReporting, 8, 100);
     HAL_Delay(100);

     // 3. Ölçümü başlat
     uint8_t startMeasurement[] = {0x01, 0x06, 0x00, 0x03, 0x00, 0x01, 0xb8, 0x0a};
     HAL_UART_Transmit(&huart1, startMeasurement, 8, 100);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
	  if (dataReceived)
	          {
	              dataReceived = 0;

	              // Mesafe ve sıcaklık verilerini kullan
	              float temperature_celsius = temperature_raw / 10.0f;

	              // Örnek: Mesafe 50cm'den yakınsa LED'i hızlı yanıp söndür
	              if (distance_cm < 500) // 500 = 5 metre
	              {
	                  // Yakın mesafe - hızlı yanıp sönme
	                  HAL_Delay(50);
	              }
	              else
	              {
	                  // Uzak mesafe - normal hız
	                  HAL_Delay(200);
	              }

	              // Debug için (USART2'ye yazdırabilirsin)
	              // printf("Mesafe: %d cm, Sicaklik: %.1f C\r\n", distance_cm, temperature_celsius);
	          }

	          HAL_Delay(10);
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
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

#ifdef  USE_FULL_ASSERT
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
