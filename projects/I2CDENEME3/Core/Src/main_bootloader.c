/**
  ******************************************************************************
  * @file    main_bootloader.c
  * @brief   Sadece bootloader çalıştırır (0x08000000 build için).
  *          UART + MD5, uygulama yoksa yükleme modu.
  ******************************************************************************
  */

#include "main.h"
#include "bootloader.h"
#include "gpio.h"
#include "usart.h"

static void SystemClock_Config_Boot(void);

int main(void)
{
  HAL_Init();
  SystemClock_Config_Boot();
  MX_GPIO_Init();
  MX_USART2_UART_Init();

  Bootloader_CheckAndRun();

  return 0; /* Buraya düşmez; ya uygulamaya zıplar ya da yükleme modunda kalır */
}

static void SystemClock_Config_Boot(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

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
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    for (;;) { }
  }

  if (HAL_PWREx_EnableOverDrive() != HAL_OK) {
    for (;;) { }
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                              | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
    for (;;) { }
  }
}

/**
  * @brief  UART/GPIO vb. HAL init hata durumunda usart.c tarafından çağrılır.
  *         Bootloader build'de main.c yok; bu yüzden Error_Handler burada tanımlı.
  */
void Error_Handler(void)
{
  for (;;) { }
}
