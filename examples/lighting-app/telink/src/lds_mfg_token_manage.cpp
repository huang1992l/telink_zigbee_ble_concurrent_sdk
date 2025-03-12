/*
 * @Author: chenxiaoqian chenxiaoqian@leedarson.com
 * @Date: 2024-08-07 17:09:44
 * @LastEditors: huangshiting alyssahuang@leedarson.com
 * @LastEditTime: 2025-01-16 10:18:00
 * @FilePath: /chenxiaoqian/connectedhomeip/examples/lighting-app/telink/src/lds_mfg_token_manage.cpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */

#include "lds_mfg_token_config.h"
#include "lds_log.h"
#include "lds_error_codes.h"

#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>

#include <cstring>

#ifdef __cplusplus
extern "C" {
#endif

lds_token_info_t token_information_set = {0};

const struct device * flash_token_dev = TOKEN_PARTITION_DEVICE;

void ldsGetTokenInfoFromFlash(void){
    memset(&token_information_set, 0xFF, sizeof(token_information_set));
    lds_token_info_t * token_info = &token_information_set;
    flash_read(flash_token_dev, TOKEN_PARTITION_OFFSET + OFFSET_START_ADDRESS, token_info, sizeof(token_information_set));
    LDS_LOG_I("\n\n\n sizeof of token_information_set 0x%x \n",sizeof(token_information_set));
    LDS_LOG_I("\n\n\n token_info->off_transition_time_ds 0x%x \n",token_info->off_transition_time_ds);
}

void ldsGetTokenInfoFromGlobalVariate(lds_token_info_t * token_info_pointer){
    if(token_info_pointer == NULL){
        LDS_LOG_I("i2c_type pointer is NULL \n");
    } 
     memcpy(token_info_pointer, &token_information_set, sizeof(token_information_set));
}

lds_status_t ldsGetMfgTokenDriverI2cType(uint8_t * i2c_type ){
    if(i2c_type == NULL){
        LDS_LOG_I("i2c_type pointer is NULL \n");
        return LDS_ERROR_INVALID_PARAMETER;
    }
    *i2c_type = token_information_set.driver_i2c_type;
    if(*i2c_type == 0xFF){
        LDS_LOG_I("driver_i2c_type from token is invaild (0xFF) \n");
        return LDS_ERROR_INVALID_PARAMETER;
    }
    return LDS_SUCCESS;
}

lds_status_t ldsMfgTokenDriverI2cType(uint8_t * i2c_type)
{
    return ldsGetMfgTokenDriverI2cType(i2c_type);
}

lds_status_t ldsGetMfgTokenDriverMode(uint8_t * driver_mode){
    if(driver_mode == NULL){
        LDS_LOG_I("driver_mode pointer is NULL \n");
        return LDS_ERROR_INVALID_PARAMETER;
    }
     *driver_mode = token_information_set.driver_mode;
    if(*driver_mode == 0xFF){
        LDS_LOG_I("driver_i2c_type from token is invaild (0xFF) \n");
        return LDS_ERROR_INVALID_PARAMETER;
    }
    return LDS_SUCCESS;
}

uint8_t ldsMfgTokenDriverOverTemperature(void)
{
    return ((token_information_set.over_temperature_threshold == 0xFF) ? 0x73 : token_information_set.over_temperature_threshold);
}

#if (defined COLORTEMPERATURE_LIGHT) || (defined EXTENDEDCOLOR_LIGHT)
lds_status_t ldsGetMfgTokenColorTemperaturePercentage(uint16_t color_temperature, uint8_t *color_temperature_percentage) {
    if (color_temperature_percentage == NULL) {
        LDS_LOG_I("%d color_temperature_percentage pointer is NULL \n", color_temperature);
        return LDS_ERROR_INVALID_PARAMETER;
    }

    switch (color_temperature)
    {
    case 2700:
        *color_temperature_percentage = token_information_set.color_temperature_2700k_percentage;
        break;
    case 4000:
        *color_temperature_percentage = token_information_set.color_temperature_4000k_percentage;
        break;
    default:
        // LDS_LOG_I("%d color temperature token unsupported \n", color_temperature);
        return LDS_ERROR_INVALID_PARAMETER;
    }

    if (*color_temperature_percentage == 0xFF) {
        LDS_LOG_I("%d color_temperature_percentage from token is invalid (0xFF) \n", color_temperature);
        return LDS_ERROR_INVALID_PARAMETER;
    }
    return LDS_SUCCESS;
}

uint8_t ldsMfgTokenCctDriverModeGet(){ //MOS or DCDC mode
    uint8_t cct_mode = 0x00; //dewfalut MOS
    if(token_information_set.cct_driver_mode != 0xFF)
        cct_mode =  token_information_set.cct_driver_mode;
    else
        LDS_LOG_I("cct_driver_mode from token is invaild (0xFF) \n");
    return cct_mode;
}
#endif

uint16_t ldsGetMfgTokenPwmMinDutyCycle(){
    uint16_t min_duty_cycle = 0x0A;     //dewfalut 1%
    if(token_information_set.pwm_min_duty_cycle != 0xFF)
        min_duty_cycle =  token_information_set.pwm_min_duty_cycle;
    else
        LDS_LOG_I("pwm min duty cycle from token is invaild (0xFF) \n");
    return min_duty_cycle;
}

uint16_t ldsMfgTokenDriverMaxDutyCycle(void)
{
    return ((token_information_set.pwm_max_duty_cycle >= 1E4) ? 1E4 : token_information_set.pwm_max_duty_cycle);
}

#if defined(EXTENDEDCOLOR_LIGHT)
static bool ldsTokenConfigCheck(const unsigned char *arr) {
    size_t size = sizeof(token_information_set.cws_r_light_source_param); //defalut 
    for (size_t i = 0; i < size; ++i) {
        if (arr[i] != 0xFF) {
            return true; 
        }
    }
    return false;
}

lds_status_t ldsGetMfgTokenLightSourceParam(lds_light_param_t * param){
    if(param == NULL){
        LDS_LOG_I("param pointer is NULL \n");
        return LDS_ERROR_INVALID_PARAMETER;
    }
   if(!ldsTokenConfigCheck(token_information_set.cws_r_light_source_param) \
        || !ldsTokenConfigCheck(token_information_set.cws_g_light_source_param) \
        || !ldsTokenConfigCheck(token_information_set.cws_b_light_source_param) \
        || !ldsTokenConfigCheck(token_information_set.cws_ww_light_source_param)){ 
        return LDS_ERROR_NOT_IMPLEMENTED;
   } 
   
    //R_x
    memcpy(&(param->r.x.t),token_information_set.cws_r_light_source_param,sizeof(param->r.x.t));
    memcpy(&(param->r.x.c),&(token_information_set.cws_r_light_source_param[4]),sizeof(param->r.x.c));
    //LDS_LOG_I("param->r.x.t: %d, c:%d  \n",(int32_t)(param->r.x.t*1e8),(int32_t)(param->r.x.c*1e8));
    //R_y
    memcpy(&(param->r.y.t),&(token_information_set.cws_r_light_source_param[8]),sizeof(param->r.y.t));
    memcpy(&(param->r.y.c),&(token_information_set.cws_r_light_source_param[12]),sizeof(param->r.y.c));
    //LDS_LOG_I("param->r.y.t: %d, c:%d  \n",(int32_t)(param->r.y.t*1e8),(int32_t)(param->r.y.c*1e8));
    //R_Y
    memcpy(&(param->r.Y.t),&(token_information_set.cws_r_light_source_param[16]),sizeof(param->r.Y.t));
    memcpy(&(param->r.Y.c),&(token_information_set.cws_r_light_source_param[20]),sizeof(param->r.Y.c));
    //LDS_LOG_I("param->r.Y.t: %d, c:%d  \n",(int32_t)(param->r.Y.t*1e8),(int32_t)(param->r.Y.c*1e4));

    //R_power_limit
    param->r.led_spec.led_type = 0x00;
    memcpy(&(param->r.led_spec.max_power),&(token_information_set.cws_r_light_source_param[24]),sizeof(param->r.led_spec.max_power));
    //LDS_LOG_I("param->r.led_spec.max_power: %d  \n",(int32_t)(param->r.led_spec.max_power*1e4));
    
    //G_x
    memcpy(&(param->g.x.t),token_information_set.cws_g_light_source_param,sizeof(param->g.x.t));
    memcpy(&(param->g.x.c),&(token_information_set.cws_g_light_source_param[4]),sizeof(param->g.x.c));
    //LDS_LOG_I("param->g.x.t: %d, c:%d  \n",(int32_t)(param->g.x.t*1e8),(int32_t)(param->g.x.c*1e8));
    //G_y
    memcpy(&(param->g.y.t),&(token_information_set.cws_g_light_source_param[8]),sizeof(param->g.y.t));
    memcpy(&(param->g.y.c),&(token_information_set.cws_g_light_source_param[12]),sizeof(param->g.y.c));
    //LDS_LOG_I("param->g.y.t: %d, c:%d  \n",(int32_t)(param->g.y.t*1e8),(int32_t)(param->g.y.c*1e8));
    //G_Y
    memcpy(&(param->g.Y.t),&(token_information_set.cws_g_light_source_param[16]),sizeof(param->g.Y.t));
    memcpy(&(param->g.Y.c),&(token_information_set.cws_g_light_source_param[20]),sizeof(param->g.Y.c));
    //LDS_LOG_I("param->g.Y.t: %d, c:%d  \n",(int32_t)(param->g.Y.t*1e8),(int32_t)(param->g.Y.c*1e4));

    //G_power_limit
    param->g.led_spec.led_type = 0x01;
    memcpy(&(param->g.led_spec.max_power),&(token_information_set.cws_g_light_source_param[24]),sizeof(param->g.led_spec.max_power));
    //LDS_LOG_I("param->g.led_spec.max_power: %d  \n",(int32_t)(param->g.led_spec.max_power*1e4));
    
    //B_x
    memcpy(&(param->b.x.t),token_information_set.cws_b_light_source_param,sizeof(param->b.x.t));
    memcpy(&(param->b.x.c),&(token_information_set.cws_b_light_source_param[4]),sizeof(param->b.x.c));
    //LDS_LOG_I("param->b.x.t: %d, c:%d  \n",(int32_t)(param->b.x.t*1e8),(int32_t)(param->b.x.c*1e8));
    //B_y
    memcpy(&(param->b.y.t),&(token_information_set.cws_b_light_source_param[8]),sizeof(param->b.y.t));
    memcpy(&(param->b.y.c),&(token_information_set.cws_b_light_source_param[12]),sizeof(param->b.y.c));
    //LDS_LOG_I("param->b.y.t: %d, c:%d  \n",(int32_t)(param->b.y.t*1e8),(int32_t)(param->b.y.c*1e8));
    //B_Y
    memcpy(&(param->b.Y.t),&(token_information_set.cws_b_light_source_param[16]),sizeof(param->b.Y.t));
    memcpy(&(param->b.Y.c),&(token_information_set.cws_b_light_source_param[20]),sizeof(param->b.Y.c));
    //LDS_LOG_I("param->b.Y.t: %d, c:%d  \n",(int32_t)(param->b.Y.t*1e8),(int32_t)(param->b.Y.c*1e4));

    //B_power_limit
    param->b.led_spec.led_type = 0x02;
    memcpy(&(param->b.led_spec.max_power),&(token_information_set.cws_b_light_source_param[24]),sizeof(param->b.led_spec.max_power));
    //LDS_LOG_I("param->b.led_spec.max_power: %d  \n",(int32_t)(param->b.led_spec.max_power*1e4));

     //W1_x
    memcpy(&(param->w1.x.t),token_information_set.cws_ww_light_source_param,sizeof(param->w1.x.t));
    memcpy(&(param->w1.x.c),&(token_information_set.cws_ww_light_source_param[4]),sizeof(param->w1.x.c));
    //LDS_LOG_I("param->w1.x.t: %d, c:%d  \n",(int32_t)(param->w1.x.t*1e8),(int32_t)(param->w1.x.c*1e8));
    //W1_y
    memcpy(&(param->w1.y.t),&(token_information_set.cws_ww_light_source_param[8]),sizeof(param->w1.y.t));
    memcpy(&(param->w1.y.c),&(token_information_set.cws_ww_light_source_param[12]),sizeof(param->w1.y.c));
    //LDS_LOG_I("param->w1.y.t: %d, c:%d  \n",(int32_t)(param->w1.y.t*1e8),(int32_t)(param->w1.y.c*1e8));
    //W1_Y
    memcpy(&(param->w1.Y.t),&(token_information_set.cws_ww_light_source_param[16]),sizeof(param->w1.Y.t));
    memcpy(&(param->w1.Y.c),&(token_information_set.cws_ww_light_source_param[20]),sizeof(param->w1.Y.c));
    //LDS_LOG_I("param->w1.Y.t: %d, c:%d  \n",(int32_t)(param->w1.Y.t*1e8),(int32_t)(param->w1.Y.c*1e4));

    //W1_power_limit
    param->w1.led_spec.led_type = 0x03;
    memcpy(&(param->w1.led_spec.max_power),&(token_information_set.cws_ww_light_source_param[24]),sizeof(param->w1.led_spec.max_power));
    //LDS_LOG_I("param->w1.led_spec.max_power: %d  \n",(int32_t)(param->w1.led_spec.max_power*1e4));
    
#if defined(RGBTW)
    if(!ldsTokenConfigCheck(token_information_set.cws_cw_light_source_param)) {
         return LDS_ERROR_NOT_IMPLEMENTED;
    }
 
    //W2_x
    memcpy(&(param->w2.x.t),token_information_set.cws_cw_light_source_param,sizeof(param->w2.x.t));
    memcpy(&(param->w2.x.c),&(token_information_set.cws_cw_light_source_param[4]),sizeof(param->w2.x.c));
    //LDS_LOG_I("param->w2.x.t: %d, c:%d  \n",(int32_t)(param->w2.x.t*1e8),(int32_t)(param->w2.x.c*1e8));
    //W2_y
    memcpy(&(param->w2.y.t),&(token_information_set.cws_cw_light_source_param[8]),sizeof(param->w2.y.t));
    memcpy(&(param->w2.y.c),&(token_information_set.cws_cw_light_source_param[12]),sizeof(param->w2.y.c));
    //LDS_LOG_I("param->w2.y.t: %d, c:%d  \n",(int32_t)(param->w2.y.t*1e8),(int32_t)(param->w2.y.c*1e8));
    //W2_Y
    memcpy(&(param->w2.Y.t),&(token_information_set.cws_cw_light_source_param[16]),sizeof(param->w2.Y.t));
    memcpy(&(param->w2.Y.c),&(token_information_set.cws_cw_light_source_param[20]),sizeof(param->w2.Y.c));
   // LDS_LOG_I("param->w2.Y.t: %d, c:%d  \n",(int32_t)(param->w2.Y.t*1e8),(int32_t)(param->w2.Y.c*1e4));

    //W2_power_limit
    param->w2.led_spec.led_type = 0x04;
    memcpy(&(param->w2.led_spec.max_power),&(token_information_set.cws_cw_light_source_param[24]),sizeof(param->w2.led_spec.max_power));
   // LDS_LOG_I("param->w2.led_spec.max_power: %d  \n",(int32_t)(param->w2.led_spec.max_power*1e4));
#endif
    //RGB power limit
    memcpy(&(param->rgb_power_limit),token_information_set.cws_rgb_power_limit,sizeof(param->rgb_power_limit));

    //Total power limit
    if(!ldsTokenConfigCheck(token_information_set.cws_total_power_limit)) {
#if defined(RGBTW)
         param->total_power_limit = (param->w1.led_spec.max_power >= param->w2.led_spec.max_power)? \
         param->w1.led_spec.max_power:param->w2.led_spec.max_power;
#else
        param->total_power_limit = param->w1.led_spec.max_power;
#endif
    }else{
        memcpy(&(param->total_power_limit),token_information_set.cws_total_power_limit,sizeof(param->total_power_limit));
    }
    //LDS_LOG_I("param->rgb_power_limit: %d, total_power_limit:%d  \n",(int32_t)(param->rgb_power_limit*1e4),(int32_t)(param->total_power_limit*1e4));

    return LDS_SUCCESS;
}
#endif

#if (defined COLORTEMPERATURE_LIGHT) || (defined EXTENDEDCOLOR_LIGHT)
uint16_t ldsTokenWwOutputLimit(void)
{
    return ((token_information_set.ww_output_limit >= LDS_DEFAULT_VALUE) ? LDS_DEFAULT_VALUE : token_information_set.ww_output_limit);
}

uint16_t ldsTokenCwOutputLimit(void)
{
    return ((token_information_set.cw_output_limit >= LDS_DEFAULT_VALUE) ? LDS_DEFAULT_VALUE : token_information_set.cw_output_limit);
}

uint16_t ldsTokenMediumOutputLimit(void)
{
    return ((token_information_set.medium_output_limit >= LDS_DEFAULT_VALUE) ? LDS_DEFAULT_VALUE : token_information_set.medium_output_limit);
}
#endif

uint16_t ldsTokenPrechargeDutyCycle(void)
{
    uint16_t dutyCycle = (token_information_set.precharge_duty_cycle == 0xFF) ? token_information_set.pwm_min_duty_cycle : ((uint16_t)token_information_set.precharge_duty_cycle * 10);

    return dutyCycle;
}

uint16_t ldsTokenPrechargeDutyCycleOutputTime(void)
{
    uint16_t time = (token_information_set.precharge_duty_cycle_time == 0xFF) ? 20 : (uint16_t)token_information_set.precharge_duty_cycle_time;

    if (ldsTokenPrechargeDutyCycleCondition() != 1000)
    {
        time *= 100;
    }

    return time;
}

uint8_t ldsTokenPrechargeDutyCycleCondition(void)
{
    uint8_t dutyCycle = ((token_information_set.precharge_duty_cycle_condition == 0xFF) ? 5 : token_information_set.precharge_duty_cycle_condition);

    return dutyCycle;
}

uint32_t ldsTokenPrechargeOnOffIntervalTimeMs(void)
{
    uint32_t time = ((token_information_set.precharge_on_off_interval_time_100ms == 0xFFFF) ? 2000 : ((uint32_t)token_information_set.precharge_on_off_interval_time_100ms * 100));

    return time;
}

uint8_t ldsMfgTokenMatterDacKey(void)
{
    return token_information_set.matter_dac_key;
}

uint16_t ldsMfgTokenOnOffTransitionTimeMs(void)
{
    uint16_t time = (token_information_set.on_off_transition_time_100ms == 0xFF) ? 200 : (uint16_t)(token_information_set.on_off_transition_time_100ms * 100);

    return time;
}

#ifdef EXTENDEDCOLOR_LIGHT
lds_status_t ldsGetMfgTokenCwsAlgorithmMode(uint8_t *mode)
{
    if(mode == NULL){
        LDS_LOG_E("cwsMode pointer is NULL\n");
        return LDS_ERROR_INVALID_PARAMETER;
    }

    *mode = (token_information_set.cws_5_ways_algorithm_mode == 0xFF) ? 0 : token_information_set.cws_5_ways_algorithm_mode;

    return LDS_SUCCESS;
}
#endif

#ifdef __cplusplus
}
#endif