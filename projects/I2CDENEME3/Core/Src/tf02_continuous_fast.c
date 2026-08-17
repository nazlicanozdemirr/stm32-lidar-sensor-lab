

#include "main.h"
#include "tf02_continuous_fast.h"
#include "i2c.h"
#include "usart.h"
#include "gpio.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>


#define TFMINI_ADDR_7BIT   0x10u
#define TFMINI_ADDR        (TFMINI_ADDR_7BIT << 1)

#define TF_WAIT_CFG_MS     100u  
#define TF_GETCM_WAIT_MS   0u     

#define PRINT_DELTA_CM     2u     
#define MED_WIN            5u     

extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart2;

/* --------------------------------------------------------------------------- */
static void LOG(const char *fmt, ...)
{
  char buf[256];
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  if (n <= 0) return;
  if (n > (int)sizeof(buf)) n = (int)sizeof(buf);
  HAL_UART_Transmit(&huart2, (uint8_t*)buf, (uint16_t)n, HAL_MAX_DELAY);
}

/* --------------------------------------------------------------------------- */
void TF02_I2C1_BusRecover(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  __HAL_RCC_GPIOB_CLK_ENABLE();

  GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8 | GPIO_PIN_9, GPIO_PIN_SET);
  HAL_Delay(2);

  for (int i = 0; i < 9; i++) {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
    HAL_Delay(1);
  }

  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET);
  HAL_Delay(1);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
  HAL_Delay(1);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);
  HAL_Delay(1);
}

/* --------------------------------------------------------------------------- */
static int TFmini_WaitFound(uint32_t timeout_ms)
{
  uint32_t t0 = HAL_GetTick();
  while ((HAL_GetTick() - t0) < timeout_ms) {
    if (HAL_I2C_IsDeviceReady(&hi2c1, TFMINI_ADDR, 2, 10) == HAL_OK)
      return 1;
    HAL_Delay(20);
  }
  return 0;
}

/* --------------------------------------------------------------------------- */
static uint8_t tf_checksum(const uint8_t *b, uint8_t n_without_checksum)
{
  uint16_t s = 0;
  for (uint8_t i = 0; i < n_without_checksum; i++) s += b[i];
  return (uint8_t)(s & 0xFFu);
}

/* --------------------------------------------------------------------------- */
static int tf_send_cmd_get_resp5(uint8_t *cmd, uint8_t len)
{
  cmd[len - 1] = tf_checksum(cmd, len - 1);

  if (HAL_I2C_Master_Transmit(&hi2c1, TFMINI_ADDR, cmd, len, 50) != HAL_OK)
    return 0;

  HAL_Delay(TF_WAIT_CFG_MS);

  uint8_t resp[5] = {0};
  if (HAL_I2C_Master_Receive(&hi2c1, TFMINI_ADDR, resp, 5, 50) == HAL_OK) {
    LOG("RESP: %02X %02X %02X %02X %02X\r\n", resp[0], resp[1], resp[2], resp[3], resp[4]);
  } else {
    LOG("RESP: (no bytes)\r\n");
  }
  return 1;
}

/* --------------------------------------------------------------------------- */
static int TFmini_OutputEnable_I2C(void)
{
  uint8_t cmd[5] = {0x5A, 0x05, 0x07, 0x01, 0x00};
  LOG("CMD: OUTPUT ENABLE\r\n");
  return tf_send_cmd_get_resp5(cmd, sizeof(cmd));
}

/* --------------------------------------------------------------------------- */
static int TFmini_ChecksumOK9(const uint8_t f[9])
{
  uint16_t s = 0;
  for (int i = 0; i < 8; i++) s += f[i];
  return ((uint8_t)(s & 0xFFu) == f[8]);
}

/* --------------------------------------------------------------------------- */
static int TFmini_ReadFrame_CM(uint8_t f[9])
{
  uint8_t cmd[5] = {0x5A, 0x05, 0x00, 0x01, 0x00};
  cmd[4] = tf_checksum(cmd, 4);

  if (HAL_I2C_Master_Transmit(&hi2c1, TFMINI_ADDR, cmd, 5, 50) != HAL_OK)
    return 0;

  HAL_Delay(TF_GETCM_WAIT_MS);  

  if (HAL_I2C_Master_Receive(&hi2c1, TFMINI_ADDR, f, 9, 50) != HAL_OK)
    return 0;

  if (f[0] != 0x59 || f[1] != 0x59) return 0;
  if (!TFmini_ChecksumOK9(f))    return 0;

  return 1;
}

/* --------------------------------------------------------------------------- */
static void sort2_u16(uint16_t *x, uint16_t *y)
{
  if (*x > *y) { uint16_t t = *x; *x = *y; *y = t; }
}

static uint16_t median5_u16(uint16_t a, uint16_t b, uint16_t c, uint16_t d, uint16_t e)
{
  sort2_u16(&a, &b);
  sort2_u16(&d, &e);
  sort2_u16(&c, &e);
  sort2_u16(&c, &d);
  sort2_u16(&a, &d);
  sort2_u16(&a, &c);
  sort2_u16(&b, &e);
  sort2_u16(&b, &d);
  sort2_u16(&b, &c);
  return c;
}

/* --------------------------------------------------------------------------- */
void TF02_Run_Continuous_Fast(void)
{
  LOG("\r\nTF02 start\r\n");  /* ilk satir: seri baglanti kontrolu */
  LOG("\r\n--- TF02-Pro I2C Continuous (FAST: GET_CM wait %u ms) ---\r\n", (unsigned)TF_GETCM_WAIT_MS);
  LOG("Mode: GET_CM + Median(%u) + Print on change >= %u cm\r\n", (unsigned)MED_WIN, (unsigned)PRINT_DELTA_CM);

  HAL_Delay(400);

  if (!TFmini_WaitFound(2000)) {
    LOG("TF02-Pro NOT FOUND at 0x10 (check pull-up/wiring/mode)\r\n");
    return;
  }
  LOG("FOUND I2C device at 0x10\r\n");

  TFmini_OutputEnable_I2C();
  LOG("\r\nStreaming...\r\n");

  uint32_t t_stat0 = HAL_GetTick();
  volatile uint32_t ok_cnt = 0, err_cnt = 0;
  volatile uint32_t change_cnt = 0;

  volatile uint16_t last_raw_dist = 0, last_raw_str = 0;
  volatile int16_t  last_raw_temp = 0;
  volatile uint16_t last_med_dist = 0;
  uint16_t last_printed_med = 0;
  uint8_t  have_printed = 0;

  uint16_t win[MED_WIN] = {0};
  uint8_t  win_i = 0;
  uint8_t  win_fill = 0;

  uint8_t f[9];

  for (;;) {
    if (TFmini_ReadFrame_CM(f)) {
      uint16_t dist = (uint16_t)f[2] | ((uint16_t)f[3] << 8);
      uint16_t str  = (uint16_t)f[4] | ((uint16_t)f[5] << 8);
      uint16_t tr   = (uint16_t)f[6] | ((uint16_t)f[7] << 8);
      int16_t  temp = (int16_t)((tr / 8) - 256);

      ok_cnt++;
      last_raw_dist = dist;
      last_raw_str  = str;
      last_raw_temp = temp;

      if (str > 0) {
        win[win_i] = dist;
        win_i = (uint8_t)((win_i + 1u) % MED_WIN);
        if (win_fill < MED_WIN) win_fill++;

        if (win_fill == MED_WIN) {
          uint16_t med = median5_u16(win[0], win[1], win[2], win[3], win[4]);
          last_med_dist = med;

          uint16_t diff = (med > last_printed_med) ? (med - last_printed_med) : (last_printed_med - med);

          if (!have_printed || diff >= PRINT_DELTA_CM) {
            have_printed = 1;
            last_printed_med = med;
            change_cnt++;

            LOG("RAW D=%u cm  MED=%u cm  S=%u  T=%d C\r\n",
                (unsigned)dist, (unsigned)med, (unsigned)str, (int)temp);
          }
        }
      } else {
        
        LOG("WARN: S=0 (invalid) D=%u\r\n", (unsigned)dist);
      }
    } else {
      err_cnt++;
      
      LOG("ERR: frame #%lu\r\n", (unsigned long)err_cnt);
    }

    uint32_t now = HAL_GetTick();
    if ((now - t_stat0) >= 1000u) {
      uint32_t elapsed = now - t_stat0;
      float hz = (elapsed > 0) ? (1000.0f * (float)ok_cnt / (float)elapsed) : 0.0f;

      LOG("\r\n[1s] OK=%lu ERR=%lu Rate=%.1f Hz  Changes=%lu  LastRAW=%u  LastMED=%u  S=%u T=%dC\r\n\r\n",
          (unsigned long)ok_cnt, (unsigned long)err_cnt, hz,
          (unsigned long)change_cnt,
          (unsigned)last_raw_dist, (unsigned)last_med_dist,
          (unsigned)last_raw_str, (int)last_raw_temp);

      t_stat0 = now;
      ok_cnt = 0;
      err_cnt = 0;
      change_cnt = 0;
    }
  }
}
