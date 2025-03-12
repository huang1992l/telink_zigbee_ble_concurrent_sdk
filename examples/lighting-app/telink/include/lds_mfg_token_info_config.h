#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#ifdef CONFIG_DeviceType_DimmableLight
#if CONFIG_DeviceType_DimmableLight

#ifndef DIMMABLE_LIGHT
#define DIMMABLE_LIGHT
#endif

#endif
#endif

#ifdef CONFIG_DeviceType_ColorTemperatureLight
#if CONFIG_DeviceType_ColorTemperatureLight

#ifndef COLORTEMPERATURE_LIGHT
#define COLORTEMPERATURE_LIGHT
#endif

#endif
#endif

#ifdef CONFIG_DeviceType_ExtendedColorLight
#if CONFIG_DeviceType_ExtendedColorLight

#ifndef EXTENDEDCOLOR_LIGHT
#define EXTENDEDCOLOR_LIGHT
#endif

#endif
#endif

#define TOKEN_PARTITION		user_token_partition
#define TOKEN_PARTITION_DEVICE	FIXED_PARTITION_DEVICE(TOKEN_PARTITION)
#define TOKEN_PARTITION_OFFSET	FIXED_PARTITION_OFFSET(TOKEN_PARTITION)
#define TOKEN_PARTITION_SIZE 	FIXED_PARTITION_SIZE(TOKEN_PARTITION) 


#define OFFSET_START_ADDRESS  0x00
#define LDS_DEFAULT_VALUE     10000
/*SThe struct members must be in the same order as the json tokens*/
typedef struct lds_token_info_t lds_token_info_t;
struct lds_token_info_t{
    //token info
    uint8_t token_version;
    //Device Info
    uint16_t image_type;
    uint8_t key_type;
    //Driver mode
    uint8_t driver_mode;
    uint8_t driver_i2c_type;
    //Basic Cluster Info
    uint8_t model_identifier[48];
    uint8_t hardware_version;
    uint8_t product_code[16];
    uint8_t product_type;
    uint8_t device_class;
    uint8_t device_type;
    //power settings
    uint8_t zll_rssi_threshold;
    struct {
        uint8_t low_channel : 4;
        uint8_t high_channel : 4;
    } zigbee_tx_power;
    struct {
        uint8_t low_channel : 4;
        uint8_t high_channel : 4;
    } ble_tx_power;
    //PWM settings
#if defined(DIMMABLE_LIGHT)
    uint8_t pwm_port_pin;
#elif defined(COLORTEMPERATURE_LIGHT)
    uint8_t pwm_port_pin[2];
#elif defined(EXTENDEDCOLOR_LIGHT)
    uint8_t pwm_port_pin[5];
#endif
    uint16_t pwm_frequence;
    uint16_t pwm_min_duty_cycle;  // Accuracy 1000
    uint16_t pwm_max_duty_cycle;
    //Reset settings
    uint16_t power_toggle_counter_time_limit;
    //Protection settings
    uint8_t trf_adc_source_pin;
    uint8_t over_temperature_threshold;
    uint16_t trf_pwm_limit;
#if (defined COLORTEMPERATURE_LIGHT) || (defined EXTENDEDCOLOR_LIGHT)
    //Color temperature percentage 
    uint8_t color_temperature_2700k_percentage;
    uint8_t color_temperature_4000k_percentage;
    //Color temperature mireds
    uint16_t color_temperature_mireds_min;
    uint16_t color_temperature_mireds_max;
    uint16_t default_color_temperature_mired;
    //CCT Driver mode
    uint8_t cct_driver_mode;
#endif
    //I2C Settings
#ifndef DIMMABLE_LIGHT
#if defined(EXTENDEDCOLOR_LIGHT)
    uint8_t i2c_r_current_max;
    uint8_t i2c_g_current_max;
    uint8_t i2c_b_current_max;
#endif
    uint8_t i2c_ww_current_max;
    uint8_t i2c_cw_current_max;
    uint8_t i2c_standby_mode_config;

#if defined(COLORTEMPERATURE_LIGHT)
    uint8_t i2c_out_pin[2];
#elif defined(EXTENDEDCOLOR_LIGHT)
    uint8_t i2c_out_pin[5];
#endif
    uint8_t i2c_adjust_dimming_mode;
    uint8_t i2c_rgb_dimming_mode;
    uint8_t i2c_otp_and_ovp_config;
#if defined(COLORTEMPERATURE_LIGHT)
    uint16_t i2c_adjust_gray_scale[2];
#elif defined(EXTENDEDCOLOR_LIGHT)
    uint16_t i2c_adjust_gray_scale[5];
#endif
    uint8_t i2c_chopping_frequency;
    uint8_t i2c_led_minl_threshold_setting;
#endif
    //onoff settings
    uint8_t on_transition_time_ds;
    uint8_t off_transition_time_ds;
#if defined(EXTENDEDCOLOR_LIGHT)
    //Light source type
    uint8_t light_source_number;
    //Color param
    uint8_t cws_r_light_source_param[28];
    uint8_t cws_g_light_source_param[28];
    uint8_t cws_b_light_source_param[28];
    uint8_t cws_ww_light_source_param[28];
    uint8_t cws_cw_light_source_param[28];
    uint8_t cws_rgb_power_limit[4];
    uint8_t cws_total_power_limit[4];
#endif
#if (defined COLORTEMPERATURE_LIGHT) || (defined EXTENDEDCOLOR_LIGHT)
    //Color temperature power limit
    uint16_t ww_output_limit;
    uint16_t cw_output_limit;
    uint16_t medium_output_limit;
#endif
    uint8_t freq_offset_value;
    uint8_t precharge_duty_cycle;           // Accuracy 100
    uint8_t precharge_duty_cycle_time;
    uint8_t precharge_duty_cycle_condition; // Accuracy 100
    uint16_t precharge_on_off_interval_time_100ms;
    uint8_t matter_dac_key;
    uint8_t on_off_transition_time_100ms;
#ifdef EXTENDEDCOLOR_LIGHT
    uint8_t cws_5_ways_algorithm_mode; // RGB+CCT:0x00, RGBW+CCT:0x01(gu10)
#endif
}__attribute__((packed));

#ifdef __cplusplus
}
#endif