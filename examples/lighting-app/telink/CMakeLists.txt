#
#    Copyright (c) 2022-2024 Project CHIP Authors
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#        http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#

cmake_minimum_required(VERSION 3.13.1)

get_filename_component(CHIP_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/third_party/connectedhomeip REALPATH)
get_filename_component(TELINK_COMMON ${CHIP_ROOT}/examples/platform/telink REALPATH)
get_filename_component(GEN_DIR ${CHIP_ROOT}/zzz_generated/ REALPATH)
get_filename_component(CBB  ${CHIP_ROOT}/third_party/lighting_cbb REALPATH)



include(${TELINK_COMMON}/common.cmake)
include(${CHIP_ROOT}/config/telink/app/enable-gnu-std.cmake)
include(${CHIP_ROOT}/src/app/chip_data_model.cmake)

project(chip-telink-lighting-example)

target_include_directories(app PRIVATE
                           include
                           ${GEN_DIR}/app-common
                           ${GEN_DIR}/lighting-app
                           ${TELINK_COMMON}/common/include
                           ${TELINK_COMMON}/util/include
                           ${TELINK_COMMON}/app/include
                           ${TELINK_COMMON}/zephyr_ext
                           ${CBB}/bsp/common
                           ${CBB}/bsp/telink/common
                           ${CBB}/bsp/telink/hal/adc
                           ${CBB}/bsp/telink/hal/pwm
                           ${CBB}/bsp/telink/hal/i2c  #RGBTW
                           ${CBB}/bsp/telink/hal/flash
                           ${CBB}/bsp/telink/osal
                           ${CBB}/components/lds_cluster_utility/inc
                           ${CBB}/components/lds_color_algorithm/inc
                           ${CBB}/components/lds_color_conversion/inc
                           ${CBB}/components/lds_dim_algorithm/inc
                           ${CBB}/components/lds_driver_minitrim/inc
                           ${CBB}/components/lds_light_cct_algorithm/inc
                           ${CBB}/components/lds_light_control/inc
                           ${CBB}/components/lds_light_effect/inc
                           ${CBB}/components/lds_light_trf/inc
                           ${CBB}/components/lds_log/inc
                           ${CBB}/components/lds_osal/inc
                           ${CBB}/components/lds_power_on_count/inc
                           ${CBB}/driver/kp18057driver
                           ${CBB}/driver/kp18056driver
                           ${CBB}/toolbox/include
                           )

target_sources(app PRIVATE
               src/AppTask.cpp
               src/ZclCallbacks.cpp
               src/lds_functions.cpp
               src/lds_mfg_token_manage.cpp
               ${TELINK_COMMON}/common/src/mainCommon.cpp
               ${TELINK_COMMON}/common/src/AppTaskCommon.cpp
               ${TELINK_COMMON}/util/src/LEDManager.cpp
               ${TELINK_COMMON}/util/src/ButtonManager.cpp
               ${TELINK_COMMON}/util/src/ThreadUtil.cpp
               ${TELINK_COMMON}/util/src/PWMManager.cpp
               ${TELINK_COMMON}/util/src/ColorFormat.cpp
               ${TELINK_COMMON}/zephyr_ext/zephyr_key_matrix.c
               ${TELINK_COMMON}/zephyr_ext/zephyr_key_pool.c
               ${TELINK_COMMON}/zephyr_ext/zephyr_led_pool.c
               ${TELINK_COMMON}/zephyr_ext/zephyr_pwm_pool.c
               ${TELINK_COMMON}/zephyr_ext/zephyr_ws2812.c
                )

if(CONFIG_DeviceType_DimmableLight)
  chip_configure_data_model(app
    INCLUDE_SERVER
    ZAP_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../lighting-common/dimmablelight-lighting-app.zap
  )
endif()

if(CONFIG_DeviceType_ColorTemperatureLight)
add_definitions(-DTELINK_DEVICE 
                -DTELINK_DEVICE_MATTER
                -DCOLORTEMPERATURE_LIGHT 
                )

chip_configure_data_model(app
    INCLUDE_SERVER
    ZAP_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../lighting-common/colortemperaturelight-lighting-app.zap
)
endif()

if(CONFIG_DeviceType_ExtendedColorLight)
add_definitions(-DTELINK_DEVICE 
                -DTELINK_DEVICE_MATTER
                -DEXTENDEDCOLOR_LIGHT 
                -DRGBTW  
                )

chip_configure_data_model(app
    INCLUDE_SERVER
    ZAP_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../lighting-common/extendedcolorlight-lighting-app.zap
)
endif()

if(CONFIG_BOOTLOADER_MCUBOOT)
  target_sources(app PRIVATE ${TELINK_COMMON}/util/src/OTAUtil.cpp)
endif()

if (CONFIG_CHIP_PW_RPC)
  include(${TELINK_COMMON}/Rpc.cmake)
endif(CONFIG_CHIP_PW_RPC)

set(CBB_LIB_PATH ${CBB}/lighting_cbb_lib.a)
target_link_libraries(app PRIVATE ${CBB_LIB_PATH})
