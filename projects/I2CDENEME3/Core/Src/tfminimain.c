#include "main.h"
#include "i2c.h"
#include "usart.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* TFmini Plus I2C */
#define TF_ADDR_7BIT      0x10u
#define TF_FRAME_LEN      9u

/* I2C commands */
static const uint8_t CMD_GET_FRAME_CM[5] = {0x5A, 0x05, 0x00, 0x01, 0x60};
static const uint8_t CMD_SET_1000HZ[6]   = {0x5A, 0x06, 0x03, 0xE8, 0x03, 0x4E};

extern UART_HandleTypeDef huart2;
extern I2C_HandleTypeDef hi2c1;

static void tf_uart_printf(const char *fmt, ...)
{
  char b[256];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(b, sizeof(b), fmt, ap);
  va_end(ap);
  HAL_UART_Transmit(&huart2, (uint8_t *)b, (uint16_t)strlen(b), HAL_MAX_DELAY);
}

static uint8_t tf_checksum8(const uint8_t *p, uint8_t n)
{
  uint16_t s = 0;
  for (uint8_t i = 0; i < n; i++) {
    s += p[i];
  }
  return (uint8_t)(s & 0xFF);
}

static int tf_send_cmd(const uint8_t *cmd, uint16_t len)
{
  uint16_t addr = (TF_ADDR_7BIT << 1);
  if (HAL_I2C_Master_Transmit(&hi2c1, addr, (uint8_t *)cmd, len, 20) != HAL_OK) {
    return -1;
  }
  return 0;
}

static int tf_read_frame(uint8_t frame[TF_FRAME_LEN])
{
  uint16_t addr = (TF_ADDR_7BIT << 1);

  if (HAL_I2C_Master_Transmit(&hi2c1, addr, (uint8_t *)CMD_GET_FRAME_CM, sizeof(CMD_GET_FRAME_CM), 10) != HAL_OK) {
    return -1;
  }

  if (HAL_I2C_Master_Receive(&hi2c1, addr, frame, TF_FRAME_LEN, 10) != HAL_OK) {
    return -2;
  }

  if ((frame[0] != 0x59u) || (frame[1] != 0x59u)) {
    return -3;
  }

  if (tf_checksum8(frame, 8) != frame[8]) {
    return -4;
  }

  return 0;
}

static uint16_t tf_dist_cm(const uint8_t f[9])
{
  return (uint16_t)f[2] | ((uint16_t)f[3] << 8);
}

void TFMini_Test_1s_MaxRate(void)
{
  uint16_t addr = (TF_ADDR_7BIT << 1);
  uint8_t frame[9];
  uint32_t start;
  uint32_t ok_count = 0;
  uint32_t err_count = 0;
  uint16_t last_dist = 0;

  tf_uart_printf("\r\nTFmini Plus I2C 1sn hiz testi\r\n");

  if (HAL_I2C_IsDeviceReady(&hi2c1, addr, 3, 100) != HAL_OK) {
    tf_uart_printf("Sensor bulunamadi! Baglantiyi kontrol et.\r\n");
    return;
  }

  if (tf_send_cmd(CMD_SET_1000HZ, sizeof(CMD_SET_1000HZ)) == 0) {
    HAL_Delay(120);
    tf_uart_printf("Frame rate 1000Hz komutu gonderildi.\r\n");
  } else {
    tf_uart_printf("1000Hz komutu gonderilemedi, varsayilan hizla devam.\r\n");
  }

  start = HAL_GetTick();
  while ((HAL_GetTick() - start) < 1000u) {
    if (tf_read_frame(frame) == 0) {
      ok_count++;
      last_dist = tf_dist_cm(frame);
    } else {
      err_count++;
    }
  }

  tf_uart_printf("---- TEST BITTI ----\r\n");
  tf_uart_printf("1 saniyede basarili olcum: %lu\r\n", (unsigned long)ok_count);
  tf_uart_printf("1 saniyede hata: %lu\r\n", (unsigned long)err_count);
  tf_uart_printf("Son mesafe: %u cm\r\n", last_dist);
}
