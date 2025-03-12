/*
 * @Author: chenxiaoqian chenxiaoqian@leedarson.com
 * @Date: 2024-07-26 15:57:06
 * @LastEditors: chenxiaoqian chenxiaoqian@leedarson.com
 * @LastEditTime: 2024-08-21 16:01:07
 * @FilePath: /chenxiaoqian/connectedhomeip/examples/lighting-app/telink/src/lds_functions.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */



// in matter sdk
#include "lds_functions.h"
#include "AppTaskCommon.h"

//in lighting cbb
#include "lds_light_control.h"
#include "lds_attribute_types.h"
#include "lds_log.h"

#ifdef __cplusplus
extern "C" {
#endif

void ldsGetStateFromZigbee(lds_light_control_state_t * state ){
    light_para_t *p_para = &light_para;
    state->currentOnOff = p_para->onoff;
    state->currentLevel =  p_para->level;
    state->currentColorMode =  p_para->color_mode;
    switch (p_para->color_mode)
    {
        case EMBER_ZCL_COLOR_MODE_CURRENT_X_AND_CURRENT_Y:
            // Set CurrentX value
            state->currentX = p_para->currentx;

            // Set CurrentY value
           state->currentY = p_para->currenty;
            break;
        case EMBER_ZCL_COLOR_MODE_COLOR_TEMPERATURE:
            // Set ColorTemperatureMireds value
            state->currentColorTempMired = p_para->color_temp_mireds;
            break;
#ifdef EXTENDEDCOLOR_LIGHT
        case EMBER_ZCL_COLOR_MODE_CURRENT_HUE_AND_CURRENT_SATURATION:
             // Set EnhancedCurrentHue value
            state->currentEnhanceHue = p_para->enhanced_current_hue;

            // Set CurrentHue value
            state->currentHue = p_para->cur_hue;

            // Set CurrentSaturation value
            state->currentSaturation =  p_para->cur_saturation;
            break;
#endif      
        default:
            break;
    }
    
}

#ifdef __cplusplus
}
#endif