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

#pragma once

#include "AppConfig.h"
#include "AppEventCommon.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <platform/CHIPDeviceLayer.h>

#if CONFIG_CHIP_FACTORY_DATA
#include <platform/telink/FactoryDataProvider.h>
#endif

#ifdef CONFIG_CHIP_PW_RPC
#include "Rpc.h"
#endif

#include <credentials/examples/DeviceAttestationCredsExample.h>

#include <cstdint>

#include <zephyr/fs/nvs.h>
#include <zephyr/settings/settings.h>

#include <lds_power_on_count.h>

using namespace ::chip;
using namespace ::chip::app;
using namespace ::chip::Credentials;
using namespace ::chip::DeviceLayer;

namespace {
inline constexpr EndpointId kExampleEndpointId = 1;
inline constexpr uint8_t kDefaultMinLevel      = 0;
inline constexpr uint8_t kDefaultMaxLevel      = 254;
inline constexpr uint8_t kButtonPushEvent      = 1;
inline constexpr uint8_t kButtonReleaseEvent   = 0;
} // namespace

class LedManager;
class PwmManager;
class ButtonManager;

struct Identify;

class AppTaskCommon
{
public:
#ifdef CONFIG_CHIP_ENABLE_POWER_ON_FACTORY_RESET
    void PowerOnFactoryReset(void);
#endif /* CONFIG_CHIP_ENABLE_POWER_ON_FACTORY_RESET */
    CHIP_ERROR StartApp();
    void PostEvent(AppEvent * event);

    static void IdentifyEffectHandler(Clusters::Identify::EffectIdentifierEnum aEffect);
    static void IdentifyStartHandler(Identify *);
    static void IdentifyStopHandler(Identify *);

    static bool OtaGetAnaFlagPublic(void);

#ifdef CONFIG_CHIP_PW_RPC
    enum ButtonId_t
    {
        kButtonId_ExampleAction = 1,
        kButtonId_FactoryReset,
        kButtonId_StartWiFi,
        kButtonId_StartThread,
        kButtonId_StartBleAdv
    } ButtonId;
#endif

protected:
    CHIP_ERROR InitCommonParts(void);

    void DispatchEvent(AppEvent * event);
    void GetEvent(AppEvent * aEvent);

    void InitLeds();
    virtual void LinkLeds(LedManager & ledManager);
    void InitPwms();
    virtual void LinkPwms(PwmManager & pwmManager);
    void InitButtons(void);
    virtual void LinkButtons(ButtonManager & buttonManager);

    static void IndependentFactoryReset(void);

    static void FactoryResetTimerTimeoutCallback(k_timer * timer);
    static void DnssTimerTimeoutCallback(k_timer * timer);
    static void FactoryResetTimerEventHandler(AppEvent * aEvent);
    static void FactoryResetButtonEventHandler(void);
    static void FactoryResetHandler(AppEvent * aEvent);
    static void ldsButtonEventHandler(void);
    static void buttonEventHandler(AppEvent * aEvent);
    static void StartBleAdvButtonEventHandler(void);
    static void StartBleAdvHandler(AppEvent * aEvent);

#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
    static void StartThreadButtonEventHandler(void);
    static void StartThreadHandler(AppEvent * aEvent);
#elif CHIP_DEVICE_CONFIG_ENABLE_WIFI
    static void StartWiFiButtonEventHandler(void);
    static void StartWiFiHandler(AppEvent * aEvent);
#endif

    static void ExampleActionButtonEventHandler(void);

    void SetExampleButtonCallbacks(EventHandler aAction_CB);
    EventHandler ExampleActionEventHandler;

    static void ChipEventHandler(const chip::DeviceLayer::ChipDeviceEvent * event, intptr_t arg);

    static void Get_Startup_Cluster_Info(void);

    static void UpdateStatusLED(void);

    static void OtaEventsHandler(const ChipDeviceEvent * event);
    static void OtaSetAnaFlag(void);
    static bool OtaGetAnaFlag(void);

#if CONFIG_CHIP_FACTORY_DATA
    chip::DeviceLayer::FactoryDataProvider<chip::DeviceLayer::ExternalFlashFactoryData> mFactoryDataProvider;
#endif

#ifdef CONFIG_CHIP_PW_RPC
    friend class chip::rpc::TelinkButton;
    static void ButtonEventHandler(ButtonId_t btnId, bool btnPressed);
#endif
};

#define USER_MATTER_PAIR_VAL    0x55  // jump to matter

#define USER_INIT_VAL           0xff  // init state or others will go into zb 
#define USER_ZB_SW_VAL          0xaa  // jump to matter,use XIP
#define USER_MATTER_BACK_ZB     0xa0  // only commisiion fail will back to zb 
#define USER_PARTITION user_para_partition
#define USER_PARTITION_DEVICE FIXED_PARTITION_DEVICE(USER_PARTITION)
#define USER_PARTITION_OFFSET FIXED_PARTITION_OFFSET(USER_PARTITION)
#define USER_PARTITION_SIZE FIXED_PARTITION_SIZE(USER_PARTITION)

#define USER_PARA_MAC_OFFSET            0x100

#define MATTER_NVS_START_ADR    0x380000
#define MATTER_NVS_SEC_SIZE     0xB000

// #define ZB_NVS_START_ADR    0x38b000
// #define ZB_NVS_SEC_SIZE     0x19000

#define ZB_NVS_PARTITION zigbee_partition
#define ZB_NVS_PARTITION_DEVICE FIXED_PARTITION_DEVICE(ZB_NVS_PARTITION)
#define ZB_NVS_START_ADR FIXED_PARTITION_OFFSET(ZB_NVS_PARTITION)
#define ZB_NVS_SEC_SIZE FIXED_PARTITION_SIZE(ZB_NVS_PARTITION)

#if APP_LIGHT_USER_MODE_EN
#if CONFIG_STARTUP_OPTIMIZATE
#define USER_CLUSTER_PARTITION user_cluster_partition
#define USER_CLUSTER_PARTITION_DEVICE FIXED_PARTITION_DEVICE(USER_CLUSTER_PARTITION)
#define USER_CLUSTER_PARTITION_OFFSET FIXED_PARTITION_OFFSET(USER_CLUSTER_PARTITION)
#define USER_CLUSTER_PARTITION_SIZE FIXED_PARTITION_SIZE(USER_CLUSTER_PARTITION)

typedef struct
{
    bool onOff;
    uint8_t startUpOnOff;

    uint8_t currentLevel;
    uint8_t minLevel;
    uint8_t maxLevel;
    uint8_t startUpCurrentLevel;

#if (defined COLORTEMPERATURE_LIGHT) || (defined EXTENDEDCOLOR_LIGHT)
    uint16_t colorTemperatureMireds;
    uint8_t  colorMode;
    uint16_t startUpColorTemperatureMireds;
#endif

#ifdef EXTENDEDCOLOR_LIGHT
    uint8_t  currentHue;
    uint8_t  currentSaturation;

    uint16_t currentX;
    uint16_t currentY;

    uint16_t enhancedCurrentHue;
    uint8_t  enhancedColorMode;
#endif

    uint8_t rfu[7];
} cluster_startup_para;

void init_cluster_partition(void);
int store_cluster_para(cluster_startup_para *data);
int read_cluster_para(cluster_startup_para *data);

extern cluster_startup_para g_light_cluster_para;
#endif
#endif

typedef struct{
    uint8_t val ;
    uint8_t on_net;
}user_para_t;

typedef struct{
    uint8_t     onoff;
    uint8_t     level;
    uint16_t    color_temp_mireds;
    uint16_t    currentx;
    uint16_t    currenty;
    uint16_t    enhanced_current_hue;
    uint16_t    onoff_transition;
    uint8_t     cur_hue;
    uint8_t     cur_saturation;
    uint8_t     color_mode;
}light_para_t;

extern user_para_t user_para;
extern light_para_t  light_para;
extern unsigned char para_lightness;

#ifdef __cplusplus
extern "C" {
#endif

 void matter_nvs_read(power_on_count_data_save_t *data);

 void matter_nvs_write(power_on_count_data_save_t *data);

 void matter_nvs_delete(void);

 void matter_factory_reset(void);

 void matter_factory_reset_with_restart(void);
 
 void matter_shut_down(void);

#if CONFIG_WATCHDOG
bool ldsWatchdogGetAnaFlag(void);
#endif

#ifdef __cplusplus
}
#endif