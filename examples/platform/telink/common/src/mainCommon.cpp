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

void matter_shut_down(void)
{
    PlatformMgr().Shutdown();
}

#endif

#if APP_LIGHT_USER_MODE_EN
#if CONFIG_STARTUP_OPTIMIZATE
#include "AppTaskCommon.h"

#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>

static void init_startup_para(void)
{
    cluster_startup_para light_cluster_para;

    if (read_cluster_para(&light_cluster_para) != 0)
    {
        memset((void *) (&light_cluster_para), 0xff, (sizeof(cluster_startup_para)));

        light_cluster_para.onOff                         = 1;
        light_cluster_para.startUpOnOff                  = (uint8_t)chip::app::Clusters::OnOff::StartUpOnOffEnum::kOn;

        light_cluster_para.currentLevel                  = 254;
        light_cluster_para.minLevel                      = 1;
        light_cluster_para.maxLevel                      = 254;
        light_cluster_para.startUpCurrentLevel           = 0xff;

#if (defined COLORTEMPERATURE_LIGHT) || (defined EXTENDEDCOLOR_LIGHT)
        light_cluster_para.colorTemperatureMireds        = 0x0172;
        light_cluster_para.colorMode                     = 0x02;
        light_cluster_para.startUpColorTemperatureMireds = 0xffff;
#endif

#ifdef EXTENDEDCOLOR_LIGHT
        light_cluster_para.currentHue                    = 0x18;
        light_cluster_para.currentSaturation             = 0xC9;
        light_cluster_para.currentX                      = 0x753F;
        light_cluster_para.currentY                      = 0x68F6;
        light_cluster_para.enhancedCurrentHue            = 0x0000;
        light_cluster_para.enhancedColorMode             = 0x02;
#endif
    }
    else
    {
        if (!GetAppTask().OtaGetAnaFlagPublic())
        {
            chip::app::Clusters::OnOff::StartUpOnOffEnum cmp_startUpOnOff =
                (chip::app::Clusters::OnOff::StartUpOnOffEnum) light_cluster_para.startUpOnOff;
            if (cmp_startUpOnOff == chip::app::Clusters::OnOff::StartUpOnOffEnum::kOff)
            {
                light_cluster_para.onOff = 0;
            }
            else if (cmp_startUpOnOff == chip::app::Clusters::OnOff::StartUpOnOffEnum::kOn)
            {
                light_cluster_para.onOff = 1;
            }
            else if (cmp_startUpOnOff == chip::app::Clusters::OnOff::StartUpOnOffEnum::kToggle)
            {
                light_cluster_para.onOff = !light_cluster_para.onOff;
            }
            else
            {
                light_cluster_para.startUpOnOff = 0xff;
            }
        }

        if (light_cluster_para.currentLevel == 0xff)
        {
            light_cluster_para.currentLevel = 254;
        }
        if (light_cluster_para.minLevel == 0xff)
        {
            light_cluster_para.minLevel = 1;
        }
        if (light_cluster_para.maxLevel == 0xff)
        {
            light_cluster_para.maxLevel = 254;
        }
        if (light_cluster_para.minLevel > light_cluster_para.maxLevel)
        {
            light_cluster_para.minLevel = 1;
            light_cluster_para.maxLevel = 254;
        }

        if (light_cluster_para.startUpCurrentLevel == 0)
        {
            light_cluster_para.currentLevel = light_cluster_para.minLevel;
        }
        else if (light_cluster_para.startUpCurrentLevel != 0xff)
        {
            light_cluster_para.currentLevel = light_cluster_para.startUpCurrentLevel;
        }

        if (light_cluster_para.currentLevel < light_cluster_para.minLevel)
        {
            light_cluster_para.currentLevel = light_cluster_para.minLevel;
        }
        if (light_cluster_para.currentLevel > light_cluster_para.maxLevel)
        {
            light_cluster_para.currentLevel = light_cluster_para.maxLevel;
        }

#ifdef COLORTEMPERATURE_LIGHT

        light_cluster_para.colorMode = 0x02;

        if (light_cluster_para.colorTemperatureMireds == 0xffff)
        {
            light_cluster_para.colorTemperatureMireds = 0x0172;
        }

        if ((light_cluster_para.startUpColorTemperatureMireds >= 1) &&
            (light_cluster_para.startUpColorTemperatureMireds <= 65279))
        {
            light_cluster_para.colorTemperatureMireds = light_cluster_para.startUpColorTemperatureMireds;
        }
#endif

#ifdef EXTENDEDCOLOR_LIGHT
        if ((light_cluster_para.startUpColorTemperatureMireds >= 1) &&
            (light_cluster_para.startUpColorTemperatureMireds <= 65279))
        {
            light_cluster_para.colorMode = 0x02;
            light_cluster_para.enhancedColorMode = 0x02;
            light_cluster_para.colorTemperatureMireds = light_cluster_para.startUpColorTemperatureMireds;
        }
        else
        {
            if (light_cluster_para.colorMode > 2)
            {
                light_cluster_para.colorMode = 2;
            }

            if (light_cluster_para.enhancedColorMode > 3)
            {
                light_cluster_para.enhancedColorMode = light_cluster_para.colorMode;
            }

            if (light_cluster_para.colorMode == 0)
            {
                if (light_cluster_para.currentHue == 0xff)
                {
                    light_cluster_para.currentHue = 0x18;
                }
                if (light_cluster_para.currentSaturation == 0xff)
                {
                    light_cluster_para.currentSaturation = 0xC9;
                }
            }
            else if (light_cluster_para.colorMode == 1)
            {
                if (light_cluster_para.currentX == 0xffff)
                {
                    light_cluster_para.currentX = 0x753F;
                }
                if (light_cluster_para.currentY == 0xffff)
                {
                    light_cluster_para.currentY = 0x68F6;
                }
            }
            else
            {
                if (light_cluster_para.colorTemperatureMireds == 0xffff)
                {
                    light_cluster_para.colorTemperatureMireds = 0x0172;
                }
            }
        }
#endif
    }

    memcpy(&g_light_cluster_para, &light_cluster_para, (sizeof(cluster_startup_para)));

    lds_light_control_state_t state = {0};

    state.currentOnOff          = light_cluster_para.onOff;
    state.currentLevel          = light_cluster_para.currentLevel;
    state.transitionTime        = 100;

#if (defined COLORTEMPERATURE_LIGHT) || (defined EXTENDEDCOLOR_LIGHT)
    state.currentColorMode      = light_cluster_para.colorMode;
    state.currentColorTempMired = light_cluster_para.colorTemperatureMireds;
#endif

#ifdef EXTENDEDCOLOR_LIGHT
    state.currentX              = light_cluster_para.currentX;
    state.currentY              = light_cluster_para.currentY;
    state.currentEnhanceHue     = light_cluster_para.enhancedCurrentHue;
    state.currentHue            = light_cluster_para.currentHue;
    state.currentSaturation     = light_cluster_para.currentSaturation;
#endif

    init_to_startup_matter(state);
}
#endif
#endif

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
    ldsPowerOnCountInitTest();
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
    ldsMinitrimInit();
    ldsLightEffectInit();
    ldsDriverAdcInit();
    ldsDeivceNtcInit();
#if APP_LIGHT_USER_MODE_EN
#if CONFIG_STARTUP_OPTIMIZATE
    unsigned char val;
    flash_read(flash_para_dev, USER_PARTITION_OFFSET, &val, 1);

    if (val == USER_MATTER_PAIR_VAL)
    {
        init_cluster_partition();
        init_startup_para();
    }else if (val == USER_INIT_VAL)
    {
        init_to_startup(kExampleEndpointId,true);
    }
#endif
#endif

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
