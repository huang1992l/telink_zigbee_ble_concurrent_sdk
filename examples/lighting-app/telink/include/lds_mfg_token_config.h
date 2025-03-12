/*
 * @Author: chenxiaoqian chenxiaoqian@leedarson.com
 * @Date: 2024-08-06 20:20:39
 * @LastEditors: huangshiting alyssahuang@leedarson.com
 * @LastEditTime: 2025-01-16 10:15:59
 * @FilePath: /chenxiaoqian/connectedhomeip/examples/lighting-app/telink/include/lds_mfg_token_config.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "lds_mfg_token_info_config.h"
#include "lds_error_codes.h"

#if defined(EXTENDEDCOLOR_LIGHT)
#include "lds_color_algorithm.h"
#endif


void ldsGetTokenInfoFromFlash(void);
lds_status_t ldsGetMfgTokenDriverI2cType(uint8_t * i2c_type );
lds_status_t ldsMfgTokenDriverI2cType(uint8_t * i2c_type);
lds_status_t ldsGetMfgTokenDriverMode(uint8_t * driver_mode);  //PWM or I2C mode
void ldsGetTokenInfoFromGlobalVariate(lds_token_info_t * token_info_pointer);
//lds_status_t ldsGetMfgTokenI2cGrayScale(uint8_t * i2c_gray_scale);
uint8_t ldsMfgTokenDriverOverTemperature(void);
lds_status_t ldsGetMfgTokenColorTemperaturePercentage(uint16_t color_temperature, uint8_t *color_temperature_percentage);
uint8_t ldsMfgTokenCctDriverModeGet(); //MOS or DCDC mode
uint16_t ldsGetMfgTokenPwmMinDutyCycle();
uint16_t ldsMfgTokenDriverMaxDutyCycle(void);

#if defined(EXTENDEDCOLOR_LIGHT)
lds_status_t ldsGetMfgTokenLightSourceParam(lds_light_param_t * param);
#endif

#if (defined COLORTEMPERATURE_LIGHT) || (defined EXTENDEDCOLOR_LIGHT)
uint16_t ldsTokenWwOutputLimit(void);
uint16_t ldsTokenCwOutputLimit(void);
uint16_t ldsTokenMediumOutputLimit(void);
#endif

uint16_t ldsTokenPrechargeDutyCycle(void);
uint16_t ldsTokenPrechargeDutyCycleOutputTime(void);
uint8_t ldsTokenPrechargeDutyCycleCondition(void);
uint32_t ldsTokenPrechargeOnOffIntervalTimeMs(void);
uint8_t ldsMfgTokenMatterDacKey(void);
uint16_t ldsMfgTokenOnOffTransitionTimeMs(void);
lds_status_t ldsGetMfgTokenCwsAlgorithmMode(uint8_t *mode);

#ifdef __cplusplus
}
#endif