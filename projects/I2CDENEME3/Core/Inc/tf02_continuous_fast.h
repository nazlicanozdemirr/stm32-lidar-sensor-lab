/**
  ******************************************************************************
  * @file    tf02_continuous_fast.h
  * @brief   TF02-Pro I2C Continuous + Median(5) + Change-detect — frekans artırılmış
  *          Deneme için ayrı modül; main.c'den TF02_Run_Continuous_Fast() çağır.
  ******************************************************************************
  */

#ifndef TF02_CONTINUOUS_FAST_H
#define TF02_CONTINUOUS_FAST_H

#ifdef __cplusplus
extern "C" {
#endif

/**
  * @brief  I2C init'ten ÖNCE çağrılacak bus recovery (PB8=SCL, PB9=SDA).
  */
void TF02_I2C1_BusRecover(void);

/**
  * @brief  TF02-Pro sürekli okuma döngüsünü çalıştırır.
  *         GET_CM + Median(5) + değişimde yazdırma; GET_CM sonrası 2 ms bekleme (yüksek Hz).
  * @note   Çağırmadan önce: HAL_Init(), SystemClock_Config(), MX_GPIO_Init(),
  *         MX_USART2_Init(), TF02_I2C1_BusRecover(), MX_I2C1_Init().
  */
void TF02_Run_Continuous_Fast(void);

#ifdef __cplusplus
}
#endif

#endif /* TF02_CONTINUOUS_FAST_H */
