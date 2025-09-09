#include "usb_device_config.h"
#include "usb.h"
#include "usb_device.h"

#include "usb_device_class.h"
#if ((defined(USB_DEVICE_CONFIG_AUDIO_MIDI)) && (USB_DEVICE_CONFIG_AUDIO_MIDI > 0U))
#include "usb_device_midi.h"
//#include "usb_device_audio.h"

uint8_t recvbuf[64];
/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
static usb_status_t USB_DeviceAudioMIDIStreamingAllocateHandle(usb_device_audio_midi_struct_t **handle);
static usb_status_t USB_DeviceAudioMIDIStreamingFreeHandle(usb_device_audio_midi_struct_t *handle);
usb_status_t USB_DeviceAudioMIDIStreamingEndpointsInit(usb_device_audio_midi_struct_t *audioHandle);
usb_status_t USB_DeviceAudioMIDIStreamingEndpointsDeinit(usb_device_audio_midi_struct_t *audioHandle);

/*******************************************************************************
 * Variables
 ******************************************************************************/

USB_GLOBAL USB_RAM_ADDRESS_ALIGNMENT(USB_DATA_ALIGN_SIZE) static usb_device_audio_midi_struct_t
    s_UsbDeviceAudioMidiHandle[USB_DEVICE_CONFIG_AUDIO_MIDI];

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief Allocate a device audio class handle.
 *
 * This function allocates a device audio class handle.
 *
 * @param handle          It is out parameter, is used to return pointer of the device audio class handle to the caller.
 *
 * @retval kStatus_USB_Success              Get a device audio class handle successfully.
 * @retval kStatus_USB_Busy                 Cannot allocate a device audio class handle.
 */
static usb_status_t USB_DeviceAudioMIDIStreamingAllocateHandle(usb_device_audio_midi_struct_t **handle)
{
    int32_t count;
    OSA_SR_ALLOC();

    OSA_ENTER_CRITICAL();
    for (count = 0; count < USB_DEVICE_CONFIG_AUDIO_MIDI; count++)
    {
        if (NULL == s_UsbDeviceAudioMidiHandle[count].handle)
        {
            *handle = &s_UsbDeviceAudioMidiHandle[count];
            OSA_EXIT_CRITICAL();
            return kStatus_USB_Success;
        }
    }
    OSA_EXIT_CRITICAL();
    return kStatus_USB_Busy;
}

/*!
 * @brief Free a device audio class handle.
 *
 * This function frees a device audio class handle.
 *
 * @param handle          The device audio class handle.
 *
 * @retval kStatus_USB_Success              Free device audio class handle successfully.
 */
static usb_status_t USB_DeviceAudioMIDIStreamingFreeHandle(usb_device_audio_midi_struct_t *handle)
{
    OSA_SR_ALLOC();

    OSA_ENTER_CRITICAL();
    handle->handle = NULL;
    handle->configStruct = (usb_device_class_config_struct_t *)NULL;
    handle->configuration = 0U;
    OSA_EXIT_CRITICAL();
    return kStatus_USB_Success;
}


usb_status_t USB_DeviceAudioMIDIStreamingInit(uint8_t controllerId, usb_device_class_config_struct_t *config, class_handle_t *handle)
{
    usb_device_audio_midi_struct_t *audioHandle;
    usb_status_t error = kStatus_USB_Error;

    /* Allocate a audio class handle. */
    error = USB_DeviceAudioMIDIStreamingAllocateHandle(&audioHandle);

    if (kStatus_USB_Success != error)
    {
        return error;
    }

    /* Get the device handle according to the controller id. */
    error = USB_DeviceClassGetDeviceHandle(controllerId, &audioHandle->handle);

    if (kStatus_USB_Success != error)
    {
        return error;
    }

    if (!audioHandle->handle)
    {
        return kStatus_USB_InvalidHandle;
    }
    /* Save the configuration of the class. */
    audioHandle->configStruct = config;
    /* Clear the configuration value. */
    audioHandle->configuration = 0U;
    audioHandle->midiStreamingAlternate =0xffU;

    *handle = (class_handle_t)audioHandle;
    return error;
}

/*!
 * @brief De-initialize the device audio class.
 *
 * The function de-initializes the device audio class.
 *
 * @param handle The ccid class handle got from usb_device_class_config_struct_t::classHandle.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
usb_status_t USB_DeviceAudioMIDIStreamingDeinit(class_handle_t handle)
{
    usb_device_audio_midi_struct_t *audioHandle;
    usb_status_t error = kStatus_USB_Error;

    audioHandle = (usb_device_audio_midi_struct_t *)handle;

    if (!audioHandle)
    {
        return kStatus_USB_InvalidHandle;
    }
    error = USB_DeviceAudioMIDIStreamingEndpointsDeinit(audioHandle);
    USB_DeviceAudioMIDIStreamingFreeHandle(audioHandle);
    return error;
}


usb_status_t USB_DeviceAudioMIDIStreamingOut(usb_device_handle handle,
                                           usb_device_endpoint_callback_message_struct_t *message,
                                           void *callbackParam){
    usb_device_audio_midi_struct_t *audioHandle;
    audioHandle = (usb_device_audio_midi_struct_t *)callbackParam;
    if (!audioHandle)
    {
        return kStatus_USB_InvalidHandle;
    }
    usb_status_t error = kStatus_USB_Success;
    if ((NULL != audioHandle->configStruct) && (audioHandle->configStruct->classCallback))
    {
    	//Call USB_DeviceAudioCallback
        error = audioHandle->configStruct->classCallback((class_handle_t)audioHandle,
                                                         kUSB_DeviceAudioEventMIDIRecv,
														 message);
    }
    return error;
}

usb_status_t USB_DeviceAudioMIDIStreamingIn(usb_device_handle handle,
                                           usb_device_endpoint_callback_message_struct_t *message,
                                           void *callbackParam){
    usb_device_audio_midi_struct_t *audioHandle;
    audioHandle = (usb_device_audio_midi_struct_t *)callbackParam;
    if (!audioHandle)
    {
        return kStatus_USB_InvalidHandle;
    }
    usb_status_t error = kStatus_USB_Success;
    if ((NULL != audioHandle->configStruct) && (audioHandle->configStruct->classCallback))
    {
    	//Call USB_DeviceAudioCallback
        error = audioHandle->configStruct->classCallback((class_handle_t)audioHandle,
                                                         kUSB_DeviceAudioEventMIDISend,
														 message);
    }
    return error;
}


usb_status_t USB_DeviceAudioMIDIStreamingEndpointsInit(usb_device_audio_midi_struct_t *audioHandle)
{
    usb_device_interface_list_t *interfaceList;
    usb_device_interface_struct_t *interface = (usb_device_interface_struct_t *)NULL;
    usb_status_t error = kStatus_USB_Error;

    /* Check the configuration is valid or not. */
    if (!audioHandle->configuration)
    {
        return error;
    }

    /* Check the configuration is valid or not. */
    if (audioHandle->configuration > audioHandle->configStruct->classInfomation->configurations)
    {
        return error;
    }

    if (NULL == audioHandle->configStruct->classInfomation->interfaceList)
    {
        return error;
    }

    /* Get the interface list of the new configuration. */
    interfaceList = &audioHandle->configStruct->classInfomation->interfaceList[audioHandle->configuration - 1];

    for (int count = 0U; count < interfaceList->count; count++)
    {
        if ((USB_DEVICE_CONFIG_AUDIO_CLASS_CODE == interfaceList->interfaces[count].classCode) &&
            (0x03 == interfaceList->interfaces[count].subclassCode))
        {
            interface = &interfaceList->interfaces[count].interface[0];
            audioHandle->midiStreamingInterfaceNumber = interfaceList->interfaces[count].interfaceNumber;
            break;
        }
    }
    if (!interface)
    {
        return error;
    }
    /* Keep new stream interface handle. */
    audioHandle->midistereamingInterfaceHandle = interface;

    /* Initialize the endpoints of the new interface. */
    for (int count = 0U; count < interface->endpointList.count; count++)
    {
        usb_device_endpoint_init_struct_t epInitStruct;
        usb_device_endpoint_callback_struct_t epCallback;
        epInitStruct.zlt = 0U;
        epInitStruct.endpointAddress = interface->endpointList.endpoint[count].endpointAddress;
        epInitStruct.maxPacketSize = interface->endpointList.endpoint[count].maxPacketSize;
        epInitStruct.transferType = interface->endpointList.endpoint[count].transferType;
        if (USB_ENDPOINT_BULK == epInitStruct.transferType){
        	if(epInitStruct.endpointAddress & 0x80){
        		epCallback.callbackFn = USB_DeviceAudioMIDIStreamingIn;
        	}else{
        		epCallback.callbackFn = USB_DeviceAudioMIDIStreamingOut;
        	}
        }
        epCallback.callbackParam = audioHandle;

        error = USB_DeviceInitEndpoint(audioHandle->handle, &epInitStruct, &epCallback);
    }
    return error;
}

usb_status_t USB_DeviceAudioMIDIStreamingEndpointsDeinit(usb_device_audio_midi_struct_t *audioHandle)
{
    usb_status_t error = kStatus_USB_Error;
    //usb_device_endpoint_callback_message_struct_t message;

    if (!audioHandle->midistereamingInterfaceHandle)
    {
        return error;
    }
    /* De-initialize all stream endpoints of the interface */
    for (int count = 0U; count < audioHandle->midistereamingInterfaceHandle->endpointList.count; count++)
    {
        error = USB_DeviceDeinitEndpoint(
            audioHandle->handle, audioHandle->midistereamingInterfaceHandle->endpointList.endpoint[count].endpointAddress);
    }

    for (int count = 0U; count < audioHandle->midistereamingInterfaceHandle->endpointList.count; count++)
    {
        if ((audioHandle->midistereamingInterfaceHandle->endpointList.endpoint[count].endpointAddress &
             USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_MASK) >>
                USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT ==
            USB_IN)
        {

        }
        else
        {

        }
    }

    audioHandle->midistereamingInterfaceHandle = NULL;
    return error;
}



/*!
 * @brief De-initialize the stream endpoints of the audio class.
 *
 * This callback function is used to de-initialize the stream endpoints of the audio class.
 *
 * @param audioHandle          The device audio class handle. It equals the value returned from
 * usb_device_class_config_struct_t::classHandle.
 *
 * @return A USB error code or kStatus_USB_Success.
 */




/*!
 * @brief Handle the event passed to the audio class.
 *
 * This function handles the event passed to the audio class.
 *
 * @param handle          The audio class handle, got from the usb_device_class_config_struct_t::classHandle.
 * @param event           The event codes. Please refer to the enumeration usb_device_class_event_t.
 * @param param           The param type is determined by the event code.
 *
 * @return A USB error code or kStatus_USB_Success.
 * @retval kStatus_USB_Success              Free device handle successfully.
 * @retval kStatus_USB_InvalidParameter     The device handle not be found.
 * @retval kStatus_USB_InvalidRequest       The request is invalid, and the control pipe will be stalled by the caller.
 */
usb_status_t USB_DeviceAudioMIDIStreamingEvent(void *handle, uint32_t event, void *param)
{
    usb_device_audio_midi_struct_t *audioHandle;
    usb_status_t error = kStatus_USB_Error;
    uint16_t interfaceAlternate;
    uint8_t *temp8;
    uint8_t alternate;

    if ((!param) || (!handle))
    {
        return kStatus_USB_InvalidHandle;
    }

    /* Get the audio class handle. */
    audioHandle = (usb_device_audio_midi_struct_t *)handle;

    switch (event)
    {
        case kUSB_DeviceClassEventDeviceReset:
            //usb_echo("kUSB_DeviceClassEventDeviceReset \r\n");
            /* Bus reset, clear the configuration. */
            audioHandle->configuration = 0;
            break;
        case kUSB_DeviceClassEventSetConfiguration:
            //usb_echo("kUSB_DeviceClassEventSetConfiguration \r\n");
            /* Get the new configuration. */
            temp8 = ((uint8_t *)param);
            if (!audioHandle->configStruct)
            {
                break;
            }
            if (*temp8 == audioHandle->configuration)
            {
                break;
            }
            /* De-initialize the endpoints when current configuration is none zero. */
            if (audioHandle->configuration)
            {
                error = USB_DeviceAudioMIDIStreamingEndpointsDeinit(audioHandle);
            }
            /* Save new configuration. */
            audioHandle->configuration = *temp8;
            audioHandle->midiStreamingAlternate = 0;
            audioHandle->midistereamingInterfaceHandle =NULL;
            error = USB_DeviceAudioMIDIStreamingEndpointsInit(audioHandle);
            break;
        case kUSB_DeviceClassEventSetInterface:
            //usb_echo("kUSB_DeviceClassEventSetInterface--- \r\n");
            if (!audioHandle->configStruct)
            {
                break;
            }
            /* Get the new alternate setting of the interface */
            interfaceAlternate = *((uint16_t *)param);
            /* Get the alternate setting value */
            alternate = (uint8_t)(interfaceAlternate & 0xFFU);

            /* Whether the interface belongs to the class. */

            if (audioHandle->midiStreamingInterfaceNumber == ((uint8_t)(interfaceAlternate >> 8U)))
				{
					/* When the interface is stream interface. */
					/* Only handle new alternate setting. */
					if (alternate == audioHandle->midiStreamingAlternate)
					{
						break;
					}
					/* De-initialize old endpoints */
					error = USB_DeviceAudioMIDIStreamingEndpointsDeinit(audioHandle);
					audioHandle->midiStreamingAlternate = alternate;
					/* Initialize new endpoints */
					error = USB_DeviceAudioMIDIStreamingEndpointsInit(audioHandle);
				}
            else
            {
            }
            break;
        case kUSB_DeviceClassEventSetEndpointHalt:
            //usb_echo("kUSB_DeviceClassEventSetEndpointHalt \r\n");
            if (!audioHandle->configStruct)
            {
                break;
            }
            /* Get the endpoint address */
            temp8 = ((uint8_t *)param);

            if (audioHandle->midistereamingInterfaceHandle)
                        {
                            for (int count = 0U; count < audioHandle->midistereamingInterfaceHandle->endpointList.count; count++)
                            {
                                if (*temp8 == audioHandle->midistereamingInterfaceHandle->endpointList.endpoint[count].endpointAddress)
                                {
                                    /* Only stall the endpoint belongs to stream interface of the class */
                                    error = USB_DeviceStallEndpoint(audioHandle->handle, *temp8);
                                }
                            }
                        }
            break;
        case kUSB_DeviceClassEventClearEndpointHalt:
            //usb_echo("kUSB_DeviceClassEventClearEndpointHalt \r\n");
            if (!audioHandle->configStruct)
            {
                break;
            }
            /* Get the endpoint address */
            temp8 = ((uint8_t *)param);
            if (audioHandle->midistereamingInterfaceHandle)
                        {
                            for (int count = 0U; count < audioHandle->midistereamingInterfaceHandle->endpointList.count; count++)
                            {
                                if (*temp8 == audioHandle->midistereamingInterfaceHandle->endpointList.endpoint[count].endpointAddress)
                                {
                                    /* Only un-stall the endpoint belongs to stream interface of the class */
                                    error = USB_DeviceUnstallEndpoint(audioHandle->handle, *temp8);
                                }
                            }
                        }
            break;
        case kUSB_DeviceClassEventClassRequest:
            //usb_echo("kUSB_DeviceClassEventClassRequest \r\n");
#if USBCFG_AUDIO_CLASS_2_0
            if (param)
            {
                /* Handle the audio class specific request. */
                usb_device_control_request_struct_t *controlRequest = (usb_device_control_request_struct_t *)param;
                uint8_t i;
                usb_device_audio_entities_struct_t *entity_list;
                uint8_t entityId = (uint8_t)(controlRequest->setup->wIndex >> 0x08);
                uint32_t audioCommand = 0;
                uint8_t interface_index = (uint8_t)controlRequest->setup->wIndex;

                if (audioHandle->controlInterfaceNumber == interface_index)
                {
                    entity_list =
                        (usb_device_audio_entities_struct_t *)audioHandle->controlInterfaceHandle->classSpecific;
                    for (i = 0; i < entity_list->count; i++)
                    {
                        if (entityId == entity_list->entity[i].entityId)
                        {
                            switch (entity_list->entity[i].entityType)
                            {
                                case USB_DESCRIPTOR_SUBTYPE_AUDIO_CONTROL_OUTPUT_TERMINAL:
                                    break;
                                case USB_DESCRIPTOR_SUBTYPE_AUDIO_CONTROL_FEATURE_UNIT:
                                    if (((controlRequest->setup->bmRequestType & USB_REQUEST_TYPE_DIR_MASK) ==
                                         USB_REQUEST_TYPE_DIR_IN))
                                    {
                                        switch (controlRequest->setup->wValue >> 8)
                                        {
                                            case USB_DEVICE_AUDIO_FU_MUTE_CONTROL:
                                                audioCommand = USB_DEVICE_AUDIO_GET_CUR_MUTE_CONTROL_AUDIO20;
                                                break;
                                            case USB_DEVICE_AUDIO_FU_VOLUME_CONTROL:
                                                if (controlRequest->setup->bRequest == USB_DEVICE_AUDIO_REQUEST_CUR)
                                                {
                                                    audioCommand = USB_DEVICE_AUDIO_GET_CUR_VOLUME_CONTROL_AUDIO20;
                                                }
                                                else if (controlRequest->setup->bRequest ==
                                                         USB_DEVICE_AUDIO_REQUEST_RANGE)
                                                {
                                                    audioCommand = USB_DEVICE_AUDIO_GET_RANGE_VOLUME_CONTROL_AUDIO20;
                                                }
                                                else
                                                {
                                                }
                                                break;
                                            default:
                                                break;
                                        }
                                    }
                                    else if (((controlRequest->setup->bmRequestType & USB_REQUEST_TYPE_DIR_MASK) ==
                                          USB_REQUEST_TYPE_DIR_OUT))
                                    {
                                        switch (controlRequest->setup->wValue >> 8)
                                        {
                                            case USB_DEVICE_AUDIO_FU_MUTE_CONTROL:
                                                audioCommand = USB_DEVICE_AUDIO_SET_CUR_MUTE_CONTROL_AUDIO20;
                                                break;
                                            case USB_DEVICE_AUDIO_FU_VOLUME_CONTROL:
                                                if (controlRequest->setup->bRequest == USB_DEVICE_AUDIO_REQUEST_CUR)
                                                {
                                                    audioCommand = USB_DEVICE_AUDIO_SET_CUR_VOLUME_CONTROL_AUDIO20;
                                                }
                                                break;
                                            default:
                                                break;
                                        }
                                    }

                                    break;
                                case USB_DESCRIPTOR_SUBTYPE_AUDIO_CONTROL_CLOCK_SOURCE_UNIT:
                                    if (((controlRequest->setup->bmRequestType & USB_REQUEST_TYPE_DIR_MASK) ==
                                         USB_REQUEST_TYPE_DIR_IN))
                                    {
                                        switch (controlRequest->setup->wValue >> 8)
                                        {
                                            case USB_DEVICE_AUDIO_CS_SAM_FREQ_CONTROL:
                                                if (controlRequest->setup->bRequest == USB_DEVICE_AUDIO_REQUEST_CUR)
                                                {
                                                    audioCommand = USB_DEVICE_AUDIO_GET_CUR_SAM_FREQ_CONTROL;
                                                }
                                                else if (controlRequest->setup->bRequest ==
                                                         USB_DEVICE_AUDIO_REQUEST_RANGE)
                                                {
                                                    audioCommand = USB_DEVICE_AUDIO_GET_RANGE_SAM_FREQ_CONTROL;
                                                }
                                                else
                                                {
                                                }
                                                break;
                                            case USB_DEVICE_AUDIO_CS_CLOCK_VALID_CONTROL:
                                                audioCommand = USB_DEVICE_AUDIO_GET_CUR_CLOCK_VALID_CONTROL;
                                                break;
                                            default:
                                                break;
                                        }
                                    }
                                    else if (((controlRequest->setup->bmRequestType & USB_REQUEST_TYPE_DIR_MASK) ==
                                              USB_REQUEST_TYPE_DIR_OUT))
                                    {
                                        switch (controlRequest->setup->wValue >> 8)
                                        {
                                            case USB_DEVICE_AUDIO_CS_SAM_FREQ_CONTROL:
                                                audioCommand = USB_DEVICE_AUDIO_SET_CUR_SAM_FREQ_CONTROL;
                                                break;
                                            case USB_DEVICE_AUDIO_CS_CLOCK_VALID_CONTROL:
                                                audioCommand = USB_DEVICE_AUDIO_SET_CUR_CLOCK_VALID_CONTROL;
                                                break;
                                            default:
                                                break;
                                        }
                                    }
                                    else
                                    {
                                    }
                                    break;
                                default:
                                    break;
                            }
                        }
                    }
                    if ((audioCommand) && (NULL != audioHandle->configStruct) &&
                        (audioHandle->configStruct->classCallback))
                    {
                        /* classCallback is initialized in classInit of s_UsbDeviceClassInterfaceMap,
                                 it is from the second parameter of classInit*/
                        error = audioHandle->configStruct->classCallback((class_handle_t)audioHandle, audioCommand,
                                                                         controlRequest);
                    }
                }
            }
            break;

#else

            break;
#endif
        default:
            break;
    }
    return error;
}





usb_status_t USB_DeviceAudioMIDIStreamingRecv(class_handle_t handle, uint8_t ep, uint8_t *buffer, uint32_t length)
{
    usb_device_audio_midi_struct_t *audioHandle;
    usb_status_t error = kStatus_USB_Error;
    audioHandle = (usb_device_audio_midi_struct_t *)handle;
    error = USB_DeviceRecvRequest(audioHandle->handle, ep, buffer, length);
    return error;
}

usb_status_t USB_DeviceAudioMIDIStreamingSend(class_handle_t handle, uint8_t ep, uint8_t *buffer, uint32_t length)
{
    usb_device_audio_midi_struct_t *audioHandle;
    usb_status_t error = kStatus_USB_Error;
    audioHandle = (usb_device_audio_midi_struct_t *)handle;
    error = USB_DeviceSendRequest(audioHandle->handle, ep, buffer, length);
    return error;
}


#endif
