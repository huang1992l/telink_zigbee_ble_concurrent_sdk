/*
 *
 *    Copyright (c) 2021-2024 Project CHIP Authors
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

#include <lib/support/CHIPMem.h>
#include <platform/CHIPDeviceLayer.h>

#if CHIP_DEVICE_CONFIG_ENABLE_WIFI
#include <app/clusters/network-commissioning/network-commissioning.h>
#include <platform/telink/wifi/TelinkWiFiDriver.h>
#endif

#include <zephyr/kernel.h>

#ifdef CONFIG_USB_DEVICE_STACK
#include <zephyr/usb/usb_device.h>
#endif /* CONFIG_USB_DEVICE_STACK */

#ifdef CONFIG_CHIP_PW_RPC
#include "Rpc.h"
#endif

#include <DeviceInfoProviderImpl.h>
#include <app/clusters/identify-server/identify-server.h>
#include <app/clusters/ota-requestor/OTATestEventTriggerHandler.h>
#include <app/server/OnboardingCodesUtil.h>
#include <app/server/Server.h>
#include <app/util/attribute-storage.h>
#include <app/util/endpoint-config-api.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/reboot.h>
#include <app/clusters/color-control-server/color-control-server.h>

#include "lds_light_control.h"
#include "lds_log.h"
#include "lds_on_off_utility.h"
#include "lds_attribute_ids.h"
#include "lds_dim_algorithm.h"
#include "lds_system_common.h"
#include "lds_power_on_count.h"
#include "lds_mfg_token_config.h"
#include "lds_light_cct_algorithm.h"
#include "lds_driver_minitrim.h"
#include "lds_light_effect.h"
#include "lds_device_adc.h"
#include "lds_trf.h"
#if CONFIG_DeviceType_ExtendedColorLight
#include "lds_color_algorithm.h"
#endif

LOG_MODULE_REGISTER(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::Inet;
using namespace ::chip::DeviceLayer;

#if CHIP_DEVICE_CONFIG_ENABLE_WIFI
app::Clusters::NetworkCommissioning::Instance sWiFiCommissioningInstance(0, &(NetworkCommissioning::TelinkWiFiDriver::Instance()));
#endif

#ifdef CONFIG_CHIP_ENABLE_POWER_ON_FACTORY_RESET
static constexpr uint32_t kFactoryResetOnBootMaxCnt       = 5;
static constexpr char kFactoryResetOnBootStoreKey[]       = "TelinkFactoryResetOnBootCnt";
static constexpr uint32_t kFactoryResetUsualBootTimeoutMs = 5000;

static k_timer FactoryResetUsualBootTimer;

static void FactoryResetUsualBoot(struct k_timer * dummy)
{
    (void) dummy;
    (void) chip::DeviceLayer::PersistedStorage::KeyValueStoreMgr().Delete(kFactoryResetOnBootStoreKey);
    LOG_INF("Schedule factory counter deleted");
}

static void FactoryResetOnBoot(void)
{
    uint32_t FactoryResetOnBootCnt;
    CHIP_ERROR FactoryResetOnBootErr = chip::DeviceLayer::PersistedStorage::KeyValueStoreMgr().Get(
        kFactoryResetOnBootStoreKey, &FactoryResetOnBootCnt, sizeof(FactoryResetOnBootCnt));

    if (FactoryResetOnBootErr == CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND)
    {
        FactoryResetOnBootCnt = 1;
        if (chip::DeviceLayer::PersistedStorage::KeyValueStoreMgr().Put(kFactoryResetOnBootStoreKey, &FactoryResetOnBootCnt,
                                                                        sizeof(FactoryResetOnBootCnt)) != CHIP_NO_ERROR)
        {
            LOG_ERR("FactoryResetOnBootCnt write fail");
        }
        else
        {
            LOG_INF("Schedule factory counter %u", FactoryResetOnBootCnt);
        }
    }
    else if (FactoryResetOnBootErr == CHIP_NO_ERROR)
    {
        FactoryResetOnBootCnt++;
        if (chip::DeviceLayer::PersistedStorage::KeyValueStoreMgr().Put(kFactoryResetOnBootStoreKey, &FactoryResetOnBootCnt,
                                                                        sizeof(FactoryResetOnBootCnt)) != CHIP_NO_ERROR)
        {
            LOG_ERR("FactoryResetOnBootCnt write fail");
        }
        else
        {
            LOG_INF("Schedule factory counter %u", FactoryResetOnBootCnt);
            if (FactoryResetOnBootCnt >= kFactoryResetOnBootMaxCnt)
            {
                GetAppTask().PowerOnFactoryReset();
            }
        }
    }
    else
    {
        LOG_ERR("FactoryResetOnBootCnt read fail");
    }
    k_timer_init(&FactoryResetUsualBootTimer, FactoryResetUsualBoot, nullptr);
    k_timer_start(&FactoryResetUsualBootTimer, K_MSEC(kFactoryResetUsualBootTimeoutMs), K_NO_WAIT);
}
#endif /* CONFIG_CHIP_ENABLE_POWER_ON_FACTORY_RESET */

#define MATTER_NVS_DEMO_EN  0
#if  MATTER_NVS_DEMO_EN
void matter_nvs_demo(void)
{
    static constexpr char kFactoryResetOnBootStoreKey[]       = "TelinkFactoryResetOnBootCnt";
    uint32_t test_flag =0x55;
    
    if (chip::DeviceLayer::PersistedStorage::KeyValueStoreMgr().Put(kFactoryResetOnBootStoreKey, &test_flag,
                                                                        sizeof(test_flag)) != CHIP_NO_ERROR)
    {
        printk("FactoryResetOnBootCnt write fail\n");
    }
                                                                       
    test_flag =0xaa;
    CHIP_ERROR FactoryResetOnBootErr = chip::DeviceLayer::PersistedStorage::KeyValueStoreMgr().Get(
        kFactoryResetOnBootStoreKey, &test_flag, sizeof(test_flag));
    if(FactoryResetOnBootErr != CHIP_NO_ERROR ){
        printk("FactoryResetOnBootCnt get fail\n");
    }
    printk("nvs read value is %x \n",test_flag);
    (void) chip::DeviceLayer::PersistedStorage::KeyValueStoreMgr().Delete(kFactoryResetOnBootStoreKey);
    test_flag =0xbb;
    FactoryResetOnBootErr = chip::DeviceLayer::PersistedStorage::KeyValueStoreMgr().Get(
        kFactoryResetOnBootStoreKey, &test_flag, sizeof(test_flag));
    if(FactoryResetOnBootErr != CHIP_NO_ERROR ){
        printk("FactoryResetOnBootCnt delete after read fail \n");
    }
}
#endif

#define MATTER_NVS_EN  1
#if  MATTER_NVS_EN
#include "lds_flash_remember.h"
#include "AppTaskCommon.h"

static constexpr char kFactoryResetOnBootStoreKey[]       = "TelinkFactoryResetOnBootCnt";
const struct device * flash_para_dev = USER_PARTITION_DEVICE;
const struct device * zb_para_dev = ZB_NVS_PARTITION_DEVICE;

void matter_nvs_read(power_on_count_data_save_t *data){
    LOG_INF("*******************data->index: %x **************************\n",data->index);
    CHIP_ERROR FactoryResetOnBootErr = chip::DeviceLayer::PersistedStorage::KeyValueStoreMgr().Get(
        kFactoryResetOnBootStoreKey, data, sizeof(power_on_count_data_save_t));
    if(FactoryResetOnBootErr != CHIP_NO_ERROR ){
        LOG_INF("FactoryResetOnBootCnt get fail\n");
    }
    LOG_INF("*******************nvs read value is %x \n**************************",data->index);
}

void matter_nvs_write(power_on_count_data_save_t *data){
    if (chip::DeviceLayer::PersistedStorage::KeyValueStoreMgr().Put(kFactoryResetOnBootStoreKey, data,
                                                                            sizeof(power_on_count_data_save_t)) != CHIP_NO_ERROR)
    {
        LOG_INF("FactoryResetOnBootCnt write fail\n");
    }
}

void matter_nvs_delete(void){
    
    //(void) chip::DeviceLayer::PersistedStorage::KeyValueStoreMgr().Delete(kFactoryResetOnBootStoreKey);
    // flash_erase(flash_para_dev, MATTER_NVS_START_ADR, MATTER_NVS_SEC_SIZE);

    void * storage = nullptr;
    int status     = settings_storage_get(&storage);

    if (!status)
    {
        status = nvs_clear(static_cast<nvs_fs *>(storage));
    }

    if (!status)
    {
        status = nvs_mount(static_cast<nvs_fs *>(storage));
    }

    if (status)
    {
        ChipLogError(DeviceLayer, "Storage clearance failed: %d", status);
    }
}

void matter_factory_reset(void){
    // Erase user parameters partition and reset to Zigbee mode upon factory reset
    flash_erase(flash_para_dev, USER_PARTITION_OFFSET, USER_PARTITION_SIZE);
    // Need to erase zb nvs part 
    flash_erase(zb_para_dev, ZB_NVS_START_ADR, ZB_NVS_SEC_SIZE);

    LOG_INF("Factory reset triggered by power on 6 times, resetting to Zigbee mode");

    chip::Server::GetInstance().ScheduleFactoryResetWithoutReboot();
}

void matter_factory_reset_with_restart(void){
    // Erase user parameters partition and reset to Zigbee mode upon factory reset
    flash_erase(flash_para_dev, USER_PARTITION_OFFSET, USER_PARTITION_SIZE);
    // Need to erase zb nvs part 
    flash_erase(zb_para_dev, ZB_NVS_START_ADR, ZB_NVS_SEC_SIZE);

    LOG_INF("Factory reset triggered by power on 6 times, resetting to Zigbee mode");

    chip::Server::GetInstance().ScheduleFactoryReset();
}

void matter_shut_down(void)
{
    PlatformMgr().Shutdown();
}

#endif


static const char onoff_key[]                = "g/a/1/6/0";
static const char startUpOnOff_key[]         = "g/a/1/6/4003";
static const char currentLevel_key[]         = "g/a/1/8/0";
static const char minLevel_key[]             = "g/a/1/8/2";
static const char maxLevel_key[]             = "g/a/1/8/3";
static const char startUpCurrentLevel_key[]  = "g/a/1/8/4000";

#if (defined COLORTEMPERATURE_LIGHT) || (defined EXTENDEDCOLOR_LIGHT)
static const char x_key[]                    = "g/a/1/300/3";
static const char y_key[]                    = "g/a/1/300/4";
static const char colorTemp_key[]            = "g/a/1/300/7";
static const char colorMode_key[]            = "g/a/1/300/8";
static const char colorTempPhysicalMin_key[] = "g/a/1/300/400b";
static const char colorTempPhysicalMax_key[] = "g/a/1/300/400c";
static const char startUpColorTemp_key[]     = "g/a/1/300/4010";
#endif

#ifdef EXTENDEDCOLOR_LIGHT
static const char hue_key[]                  = "g/a/1/300/0";
static const char saturation_key[]           = "g/a/1/300/1";
#endif


bool ldsCheckLightOnOff()
{
    bool onoff = true;
    uint8_t startUpOnOff = 0x01;
    CHIP_ERROR onOffErr        = chip::DeviceLayer::PersistedStorage::KeyValueStoreMgr().Get(onoff_key, &onoff);
    CHIP_ERROR startUpOnOffErr = chip::DeviceLayer::PersistedStorage::KeyValueStoreMgr().Get(startUpOnOff_key, &startUpOnOff);

    if (!GetAppTask().OtaGetAnaFlagPublic())
    {
        // if(startUpOnOffErr == CHIP_NO_ERROR)
        {
            if (startUpOnOff == 0x00 && onoff == true) 
            {
                // Set the current OnOff attribute to 0 (off)
                onoff = false;
            } 
            else if (startUpOnOff == 0x01 && onoff == false) 
            {
                // Set the current OnOff attribute to 1 (on)
                onoff = true;
            } 
            else if (startUpOnOff == 0x02) 
            {
                // Toggle current OnOff attribute
                onoff = !onoff;
            }
        }
    }

    return onoff;
}

uint8_t ldsCheckLightCurrentLevel()
{
    uint8_t currentLevel = 0xFE;
    uint8_t startUpCurrentLevel = 0xFF;
    uint8_t minLevel = 0x01;
    uint8_t maxLevel = 0xFE;
    chip::DeviceLayer::PersistedStorage::KeyValueStoreMgr().Get(minLevel_key, &minLevel);
    chip::DeviceLayer::PersistedStorage::KeyValueStoreMgr().Get(maxLevel_key, &maxLevel);
    CHIP_ERROR err = chip::DeviceLayer::PersistedStorage::KeyValueStoreMgr().Get(currentLevel_key, &currentLevel);
    CHIP_ERROR err1 = chip::DeviceLayer::PersistedStorage::KeyValueStoreMgr().Get(startUpCurrentLevel_key, &startUpCurrentLevel);
  
    if(err1 == CHIP_NO_ERROR)
    {
        if (startUpCurrentLevel == 0x0) {
        // Set the CurrentLevel attribute to the minimum value
            currentLevel = minLevel;
        } 
        else if(startUpCurrentLevel !=  0xFF)  
        {
            if(startUpCurrentLevel < minLevel)
            {
                currentLevel = minLevel;
            }else if(startUpCurrentLevel > maxLevel)
            {
                currentLevel = maxLevel;
            }else{
                currentLevel = startUpCurrentLevel;
            }
        }
        
    }
    // LDS_LOG_I("CurrentLevel:%d, startUpCurrentLevel:%d, err:%d,%d", currentLevel, startUpCurrentLevel, err, err1);
    return currentLevel;
}

bool ldsCheckLightColorTemp(uint8_t *colorMode, uint16_t *colorTempMired)
{
    uint16_t colorTempMiredMin = 0x0099;
#if defined(COLORTEMPERATURE_LIGHT)
    uint16_t colorTempMiredMax = 0x01C6;
#elif defined(EXTENDEDCOLOR_LIGHT)
    uint16_t colorTempMiredMax = 0x022B;
#endif

    uint16_t startUpColorTempMired = 0xFFFF;
    chip::DeviceLayer::PersistedStorage::KeyValueStoreMgr().Get(colorTempPhysicalMin_key, &colorTempMiredMin);
    chip::DeviceLayer::PersistedStorage::KeyValueStoreMgr().Get(colorTempPhysicalMax_key, &colorTempMiredMax);
    CHIP_ERROR err = chip::DeviceLayer::PersistedStorage::KeyValueStoreMgr().Get(startUpColorTemp_key, &startUpColorTempMired);
    if(err == CHIP_NO_ERROR)
    {
        if (startUpColorTempMired >= colorTempMiredMin && startUpColorTempMired <= colorTempMiredMax) {
            // Update Current color temp value to StartUpColor temp value
            *colorTempMired = startUpColorTempMired;
            *colorMode = 0x02;
            return true;
        }     
    }

    return false;
}

void ldsLightInit()
{
    uint8_t target_onoff = 0x01;
    uint8_t target_level = 0xFE;

#if (defined COLORTEMPERATURE_LIGHT) || (defined EXTENDEDCOLOR_LIGHT)
    uint16_t xValue         = 0x501d;
    uint16_t yValue         = 0x52b8;
    uint8_t  colorMode      = 0x02;
    uint16_t colorTempMired = 0x0099;
#endif

#ifdef EXTENDEDCOLOR_LIGHT
    uint8_t  currentHue     = 0xC3;
    uint8_t  saturation     = 0x5;
#endif

    
    target_onoff = ldsCheckLightOnOff();
    target_level = ldsCheckLightCurrentLevel();

    #if (defined COLORTEMPERATURE_LIGHT) || (defined EXTENDEDCOLOR_LIGHT)
    if (!ldsCheckLightColorTemp(&colorMode, &colorTempMired))
    {
        chip::DeviceLayer::PersistedStorage::KeyValueStoreMgr().Get(colorMode_key, &colorMode);
        switch (colorMode)
        {   
#ifdef EXTENDEDCOLOR_LIGHT
            case ColorControlServer::EnhancedColorMode::kCurrentHueAndCurrentSaturation:
            {     
                chip::DeviceLayer::PersistedStorage::KeyValueStoreMgr().Get(hue_key, &currentHue);
                chip::DeviceLayer::PersistedStorage::KeyValueStoreMgr().Get(saturation_key, &saturation);
                break;
            }
#endif
            case ColorControlServer::EnhancedColorMode::kCurrentXAndCurrentY:
            {           
                chip::DeviceLayer::PersistedStorage::KeyValueStoreMgr().Get(x_key, &xValue);
                chip::DeviceLayer::PersistedStorage::KeyValueStoreMgr().Get(y_key, &yValue);
                break;
            }
            case ColorControlServer::EnhancedColorMode::kColorTemperature:
            {
                chip::DeviceLayer::PersistedStorage::KeyValueStoreMgr().Get(colorTemp_key, &colorTempMired);
                break;
            }
        }
    }
#endif

    lds_light_control_state_t light_state = {0};

    light_state.currentOnOff = target_onoff;
    light_state.currentLevel = target_level;

#if (defined COLORTEMPERATURE_LIGHT) || (defined EXTENDEDCOLOR_LIGHT)
    light_state.currentColorMode = colorMode;

#ifdef COLORTEMPERATURE_LIGHT
    ldsColorConversionColorTempLight(colorMode, &xValue, &yValue, &colorTempMired);
#endif

#ifdef EXTENDEDCOLOR_LIGHT
    ldsColorConversion(colorMode, &currentHue, &saturation, &xValue, &yValue, &colorTempMired);
    light_state.currentHue = currentHue;
    light_state.currentSaturation = saturation;
#endif

    light_state.currentX = xValue;
    light_state.currentY = yValue;
    light_state.currentColorTempMired = colorTempMired;
#endif

    light_state.transitionTime = 100;

    init_to_startup_matter(light_state);

}


int main(void)
{
#if defined(CONFIG_USB_DEVICE_STACK) && !defined(CONFIG_CHIP_PW_RPC)
    usb_enable(NULL);
#endif /* CONFIG_USB_DEVICE_STACK */

    CHIP_ERROR err = CHIP_NO_ERROR;

#ifdef CONFIG_CHIP_PW_RPC
    rpc::Init();
#endif
    ldsGetTokenInfoFromFlash();
#ifdef COLORTEMPERATURE_LIGHT   
    ldsPowerOnCountInitTest();
#endif    
    tsCCTAlgorithmParam_t cctAlgParam = {
                                        .driveMode = (ldsMfgTokenCctDriverModeGet() == 0x00) ? DRV_MOS : DRV_DCDC,    // Default value, DRV_DCDC
                                        .minColorTemp = 2200,                // Default value, 2700K
                                        .maxColorTemp = 6500,                // Default value, 6500K
                                        .defaultColorTemp = 2700,            // Default value, 2700K
                                        .minLevel = 0.01,                     // Default value, 0.01
                                        .defaultColorTempDutyCycle = 0.0,    // Default value, 0%
    };
    ldsLightingCCTAlgorithmInit(cctAlgParam);
#if CONFIG_DeviceType_ExtendedColorLight
    lds_color_algorithm_init();
#endif
    uint16_t pwm_min_duty_cycle = ldsGetMfgTokenPwmMinDutyCycle();
    ldsDimAlgorithmParamSet(pwm_min_duty_cycle);
    ldsPwmLutInitTable();
    ldsDriverCommonInit();


    chip::Logging::SetLogFilter(chip::Logging::kLogCategory_None);
    
    ldsMinitrimInit();
    ldsLightEffectInit();
    ldsDriverAdcInit();
    ldsDeivceNtcInit();

    unsigned char val;
    flash_read(flash_para_dev, USER_PARTITION_OFFSET, &val, 1);

    if (val == USER_MATTER_PAIR_VAL)
    {
        ldsLightInit();
    }else if (val == USER_INIT_VAL)
    {
        init_to_startup(kExampleEndpointId,true);
    }
    
    err = chip::Platform::MemoryInit();
    if (err != CHIP_NO_ERROR)
    {
        LOG_ERR("MemoryInit fail");
        goto exit;
    }

    err = PlatformMgr().InitChipStack();
    if (err != CHIP_NO_ERROR)
    {
        LOG_ERR("InitChipStack fail");
        goto exit;
    }

    err = PlatformMgr().StartEventLoopTask();
    if (err != CHIP_NO_ERROR)
    {
        LOG_ERR("StartEventLoopTask fail");
        goto exit;
    }

#ifdef CONFIG_CHIP_ENABLE_POWER_ON_FACTORY_RESET
    FactoryResetOnBoot();
#endif /* CONFIG_CHIP_ENABLE_POWER_ON_FACTORY_RESET */

#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
    err = ThreadStackMgr().InitThreadStack();
    if (err != CHIP_NO_ERROR)
    {
        LOG_ERR("InitThreadStack fail");
        goto exit;
    }

#if defined(CONFIG_CHIP_THREAD_DEVICE_ROLE_ROUTER)
    err = ConnectivityMgr().SetThreadDeviceType(ConnectivityManager::kThreadDeviceType_Router);
#elif defined(CONFIG_CHIP_THREAD_DEVICE_ROLE_END_DEVICE)
    err = ConnectivityMgr().SetThreadDeviceType(ConnectivityManager::kThreadDeviceType_MinimalEndDevice);
#elif defined(CONFIG_CHIP_THREAD_DEVICE_ROLE_SLEEPY_END_DEVICE)
    err = ConnectivityMgr().SetThreadDeviceType(ConnectivityManager::kThreadDeviceType_SleepyEndDevice);
#else
#error THREAD_DEVICE_ROLE not selected
#endif
    if (err != CHIP_NO_ERROR)
    {
        LOG_ERR("SetThreadDeviceType fail");
        goto exit;
    }
#elif CHIP_DEVICE_CONFIG_ENABLE_WIFI
    sWiFiCommissioningInstance.Init();
#else
    err = CHIP_ERROR_INTERNAL;
    goto exit;
#endif /* CHIP_DEVICE_CONFIG_ENABLE_THREAD */

#if  MATTER_NVS_DEMO_EN
    matter_nvs_demo();
#endif

    err = GetAppTask().StartApp();
    
exit:
    LOG_ERR("Exit err %" CHIP_ERROR_FORMAT, err.Format());
    return (err == CHIP_NO_ERROR) ? EXIT_SUCCESS : EXIT_FAILURE;
}
