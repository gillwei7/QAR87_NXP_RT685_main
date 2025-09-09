#ifndef DEVICE_CLASS_AUDIO_USB_DEVICE_MIDI_H_
#define DEVICE_CLASS_AUDIO_USB_DEVICE_MIDI_H_

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @brief Enables/disables the Audio Class 2.0 */
#if USB_DEVICE_CONFIG_AUDIO_MIDI
#define USBCFG_AUDIO_CLASS_2_0 0
#endif

/*!
 * @name USB Audio class codes
 * @{
 */

/*! @brief Audio device class code */
#ifndef USB_DEVICE_CONFIG_AUDIO_CLASS_CODE
#define USB_DEVICE_CONFIG_AUDIO_CLASS_CODE (0x01)
#endif

/*! @brief Audio device subclass code */
#define USB_DEVICE_AUDIO_MIDI_STREAMING_SUBCLASS (0x03)
#ifndef USB_DEVICE_AUDIO_STREAM_SUBCLASS
#define USB_DEVICE_AUDIO_STREAM_SUBCLASS (0x02)
#endif
#ifndef USB_DEVICE_AUDIO_CONTROL_SUBCLASS
#define USB_DEVICE_AUDIO_CONTROL_SUBCLASS (0x01)
#endif


/*! @brief Available common EVENT types in audio class callback */
typedef enum
{
	kUSB_DeviceAudioEventMIDIRecv,
	kUSB_DeviceAudioEventMIDISend
} usb_device_audio_midi_event_t;


/*! @brief The audio device class status structure */
typedef struct _usb_device_audio_midi_struct
{
	usb_device_handle handle;                              /*!< The device handle */
	usb_device_class_config_struct_t *configStruct;        /*!< The configuration of the class. */
	usb_device_interface_struct_t *midistereamingInterfaceHandle;
    //usb_device_interface_struct_t *controlInterfaceHandle; /*!< Current control interface handle */
	uint8_t configuration;                                 /*!< Current configuration */
	uint8_t midiStreamingInterfaceNumber;
	uint8_t midiStreamingAlternate;
	uint8_t midiStreamingInPIpeBusy;
	uint8_t midiStreamingOutPipeBusy;
	//uint8_t controlInterfaceNumber;

} usb_device_audio_midi_struct_t;



/*******************************************************************************
 * API
 ******************************************************************************/
extern usb_status_t USB_DeviceAudioMIDIStreamingInit(uint8_t controllerId,
                                        usb_device_class_config_struct_t *config,
                                        class_handle_t *handle);
extern usb_status_t USB_DeviceAudioMIDIStreamingDeinit(class_handle_t handle);
extern usb_status_t USB_DeviceAudioMIDIStreamingEvent(void *handle, uint32_t event, void *param);
extern usb_status_t USB_DeviceAudioMIDIStreamingSend(class_handle_t handle, uint8_t ep, uint8_t *buffer, uint32_t length);
extern usb_status_t USB_DeviceAudioMIDIStreamingRecv(class_handle_t handle, uint8_t ep, uint8_t *buffer, uint32_t length);
#endif /* DEVICE_CLASS_AUDIO_USB_DEVICE_MIDI_H_ */
