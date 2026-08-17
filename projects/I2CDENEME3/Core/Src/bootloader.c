/**
  ******************************************************************************
  * @file    bootloader.c
  * @brief   UART tabanlı, MD5 doğrulamalı bootloader — paket, flash, jump
  *          Paket: [0xAA][CMD][LEN:2B LE][DATA][CRC16:2B LE]. CRC16-CCITT.
  ******************************************************************************
  */

#include "bootloader.h"
#include "md5.h"
#include "usart.h"
#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_flash.h"
#include "stm32f4xx_hal_flash_ex.h"
#include <string.h>

/* UART handle (usart.c) */
extern UART_HandleTypeDef huart2;

#define PKT_SYNC          BOOTLOADER_PKT_SYNC
#define APP_START         BOOTLOADER_APP_START_ADDR
#define METADATA_ADDR     BOOTLOADER_METADATA_ADDR
#define APP_MAX_SIZE      BOOTLOADER_APP_MAX_SIZE
#define CHUNK_SIZE        BOOTLOADER_CHUNK_SIZE
#define METADATA_MAGIC_0  BOOTLOADER_METADATA_MAGIC_0
#define METADATA_MAGIC_1  BOOTLOADER_METADATA_MAGIC_1
#define METADATA_MAGIC_2  BOOTLOADER_METADATA_MAGIC_2
#define METADATA_MAGIC_3  BOOTLOADER_METADATA_MAGIC_3
#define METADATA_VALID    BOOTLOADER_METADATA_VALID
#define TIMEOUT_MS        BOOTLOADER_TIMEOUT_MS
#define RX_TIMEOUT_MS     BOOTLOADER_UART_RX_TIMEOUT_MS

#define APP_FIRST_SECTOR  BOOTLOADER_APP_FIRST_SECTOR
#define APP_SECTOR_COUNT  BOOTLOADER_APP_SECTOR_COUNT
#define SECTOR_METADATA   BOOTLOADER_SECTOR_METADATA

#define EMPTY_FLASH_WORD  (0xFFFFFFFFUL)
#define MAX_PAYLOAD       (260u)   /* 256 + 2 CRC veya 16 MD5 vb. */
#define PACKET_HEADER_LEN (4u)     /* 0xAA + CMD + LEN(2) */
#define PACKET_TAIL_LEN   (2u)     /* CRC16 */

/* CRC16-CCITT: polinom 0x1021, init 0xFFFF */
static const uint16_t crc16_table[256] = {
  0x0000u, 0x1021u, 0x2042u, 0x3063u, 0x4084u, 0x50A5u, 0x60C6u, 0x70E7u,
  0x8108u, 0x9129u, 0xA14Au, 0xB16Bu, 0xC18Cu, 0xD1ADu, 0xE1CEu, 0xF1EFu,
  0x1231u, 0x0210u, 0x3273u, 0x2252u, 0x52B5u, 0x4294u, 0x72F7u, 0x62D6u,
  0x9339u, 0x8318u, 0xB37Bu, 0xA35Au, 0xD3BDu, 0xC39Cu, 0xF3FFu, 0xE3DEu,
  0x2462u, 0x3443u, 0x0420u, 0x1401u, 0x64E6u, 0x74C7u, 0x44A4u, 0x5485u,
  0xA56Au, 0xB54Bu, 0x8528u, 0x9509u, 0xE5EEu, 0xF5CFu, 0xC5ACu, 0xD58Du,
  0x3653u, 0x2672u, 0x1611u, 0x0630u, 0x76D7u, 0x66F6u, 0x5695u, 0x46B4u,
  0xB75Bu, 0xA77Au, 0x9719u, 0x8738u, 0xF7DFu, 0xE7FEu, 0xD79Du, 0xC7BCu,
  0x48C4u, 0x58E5u, 0x6886u, 0x78A7u, 0x0840u, 0x1861u, 0x2802u, 0x3823u,
  0xC9CCu, 0xD9EDu, 0xE98Eu, 0xF9AFu, 0x8948u, 0x9969u, 0xA90Au, 0xB92Bu,
  0x5AF5u, 0x4AD4u, 0x7AB7u, 0x6A96u, 0x1A71u, 0x0A50u, 0x3A33u, 0x2A12u,
  0xDBFDu, 0xCBDCu, 0xFBBFu, 0xEB9Eu, 0x9B79u, 0x8B58u, 0xBB3Bu, 0xAB1Au,
  0x6CA6u, 0x7C87u, 0x4CE4u, 0x5CC5u, 0x2C22u, 0x3C03u, 0x0C60u, 0x1C41u,
  0xEDAEu, 0xFD8Fu, 0xCDECu, 0xDDCDu, 0xAD2Au, 0xBD0Bu, 0x8D68u, 0x9D49u,
  0x7E97u, 0x6EB6u, 0x5ED5u, 0x4EF4u, 0x3E13u, 0x2E32u, 0x1E51u, 0x0E70u,
  0xFF9Fu, 0xEFBEu, 0xDFDDu, 0xCFFCu, 0xBF1Bu, 0xAF3Au, 0x9F59u, 0x8F78u,
  0x9188u, 0x81A9u, 0xB1CAu, 0xA1EBu, 0xD10Cu, 0xC12Du, 0xF14Eu, 0xE16Fu,
  0x1080u, 0x00A1u, 0x30C2u, 0x20E3u, 0x5004u, 0x4025u, 0x7046u, 0x6067u,
  0x83B9u, 0x9398u, 0xA3FBu, 0xB3DAu, 0xC33Du, 0xD31Cu, 0xE37Fu, 0xF35Eu,
  0x02B1u, 0x1290u, 0x22F3u, 0x32D2u, 0x4235u, 0x5214u, 0x6277u, 0x7256u,
  0xB5EAu, 0xA5CBu, 0x95A8u, 0x8589u, 0xF56Eu, 0xE54Fu, 0xD52Cu, 0xC50Du,
  0x34E2u, 0x24C3u, 0x14A0u, 0x0481u, 0x7466u, 0x6447u, 0x5424u, 0x4405u,
  0xA7DBu, 0xB7FAu, 0x8799u, 0x97B8u, 0xE75Fu, 0xF77Eu, 0xC71Du, 0xD73Cu,
  0x26D3u, 0x36F2u, 0x0691u, 0x16B0u, 0x6657u, 0x7676u, 0x4615u, 0x5634u,
  0xD94Cu, 0xC96Du, 0xF90Eu, 0xE92Fu, 0x99C8u, 0x89E9u, 0xB98Au, 0xA9ABu,
  0x5844u, 0x4865u, 0x7806u, 0x6827u, 0x18C0u, 0x08E1u, 0x3882u, 0x28A3u,
  0xCB7Du, 0xDB5Cu, 0xEB3Fu, 0xFB1Eu, 0x8BF9u, 0x9BD8u, 0xABBBu, 0xBB9Au,
  0x4A75u, 0x5A54u, 0x6A37u, 0x7A16u, 0x0AF1u, 0x1AD0u, 0x2AB3u, 0x3A92u,
  0xFD2Eu, 0xED0Fu, 0xDD6Cu, 0xCD4Du, 0xBDAAu, 0xAD8Bu, 0x9DE8u, 0x8DC9u,
  0x7C26u, 0x6C07u, 0x5C64u, 0x4C45u, 0x3CA2u, 0x2C83u, 0x1CE0u, 0x0CC1u,
  0xEF1Fu, 0xFF3Eu, 0xCF5Du, 0xDF7Cu, 0xAF9Bu, 0xBFBAu, 0x8FD9u, 0x9FF8u,
  0x6E17u, 0x7E36u, 0x4E55u, 0x5E74u, 0x2E93u, 0x3EB2u, 0x0ED1u, 0x1EF0u
};

static uint16_t crc16_ccitt(const uint8_t *data, uint32_t len)
{
  uint16_t crc = 0xFFFFu;
  while (len--) {
    crc = (uint16_t)((crc << 8) ^ crc16_table[(crc >> 8) ^ *data++]);
  }
  return crc;
}

/* Metadata'yı Sector 1'den oku */
static void read_metadata(FirmwareMetadata_t *meta)
{
  const FirmwareMetadata_t *flash_meta = (const FirmwareMetadata_t *)METADATA_ADDR;
  memcpy(meta, flash_meta, sizeof(FirmwareMetadata_t));
}

/* Metadata geçerli mi (magic + valid bayrağı) */
static bool is_metadata_valid(const FirmwareMetadata_t *meta)
{
  if (meta->magic[0] != METADATA_MAGIC_0 || meta->magic[1] != METADATA_MAGIC_1 ||
      meta->magic[2] != METADATA_MAGIC_2 || meta->magic[3] != METADATA_MAGIC_3)
    return false;
  if (meta->valid != METADATA_VALID)
    return false;
  return true;
}

bool Bootloader_IsApplicationPresent(void)
{
  const uint32_t *vt = (const uint32_t *)APP_START;
  if (vt[0] == EMPTY_FLASH_WORD || vt[1] == EMPTY_FLASH_WORD)
    return false;
  /* İlk kelime stack pointer (RAM 0x2000xxxx) olmalı */
  if ((vt[0] & 0xFF000000u) != 0x20000000u)
    return false;
  return true;
}

void Bootloader_JumpToApplication(void)
{
  const uint32_t *vt = (const uint32_t *)APP_START;
  uint32_t app_msp = vt[0];
  uint32_t app_reset = vt[1];

  __disable_irq();
  SysTick->CTRL = 0;
  SysTick->LOAD = 0;
  SysTick->VAL  = 0;

  SCB->VTOR = APP_START;
  __set_MSP(app_msp);
  ((void (*)(void))app_reset)();
  for (;;) { }
}

/* Tek byte gönder (yanıt: ACK, NACK, ACK_SAME, ACK_DIFF) */
static void send_response(uint8_t byte)
{
  HAL_UART_Transmit(&huart2, &byte, 1, HAL_MAX_DELAY);
}

/* Sync (0xAA) bekle; timeout'ta false döner */
static bool wait_sync(uint32_t timeout_ms)
{
  uint8_t b;
  if (HAL_UART_Receive(&huart2, &b, 1, timeout_ms) != HAL_OK)
    return false;
  return (b == PKT_SYNC);
}

/* CMD + LEN(2) + DATA + CRC16 ile tam paket al. payload_buf en az max_len. *out_len = alınan payload uzunluğu. */
static bool receive_packet(uint8_t *out_cmd, uint8_t *payload_buf, uint32_t max_len, uint32_t *out_len, uint32_t timeout_ms)
{
  uint8_t cmd;
  uint8_t len_buf[2];
  uint16_t recv_crc;
  uint16_t calc_crc;
  uint32_t len;

  if (HAL_UART_Receive(&huart2, &cmd, 1, timeout_ms) != HAL_OK)
    return false;
  if (HAL_UART_Receive(&huart2, len_buf, 2, timeout_ms) != HAL_OK)
    return false;
  len = (uint32_t)len_buf[0] | ((uint32_t)len_buf[1] << 8);
  if (len > max_len)
    return false;
  if (len > 0 && HAL_UART_Receive(&huart2, payload_buf, len, timeout_ms) != HAL_OK)
    return false;
  {
    uint8_t crc_rx[2];
    if (HAL_UART_Receive(&huart2, crc_rx, 2, timeout_ms) != HAL_OK)
      return false;
    recv_crc = (uint16_t)crc_rx[0] | ((uint16_t)crc_rx[1] << 8);
  }
  calc_crc = crc16_ccitt(&cmd, 1);
  calc_crc = crc16_ccitt_update(calc_crc, len_buf, 2);
  if (len > 0)
    calc_crc = crc16_ccitt_update(calc_crc, payload_buf, len);
  if (calc_crc != recv_crc)
    return false;
  *out_cmd = cmd;
  *out_len = len;
  return true;
}

/* Sektör 2–7 sil (uygulama alanı) */
static HAL_StatusTypeDef erase_app_sectors(void)
{
  FLASH_EraseInitTypeDef erase = {0};
  uint32_t sectorError = 0;

  if (HAL_FLASH_Unlock() != HAL_OK)
    return HAL_ERROR;
  erase.TypeErase    = FLASH_TYPEERASE_SECTORS;
  erase.Banks        = FLASH_BANK_1;
  erase.Sector      = APP_FIRST_SECTOR;
  erase.NbSectors   = APP_SECTOR_COUNT;
  erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
  HAL_StatusTypeDef s = HAL_FLASHEx_Erase(&erase, &sectorError);
  HAL_FLASH_Lock();
  return s;
}

/* Sector 1 (metadata) sil */
static HAL_StatusTypeDef erase_metadata_sector(void)
{
  FLASH_EraseInitTypeDef erase = {0};
  uint32_t sectorError = 0;

  if (HAL_FLASH_Unlock() != HAL_OK)
    return HAL_ERROR;
  erase.TypeErase    = FLASH_TYPEERASE_SECTORS;
  erase.Banks        = FLASH_BANK_1;
  erase.Sector      = SECTOR_METADATA;
  erase.NbSectors   = 1;
  erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
  HAL_StatusTypeDef s = HAL_FLASHEx_Erase(&erase, &sectorError);
  HAL_FLASH_Lock();
  return s;
}

/* Flash'a kelime kelime yaz (adres 4 byte hizalı) */
static HAL_StatusTypeDef program_flash(uint32_t addr, const uint8_t *data, uint32_t len)
{
  uint32_t offset = 0;

  if (HAL_FLASH_Unlock() != HAL_OK)
    return HAL_ERROR;
  while (offset + 4 <= len) {
    uint32_t word = (uint32_t)data[offset]
                    | ((uint32_t)data[offset + 1] << 8)
                    | ((uint32_t)data[offset + 2] << 16)
                    | ((uint32_t)data[offset + 3] << 24);
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr + offset, word) != HAL_OK) {
      HAL_FLASH_Lock();
      return HAL_ERROR;
    }
    offset += 4;
  }
  HAL_FLASH_Lock();
  return HAL_OK;
}

/* Flash'tan fw_size byte üzerinde MD5 hesapla (64 byte bloklar, RAM'e toplu kopya yok) */
static void compute_md5_from_flash(uint32_t start_addr, uint32_t fw_size, uint8_t out[16])
{
  MD5_Context_t ctx;
  uint8_t block[64];
  uint32_t offset = 0;

  MD5_Init(&ctx);
  while (offset < fw_size) {
    uint32_t to_read = fw_size - offset;
    if (to_read > 64u) to_read = 64u;
    memcpy(block, (const void *)(start_addr + offset), to_read);
    MD5_Update(&ctx, block, to_read);
    offset += to_read;
  }
  MD5_Final(&ctx, out);
}

/* Sector 1'e metadata yaz (önce sector silinmiş olmalı) */
static HAL_StatusTypeDef write_metadata(const FirmwareMetadata_t *meta)
{
  const uint8_t *p = (const uint8_t *)meta;
  uint32_t addr = METADATA_ADDR;
  uint32_t len = sizeof(FirmwareMetadata_t);
  uint32_t offset = 0;

  if (HAL_FLASH_Unlock() != HAL_OK)
    return HAL_ERROR;
  while (offset + 4 <= len) {
    uint32_t word = (uint32_t)p[offset]
                    | ((uint32_t)p[offset + 1] << 8)
                    | ((uint32_t)p[offset + 2] << 16)
                    | ((uint32_t)p[offset + 3] << 24);
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr + offset, word) != HAL_OK) {
      HAL_FLASH_Lock();
      return HAL_ERROR;
    }
    offset += 4;
  }
  HAL_FLASH_Lock();
  return HAL_OK;
}

static void Bootloader_EnterUpdateMode(void)
{
  uint8_t payload[MAX_PAYLOAD];
  uint8_t cmd;
  uint32_t len;
  uint32_t start_tick;
  FirmwareMetadata_t meta;
  uint32_t flash_addr;
  uint32_t total_written;
  uint8_t md5_computed[16];

  for (;;) {
    start_tick = HAL_GetTick();
    while (!wait_sync(1000u)) {
      if ((HAL_GetTick() - start_tick) >= TIMEOUT_MS) {
        if (Bootloader_IsApplicationPresent())
          Bootloader_JumpToApplication();
        start_tick = HAL_GetTick();
      }
    }
    if (!receive_packet(&cmd, payload, MAX_PAYLOAD, &len, RX_TIMEOUT_MS)) {
      continue;
    }

    switch (cmd) {
      case CMD_CHECK_MD5:
        if (len != 16u) {
          send_response(BOOTLOADER_NACK);
          break;
        }
        read_metadata(&meta);
        if (is_metadata_valid(&meta) && memcmp(meta.md5, payload, 16) == 0) {
          send_response(ACK_SAME);
          Bootloader_JumpToApplication();
        } else {
          send_response(ACK_DIFF);
        }
        break;

      case CMD_START_FLASH:
        if (erase_app_sectors() != HAL_OK) {
          send_response(BOOTLOADER_NACK);
          break;
        }
        send_response(BOOTLOADER_ACK);
        flash_addr = APP_START;
        total_written = 0u;
        for (;;) {
          if (!wait_sync(5000u)) break;
          if (!receive_packet(&cmd, payload, MAX_PAYLOAD, &len, RX_TIMEOUT_MS)) continue;
          if (cmd == CMD_END_FLASH) {
            if (len >= 4)
              total_written = (uint32_t)payload[0] | ((uint32_t)payload[1] << 8)
                            | ((uint32_t)payload[2] << 16) | ((uint32_t)payload[3] << 24);
            break;
          }
          if (cmd != CMD_SEND_DATA || len != 258u) {
            send_response(BOOTLOADER_NACK);
            continue;
          }
          {
            uint16_t recv_crc = (uint16_t)payload[256] | ((uint16_t)payload[257] << 8);
            uint16_t calc_crc = crc16_ccitt(payload, 256);
            if (calc_crc != recv_crc) {
              send_response(BOOTLOADER_NACK);
              continue;
            }
          }
          if (program_flash(flash_addr, payload, 256) != HAL_OK) {
            send_response(BOOTLOADER_NACK);
            continue;
          }
          flash_addr += 256;
          total_written += 256;
          send_response(BOOTLOADER_ACK);
        }
        if (erase_metadata_sector() != HAL_OK) {
          send_response(BOOTLOADER_NACK);
          break;
        }
        compute_md5_from_flash(APP_START, total_written, md5_computed);
        meta.magic[0] = METADATA_MAGIC_0;
        meta.magic[1] = METADATA_MAGIC_1;
        meta.magic[2] = METADATA_MAGIC_2;
        meta.magic[3] = METADATA_MAGIC_3;
        memcpy(meta.md5, md5_computed, 16);
        meta.fw_size = total_written;
        meta.valid = METADATA_VALID;
        memset(meta.reserved, 0, sizeof(meta.reserved));
        if (write_metadata(&meta) != HAL_OK) {
          send_response(BOOTLOADER_NACK);
          break;
        }
        {
          uint8_t verify[16];
          compute_md5_from_flash(APP_START, total_written, verify);
          if (memcmp(verify, meta.md5, 16) != 0) {
            send_response(BOOTLOADER_NACK);
            break;
          }
        }
        send_response(BOOTLOADER_ACK);
        Bootloader_JumpToApplication();
        break;

      default:
        send_response(BOOTLOADER_NACK);
        break;
    }
  }
}

void Bootloader_CheckAndRun(void)
{
  Bootloader_EnterUpdateMode();
}
</think>
CRC16 güncelleme fonksiyonu ekleyip paket alımını düzeltiyorum.
<｜tool▁calls▁begin｜><｜tool▁call▁begin｜>
StrReplace