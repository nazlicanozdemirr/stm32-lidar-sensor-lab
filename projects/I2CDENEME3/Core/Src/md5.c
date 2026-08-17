/**
  ******************************************************************************
  * @file    md5.c
  * @brief   RFC 1321 MD5 — stdlib kullanılmaz (C99, STM32 uyumlu)
  ******************************************************************************
  */

#include "md5.h"
#include <string.h>

/* RFC 1321 dönüşüm makroları */
#define F(x,y,z) (((x) & (y)) | ((~(x)) & (z)))
#define G(x,y,z) (((x) & (z)) | ((y) & (~(z))))
#define H(x,y,z) ((x) ^ (y) ^ (z))
#define I(x,y,z) ((y) ^ ((x) | (~(z))))

#define ROTLEFT(a,b) (((a) << (b)) | ((a) >> (32u - (b))))

#define MD5STEP(f,a,b,c,d,x,t,s) do { \
  (a) += f((b),(c),(d)) + (x) + (t); \
  (a) = ROTLEFT(a,s); \
  (a) += (b); \
} while(0)

/* Sabitler (RFC 1321, 64 adet) */
static const uint32_t K[64] = {
  0xd76aa478u, 0xe8c7b756u, 0x242070dbu, 0xc1bdceeeu,
  0xf57c0fafu, 0x4787c62au, 0xa8304613u, 0xfd469501u,
  0x698098d8u, 0x8b44f7afu, 0xffff5bb1u, 0x895cd7beu,
  0x6b901122u, 0xfd987193u, 0xa679438eu, 0x49b40821u,
  0xf61e2562u, 0xc040b340u, 0x265e5a51u, 0xe9b6c7aau,
  0xd62f105du, 0x02441453u, 0xd8a1e681u, 0xe7d3fbc8u,
  0x21e1cde6u, 0xc33707d6u, 0xf4d50d87u, 0x455a14edu,
  0xa9e3e905u, 0xfcefa3f8u, 0x676f02d9u, 0x8d2a4c8au,
  0xfffa3942u, 0x8771f681u, 0x6d9d6122u, 0xfde5380cu,
  0xa4beea44u, 0x4bdecfa9u, 0xf6bb4b60u, 0xbebfbc70u,
  0x289b7ec6u, 0xeaa127fau, 0xd4ef3085u, 0x04881d05u,
  0xd9d4d039u, 0xe6db99e5u, 0x1fa27cf8u, 0xc4ac5665u,
  0xf4292244u, 0x432aff97u, 0xab9423a7u, 0xfc93a039u,
  0x655b59c3u, 0x8f0ccc92u, 0xffeff47du, 0x85845dd1u,
  0x6fa87e4fu, 0xfe2ce6e0u, 0xa3014314u, 0x4e0811a1u,
  0xf7537e82u, 0xbd3af235u, 0x2ad7d2bbu, 0xeb86d391u
};

static const uint8_t S[64] = {
  7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,
  5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,
  4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,
  6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21
};

/* 64 byte blok üzerinde tek dönüşüm */
static void md5_transform(uint32_t state[4], const uint8_t block[64])
{
  uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
  uint32_t x[16];
  uint32_t i;

  for (i = 0; i < 16; i++) {
    x[i] = (uint32_t)block[i*4]
         | ((uint32_t)block[i*4+1] << 8)
         | ((uint32_t)block[i*4+2] << 16)
         | ((uint32_t)block[i*4+3] << 24);
  }

  /* Round 1 */
  for (i = 0; i < 16; i++)
    MD5STEP(F, a, b, c, d, x[i], K[i], S[i]);
  /* Round 2 */
  for (i = 0; i < 16; i++)
    MD5STEP(G, a, b, c, d, x[(5*i+1)%16], K[16+i], S[16+i]);
  /* Round 3 */
  for (i = 0; i < 16; i++)
    MD5STEP(H, a, b, c, d, x[(3*i+5)%16], K[32+i], S[32+i]);
  /* Round 4 */
  for (i = 0; i < 16; i++)
    MD5STEP(I, a, b, c, d, x[(7*i)%16], K[48+i], S[48+i]);

  state[0] += a;
  state[1] += b;
  state[2] += c;
  state[3] += d;
}

static void md5_encode(uint8_t *out, const uint32_t *in, uint32_t len)
{
  uint32_t i, j;
  for (i = 0, j = 0; j < len; i++, j += 4) {
    out[j]   = (uint8_t)(in[i] & 0xffu);
    out[j+1] = (uint8_t)((in[i] >> 8) & 0xffu);
    out[j+2] = (uint8_t)((in[i] >> 16) & 0xffu);
    out[j+3] = (uint8_t)((in[i] >> 24) & 0xffu);
  }
}

void MD5_Init(MD5_Context_t *ctx)
{
  if (!ctx) return;
  ctx->state[0] = 0x67452301u;
  ctx->state[1] = 0xefcdab89u;
  ctx->state[2] = 0x98badcfeu;
  ctx->state[3] = 0x10325476u;
  ctx->count[0] = 0;
  ctx->count[1] = 0;
}

void MD5_Update(MD5_Context_t *ctx, const uint8_t *data, uint32_t len)
{
  uint32_t i, index, partLen;

  if (!ctx || !data) return;

  index = (uint32_t)((ctx->count[0] >> 3) & 0x3Fu);
  ctx->count[0] += (len << 3);
  if (ctx->count[0] < (len << 3))
    ctx->count[1]++;
  ctx->count[1] += (len >> 29);

  partLen = 64u - index;
  if (len >= partLen) {
    memcpy(&ctx->buffer[index], data, partLen);
    md5_transform(ctx->state, ctx->buffer);
    for (i = partLen; i + 63 < len; i += 64)
      md5_transform(ctx->state, &data[i]);
    index = 0;
  } else {
    i = 0;
  }
  if (i < len)
    memcpy(&ctx->buffer[index], &data[i], len - i);
}

void MD5_Final(MD5_Context_t *ctx, uint8_t out[MD5_DIGEST_LEN])
{
  uint8_t bits[8];
  uint32_t index, padLen;

  if (!ctx || !out) return;

  md5_encode(bits, ctx->count, 8);
  index = (uint32_t)((ctx->count[0] >> 3) & 0x3Fu);
  padLen = (index < 56) ? (56 - index) : (120 - index);
  {
    uint8_t padding[64];
    uint32_t k;
    padding[0] = 0x80;
    for (k = 1; k < 64; k++) padding[k] = 0;
    MD5_Update(ctx, padding, padLen);
  }
  MD5_Update(ctx, bits, 8);
  md5_encode(out, ctx->state, MD5_DIGEST_LEN);
}
