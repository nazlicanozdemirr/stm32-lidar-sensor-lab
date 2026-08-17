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

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

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
  MX_I2C1_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  serial_print("XT-S1 LiDAR - Serial Monitor\r\n");
  /* Sensörün I2C'de hazir olmasi icin kisa bekleme (guc acilisi) */
     HAL_Delay(100);

     HAL_Delay(100);

        /* I2C'de sensör var mi? (Bazi HAL surumleri 7-bit, bazisi 8-bit adres ister) */
        {
            const uint16_t addr7 = XT_S1_I2C_ADDR;           /* 0x20 = 7-bit */
            const uint16_t addr8 = (XT_S1_I2C_ADDR << 1);    /* 0x40 = 8-bit */
            HAL_StatusTypeDef r7 = HAL_I2C_IsDeviceReady(&hi2c1, addr7, 3, 200);
            HAL_StatusTypeDef r8 = HAL_I2C_IsDeviceReady(&hi2c1, addr8, 3, 200);
            if (r7 != HAL_OK && r8 != HAL_OK) {
                serial_print("HATA: XT-S1 I2C'de yok (0x20). Kontrol: SDA=PB9, SCL=PB8, 5V, GND, Mode=GND, CubeMX I2C1.\r\n");
            } else {
                serial_print("XT-S1 I2C'de bulundu.\r\n");
            }
        }


      uint16_t distance_mm;
      int16_t temp_x01;
      uint8_t err;
      char buf[64];

      for (;;)
          {
              HAL_Delay(20);   /* I2C bus bosalsin */
              if (!XTS1_StartMeasurement(&hi2c1)) {
                  serial_print("I2C Start error\r\n");
                  HAL_Delay(200);
                  continue;
              }

          /* 2) Sensörün ölçümü bitirmesi için bekleme (datasheet'e göre 30–50 ms) */
          HAL_Delay(40);

          /* 3) Mesafe oku */
          if (XTS1_ReadDistance(&hi2c1, &distance_mm)) {
              snprintf(buf, sizeof(buf), "Distance: %u mm\r\n", (unsigned)distance_mm);
              serial_print(buf);
          } else {
              if (XTS1_ReadError(&hi2c1, &err)) {
                  if (err & ERR_NO_OBJECT) serial_print("No object\r\n");
                  else if (err & ERR_SATURATION) serial_print("Saturation\r\n");
                  else serial_print("Read error\r\n");
              } else {
                  serial_print("Read error\r\n");
              }
          }

          /* 4) Sıcaklık (isteğe bağlı) */
          if (XTS1_ReadTemperature(&hi2c1, &temp_x01)) {
              snprintf(buf, sizeof(buf), "Temp: %d.%d C\r\n",
                       (int)(temp_x01 / 10), (int)(temp_x01 >= 0 ? (temp_x01 % 10) : (-(-temp_x01 % 10))));
              serial_print(buf);
          }

          serial_print("---\r\n");
          HAL_Delay(500);
      }
  }
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
  * in the RCC_OscInitTypeDef structure.
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
