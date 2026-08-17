/**
  ******************************************************************************
  * @file    md5.h
  * @brief   RFC 1321 MD5 — stdlib bağımlılığı yok, STM32 uyumlu (C99)
  ******************************************************************************
  */

#ifndef __MD5_H
#define __MD5_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define MD5_DIGEST_LEN 16u

typedef struct {
  uint32_t state[4];
  uint32_t count[2];   /* bit count: count[0] = low 32 bit, count[1] = high */
  uint8_t  buffer[64];
} MD5_Context_t;

/** Context sıfırla ve MD5 hesaplamasını başlat */
void MD5_Init(MD5_Context_t *ctx);

/** Veri ekle (flash’tan 64’lük bloklar halinde okunup verilebilir) */
void MD5_Update(MD5_Context_t *ctx, const uint8_t *data, uint32_t len);

/** Sonucu 16 byte out[] dizisine yaz (finalize) */
void MD5_Final(MD5_Context_t *ctx, uint8_t out[MD5_DIGEST_LEN]);

#ifdef __cplusplus
}
#endif

#endif /* __MD5_H */
