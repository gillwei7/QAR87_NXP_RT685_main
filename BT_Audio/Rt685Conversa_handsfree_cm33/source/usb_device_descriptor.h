/*
 * Copyright (c) 2015 - 2016, Freescale Semiconductor, Inc.
 * Copyright 2016 - 2017,2019 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __USB_DEVICE_DESCRIPTOR_H__
#define __USB_DEVICE_DESCRIPTOR_H__

/*******************************************************************************
* Definitions
******************************************************************************/
#include "GlobalDef.h"

#if EnableUsbComAndAudio==1


//#define HS_ISO_OUT_ENDP_INTERVAL (0x04)
//#define HS_ISO_IN_ENDP_INTERVAL  (0x04)

/*! @brief Whether USB Audio use syn mode or not. */

//NOTE: for this program: VCOM + USB audio + MSC,  should never disable USB_DEVICE_AUDIO_USE_SYNC_MODE, other wise one more EP is needed, thus exceeds the max EP number

#define USB_DEVICE_AUDIO_USE_SYNC_MODE (1U)

#define USB_DEVICE_SPECIFIC_BCD_VERSION (0x0200U)
#define USB_DEVICE_DEMO_BCD_VERSION     (0x0101U)

#define USB_DEVICE_MAX_POWER (0x32U)

/*! @brief Workaround for USB audio 2.0 supported by Windows OS. Please set 1 when meets the following conditions:
    1. device is full speed running audio 2.0
    2. usb host is Windows OS that supports USB audio 2.0, like Win 10
    3. use feedback endpoint
*/
#define USB_DEVICE_WORKAROUND_AUDIO_20_WINDOWS (0U)

/* Communication Class SubClass Codes */
#define USB_CDC_DIRECT_LINE_CONTROL_MODEL (0x01)
#define USB_CDC_ABSTRACT_CONTROL_MODEL (0x02)
#define USB_CDC_TELEPHONE_CONTROL_MODEL (0x03)
#define USB_CDC_MULTI_CHANNEL_CONTROL_MODEL (0x04)
#define USB_CDC_CAPI_CONTROL_MOPDEL (0x05)
#define USB_CDC_ETHERNET_NETWORKING_CONTROL_MODEL (0x06)
#define USB_CDC_ATM_NETWORKING_CONTROL_MODEL (0x07)
#define USB_CDC_WIRELESS_HANDSET_CONTROL_MODEL (0x08)
#define USB_CDC_DEVICE_MANAGEMENT (0x09)
#define USB_CDC_MOBILE_DIRECT_LINE_MODEL (0x0A)
#define USB_CDC_OBEX (0x0B)
#define USB_CDC_ETHERNET_EMULATION_MODEL (0x0C)

/* Communication Class Protocol Codes */
#define USB_CDC_NO_CLASS_SPECIFIC_PROTOCOL (0x00) /*also for Data Class Protocol Code */
#define USB_CDC_AT_250_PROTOCOL (0x01)
#define USB_CDC_AT_PCCA_101_PROTOCOL (0x02)
#define USB_CDC_AT_PCCA_101_ANNEX_O (0x03)
#define USB_CDC_AT_GSM_7_07 (0x04)
#define USB_CDC_AT_3GPP_27_007 (0x05)
#define USB_CDC_AT_TIA_CDMA (0x06)
#define USB_CDC_ETHERNET_EMULATION_PROTOCOL (0x07)
#define USB_CDC_EXTERNAL_PROTOCOL (0xFE)
#define USB_CDC_VENDOR_SPECIFIC (0xFF) /*also for Data Class Protocol Code */

/* Data Class Protocol Codes */
#define USB_CDC_PYHSICAL_INTERFACE_PROTOCOL (0x30)
#define USB_CDC_HDLC_PROTOCOL (0x31)
#define USB_CDC_TRANSPARENT_PROTOCOL (0x32)
#define USB_CDC_MANAGEMENT_PROTOCOL (0x50)
#define USB_CDC_DATA_LINK_Q931_PROTOCOL (0x51)
#define USB_CDC_DATA_LINK_Q921_PROTOCOL (0x52)
#define USB_CDC_DATA_COMPRESSION_V42BIS (0x90)
#define USB_CDC_EURO_ISDN_PROTOCOL (0x91)
#define USB_CDC_RATE_ADAPTION_ISDN_V24 (0x92)
#define USB_CDC_CAPI_COMMANDS (0x93)
#define USB_CDC_HOST_BASED_DRIVER (0xFD)
#define USB_CDC_UNIT_FUNCTIONAL (0xFE)

/* Descriptor SubType in Communications Class Functional Descriptors */
#define USB_CDC_HEADER_FUNC_DESC (0x00)
#define USB_CDC_CALL_MANAGEMENT_FUNC_DESC (0x01)
#define USB_CDC_ABSTRACT_CONTROL_FUNC_DESC (0x02)
#define USB_CDC_DIRECT_LINE_FUNC_DESC (0x03)
#define USB_CDC_TELEPHONE_RINGER_FUNC_DESC (0x04)
#define USB_CDC_TELEPHONE_REPORT_FUNC_DESC (0x05)
#define USB_CDC_UNION_FUNC_DESC (0x06)
#define USB_CDC_COUNTRY_SELECT_FUNC_DESC (0x07)
#define USB_CDC_TELEPHONE_MODES_FUNC_DESC (0x08)
#define USB_CDC_TERMINAL_FUNC_DESC (0x09)
#define USB_CDC_NETWORK_CHANNEL_FUNC_DESC (0x0A)
#define USB_CDC_PROTOCOL_UNIT_FUNC_DESC (0x0B)
#define USB_CDC_EXTENSION_UNIT_FUNC_DESC (0x0C)
#define USB_CDC_MULTI_CHANNEL_FUNC_DESC (0x0D)
#define USB_CDC_CAPI_CONTROL_FUNC_DESC (0x0E)
#define USB_CDC_ETHERNET_NETWORKING_FUNC_DESC (0x0F)
#define USB_CDC_ATM_NETWORKING_FUNC_DESC (0x10)
#define USB_CDC_WIRELESS_CONTROL_FUNC_DESC (0x11)
#define USB_CDC_MOBILE_DIRECT_LINE_FUNC_DESC (0x12)
#define USB_CDC_MDLM_DETAIL_FUNC_DESC (0x13)
#define USB_CDC_DEVICE_MANAGEMENT_FUNC_DESC (0x14)
#define USB_CDC_OBEX_FUNC_DESC (0x15)
#define USB_CDC_COMMAND_SET_FUNC_DESC (0x16)
#define USB_CDC_COMMAND_SET_DETAIL_FUNC_DESC (0x17)
#define USB_CDC_TELEPHONE_CONTROL_FUNC_DESC (0x18)
#define USB_CDC_OBEX_SERVICE_ID_FUNC_DESC (0x19)

/* usb descriptor length */
#define USB_DESCRIPTOR_LENGTH_CONFIGURATION_ALL (sizeof(g_UsbDeviceConfigurationDescriptor))
#define USB_ENDPOINT_AUDIO_DESCRIPTOR_LENGTH        (9)

#if USB_DEVICE_CONFIG_MSC==1
#define USB_MSC_DISK_REPORT_DESCRIPTOR_LENGTH (63)
#endif

#define USB_CDC_VCOM_REPORT_DESCRIPTOR_LENGTH (33)
#define USB_IAD_DESC_SIZE (8)
#define USB_DESCRIPTOR_LENGTH_CDC_HEADER_FUNC (5)
#define USB_DESCRIPTOR_LENGTH_CDC_CALL_MANAG (5)
#define USB_DESCRIPTOR_LENGTH_CDC_ABSTRACT (4)
#define USB_DESCRIPTOR_LENGTH_CDC_UNION_FUNC (5)

#if USB_DEVICE_CONFIG_MSC==1

#define USB_MSC_DISK_CLASS (0x08)
/* scsi command set */
#define USB_MSC_DISK_SUBCLASS (0x06)
/* bulk only transport protocol */
#define USB_MSC_DISK_PROTOCOL (0x50)

#define USB_MSC_DISK_INTERFACE_COUNT (1)
#define USB_MSC_DISK_INTERFACE_INDEX (5)
#define USB_MSC_DISK_ENDPOINT_COUNT (2)
//zzz --- MSC in EP3
#define USB_MSC_DISK_BULK_IN_ENDPOINT (3)
//zzz --- MSC out EP3
#define USB_MSC_DISK_BULK_OUT_ENDPOINT (3)

#define HS_MSC_DISK_BULK_IN_PACKET_SIZE (512)
#define FS_MSC_DISK_BULK_IN_PACKET_SIZE (64)
#define HS_MSC_DISK_BULK_OUT_PACKET_SIZE (512)
#define FS_MSC_DISK_BULK_OUT_PACKET_SIZE (64)

#endif


#define USB_AUDIO_CLASS_SPECIFIC_ENDPOINT_LENGTH    (8)
#define USB_AUDIO_CLOCK_SOURCE_LENGTH               (8)
#define USB_DESCRIPTOR_LENGTH_AC_INTERRUPT_ENDPOINT (9)
#if (USB_DEVICE_CONFIG_AUDIO_CLASS_2_0)
#define USB_AUDIO_CONTROL_INTERFACE_HEADER_LENGTH (9)
#else
#define USB_AUDIO_CONTROL_INTERFACE_HEADER_LENGTH (10)
#endif
#define USB_AUDIO_INPUT_TERMINAL_ONLY_DESC_SIZE            (12)
#define USB_AUDIO_OUTPUT_TERMINAL_ONLY_DESC_SIZE           (9)
#define USB_AUDIO_FEATURE_UNIT_ONLY_DESC_SIZE(ch, n)       (0x07 + (ch + 1) * n)
#define USB_AUDIO_STREAMING_IFACE_DESC_SIZE                (7)
#define USB_AUDIO_STREAMING_ENDP_DESC_SIZE                 (7)
#define USB_AUDIO_STANDARD_AS_ISO_FEEDBACK_ENDPOINT_LENGTH (7)
#define USB_AUDIO_STREAMING_TYPE_I_DESC_SIZE               (11)

/* Configuration, interface and endpoint. */
#define USB_DEVICE_CONFIGURATION_COUNT (1)
#define USB_DEVICE_STRING_COUNT        (7)
#define USB_DEVICE_LANGUAGE_COUNT      (1)
#define USB_DEVICE_INTERFACE_COUNT     (5)


#define USB_AUDIO_SPEAKER_CONFIGURE_INDEX         (1)
#define USB_COMPOSITE_CONFIGURE_INDEX             (1)
#define USB_AUDIO_CONTROL_INTERFACE_INDEX         (0)
#define USB_AUDIO_RECORDER_STREAM_INTERFACE_INDEX (1)
#define USB_AUDIO_SPEAKER_STREAM_INTERFACE_INDEX  (2)

#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
#define USB_AUDIO_SPEAKER_STREAM_ENDPOINT_COUNT (1)
#else
#define USB_AUDIO_SPEAKER_STREAM_ENDPOINT_COUNT (2)
#endif
#define USB_AUDIO_CONTROL_ENDPOINT_COUNT         (1)
#define USB_AUDIO_RECORDER_STREAM_ENDPOINT_COUNT (1)

//zzz --- audio spk out EP2
#define USB_AUDIO_SPEAKER_STREAM_ENDPOINT (2)
//zzz --- audio control in EP1
#define USB_AUDIO_CONTROL_ENDPOINT (1)
#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
#else
/*If multiple data endpoints are to be serviced by the same feedback endpoint, the data endpoints must have ascending
ordered-but not necessarily consecutive-endpoint numbers. The first data endpoint and the feedback endpoint must have 
the same endpoint number (and opposite direction). For more information, please refer to Universal Serial Bus Specification,
 Revision 2.0 chapter 9.6.6*/
#de fine USB_AUDIO_SPEAKER_FEEDBACK_ENDPOINT (2)
#endif
//zzz --- audio mic in EP2
#define USB_AUDIO_RECORDER_STREAM_ENDPOINT (2)



/* Configuration, interface and endpoint. */

#define USB_AUDIO_SPEAKER_CONTROL_INTERFACE_COUNT (1)
#define USB_AUDIO_SPEAKER_STREAM_INTERFACE_COUNT  (1)
#define USB_AUDIO_RECORDER_STREAM_INTERFACE_COUNT (1)
#define USB_AUDIO_SpkAndRec_INTERFACE_COUNT   (USB_AUDIO_SPEAKER_CONTROL_INTERFACE_COUNT + USB_AUDIO_SPEAKER_STREAM_INTERFACE_COUNT + USB_AUDIO_RECORDER_STREAM_INTERFACE_COUNT)

//#define AUDIO_OUT_SAMPLING_RATE_KHZ (48)
//#define AUDIO_IN_SAMPLING_RATE_KHZ  (16)

//#define AUDIO_SAMPLING_RATE_16KHZ (16)
/*reference macro for audio sample macro */
//#define AUDIO_IN_SAMPLING_RATE  (AUDIO_IN_SAMPLING_RATE_KHZ * 1000)
//#define AUDIO_OUT_SAMPLING_RATE (AUDIO_OUT_SAMPLING_RATE_KHZ * 1000)
/*if audio in and audio out sample rate are different, please use AUDIO_OUT_SAMPLING_RATE and AUDIO_IN_SAMPLING_RATE
to initialize out and in sample rate respectively*/
//#define AUDIO_SAMPLING_RATE (AUDIO_OUT_SAMPLING_RATE)

/* Audio data format */
//#define AUDIO_OUT_FORMAT_CHANNELS (0x02U)
//#define AUDIO_OUT_FORMAT_BITS     (16)
//#define AUDIO_OUT_FORMAT_SIZE     (0x02)
//#define AUDIO_IN_FORMAT_CHANNELS  (0x01U)
//#define AUDIO_IN_FORMAT_BITS      (16)
//#define AUDIO_IN_FORMAT_SIZE      (0x02)
/* Packet size and interval. */
#define HS_AUDIO_INTERRUPT_IN_PACKET_SIZE (8)
#define FS_AUDIO_INTERRUPT_IN_PACKET_SIZE (8)
#define HS_AUDIO_INTERRUPT_IN_INTERVAL    (0x07U) /* 2^(7-1) = 8ms */
#define FS_AUDIO_INTERRUPT_IN_INTERVAL    (0x08U)

//NOTE: UAC1.0, high speed: interval must be 4
//NOTE: UAC2.0, high speed: interval must can be 1~4
//interval value 1: 0.125 ms each packet
//interval value 2: 0.250 ms each packet
//interval value 3: 0.500 ms each packet
//interval value 4: 1.000 ms each packet

#define FS_ISO_OUT_ENDP_INTERVAL (0x01)
#define FS_ISO_IN_ENDP_INTERVAL  (0x01)


#if HS_ISO_OUT_ENDP_INTERVAL==1
	#define HS_ISO_OUT_ENDP_PACKET_SIZE (AUDIO_OUT_SAMPLING_RATE_KHZ * AUDIO_OUT_FORMAT_CHANNELS * AUDIO_OUT_FORMAT_SIZE/8)
#endif
#if HS_ISO_OUT_ENDP_INTERVAL==2
	#define HS_ISO_OUT_ENDP_PACKET_SIZE (AUDIO_OUT_SAMPLING_RATE_KHZ * AUDIO_OUT_FORMAT_CHANNELS * AUDIO_OUT_FORMAT_SIZE/4)
#endif
#if HS_ISO_OUT_ENDP_INTERVAL==3
	#define HS_ISO_OUT_ENDP_PACKET_SIZE (AUDIO_OUT_SAMPLING_RATE_KHZ * AUDIO_OUT_FORMAT_CHANNELS * AUDIO_OUT_FORMAT_SIZE/2)
#endif
#if HS_ISO_OUT_ENDP_INTERVAL==4
	#define HS_ISO_OUT_ENDP_PACKET_SIZE (AUDIO_OUT_SAMPLING_RATE_KHZ * AUDIO_OUT_FORMAT_CHANNELS * AUDIO_OUT_FORMAT_SIZE/1)
#endif

#if HS_ISO_IN_ENDP_INTERVAL==1
	#define HS_ISO_IN_ENDP_PACKET_SIZE  (AUDIO_IN_SAMPLING_RATE_KHZ * AUDIO_IN_FORMAT_CHANNELS * AUDIO_IN_FORMAT_SIZE/8)
#endif
#if HS_ISO_IN_ENDP_INTERVAL==2
	#define HS_ISO_IN_ENDP_PACKET_SIZE  (AUDIO_IN_SAMPLING_RATE_KHZ * AUDIO_IN_FORMAT_CHANNELS * AUDIO_IN_FORMAT_SIZE/4)
#endif
#if HS_ISO_IN_ENDP_INTERVAL==3
	#define HS_ISO_IN_ENDP_PACKET_SIZE  (AUDIO_IN_SAMPLING_RATE_KHZ * AUDIO_IN_FORMAT_CHANNELS * AUDIO_IN_FORMAT_SIZE/2)
#endif
#if HS_ISO_IN_ENDP_INTERVAL==4
	#define HS_ISO_IN_ENDP_PACKET_SIZE  (AUDIO_IN_SAMPLING_RATE_KHZ * AUDIO_IN_FORMAT_CHANNELS * AUDIO_IN_FORMAT_SIZE/1)
#endif



#define FS_ISO_OUT_ENDP_PACKET_SIZE (AUDIO_OUT_SAMPLING_RATE_KHZ * AUDIO_OUT_FORMAT_CHANNELS * AUDIO_OUT_FORMAT_SIZE)
#define FS_ISO_IN_ENDP_PACKET_SIZE  (AUDIO_IN_SAMPLING_RATE_KHZ * AUDIO_IN_FORMAT_CHANNELS * AUDIO_IN_FORMAT_SIZE)

#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
#else
#if (USB_DEVICE_CONFIG_AUDIO_CLASS_2_0)
#define HS_ISO_FEEDBACK_ENDP_PACKET_SIZE (4)
#if defined(USB_DEVICE_WORKAROUND_AUDIO_20_WINDOWS) && (USB_DEVICE_WORKAROUND_AUDIO_20_WINDOWS > 0)
#define FS_ISO_FEEDBACK_ENDP_PACKET_SIZE (4)
#else
#define FS_ISO_FEEDBACK_ENDP_PACKET_SIZE (3)
#endif
#else
#define HS_ISO_FEEDBACK_ENDP_PACKET_SIZE (3)
#define FS_ISO_FEEDBACK_ENDP_PACKET_SIZE (3)
#endif
#endif


#if ((!USB_DEVICE_CONFIG_AUDIO_CLASS_2_0) && (HS_ISO_OUT_ENDP_INTERVAL != 4))
#error "iso feedback endpoint interval must be 1 ms for usb audio 1.0"
#endif

/* Configuration, interface and endpoint. */
#define USB_CDC_VCOM_CIC_CLASS (0x02)
#define USB_CDC_VCOM_CIC_SUBCLASS (0x02)
#define USB_CDC_VCOM_CIC_PROTOCOL (0x00)
#define USB_CDC_VCOM_DIC_CLASS (0x0A)
#define USB_CDC_VCOM_DIC_SUBCLASS (0x00)
#define USB_CDC_VCOM_DIC_PROTOCOL (0x00)

#define USB_CDC_VCOM_INTERFACE_COUNT (2)
#define USB_AUDIO_MIDI_CONTROL_INTERFACE_INDEX (5)
#define USB_AUDIO_MIDI_STREAM_INTERFACE_INDEX (6)
#define USB_CDC_VCOM_CIC_INTERFACE_INDEX (3)
#define USB_CDC_VCOM_DIC_INTERFACE_INDEX (4)            //midi added
#define USB_CDC_VCOM_CIC_ENDPOINT_COUNT (1)
//zzz --- VCOM CIC intr in EP4
#define USB_CDC_VCOM_CIC_INTERRUPT_IN_ENDPOINT (4) 
#define USB_CDC_VCOM_DIC_ENDPOINT_COUNT (2)
//zzz --- VCOM data in EP5
#define USB_CDC_VCOM_DIC_BULK_IN_ENDPOINT (5)
//zzz --- VCOM data out EP5
#define USB_CDC_VCOM_DIC_BULK_OUT_ENDPOINT (5)

/* Packet size. */
#define HS_CDC_VCOM_INTERRUPT_IN_PACKET_SIZE (16)
#define FS_CDC_VCOM_INTERRUPT_IN_PACKET_SIZE (16)
#define HS_CDC_VCOM_INTERRUPT_IN_INTERVAL (0x07) /* 2^(7-1) = 8ms */
#define FS_CDC_VCOM_INTERRUPT_IN_INTERVAL (0x08)

#define HS_CDC_VCOM_BULK_IN_PACKET_SIZE (512)
#define FS_CDC_VCOM_BULK_IN_PACKET_SIZE (64)
#define HS_CDC_VCOM_BULK_OUT_PACKET_SIZE (512)
#define FS_CDC_VCOM_BULK_OUT_PACKET_SIZE (64)

/* String descriptor length. */
#define USB_DESCRIPTOR_LENGTH_STRING0 (sizeof(g_UsbDeviceString0))
#define USB_DESCRIPTOR_LENGTH_STRING1 (sizeof(g_UsbDeviceString1))
#define USB_DESCRIPTOR_LENGTH_STRING2 (sizeof(g_UsbDeviceString2))
#define USB_DESCRIPTOR_LENGTH_STRING3 (sizeof(g_UsbDeviceString3))
#define USB_DESCRIPTOR_LENGTH_STRING4 (sizeof(g_UsbDeviceString4))

#define USB_DESCRIPTOR_TYPE_CDC_CS_INTERFACE (0x24)
#define USB_DESCRIPTOR_TYPE_CDC_CS_ENDPOINT (0x25)

#if 0
	/* Class code. */
	#define USB_DEVICE_CLASS (0x00)
	#define USB_DEVICE_SUBCLASS (0x00)
	//zzz this one maybe try 0x00 ???
	#define USB_DEVICE_PROTOCOL (0x01)
#else
	//???
	/* Class code. */ //zzz--- 0xef, 0x02, 0x01 seems to be specail needed for composite device --- IAD dscr related
	#define USB_DEVICE_CLASS (0xef)
	#define USB_DEVICE_SUBCLASS (0x02)
	#define USB_DEVICE_PROTOCOL (0x01)
#endif

#define USB_AUDIO_CLASS           (0x01)
#define USB_SUBCLASS_AUDIOCONTROL (0x01)
#define USB_SUBCLASS_AUDIOSTREAM  (0x02)
#if (USB_DEVICE_CONFIG_AUDIO_CLASS_2_0)
#define USB_AUDIO_PROTOCOL (0x20)
#else
#define USB_AUDIO_PROTOCOL (0x00)
#endif

#define USB_AUDIO_FORMAT_TYPE_I                 (0x01)
#define USB_AUDIO_STREAM_ENDPOINT_DESCRIPTOR    (0x25)
#define USB_AUDIO_EP_GENERAL_DESCRIPTOR_SUBTYPE (0x01)

#if (USB_DEVICE_CONFIG_AUDIO_CLASS_2_0)
#define USB_AUDIO_RECORDER_CONTROL_CLOCK_SOURCE_ID (0x10)
#define USB_AUDIO_SPEAKER_CONTROL_CLOCK_SOURCE_ID  (0x20)
#endif

#define USB_AUDIO_RECORDER_CONTROL_INPUT_TERMINAL_ID  (0x01)
#define USB_AUDIO_RECORDER_CONTROL_FEATURE_UNIT_ID    (0x02)
#define USB_AUDIO_RECORDER_CONTROL_OUTPUT_TERMINAL_ID (0x03)

#define USB_AUDIO_SPEAKER_CONTROL_INPUT_TERMINAL_ID  (0x04)
#define USB_AUDIO_SPEAKER_CONTROL_FEATURE_UNIT_ID    (0x05)
#define USB_AUDIO_SPEAKER_CONTROL_OUTPUT_TERMINAL_ID (0x06)















/*******************************************************************************
 * API
 ******************************************************************************/
/*!
 * @brief USB device set speed function.
 *
 * This function sets the speed of the USB device.
 *
 * Due to the difference of HS and FS descriptors, the device descriptors and configurations need to be updated to match
 * current speed.
 * As the default, the device descriptors and configurations are configured by using FS parameters for both EHCI and
 * KHCI.
 * When the EHCI is enabled, the application needs to call this function to update device by using current speed.
 * The updated information includes endpoint max packet size, endpoint interval, etc.
 *
 * @param handle The USB device handle.
 * @param speed Speed type. USB_SPEED_HIGH/USB_SPEED_FULL/USB_SPEED_LOW.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
extern usb_status_t USB_DeviceSetSpeed(usb_device_handle handle, uint8_t speed);
/*!
 * @brief USB device get device descriptor function.
 *
 * This function gets the device descriptor of the USB device.
 *
 * @param handle The USB device handle.
 * @param deviceDescriptor The pointer to the device descriptor structure.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
usb_status_t USB_DeviceGetDeviceDescriptor(usb_device_handle handle,
                                           usb_device_get_device_descriptor_struct_t *deviceDescriptor);

/*!
 * @brief USB device get configuration descriptor function.
 *
 * This function gets the configuration descriptor of the USB device.
 *
 * @param handle The USB device handle.
 * @param configurationDescriptor The pointer to the configuration descriptor structure.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
usb_status_t USB_DeviceGetConfigurationDescriptor(
    usb_device_handle handle, usb_device_get_configuration_descriptor_struct_t *configurationDescriptor);

/*!
 * @brief USB device get string descriptor function.
 *
 * This function gets the string descriptor of the USB device.
 *
 * @param handle The USB device handle.
 * @param stringDescriptor Pointer to the string descriptor structure.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
usb_status_t USB_DeviceGetStringDescriptor(usb_device_handle handle,
                                           usb_device_get_string_descriptor_struct_t *stringDescriptor);

#if (defined(USB_DEVICE_CONFIG_CV_TEST) && (USB_DEVICE_CONFIG_CV_TEST > 0U))
/* Get device qualifier descriptor request */
usb_status_t USB_DeviceGetDeviceQualifierDescriptor(
    usb_device_handle handle, usb_device_get_device_qualifier_descriptor_struct_t *deviceQualifierDescriptor);
#endif

#endif

#endif /* _USB_DEVICE_DESCRIPTOR_H_ */
