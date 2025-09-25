################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include wifi/wifidriver/subdir.mk
-include wifi/wifi_bt_firmware/nw61x/subdir.mk
-include wifi/wifi_bt_firmware/iw610/subdir.mk
-include wifi/wifi_bt_firmware/IW416/subdir.mk
-include wifi/wifi_bt_firmware/8987/subdir.mk
-include wifi/sdio_nxp_abs/subdir.mk
-include wifi/port/osa/subdir.mk
-include wifi/fwdnld_intf_abs/subdir.mk
-include wifi/firmware_dnld/subdir.mk
-include utilities/str/subdir.mk
-include utilities/debug_console/subdir.mk
-include utilities/subdir.mk
-include usb/subdir.mk
-include startup/subdir.mk
-include source/subdir.mk
-include sdmmc/sdio/subdir.mk
-include sdmmc/osa/subdir.mk
-include sdmmc/host/usdhc/blocking/subdir.mk
-include sdmmc/common/subdir.mk
-include mbedtls/port/ksdk/subdir.mk
-include mbedtls/library/subdir.mk
-include littlefs/subdir.mk
-include freertos/freertos-kernel/portable/MemMang/subdir.mk
-include freertos/freertos-kernel/portable/GCC/ARM_CM33_NTZ/non_secure/subdir.mk
-include freertos/freertos-kernel/subdir.mk
-include flash_config/subdir.mk
-include flash/mflash/mimxrt685/subdir.mk
-include flash/mflash/subdir.mk
-include edgefast/bluetooth/source/porting/work/subdir.mk
-include edgefast/bluetooth/source/porting/thread/subdir.mk
-include edgefast/bluetooth/source/porting/slist/subdir.mk
-include edgefast/bluetooth/source/porting/slab/subdir.mk
-include edgefast/bluetooth/source/porting/sem/subdir.mk
-include edgefast/bluetooth/source/porting/queue/subdir.mk
-include edgefast/bluetooth/source/porting/poll/subdir.mk
-include edgefast/bluetooth/source/porting/net/subdir.mk
-include edgefast/bluetooth/source/porting/lifo/subdir.mk
-include edgefast/bluetooth/source/porting/fifo/subdir.mk
-include edgefast/bluetooth/source/porting/subdir.mk
-include edgefast/bluetooth/source/impl/ethermind/host/a2dp_codec/sbc/subdir.mk
-include edgefast/bluetooth/source/impl/ethermind/host/subdir.mk
-include edgefast/bluetooth/source/impl/ethermind/crypto/subdir.mk
-include edgefast/bluetooth/source/impl/ethermind/common/subdir.mk
-include edgefast/bluetooth/porting/subdir.mk
-include drivers/subdir.mk
-include device/subdir.mk
-include component/uart/subdir.mk
-include component/serial_manager/subdir.mk
-include component/pmic/pca9420/subdir.mk
-include component/osa/subdir.mk
-include component/log/subdir.mk
-include component/lists/subdir.mk
-include component/i2c/subdir.mk
-include component/gpio/subdir.mk
-include component/common_task/subdir.mk
-include component/audio/subdir.mk
-include codec/port/wm8904/subdir.mk
-include codec/port/subdir.mk
-include codec/subdir.mk
-include bt_ble/source/impl/ethermind/platform/subdir.mk
-include bt_ble/source/impl/ethermind/subdir.mk
-include bt_ble/port/pal/mcux/toolspec/mcuxpresso/subdir.mk
-include bt_ble/port/pal/mcux/ethal/subdir.mk
-include bt_ble/port/pal/mcux/bluetooth/controller/subdir.mk
-include bt_ble/port/pal/mcux/bluetooth/subdir.mk
-include bt_ble/port/pal/mcux/subdir.mk
-include bt_ble/port/osal/src/freertos/subdir.mk
-include bt_ble/bluetooth/export/vendor/subdir.mk
-include bt_ble/bluetooth/export/extension/subdir.mk
-include board/subdir.mk
ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

OPTIONAL_TOOL_DEPS := \
$(wildcard ../makefile.defs) \
$(wildcard ../makefile.init) \
$(wildcard ../makefile.targets) \


BUILD_ARTIFACT_NAME := Rt685Conversa_handsfree_cm33
BUILD_ARTIFACT_EXTENSION := axf
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all:
	+@$(MAKE) --no-print-directory main-build && $(MAKE) --no-print-directory post-build

# Main-build Target
main-build: Rt685Conversa_handsfree_cm33.axf

# Tool invocations
Rt685Conversa_handsfree_cm33.axf: $(OBJS) $(USER_OBJS) makefile $(OPTIONAL_TOOL_DEPS)
	@echo 'Building target: $@'
	@echo 'Invoking: MCU Linker'
	arm-none-eabi-gcc -nostdlib -L"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\bt_ble\bluetooth\private\lib" -Xlinker -no-warn-rwx-segments -Xlinker -Map="Rt685Conversa_handsfree_cm33.map" -Xlinker --gc-sections -Xlinker -print-memory-usage -Xlinker --sort-section=alignment -Xlinker --cref -mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -T Rt685Conversa_handsfree_cm33_Debug.ld -o "Rt685Conversa_handsfree_cm33.axf" $(OBJS) $(USER_OBJS) $(LIBS) -Wl,--start-group -lethermind_br_a2dp -lethermind_br_avrcp -lethermind_br_core -lethermind_br_hfp -lethermind_br_map -lethermind_br_pbap -lethermind_br_protocol -lethermind_br_spp -lethermind_br_util -lethermind_br_sbc_common -lethermind_br_sbc_decoder -lethermind_br_sbc_encoder -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) Rt685Conversa_handsfree_cm33.axf
	-@echo ' '

post-build:
	-@echo 'Performing post-build steps'
	-arm-none-eabi-size "Rt685Conversa_handsfree_cm33.axf"; # arm-none-eabi-objcopy -v -O binary "Rt685Conversa_handsfree_cm33.axf" "Rt685Conversa_handsfree_cm33.bin" ; # checksum -p MIMXRT685S -d "Rt685Conversa_handsfree_cm33.bin";
	-@echo ' '

.PHONY: all clean dependents main-build post-build

-include ../makefile.targets
