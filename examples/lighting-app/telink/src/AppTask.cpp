/*
 *
 *    Copyright (c) 2022-2024 Project CHIP Authors
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "AppTask.h"
#include "AppConfig.h"
#include <app/server/Server.h>

#include "ColorFormat.h"
#include "PWMManager.h"

#include <app-common/zap-generated/attributes/Accessors.h>
#include <app/DeferredAttributePersistenceProvider.h>

#include "lds_light_control.h"
#include "lds_system_common.h"
#include "lds_attribute_types.h"
#include "lds_functions.h"
#include "lds_mfg_token_config.h"

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

namespace {
bool sfixture_on;
uint8_t sBrightness;
AppTask::Fixture_Action sColorAction = AppTask::INVALID_ACTION;
XyColor_t sXY;
HsvColor_t sHSV;
CtColor_t sCT;
RgbColor_t sLedRgb;
bool initComplete;
} // namespace

AppTask AppTask::sAppTask;

/* Define a custom attribute persister which makes actual write of the CurrentHue, CurrentSaturation, CurrentLevel attributes value
 * to the non-volatile storage only when it has remained constant for 5 seconds. This is to reduce
 * the flash wearout when the attribute changes frequently as a result of MoveToLevel command.
 * DeferredAttribute object describes a deferred attribute, but also holds a buffer with a value to
 * be written, so it must live so long as the DeferredAttributePersistenceProvider object.
 * 
 * CurrentHue:
 *    DeferredAttribute(ConcreteAttributePath(kExampleEndpointId, Clusters::ColorControl::Id, Clusters::ColorControl::Attributes::CurrentHue::Id))
 * CurrentSaturation:
 *    DeferredAttribute(ConcreteAttributePath(kExampleEndpointId, Clusters::ColorControl::Id, Clusters::ColorControl::Attributes::CurrentSaturation::Id))
 * 
 * Span<DeferredAttribute>(gPersisters, 1) : (databuf, datalen)
 * System::Clock::Milliseconds32(5000)     : to store attribute when timeout 5s
 */
DeferredAttribute gPersisters[] = {
#if CONFIG_DEFERRED_ATTR_STORAGE

#ifdef EXTENDEDCOLOR_LIGHT
    DeferredAttribute(ConcreteAttributePath(kExampleEndpointId, Clusters::ColorControl::Id, Clusters::ColorControl::Attributes::CurrentHue::Id)),
    DeferredAttribute(ConcreteAttributePath(kExampleEndpointId, Clusters::ColorControl::Id, Clusters::ColorControl::Attributes::CurrentSaturation::Id)),
    DeferredAttribute(ConcreteAttributePath(kExampleEndpointId, Clusters::ColorControl::Id, Clusters::ColorControl::Attributes::EnhancedCurrentHue::Id)),
#endif

#if (defined COLORTEMPERATURE_LIGHT) || (defined EXTENDEDCOLOR_LIGHT)
    DeferredAttribute(ConcreteAttributePath(kExampleEndpointId, Clusters::ColorControl::Id, Clusters::ColorControl::Attributes::CurrentX::Id)),
    DeferredAttribute(ConcreteAttributePath(kExampleEndpointId, Clusters::ColorControl::Id, Clusters::ColorControl::Attributes::CurrentY::Id)),
    DeferredAttribute(ConcreteAttributePath(kExampleEndpointId, Clusters::ColorControl::Id, Clusters::ColorControl::Attributes::ColorTemperatureMireds::Id)),
    DeferredAttribute(ConcreteAttributePath(kExampleEndpointId, Clusters::ColorControl::Id, Clusters::ColorControl::Attributes::ColorMode::Id)),
    DeferredAttribute(ConcreteAttributePath(kExampleEndpointId, Clusters::ColorControl::Id, Clusters::ColorControl::Attributes::EnhancedColorMode::Id)),
#endif

    DeferredAttribute(ConcreteAttributePath(kExampleEndpointId, Clusters::OnOff::Id, Clusters::OnOff::Attributes::OnOff::Id)),
    DeferredAttribute(ConcreteAttributePath(kExampleEndpointId, Clusters::LevelControl::Id, Clusters::LevelControl::Attributes::CurrentLevel::Id))
#endif // CONFIG_DEFERRED_ATTR_STORAGE
};

DeferredAttributePersistenceProvider gDeferredAttributePersister(Server::GetInstance().GetDefaultAttributePersister(),
                                                                 Span<DeferredAttribute>(gPersisters, ArraySize(gPersisters)),
                                                                 System::Clock::Milliseconds32(1000));

bool AppTask::IsTurnedOn() const
{
    return sfixture_on;
}

bool AppTask::IsLightControlInitCompleted()
{
    return initComplete;
}

#ifdef CONFIG_CHIP_ENABLE_POWER_ON_FACTORY_RESET
void AppTask::PowerOnFactoryReset(void)
{
    LOG_INF("Lighting App Power On Factory Reset");
    AppEvent event;
    event.Type    = AppEvent::kEventType_DeviceAction;
    event.Handler = PowerOnFactoryResetEventHandler;
    GetAppTask().PostEvent(&event);
}
#endif /* CONFIG_CHIP_ENABLE_POWER_ON_FACTORY_RESET */

#if (APP_LIGHT_MODE == APP_LIGHT_I2C)
#if 0
void i2c_demo_proc(void)
{
    const uint8_t tx_buf[23]= { 0xc0,0x63,0x3f,0x63,    0x63,0x63,0x22,0x22,
                                0x00,0x00,0x00,0x00,    0x3f,0x3f,0x00,0x00,
                                0x00,0x00,0xff,0xff,    0x2b,0x06,0xbe};
    printk("i2c demo start \n.");
    uint32_t i2c_cfg = I2C_SPEED_SET(I2C_SPEED_FAST) | I2C_MODE_CONTROLLER;
    /* get i2c device */
    int rc;
	const struct i2c_dt_spec i2c = I2C_DT_SPEC_GET(DT_COMPAT_GET_ANY_STATUS_OKAY(ledcontrol_i2c));
    if (!device_is_ready(i2c.bus)) {
		printf("Device %s is not ready\n", i2c.bus->name);
		return ;
	}
    rc = i2c_configure(i2c.bus, i2c_cfg);
	if(rc != 0){
		printf("Failed to configure i2c device\n");
		return ;
	}
    i2c_write(i2c.bus, tx_buf+1, sizeof(tx_buf)-1,tx_buf[0]);
    printk("i2c demo stop ,finish transfer\n");
}
#endif
#endif

#if (APP_LIGHT_MODE == APP_LIGHT_ADC)
#if 0
void adc_demo_proc(void)
{
	/* Data of ADC io-channels specified in devicetree. */
	#define DT_SPEC_AND_COMMA(node_id, prop, idx) \
		ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

	static const struct adc_dt_spec adc_channels[] = {
		DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels, 
					DT_SPEC_AND_COMMA)
	};

	/* Define ADC data structure. */
	uint16_t buf;
	struct adc_sequence sequence = {
		.buffer = &buf,
		/* buffer size in bytes, not number of samples */
		.buffer_size = sizeof(buf),
	};

	int err = 0;

	/* Configure channels individually prior to sampling. */
	for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++) {
		if (!device_is_ready(adc_channels[i].dev)) {
			printf("ADC controller device %s not ready\n", adc_channels[i].dev->name);
			return;
		}

		err = adc_channel_setup_dt(&adc_channels[i]);
		if (err < 0) {
			printf("Could not setup channel #%d (%d)\n", i, err);
			return;
		}
	}

	/* ADC sampling. */
	for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++) {  // channel default is 0.
		printf("- %s, channel %d: ",
				adc_channels[i].dev->name,
				adc_channels[i].channel_id);

		(void)adc_sequence_init_dt(&adc_channels[i], &sequence);

		err = adc_read(adc_channels[i].dev, &sequence);
		if (err < 0) {
			printf("Could not read (%d)\n", err);
			continue;
		}

		int32_t val_mv = 0;

		/*
		 * If using differential mode, the 16 bit value
		 * in the ADC sample buffer should be a signed 2's
		 * complement value.
		 */
		if (adc_channels[i].channel_cfg.differential) {  // differential default is 0.
			val_mv = (int32_t)((int16_t)buf);
		} else {
			val_mv = (int32_t)buf;
		}
		printf("%" PRId32, val_mv);

		/* conversion to mV may not be supported, skip if not */
#if 0
		err = adc_raw_to_millivolts_dt(
			&adc_channels[i], &val_mv
		);
		if (err < 0) {
			printf(" (value in mV not available)\n");
		} else {
			printf(" = %" PRId32" mV\n", val_mv);
		}
#endif
	}
}
#endif
#endif

#if (0)
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>

// we need to add token partition in DTS
//=========================================
//--- a/src/platform/telink/tlsr9518adk80d_2m_flash.overlay
//  +++ b/src/platform/telink/tlsr9518adk80d_2m_flash.overlay
//  +       token_partition: partition@1fd000 {
//  +       label = "token";
//  +           reg = <0x1fd000 0x1000>; //store token info.
//  +       };
//  =========================================

#define TOKEN_PARTITION user_token_partition
#define TOKEN_PARTITION_DEVICE FIXED_PARTITION_DEVICE(TOKEN_PARTITION)
#define TOKEN_PARTITION_OFFSET FIXED_PARTITION_OFFSET(TOKEN_PARTITION)
#define TOKEN_PARTITION_SIZE FIXED_PARTITION_SIZE(TOKEN_PARTITION)

#define OFFSET_BASIC_CLUSTER_MODEL_IDENTIFIER 0x00
#define LENGHT_BASIC_CLUSTER_MODEL_IDENTIFIER 32

#define OFFSET_BASIC_CLUSTER_HARDWARE_VER 0x20
#define LENGHT_BASIC_CLUSTER_HARDWARE_VER 1

#define OFFSET_BASIC_CLUSTER_PRODUCT_CODE 0x21
#define LENGHT_BASIC_CLUSTER_PRODUCT_CODE 16

#define OFFSET_READ_COUNT 0x400
#define LENGHT_READ_COUNT 1

const struct device * flash_token_dev = TOKEN_PARTITION_DEVICE;
void token_flash_demo_proc(void)
{

    // read token infor form token bin
    uint8_t tmp_model_identifier[LENGHT_BASIC_CLUSTER_MODEL_IDENTIFIER];
    flash_read(flash_token_dev, TOKEN_PARTITION_OFFSET + OFFSET_BASIC_CLUSTER_MODEL_IDENTIFIER, tmp_model_identifier,
               LENGHT_BASIC_CLUSTER_MODEL_IDENTIFIER);
    LOG_HEXDUMP_INF(tmp_model_identifier, LENGHT_BASIC_CLUSTER_MODEL_IDENTIFIER, "flash_read:model_identifier:");

    uint8_t tmp_hardware_ver[LENGHT_BASIC_CLUSTER_HARDWARE_VER];
    flash_read(flash_token_dev, TOKEN_PARTITION_OFFSET + OFFSET_BASIC_CLUSTER_HARDWARE_VER, tmp_hardware_ver,
               LENGHT_BASIC_CLUSTER_HARDWARE_VER);
    LOG_HEXDUMP_INF(tmp_hardware_ver, LENGHT_BASIC_CLUSTER_HARDWARE_VER, "flash_read:tmp_hardware_ver:");

    uint8_t tmp_product_code[LENGHT_BASIC_CLUSTER_PRODUCT_CODE];
    flash_read(flash_token_dev, TOKEN_PARTITION_OFFSET + OFFSET_BASIC_CLUSTER_PRODUCT_CODE, tmp_product_code,
               LENGHT_BASIC_CLUSTER_PRODUCT_CODE);
    LOG_HEXDUMP_INF(tmp_product_code, LENGHT_BASIC_CLUSTER_PRODUCT_CODE, "flash_read:tmp_product_code:");

    uint8_t tmp_read_count;
    flash_read(flash_token_dev, TOKEN_PARTITION_OFFSET + OFFSET_READ_COUNT, &tmp_read_count, 1);
    if (tmp_read_count == 0xff)
        tmp_read_count = 0;
    LOG_HEXDUMP_INF(&tmp_read_count, LENGHT_READ_COUNT, "flash_read:tmp_read_count:");

    // erase token infor
    flash_erase(flash_token_dev, TOKEN_PARTITION_OFFSET, TOKEN_PARTITION_SIZE);

    // write token infor
    flash_write(flash_token_dev, TOKEN_PARTITION_OFFSET + OFFSET_BASIC_CLUSTER_MODEL_IDENTIFIER, tmp_model_identifier,
                LENGHT_BASIC_CLUSTER_MODEL_IDENTIFIER);
    LOG_HEXDUMP_INF(tmp_model_identifier, LENGHT_BASIC_CLUSTER_MODEL_IDENTIFIER, "flash_write:model_identifier:");

    flash_write(flash_token_dev, TOKEN_PARTITION_OFFSET + OFFSET_BASIC_CLUSTER_HARDWARE_VER, tmp_hardware_ver,
                LENGHT_BASIC_CLUSTER_HARDWARE_VER);
    LOG_HEXDUMP_INF(tmp_hardware_ver, LENGHT_BASIC_CLUSTER_HARDWARE_VER, "flash_write:tmp_hardware_ver:");

    flash_write(flash_token_dev, TOKEN_PARTITION_OFFSET + OFFSET_BASIC_CLUSTER_PRODUCT_CODE, tmp_product_code,
                LENGHT_BASIC_CLUSTER_PRODUCT_CODE);
    LOG_HEXDUMP_INF(tmp_product_code, LENGHT_BASIC_CLUSTER_PRODUCT_CODE, "flash_write:tmp_product_code:");

    tmp_read_count += 1;
    flash_write(flash_token_dev, TOKEN_PARTITION_OFFSET + OFFSET_READ_COUNT, &tmp_read_count, 1);
    if (tmp_read_count == 0xff)
        tmp_read_count = 0;
    LOG_HEXDUMP_INF(&tmp_read_count, LENGHT_READ_COUNT, "flash_write:tmp_read_count:");
}
#endif

void AppTask::Init_cluster_info(void)
{
    light_para_t *p_para = &light_para;
    Protocols::InteractionModel::Status status;
    bool onoff_sts;
    status = Clusters::OnOff::Attributes::OnOff::Get(1, &(onoff_sts));
    p_para->onoff = (uint8_t)onoff_sts;
    app::DataModel::Nullable<uint8_t> brightness;
    // Read brightness value
    status = Clusters::LevelControl::Attributes::CurrentLevel::Get(kExampleEndpointId, brightness);
    if (status == Protocols::InteractionModel::Status::Success && !brightness.IsNull())
    {
        p_para->level = brightness.Value();
    }
    // Read ColorMode value
    status = Clusters::ColorControl::Attributes::ColorMode::Get(1, &(p_para->color_mode));

    // Read ColorTemperatureMireds value
    status = Clusters::ColorControl::Attributes::ColorTemperatureMireds::Get(1, &(p_para->color_temp_mireds));

    // Read CurrentX value
    status = Clusters::ColorControl::Attributes::CurrentX::Get(1, &(p_para->currentx));

    // Read CurrentY value
    status = Clusters::ColorControl::Attributes::CurrentY::Get(1, &(p_para->currenty));

    // Read EnhancedCurrentHue value
    status = Clusters::ColorControl::Attributes::EnhancedCurrentHue::Get(1, &(p_para->enhanced_current_hue));
 
    // Read CurrentHue value
    status = Clusters::ColorControl::Attributes::CurrentHue::Get(1, &(p_para->cur_hue));

    // Read CurrentSaturation value
    status = Clusters::ColorControl::Attributes::CurrentSaturation::Get(1, &(p_para->cur_saturation));

    // Read OnOffTransitionTime value
    status = Clusters::LevelControl::Attributes::OnOffTransitionTime::Get(1, &(p_para->onoff_transition));

}

void AppTask::Set_cluster_info(void)
{
    light_para_t *p_para = &light_para;
    Protocols::InteractionModel::Status status;
    status = Clusters::OnOff::Attributes::OnOff::Set(1, p_para->onoff);
    // Set brightness value
    status = Clusters::LevelControl::Attributes::CurrentLevel::Set(kExampleEndpointId, p_para->level);
    // Set ColorMode value
    status = Clusters::ColorControl::Attributes::ColorMode::Set(1, p_para->color_mode);
    switch (p_para->color_mode)
    {
        case EMBER_ZCL_COLOR_MODE_CURRENT_X_AND_CURRENT_Y:
            // Set CurrentX value
            status = Clusters::ColorControl::Attributes::CurrentX::Set(1, p_para->currentx);

            // Set CurrentY value
            status = Clusters::ColorControl::Attributes::CurrentY::Set(1, p_para->currenty);
            break;
        case EMBER_ZCL_COLOR_MODE_COLOR_TEMPERATURE:
            // Set ColorTemperatureMireds value
            status = Clusters::ColorControl::Attributes::ColorTemperatureMireds::Set(1, p_para->color_temp_mireds);
            break;
#ifdef EXTENDEDCOLOR_LIGHT
        case EMBER_ZCL_COLOR_MODE_CURRENT_HUE_AND_CURRENT_SATURATION:
             // Set EnhancedCurrentHue value
            status = Clusters::ColorControl::Attributes::EnhancedCurrentHue::Set(1, p_para->enhanced_current_hue);

            // Set CurrentHue value
            status = Clusters::ColorControl::Attributes::CurrentHue::Set(1, p_para->cur_hue);

            // Set CurrentSaturation value
            status = Clusters::ColorControl::Attributes::CurrentSaturation::Set(1, p_para->cur_saturation);
            break;
#endif      
        default:
            break;
    }
}


CHIP_ERROR AppTask::Init(void)
{
    InitCommonParts();

    /* set provider to deferred store attribute */
    app::SetAttributePersistenceProvider(&gDeferredAttributePersister);

    /*user mode means led control by the customer*/
#if APP_LIGHT_USER_MODE_EN
    /* switch from zigbee , which means uncommission state .*/
    if (user_para.val == USER_ZB_SW_VAL){
        // read from flash , already proced in the AppTaskCommon::StartApp .    
        ldsInitMatterStartUp();
         //update attributes from zigbee attributes
        Set_cluster_info();

    }
// else if (user_para.val == USER_MATTER_PAIR_VAL){
// // need to get the para from the flash , which means commissioned
// #if 0
//         Init_cluster_info();
// #endif
//         // init_to_startup(kExampleEndpointId,false);
//     }else if(user_para.val == USER_INIT_VAL){
//         init_to_startup(kExampleEndpointId,true);

//     }else{
//         // will not proc .

//     }
    initComplete = true;

#if (APP_LIGHT_MODE == APP_LIGHT_I2C)
    // printk("app light mode is i2c\n");
#if 0
        i2c_demo_proc();// add i2c demo code to show the para part .
#endif
#elif (APP_LIGHT_MODE == APP_LIGHT_ADC)
    // printk("app light mode is adc\n");
    // adc_demo_proc();  // add adc demo code .
#elif (APP_LIGHT_MODE == APP_LIGHT_PWM)
    /*add pwm proc here */
    // printk("app light mode is pwm\n");
    #else
        printk("Function expansion preset position\n");
#endif
    
#else
    Protocols::InteractionModel::Status status;

    app::DataModel::Nullable<uint8_t> brightness;
    // Read brightness value
    status = Clusters::LevelControl::Attributes::CurrentLevel::Get(kExampleEndpointId, brightness);
    if (status == Protocols::InteractionModel::Status::Success && !brightness.IsNull())
    {
        sBrightness = brightness.Value();
    }

    memset(&sLedRgb, sBrightness, sizeof(RgbColor_t));

    bool storedValue;
    // Read storedValue on/off value
    status = Clusters::OnOff::Attributes::OnOff::Get(1, &storedValue);
    if (status == Protocols::InteractionModel::Status::Success)
    {
        // Set actual state to stored before reboot
        SetInitiateAction(storedValue ? ON_ACTION : OFF_ACTION, static_cast<int32_t>(AppEvent::kEventType_DeviceAction), nullptr);
    }
#endif
    return CHIP_NO_ERROR;
}

void AppTask::LightingActionEventHandler(AppEvent * aEvent)
{
    Fixture_Action action = INVALID_ACTION;
    int32_t actor         = 0;

    if (aEvent->Type == AppEvent::kEventType_DeviceAction)
    {
        action = static_cast<Fixture_Action>(aEvent->DeviceEvent.Action);
        actor  = aEvent->DeviceEvent.Actor;
    }
    else if (aEvent->Type == AppEvent::kEventType_Button)
    {
        sfixture_on = !sfixture_on;

        sAppTask.UpdateClusterState();
    }
}

void AppTask::UpdateClusterState(void)
{
    Protocols::InteractionModel::Status status;
    bool isTurnedOn  = sfixture_on;
    uint8_t setLevel = sBrightness;

    // write the new on/off value
    status = Clusters::OnOff::Attributes::OnOff::Set(kExampleEndpointId, isTurnedOn);
    if (status != Protocols::InteractionModel::Status::Success)
    {
        LOG_ERR("Update OnOff fail: %x", to_underlying(status));
    }

    status = Clusters::LevelControl::Attributes::CurrentLevel::Set(kExampleEndpointId, setLevel);
    if (status != Protocols::InteractionModel::Status::Success)
    {
        LOG_ERR("Update CurrentLevel fail: %x", to_underlying(status));
    }
}

void AppTask::SetInitiateAction(Fixture_Action aAction, int32_t aActor, uint8_t * value)
{
    bool setRgbAction = false;

    if (aAction == ON_ACTION || aAction == OFF_ACTION)
    {
        if (aAction == ON_ACTION)
        {
            sfixture_on = true;
            PwmManager::getInstance().setPwm(PwmManager::EAppPwm_Red, (((uint32_t) sLedRgb.r * 1000) / UINT8_MAX));
            PwmManager::getInstance().setPwm(PwmManager::EAppPwm_Green, (((uint32_t) sLedRgb.g * 1000) / UINT8_MAX));
            PwmManager::getInstance().setPwm(PwmManager::EAppPwm_Blue, (((uint32_t) sLedRgb.b * 1000) / UINT8_MAX));
        }
        else
        {
            sfixture_on = false;
            PwmManager::getInstance().setPwm(PwmManager::EAppPwm_Red, false);
            PwmManager::getInstance().setPwm(PwmManager::EAppPwm_Green, false);
            PwmManager::getInstance().setPwm(PwmManager::EAppPwm_Blue, false);
        }
    }
    else if (aAction == LEVEL_ACTION)
    {
        // Save a new brightness for ColorControl
        sBrightness = *value;

        if (sColorAction == COLOR_ACTION_XY)
        {
            sLedRgb = XYToRgb(sBrightness, sXY.x, sXY.y);
        }
        else if (sColorAction == COLOR_ACTION_HSV)
        {
            sHSV.v  = sBrightness;
            sLedRgb = HsvToRgb(sHSV);
        }
        else
        {
            memset(&sLedRgb, sBrightness, sizeof(RgbColor_t));
        }

        ChipLogProgress(Zcl, "New brightness: %u | R: %u, G: %u, B: %u", sBrightness, sLedRgb.r, sLedRgb.g, sLedRgb.b);
        setRgbAction = true;
    }
    else if (aAction == COLOR_ACTION_XY)
    {
        sXY     = *reinterpret_cast<XyColor_t *>(value);
        sLedRgb = XYToRgb(sBrightness, sXY.x, sXY.y);
        ChipLogProgress(Zcl, "XY to RGB: X: %u, Y: %u, Level: %u | R: %u, G: %u, B: %u", sXY.x, sXY.y, sBrightness, sLedRgb.r,
                        sLedRgb.g, sLedRgb.b);
        setRgbAction = true;
        sColorAction = COLOR_ACTION_XY;
    }
    else if (aAction == COLOR_ACTION_HSV)
    {
        sHSV    = *reinterpret_cast<HsvColor_t *>(value);
        sHSV.v  = sBrightness;
        sLedRgb = HsvToRgb(sHSV);
        ChipLogProgress(Zcl, "HSV to RGB: H: %u, S: %u, V: %u | R: %u, G: %u, B: %u", sHSV.h, sHSV.s, sHSV.v, sLedRgb.r, sLedRgb.g,
                        sLedRgb.b);
        setRgbAction = true;
        sColorAction = COLOR_ACTION_HSV;
    }
    else if (aAction == COLOR_ACTION_CT)
    {
        sCT = *reinterpret_cast<CtColor_t *>(value);
        if (sCT.ctMireds)
        {
            sLedRgb = CTToRgb(sCT);
            ChipLogProgress(Zcl, "ColorTemp to RGB: CT: %u | R: %u, G: %u, B: %u", sCT.ctMireds, sLedRgb.r, sLedRgb.g, sLedRgb.b);
            setRgbAction = true;
            sColorAction = COLOR_ACTION_CT;
        }
    }

    if (setRgbAction)
    {
        PwmManager::getInstance().setPwm(PwmManager::EAppPwm_Red, (((uint32_t) sLedRgb.r * 1000) / UINT8_MAX));
        PwmManager::getInstance().setPwm(PwmManager::EAppPwm_Green, (((uint32_t) sLedRgb.g * 1000) / UINT8_MAX));
        PwmManager::getInstance().setPwm(PwmManager::EAppPwm_Blue, (((uint32_t) sLedRgb.b * 1000) / UINT8_MAX));
    }
}

#ifdef CONFIG_CHIP_ENABLE_POWER_ON_FACTORY_RESET
static constexpr uint32_t kPowerOnFactoryResetIndicationMax    = 4;
static constexpr uint32_t kPowerOnFactoryResetIndicationTimeMs = 1000;

unsigned int AppTask::sPowerOnFactoryResetTimerCnt;
k_timer AppTask::sPowerOnFactoryResetTimer;

void AppTask::PowerOnFactoryResetEventHandler(AppEvent * aEvent)
{
    LOG_INF("Lighting App Power On Factory Reset Handler");
    sPowerOnFactoryResetTimerCnt = 1;
    PwmManager::getInstance().setPwm(PwmManager::EAppPwm_Red, (bool) (sPowerOnFactoryResetTimerCnt % 2));
    PwmManager::getInstance().setPwm(PwmManager::EAppPwm_Green, (bool) (sPowerOnFactoryResetTimerCnt % 2));
    PwmManager::getInstance().setPwm(PwmManager::EAppPwm_Blue, (bool) (sPowerOnFactoryResetTimerCnt % 2));
    k_timer_init(&sPowerOnFactoryResetTimer, PowerOnFactoryResetTimerEvent, nullptr);
    k_timer_start(&sPowerOnFactoryResetTimer, K_MSEC(kPowerOnFactoryResetIndicationTimeMs),
                  K_MSEC(kPowerOnFactoryResetIndicationTimeMs));
}

void AppTask::PowerOnFactoryResetTimerEvent(struct k_timer * timer)
{
    sPowerOnFactoryResetTimerCnt++;
    LOG_INF("Lighting App Power On Factory Reset Handler %u", sPowerOnFactoryResetTimerCnt);
    PwmManager::getInstance().setPwm(PwmManager::EAppPwm_Red, (bool) (sPowerOnFactoryResetTimerCnt % 2));
    PwmManager::getInstance().setPwm(PwmManager::EAppPwm_Green, (bool) (sPowerOnFactoryResetTimerCnt % 2));
    PwmManager::getInstance().setPwm(PwmManager::EAppPwm_Blue, (bool) (sPowerOnFactoryResetTimerCnt % 2));
    if (sPowerOnFactoryResetTimerCnt > kPowerOnFactoryResetIndicationMax)
    {
        k_timer_stop(timer);
        LOG_INF("schedule factory reset");
        chip::Server::GetInstance().ScheduleFactoryReset();
    }
}
#endif /* CONFIG_CHIP_ENABLE_POWER_ON_FACTORY_RESET */

