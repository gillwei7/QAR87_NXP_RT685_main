/*
 * Copyright (c) 2015 - 2016, Freescale Semiconductor, Inc.
 * Copyright 2016 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __Main_H__
#define __Main_H__ 1

#if EnableUsbComAndAudio==1

#include "audio_unified.h"
#include "virtual_com.h"


/*******************************************************************************
* Definitions
******************************************************************************/
#if defined(USB_DEVICE_CONFIG_EHCI) && (USB_DEVICE_CONFIG_EHCI > 0U)
#define CONTROLLER_ID kUSB_ControllerEhci0
#endif
#if defined(USB_DEVICE_CONFIG_KHCI) && (USB_DEVICE_CONFIG_KHCI > 0U)
#define CONTROLLER_ID kUSB_ControllerKhci0
#endif
#if defined(USB_DEVICE_CONFIG_LPCIP3511FS) && (USB_DEVICE_CONFIG_LPCIP3511FS > 0U)
#define CONTROLLER_ID kUSB_ControllerLpcIp3511Fs0
#endif
#if defined(USB_DEVICE_CONFIG_LPCIP3511HS) && (USB_DEVICE_CONFIG_LPCIP3511HS > 0U)
#define CONTROLLER_ID kUSB_ControllerLpcIp3511Hs0
#endif

//#define USB_DEVICE_INTERRUPT_PRIORITY (3U)			//define this in project setting
#define AUDIO_SAMPLING_RATE_TO_10_14 (AUDIO_OUT_SAMPLING_RATE_KHZ << 10)
#define AUDIO_SAMPLING_RATE_TO_16_16 (AUDIO_OUT_SAMPLING_RATE_KHZ << 13)
#if (USB_DEVICE_CONFIG_AUDIO_CLASS_2_0)
#if defined(USB_DEVICE_WORKAROUND_AUDIO_20_WINDOWS) && (USB_DEVICE_WORKAROUND_AUDIO_20_WINDOWS > 0)
/* change 10.10 data to 10.14 or 16.16 (12.13) */
#define AUDIO_UPDATE_FEEDBACK_DATA(m, n)                  \
    {                                                     \
        m[0] = (((n & 0x000003FFu) << 3) & 0xFFu);        \
        m[1] = ((((n & 0x000003FFu) << 3) >> 8) & 0xFFu); \
        m[2] = (((n & 0x000FFC00u) >> 10) & 0xFFu);       \
        m[3] = (((n & 0x000FFC00u) >> 18) & 0xFFu);       \
    }
#else
/* change 10.10 data to 10.14 or 16.16 (12.13) */
#define AUDIO_UPDATE_FEEDBACK_DATA(m, n)                  \
    if (USB_SPEED_HIGH == g_composite.speed)              \
    {                                                     \
        m[0] = (((n & 0x000003FFu) << 3) & 0xFFu);        \
        m[1] = ((((n & 0x000003FFu) << 3) >> 8) & 0xFFu); \
        m[2] = (((n & 0x000FFC00u) >> 10) & 0xFFu);       \
        m[3] = (((n & 0x000FFC00u) >> 18) & 0xFFu);       \
    }                                                     \
    else                                                  \
    {                                                     \
        m[0] = ((n << 4) & 0xFFU);                        \
        m[1] = (((n << 4) >> 8U) & 0xFFU);                \
        m[2] = (((n << 4) >> 16U) & 0xFFU);               \
    }
#endif
#else
/* change 10.10 data to 10.14 */
#define AUDIO_UPDATE_FEEDBACK_DATA(m, n)    \
    {                                       \
        m[0] = ((n << 4) & 0xFFU);          \
        m[1] = (((n << 4) >> 8U) & 0xFFU);  \
        m[2] = (((n << 4) >> 16U) & 0xFFU); \
    }
#endif
typedef struct _usb_device_composite_struct
{
    usb_device_handle deviceHandle;
    usb_audio_composite_struct_t audioUnified;
    usb_cdc_vcom_struct_t cdcVcom; /* CDC virtual com device structure. */
    //usb_msc_struct_t mscDisk; /* MSC disk device structure.                                                        */

	#ifdef EnableMidiInTheDemo
		usb_audio_midi_struct_t midiDev;
	#endif

    uint8_t speed;
    uint8_t attach;
    uint8_t currentConfiguration;
    uint8_t currentInterfaceAlternateSetting[USB_DEVICE_INTERFACE_COUNT];
} usb_device_composite_struct_t;


extern float UsbDnStreamGainPrevValueL;
extern float UsbDnStreamGainPrevValueR;
extern float UsbDnStreamGainL;
extern float UsbDnStreamGainR;

/*******************************************************************************
* API
******************************************************************************/
/*!
 * @brief Audio class specific callback function.
 *
 * This function handles the Audio class specific requests.
 *
 * @param handle		  The Audio class handle.
 * @param event 		  The Audio class event type.
 * @param param 		  The parameter of the class specific request.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
extern usb_status_t USB_DeviceAudioCompositeCallback(class_handle_t handle, uint32_t event, void *param);
/*!
 * @brief Audio device set configuration function.
 *
 * This function sets configuration for Audio class.
 *
 * @param handle The Audio class handle.
 * @param configure The Audio class configure index.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
extern usb_status_t USB_DeviceAudioCompositeSetConfigure(class_handle_t handle, uint8_t configure);
/*!
 * @brief Audio device initialization function.
 *
 * This function initializes the device with the composite device class information.
 *
 * @param device_composite The pointer to the composite device structure.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
extern usb_status_t USB_DeviceAudioCompositeInit(usb_device_composite_struct_t *device_composite);
extern usb_status_t USB_DeviceAudioRecorderSetInterface(class_handle_t handle,
                                                        uint8_t interface,
                                                        uint8_t alternateSetting);
extern usb_status_t USB_DeviceAudioSpeakerSetInterface(class_handle_t handle,
                                                       uint8_t interface,
                                                       uint8_t alternateSetting);
/*!
 * @brief CDC class specific callback function.
 *
 * This function handles the CDC class specific requests.
 *
 * @param handle          The CDC ACM class handle.
 * @param event           The CDC ACM class event type.
 * @param param           The parameter of the class specific request.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
extern usb_status_t USB_DeviceCdcVcomCallback(class_handle_t handle, uint32_t event, void *param);
/*!
 * @brief Virtual COM device set configuration function.
 *
 * This function sets configuration for CDC class.
 *
 * @param handle The CDC ACM class handle.
 * @param configure The CDC ACM class configure index.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
extern usb_status_t USB_DeviceCdcVcomSetConfigure(class_handle_t handle, uint8_t configure);
/*!
 * @brief Virtual COM device initialization function.
 *
 * This function initializes the device with the composite device class information.
 *
 * @param deviceComposite The pointer to the composite device structure.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
extern usb_status_t USB_DeviceCdcVcomInit(usb_device_composite_struct_t *deviceComposite);
/*!
 * @brief Application task function.
 *
 * This function runs the task for application.
 *
 * @return None.
 */
extern void USB_DeviceCdcVcomTask(void);

/*!
 * @brief MSC class specific callback function.
 *
 * This function handles the MSCclass specific requests.
 *
 * @param handle          The MSC class handle.
 * @param event           The MSC class event type.
 * @param param           The parameter of the class specific request.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
extern usb_status_t USB_DeviceMscCallback(class_handle_t handle, uint32_t event, void *param);
/*!
 * @brief MSC disk device set configuration function.
 *
 * This function sets configuration for MSC class.
 *
 * @param handle The MSC class handle.
 * @param configure The MSC class configure index.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
extern usb_status_t USB_DeviceMscDiskSetConfigure(class_handle_t handle, uint8_t configure);
/*!
 * @brief MSC disk device initialization function.
 *
 * This function initializes the device with the composite device class information.
 *
 * @param deviceComposite The pointer to the composite device structure.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
extern usb_status_t USB_DeviceMscDiskInit(usb_device_composite_struct_t *deviceComposite);

extern usb_status_t USB_DeviceAudioCompositeMidiCallback(class_handle_t handle, uint32_t event, void *param);
extern usb_status_t USB_DeviceMidiInit(usb_device_composite_struct_t *device_composite);

extern void UsbAppInit(void);

#endif
#endif
