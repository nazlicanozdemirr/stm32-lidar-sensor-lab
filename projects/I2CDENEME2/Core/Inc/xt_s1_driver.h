/**
 * @file xt_s1_driver.h
 * @brief XT-S1 LiDAR sensörü I2C sürücü (Nucleo-F446RE / STM32 HAL) - C API
 */

#ifndef XT_S1_DRIVER_H_
#define XT_S1_DRIVER_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------------------
 * XT-S1 I2C Adresi
 * --------------------------------------------------------------------------- */
#define XT_S1_I2C_ADDR       (0x20U)

/* ---------------------------------------------------------------------------
 * Register Adresleri (8-bit erişim - HAL I2C Mem)
 * --------------------------------------------------------------------------- */
#define REG_MODE_SEL         (0x00U)  /* Index 0 */
#define REG_COMMAND          (0x06U)  /* Index 3: 0x01=Start, 0x02=Stop */
#define REG_SYS_ERROR        (0x2CU)  /* Index 22: Hata kodları */
#define REG_DISTANCE         (0x2EU)  /* Index 23: Mesafe (mm) */
#define REG_TEMPERATURE      (0x30U)  /* Index 24: Sıcaklık (0.1°C) */
#define REG_SIGNAL_AMP       (0x32U)  /* Index 25: Sinyal genliği */
#define REG_SLAVE_ADDR       (0x80U)  /* Index 64: I2C adres değiştirme */
#define REG_OP_MODE          (0x82U)  /* Index 65: 0=Passive, 1=Active */
#define REG_PERIOD           (0x84U)  /* Index 66: Ölçüm periyodu (ms) */

/* ---------------------------------------------------------------------------
 * Komutlar (REG_COMMAND)
 * --------------------------------------------------------------------------- */
#define CMD_START            (0x01U)
#define CMD_STOP             (0x02U)

/* ---------------------------------------------------------------------------
 * Hata kodları (REG_SYS_ERROR)
 * --------------------------------------------------------------------------- */
#define ERR_SATURATION       (0x02U)  /* Piksel doyumu */
#define ERR_NO_OBJECT        (0x08U)  /* Nesne tespit edilmedi */

/* ---------------------------------------------------------------------------
 * Sürücü API - I2C handle CubeIDE'de main.c'de extern olarak tanımlı (hi2c1)
 * --------------------------------------------------------------------------- */

/**
 * @brief Tek ölçüm başlatır.
 * @param hi2c STM32 HAL I2C handle (örn. &hi2c1)
 * @return true başarılı, false I2C hatası
 */
bool XTS1_StartMeasurement(void *hi2c);

/**
 * @brief Mesafeyi okur (mm). Önce XTS1_StartMeasurement() çağrılmalı, kısa gecikme sonrası okunmalı.
 * @param hi2c STM32 HAL I2C handle
 * @param distance_mm Çıkış: okunan mesafe (mm)
 * @return true okuma başarılı, false I2C veya sensör hatası
 */
bool XTS1_ReadDistance(void *hi2c, uint16_t *distance_mm);

/**
 * @brief Sistem hata register'ını okur.
 * @param hi2c STM32 HAL I2C handle
 * @param error_code Çıkış: REG_SYS_ERROR değeri (ERR_* maskeleri)
 * @return true okuma başarılı
 */
bool XTS1_ReadError(void *hi2c, uint8_t *error_code);

/**
 * @brief Sıcaklığı okur (0.1°C biriminde).
 * @param hi2c STM32 HAL I2C handle
 * @param temp_x01_c Çıkış: sıcaklık (0.1°C), örn. 235 = 23.5°C
 * @return true okuma başarılı
 */
bool XTS1_ReadTemperature(void *hi2c, int16_t *temp_x01_c);

/**
 * @brief Sinyal genliğini okur.
 * @param hi2c STM32 HAL I2C handle
 * @param amplitude Çıkış: genlik değeri
 * @return true okuma başarılı
 */
bool XTS1_ReadSignalAmplitude(void *hi2c, uint16_t *amplitude);

#ifdef __cplusplus
}
#endif

#endif /* XT_S1_DRIVER_H_ */
