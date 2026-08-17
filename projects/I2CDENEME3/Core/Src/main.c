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
#include "i2c.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "tf02_continuous_fast.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define TFMINI_ADDR_7BIT          0x10u
#define TFMINI_FRAME_LEN          9u
#define TFMINI_I2C_TIMEOUT_MS     5u
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
static void UART_Printf(const char *fmt, ...);
static uint8_t TF_Checksum8(const uint8_t *data, uint8_t len);
static int TF_ReadFrameCm(uint8_t frame[TFMINI_FRAME_LEN]);
static void Run_TFMini_I2C_MaxRate_Test_1s(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static const uint8_t TF_CMD_SET_1000HZ[6] = {0x5A, 0x06, 0x03, 0xE8, 0x03, 0x4E};
static const uint8_t TF_CMD_GET_FRAME_CM[5] = {0x5A, 0x05, 0x00, 0x01, 0x60};

static void UART_Printf(const char *fmt, ...)
{
  char buffer[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);
  HAL_UART_Transmit(&huart2, (uint8_t *)buffer, (uint16_t)strlen(buffer), HAL_MAX_DELAY);
}

static uint8_t TF_Checksum8(const uint8_t *data, uint8_t len)
{
  uint16_t sum = 0;
  for (uint8_t i = 0; i < len; i++) {
    sum += data[i];
  }
  return (uint8_t)(sum & 0xFFu);
}

static int TF_ReadFrameCm(uint8_t frame[TFMINI_FRAME_LEN])
{
  const uint16_t addr = (TFMINI_ADDR_7BIT << 1);

  if (HAL_I2C_Master_Transmit(&hi2c1, addr, (uint8_t *)TF_CMD_GET_FRAME_CM, sizeof(TF_CMD_GET_FRAME_CM), TFMINI_I2C_TIMEOUT_MS) != HAL_OK) {
    return -1;
  }

  if (HAL_I2C_Master_Receive(&hi2c1, addr, frame, TFMINI_FRAME_LEN, TFMINI_I2C_TIMEOUT_MS) != HAL_OK) {
    return -2;
  }

  if ((frame[0] != 0x59u) || (frame[1] != 0x59u)) {
    return -3;
  }

  if (TF_Checksum8(frame, 8) != frame[8]) {
    return -4;
  }

  return 0;
}

static void Run_TFMini_I2C_MaxRate_Test_1s(void)
{
  const uint16_t addr = (TFMINI_ADDR_7BIT << 1);
  uint8_t frame[TFMINI_FRAME_LEN];
  uint32_t t0 = 0;
  uint32_t ok_count = 0;
  uint32_t err_count = 0;
  uint16_t last_distance_cm = 0;
  uint16_t min_distance_cm = 0xFFFFu;
  uint16_t max_distance_cm = 0;
  uint16_t last_strength = 0;
  uint16_t min_strength = 0xFFFFu;
  uint16_t max_strength = 0;

  UART_Printf("\r\n=== TFmini Plus I2C hiz testi (1 saniye) ===\r\n");

  if (HAL_I2C_IsDeviceReady(&hi2c1, addr, 3, 100) != HAL_OK) {
    UART_Printf("Sensor bulunamadi (0x10). Kablolamayi kontrol et.\r\n");
    return;
  }

  if (HAL_I2C_Master_Transmit(&hi2c1, addr, (uint8_t *)TF_CMD_SET_1000HZ, sizeof(TF_CMD_SET_1000HZ), 50) == HAL_OK) {
    HAL_Delay(120);
    UART_Printf("Frame rate 1000Hz komutu gonderildi.\r\n");
  } else {
    UART_Printf("1000Hz komutu gonderilemedi, mevcut ayarla devam.\r\n");
  }

  t0 = HAL_GetTick();
  while ((HAL_GetTick() - t0) < 1000u) {
    if (TF_ReadFrameCm(frame) == 0) {
      uint16_t distance = (uint16_t)frame[2] | ((uint16_t)frame[3] << 8);
      uint16_t strength = (uint16_t)frame[4] | ((uint16_t)frame[5] << 8);
      ok_count++;
      last_distance_cm = distance;
      last_strength = strength;
      if (distance < min_distance_cm) {
        min_distance_cm = distance;
      }
      if (distance > max_distance_cm) {
        max_distance_cm = distance;
      }
      if (strength < min_strength) {
        min_strength = strength;
      }
      if (strength > max_strength) {
        max_strength = strength;
      }
    } else {
      err_count++;
    }
  }

  UART_Printf("=== TEST BITTI ===\r\n");
  UART_Printf("1 saniyede basarili olcum: %lu\r\n", (unsigned long)ok_count);
  UART_Printf("1 saniyede hata: %lu\r\n", (unsigned long)err_count);
  UART_Printf("Son mesafe: %u cm\r\n", last_distance_cm);
  if (ok_count > 0u) {
    UART_Printf("Min/Max mesafe: %u / %u cm\r\n", min_distance_cm, max_distance_cm);
    UART_Printf("Sinyal gucu (Strength): min=%u max=%u son=%u\r\n", min_strength, max_strength, last_strength);
    if (last_distance_cm == 0u && max_strength < 100u) {
      UART_Printf("(Strength<100 -> sensor mesafe=0 yapar. Hedefi 20cm-2m arasi, beyaz yuzey dene.)\r\n");
    } else if (last_distance_cm == 0u && min_strength == 65535u) {
      UART_Printf("(Asiri aydinlik/overexposed -> mesafe 0. Irigi azalt.)\r\n");
    }
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
  MX_USART2_UART_Init();

  TF02_I2C1_BusRecover();  /* I2C oncesi bus recovery */
  MX_I2C1_Init();

  /* USER CODE BEGIN 2 */
  TF02_Run_Continuous_Fast();  /* sonsuz dongu burada, geri donmez */
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitStruct structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 180;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
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
