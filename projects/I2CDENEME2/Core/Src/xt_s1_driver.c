/**
 * @file xt_s1_driver.c
 * @brief XT-S1 LiDAR I2C sürücü implementasyonu (STM32 HAL) - C
 */

#include "xt_s1_driver.h"
#include "main.h"   /* I2C_HandleTypeDef, HAL_I2C_Mem_*, I2C_MEMADD_SIZE_8BIT, HAL_OK */

#define XT_S1_I2C_TIMEOUT    (100U)

/* XT-S1: 16-bit register adresi, LOW byte once (bazi moduller boyle ister) */
#define XT_S1_16BIT_LE      0   /* 1=16bit low-byte first, 0=8bit register */

#if XT_S1_16BIT_LE
static bool write_reg(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t value)
{
    const uint8_t devaddr = (XT_S1_I2C_ADDR << 1U);
    uint16_t memaddr = (uint16_t)reg << 8U;   /* 0x06 -> 0x0600 -> bus: 0x06 0x00 */
    return HAL_I2C_Mem_Write(hi2c, devaddr, memaddr, I2C_MEMADD_SIZE_16BIT,
                             &value, 1U, XT_S1_I2C_TIMEOUT) == HAL_OK;
}
static bool read_reg16(I2C_HandleTypeDef *hi2c, uint8_t reg, uint16_t *value)
{
    const uint8_t devaddr = (XT_S1_I2C_ADDR << 1U);
    uint16_t memaddr = (uint16_t)reg << 8U;
    uint8_t buf[2];
    if (HAL_I2C_Mem_Read(hi2c, devaddr, memaddr, I2C_MEMADD_SIZE_16BIT,
                         buf, 2U, XT_S1_I2C_TIMEOUT) != HAL_OK)
        return false;
    *value = (uint16_t)buf[0] | ((uint16_t)buf[1] << 8U);
    return true;
}
#else
/* 8-bit register adresi */
static bool write_reg(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t value)
{
    const uint8_t devaddr = (XT_S1_I2C_ADDR << 1U);
    return HAL_I2C_Mem_Write(hi2c, devaddr, reg, I2C_MEMADD_SIZE_8BIT,
                             &value, 1U, XT_S1_I2C_TIMEOUT) == HAL_OK;
}
static bool read_reg16(I2C_HandleTypeDef *hi2c, uint8_t reg, uint16_t *value)
{
    const uint8_t devaddr = (XT_S1_I2C_ADDR << 1U);
    uint8_t buf[2];
    if (HAL_I2C_Mem_Read(hi2c, devaddr, reg, I2C_MEMADD_SIZE_8BIT,
                         buf, 2U, XT_S1_I2C_TIMEOUT) != HAL_OK)
        return false;
    *value = (uint16_t)buf[0] | ((uint16_t)buf[1] << 8U);
    return true;
}
#endif

bool XTS1_StartMeasurement(void *hi2c)
{
    if (hi2c == NULL) return false;
    return write_reg((I2C_HandleTypeDef*)hi2c, REG_COMMAND, CMD_START);
}

bool XTS1_ReadDistance(void *hi2c, uint16_t *distance_mm)
{
    I2C_HandleTypeDef *h = (I2C_HandleTypeDef*)hi2c;
    uint16_t raw;
    uint8_t err;

    if (hi2c == NULL || distance_mm == NULL) return false;
    if (!read_reg16(h, REG_DISTANCE, &raw)) return false;

    if (XTS1_ReadError(hi2c, &err)) {
        if (err & ERR_NO_OBJECT) return false;
        if (err & ERR_SATURATION) return false;
    }
    *distance_mm = raw;
    return true;
}

bool XTS1_ReadError(void *hi2c, uint8_t *error_code)
{
    if (hi2c == NULL || error_code == NULL) return false;
    const uint8_t devaddr = (XT_S1_I2C_ADDR << 1U);
#if XT_S1_16BIT_LE
    uint16_t memaddr = (uint16_t)REG_SYS_ERROR << 8U;
    return HAL_I2C_Mem_Read((I2C_HandleTypeDef*)hi2c, devaddr, memaddr,
                            I2C_MEMADD_SIZE_16BIT, error_code, 1U,
                            XT_S1_I2C_TIMEOUT) == HAL_OK;
#else
    return HAL_I2C_Mem_Read((I2C_HandleTypeDef*)hi2c, devaddr, REG_SYS_ERROR,
                            I2C_MEMADD_SIZE_8BIT, error_code, 1U,
                            XT_S1_I2C_TIMEOUT) == HAL_OK;
#endif
}

bool XTS1_ReadTemperature(void *hi2c, int16_t *temp_x01_c)
{
    uint16_t raw;
    if (hi2c == NULL || temp_x01_c == NULL) return false;
    if (!read_reg16((I2C_HandleTypeDef*)hi2c, REG_TEMPERATURE, &raw)) return false;
    *temp_x01_c = (int16_t)raw;
    return true;
}

bool XTS1_ReadSignalAmplitude(void *hi2c, uint16_t *amplitude)
{
    if (hi2c == NULL || amplitude == NULL) return false;
    return read_reg16((I2C_HandleTypeDef*)hi2c, REG_SIGNAL_AMP, amplitude);
}
