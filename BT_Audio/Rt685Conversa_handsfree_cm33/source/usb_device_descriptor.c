/*
 * Copyright (c) 2015 - 2016, Freescale Semiconductor, Inc.
 * Copyright 2016 - 2019 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "GlobalDef.h"


#if EnableUsbComAndAudio==1

#include "usb_device_config.h"
#include "usb.h"
#include "usb_device.h"

#include "usb_device_class.h"
#include "usb_device_audio.h"
#include "usb_device_descriptor.h"
#include "audio_unified.h"




/*******************************************************************************
 * Variables
 ******************************************************************************/
// CDC endpoint, interface, class
#if 1
/* Define endpoint for communication class */
usb_device_endpoint_struct_t g_cdcVcomCicEndpoints[USB_CDC_VCOM_CIC_ENDPOINT_COUNT] = {
    {
        USB_CDC_VCOM_CIC_INTERRUPT_IN_ENDPOINT | (USB_IN << 7U),
        USB_ENDPOINT_INTERRUPT,
        FS_CDC_VCOM_INTERRUPT_IN_PACKET_SIZE,
        FS_CDC_VCOM_INTERRUPT_IN_INTERVAL,
    },
};

/* Define endpoint for data class */
usb_device_endpoint_struct_t g_cdcVcomDicEndpoints[USB_CDC_VCOM_DIC_ENDPOINT_COUNT] = {
    {
        USB_CDC_VCOM_DIC_BULK_IN_ENDPOINT | (USB_IN << 7U),
        USB_ENDPOINT_BULK,
        FS_CDC_VCOM_BULK_IN_PACKET_SIZE,
        0U,
    },
    {
        USB_CDC_VCOM_DIC_BULK_OUT_ENDPOINT | (USB_OUT << 7U),
        USB_ENDPOINT_BULK,
        FS_CDC_VCOM_BULK_OUT_PACKET_SIZE,
        0U,
    },
};

/* Define interface for communication class */
usb_device_interface_struct_t g_cdcVcomCicInterface[] = {{0,
                                                          {
                                                              USB_CDC_VCOM_CIC_ENDPOINT_COUNT,
                                                              g_cdcVcomCicEndpoints,
                                                          },
                                                          NULL}};

/* Define interface for data class */
usb_device_interface_struct_t g_cdcVcomDicInterface[] = {{0,
                                                          {
                                                              USB_CDC_VCOM_DIC_ENDPOINT_COUNT,
                                                              g_cdcVcomDicEndpoints,
                                                          },
                                                          NULL}};

/* Define interfaces for virtual com */
usb_device_interfaces_struct_t g_cdcVcomInterfaces[USB_CDC_VCOM_INTERFACE_COUNT] =
{
    {
   		USB_CDC_VCOM_CIC_CLASS,
   		USB_CDC_VCOM_CIC_SUBCLASS,
		USB_CDC_VCOM_CIC_PROTOCOL,
		USB_CDC_VCOM_CIC_INTERFACE_INDEX,
		g_cdcVcomCicInterface, sizeof(g_cdcVcomCicInterface) / sizeof(usb_device_interface_struct_t)
    },
    {
   		USB_CDC_VCOM_DIC_CLASS,
		USB_CDC_VCOM_DIC_SUBCLASS,
		USB_CDC_VCOM_DIC_PROTOCOL,
		USB_CDC_VCOM_DIC_INTERFACE_INDEX,
		g_cdcVcomDicInterface, sizeof(g_cdcVcomDicInterface) / sizeof(usb_device_interface_struct_t)
    },
};

/* Define configurations for virtual com */
usb_device_interface_list_t g_UsbDeviceCdcVcomInterfaceList[USB_DEVICE_CONFIGURATION_COUNT] = {
    {
        USB_CDC_VCOM_INTERFACE_COUNT,
        g_cdcVcomInterfaces,
    },
};

/* Define class information for virtual com */
usb_device_class_struct_t g_UsbDeviceCdcVcomConfig = {
    g_UsbDeviceCdcVcomInterfaceList,
    kUSB_DeviceClassTypeCdc,
    USB_DEVICE_CONFIGURATION_COUNT,
};
#endif

// USB Audio endpoint, interface, class
#if 1
/* Audio generator stream endpoint information */
usb_device_endpoint_struct_t g_UsbDeviceAudioRecorderEndpoints[USB_AUDIO_RECORDER_STREAM_ENDPOINT_COUNT] = {
    /* Audio generator ISO IN pipe */
    {
        USB_AUDIO_RECORDER_STREAM_ENDPOINT | (USB_IN << USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT),
        USB_ENDPOINT_ISOCHRONOUS,
        FS_ISO_IN_ENDP_PACKET_SIZE + AUDIO_IN_FORMAT_CHANNELS * AUDIO_IN_FORMAT_SIZE,
        FS_ISO_IN_ENDP_INTERVAL,
    },
};

#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
usb_device_endpoint_struct_t g_UsbDeviceAudioSpeakerEndpoints[USB_AUDIO_SPEAKER_STREAM_ENDPOINT_COUNT] = {
    /* Audio generator ISO OUT pipe */
    {
        USB_AUDIO_SPEAKER_STREAM_ENDPOINT | (USB_OUT << USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT),
        USB_ENDPOINT_ISOCHRONOUS,
        FS_ISO_OUT_ENDP_PACKET_SIZE, /* The max
                          packet size should be increased otherwise if host send data larger
                          than max packet size will cause DMA error. */
        FS_ISO_OUT_ENDP_INTERVAL,
    },
};
#else
usb_device_endpoint_struct_t g_UsbDeviceAudioSpeakerEndpoints[USB_AUDIO_SPEAKER_STREAM_ENDPOINT_COUNT] = {
    /* Audio generator ISO OUT pipe */
    {
        USB_AUDIO_SPEAKER_STREAM_ENDPOINT | (USB_OUT << USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT),
        USB_ENDPOINT_ISOCHRONOUS,
        FS_ISO_OUT_ENDP_PACKET_SIZE + AUDIO_OUT_FORMAT_CHANNELS *AUDIO_OUT_FORMAT_SIZE, /* The max
                          packet size should be increased otherwise if host send data larger
                          than max packet size will cause DMA error. */
        FS_ISO_OUT_ENDP_INTERVAL,
    },
    {
        USB_AUDIO_SPEAKER_FEEDBACK_ENDPOINT | (USB_IN << USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT),
        USB_ENDPOINT_ISOCHRONOUS,
        FS_ISO_FEEDBACK_ENDP_PACKET_SIZE,
        1U,
    }};
#endif

/* Audio speaker control endpoint information */
usb_device_endpoint_struct_t g_UsbDeviceAudioControlEndpoints[USB_AUDIO_CONTROL_ENDPOINT_COUNT] = {{
    USB_AUDIO_CONTROL_ENDPOINT | (USB_IN << USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT),
    USB_ENDPOINT_INTERRUPT,
    FS_AUDIO_INTERRUPT_IN_PACKET_SIZE,
    FS_AUDIO_INTERRUPT_IN_INTERVAL,
}};

/* Audio generator entity struct */
usb_device_audio_entity_struct_t g_UsbDeviceAudioRecorderEntity[] = {
#if (USB_DEVICE_CONFIG_AUDIO_CLASS_2_0)
    {
        USB_AUDIO_RECORDER_CONTROL_CLOCK_SOURCE_ID,
        USB_DESCRIPTOR_SUBTYPE_AUDIO_CONTROL_CLOCK_SOURCE_UNIT,
        0U,
    },
#endif
    {
        USB_AUDIO_RECORDER_CONTROL_INPUT_TERMINAL_ID,
        USB_DESCRIPTOR_SUBTYPE_AUDIO_CONTROL_INPUT_TERMINAL,
        0U,
    },
    {
        USB_AUDIO_RECORDER_CONTROL_FEATURE_UNIT_ID,
        USB_DESCRIPTOR_SUBTYPE_AUDIO_CONTROL_FEATURE_UNIT,
        0U,
    },
    {
        USB_AUDIO_RECORDER_CONTROL_OUTPUT_TERMINAL_ID,
        USB_DESCRIPTOR_SUBTYPE_AUDIO_CONTROL_OUTPUT_TERMINAL,
        0U,
    },
};

usb_device_audio_entity_struct_t g_UsbDeviceAudioSpeakerEntity[] = {
#if (USB_DEVICE_CONFIG_AUDIO_CLASS_2_0)
    {
        USB_AUDIO_SPEAKER_CONTROL_CLOCK_SOURCE_ID,
        USB_DESCRIPTOR_SUBTYPE_AUDIO_CONTROL_CLOCK_SOURCE_UNIT,
        0U,
    },
#endif
    {
        USB_AUDIO_SPEAKER_CONTROL_INPUT_TERMINAL_ID,
        USB_DESCRIPTOR_SUBTYPE_AUDIO_CONTROL_INPUT_TERMINAL,
        0U,
    },
    {
        USB_AUDIO_SPEAKER_CONTROL_FEATURE_UNIT_ID,
        USB_DESCRIPTOR_SUBTYPE_AUDIO_CONTROL_FEATURE_UNIT,
        0U,
    },
    {
        USB_AUDIO_SPEAKER_CONTROL_OUTPUT_TERMINAL_ID,
        USB_DESCRIPTOR_SUBTYPE_AUDIO_CONTROL_OUTPUT_TERMINAL,
        0U,
    },

};
/* Audio speaker entity information */
usb_device_audio_entities_struct_t g_UsbDeviceAudioRecorderEntities = {
    g_UsbDeviceAudioRecorderEntity,
    sizeof(g_UsbDeviceAudioRecorderEntity) / sizeof(usb_device_audio_entity_struct_t),
};

usb_device_audio_entities_struct_t g_UsbDeviceAudioSpeakerEntities = {
    g_UsbDeviceAudioSpeakerEntity,
    sizeof(g_UsbDeviceAudioSpeakerEntity) / sizeof(usb_device_audio_entity_struct_t),
};

/* Audio speaker control interface information */
usb_device_interface_struct_t g_UsbDeviceAudioRecorderControInterface[] = {{
    0U,
    {
        USB_AUDIO_CONTROL_ENDPOINT_COUNT,
        g_UsbDeviceAudioControlEndpoints,
    },
    &g_UsbDeviceAudioRecorderEntities,
}};

usb_device_interface_struct_t g_UsbDeviceAudioSpeakerControInterface[] = {{
    0U,
    {
        USB_AUDIO_CONTROL_ENDPOINT_COUNT,
        g_UsbDeviceAudioControlEndpoints,
    },
    &g_UsbDeviceAudioSpeakerEntities,
}};

/* Audio recorder stream interface information */
usb_device_interface_struct_t g_UsbDeviceAudioRecStreamInterface[] = {
    {
        0U,
        {
            0U,
            NULL,
        },
        NULL,
    },
    {
        1U,
        {
            USB_AUDIO_RECORDER_STREAM_ENDPOINT_COUNT,
            g_UsbDeviceAudioRecorderEndpoints,
        },
        NULL,
    },
};

/* Audio speaker stream interface information */
usb_device_interface_struct_t g_UsbDeviceAudioSpeakerStreamInterface[] = {
    {
        0U,
        {
            0U,
            NULL,
        },
        NULL,
    },
    {
        1U,
        {
            USB_AUDIO_SPEAKER_STREAM_ENDPOINT_COUNT,
            g_UsbDeviceAudioSpeakerEndpoints,
        },
        NULL,
    },
};

/* Define interfaces for audio speaker */
usb_device_interfaces_struct_t g_UsbDeviceAudioRecorderInterfaces[2] = {
    {
        USB_AUDIO_CLASS,                         /* Audio class code */
        USB_SUBCLASS_AUDIOCONTROL,               /* Audio control subclass code */
        USB_AUDIO_PROTOCOL,                      /* Audio protocol code */
        USB_AUDIO_CONTROL_INTERFACE_INDEX,       /* The interface number of the Audio control */
        g_UsbDeviceAudioRecorderControInterface, /* The handle of Audio control */
        sizeof(g_UsbDeviceAudioRecorderControInterface) / sizeof(usb_device_interface_struct_t),
    },
    {
        USB_AUDIO_CLASS,                           /* Audio class code */
        USB_SUBCLASS_AUDIOSTREAM,                  /* Audio stream subclass code */
        USB_AUDIO_PROTOCOL,                        /* Audio protocol code */
        USB_AUDIO_RECORDER_STREAM_INTERFACE_INDEX, /* The interface number of the Audio control */
        g_UsbDeviceAudioRecStreamInterface,        /* The handle of Audio stream */
        sizeof(g_UsbDeviceAudioRecStreamInterface) / sizeof(usb_device_interface_struct_t),
    }

};
usb_device_interfaces_struct_t g_UsbDeviceAudioSpeakerInterfaces[2] = {
    {
        USB_AUDIO_CLASS,                        /* Audio class code */
        USB_SUBCLASS_AUDIOCONTROL,              /* Audio control subclass code */
        USB_AUDIO_PROTOCOL,                     /* Audio protocol code */
        USB_AUDIO_CONTROL_INTERFACE_INDEX,      /* The interface number of the Audio control */
        g_UsbDeviceAudioSpeakerControInterface, /* The handle of Audio control */
        sizeof(g_UsbDeviceAudioSpeakerControInterface) / sizeof(usb_device_interface_struct_t),
    },
    {
        USB_AUDIO_CLASS,                          /* Audio class code */
        USB_SUBCLASS_AUDIOSTREAM,                 /* Audio stream subclass code */
        USB_AUDIO_PROTOCOL,                       /* Audio protocol code */
        USB_AUDIO_SPEAKER_STREAM_INTERFACE_INDEX, /* The interface number of the Audio control */
        g_UsbDeviceAudioSpeakerStreamInterface,   /* The handle of Audio stream */
        sizeof(g_UsbDeviceAudioSpeakerStreamInterface) / sizeof(usb_device_interface_struct_t),
    }

};

/* Define configurations for USB audio --- recorder */
usb_device_interface_list_t g_UsbDeviceAudioInterfaceListRecorder[USB_DEVICE_CONFIGURATION_COUNT] = {
    {
        2,
        g_UsbDeviceAudioRecorderInterfaces,
    },
};

/* Define configurations for USB audio --- speaker */
usb_device_interface_list_t g_UsbDeviceAudioInterfaceListSpeaker[USB_DEVICE_CONFIGURATION_COUNT] = {
    {
        2,
        g_UsbDeviceAudioSpeakerInterfaces,
    },
};

/* Define class information for audio recorder */
usb_device_class_struct_t g_UsbDeviceAudioClassRecorder = {
    g_UsbDeviceAudioInterfaceListRecorder,
    kUSB_DeviceClassTypeAudio,
    USB_DEVICE_CONFIGURATION_COUNT,
};

/* Define class information for audio speaker */
usb_device_class_struct_t g_UsbDeviceAudioClassSpeaker = {
    g_UsbDeviceAudioInterfaceListSpeaker,
    kUSB_DeviceClassTypeAudio,
    USB_DEVICE_CONFIGURATION_COUNT,
};
#endif

//dev dscr
#if 1
/* Define device descriptor */
USB_DMA_INIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE)
uint8_t g_UsbDeviceDescriptor[] = {
    USB_DESCRIPTOR_LENGTH_DEVICE, /* Size of this descriptor in bytes */
    USB_DESCRIPTOR_TYPE_DEVICE,   /* DEVICE Descriptor Type */
    USB_SHORT_GET_LOW(USB_DEVICE_SPECIFIC_BCD_VERSION),
    USB_SHORT_GET_HIGH(USB_DEVICE_SPECIFIC_BCD_VERSION), /* USB Specification Release Number in
                                                            Binary-Coded Decimal (i.e., 2.10 is 210H). */
    USB_DEVICE_CLASS,                                    /* Class code (assigned by the USB-IF). */
    USB_DEVICE_SUBCLASS,                                 /* Subclass code (assigned by the USB-IF). */
    USB_DEVICE_PROTOCOL,                                 /* Protocol code (assigned by the USB-IF). */
    USB_CONTROL_MAX_PACKET_SIZE,                         /* Maximum packet size for endpoint zero
                                                            (only 8, 16, 32, or 64 are valid) */
	USB_SHORT_GET_LOW(USB_DEVICE_VID),
	USB_SHORT_GET_HIGH(USB_DEVICE_VID), /* Vendor ID (assigned by the USB-IF) */

	//USB_SHORT_GET_LOW(USB_DEVICE_PID),
    //USB_SHORT_GET_HIGH(USB_DEVICE_PID),
	(USB_DEVICE_PID_Bit4To7<<4)|USB_DEVICE_PID_Bit0To3,
	USB_DEVICE_PID_Bit8To15,

    USB_SHORT_GET_LOW(USB_DEVICE_DEMO_BCD_VERSION),
    USB_SHORT_GET_HIGH(USB_DEVICE_DEMO_BCD_VERSION), /* Device release number in binary-coded decimal */
    0x01U,                                           /* Index of string descriptor describing manufacturer */
    0x02U,                                           /* Index of string descriptor describing product */
    0x00U,                                           /* Index of string descriptor describing the
                                                        device's serial number */
    USB_DEVICE_CONFIGURATION_COUNT,                  /* Number of possible configurations */
};
#endif

//cfg dscr
#if 1
/* Define configuration descriptor */
USB_DMA_INIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE)
uint8_t g_UsbDeviceConfigurationDescriptor[] = {
    /* Configuration Descriptor Size*/
    USB_DESCRIPTOR_LENGTH_CONFIGURE,
    /* CONFIGURATION Descriptor Type */
    USB_DESCRIPTOR_TYPE_CONFIGURE,
    /* Total length of data returned for this configuration. */
	#if 1
    USB_SHORT_GET_LOW(
			USB_DESCRIPTOR_LENGTH_CONFIGURE +
			//audio
			#if (USB_DEVICE_CONFIG_AUDIO_CLASS_2_0)
				USB_IAD_DESC_SIZE+USB_AUDIO_CONTROL_INTERFACE_HEADER_LENGTH+0x77+
				USB_DESCRIPTOR_LENGTH_INTERFACE+USB_DESCRIPTOR_LENGTH_INTERFACE+0x10+0x06+USB_AUDIO_STREAMING_ENDP_DESC_SIZE+USB_AUDIO_CLASS_SPECIFIC_ENDPOINT_LENGTH+
				USB_DESCRIPTOR_LENGTH_INTERFACE+USB_DESCRIPTOR_LENGTH_INTERFACE+0x10+0x06+USB_AUDIO_STREAMING_ENDP_DESC_SIZE+USB_AUDIO_CLASS_SPECIFIC_ENDPOINT_LENGTH+
				#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
				#else
								USB_AUDIO_STANDARD_AS_ISO_FEEDBACK_ENDPOINT_LENGTH+
				#endif
			#else
			#endif
			//CDC
			USB_IAD_DESC_SIZE + USB_DESCRIPTOR_LENGTH_INTERFACE + USB_DESCRIPTOR_LENGTH_CDC_HEADER_FUNC +
			USB_DESCRIPTOR_LENGTH_CDC_CALL_MANAG + USB_DESCRIPTOR_LENGTH_CDC_ABSTRACT +
			USB_DESCRIPTOR_LENGTH_CDC_UNION_FUNC + USB_DESCRIPTOR_LENGTH_ENDPOINT + USB_DESCRIPTOR_LENGTH_INTERFACE +
			USB_DESCRIPTOR_LENGTH_ENDPOINT + USB_DESCRIPTOR_LENGTH_ENDPOINT
		),
    USB_SHORT_GET_HIGH(
			USB_DESCRIPTOR_LENGTH_CONFIGURE +
			//audio
			#if (USB_DEVICE_CONFIG_AUDIO_CLASS_2_0)
				USB_IAD_DESC_SIZE+USB_AUDIO_CONTROL_INTERFACE_HEADER_LENGTH+0x77+
				USB_DESCRIPTOR_LENGTH_INTERFACE+USB_DESCRIPTOR_LENGTH_INTERFACE+0x10+0x06+USB_AUDIO_STREAMING_ENDP_DESC_SIZE+USB_AUDIO_CLASS_SPECIFIC_ENDPOINT_LENGTH+
				USB_DESCRIPTOR_LENGTH_INTERFACE+USB_DESCRIPTOR_LENGTH_INTERFACE+0x10+0x06+USB_AUDIO_STREAMING_ENDP_DESC_SIZE+USB_AUDIO_CLASS_SPECIFIC_ENDPOINT_LENGTH+
				#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
				#else
								USB_AUDIO_STANDARD_AS_ISO_FEEDBACK_ENDPOINT_LENGTH+
				#endif
			#else
			#endif
			//CDC
			USB_IAD_DESC_SIZE + USB_DESCRIPTOR_LENGTH_INTERFACE + USB_DESCRIPTOR_LENGTH_CDC_HEADER_FUNC +
			USB_DESCRIPTOR_LENGTH_CDC_CALL_MANAG + USB_DESCRIPTOR_LENGTH_CDC_ABSTRACT +
			USB_DESCRIPTOR_LENGTH_CDC_UNION_FUNC + USB_DESCRIPTOR_LENGTH_ENDPOINT + USB_DESCRIPTOR_LENGTH_INTERFACE +
			USB_DESCRIPTOR_LENGTH_ENDPOINT + USB_DESCRIPTOR_LENGTH_ENDPOINT
		),
	#endif
    /* Number of interfaces supported by this configuration */
		USB_DEVICE_INTERFACE_COUNT,
    /* Value to use as an argument to the SetConfiguration() request to select this configuration */
    USB_COMPOSITE_CONFIGURE_INDEX,
    /* Index of string descriptor describing this configuration */
    0,
    /* Configuration characteristics D7: Reserved (set to one) D6: Self-powered D5: Remote Wakeup D4...0: Reserved
       (reset to zero) */
    (USB_DESCRIPTOR_CONFIGURE_ATTRIBUTE_D7_MASK) |
        (USB_DEVICE_CONFIG_SELF_POWER << USB_DESCRIPTOR_CONFIGURE_ATTRIBUTE_SELF_POWERED_SHIFT) |
        (USB_DEVICE_CONFIG_REMOTE_WAKEUP << USB_DESCRIPTOR_CONFIGURE_ATTRIBUTE_REMOTE_WAKEUP_SHIFT),
    /* Maximum power consumption of the USB * device from the bus in this specific * configuration when the device is
       fully * operational. Expressed in 2 mA units *  (i.e., 50 = 100 mA).  */
    USB_DEVICE_MAX_POWER,

//-----next is for audio class
//-----next is for audio class
//-----next is for audio class
//-----next is for audio class
//-----next is for audio class
//-----next is for audio class
#if (USB_DEVICE_CONFIG_AUDIO_CLASS_2_0)
	/* Interface Association Descriptor */
	/* Size of this descriptor in bytes */
	USB_IAD_DESC_SIZE,
	/* INTERFACE_ASSOCIATION Descriptor Type  */
	USB_DESCRIPTOR_TYPE_INTERFACE_ASSOCIATION,
	/* The first interface number associated with this function */
	USB_AUDIO_CONTROL_INTERFACE_INDEX,
	/* The number of contiguous interfaces associated with this function */
	USB_AUDIO_SpkAndRec_INTERFACE_COUNT,
	/* The function belongs to the Communication Device/Interface Class  */
	USB_AUDIO_CLASS, 0x00U,
	/* Protocol code = 32    --- UAC2.0*/
	USB_AUDIO_PROTOCOL,
	/* The Function string descriptor index */
	0x04,

//Interface 0, ALT 0 only, EP0 + 1 extra EP, Audio+Control
//Interface 0, ALT 0 only, EP0 + 1 extra EP, Audio+Control
//Interface 0, ALT 0 only, EP0 + 1 extra EP, Audio+Control
	    USB_AUDIO_CONTROL_INTERFACE_HEADER_LENGTH, /* Size of the descriptor, in bytes  */
	    USB_DESCRIPTOR_TYPE_INTERFACE,             /* CS_INTERFACE Descriptor Type   */
	    USB_AUDIO_CONTROL_INTERFACE_INDEX,         /* The number of this interface is 0 */
	    0x00U,                     /* The value used to select the alternate setting for this interface is 0   */
	    0x00U,                     /* The number of endpoints used by this interface is 0 (excluding endpoint zero)   */
	    USB_AUDIO_CLASS,           /* The interface implements the Audio Interface class   */
	    USB_SUBCLASS_AUDIOCONTROL, /* The interface implements the AUDIOCONTROL Subclass   */
	    USB_AUDIO_PROTOCOL,        /* The Protocol code is 32   */
	    0x00U,                     /* The interface string descriptor index is 2  */

	    USB_AUDIO_CONTROL_INTERFACE_HEADER_LENGTH, /* Size of the descriptor, in bytes  */
	    USB_DESCRIPTOR_TYPE_AUDIO_CS_INTERFACE,    /* CS_INTERFACE Descriptor Type   */
	    0x01U,                                     /* HEADER descriptor subtype  */
	    0x00U, 0x02U,                              /* Audio Device compliant to the USB Audio specification version 2.00  */
	    0x04,         /* Undefied(0x00) : Indicating the primary use of this audio function   */
	    0x77U, 0x00U, /* Total number of bytes returned for the class-specific
	                    AudioControl interface descriptor. Includes the combined length of this descriptor header and all
	                    Unit and Terminal descriptors.   */
	    0x00U,        /* D1..0: Latency Control  */

					USB_AUDIO_CLOCK_SOURCE_LENGTH,              /* Size of the descriptor, in bytes  */
					USB_DESCRIPTOR_TYPE_AUDIO_CS_INTERFACE,     /* CS_INTERFACE Descriptor Type  */
					0x0AU,                                      /* CLOCK_SOURCE descriptor subtype  */
					USB_AUDIO_RECORDER_CONTROL_CLOCK_SOURCE_ID, /* Constant uniquely identifying the Clock Source Entity within the
																 * audio funcion
																 */
					0x01U,                                      /* D1..0: 01: Internal Fixed Clock
																   D2: 0 Clock is not synchronized to SOF
																   D7..3: Reserved, should set to 0   */
					0x07U,                                      /* D1..0: Clock Frequency Control is present and Host programmable
																   D3..2: Clock Validity Control is present but read-only
																   D7..4: Reserved, should set to 0 */
					0x00U,                                      /* This Clock Source has no association   */
					0x00U,                                      /* Index of a string descriptor, describing the Clock Source Entity  */

						0x11U,                                               /* Size of the descriptor, in bytes  */
						USB_DESCRIPTOR_TYPE_AUDIO_CS_INTERFACE,              /* CS_INTERFACE Descriptor Type   */
						USB_DESCRIPTOR_SUBTYPE_AUDIO_CONTROL_INPUT_TERMINAL, /* INPUT_TERMINAL descriptor subtype   */
						USB_AUDIO_RECORDER_CONTROL_INPUT_TERMINAL_ID,        /* Constant uniquely identifying the Terminal within the audio
										 function. This value is used in all requests        to address this Terminal.   */
						0x01U, 0x02U, /* A generic microphone that does not fit under any of the other classifications.  */
						0x00U,        /* This Input Terminal has no association   */
						USB_AUDIO_RECORDER_CONTROL_CLOCK_SOURCE_ID, /* ID of the Clock Entity to which this Input Terminal is connected.  */
						AUDIO_IN_FORMAT_CHANNELS,   /* This Terminal's output audio channel cluster has 16 logical output channels   */
						0x01U, 0x00U, 0x00U, 0x00U, /* Describes the spatial location of the logical channels:: Mono, no spatial location */
						0x00U,                      /* Index of a string descriptor, describing the name of the first logical channel.  */
						0x00U, 0x00U,               /* bmControls D1..0: Copy Protect Control is not present
													   D3..2: Connector Control is not present
													   D5..4: Overload Control is not present
													   D7..6: Cluster Control is not present
													   D9..8: Underflow Control is not present
													   D11..10: Overflow Control is not present
													   D15..12: Reserved, should set to 0*/
						0x00U,                      /* Index of a string descriptor, describing the Input Terminal.  */

							0x12U,                                             /* Size of the descriptor, in bytes  : 6 + (2 + 1) * 4 */
							USB_DESCRIPTOR_TYPE_AUDIO_CS_INTERFACE,            /* CS_INTERFACE Descriptor Type   */
							USB_DESCRIPTOR_SUBTYPE_AUDIO_CONTROL_FEATURE_UNIT, /* FEATURE_UNIT descriptor subtype   */
							USB_AUDIO_RECORDER_CONTROL_FEATURE_UNIT_ID,   /* Constant uniquely identifying the Unit within the audio function.
										This value is used in all requests to   address this Unit.  */
							USB_AUDIO_RECORDER_CONTROL_INPUT_TERMINAL_ID, /* ID of the Unit or Terminal to which this Feature Unit is connected.
																		   */
							0x0FU, 0x00U, 0x00U, 0x00U, /* logic channel 0 bmaControls(0)(0x0000000F):  D1..0: Mute Control is present and host
														   programmable D3..2: Volume Control is present and host programmable D5..4: Bass
														   Control is not present D7..6: Mid Control is not present D9..8: Treble Control is not
														   present D11..10: Graphic Equalizer Control is not present D13..12: Automatic Gain
														   Control is not present D15..14: Delay Control is not present D17..16: Bass Control is
														   not present D19..18: Loudness Control is not present D21..20: Input Gain Control is
														   not present D23..22: Input Gain Pad Control is not present D25..24: Phase Inverter
														   Control is not present D27..26: Underflow Control is not present D29..28: Overflow
														   Control is not present D31..30: Reserved, should set to 0 */
							0x00U, 0x00U, 0x00U, 0x00U, /* The Controls bitmap for logical channel 1. */
							0x00U, 0x00U, 0x00U, 0x00U, /* The Controls bitmap for logical channel 2. */
							0x00U,                      /* Index of a string descriptor, describing this Feature Unit.   */

								0x0CU,                                                /* Size of the descriptor, in bytes   */
								USB_DESCRIPTOR_TYPE_AUDIO_CS_INTERFACE,               /* CS_INTERFACE Descriptor Type  */
								USB_DESCRIPTOR_SUBTYPE_AUDIO_CONTROL_OUTPUT_TERMINAL, /* OUTPUT_TERMINAL descriptor subtype   */
								USB_AUDIO_RECORDER_CONTROL_OUTPUT_TERMINAL_ID,        /* Constant uniquely identifying the Terminal within the audio
												 function. This value is used in all requests        to address this Terminal.   */
								0x01U,
								0x01U, /* A Terminal dealing with a signal carried over an endpoint in an AudioStreaming interface. The
									   AudioStreaming interface descriptor points to the associated Terminal through the bTerminalLink field.  */
								0x00U, /* This Output Terminal has no association  */
								USB_AUDIO_RECORDER_CONTROL_FEATURE_UNIT_ID, /* ID of the Unit or Terminal to which this Terminal is connected.  */
								USB_AUDIO_RECORDER_CONTROL_CLOCK_SOURCE_ID, /* ID of the Clock Entity to which this Output Terminal is connected  */
								0x00U, 0x00U,                               /* bmControls:   D1..0: Copy Protect Control is not present
																			   D3..2: Connector Control is not present
																			   D5..4: Overload Control is not present
																			   D7..6: Underflow Control is not present
																			   D9..8: Overflow Control is not present
																			   D15..10: Reserved, should set to 0   */
								0x00U,                                      /* Index of a string descriptor, describing the Output Terminal.  */

					USB_AUDIO_CLOCK_SOURCE_LENGTH,             /* Size of the descriptor, in bytes  */
					USB_DESCRIPTOR_TYPE_AUDIO_CS_INTERFACE,    /* CS_INTERFACE Descriptor Type  */
					0x0AU,                                     /* CLOCK_SOURCE descriptor subtype  */
					USB_AUDIO_SPEAKER_CONTROL_CLOCK_SOURCE_ID, /* Constant uniquely identifying the Clock Source Entity within the audio
																* funcion
																*/
					0x01U,                                     /* D1..0: 01: Internal Fixed Clock
																  D2: 0 Clock is not synchronized to SOF
																  D7..3: Reserved, should set to 0   */
					0x07U,                                     /* D1..0: Clock Frequency Control is present and Host programmable
																  D3..2: Clock Validity Control is present but read-only
																  D7..4: Reserved, should set to 0 */
					0x00U,                                     /* This Clock Source has no association   */
					0x00U,                                     /* Index of a string descriptor, describing the Clock Source Entity  */

						0x11U,                                               /* Size of the descriptor, in bytes  */
						USB_DESCRIPTOR_TYPE_AUDIO_CS_INTERFACE,              /* CS_INTERFACE Descriptor Type   */
						USB_DESCRIPTOR_SUBTYPE_AUDIO_CONTROL_INPUT_TERMINAL, /* INPUT_TERMINAL descriptor subtype   */
						USB_AUDIO_SPEAKER_CONTROL_INPUT_TERMINAL_ID,         /* Constant uniquely identifying the Terminal within the audio
										  function. This value is used in all requests         to address this Terminal.   */
						0x01U,
						0x01U, /* A Terminal dealing with a signal carried over an endpoint in an AudioStreaming interface. The
								  AudioStreaming interface descriptor points to the associated Terminal through the bTerminalLink field. */
						0x00U, /* This Input Terminal has no association   */
						USB_AUDIO_SPEAKER_CONTROL_CLOCK_SOURCE_ID, /* ID of the Clock Entity to which this Input Terminal is connected.  */
						AUDIO_OUT_FORMAT_CHANNELS,  /* This Terminal's output audio channel cluster has 16 logical output channels   */
						0x03U, 0x00U, 0x00U, 0x00U, /* Describes the spatial location of the logical channels:: Mono, no spatial location */
						0x00U,                      /* Index of a string descriptor, describing the name of the first logical channel.  */
						0x00U, 0x00U,               /* bmControls D1..0: Copy Protect Control is not present
													   D3..2: Connector Control is not present
													   D5..4: Overload Control is not present
													   D7..6: Cluster Control is not present
													   D9..8: Underflow Control is not present
													   D11..10: Overflow Control is not present
													   D15..12: Reserved, should set to 0*/
						0x00U,                      /* Index of a string descriptor, describing the Input Terminal.  */

							18,                                              /* Size of the descriptor, in bytes  : 6 + (8 + 1) * 4 =42*/
							USB_DESCRIPTOR_TYPE_AUDIO_CS_INTERFACE,            /* CS_INTERFACE Descriptor Type   */
							USB_DESCRIPTOR_SUBTYPE_AUDIO_CONTROL_FEATURE_UNIT, /* FEATURE_UNIT descriptor subtype   */
							USB_AUDIO_SPEAKER_CONTROL_FEATURE_UNIT_ID, /* Constant uniquely identifying the Unit within the audio function. This
									  value is used in all requests to address this Unit.  */
							USB_AUDIO_SPEAKER_CONTROL_INPUT_TERMINAL_ID, /* ID of the Unit or Terminal to which this Feature Unit is connected.
																		  */
							0x0FU, 0x00U, 0x00U, 0x00U, /* logic channel 0 bmaControls(0)(0x0000000F):  D1..0: Mute Control is present and host
														   programmable D3..2: Volume Control is present and host programmable D5..4: Bass
														   Control is not present D7..6: Mid Control is not present D9..8: Treble Control is not
														   present D11..10: Graphic Equalizer Control is not present D13..12: Automatic Gain
														   Control is not present D15..14: Delay Control is not present D17..16: Bass Control is
														   not present D19..18: Loudness Control is not present D21..20: Input Gain Control is
														   not present D23..22: Input Gain Pad Control is not present D25..24: Phase Inverter
														   Control is not present D27..26: Underflow Control is not present D29..28: Overflow
														   Control is not present D31..30: Reserved, should set to 0 */
							0x00U, 0x00U, 0x00U, 0x00U, /* The Controls bitmap for logical channel 1. */
							0x00U, 0x00U, 0x00U, 0x00U, /* The Controls bitmap for logical channel 2. */
							0x00U,                      /* Index of a string descriptor, describing this Feature Unit.   */

								0x0CU,                                                /* Size of the descriptor, in bytes   */
								USB_DESCRIPTOR_TYPE_AUDIO_CS_INTERFACE,               /* CS_INTERFACE Descriptor Type  */
								USB_DESCRIPTOR_SUBTYPE_AUDIO_CONTROL_OUTPUT_TERMINAL, /* OUTPUT_TERMINAL descriptor subtype   */
								USB_AUDIO_SPEAKER_CONTROL_OUTPUT_TERMINAL_ID,         /* Constant uniquely identifying the Terminal within the audio
												  function. This value is used in all requests         to address this Terminal.   */
								0x01U, 0x03U, /* A generic speaker or set of speakers that does not fit under any of the other classifications. */
								0x00U,        /* This Output Terminal has no association  */
								USB_AUDIO_SPEAKER_CONTROL_FEATURE_UNIT_ID, /* ID of the Unit or Terminal to which this Terminal is connected.  */
								USB_AUDIO_SPEAKER_CONTROL_CLOCK_SOURCE_ID, /* ID of the Clock Entity to which this Output Terminal is connected  */
								0x00U, 0x00U,                              /* bmControls:   D1..0: Copy Protect Control is not present
																			  D3..2: Connector Control is not present
																			  D5..4: Overload Control is not present
																			  D7..6: Underflow Control is not present
																			  D9..8: Overflow Control is not present
																			  D15..10: Reserved, should set to 0   */
								0x00U,                                     /* Index of a string descriptor, describing the Output Terminal.  */


//Interface 1, ALT 0,1 EP0 + 1 extra EP, stream --- mic up streaming
//Interface 1, ALT 0,1 EP0 + 1 extra EP, stream --- mic up streaming
//Interface 1, ALT 0,1 EP0 + 1 extra EP, stream --- mic up streaming

	    /* Audio Class Specific INTERFACE Descriptor, alternative interface 0  */
	    USB_DESCRIPTOR_LENGTH_INTERFACE,           /* Descriptor size is 9 bytes  */
	    USB_DESCRIPTOR_TYPE_INTERFACE,             /* INTERFACE Descriptor Type   */
	    USB_AUDIO_RECORDER_STREAM_INTERFACE_INDEX, /* The number of this interface is 1.  */
	    0x00U,                    /* The value used to select the alternate setting for this interface is 0   */
	    0x00U,                    /* The number of endpoints used by this interface is 0 (excluding endpoint zero)   */
	    USB_AUDIO_CLASS,          /* The interface implements the Audio Interface class   */
	    USB_SUBCLASS_AUDIOSTREAM, /* The interface implements the AUDIOSTREAMING Subclass   */
	    USB_AUDIO_PROTOCOL,       /* The Protocol code is 32   */
	    0x00U,                    /* The device doesn't have a string descriptor describing this iInterface  */

	    /* Audio Class Specific INTERFACE Descriptor, alternative interface 1 */
	    USB_DESCRIPTOR_LENGTH_INTERFACE,           /* Descriptor size is 9 bytes  */
	    USB_DESCRIPTOR_TYPE_INTERFACE,             /* INTERFACE Descriptor Type  */
	    USB_AUDIO_RECORDER_STREAM_INTERFACE_INDEX, /*The number of this interface is 1.  */
	    0x01U,                    /* The value used to select the alternate setting for this interface is 1  */
	    0x01U,                    /* The number of endpoints used by this interface is 1 (excluding endpoint zero)    */
	    USB_AUDIO_CLASS,          /* The interface implements the Audio Interface class   */
	    USB_SUBCLASS_AUDIOSTREAM, /* The interface implements the AUDIOSTREAMING Subclass   */
	    USB_AUDIO_PROTOCOL,       /* The Protocol code is 32   */
	    0x00U,                    /* The device doesn't have a string descriptor describing this iInterface  */

	    0x10U,                                          /* Size of the descriptor, in bytes   */
	    USB_DESCRIPTOR_TYPE_AUDIO_CS_INTERFACE,         /* CS_INTERFACE Descriptor Type  */
	    USB_DESCRIPTOR_SUBTYPE_AUDIO_STREAMING_GENERAL, /* AS_GENERAL descriptor subtype   */
	    USB_AUDIO_RECORDER_CONTROL_OUTPUT_TERMINAL_ID,  /* The Terminal ID of the terminal to which this interface is
	                                                       connected   */
	    0x00U,                      /* bmControls : D1..0: Active Alternate Setting Control is not present
	                                   D3..2: Valid Alternate Settings Control is not present
	                                   D7..4: Reserved, should set to 0   */
	    0x01U,                      /* The format type AudioStreaming interfae using is FORMAT_TYPE_I (0x01)   */
	    0x01U, 0x00U, 0x00U, 0x00U, /* The Audio Data Format that can be Used to communicate with this interface */
	    AUDIO_IN_FORMAT_CHANNELS,   /* Number of physical channels in the AS Interface audio channel cluster */
	    0x01U, 0x00U, 0x00U, 0x00U, /* Describes the spatial location of the logical channels: */
	    0x00U,                      /* Index of a string descriptor, describing the name of the first physical channel   */

	    0x06U,                                              /* Size of the descriptor, in bytes   */
	    USB_DESCRIPTOR_TYPE_AUDIO_CS_INTERFACE,             /* CS_INTERFACE Descriptor Type   */
	    USB_DESCRIPTOR_SUBTYPE_AUDIO_STREAMING_FORMAT_TYPE, /* FORMAT_TYPE descriptor subtype   */
	    0x01U, /* The format type AudioStreaming interfae using is FORMAT_TYPE_I (0x01)   */
		AUDIO_IN_FORMAT_SIZE, /* The number of bytes occupied by one audio subslot. Can be 1, 2, 3 or 4.  */
		(8*AUDIO_IN_FORMAT_SIZE), /* The number of effectively used bits from the available bits in an audio subslot   */

	    /* ENDPOINT Descriptor */
	    USB_AUDIO_STREAMING_ENDP_DESC_SIZE,                 /* Descriptor size is 7 bytes  */
	    USB_DESCRIPTOR_TYPE_ENDPOINT,                       /* ENDPOINT Descriptor Type   */
	    USB_AUDIO_RECORDER_STREAM_ENDPOINT | (USB_IN << 7), /* This is an IN endpoint with endpoint number 2   */
	    0x0D,                                               /* Types -
	                                                                                                 Transfer: ISOCHRONOUS
	                                                                                                 Sync: Sync
	                                                                                                 Usage: Data EP  */
	    USB_SHORT_GET_LOW(FS_ISO_IN_ENDP_PACKET_SIZE),
	    USB_SHORT_GET_HIGH(FS_ISO_IN_ENDP_PACKET_SIZE), /* Maximum packet size for this endpoint */
	    FS_ISO_IN_ENDP_INTERVAL, /* The polling interval value is every 1 Frames. If Hi-Speed, every 1 uFrames   */

	    /* Audio Class Specific ENDPOINT Descriptor  */
	    USB_AUDIO_CLASS_SPECIFIC_ENDPOINT_LENGTH, /*  Size of the descriptor, in bytes  */
	    USB_AUDIO_STREAM_ENDPOINT_DESCRIPTOR,     /* CS_ENDPOINT Descriptor Type  */
	    USB_AUDIO_EP_GENERAL_DESCRIPTOR_SUBTYPE,  /* AUDIO_EP_GENERAL descriptor subtype  */
	    0x00U, 0x00U, 0x00U, 0x00U, 0x00U,


//Interface 2, ALT 0,1 EP0 + 1 extra EP, stream --- spk downstreaming
//Interface 2, ALT 0,1 EP0 + 1 extra EP, stream --- spk downstreaming
//Interface 2, ALT 0,1 EP0 + 1 extra EP, stream --- spk downstreaming

	    USB_DESCRIPTOR_LENGTH_INTERFACE,          /* Descriptor size is 9 bytes   */
	    USB_DESCRIPTOR_TYPE_INTERFACE,            /* INTERFACE Descriptor Type   */
	    USB_AUDIO_SPEAKER_STREAM_INTERFACE_INDEX, /* The number of this interface is 1.   */
	    0x00U,                    /* The value used to select the alternate setting for this interface is 0   */
	    0x00U,                    /* The number of endpoints used by this interface is 0 (excluding endpoint zero)   */
	    USB_AUDIO_CLASS,          /* The interface implements the Audio Interface class   */
	    USB_SUBCLASS_AUDIOSTREAM, /* The interface implements the AUDIOSTREAMING Subclass   */
	    USB_AUDIO_PROTOCOL,       /* The Protocol code is 32   */
	    0x00U,                    /* The interface string descriptor index is 0 */

	    USB_DESCRIPTOR_LENGTH_INTERFACE,          /* Descriptor size is 9 bytes   */
	    USB_DESCRIPTOR_TYPE_INTERFACE,            /* INTERFACE Descriptor Type   */
	    USB_AUDIO_SPEAKER_STREAM_INTERFACE_INDEX, /* The number of this interface is 1.  */
	    0x01U, /* The value used to select the alternate setting for this interface is 1   */
	#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
	    0x01U, /* The number of endpoints used by this interface is 1 (excluding endpoint zero)    */
	#else
	    0x02U, /* The number of endpoints used by this interface is 2 (excluding endpoint zero)    */
	#endif
	    USB_AUDIO_CLASS,          /* The interface implements the Audio Interface class   */
	    USB_SUBCLASS_AUDIOSTREAM, /* The interface implements the AUDIOSTREAMING Subclass   */
	    USB_AUDIO_PROTOCOL,       /* The Protocol code is 32   */
	    0x00U,                    /* The interface string descriptor index is 2  */

	    0x10U,                                          /* Size of the descriptor, in bytes   */
	    USB_DESCRIPTOR_TYPE_AUDIO_CS_INTERFACE,         /* CS_INTERFACE Descriptor Type  */
	    USB_DESCRIPTOR_SUBTYPE_AUDIO_STREAMING_GENERAL, /* AS_GENERAL descriptor subtype   */
	    USB_AUDIO_SPEAKER_CONTROL_INPUT_TERMINAL_ID, /* The Terminal ID of the terminal to which this interface is connected
	                                                  */
	    0x00U,                                       /* bmControls : D1..0: Active Alternate Setting Control is not present
	                                                    D3..2: Valid Alternate Settings Control is not present
	                                                    D7..4: Reserved, should set to 0   */
	    0x01U,                      /* The format type AudioStreaming interfae using is FORMAT_TYPE_I (0x01)   */
	    0x01U, 0x00U, 0x00U, 0x00U, /* The Audio Data Format that can be Used to communicate with this interface */
	    AUDIO_OUT_FORMAT_CHANNELS,  /* Number of physical channels in the AS Interface audio channel cluster */
	    0x03U, 0x00U, 0x00U, 0x00U, /* Describes the spatial location of the logical channels: */
	    0x00U,                      /* Index of a string descriptor, describing the name of the first physical channel   */

	    0x06U,                                              /* Size of the descriptor, in bytes   */
	    USB_DESCRIPTOR_TYPE_AUDIO_CS_INTERFACE,             /* CS_INTERFACE Descriptor Type   */
	    USB_DESCRIPTOR_SUBTYPE_AUDIO_STREAMING_FORMAT_TYPE, /* FORMAT_TYPE descriptor subtype   */
	    0x01U, /* The format type AudioStreaming interfae using is FORMAT_TYPE_I (0x01)   */
		AUDIO_OUT_FORMAT_SIZE, /* The number of bytes occupied by one audio subslot. Can be 1, 2, 3 or 4.   */
		(8*AUDIO_OUT_FORMAT_SIZE), /* The number of effectively used bits from the available bits in an audio subslot   */

	#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
	    USB_AUDIO_STREAMING_ENDP_DESC_SIZE, /* Descriptor size is 7 bytes */
	    USB_DESCRIPTOR_TYPE_ENDPOINT,       /* ENDPOINT Descriptor Type*/
	    USB_AUDIO_SPEAKER_STREAM_ENDPOINT |
	        (USB_OUT
	         << USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT), /* This is an OUT endpoint with endpoint number 2   */
	    0x0D,                                                     /* Types -
	                                                                  Transfer: ISOCHRONOUS
	                                                                  Sync: Async
	                                                                  Usage: Data EP  */
	    USB_SHORT_GET_LOW(FS_ISO_OUT_ENDP_PACKET_SIZE),
	    USB_SHORT_GET_HIGH(FS_ISO_OUT_ENDP_PACKET_SIZE), /* Maximum packet size for this endpoint */
	    FS_ISO_OUT_ENDP_INTERVAL, /* The polling interval value is every 1 Frames. If Hi-Speed, 1 uFrames/NAK */
	#else
	    USB_AUDIO_STREAMING_ENDP_DESC_SIZE, /* Descriptor size is 7 bytes */
	    USB_DESCRIPTOR_TYPE_ENDPOINT,       /* ENDPOINT Descriptor Type*/
	    USB_AUDIO_SPEAKER_STREAM_ENDPOINT |
	        (USB_OUT
	         << USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT), /* This is an OUT endpoint with endpoint number 2   */
	    0x05U,                                                    /* Types -
	                                                                 Transfer: ISOCHRONOUS
	                                                                 Sync: Async
	                                                                 Usage: Data EP  */
	    USB_SHORT_GET_LOW(FS_ISO_OUT_ENDP_PACKET_SIZE + AUDIO_OUT_FORMAT_CHANNELS * AUDIO_OUT_FORMAT_SIZE),
	    USB_SHORT_GET_HIGH(FS_ISO_OUT_ENDP_PACKET_SIZE + AUDIO_OUT_FORMAT_CHANNELS * AUDIO_OUT_FORMAT_SIZE), /* Maximum packet size for this endpoint */
	    FS_ISO_OUT_ENDP_INTERVAL, /* The polling interval value is every 1 Frames. If Hi-Speed, 1 uFrames/NAK */
	#endif

	    /* Audio Class Specific ENDPOINT Descriptor  */
	    USB_AUDIO_CLASS_SPECIFIC_ENDPOINT_LENGTH, /* Size of the descriptor, 8 bytes  */
	    USB_AUDIO_STREAM_ENDPOINT_DESCRIPTOR,     /* CS_ENDPOINT Descriptor Type   */
	    USB_AUDIO_EP_GENERAL_DESCRIPTOR_SUBTYPE,  /* AUDIO_EP_GENERAL descriptor subtype */
	    0x00U, 0x00U, 0x00U, 0x00U, 0x00U,

	#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
	#else
	    /* Endpoint 3 Feedback ENDPOINT */
	    USB_AUDIO_STANDARD_AS_ISO_FEEDBACK_ENDPOINT_LENGTH, /* Descriptor size is 7 bytes */
	    USB_DESCRIPTOR_TYPE_ENDPOINT,                       /* bDescriptorType */
	    USB_AUDIO_SPEAKER_FEEDBACK_ENDPOINT |
	        (USB_IN << USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT), /* This is an IN endpoint with endpoint number 3 */
	    USB_ENDPOINT_ISOCHRONOUS | 0x10,                                 /*  Types -
	                                                                         Transfer: ISOCHRONOUS
	                                                                         Sync: Async
	                                                                         Usage: Feedback EP   */
	#if defined(USB_DEVICE_WORKAROUND_AUDIO_20_WINDOWS) && (USB_DEVICE_WORKAROUND_AUDIO_20_WINDOWS > 0)
	    0x04, 0x00,                                                      /* wMaxPacketSize */
	#else
	    0x03, 0x00, /* wMaxPacketSize */
	#endif
	    0x01,                                                            /* interval polling(2^x ms) */
	#endif
#else
#endif		//--- #if (USB_DEVICE_CONFIG_AUDIO_CLASS_2_0)



//-----next is for VCOM
//-----next is for VCOM
//-----next is for VCOM
//-----next is for VCOM
//-----next is for VCOM
//-----next is for VCOM
#if 1
	    /* Interface Association Descriptor */
	    /* Size of this descriptor in bytes */
	    USB_IAD_DESC_SIZE,
	    /* INTERFACE_ASSOCIATION Descriptor Type  */
	    USB_DESCRIPTOR_TYPE_INTERFACE_ASSOCIATION,
	    /* The first interface number associated with this function */
	    USB_CDC_VCOM_CIC_INTERFACE_INDEX,
	    /* The number of contiguous interfaces associated with this function */
	    USB_CDC_VCOM_INTERFACE_COUNT,
	    /* The function belongs to the Communication Device/Interface Class  */
	    USB_CDC_VCOM_CIC_CLASS, USB_CDC_VCOM_CIC_SUBCLASS,
	    /* The function uses the No class specific protocol required Protocol  */
	    0x00,
	    /* The Function string descriptor index */
	    0x05,

	    /* Communication Interface Descriptor */
	    USB_DESCRIPTOR_LENGTH_INTERFACE, USB_DESCRIPTOR_TYPE_INTERFACE, USB_CDC_VCOM_CIC_INTERFACE_INDEX, 0x00,
	    USB_CDC_VCOM_CIC_ENDPOINT_COUNT, USB_CDC_VCOM_CIC_CLASS, USB_CDC_VCOM_CIC_SUBCLASS, USB_CDC_VCOM_CIC_PROTOCOL, 0x00,

			/* CDC Class-Specific descriptor */
			USB_DESCRIPTOR_LENGTH_CDC_HEADER_FUNC, /* Size of this descriptor in bytes */
			USB_DESCRIPTOR_TYPE_CDC_CS_INTERFACE,  /* CS_INTERFACE Descriptor Type */
			USB_CDC_HEADER_FUNC_DESC, 0x10,
			0x01, /* USB Class Definitions for Communications the Communication specification version 1.10 */

			USB_DESCRIPTOR_LENGTH_CDC_CALL_MANAG, /* Size of this descriptor in bytes */
			USB_DESCRIPTOR_TYPE_CDC_CS_INTERFACE, /* CS_INTERFACE Descriptor Type */
			USB_CDC_CALL_MANAGEMENT_FUNC_DESC,
			0x01, /*Bit 0: Whether device handle call management itself 1, Bit 1: Whether device can send/receive call
					 management information over a Data Class Interface 0 */
			0x01, /* Indicates multiplexed commands are handled via data interface */

			USB_DESCRIPTOR_LENGTH_CDC_ABSTRACT,   /* Size of this descriptor in bytes */
			USB_DESCRIPTOR_TYPE_CDC_CS_INTERFACE, /* CS_INTERFACE Descriptor Type */
			USB_CDC_ABSTRACT_CONTROL_FUNC_DESC,
			0x06, /* Bit 0: Whether device supports the request combination of Set_Comm_Feature, Clear_Comm_Feature, and
					 Get_Comm_Feature 0, Bit 1: Whether device supports the request combination of Set_Line_Coding,
					 Set_Control_Line_State, Get_Line_Coding, and the notification Serial_State 1, Bit ...  */

			USB_DESCRIPTOR_LENGTH_CDC_UNION_FUNC, /* Size of this descriptor in bytes */
			USB_DESCRIPTOR_TYPE_CDC_CS_INTERFACE, /* CS_INTERFACE Descriptor Type */
			USB_CDC_UNION_FUNC_DESC,
			USB_CDC_VCOM_CIC_INTERFACE_INDEX, /* The interface number of the Communications or Data Class interface  */
			USB_CDC_VCOM_DIC_INTERFACE_INDEX, /* Interface number of subordinate interface in the Union  */

			/*Notification Endpoint descriptor */
			USB_DESCRIPTOR_LENGTH_ENDPOINT, USB_DESCRIPTOR_TYPE_ENDPOINT,
			USB_CDC_VCOM_CIC_INTERRUPT_IN_ENDPOINT | (USB_IN << 7U), USB_ENDPOINT_INTERRUPT,
			USB_SHORT_GET_LOW(FS_CDC_VCOM_INTERRUPT_IN_PACKET_SIZE), USB_SHORT_GET_HIGH(FS_CDC_VCOM_INTERRUPT_IN_PACKET_SIZE),
			FS_CDC_VCOM_INTERRUPT_IN_INTERVAL,

	    /* Data Interface Descriptor */
	    USB_DESCRIPTOR_LENGTH_INTERFACE, USB_DESCRIPTOR_TYPE_INTERFACE, USB_CDC_VCOM_DIC_INTERFACE_INDEX, 0x00,
	    USB_CDC_VCOM_DIC_ENDPOINT_COUNT, USB_CDC_VCOM_DIC_CLASS, USB_CDC_VCOM_DIC_SUBCLASS, USB_CDC_VCOM_DIC_PROTOCOL,
	    0x00, /* Interface Description String Index*/

			/*Bulk IN Endpoint descriptor */
			USB_DESCRIPTOR_LENGTH_ENDPOINT, USB_DESCRIPTOR_TYPE_ENDPOINT, USB_CDC_VCOM_DIC_BULK_IN_ENDPOINT | (USB_IN << 7U),
			USB_ENDPOINT_BULK, USB_SHORT_GET_LOW(FS_CDC_VCOM_BULK_IN_PACKET_SIZE),
			USB_SHORT_GET_HIGH(FS_CDC_VCOM_BULK_IN_PACKET_SIZE), 0x00, /* The polling interval value is every 0 Frames */

			/*Bulk OUT Endpoint descriptor */
			USB_DESCRIPTOR_LENGTH_ENDPOINT, USB_DESCRIPTOR_TYPE_ENDPOINT, USB_CDC_VCOM_DIC_BULK_OUT_ENDPOINT | (USB_OUT << 7U),
			USB_ENDPOINT_BULK, USB_SHORT_GET_LOW(FS_CDC_VCOM_BULK_OUT_PACKET_SIZE),
			USB_SHORT_GET_HIGH(FS_CDC_VCOM_BULK_OUT_PACKET_SIZE), 0x00, /* The polling interval value is every 0 Frames */
#endif
};
#endif

#if (defined(USB_DEVICE_CONFIG_CV_TEST) && (USB_DEVICE_CONFIG_CV_TEST > 0U))
USB_DMA_INIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE)
uint8_t g_UsbDeviceQualifierDescriptor[] = {
    USB_DESCRIPTOR_LENGTH_DEVICE_QUALITIER, /* Size of this descriptor in bytes */
    USB_DESCRIPTOR_TYPE_DEVICE_QUALITIER,   /* DEVICE Descriptor Type */
    USB_SHORT_GET_LOW(USB_DEVICE_SPECIFIC_BCD_VERSION),
    USB_SHORT_GET_HIGH(USB_DEVICE_SPECIFIC_BCD_VERSION), /* USB Specification Release Number in
                                                            Binary-Coded Decimal (i.e., 2.10 is 210H). */
    USB_DEVICE_CLASS,                                    /* Class code (assigned by the USB-IF). */
    USB_DEVICE_SUBCLASS,                                 /* Subclass code (assigned by the USB-IF). */
    USB_DEVICE_PROTOCOL,                                 /* Protocol code (assigned by the USB-IF). */
    USB_CONTROL_MAX_PACKET_SIZE,                         /* Maximum packet size for endpoint zero
                                                            (only 8, 16, 32, or 64 are valid) */
    0x00U,                                               /* Number of Other-speed Configurations */
    0x00U,                                               /* Reserved for future use, must be zero */
};
#endif

//string and language dscr
#if 1

/* Define string descriptor */
USB_DMA_INIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE)
uint8_t g_UsbDeviceString0[] = {
    2U + 2U,
    USB_DESCRIPTOR_TYPE_STRING,
    0x09U,
    0x04U,
};

USB_DMA_INIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE)
uint8_t g_UsbDeviceString1[] = {
    2U + 2U * 10U, USB_DESCRIPTOR_TYPE_STRING,
    'N',           0x00U,
    'x',           0x00U,
    'p',           0x00U,
    '-',           0x00U,
    '-',           0x00U,
    '-',           0x00U,
    '0',           0x00U,
    '0',           0x00U,
    '0',           0x00U,
    '1',           0x00U,
};


USB_DMA_INIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE)
uint8_t g_UsbDeviceString2[] =
{
	2U + 2U * 13U, USB_DESCRIPTOR_TYPE_STRING,
	'U',           0x00U,
	'S',           0x00U,
	'B',           0x00U,
	' ',           0x00U,
	'A',           0x00U,
	'U',           0x00U,
	'D',           0x00U,
	'I',           0x00U,
	'O',           0x00U,
	'+',           0x00U,
	'C',           0x00U,
	'O',           0x00U,
	'M',           0x00U,
};

USB_DMA_INIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE)
uint8_t g_UsbDeviceString3[] = {2U + 2U * 16U, USB_DESCRIPTOR_TYPE_STRING,
                                '0',           0x00U,
                                '1',           0x00U,
                                '2',           0x00U,
                                '3',           0x00U,
                                '4',           0x00U,
                                '5',           0x00U,
                                '6',           0x00U,
                                '7',           0x00U,
                                '8',           0x00U,
                                '9',           0x00U,
                                'A',           0x00U,
                                'B',           0x00U,
                                'C',           0x00U,
                                'D',           0x00U,
                                'E',           0x00U,
                                'F',           0x00U};

USB_DMA_INIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE)
uint8_t g_UsbDeviceString4[] = {2U + 2U * 18U, USB_DESCRIPTOR_TYPE_STRING,
                                'U',           0,
                                'S',           0,
                                'B',           0,
                                ' ',           0,
                                'C',           0,
                                'o',           0,
                                'm',           0,
                                'p',           0,
                                'o',           0,
                                's',           0,
                                'i',           0,
                                't',           0,
                                ' ',           0,
                                'A',           0,
                                'u',           0,
                                'd',           0,
                                'i',           0,
                                'o',           0};
USB_DMA_INIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE)
uint8_t g_UsbDeviceString5[] = {2U + 2U * 16U, USB_DESCRIPTOR_TYPE_STRING,
                                'U',           0,
                                'S',           0,
                                'B',           0,
                                ' ',           0,
                                'C',           0,
                                'o',           0,
                                'm',           0,
                                'p',           0,
                                'o',           0,
                                's',           0,
                                'i',           0,
                                't',           0,
                                ' ',           0,
                                'C',           0,
                                'O',           0,
                                'M',           0};
USB_DMA_INIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE)
uint8_t g_UsbDeviceString6[] = {2U + 2U * 17U, USB_DESCRIPTOR_TYPE_STRING,
                                'U',           0,
                                'S',           0,
                                'B',           0,
                                ' ',           0,
                                'C',           0,
                                'o',           0,
                                'm',           0,
                                'p',           0,
                                'o',           0,
                                's',           0,
                                'i',           0,
                                't',           0,
                                ' ',           0,
                                'M',           0,
                                'I',           0,
                                'D',           0,
                                'I',           0};
								
uint32_t g_UsbDeviceStringDescriptorLength[USB_DEVICE_STRING_COUNT] =
{
    sizeof(g_UsbDeviceString0),
	sizeof(g_UsbDeviceString1),
	sizeof(g_UsbDeviceString2),
	sizeof(g_UsbDeviceString3),
	sizeof(g_UsbDeviceString4),
	sizeof(g_UsbDeviceString5),
    sizeof(g_UsbDeviceString6)
};

uint8_t *g_UsbDeviceStringDescriptorArray[USB_DEVICE_STRING_COUNT] =
{
    g_UsbDeviceString0,
	g_UsbDeviceString1,
	g_UsbDeviceString2,
	g_UsbDeviceString3,
	g_UsbDeviceString4,
	g_UsbDeviceString5,
	g_UsbDeviceString6
};

usb_language_t g_UsbDeviceLanguage[USB_DEVICE_LANGUAGE_COUNT] = {{
    g_UsbDeviceStringDescriptorArray,
    g_UsbDeviceStringDescriptorLength,
    (uint16_t)0x0409U,
}};

usb_language_list_t g_UsbDeviceLanguageList = {
    g_UsbDeviceString0,
    sizeof(g_UsbDeviceString0),
    g_UsbDeviceLanguage,
    USB_DEVICE_LANGUAGE_COUNT,
};
#endif

/*******************************************************************************
 * Code
 ******************************************************************************/
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
                                           usb_device_get_device_descriptor_struct_t *deviceDescriptor)
{
    deviceDescriptor->buffer = g_UsbDeviceDescriptor;
    deviceDescriptor->length = USB_DESCRIPTOR_LENGTH_DEVICE;
    return kStatus_USB_Success;
}
#if (defined(USB_DEVICE_CONFIG_CV_TEST) && (USB_DEVICE_CONFIG_CV_TEST > 0U))
/* Get device qualifier descriptor request */
usb_status_t USB_DeviceGetDeviceQualifierDescriptor(
    usb_device_handle handle, usb_device_get_device_qualifier_descriptor_struct_t *deviceQualifierDescriptor)
{
    deviceQualifierDescriptor->buffer = g_UsbDeviceQualifierDescriptor;
    deviceQualifierDescriptor->length = USB_DESCRIPTOR_LENGTH_DEVICE_QUALITIER;
    return kStatus_USB_Success;
}
#endif
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
    usb_device_handle handle, usb_device_get_configuration_descriptor_struct_t *configurationDescriptor)
{
    if (USB_COMPOSITE_CONFIGURE_INDEX > configurationDescriptor->configuration)
    {
        configurationDescriptor->buffer = g_UsbDeviceConfigurationDescriptor;
        configurationDescriptor->length = USB_DESCRIPTOR_LENGTH_CONFIGURATION_ALL;
        return kStatus_USB_Success;
    }
    return kStatus_USB_InvalidRequest;
}

/*!
 * @brief USB device get string descriptor function.
 *
 * This function gets the string descriptor of the USB device.
 *
 * @param handle The USB device handle.
 * @param stringDescriptor The pointer to the string descriptor structure.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
usb_status_t USB_DeviceGetStringDescriptor(usb_device_handle handle,
                                           usb_device_get_string_descriptor_struct_t *stringDescriptor)
{
    if (stringDescriptor->stringIndex == 0U)
    {
        stringDescriptor->buffer = (uint8_t *)g_UsbDeviceLanguageList.languageString;
        stringDescriptor->length = g_UsbDeviceLanguageList.stringLength;
    }
    else
    {
        uint8_t languageId    = 0U;
        uint8_t languageIndex = USB_DEVICE_STRING_COUNT;

        for (; languageId < USB_DEVICE_LANGUAGE_COUNT; languageId++)
        {
            if (stringDescriptor->languageId == g_UsbDeviceLanguageList.languageList[languageId].languageId)
            {
                if (stringDescriptor->stringIndex < USB_DEVICE_STRING_COUNT)
                {
                    languageIndex = stringDescriptor->stringIndex;
                }
                break;
            }
        }

        if (USB_DEVICE_STRING_COUNT == languageIndex)
        {
            return kStatus_USB_InvalidRequest;
        }
        stringDescriptor->buffer = (uint8_t *)g_UsbDeviceLanguageList.languageList[languageId].string[languageIndex];
        stringDescriptor->length = g_UsbDeviceLanguageList.languageList[languageId].length[languageIndex];
    }
    return kStatus_USB_Success;
}

/* Due to the difference of HS and FS descriptors, the device descriptors and configurations need to be updated to match
 * current speed.
 * As the default, the device descriptors and configurations are configured by using FS parameters for both EHCI and
 * KHCI.
 * When the EHCI is enabled, the application needs to call this fucntion to update device by using current speed.
 * The updated information includes endpoint max packet size, endpoint interval, etc. */
usb_status_t USB_DeviceSetSpeed(usb_device_handle handle, uint8_t speed)
{
    usb_descriptor_union_t *descriptorHead;
    usb_descriptor_union_t *descriptorTail;

    descriptorHead = (usb_descriptor_union_t *)(&g_UsbDeviceConfigurationDescriptor[0]);
    descriptorTail = (usb_descriptor_union_t *)(&g_UsbDeviceConfigurationDescriptor[USB_DESCRIPTOR_LENGTH_CONFIGURATION_ALL - 1U]);

    while (descriptorHead < descriptorTail)
    {
        if (descriptorHead->common.bDescriptorType == USB_DESCRIPTOR_TYPE_ENDPOINT)
        {
            if (USB_SPEED_HIGH == speed)
            {
				//check Usb audio
#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
                if ((USB_AUDIO_SPEAKER_STREAM_ENDPOINT ==
                     (descriptorHead->endpoint.bEndpointAddress & USB_ENDPOINT_NUMBER_MASK)) &&
                    ((descriptorHead->endpoint.bEndpointAddress >> USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT) == USB_OUT))
                {
                    descriptorHead->endpoint.bInterval = HS_ISO_OUT_ENDP_INTERVAL;
                    USB_SHORT_TO_LITTLE_ENDIAN_ADDRESS( (HS_ISO_OUT_ENDP_PACKET_SIZE), descriptorHead->endpoint.wMaxPacketSize);
                }
#else
                i f ((USB_AUDIO_SPEAKER_STREAM_ENDPOINT ==
                     (descriptorHead->endpoint.bEndpointAddress & USB_ENDPOINT_NUMBER_MASK)) &&
                    ((descriptorHead->endpoint.bEndpointAddress >> USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT) == USB_OUT))
                {
                    descriptorHead->endpoint.bInterval = HS_ISO_OUT_ENDP_INTERVAL;
                    USB_SHORT_TO_LITTLE_ENDIAN_ADDRESS( (HS_ISO_OUT_ENDP_PACKET_SIZE + AUDIO_OUT_FORMAT_CHANNELS * AUDIO_OUT_FORMAT_SIZE), descriptorHead->endpoint.wMaxPacketSize);
                }
                else if ((USB_AUDIO_SPEAKER_FEEDBACK_ENDPOINT ==
                          (descriptorHead->endpoint.bEndpointAddress & USB_ENDPOINT_NUMBER_MASK)) &&
                         ((descriptorHead->endpoint.bEndpointAddress >> USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT) == USB_IN))
                {
                    descriptorHead->endpoint.bInterval = HS_ISO_IN_ENDP_INTERVAL;
                    USB_SHORT_TO_LITTLE_ENDIAN_ADDRESS(HS_ISO_FEEDBACK_ENDP_PACKET_SIZE,                                                   descriptorHead->endpoint.wMaxPacketSize);
                }
#endif
                else if ((USB_AUDIO_RECORDER_STREAM_ENDPOINT ==
                          (descriptorHead->endpoint.bEndpointAddress & USB_ENDPOINT_NUMBER_MASK)) &&
                         ((descriptorHead->endpoint.bEndpointAddress >> USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT) == USB_IN))
                {
                    descriptorHead->endpoint.bInterval = HS_ISO_IN_ENDP_INTERVAL;
                    USB_SHORT_TO_LITTLE_ENDIAN_ADDRESS((HS_ISO_IN_ENDP_PACKET_SIZE + AUDIO_IN_FORMAT_CHANNELS * AUDIO_IN_FORMAT_SIZE),     descriptorHead->endpoint.wMaxPacketSize);
                }
				//check VCOM
                else if ((USB_CDC_VCOM_CIC_INTERRUPT_IN_ENDPOINT ==
                          (descriptorHead->endpoint.bEndpointAddress & USB_ENDPOINT_NUMBER_MASK)) &&
                         ((descriptorHead->endpoint.bEndpointAddress & USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_MASK) ==
                          USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_IN))
                {
                    descriptorHead->endpoint.bInterval = HS_CDC_VCOM_INTERRUPT_IN_INTERVAL;
                    USB_SHORT_TO_LITTLE_ENDIAN_ADDRESS(HS_CDC_VCOM_INTERRUPT_IN_PACKET_SIZE,
                                                       descriptorHead->endpoint.wMaxPacketSize);
                }
                else if ((USB_CDC_VCOM_DIC_BULK_IN_ENDPOINT ==
                          (descriptorHead->endpoint.bEndpointAddress & USB_ENDPOINT_NUMBER_MASK)) &&
                         ((descriptorHead->endpoint.bEndpointAddress & USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_MASK) ==
                          USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_IN))
                {
                    USB_SHORT_TO_LITTLE_ENDIAN_ADDRESS(HS_CDC_VCOM_BULK_IN_PACKET_SIZE, descriptorHead->endpoint.wMaxPacketSize);
                }
                else if ((USB_CDC_VCOM_DIC_BULK_OUT_ENDPOINT ==
                          (descriptorHead->endpoint.bEndpointAddress & USB_ENDPOINT_NUMBER_MASK)) &&
                         ((descriptorHead->endpoint.bEndpointAddress & USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_MASK) ==
                          USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_OUT))
                {
                    USB_SHORT_TO_LITTLE_ENDIAN_ADDRESS(HS_CDC_VCOM_BULK_OUT_PACKET_SIZE, descriptorHead->endpoint.wMaxPacketSize);
                }
                else
                {
                }
            }
            else
            {
#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
                if ((USB_AUDIO_SPEAKER_STREAM_ENDPOINT ==
                     (descriptorHead->endpoint.bEndpointAddress & USB_ENDPOINT_NUMBER_MASK)) &&
                    ((descriptorHead->endpoint.bEndpointAddress >> USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT) == USB_OUT))
                {
                    descriptorHead->endpoint.bInterval = FS_ISO_OUT_ENDP_INTERVAL;
                    USB_SHORT_TO_LITTLE_ENDIAN_ADDRESS( (FS_ISO_OUT_ENDP_PACKET_SIZE),                                                     descriptorHead->endpoint.wMaxPacketSize);
                }
#else
                i f ((USB_AUDIO_SPEAKER_STREAM_ENDPOINT ==
                     (descriptorHead->endpoint.bEndpointAddress & USB_ENDPOINT_NUMBER_MASK)) &&
                    ((descriptorHead->endpoint.bEndpointAddress >> USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT) == USB_OUT))
                {
                    descriptorHead->endpoint.bInterval = FS_ISO_OUT_ENDP_INTERVAL;
                    USB_SHORT_TO_LITTLE_ENDIAN_ADDRESS( (FS_ISO_OUT_ENDP_PACKET_SIZE + AUDIO_OUT_FORMAT_CHANNELS * AUDIO_OUT_FORMAT_SIZE), descriptorHead->endpoint.wMaxPacketSize);
                }
                else if ((USB_AUDIO_SPEAKER_FEEDBACK_ENDPOINT ==
                          (descriptorHead->endpoint.bEndpointAddress & USB_ENDPOINT_NUMBER_MASK)) &&
                         ((descriptorHead->endpoint.bEndpointAddress >> USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT) == USB_IN))
                {
                    descriptorHead->endpoint.bInterval = FS_ISO_IN_ENDP_INTERVAL;
                    USB_SHORT_TO_LITTLE_ENDIAN_ADDRESS(FS_ISO_FEEDBACK_ENDP_PACKET_SIZE,                                                  descriptorHead->endpoint.wMaxPacketSize);
                }
#endif
                else if ((USB_AUDIO_RECORDER_STREAM_ENDPOINT ==
                          (descriptorHead->endpoint.bEndpointAddress & USB_ENDPOINT_NUMBER_MASK)) &&
                         ((descriptorHead->endpoint.bEndpointAddress >> USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT) == USB_IN))
                {
                    descriptorHead->endpoint.bInterval = FS_ISO_IN_ENDP_INTERVAL;
                    USB_SHORT_TO_LITTLE_ENDIAN_ADDRESS( (FS_ISO_IN_ENDP_PACKET_SIZE + AUDIO_IN_FORMAT_CHANNELS * AUDIO_IN_FORMAT_SIZE),   descriptorHead->endpoint.wMaxPacketSize);
                }
                else if ((USB_CDC_VCOM_CIC_INTERRUPT_IN_ENDPOINT ==
                          (descriptorHead->endpoint.bEndpointAddress & USB_ENDPOINT_NUMBER_MASK)) &&
                         ((descriptorHead->endpoint.bEndpointAddress & USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_MASK) ==                           USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_IN))
                {
                    descriptorHead->endpoint.bInterval = FS_CDC_VCOM_INTERRUPT_IN_INTERVAL;
                    USB_SHORT_TO_LITTLE_ENDIAN_ADDRESS(FS_CDC_VCOM_INTERRUPT_IN_PACKET_SIZE,                                                        descriptorHead->endpoint.wMaxPacketSize);
                }
                else if ((USB_CDC_VCOM_DIC_BULK_IN_ENDPOINT ==
                          (descriptorHead->endpoint.bEndpointAddress & USB_ENDPOINT_NUMBER_MASK)) &&
                         ((descriptorHead->endpoint.bEndpointAddress & USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_MASK) ==                           USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_IN))
                {
                    USB_SHORT_TO_LITTLE_ENDIAN_ADDRESS(FS_CDC_VCOM_BULK_IN_PACKET_SIZE, descriptorHead->endpoint.wMaxPacketSize);
                }
                else if ((USB_CDC_VCOM_DIC_BULK_OUT_ENDPOINT ==
                          (descriptorHead->endpoint.bEndpointAddress & USB_ENDPOINT_NUMBER_MASK)) &&
                         ((descriptorHead->endpoint.bEndpointAddress & USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_MASK) ==                           USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_OUT))
                {
                    USB_SHORT_TO_LITTLE_ENDIAN_ADDRESS(FS_CDC_VCOM_BULK_OUT_PACKET_SIZE, descriptorHead->endpoint.wMaxPacketSize);
                }
                else
                {
                }
            }
        }
        descriptorHead = (usb_descriptor_union_t *)((uint8_t *)descriptorHead + descriptorHead->common.bLength);
    }

	if (USB_SPEED_HIGH == speed)
	{
		#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
			g_UsbDeviceAudioSpeakerEndpoints[0].maxPacketSize = (HS_ISO_OUT_ENDP_PACKET_SIZE);
			g_UsbDeviceAudioSpeakerEndpoints[0].interval      = (HS_ISO_OUT_ENDP_INTERVAL);
		#else
			g_UsbDeviceAudioSpeakerEndpoints[0].maxPacketSize = (HS_ISO_OUT_ENDP_PACKET_SIZE + AUDIO_OUT_FORMAT_CHANNELS * AUDIO_OUT_FORMAT_SIZE);
			g_UsbDeviceAudioSpeakerEndpoints[0].interval      = (HS_ISO_OUT_ENDP_INTERVAL);
			g_UsbDeviceAudioSpeakerEndpoints[1].maxPacketSize = HS_ISO_FEEDBACK_ENDP_PACKET_SIZE;
		#endif
	}
	else
	{
		#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
			g_UsbDeviceAudioSpeakerEndpoints[0].maxPacketSize = (FS_ISO_OUT_ENDP_PACKET_SIZE);
			g_UsbDeviceAudioSpeakerEndpoints[0].interval      = (FS_ISO_OUT_ENDP_INTERVAL);
		#else
			g_UsbDeviceAudioSpeakerEndpoints[0].maxPacketSize = (FS_ISO_OUT_ENDP_PACKET_SIZE + AUDIO_OUT_FORMAT_CHANNELS * AUDIO_OUT_FORMAT_SIZE);
			g_UsbDeviceAudioSpeakerEndpoints[0].interval      = (FS_ISO_OUT_ENDP_INTERVAL);
			g_UsbDeviceAudioSpeakerEndpoints[1].maxPacketSize = FS_ISO_FEEDBACK_ENDP_PACKET_SIZE;
		#endif
	}
	if (USB_SPEED_HIGH == speed)
	{
		g_UsbDeviceAudioRecorderEndpoints[0].maxPacketSize = (HS_ISO_IN_ENDP_PACKET_SIZE + AUDIO_IN_FORMAT_CHANNELS * AUDIO_IN_FORMAT_SIZE);
		g_UsbDeviceAudioRecorderEndpoints[0].interval      = (HS_ISO_IN_ENDP_INTERVAL);
	}
	else
	{
		g_UsbDeviceAudioRecorderEndpoints[0].maxPacketSize = (FS_ISO_IN_ENDP_PACKET_SIZE + AUDIO_IN_FORMAT_CHANNELS * AUDIO_IN_FORMAT_SIZE);
		g_UsbDeviceAudioRecorderEndpoints[0].interval      = (FS_ISO_IN_ENDP_INTERVAL);
	}
	if (USB_SPEED_HIGH == speed)
	{
		g_UsbDeviceAudioControlEndpoints[0].maxPacketSize = (HS_AUDIO_INTERRUPT_IN_PACKET_SIZE);
		g_UsbDeviceAudioControlEndpoints[0].interval      = (HS_AUDIO_INTERRUPT_IN_INTERVAL);
	}
	else
	{
		g_UsbDeviceAudioControlEndpoints[0].maxPacketSize = (FS_AUDIO_INTERRUPT_IN_PACKET_SIZE);
		g_UsbDeviceAudioControlEndpoints[0].interval      = (FS_AUDIO_INTERRUPT_IN_INTERVAL);
	}

    for (int i = 0; i < USB_CDC_VCOM_CIC_ENDPOINT_COUNT; i++)
    {
        if (USB_SPEED_HIGH == speed)
        {
            g_cdcVcomCicEndpoints[i].maxPacketSize = HS_CDC_VCOM_INTERRUPT_IN_PACKET_SIZE;
            g_cdcVcomCicEndpoints[i].interval      = HS_CDC_VCOM_INTERRUPT_IN_INTERVAL;
        }
        else
        {
            g_cdcVcomCicEndpoints[i].maxPacketSize = FS_CDC_VCOM_INTERRUPT_IN_PACKET_SIZE;
            g_cdcVcomCicEndpoints[i].interval      = FS_CDC_VCOM_INTERRUPT_IN_INTERVAL;
        }
    }

    for (int i = 0; i < USB_CDC_VCOM_DIC_ENDPOINT_COUNT; i++)
    {
        if (USB_SPEED_HIGH == speed)
        {
            if (g_cdcVcomDicEndpoints[i].endpointAddress & USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_MASK)
            {
                g_cdcVcomDicEndpoints[i].maxPacketSize = HS_CDC_VCOM_BULK_IN_PACKET_SIZE;
            }
            else
            {
                g_cdcVcomDicEndpoints[i].maxPacketSize = HS_CDC_VCOM_BULK_OUT_PACKET_SIZE;
            }
        }
        else
        {
            if (g_cdcVcomDicEndpoints[i].endpointAddress & USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_MASK)
            {
                g_cdcVcomDicEndpoints[i].maxPacketSize = FS_CDC_VCOM_BULK_IN_PACKET_SIZE;
            }
            else
            {
                g_cdcVcomDicEndpoints[i].maxPacketSize = FS_CDC_VCOM_BULK_OUT_PACKET_SIZE;
            }
        }
    }

    return kStatus_USB_Success;
}

#endif
