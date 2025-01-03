/********************************************************************************************************
 * @file    drv_hw.c
 *
 * @brief   This is the source file for drv_hw
 *
 * @author  Zigbee Group
 * @date    2021
 *
 * @par     Copyright (c) 2021, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *          All rights reserved.
 *
 *          Licensed under the Apache License, Version 2.0 (the "License");
 *          you may not use this file except in compliance with the License.
 *          You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *          Unless required by applicable law or agreed to in writing, software
 *          distributed under the License is distributed on an "AS IS" BASIS,
 *          WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *          See the License for the specific language governing permissions and
 *          limitations under the License.
 *
 *******************************************************************************************************/

#include "../tl_common.h"

/*
 * system clock configuration
 */
#if defined(MCU_CORE_826x)
	#if(CLOCK_SYS_CLOCK_HZ == 32000000)
		#define SYS_CLOCK_VALUE		SYS_CLK_32M_PLL
	#elif(CLOCK_SYS_CLOCK_HZ == 16000000)
		#define SYS_CLOCK_VALUE		SYS_CLK_16M_PLL
	#else
		#error please config system clock
	#endif
#elif defined(MCU_CORE_8258) || defined(MCU_CORE_8278)
	#if(CLOCK_SYS_CLOCK_HZ == 24000000)
		#define SYS_CLOCK_VALUE		SYS_CLK_24M_Crystal
	#elif(CLOCK_SYS_CLOCK_HZ == 16000000)
		#define SYS_CLOCK_VALUE		SYS_CLK_16M_Crystal
	#elif(CLOCK_SYS_CLOCK_HZ == 48000000)
		#define SYS_CLOCK_VALUE		SYS_CLK_48M_Crystal
	#else
		#error please config system clock
	#endif
#elif defined(MCU_CORE_B91) || defined(MCU_CORE_B92)
	#if(CLOCK_SYS_CLOCK_HZ == 48000000)
		#define CLOCK_INIT			CCLK_48M_HCLK_48M_PCLK_24M
	#else
		#error please config system clock
	#endif
#elif defined(MCU_CORE_B95)
	#if(CLOCK_SYS_CLOCK_HZ == 120000000)
		#define CLOCK_INIT			PLL_240M_CCLK_120M_HCLK_60M_PCLK_60M_MSPI_48M
	#else
		#error please config system clock
	#endif
#endif


//system ticks per US
u32 sysTimerPerUs;


static void randInit(void)
{
#if defined(MCU_CORE_8258) || defined(MCU_CORE_8278)
	random_generator_init();
#elif defined(MCU_CORE_B91) || defined(MCU_CORE_B92) || defined(MCU_CORE_B95)
	trng_init();
#endif
}

/*********************************************************************
 * @fn      internalFlashSizeCheck
 *
 * @brief   This function is provided to get and update to the correct flash address
 * 			where are stored the right MAC address and pre-configured parameters.
 * 			NOTE: It should be called before ZB_RADIO_INIT().
 *
 * @param   None
 *
 * @return  None
 */
static void internalFlashSizeCheck(void){
#if defined(MCU_CORE_8258) || defined(MCU_CORE_8278) || defined(MCU_CORE_B91) || defined(MCU_CORE_B92) || defined(MCU_CORE_B95)
#if defined(MCU_CORE_B95)
	u32 mid = flash_read_mid_with_device_num(SLAVE0);
#else
	u32 mid = flash_read_mid();
#endif

	u8 *pMid = (u8 *)&mid;

	if( (pMid[2] < FLASH_SIZE_512K) || \
		((g_u32MacFlashAddr == FLASH_ADDR_OF_MAC_ADDR_1M) && (pMid[2] < FLASH_SIZE_1M)) || \
		((g_u32MacFlashAddr == FLASH_ADDR_OF_MAC_ADDR_2M) && (pMid[2] < FLASH_SIZE_2M)) || \
		((g_u32MacFlashAddr == FLASH_ADDR_OF_MAC_ADDR_4M) && (pMid[2] < FLASH_SIZE_4M))){
		/* Flash space not matched. */
		while(1);
	}

	switch(pMid[2]){
		case FLASH_SIZE_1M:
			g_u32MacFlashAddr = FLASH_ADDR_OF_MAC_ADDR_1M;
			g_u32CfgFlashAddr = FLASH_ADDR_OF_F_CFG_INFO_1M;
			break;
		case FLASH_SIZE_2M:
			g_u32MacFlashAddr = FLASH_ADDR_OF_MAC_ADDR_2M;
			g_u32CfgFlashAddr = FLASH_ADDR_OF_F_CFG_INFO_2M;
			break;
		case FLASH_SIZE_4M:
			g_u32MacFlashAddr = FLASH_ADDR_OF_MAC_ADDR_4M;
			g_u32CfgFlashAddr = FLASH_ADDR_OF_F_CFG_INFO_4M;
			break;
		default:
			break;
	}
#endif
}

#if VOLTAGE_DETECT_ENABLE || defined(MCU_CORE_8258)
static void voltage_detect_init(u32 detectPin)
{
	drv_adc_init();

#if defined(MCU_CORE_826x)
	(void)detectPin;
	drv_adc_mode_pin_set(DRV_ADC_VBAT_MODE, NOINPUT);
#elif defined(MCU_CORE_8258) || defined(MCU_CORE_8278)
	drv_adc_mode_pin_set(DRV_ADC_VBAT_MODE, (GPIO_PinTypeDef)detectPin);
#elif defined(MCU_CORE_B91) || defined(MCU_CORE_B92)
	drv_adc_mode_pin_set(DRV_ADC_BASE_MODE, (adc_input_pin_def_e)detectPin);
#elif defined(MCU_CORE_B95)
	drv_adc_mode_pin_set(DRV_ADC_BASE_MODE, (adc_input_pin_e)detectPin);
#endif

	drv_adc_enable(1);
	WaitUs(100);
}
#endif

#if VOLTAGE_DETECT_ENABLE
#define VOLTAGE_DEBOUNCE_NUM 	5
u16 voltage_detect(bool powerOn, u16 volThreshold)
{
	u16 voltage = drv_get_adc_data();
	u32 curTick = clock_time();
	s32 debounceNum = VOLTAGE_DEBOUNCE_NUM;
	u16 v_min = 0;

	//printf("VDD: %d\n", voltage);
	if(powerOn || voltage <= volThreshold){
		while(debounceNum > 0){
			WaitUs(100);
			voltage = drv_get_adc_data();
			if(voltage > volThreshold){
				debounceNum--;
			}else{
				debounceNum = VOLTAGE_DEBOUNCE_NUM;
				v_min = voltage;
			}

			if(clock_time_exceed(curTick, 1000 * 1000)){
				return v_min;
			}
		}
	}

	return voltage;
}
#endif

static startup_state_e platform_wakeup_init(void)
{
	startup_state_e state = SYSTEM_BOOT;

#if defined(MCU_CORE_826x) || defined(MCU_CORE_8258)
	cpu_wakeup_init();
#elif defined(MCU_CORE_8278)
	cpu_wakeup_init(LDO_MODE, EXTERNAL_XTAL_24M);
#elif defined(MCU_CORE_B91)
	blc_pm_select_internal_32k_crystal();
	//reg_embase_addr = 0xc0000000;//default is 0xc0200000;
	sys_init(DCDC_1P4_LDO_1P8, VBAT_MAX_VALUE_GREATER_THAN_3V6);
#elif defined(MCU_CORE_B92)
	sys_init(DCDC_1P4_LDO_2P0, VBAT_MAX_VALUE_GREATER_THAN_3V6, GPIO_VOLTAGE_3V3, INTERNAL_CAP_XTAL24M);
#elif defined(MCU_CORE_B95)
	sys_init(LDO_0P94_LDO_1P8, VBAT_MAX_VALUE_GREATER_THAN_3V6);
#endif

#if defined(MCU_CORE_B92) || defined(MCU_CORE_B95)
    wd_32k_stop();
    wd_stop();

#if defined(MCU_CORE_B95)
    //for A0 version
	pm_set_dig_ldo_voltage(DIG_LDO_TRIM_0P850V);
#endif
#endif

#if defined(MCU_CORE_826x)
	//826x not support ram retention.
	state = (pm_mcu_status == MCU_STATUS_DEEP_BACK) ? SYSTEM_DEEP : SYSTEM_BOOT;
#elif defined(MCU_CORE_8258) || defined(MCU_CORE_8278)
	state = (startup_state_e)pm_get_mcu_status();
#elif defined(MCU_CORE_B91) || defined(MCU_CORE_B92) || defined(MCU_CORE_B95)
	if(g_pm_status_info.mcu_status == MCU_STATUS_DEEPRET_BACK){
		state = SYSTEM_DEEP_RETENTION;
	}else if(g_pm_status_info.mcu_status == MCU_STATUS_DEEP_BACK){
		state = SYSTEM_DEEP;
	}
#endif

	return state;
}

/****************************************************************************************************
* @brief 		platform initialization function
*
* @param[in] 	none
*
* @return	  	1: startup with ram retention;
*             	0: no ram retention.
*/
startup_state_e drv_platform_init(void)
{
	drv_disable_irq();
	drv_irqMask_clear();

	startup_state_e state = platform_wakeup_init();

#if defined(MCU_CORE_826x) || defined(MCU_CORE_8258) || defined(MCU_CORE_8278)
	clock_init(SYS_CLOCK_VALUE);
#elif defined(MCU_CORE_B91) || defined(MCU_CORE_B92) || defined(MCU_CORE_B95)
	CLOCK_INIT;
#endif

	/* Get system ticks per US, must be after the clock is initialized. */
#if defined(MCU_CORE_826x)
	sysTimerPerUs = tickPerUs;
#elif defined(MCU_CORE_8258) || defined(MCU_CORE_8278)
	sysTimerPerUs = sys_tick_per_us;
#elif defined(MCU_CORE_B91) || defined(MCU_CORE_B92) || defined(MCU_CORE_B95)
	sysTimerPerUs = SYSTEM_TIMER_TICK_1US;
#endif

	gpio_init(TRUE);

#if UART_PRINTF_MODE || TLKAPI_DEBUG_ENABLE
	DEBUG_TX_PIN_INIT();
#endif

	if(state != SYSTEM_DEEP_RETENTION){
		randInit();
		internalFlashSizeCheck();
#if PM_ENABLE
		PM_CLOCK_INIT();
#endif
	}else{
#if PM_ENABLE
		drv_pm_wakeupTimeUpdate();
#endif
	}

	/* Get calibration info to improve performance */
	drv_calibration();

#if VOLTAGE_DETECT_ENABLE
	voltage_detect_init(VOLTAGE_DETECT_ADC_PIN);
#endif
	
#if defined(MCU_CORE_B95)
	/* AES enable after clock_init */
	ske_dig_en();
#endif

#if defined(BLE_MODE_ONLY)
	return state;
#endif

#if defined(MCU_CORE_8258)
	if(flash_is_zb()){

#if (!VOLTAGE_DETECT_ENABLE) || !defined(VOLTAGE_DETECT_ENABLE)
		voltage_detect_init(VOLTAGE_DETECT_ADC_PIN);
#endif
		flash_safe_voltage_set(VOLTAGE_SAFETY_THRESHOLD);
		flash_unlock_mid13325e();  //add it for the flash which sr is expired
	}
#endif

	ZB_RADIO_INIT();

	//ZB_TIMER_INIT();

	return state;
}

void drv_enable_irq(void)
{
#if defined(MCU_CORE_826x) || defined(MCU_CORE_8258) || defined(MCU_CORE_8278)
	irq_enable();
#elif defined(MCU_CORE_B91) || defined(MCU_CORE_B92) || defined(MCU_CORE_B95)
	core_interrupt_enable();
#endif
}

_attribute_ram_code_ u32 drv_disable_irq(void)
{
#if defined(MCU_CORE_826x) || defined(MCU_CORE_8258) || defined(MCU_CORE_8278)
	return (u32)irq_disable();
#elif defined(MCU_CORE_B91) || defined(MCU_CORE_B92) || defined(MCU_CORE_B95)
	return core_interrupt_disable();
#endif
}

void drv_irqMask_clear(void){
#if defined(MCU_CORE_826x) || defined(MCU_CORE_8258) || defined(MCU_CORE_8278)
	irq_disable_type(FLD_IRQ_ALL);
#elif defined(MCU_CORE_B91) || defined(MCU_CORE_B92) || defined(MCU_CORE_B95)
	reg_irq_src0 = 0;
	reg_irq_src1 = 0;
	core_mie_disable(FLD_MIE_MSIE);
	core_mie_disable(FLD_MIE_MTIE);
	core_mie_disable(FLD_MIE_MEIE);
#endif
}

_attribute_ram_code_ u32 drv_restore_irq(u32 en)
{
#if defined(MCU_CORE_826x) || defined(MCU_CORE_8258) || defined(MCU_CORE_8278)
	irq_restore((u8)en);
	return 0;
#elif defined(MCU_CORE_B91) || defined(MCU_CORE_B92) || defined(MCU_CORE_B95)
	return core_restore_interrupt(en);
#endif
}

void drv_wd_setInterval(u32 ms)
{
	wd_set_interval_ms(ms);
}

void drv_wd_start(void)
{
	wd_start();
}

void drv_wd_clear(void)
{
	wd_clear();
}

u32 drv_u32Rand(void)
{
#if defined(MCU_CORE_826x) || defined(MCU_CORE_8258) || defined(MCU_CORE_8278)
	return rand();
#elif defined(MCU_CORE_B91) || defined(MCU_CORE_B92) || defined(MCU_CORE_B95)
	return trng_rand();
#else
	return 0;
#endif
}

void drv_generateRandomData(u8 *pData, u8 len)
{
	u32 randNums = 0;
	/* if len is odd */
	for(u8 i = 0; i < len; i++){
		if((i & 3) == 0){
			randNums = drv_u32Rand();
		}

		pData[i] = randNums & 0xff;
		randNums >>= 8;
	}
}

/*For vbus supply -- close vbus watchdog, watchdog time is 8s.*/
#if defined(MCU_CORE_B92)
volatile u32 g_vbus_timer_turn_off_start_tick = 0;
volatile u8 g_vbus_timer_turn_off_flag = 0;
void drv_vbusWatchdogClose(void){
	if(usb_get_vbus_detect_status()){
		if(g_vbus_timer_turn_off_flag == 0){
			if(g_vbus_timer_turn_off_start_tick == 0){
				g_vbus_timer_turn_off_start_tick = stimer_get_tick();
			}

			if(clock_time_exceed(g_vbus_timer_turn_off_start_tick, 100 * 1000)){
				wd_turn_off_vbus_timer();//clear reset

				g_vbus_timer_turn_off_start_tick = 0;
				g_vbus_timer_turn_off_flag = 1;
			}
		}
	}else if(g_vbus_timer_turn_off_flag){
		g_vbus_timer_turn_off_start_tick = 0;
		g_vbus_timer_turn_off_flag = 0;
	}
}
#endif
