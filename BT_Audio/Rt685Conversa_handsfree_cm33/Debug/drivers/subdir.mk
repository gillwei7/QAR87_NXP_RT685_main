################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../drivers/fsl_cache.c \
../drivers/fsl_casper.c \
../drivers/fsl_clock.c \
../drivers/fsl_common.c \
../drivers/fsl_common_arm.c \
../drivers/fsl_crc.c \
../drivers/fsl_dma.c \
../drivers/fsl_dmic.c \
../drivers/fsl_dmic_dma.c \
../drivers/fsl_dsp.c \
../drivers/fsl_flexcomm.c \
../drivers/fsl_flexspi.c \
../drivers/fsl_gpio.c \
../drivers/fsl_hashcrypt.c \
../drivers/fsl_i2c.c \
../drivers/fsl_i2s.c \
../drivers/fsl_i2s_bridge.c \
../drivers/fsl_i2s_dma.c \
../drivers/fsl_i3c.c \
../drivers/fsl_inputmux.c \
../drivers/fsl_mu.c \
../drivers/fsl_power.c \
../drivers/fsl_reset.c \
../drivers/fsl_sctimer.c \
../drivers/fsl_sema42.c \
../drivers/fsl_trng.c \
../drivers/fsl_usart.c \
../drivers/fsl_usart_dma.c \
../drivers/fsl_usdhc.c 

C_DEPS += \
./drivers/fsl_cache.d \
./drivers/fsl_casper.d \
./drivers/fsl_clock.d \
./drivers/fsl_common.d \
./drivers/fsl_common_arm.d \
./drivers/fsl_crc.d \
./drivers/fsl_dma.d \
./drivers/fsl_dmic.d \
./drivers/fsl_dmic_dma.d \
./drivers/fsl_dsp.d \
./drivers/fsl_flexcomm.d \
./drivers/fsl_flexspi.d \
./drivers/fsl_gpio.d \
./drivers/fsl_hashcrypt.d \
./drivers/fsl_i2c.d \
./drivers/fsl_i2s.d \
./drivers/fsl_i2s_bridge.d \
./drivers/fsl_i2s_dma.d \
./drivers/fsl_i3c.d \
./drivers/fsl_inputmux.d \
./drivers/fsl_mu.d \
./drivers/fsl_power.d \
./drivers/fsl_reset.d \
./drivers/fsl_sctimer.d \
./drivers/fsl_sema42.d \
./drivers/fsl_trng.d \
./drivers/fsl_usart.d \
./drivers/fsl_usart_dma.d \
./drivers/fsl_usdhc.d 

OBJS += \
./drivers/fsl_cache.o \
./drivers/fsl_casper.o \
./drivers/fsl_clock.o \
./drivers/fsl_common.o \
./drivers/fsl_common_arm.o \
./drivers/fsl_crc.o \
./drivers/fsl_dma.o \
./drivers/fsl_dmic.o \
./drivers/fsl_dmic_dma.o \
./drivers/fsl_dsp.o \
./drivers/fsl_flexcomm.o \
./drivers/fsl_flexspi.o \
./drivers/fsl_gpio.o \
./drivers/fsl_hashcrypt.o \
./drivers/fsl_i2c.o \
./drivers/fsl_i2s.o \
./drivers/fsl_i2s_bridge.o \
./drivers/fsl_i2s_dma.o \
./drivers/fsl_i3c.o \
./drivers/fsl_inputmux.o \
./drivers/fsl_mu.o \
./drivers/fsl_power.o \
./drivers/fsl_reset.o \
./drivers/fsl_sctimer.o \
./drivers/fsl_sema42.o \
./drivers/fsl_trng.o \
./drivers/fsl_usart.o \
./drivers/fsl_usart_dma.o \
./drivers/fsl_usdhc.o 


# Each subdirectory must supply rules for building sources it contributes
drivers/%.o: ../drivers/%.c drivers/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=gnu99 -DCPU_MIMXRT685SFVKB -DUSB_DEVICE_INTERRUPT_PRIORITY=3 -DCPU_MIMXRT685SFVKB_cm33 -DCONFIG_BT_IND_DNLD -DMCUXPRESSO_SDK -DBOOT_HEADER_ENABLE=1 -DFSL_SDK_DRIVER_QUICK_ACCESS_ENABLE=1 -DLPUART_RING_BUFFER_SIZE=1024U -DPRINTF_ADVANCED_ENABLE=1 -DSDK_DEBUGCONSOLE_UART -DSDK_OS_FREE_RTOS -DSHELL_TASK_PRIORITY=configMAX_PRIORITIES-2 -DSHELL_TASK_STACK_SIZE=2048 -DSHELL_USE_COMMON_TASK=0 -DUSE_RTOS=1 -DAPPL_USE_STANDARD_IO -DFSL_DRIVER_TRANSFER_DOUBLE_WEAK_IRQ=0 -DFSL_FEATURE_FLASH_PAGE_SIZE_BYTES=4096 -DFSL_SDK_ENABLE_DRIVER_CACHE_CONTROL=1 -DEDGEFAST_BT_LITTLEFS_MFLASH -DGATT_CLIENT -DGATT_DB -DHAL_UART_ADAPTER_FIFO=1 -DIOT_WIFI_ENABLE_SAVE_NETWORK=1 -DLFS_NO_ERROR=1 -DLFS_NO_INTRINSICS=1 -DMBEDTLS_CONFIG_FILE='"mbedtls_config_client.h"' -DSDIO_ENABLED=1 -DgMemManagerLight=0 -DSDK_DEBUGCONSOLE=1 -DMCUX_META_BUILD -DMIMXRT685S_cm33_SERIES -DDEBUG_CONSOLE_TRANSFER_NON_BLOCKING=1 -DOSA_USED -DSERIAL_PORT_TYPE_UART=1 -DSDK_I2C_BASED_COMPONENT_USED=1 -DCODEC_MULTI_ADAPTERS=1 -DCODEC_WM8904_ENABLE -DSDK_I3C_BASED_COMPONENT_USED=1 -DWIFI_IW612_BOARD_MURATA_2EL_USD -DMFLASH_FILE_BASEADDR=7340032 -DLFS_THREADSAFE=1 -DCFG_CLASSIC -DLOG_ENABLE_ASYNC_MODE=1 -DLOG_MAX_ARGUMENT_COUNT=10 -DLOG_ENABLE_OVERWRITE=1 -DCONFIG_ARM=1 -DCR_INTEGER_PRINTF -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -D__NEWLIB__ -DUSB_STACK_USE_DEDICATED_RAM=1 -DMX25U12843G=1 -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\source" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_hifi4\binary" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\flash_config" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\CMSIS" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\CMSIS\m-profile" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\device" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\device\periph" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\drivers" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\utilities" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\component\lists" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\utilities\str" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\utilities\debug_console" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\component\log" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\component\serial_manager" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\codec" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\codec\port" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\codec\port\wm8904" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\component\common_task" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\component\gpio" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\component\i2c" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\component\uart" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\component\audio" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\component\osa" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\component\wifi_bt_module\AzureWave\tx_pwr_limits" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\component\wifi_bt_module\Murata\tx_pwr_limits" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\component\wifi_bt_module\u-blox\tx_pwr_limits" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\component\wifi_bt_module\incl" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\component\pmic\pca9420" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\flash\mflash" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\flash\mflash\mimxrt685" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\wifi" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\wifi\incl" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\wifi\incl\port\osa" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\wifi\port\osa" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\wifi\incl\port" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\wifi\incl\wifidriver" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\wifi\wifi_bt_firmware" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\wifi\wifidriver" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\wifi\wifidriver\incl" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\wifi\wifi_bt_firmware\IW416" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\wifi\wifi_bt_firmware\8987" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\wifi\wifi_bt_firmware\nw61x" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\wifi\wifi_bt_firmware\iw610" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\wifi\firmware_dnld" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\wifi\sdio_nxp_abs" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\wifi\sdio_nxp_abs\incl" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\wifi\fwdnld_intf_abs" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\mbedtls\include" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\mbedtls\library" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\mbedtls\port\ksdk" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\usb" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\fatfs\source" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\fatfs\source\fsl_usb_disk" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\littlefs" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\sdmmc\common" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\sdmmc\osa" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\sdmmc\sdio" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\sdmmc\host\usdhc" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\bt_ble\port\pal\mcux\toolspec\mcuxpresso" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\bt_ble\bluetooth\export\eOSAL" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\bt_ble\bluetooth\export\include" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\bt_ble\bluetooth\export\vendor" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\bt_ble\bluetooth\private\protocols\att" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\bt_ble\bluetooth\private\protocols\avctp" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\bt_ble\bluetooth\private\protocols\avdtp" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\bt_ble\bluetooth\private\protocols\bnep" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\bt_ble\bluetooth\private\protocols\dbase" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\bt_ble\bluetooth\private\protocols\hci_1.2" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\bt_ble\bluetooth\private\protocols\mcap" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\bt_ble\bluetooth\private\protocols\obex" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\bt_ble\bluetooth\private\protocols\rfcomm" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\bt_ble\bluetooth\private\protocols\sdp" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\bt_ble\bluetooth\private\protocols\sm" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\bt_ble\bluetooth\private\protocols\smp" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\bt_ble\bluetooth\private\utils\aes_cmac" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\bt_ble\bluetooth\private\utils\at_parser" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\bt_ble\bluetooth\private\utils\object_parser" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\bt_ble\bluetooth\private\utils\racp" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\bt_ble\bluetooth\private\utils\storage" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\bt_ble\bluetooth\private\utils\xml_parser" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\bt_ble\port\pal\mcux\bluetooth" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\bt_ble\port\pal\mcux\ethal" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\bt_ble\port\pal\mcux\sbc" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\bt_ble\port\osal\src\freertos" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\bt_ble\port\pal\mcux" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\bt_ble\bluetooth\private\utils\sbc" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\bt_ble\bluetooth\export\extension" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\bt_ble\bluetooth\private\lib\config" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\edgefast\bluetooth\source\impl\ethermind\host" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\edgefast\bluetooth\include" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\edgefast\bluetooth\source\impl\ethermind\crypto" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\bt_ble\source\impl\ethermind\platform" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\bt_ble\source\impl\ethermind" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\edgefast\bluetooth\source\impl\ethermind\platform\configs" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\edgefast\bluetooth\source\porting" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\edgefast\bluetooth\porting" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\bt_ble\port\pal\mcux\bluetooth\controller" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\edgefast\bluetooth\source\impl\ethermind\controller\configs\lwip" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\edgefast\bluetooth\source\impl\ethermind\controller\configs\wifi" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\freertos\freertos-kernel\include" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\freertos\freertos-kernel\portable\GCC\ARM_CM33_NTZ\non_secure" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\board" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\freertos\freertos-kernel\template" -I"D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\freertos\freertos-kernel\template\ARM_CM33_3_priority_bits" -O0 -fno-common -g3 -gdwarf-4 -mcpu=cortex-m33 -c -ffunction-sections -fdata-sections -fno-builtin -imacros "D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\source\edgefast_bluetooth_app.h" -imacros "D:\Git\NXP\RT685\Q_dev_board_BT_V1_3_1\nxp_rt685_bu8\BT_Audio\Rt685Conversa_handsfree_cm33\source\mcux_config.h" -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -D__NEWLIB__ -fstack-usage -specs=nano.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-drivers

clean-drivers:
	-$(RM) ./drivers/fsl_cache.d ./drivers/fsl_cache.o ./drivers/fsl_casper.d ./drivers/fsl_casper.o ./drivers/fsl_clock.d ./drivers/fsl_clock.o ./drivers/fsl_common.d ./drivers/fsl_common.o ./drivers/fsl_common_arm.d ./drivers/fsl_common_arm.o ./drivers/fsl_crc.d ./drivers/fsl_crc.o ./drivers/fsl_dma.d ./drivers/fsl_dma.o ./drivers/fsl_dmic.d ./drivers/fsl_dmic.o ./drivers/fsl_dmic_dma.d ./drivers/fsl_dmic_dma.o ./drivers/fsl_dsp.d ./drivers/fsl_dsp.o ./drivers/fsl_flexcomm.d ./drivers/fsl_flexcomm.o ./drivers/fsl_flexspi.d ./drivers/fsl_flexspi.o ./drivers/fsl_gpio.d ./drivers/fsl_gpio.o ./drivers/fsl_hashcrypt.d ./drivers/fsl_hashcrypt.o ./drivers/fsl_i2c.d ./drivers/fsl_i2c.o ./drivers/fsl_i2s.d ./drivers/fsl_i2s.o ./drivers/fsl_i2s_bridge.d ./drivers/fsl_i2s_bridge.o ./drivers/fsl_i2s_dma.d ./drivers/fsl_i2s_dma.o ./drivers/fsl_i3c.d ./drivers/fsl_i3c.o ./drivers/fsl_inputmux.d ./drivers/fsl_inputmux.o ./drivers/fsl_mu.d ./drivers/fsl_mu.o ./drivers/fsl_power.d ./drivers/fsl_power.o ./drivers/fsl_reset.d ./drivers/fsl_reset.o ./drivers/fsl_sctimer.d ./drivers/fsl_sctimer.o ./drivers/fsl_sema42.d ./drivers/fsl_sema42.o ./drivers/fsl_trng.d ./drivers/fsl_trng.o ./drivers/fsl_usart.d ./drivers/fsl_usart.o ./drivers/fsl_usart_dma.d ./drivers/fsl_usart_dma.o ./drivers/fsl_usdhc.d ./drivers/fsl_usdhc.o

.PHONY: clean-drivers

