/*
 * Copyright 2021 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>
#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "ff.h"
#include <stdbool.h>

#include <porting.h>
#include <sys/atomic.h>
#include <sys/byteorder.h>
#include <sys/util.h>
#include <sys/slist.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include "sco_audio_pl.h"
#include <bluetooth/hfp_ag.h>
#include <bluetooth/hfp_hf.h>
#include "fsl_debug_console.h"
#include "fsl_shell.h"
#include "app_shell.h"

#include "app_discover.h"
#include "app_connect.h"
#include "app_avrcp.h"
#include "app_handsfree.h"
#include "display_interface.h"
#include "app_dcc.h"

#include "GlobalDef.h"
#include "CircularBufManagement.h"
#include "CircularBuf.h"
#include "SubFunc.h"
#include "WorkStateManager.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
static shell_status_t shellBt(shell_handle_t shellHandle, int32_t argc, char **argv);

/*******************************************************************************
 * Variables
 ******************************************************************************/

SHELL_COMMAND_DEFINE(bt,
					 "playopus 1~3"
					 "playsbc  1~3"
                     "\r\n\"bt\": BT related function\r\n"
                     "  USAGE: bt [dial|aincall|eincall]\r\n"
                     "    dial          dial out call.\r\n"
                     "    aincall       accept the incoming call.\r\n"
                     "    eincall       end an incoming call.\r\n"
                     "    svr           start voice recognition.\r\n"
                     "    evr           stop voice recognition.\r\n"
                     "    clip          enable CLIP notification.\r\n"
                     "    disclip       disable CLIP notification.\r\n"
                     "    ccwa          enable call waiting notification.\r\n"
                     "    disccwa       disable call waiting notification.\r\n"
                     "    micVolume     Update mic Volume.\r\n"
                     "    speakerVolume Update Speaker Volume.\r\n"
                     "    lastdial      call the last dial number.\r\n"
                     "    voicetag      Get Voice-tag Phone Number (BINP).\r\n"
                     "    multipcall    multiple call option.\r\n"
                     "    triggercodec  trigger codec connection.\r\n"
                     "    clcc          Query list of current calls.\r\n"
                     "    getIndicatorStatus Get peer's indicators' status.\r\n"

                     "  USAGE: bt [discover|connect|disconnect|delete_all|paired_list]\r\n"
					 "    discover             start to find Bluetooth devices\r\n"
					 "    connect <device_index>   \n"
					 "                         connect to the device that is found in discovery,\r\n"
                     "                         bt connect <device index> index is device index from discovery list displayed \r\n"
					 "						   bt connect <device index> PH/ph connect as Passenger headset, default is Rider headset"
                  	 "    connect_paired  <device_index> \n"
                  	 "                         The <device index> is from paired device list displayed"
                     "    disconnect           disconnect Bluetooth link \r\n"
                     "                         bt disconnect <device type> \r\n"
                     "                         For example:\r\n"
                     "                         bt disconnect M/m -> for Rider mobile device disconnection\r\n"
                     "                         bt disconnect H/h -> for Rider headset disconnection\r\n"
					 "						   bt disconnect PH/ph -> for Passenger headset disconnection\r\n"
                     "    delete_all           delete all devices. Ensure to disconnect the HCI link connection with the peer "
                     "                         device before attempting to delete the bonding information.\r\n"
		             "    delete_dev           bt delete_dev <index> To delete particular device from paired list\r\n"
		             "    paired_list          Get the paired devices\r\n"



		             "\r\n\"bt\": AVRCP related function\r\n"
                     "    USAGE: bt [play|pause|volume up|volume down|next|previous]\r\n"
                     "    play                 to play the song\r\n"
		             "    pause                to pause the song for\r\n"
		             "    volume up            increase the volume\r\n"
		             "    volume down          decrease the volume\r\n"
		             "    next                 goto the next song\r\n"
		             "    previous             goto the previous song\r\n"
		             "    get_song_detail      get current playing song details"
		             "    hs_play              <option>  to start music in DUAL A2DP mode"
		             "                         h: start play with rider headset  ph: with passenger headset"
					 "    hs_stop              <option>  to stop music in DUAL A2DP mode"
				     "                         h: stop play with rider headset   ph: with passenger headset",

                     shellBt,
                     SHELL_IGNORE_PARAMETER_COUNT);

SDK_ALIGN(static uint8_t s_shellHandleBuffer[SHELL_HANDLE_SIZE], 4);
static shell_handle_t s_shellHandle;

/*******************************************************************************
 * Code
 ******************************************************************************/
static uint32_t hfp_get_value_from_str(char *ch)
{
      uint8_t selectIndex = 0;
      uint8_t value = 0;

      for (selectIndex = 0; selectIndex < strlen(ch); ++selectIndex)
      {
          if ((ch[selectIndex] < '0') || (ch[selectIndex] > '9'))
          {
              PRINTF("The Dial parameter is wrong\r\n");
              return kStatus_SHELL_Error;
          }
      }

      if (selectIndex == 0U)
      {
          PRINTF("The Dial parameter is wrong\r\n");
      }
      else if(selectIndex == 1U)
      {
        value = (ch[0] - '0');
      }
      else if(selectIndex == 2U)
      {
        value = (ch[0] - '0')*10 + (ch[1] - '0');
      }
      return value;
}


static shell_status_t shellBt(shell_handle_t shellHandle, int32_t argc, char **argv)
{
    uint8_t *addr;

    if (argc < 1)
    {
        PRINTF("the parameter count is wrong\r\n");
    }

    if (strcmp(argv[1], "discover") == 0)
    {
        app_discover();
    }
    else if (strcmp(argv[1], "discovery") == 0)
    {
    	bt_br_set_connectable(false);
        if (bt_br_set_connectable(true))
        {
            PRINTF("BR/EDR set/reset connectable failed\n");
            return kStatus_SHELL_Error;
        }
        if (bt_br_set_discoverable(true))
        {
            PRINTF("BR/EDR set discoverable failed\n");
            return kStatus_SHELL_Error;
        }
        PRINTF("BR/EDR set connectable and discoverable done\n");
    }
    else if (strcmp(argv[1], "connect") == 0)
    {
        uint8_t selectIndex = 0;
        char *ch            = argv[2];

        if (argc < 2)
        {
        	PRINTF("Invalid index parameter.\n");
            return kStatus_SHELL_Error;
        }

        for (selectIndex = 0; selectIndex < strlen(ch); ++selectIndex)
        {
            if ((ch[selectIndex] < '0') || (ch[selectIndex] > '9'))
            {
                PRINTF("the parameter is wrong\r\n");
                return kStatus_SHELL_Error;
            }
        }

        switch (strlen(ch))
        {
            case 1:
                selectIndex = ch[0] - '0';
                break;
            case 2:
                selectIndex = (ch[0] - '0') * 10 + (ch[1] - '0');
                break;
            default:
            	PRINTF("Invalid index parameter.\n");
            	return kStatus_SHELL_Error;
        }

        if (selectIndex == 0U || selectIndex > APP_INQUIRY_NUM_RESPONSES)
        {
        	PRINTF("Invalid index range. Choose between 1 and %d\n", APP_INQUIRY_NUM_RESPONSES);
        	return kStatus_SHELL_Error;
        }

        //Retrieve device address and COD from discovery list
        uint8_t *addr = br_discovery_results[selectIndex - 1].addr.val;
        uint8_t *cod = br_discovery_results[selectIndex - 1].cod;

        //Get device type using COD
        uint8_t device_type = get_device_type_from_cod(cod);

       // PRINTF("Device [%d]: Address: %02X:%02X:%02X:%02X:%02X:%02X, Type: %s\n",
       //selectIndex, addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], device_type);


        if (g_pairedDeviceCount >= MAX_PAIRED_DEVICES)
        {
        	PRINTF("Paired devices reached max number, \n Please remove an existing device to pair new device.\n");
        	print_paired_devices();
        	return kStatus_SHELL_Error;
        }

        //Automatically connect if it's a Mobile Phone or Headset
        if (device_type == PHONE)
        {
        	app_connect(RIDER_PHONE,addr);

        }else if (device_type == HEADSET)
        {
        	if((strcmp(argv[3], "PH") == 0) || (strcmp(argv[3], "ph") == 0))
        	{
        		PRINTF("Connecting PASSENGER_HEADSET \n");
        		app_connect(PASSENGER_HEADSET, addr);

        	}else
        	{
        		app_connect(RIDER_HEADSET, addr);
        	}

        }else
        {
        	PRINTF("Device is not a Mobile Phone or Headset.\n");
        }
    }
    else if (strcmp(argv[1], "connect_paired") == 0)
    {
		if (argc < 2)
		{
			PRINTF("the parameter count is wrong\r\n");
			return kStatus_SHELL_Error;
		}
		uint8_t device_index = 0;
		char *ch = argv[2];
		device_index = ch[0] - '0';

		connect_paired_device(device_index);
    }
    else if (strcmp(argv[1], "disconnect") == 0)
    {
    	if((strcmp(argv[2], "M") == 0) || (strcmp(argv[2], "m") == 0))
    	{
    		app_disconnect(RIDER_PHONE);
    	}
    	else if((strcmp(argv[2], "H") == 0) || (strcmp(argv[2], "h") == 0))
    	{
    		app_disconnect(RIDER_HEADSET);

    	}else if((strcmp(argv[2], "PH") == 0) || (strcmp(argv[2], "ph") == 0))
    	{
    		app_disconnect(PASSENGER_HEADSET);
    	}
    	else
    	{
    		printf("Enter valid argument\n"
    				"M or m to disconnect rider phone\n"
    				"H or h to disconnect rider headset\n"
    				"PH or ph to disconnect passenger headset\n");
    	}

    }
//**************AVRCP commands start ***************
    else if (strcmp(argv[1], "play") == 0)
    {
			uint8_t select_op = 0;
			char *ch = argv[2];
			if (argc < 2)
					{
						PRINTF("the parameter count is wrong\r\n");
						return kStatus_SHELL_Error;
					}
			select_op = ch[0] - '0';
			//avrcp_play_button(select_op);
	}
    else if (strcmp(argv[1], "pause") == 0)
	{
			uint8_t select_op = 0;
			char *ch = argv[2];
			if (argc < 2)
					{
						PRINTF("the parameter count is wrong\r\n");
						return kStatus_SHELL_Error;
					}
			select_op = ch[0] - '0';

			//avrcp_pause_button(select_op);
	}
    else if (strcmp(argv[1], "next") == 0)
    {
          	uint8_t select_op = 0;
          	char *ch = argv[2];
          	if (argc < 2)
          	        {
          	            PRINTF("the parameter count is wrong\r\n");
          	            return kStatus_SHELL_Error;
          	        }
         // 	select_op = ch[0] - '0';
          	//avrcp_forward_backward(1);
    }

    else if (strcmp(argv[1], "previous") == 0)
    {
          	uint8_t select_op = 0;
          	char *ch = argv[2];
          	if (argc < 2)
          	        {
          	            PRINTF("the parameter count is wrong\r\n");
          	            return kStatus_SHELL_Error;
          	        }
         // 	select_op = ch[0] - '0';

          	//avrcp_forward_backward(0);
    }
    else if (strcmp(argv[1], "volume") == 0)
    {
              	uint8_t select_op = 0;
              	char *ch = argv[2];
              	if (argc < 2)
              	        {
              	            PRINTF("the parameter count is wrong\r\n");
              	            return kStatus_SHELL_Error;
              	        }
              	if (strcmp(argv[2], "up") == 0)
              	{
              		avrcp_volume_up_down(1);

              	}else if (strcmp(argv[2], "down") == 0)
              	{
              		avrcp_volume_up_down(0);
              	}
    }

//**************AVRCP commands end***************

	else if (strcmp(argv[1], "delete_all") == 0)
    {
		int err = 0;

		if (conn_rider_phone != NULL)
		{
			app_disconnect(RIDER_PHONE);
			while(conn_rider_phone != NULL);
		}

		#if !((defined AUTO_CONNECT_USE_BOND_INFO) && (AUTO_CONNECT_USE_BOND_INFO))
			/*First need to read the paired device*/
			if (!app_read_paired_devices())
			{
				uint8_t addr[6];
				PRINTF("Number of paired device count is %d\n", g_pairedDeviceCount);
				for(int i = 0;i < g_pairedDeviceCount; i++)
				{
					PRINTF("[%d] Address: %02X:%02X:%02X:%02X:%02X:%02X, Name: %s, Type: %d\n",
							i + 1,
							paired_devices[i].addr[0], paired_devices[i].addr[1], paired_devices[i].addr[2],
							paired_devices[i].addr[3], paired_devices[i].addr[4], paired_devices[i].addr[5],
							paired_devices[i].name, paired_devices[i].device_type);

					if (memcmp(paired_devices[i].addr, addr, 6) == 0)
					{
						bt_unpair(BT_ID_DEFAULT,(bt_addr_le_t *)addr);
					}
				}
				PRINTF("clear_paired_devices_from_lfs.\n\n");
				vTaskDelay(pdMS_TO_TICKS(50));
				app_clear_paired_devices();
			}
		#endif
    }
	else if (strcmp(argv[1], "delete_dev") == 0)
	{
        if (argc < 2)
        {
            PRINTF("the parameter count is wrong\r\n");
            return kStatus_SHELL_Error;
        }
		uint8_t device_index = 0;
		char *ch = argv[2];
		device_index = ch[0] - '0';

		if ((paired_devices[device_index - 1].device_type == RIDER_PHONE) &&
				(conn_rider_phone != NULL))
		{
			app_disconnect(RIDER_PHONE);
			PRINTF("Disconnecting device...\n");
			while(conn_rider_phone != NULL);
		}

		PRINTF("delete_device:%d\n",device_index);
		vTaskDelay(pdMS_TO_TICKS(50));
		delete_device(device_index);
	}
	else if (strcmp(argv[1], "paired_list") == 0)
    {
		print_paired_devices();
    }
    else if (strcmp(argv[1], "dial") == 0)
    {
        uint8_t selectIndex = 0;
        char *ch            = argv[2];

        if (argc < 2)
        {
            PRINTF("the parameter count is wrong\r\n");
            return kStatus_SHELL_Error;
        }

        for (selectIndex = 0; selectIndex < strlen(ch); ++selectIndex)
        {
            if ((ch[selectIndex] < '0') || (ch[selectIndex] > '9'))
            {
                PRINTF("The Dial parameter is wrong\r\n");
                return kStatus_SHELL_Error;
            }
        }

        if (selectIndex == 0U)
        {
            PRINTF("The Dial parameter is wrong\r\n");
        }
        hfp_dial(ch);
    }
    else if (strcmp(argv[1], "svr") == 0)
    {
        hfp_start_voice_recognition();
    }
    else if (strcmp(argv[1], "evr") == 0)
    {
        hfp_stop_voice_recognition();
    }
#if 0	
     else if (strcmp(argv[1], "micVolume") == 0)
    {
        if (argc < 2)
        {
            PRINTF("the parameter count is wrong\r\n");
            return kStatus_SHELL_Error;
        }
//        hfp_volume_update(hf_volume_type_mic, hfp_get_value_from_str(argv[2]));

    }
    else if (strcmp(argv[1], "speakerVolume") == 0)
    {
        if (argc < 2)
        {
            PRINTF("the parameter count is wrong\r\n");
            return kStatus_SHELL_Error;
        }
//        hfp_volume_update(hf_volume_type_speaker, hfp_get_value_from_str(argv[2]));
    }
#endif
   else if (strcmp(argv[1], "lastdial") == 0)
   {
      hfp_last_dial();
    }
   else if (strcmp(argv[1], "memorydial") == 0)
   {
      if (argc < 2)
      {
          PRINTF("the parameter count is wrong\r\n");
          return kStatus_SHELL_Error;
      }
      dial_memory(hfp_get_value_from_str(argv[2]));
    }
    else if (strcmp(argv[1], "clip") == 0)
    {
       hfp_enable_clip(1);
    }
    else if (strcmp(argv[1], "disclip") == 0)
    {
       hfp_enable_clip(0);
    }
    else if (strcmp(argv[1], "ccwa") == 0)
    {
       hfp_enable_ccwa(1);
    }
    else if (strcmp(argv[1], "disccwa") == 0)
    {
       hfp_enable_ccwa(0);
    }
     else if (strcmp(argv[1], "multipcall") == 0)
    {
        if (argc < 2)
        {
            PRINTF("the parameter count is wrong\r\n");
            return kStatus_SHELL_Error;
        }
        hfp_multiparty_call_option(hfp_get_value_from_str(argv[2]));
    }

    else if (strcmp(argv[1], "aincall") == 0)
    {

      hfp_AnswerCall();
    }
    else if (strcmp(argv[1], "voicetag") == 0)
    {

      hfp_hf_get_last_voice_tag_number();
    }

    else if (strcmp(argv[1], "eincall") == 0)
    {
      hfp_RejectCall();
    }
    else if (strcmp(argv[1], "triggercodec") == 0)
    {
        hfp_trigger_codec_connection();
    }
    else if (strcmp(argv[1], "getIndicatorStatus") == 0)
    {
        bt_hfp_hf_get_peer_indicator_status(conn_rider_phone);
    }
    else if(strcmp(argv[1], "playopus") == 0)
    {
    	//start opus play
    	if(strcmp(argv[2], "1") == 0)
    	{
    		//play the first opus file
    		VarBlockSharedByDspAndMcu.NeedToStartPlayOpus=1;
    		VarBlockSharedByDspAndMcu.PlayOpusFileIdx=0;
    	}
    	if(strcmp(argv[2], "2") == 0)
    	{
    		//play the second opus file
    		VarBlockSharedByDspAndMcu.NeedToStartPlayOpus=1;
    		VarBlockSharedByDspAndMcu.PlayOpusFileIdx=1;
    	}
    	if(strcmp(argv[2], "3") == 0)
    	{
    		//play the second opus file
    		VarBlockSharedByDspAndMcu.NeedToStartPlayOpus=1;
    		VarBlockSharedByDspAndMcu.PlayOpusFileIdx=2;
    	}
    }
    else if(strcmp(argv[1], "playsbc") == 0)
    {
    	//start sbc play
    	if(strcmp(argv[2], "1") == 0)
        {
    		//play the first sbc file
    		VarBlockSharedByDspAndMcu.NeedToStartPlaySbc=1;
    		VarBlockSharedByDspAndMcu.PlaySbcFileIdx=0;
        }
    	if(strcmp(argv[2], "2") == 0)
        {
    		//play the second sbc file
    		VarBlockSharedByDspAndMcu.NeedToStartPlaySbc=1;
    		VarBlockSharedByDspAndMcu.PlaySbcFileIdx=1;
        }
    	if(strcmp(argv[2], "3") == 0)
        {
    		//play the second sbc file
    		VarBlockSharedByDspAndMcu.NeedToStartPlaySbc=1;
    		VarBlockSharedByDspAndMcu.PlaySbcFileIdx=2;
        }
    }
    else if(strcmp(argv[1], "mvol") == 0)
	{
    	int level = atoi(argv[2]);   // convert string to int

		if(level >= 0 && level <= 15)
		{
			ChangeMasterVolumeLevel16(level);
		}
		else
		{
			PRINTF("Invalid volume level. Use: 0 ~ 15\r\n");
		}
	}
    else
    {
    	PRINTF("invalid parameter\n");
    }

/*
	U32 NeedToStartPlayOpus;
	U32 PlayOpusFileIdx;
	U32 NeedToStartPlaySbc;
	U32 PlaySbcFileIdx;
*/

    return kStatus_SHELL_Success;
}

void app_shell_init(void)
{
    DbgConsole_Flush();
    /* Init SHELL */
    s_shellHandle = &s_shellHandleBuffer[0];
    SHELL_Init(s_shellHandle, g_serialHandle, ">> ");
    PRINTF("\r\n");

    /* Add new command to commands list */
    SHELL_RegisterCommand(s_shellHandle, SHELL_COMMAND(bt));
}
