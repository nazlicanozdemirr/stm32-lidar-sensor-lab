/**
  ******************************************************************************
  * @file    bootloader.h
  * @brief   UART tabanlı, MD5 doğrulamalı bootloader — sabitler, struct, prototipler
  *          STM32F446RE: Sector 0 = Bootloader, Sector 1 = Metadata, Sector 2-7 = Uygulama
  ******************************************************************************
  */

#ifndef __BOOTLOADER_H
#define __BOOTLOADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* ----------------------------------------------------------------------------
 * Flash bellek haritası
 * ---------------------------------------------------------------------------- */
#define BOOTLOADER_FLASH_BASE       (0x08000000UL)
#define BOOTLOADER_METADATA_ADDR   (0x08004000UL)  /* Sector 1, 16 KB */
#define BOOTLOADER_APP_START_ADDR  (0x08008000UL)  /* Sector 2 başlangıcı */
#define BOOTLOADER_APP_MAX_SIZE    (480UL * 1024UL) /* ~480 KB */

/* Sektörler: 0 = bootloader, 1 = metadata, 2..7 = uygulama */
#define BOOTLOADER_SECTOR_METADATA   (1u)
#define BOOTLOADER_APP_FIRST_SECTOR  (2u)
#define BOOTLOADER_APP_SECTOR_COUNT  (6u)

/* ----------------------------------------------------------------------------
 * Paket formatı: [0xAA][CMD:1B][LEN:2B LE][DATA:N][CRC16:2B LE]
 * CRC16-CCITT: CMD + LEN(2) + DATA üzerinden
 * ---------------------------------------------------------------------------- */
#define BOOTLOADER_PKT_SYNC         (0xAAu)

/* Komutlar (host → STM32) */
#define CMD_CHECK_MD5              (0x01u)  /* Python MD5 (16 byte) gönderir */
#define CMD_START_FLASH            (0x02u)  /* Flash başlatma sinyali */
#define CMD_SEND_DATA              (0x03u)  /* 256 byte chunk + CRC16 */
#define CMD_END_FLASH              (0x04u)  /* Transfer sonu (opsiyonel: 4B fw_size) */

/* Yanıtlar (STM32 → host) */
#define BOOTLOADER_ACK             (0x79u)  /* Başarı */
#define BOOTLOADER_NACK            (0x1Fu)  /* Hata */
#define ACK_SAME                   (0x80u)  /* MD5 eşleşti, uygulamaya atlanacak */
#define ACK_DIFF                   (0x81u)  /* MD5 farklı, flash için hazır */

#define BOOTLOADER_CHUNK_SIZE      (256u)
#define BOOTLOADER_MD5_LEN         (16u)
#define BOOTLOADER_TIMEOUT_MS      (30000u) /* 30 saniye sonra uygulamaya atla */
#define BOOTLOADER_UART_RX_TIMEOUT_MS (2000u)

/* ----------------------------------------------------------------------------
 * Metadata yapısı (Sector 1 — 0x08004000)
 * ---------------------------------------------------------------------------- */
#define BOOTLOADER_METADATA_MAGIC_0 ('S')
#define BOOTLOADER_METADATA_MAGIC_1 ('T')
#define BOOTLOADER_METADATA_MAGIC_2 ('M')
#define BOOTLOADER_METADATA_MAGIC_3 ('B')
#define BOOTLOADER_METADATA_VALID   (0xAAu)

typedef struct {
  uint8_t  magic[4];    /* 'S','T','M','B' */
  uint8_t  md5[16];     /* Yüklü firmware MD5 */
  uint32_t fw_size;     /* Firmware boyutu (byte) */
  uint8_t  valid;       /* 0xAA = geçerli */
  uint8_t  reserved[7];
} FirmwareMetadata_t;

/* ----------------------------------------------------------------------------
 * API
 * ---------------------------------------------------------------------------- */

/** Metadata geçerli mi ve uygulama adresi geçerli mi kontrolü */
bool Bootloader_IsApplicationPresent(void);

/** Uygulama vektör tablosuna jump (MSP + PC, interrupt'lar kapalı) */
void Bootloader_JumpToApplication(void);

/** Ana giriş: timeout / MD5 check / flash modu; gerekirse uygulamaya atlar */
void Bootloader_CheckAndRun(void);

#ifdef __cplusplus
}
#endif

#endif /* __BOOTLOADER_H */
