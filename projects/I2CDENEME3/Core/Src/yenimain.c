/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : yenimain.c
  * @brief          : TFmini Plus I2C 1-second speed test example
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
/* 1 yap: UART ile sensoru I2C moda al ve dur. 0 yap: normal I2C hiz testi */
#define TFMINI_UART_TO_I2C_SETUP_ONLY  0u
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
static uint8_t I2C_ScanAndPickAddress(void);
static void Run_TFMini_SwitchToI2C_OverUART(void);
static void Run_TFMini_I2C_MaxRate_Test_1s(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static const uint8_t TF_CMD_SET_1000HZ[6] = {0x5A, 0x06, 0x03, 0xE8, 0x03, 0x4E};
static const uint8_t TF_CMD_GET_FRAME_CM[5] = {0x5A, 0x05, 0x00, 0x01, 0x60};
static const uint8_t TF_CMD_SET_I2C_MODE[5] = {0x5A, 0x05, 0x0A, 0x01, 0x6A};
static const uint8_t TF_CMD_SAVE_SETTINGS[4] = {0x5A, 0x04, 0x11, 0x6F};
static uint8_t g_tf_addr_7bit = TFMINI_ADDR_7BIT;

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
  const uint16_t addr = (g_tf_addr_7bit << 1);

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

static uint8_t I2C_ScanAndPickAddress(void)
{
  uint8_t found_addr = 0;
  uint8_t found_count = 0;

  UART_Printf("I2C tarama basliyor...\r\n");
  for (uint8_t a = 1; a < 0x7Fu; a++) {
    if (HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(a << 1), 2, 3) == HAL_OK) {
      UART_Printf("I2C cihaz bulundu: 0x%02X\r\n", a);
      if (found_count == 0) {
        found_addr = a;
      }
      found_count++;
    }
  }

  if (found_count == 0) {
    UART_Printf("I2C'de hic cihaz yok.\r\n");
    return 0;
  }

  if (found_count > 1) {
    UART_Printf("Birden fazla I2C cihaz var, ilk adres secildi: 0x%02X\r\n", found_addr);
  }

  return found_addr;
}

static void Run_TFMini_SwitchToI2C_OverUART(void)
{
  /* Once sensor RX is connected to PA2, debug prints can corrupt command stream.
     So send raw commands first, then print status. */
  HAL_UART_Transmit(&huart2, (uint8_t *)TF_CMD_SET_I2C_MODE, sizeof(TF_CMD_SET_I2C_MODE), 100);
  HAL_Delay(150);
  HAL_UART_Transmit(&huart2, (uint8_t *)TF_CMD_SAVE_SETTINGS, sizeof(TF_CMD_SAVE_SETTINGS), 100);
  HAL_Delay(1200);

  UART_Printf("\r\n=== TFmini UART->I2C kurulum modu ===\r\n");
  UART_Printf("Komutlar gonderildi: 5A 05 0A 01 6A + 5A 04 11 6F\r\n");
  UART_Printf("Save sonrasi 1.2sn beklendi.\r\n");
  UART_Printf("Simdi sensorun gucunu kapat-ac yap.\r\n");
  UART_Printf("Sonra I2C baglantisina donup TFMINI_UART_TO_I2C_SETUP_ONLY=0 yap.\r\n");
}

static void Run_TFMini_I2C_MaxRate_Test_1s(void)
{
  uint16_t addr = (g_tf_addr_7bit << 1);
  uint8_t frame[TFMINI_FRAME_LEN];
  uint32_t t0 = 0;
  uint32_t ok_count = 0;
  uint32_t err_count = 0;
  uint16_t last_distance_cm = 0;
  uint16_t min_distance_cm = 0xFFFFu;
  uint16_t max_distance_cm = 0;

  UART_Printf("\r\n=== TFmini Plus I2C hiz testi (1 saniye) ===\r\n");

  if (HAL_I2C_IsDeviceReady(&hi2c1, addr, 3, 100) != HAL_OK) {
    UART_Printf("0x%02X adresinde cihaz yok.\r\n", g_tf_addr_7bit);
    g_tf_addr_7bit = I2C_ScanAndPickAddress();
    if (g_tf_addr_7bit == 0) {
      UART_Printf("Sensor bulunamadi. Muhtemel sebep: sensor UART modunda.\r\n");
      UART_Printf("UART'tan su komutlari gonder: 5A 05 0A 01 6A ve 5A 04 11 6F\r\n");
      return;
    }
    addr = (uint16_t)(g_tf_addr_7bit << 1);
    UART_Printf("Test adresi: 0x%02X\r\n", g_tf_addr_7bit);
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
      ok_count++;
      last_distance_cm = distance;
      if (distance < min_distance_cm) {
        min_distance_cm = distance;
      }
      if (distance > max_distance_cm) {
        max_distance_cm = distance;
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
  }
}
/* USER CODE END 0 */

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART2_UART_Init();

  if (TFMINI_UART_TO_I2C_SETUP_ONLY) {
    Run_TFMini_SwitchToI2C_OverUART();
  } else {
    Run_TFMini_I2C_MaxRate_Test_1s();
  }

  while (1)
  {
    HAL_Delay(1000);
  }
}

void SystemClock_Config(void)
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
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                              | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  (void)file;
  (void)line;
}
#endif
