################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_UPPER_SRCS += \
F:/telink/x-zigbee-sdk/telink_zigbee_ble_concurrent_sdk_20241217/platform/boot/b92/cstartup_b92.S 

OBJS += \
./platform/boot/b92/cstartup_b92.o 

S_UPPER_DEPS += \
./platform/boot/b92/cstartup_b92.d 


# Each subdirectory must supply rules for building sources it contributes
platform/boot/b92/cstartup_b92.o: F:/telink/x-zigbee-sdk/telink_zigbee_ble_concurrent_sdk_20241217/platform/boot/b92/cstartup_b92.S platform/boot/b92/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GNU RISC-V Cross Assembler'
	riscv32-elf-gcc -mcmodel=medium -mcmodel=medium -O2 -fmessage-length=0 -ffunction-sections -fdata-sections -flto -Wall -Wno-nonnull-compare -x assembler-with-cpp -DMCU_CORE_B92=1 -DROUTER=1 -D__PROJECT_TL_LIGHT__=1 -I../../../apps/common -I../../../stack/ble -I../../../apps/sampleLight_concurrent -I../../../platform/chip_b92 -I../../../platform/riscv -I../../../platform -I../../../proj/common -I../../../proj -I../../../stack/zigbee/common/includes -I../../../stack/zigbee/zbapi -I../../../stack/zigbee/bdb/includes -I../../../stack/zigbee/gp -I../../../stack/zigbee/zcl -I../../../stack/zigbee/ota -I../../../stack/zigbee/wwah -I../../../ -I../../../zbhci -c -fmessage-length=0 -fno-builtin -fomit-frame-pointer -fno-strict-aliasing -fuse-ld=bfd -fno-jump-tables -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


