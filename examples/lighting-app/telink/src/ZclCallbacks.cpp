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
#include "ColorFormat.h"

#include <app-common/zap-generated/attributes/Accessors.h>
#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app/ConcreteAttributePath.h>
#include <lib/support/logging/CHIPLogging.h>
#include <app/clusters/color-control-server/color-control-server.h>
#include "lds_light_control.h"
#include "lds_color_utility.h"

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);



#if  APP_LIGHT_USER_MODE_EN         
using namespace chip;
using namespace chip::app::Clusters;



#if CONFIG_STARTUP_OPTIMIZATE
#include "AppTaskCommon.h"

static void UpdateOnOff(uint8_t * value)
{
    cluster_startup_para * p_para = &g_light_cluster_para;

    if (p_para->onOff != *value)
    {
        p_para->onOff = *value;

        if (store_cluster_para(p_para) != 0)
        {
            LDS_LOG_E("[clusterId:OnOff] Fail store startup cluster para\n");
        }

        onoff_attribute_change_handle(OnOff::Attributes::OnOff::Id, 0xFF, 0xFFFF, value);
    }
}

static void UpdateStartUpOnOff(uint8_t * value)
{
    cluster_startup_para * p_para = &g_light_cluster_para;

    uint8_t tmp = *value;
    DataModel::Nullable<chip::app::Clusters::OnOff::StartUpOnOffEnum> tmp1 =
        (chip::app::Clusters::OnOff::StartUpOnOffEnum) tmp;
    if (tmp1.IsNull())
    {
        tmp = 0xff;
    }

    if (p_para->startUpOnOff != tmp)
    {
        p_para->startUpOnOff = tmp;
        if (store_cluster_para(p_para) != 0)
        {
            LDS_LOG_E("[clusterId:OnOff] Fail store startup cluster para\n");
        }
    }
}

static void UpdateCurrentLevel(uint8_t * value)
{
    cluster_startup_para * p_para = &g_light_cluster_para;

    if (p_para->currentLevel != *value)
    {
        p_para->currentLevel = *value;

        if (store_cluster_para(p_para) != 0)
        {
            LDS_LOG_E("Fail store startup cluster para\n");
        }
    
        level_attribute_change_handle(LevelControl::Attributes::CurrentLevel::Id, 0xFF, 0xFFFF, value);
    }
}

static void UpdateMinLevel(uint8_t * value)
{
    cluster_startup_para * p_para = &g_light_cluster_para;

    if (p_para->minLevel != *value)
    {
        p_para->minLevel = *value;

        if (store_cluster_para(p_para) != 0)
        {
            LDS_LOG_E("Fail store startup cluster para\n");
        }
    }
}

static void UpdateMaxLevel(uint8_t * value)
{
    cluster_startup_para * p_para = &g_light_cluster_para;

    if (p_para->maxLevel != *value)
    {
        p_para->maxLevel = *value;

        if (store_cluster_para(p_para) != 0)
        {
            LDS_LOG_E("Fail store startup cluster para\n");
        }
    }
}

static void UpdateStartUpCurrentLevel(uint8_t * value)
{
    cluster_startup_para * p_para = &g_light_cluster_para;

    uint8_t tmp = *value;

    DataModel::Nullable<uint8_t> tmp1 = tmp;
    if (tmp1.IsNull())
    {
        tmp = 0xff;
    }

    p_para->startUpCurrentLevel = tmp;

    if (store_cluster_para(p_para) != 0)
    {
        LDS_LOG_E("Fail store startup cluster para\n");
    }
}

static void UpdateColorTemperatureMireds(uint8_t * value)
{
    Protocols::InteractionModel::Status status;
    cluster_startup_para * p_para = &g_light_cluster_para;

    if (p_para->colorTemperatureMireds != *reinterpret_cast<uint16_t *>(value))
    {
        p_para->colorTemperatureMireds = *reinterpret_cast<uint16_t *>(value);

        uint8_t colorModeValue = 0;

        status = Clusters::ColorControl::Attributes::ColorMode::Get(1, &colorModeValue);
        if (status == Protocols::InteractionModel::Status::Success)
        {
            p_para->colorMode = colorModeValue;
        }

        if (p_para->colorMode == (uint8_t)Clusters::ColorControl::ColorMode::kColorTemperature)
        {
#ifdef EXTENDEDCOLOR_LIGHT
            uint8_t enhancedColorModeValue = 0;

            status = Clusters::ColorControl::Attributes::EnhancedColorMode::Get(1, &enhancedColorModeValue);
            if (status == Protocols::InteractionModel::Status::Success)
            {
                p_para->enhancedColorMode = enhancedColorModeValue;
            }

            ldsColorConversion(p_para->colorMode, &p_para->currentHue, &p_para->currentSaturation, &p_para->currentX, &p_para->currentY, &p_para->colorTemperatureMireds);
#endif

            lds_light_control_state_t *state = lds_light_control_state_get();

            if (state->currentColorTempMired != p_para->colorTemperatureMireds)
            {
                state->currentColorTempMired = p_para->colorTemperatureMireds;
                state->transitionTime = 100;

#ifdef EXTENDEDCOLOR_LIGHT
                state->currentX          = p_para->currentX;
                state->currentY          = p_para->currentY;
                state->currentHue        = p_para->currentHue;
                state->currentSaturation = p_para->currentSaturation;
#endif
                ldsBulbDriverMinitrimCtrlMoveTo(state);
            }

#ifdef EXTENDEDCOLOR_LIGHT
            // lds_color_util_update_hue_saturation(1, p_para->currentHue, p_para->currentSaturation);
            // lds_color_util_update_xy(1, p_para->currentX, p_para->currentY);
            chip::app::MarkAttributeDirty markXDirty = chip::app::MarkAttributeDirty::kNo;
            ColorControl::Attributes::CurrentHue::Set(1, p_para->currentHue, markXDirty);
            ColorControl::Attributes::CurrentSaturation::Set(1, p_para->currentSaturation, markXDirty);
            ColorControl::Attributes::CurrentX::Set(1, p_para->currentX, markXDirty);
            ColorControl::Attributes::CurrentY::Set(1, p_para->currentY, markXDirty);
#endif
            if (store_cluster_para(p_para) != 0)
            {
                LDS_LOG_E("Fail store startup cluster para\n");
            }
        }
    }
}

static void UpdateColorMode(uint8_t * value)
{
    cluster_startup_para * p_para = &g_light_cluster_para;

    if (p_para->colorMode != *value)
    {
        p_para->colorMode = *value;

        lds_light_control_state_t *state = lds_light_control_state_get();
        state->currentColorMode = *value;

        if (store_cluster_para(p_para) != 0)
        {
            LDS_LOG_E("Fail store startup cluster para\n");
        }
    }
}

static void UpdateStartUpColorTemperatureMireds(uint8_t * value)
{
    Protocols::InteractionModel::Status status;
    cluster_startup_para * p_para = &g_light_cluster_para;

    uint8_t colorModeValue = 0;

    status = Clusters::ColorControl::Attributes::ColorMode::Get(1, &colorModeValue);
    if (status == Protocols::InteractionModel::Status::Success)
    {
        p_para->colorMode = colorModeValue;
    }

#ifdef EXTENDEDCOLOR_LIGHT
    uint8_t enhancedColorModeValue = 0;

    status = Clusters::ColorControl::Attributes::EnhancedColorMode::Get(1, &enhancedColorModeValue);
    if (status == Protocols::InteractionModel::Status::Success)
    {
        p_para->enhancedColorMode = enhancedColorModeValue;
    }
#endif
    uint16_t tmp = *reinterpret_cast<uint16_t *>(value);
    DataModel::Nullable<uint16_t> tmp1 = tmp;

    if (tmp1.IsNull())
    {
        tmp = 0xffff;
    }

    if (p_para->startUpColorTemperatureMireds != tmp)
    {
        p_para->startUpColorTemperatureMireds = tmp;

        if (store_cluster_para(p_para) != 0)
        {
            LDS_LOG_E("Fail store startup cluster para\n");
        }
    }
}
#ifdef EXTENDEDCOLOR_LIGHT
static void UpdateCurrentHue(uint8_t * value)
{
    Protocols::InteractionModel::Status status;
    cluster_startup_para * p_para = &g_light_cluster_para;

    if (p_para->currentHue != *value)
    {
        p_para->currentHue = *value;

        uint8_t colorModeValue = 0;

        status = Clusters::ColorControl::Attributes::ColorMode::Get(1, &colorModeValue);
        if (status == Protocols::InteractionModel::Status::Success)
        {
            p_para->colorMode = colorModeValue;
        }

        if (p_para->colorMode == (uint8_t)Clusters::ColorControl::ColorMode::kCurrentHueAndCurrentSaturation)
        {
#ifdef EXTENDEDCOLOR_LIGHT
            uint8_t enhancedColorModeValue = 0;

            status = Clusters::ColorControl::Attributes::EnhancedColorMode::Get(1, &enhancedColorModeValue);
            if (status == Protocols::InteractionModel::Status::Success)
            {
                p_para->enhancedColorMode = enhancedColorModeValue;
            }

            ldsColorConversion(p_para->colorMode, &p_para->currentHue, &p_para->currentSaturation, &p_para->currentX, &p_para->currentY, &p_para->colorTemperatureMireds);
#endif

            lds_light_control_state_t *state = lds_light_control_state_get();

            if (state->currentHue != p_para->currentHue)
            {
                state->currentHue = p_para->currentHue;
                state->transitionTime = 100;

                state->currentX              = p_para->currentX;
                state->currentY              = p_para->currentY;
                state->currentSaturation     = p_para->currentSaturation;
                state->currentColorTempMired = p_para->colorTemperatureMireds;

                ldsBulbDriverMinitrimCtrlMoveTo(state);
            }

            // lds_color_util_update_hue_saturation(1, p_para->currentHue, p_para->currentSaturation);
            // lds_color_util_update_xy(1, p_para->currentX, p_para->currentY);
            // lds_color_util_update_temp(1, p_para->colorTemperatureMireds);
            chip::app::MarkAttributeDirty markXDirty = chip::app::MarkAttributeDirty::kNo; 
            ColorControl::Attributes::ColorTemperatureMireds::Set(1, p_para->colorTemperatureMireds, markXDirty);
            ColorControl::Attributes::CurrentX::Set(1, p_para->currentX, markXDirty);
            ColorControl::Attributes::CurrentY::Set(1, p_para->currentY, markXDirty);

            if (store_cluster_para(p_para) != 0)
            {
                LDS_LOG_E("Fail store startup cluster para\n");
            }
        }
    }
}

static void UpdateCurrentSaturation(uint8_t * value)
{
    Protocols::InteractionModel::Status status;
    cluster_startup_para * p_para = &g_light_cluster_para;

    if (p_para->currentSaturation != *value)
    {
        p_para->currentSaturation = *value;

        uint8_t colorModeValue = 0;

        status = Clusters::ColorControl::Attributes::ColorMode::Get(1, &colorModeValue);
        if (status == Protocols::InteractionModel::Status::Success)
        {
            p_para->colorMode = colorModeValue;
        }

        if (p_para->colorMode == (uint8_t)Clusters::ColorControl::ColorMode::kCurrentHueAndCurrentSaturation)
        {
#ifdef EXTENDEDCOLOR_LIGHT
            uint8_t enhancedColorModeValue = 0;

            status = Clusters::ColorControl::Attributes::EnhancedColorMode::Get(1, &enhancedColorModeValue);
            if (status == Protocols::InteractionModel::Status::Success)
            {
                p_para->enhancedColorMode = enhancedColorModeValue;
            }

            ldsColorConversion(p_para->colorMode, &p_para->currentHue, &p_para->currentSaturation, &p_para->currentX, &p_para->currentY, &p_para->colorTemperatureMireds);
#endif

            lds_light_control_state_t *state = lds_light_control_state_get();

            if (state->currentSaturation != p_para->currentSaturation)
            {
                state->currentSaturation = p_para->currentSaturation;
                state->transitionTime = 100;

                state->currentX              = p_para->currentX;
                state->currentY              = p_para->currentY;
                state->currentHue            = p_para->currentHue;
                state->currentColorTempMired = p_para->colorTemperatureMireds;

                ldsBulbDriverMinitrimCtrlMoveTo(state);
            }

            // lds_color_util_update_hue_saturation(1, p_para->currentHue, p_para->currentSaturation);
            // lds_color_util_update_xy(1, p_para->currentX, p_para->currentY);
            // lds_color_util_update_temp(1, p_para->colorTemperatureMireds);

            chip::app::MarkAttributeDirty markXDirty = chip::app::MarkAttributeDirty::kNo; 
            ColorControl::Attributes::ColorTemperatureMireds::Set(1, p_para->colorTemperatureMireds, markXDirty);
            ColorControl::Attributes::CurrentX::Set(1, p_para->currentX, markXDirty);
            ColorControl::Attributes::CurrentY::Set(1, p_para->currentY, markXDirty);

            if (store_cluster_para(p_para) != 0)
            {
                LDS_LOG_E("Fail store startup cluster para\n");
            }
        }
    }
}

static void UpdateCurrentX(uint8_t * value)
{
    Protocols::InteractionModel::Status status;
    cluster_startup_para * p_para = &g_light_cluster_para;

    if (p_para->currentX != *reinterpret_cast<uint16_t *>(value))
    {
        p_para->currentX = *reinterpret_cast<uint16_t *>(value);

        uint8_t colorModeValue = 0;

        status = Clusters::ColorControl::Attributes::ColorMode::Get(1, &colorModeValue);
        if (status == Protocols::InteractionModel::Status::Success)
        {
            p_para->colorMode = colorModeValue;
        }

        if (p_para->colorMode == (uint8_t)Clusters::ColorControl::ColorMode::kCurrentXAndCurrentY)
        {
#ifdef EXTENDEDCOLOR_LIGHT
            uint8_t enhancedColorModeValue = 0;

            status = Clusters::ColorControl::Attributes::EnhancedColorMode::Get(1, &enhancedColorModeValue);
            if (status == Protocols::InteractionModel::Status::Success)
            {
                p_para->enhancedColorMode = enhancedColorModeValue;
            }

            ldsColorConversion(p_para->colorMode, &p_para->currentHue, &p_para->currentSaturation, &p_para->currentX, &p_para->currentY, &p_para->colorTemperatureMireds);
#endif

            lds_light_control_state_t *state = lds_light_control_state_get();

            if (state->currentX != p_para->currentX)
            {
                state->currentX = p_para->currentX;
                state->transitionTime = 100;

                state->currentSaturation     = p_para->currentSaturation;
                state->currentY              = p_para->currentY;
                state->currentHue            = p_para->currentHue;
                state->currentColorTempMired = p_para->colorTemperatureMireds;

                // ldsBulbDriverMinitrimCtrlMoveTo(state);
            }

            // lds_color_util_update_hue_saturation(1, p_para->currentHue, p_para->currentSaturation);
            // lds_color_util_update_xy(1, p_para->currentX, p_para->currentY);
            // lds_color_util_update_temp(1, p_para->colorTemperatureMireds);

            // chip::app::MarkAttributeDirty markXDirty = chip::app::MarkAttributeDirty::kNo; 
            // ColorControl::Attributes::ColorTemperatureMireds::Set(1, p_para->colorTemperatureMireds, markXDirty);
            // ColorControl::Attributes::CurrentHue::Set(1, p_para->currentHue, markXDirty);
            // ColorControl::Attributes::CurrentSaturation::Set(1, p_para->currentSaturation, markXDirty);

            if (store_cluster_para(p_para) != 0)
            {
                LDS_LOG_E("Fail store startup cluster para\n");
            }
        }
    }
}

static void UpdateCurrentY(uint8_t * value)
{
    Protocols::InteractionModel::Status status;
    cluster_startup_para * p_para = &g_light_cluster_para;

    if (p_para->currentY != *reinterpret_cast<uint16_t *>(value))
    {
        p_para->currentY = *reinterpret_cast<uint16_t *>(value);

        uint8_t colorModeValue = 0;

        status = Clusters::ColorControl::Attributes::ColorMode::Get(1, &colorModeValue);
        if (status == Protocols::InteractionModel::Status::Success)
        {
            p_para->colorMode = colorModeValue;
        }

        if (p_para->colorMode == (uint8_t)Clusters::ColorControl::ColorMode::kCurrentXAndCurrentY)
        {
#ifdef EXTENDEDCOLOR_LIGHT
            uint8_t enhancedColorModeValue = 0;

            status = Clusters::ColorControl::Attributes::EnhancedColorMode::Get(1, &enhancedColorModeValue);
            if (status == Protocols::InteractionModel::Status::Success)
            {
                p_para->enhancedColorMode = enhancedColorModeValue;
            }

            ldsColorConversion(p_para->colorMode, &p_para->currentHue, &p_para->currentSaturation, &p_para->currentX, &p_para->currentY, &p_para->colorTemperatureMireds);
#endif

            lds_light_control_state_t *state = lds_light_control_state_get();

            if (state->currentY != p_para->currentY)
            {
                state->currentY = p_para->currentY;
                state->transitionTime = 100;

                state->currentSaturation     = p_para->currentSaturation;
                state->currentX              = p_para->currentX;
                state->currentHue            = p_para->currentHue;
                state->currentColorTempMired = p_para->colorTemperatureMireds;

                // ldsBulbDriverMinitrimCtrlMoveTo(state);
            }

            // lds_color_util_update_hue_saturation(1, p_para->currentHue, p_para->currentSaturation);
            // lds_color_util_update_xy(1, p_para->currentX, p_para->currentY);
            // lds_color_util_update_temp(1, p_para->colorTemperatureMireds);
            // chip::app::MarkAttributeDirty markXDirty = chip::app::MarkAttributeDirty::kNo; 
            // ColorControl::Attributes::ColorTemperatureMireds::Set(1, p_para->colorTemperatureMireds, markXDirty);
            // ColorControl::Attributes::CurrentHue::Set(1, p_para->currentHue, markXDirty);
            // ColorControl::Attributes::CurrentSaturation::Set(1, p_para->currentSaturation, markXDirty);

            if (store_cluster_para(p_para) != 0)
            {
                LDS_LOG_E("Fail store startup cluster para\n");
            }
        }
    }
}

static void UpdateEnhancedCurrentHue(uint8_t * value)
{
    Protocols::InteractionModel::Status status;
    cluster_startup_para * p_para = &g_light_cluster_para;

    if (p_para->currentY != *reinterpret_cast<uint16_t *>(value))
    {
        p_para->currentY = *reinterpret_cast<uint16_t *>(value);

        uint8_t colorModeValue = 0;

        status = Clusters::ColorControl::Attributes::ColorMode::Get(1, &colorModeValue);
        if (status == Protocols::InteractionModel::Status::Success)
        {
            p_para->colorMode = colorModeValue;
        }

        if (p_para->colorMode == (uint8_t)Clusters::ColorControl::ColorMode::kCurrentHueAndCurrentSaturation)
        {
#ifdef EXTENDEDCOLOR_LIGHT
            uint8_t enhancedColorModeValue = 0;

            status = Clusters::ColorControl::Attributes::EnhancedColorMode::Get(1, &enhancedColorModeValue);
            if (status == Protocols::InteractionModel::Status::Success)
            {
                p_para->enhancedColorMode = enhancedColorModeValue;
            }

            ldsColorConversion(p_para->colorMode, &p_para->currentHue, &p_para->currentSaturation, &p_para->currentX, &p_para->currentY, &p_para->colorTemperatureMireds);
#endif

            lds_light_control_state_t *state = lds_light_control_state_get();

            if (state->currentHue != p_para->currentHue)
            {
                state->currentHue = p_para->currentHue;
                state->transitionTime = 100;

                state->currentSaturation     = p_para->currentSaturation;
                state->currentX              = p_para->currentX;
                state->currentSaturation            = p_para->currentSaturation;
                state->currentColorTempMired = p_para->colorTemperatureMireds;

                ldsBulbDriverMinitrimCtrlMoveTo(state);
            }

            // lds_color_util_update_hue_saturation(1, p_para->currentHue, p_para->currentSaturation);
            // lds_color_util_update_xy(1, p_para->currentX, p_para->currentY);
            // lds_color_util_update_temp(1, p_para->colorTemperatureMireds);
            chip::app::MarkAttributeDirty markXDirty = chip::app::MarkAttributeDirty::kNo; 
            ColorControl::Attributes::ColorTemperatureMireds::Set(1, p_para->colorTemperatureMireds, markXDirty);
            ColorControl::Attributes::CurrentX::Set(1, p_para->currentX, markXDirty);
            ColorControl::Attributes::CurrentY::Set(1, p_para->currentY, markXDirty);

            if (store_cluster_para(p_para) != 0)
            {
                LDS_LOG_E("Fail store startup cluster para\n");
            }
        }
    }
}

static void UpdateEnhancedColorMode(uint8_t * value)
{
    Protocols::InteractionModel::Status status;
    cluster_startup_para * p_para = &g_light_cluster_para;

    if (p_para->enhancedColorMode != *value)
    {
        p_para->enhancedColorMode = *value;

        if (store_cluster_para(p_para) != 0)
        {
            LDS_LOG_E("Fail store startup cluster para\n");
        }
    }
}
#endif

void MatterPostAttributeChangeCallback(const chip::app::ConcreteAttributePath & attributePath, uint8_t type, uint16_t size,
                                       uint8_t * value)
{
    ClusterId clusterId     = attributePath.mClusterId;
    AttributeId attributeId = attributePath.mAttributeId;

    switch (clusterId)
    {
        case OnOff::Id:
            switch (attributeId)
            {
                case OnOff::Attributes::OnOff::Id:
                    UpdateOnOff(value);
                    break;
                case OnOff::Attributes::StartUpOnOff::Id:
                    UpdateStartUpOnOff(value);
                    break;
                default:
                    break;
            }
            break;
        case LevelControl::Id:
            switch (attributeId)
            {
                case LevelControl::Attributes::CurrentLevel::Id:
                    UpdateCurrentLevel(value);
                    break;
                case LevelControl::Attributes::MinLevel::Id:
                    UpdateMinLevel(value);
                    break;
                case LevelControl::Attributes::MaxLevel::Id:
                    UpdateMaxLevel(value);
                    break;
                case LevelControl::Attributes::StartUpCurrentLevel::Id:
                    UpdateStartUpCurrentLevel(value);
                    break;
                default:
                    break;
            }
            break;
#if (defined COLORTEMPERATURE_LIGHT) || (defined EXTENDEDCOLOR_LIGHT)
        case ColorControl::Id:
            switch (attributeId)
            {
                case ColorControl::Attributes::ColorTemperatureMireds::Id:
                    UpdateColorTemperatureMireds(value);
                    break;
                case ColorControl::Attributes::ColorMode::Id:
                    UpdateColorMode(value);
                    break;
                case ColorControl::Attributes::StartUpColorTemperatureMireds::Id:
                    UpdateStartUpColorTemperatureMireds(value);
                    break;
#ifdef EXTENDEDCOLOR_LIGHT
                case ColorControl::Attributes::CurrentHue::Id:
                    UpdateCurrentHue(value);
                    break;
                case ColorControl::Attributes::CurrentSaturation::Id:
                    UpdateCurrentSaturation(value);
                    break;
                case ColorControl::Attributes::CurrentX::Id:
                    UpdateCurrentX(value);
                    break;
                case ColorControl::Attributes::CurrentY::Id:
                    UpdateCurrentY(value);
                    break;
                case ColorControl::Attributes::EnhancedCurrentHue::Id:
                    UpdateEnhancedCurrentHue(value);
                    break;
                case ColorControl::Attributes::EnhancedColorMode::Id:
                    UpdateEnhancedColorMode(value);
                    break;
#endif
                default:
                    break;
            }
            break;
#endif
        default:
            break;
    }
}

#else
void MatterPostAttributeChangeCallback(const chip::app::ConcreteAttributePath & attributePath, uint8_t type, uint16_t size,
                                       uint8_t * value)
{

    if(!GetAppTask().IsLightControlInitCompleted())
    {
        // LDS_LOG_E("GetAppTask().IsLightControlInitCompleted() is false");
        return;
    }
    /* user mode , add the customer code here for cb*/
    ClusterId clusterId     = attributePath.mClusterId;
    AttributeId attributeId = attributePath.mAttributeId;
    Protocols::InteractionModel::Status status;
    // printf("============MatterPostAttributeChangeCallback:clusterId:0x%x,attributeId:0x%x value:0x%x================\r\n",clusterId,attributeId,*value);

    if (clusterId == OnOff::Id )
    {         
        onoff_attribute_change_handle(attributeId,0xFF,0xFFFF,value);
    }else if (clusterId == LevelControl::Id)
    {
        level_attribute_change_handle(attributeId,0xFF,0xFFFF,value);
    }else if(clusterId == ColorControl::Id) {
        // color_attribute_change_handle(attributeId,0xFF,0xFFFF,value);

        uint8_t colorMode = 0x02;
        ColorControl::Attributes::ColorMode::Get(1, &colorMode);

#ifdef EXTENDEDCOLOR_LIGHT
        uint8_t syncHue = 0;
        uint8_t syncSaturation = 0;
#endif
        uint16_t syncX = 0;
        uint16_t syncY = 0;
        uint16_t syncColorTemp = 0;

        chip::app::MarkAttributeDirty markXDirty = chip::app::MarkAttributeDirty::kNo; 

        switch(attributeId)
        {
            case ColorControl::Attributes::CurrentX::Id:
            case ColorControl::Attributes::CurrentY::Id:
                if(colorMode == ColorControlServer::EnhancedColorMode::kCurrentXAndCurrentY) 
                {       
#ifdef COLORTEMPERATURE_LIGHT
                    ColorControl::Attributes::CurrentX::Get(1, &syncX);
                    ColorControl::Attributes::CurrentY::Get(1, &syncY);

                    ldsColorConversionColorTempLight(0x01, &syncX, &syncY, &syncColorTemp);

                    ColorControl::Attributes::ColorTemperatureMireds::Set(1, syncColorTemp, markXDirty);

                    lds_light_control_state_t *state = lds_light_control_state_get();

                    if ((state->currentX != syncX) || (state->currentY != syncY))
                    {
                        state->currentX              = syncX;
                        state->currentY              = syncY;
                        state->currentColorTempMired = syncColorTemp;
                        state->transitionTime        = 100;

                        ldsBulbDriverMinitrimCtrlMoveTo(state);
                    }
#endif
                }
                break;
#ifdef EXTENDEDCOLOR_LIGHT
            case ColorControl::Attributes::CurrentHue::Id:
            case ColorControl::Attributes::CurrentSaturation::Id:
                if(colorMode == ColorControlServer::EnhancedColorMode::kCurrentHueAndCurrentSaturation) 
                {
                    ColorControl::Attributes::CurrentHue::Get(1, &syncHue);
                    ColorControl::Attributes::CurrentSaturation::Get(1, &syncSaturation);
                    ldsColorConversion(0x00, &syncHue, &syncSaturation, &syncX, &syncY, &syncColorTemp);
                    ColorControl::Attributes::ColorTemperatureMireds::Set(1, syncColorTemp, markXDirty);
                    ColorControl::Attributes::CurrentX::Set(1, syncX, markXDirty);
                    ColorControl::Attributes::CurrentY::Set(1, syncY, markXDirty);
                    lds_light_control_set_currentX(syncX);
                    lds_light_control_set_currentY(syncY);
                    lds_light_control_set_currentColorTempMired(syncColorTemp);

                    lds_light_control_set_transitionTime(100);
                    color_attribute_change_handle(attributeId, type, size, value);
                }
                break;
#endif

            case ColorControl::Attributes::ColorTemperatureMireds::Id:
                if(colorMode == ColorControlServer::EnhancedColorMode::kColorTemperature) 
                {      
                    ColorControl::Attributes::ColorTemperatureMireds::Get(1, &syncColorTemp);
#ifdef COLORTEMPERATURE_LIGHT
                    ldsColorConversionColorTempLight(0x02, &syncX, &syncY, &syncColorTemp);
                    ColorControl::Attributes::CurrentX::Set(1, syncX, markXDirty);
                    ColorControl::Attributes::CurrentY::Set(1, syncY, markXDirty);

                    lds_light_control_set_currentX(syncX);
                    lds_light_control_set_currentY(syncY);
#endif

#ifdef EXTENDEDCOLOR_LIGHT
                    ldsColorConversion(0x02, &syncHue, &syncSaturation, &syncX, &syncY, &syncColorTemp);
                    ColorControl::Attributes::CurrentHue::Set(1, syncHue, markXDirty);
                    ColorControl::Attributes::CurrentSaturation::Set(1, syncSaturation, markXDirty);
                    ColorControl::Attributes::CurrentX::Set(1, syncX, markXDirty);
                    ColorControl::Attributes::CurrentY::Set(1, syncY, markXDirty);

                    lds_light_control_set_currentX(syncX);
                    lds_light_control_set_currentY(syncY);
                    lds_light_control_set_currentHue(syncHue);
                    lds_light_control_set_currentSaturation(syncSaturation);
#endif

                    lds_light_control_set_transitionTime(100);
                    color_attribute_change_handle(attributeId, type, size, value);
                }
                break;
    
            case ColorControl::Attributes::ColorMode::Id:
                color_attribute_change_handle(attributeId,0xFF,0xFFFF,value);
                break;
        }
    } 
    
   return ;
}

#ifdef EXTENDEDCOLOR_LIGHT
void ldsMatterAttrXyConversionReport(uint16_t currentX, uint16_t currentY)
{
    uint8_t syncHue = 0;
    uint8_t syncSaturation = 0;
    uint16_t syncColorTemp = 0;
    chip::app::MarkAttributeDirty markXDirty = chip::app::MarkAttributeDirty::kNo;

    ldsColorConversion(0x01, &syncHue, &syncSaturation, &currentX, &currentY, &syncColorTemp);
    ColorControl::Attributes::ColorTemperatureMireds::Set(1, syncColorTemp, markXDirty);
    ColorControl::Attributes::CurrentHue::Set(1, syncHue, markXDirty);
    ColorControl::Attributes::CurrentSaturation::Set(1, syncSaturation, markXDirty);
    ColorControl::Attributes::CurrentX::Set(1, currentX, markXDirty);
    ColorControl::Attributes::CurrentY::Set(1, currentY, markXDirty);

    lds_light_control_set_currentHue(syncHue);
    lds_light_control_set_currentSaturation(syncSaturation);
    lds_light_control_set_currentColorTempMired(syncColorTemp);
}
#endif

#endif
#else
using namespace chip;
using namespace chip::app::Clusters;

void MatterPostAttributeChangeCallback(const chip::app::ConcreteAttributePath & attributePath, uint8_t type, uint16_t size,
                                       uint8_t * value)
{
    static HsvColor_t hsv;
    static XyColor_t xy;
    ClusterId clusterId     = attributePath.mClusterId;
    AttributeId attributeId = attributePath.mAttributeId;

    if (clusterId == OnOff::Id && attributeId == OnOff::Attributes::OnOff::Id)
    {
        ChipLogDetail(Zcl, "Cluster OnOff: attribute OnOff set to %u", *value);
        GetAppTask().SetInitiateAction(*value ? AppTask::ON_ACTION : AppTask::OFF_ACTION,
                                       static_cast<int32_t>(AppEvent::kEventType_DeviceAction), value);
    }
    else if (clusterId == LevelControl::Id && attributeId == LevelControl::Attributes::CurrentLevel::Id)
    {
        if (GetAppTask().IsTurnedOn())
        {
            ChipLogDetail(Zcl, "Cluster LevelControl: attribute CurrentLevel set to %u", *value);
            GetAppTask().SetInitiateAction(AppTask::LEVEL_ACTION, static_cast<int32_t>(AppEvent::kEventType_DeviceAction), value);
        }
        else
        {
            ChipLogDetail(Zcl, "LED is off. Try to use move-to-level-with-on-off instead of move-to-level");
        }
    }
    else if (clusterId == ColorControl::Id)
    {
        /* Ignore several attributes that are currently not processed */
        if ((attributeId == ColorControl::Attributes::RemainingTime::Id) ||
            (attributeId == ColorControl::Attributes::EnhancedColorMode::Id) ||
            (attributeId == ColorControl::Attributes::ColorMode::Id))
        {
            return;
        }

        /* XY color space */
        if (attributeId == ColorControl::Attributes::CurrentX::Id || attributeId == ColorControl::Attributes::CurrentY::Id)
        {
            if (attributeId == ColorControl::Attributes::CurrentX::Id)
            {
                xy.x = *reinterpret_cast<uint16_t *>(value);
            }
            else if (attributeId == ColorControl::Attributes::CurrentY::Id)
            {
                xy.y = *reinterpret_cast<uint16_t *>(value);
            }

            ChipLogDetail(Zcl, "New XY color: %u|%u", xy.x, xy.y);
            GetAppTask().SetInitiateAction(AppTask::COLOR_ACTION_XY, static_cast<int32_t>(AppEvent::kEventType_DeviceAction),
                                           (uint8_t *) &xy);
        }
        /* HSV color space */
        else if (attributeId == ColorControl::Attributes::CurrentHue::Id ||
                 attributeId == ColorControl::Attributes::CurrentSaturation::Id ||
                 attributeId == ColorControl::Attributes::EnhancedCurrentHue::Id)
        {
            if (attributeId == ColorControl::Attributes::EnhancedCurrentHue::Id)
            {
                hsv.h = (uint8_t) (((*reinterpret_cast<uint16_t *>(value)) & 0xFF00) >> 8);
                hsv.s = (uint8_t) ((*reinterpret_cast<uint16_t *>(value)) & 0xFF);
            }
            else if (attributeId == ColorControl::Attributes::CurrentHue::Id)
            {
                hsv.h = *value;
            }
            else if (attributeId == ColorControl::Attributes::CurrentSaturation::Id)
            {
                hsv.s = *value;
            }
            ChipLogDetail(Zcl, "New HSV color: hue = %u| saturation = %u", hsv.h, hsv.s);
            GetAppTask().SetInitiateAction(AppTask::COLOR_ACTION_HSV, static_cast<int32_t>(AppEvent::kEventType_DeviceAction),
                                           (uint8_t *) &hsv);
        }
        /* Temperature Mireds color space */
        else if (attributeId == ColorControl::Attributes::ColorTemperatureMireds::Id)
        {
            ChipLogDetail(Zcl, "New Temperature Mireds color = %u", *(uint16_t *) value);
            GetAppTask().SetInitiateAction(AppTask::COLOR_ACTION_CT, static_cast<int32_t>(AppEvent::kEventType_DeviceAction),
                                           value);
        }
        else
        {
            ChipLogDetail(Zcl, "Ignore ColorControl attribute (%u) that is not currently processed!", attributeId);
        }
    }
}
#endif