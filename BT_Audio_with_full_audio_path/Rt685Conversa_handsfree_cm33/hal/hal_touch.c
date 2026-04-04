/*
 * hal_touch.c
 *
 *  Created on: Oct 21, 2025
 *      Author: Lydia
 */
#if UsingQAR87Board == 1
#include "hal_touch.h"
#include "aw93305.h"
#include "aw933xx.h"
#include "system_status.h"
#include "WorkStateManager.h"
#include "elan_ewd608.h"


extern volatile struct aw933xx_dev aw933xx;
extern uint8_t Novatek_boot_completed;

static uint8_t has_touch_event = 0;

#if TOUCH_EWD608_ENABLE
static uint8_t hal_touch_ewd608_get_gesture_state = 0;
static const uint8_t data_reg = 0xC0;
static uint8_t buf[EWD_FRAME_MAX_LEN];
#endif


void hal_touch_aw93305_init(void)
{
#if TOUCH_AW93305_ENABLE
	awinic_single_enter();
#endif
}

void hal_touch_aw93305_handler(void)
{
#if TOUCH_AW93305_ENABLE
	AW93305_EXTI_Callback();

	if(aw933xx.event.click >0)
	{
		unsigned int btn_event = aw933xx.event.click;
		PRINTF("[Touch] click= %d \n",btn_event);
		if(btn_event==1)
		{
#if SOC_SPI_ENABLE
			if (Novatek_boot_completed && !get_music_status() && (ss_get_state() == USAGE_STATE_MEDIA_PLAYER ||
					ss_get_state() == USAGE_STATE_MENU || ss_get_state() == USAGE_STATE_HOME)) {
				//send_spi_request(ONE_TOUCH_HEX_VALUE);
			}
#endif
		}
		else if(btn_event==2)
		{
#if SOC_SPI_ENABLE
			if (Novatek_boot_completed && !get_music_status() && (ss_get_state() == USAGE_STATE_MEDIA_PLAYER)) {
				//send_spi_request(DOUBLE_TOUCH_HEX_VALUE);
			}
#endif
		}

	}
	else if(aw933xx.event.press)
	{
		PRINTF("[Touch] press \n");
#if SOC_SPI_ENABLE
		if (Novatek_boot_completed && !get_music_status() && (ss_get_state() == USAGE_STATE_MEDIA_PLAYER ||
				ss_get_state() == USAGE_STATE_HOME || ss_get_state() == USAGE_STATE_MENU ||
				ss_get_state() == USAGE_STATE_ABOUT)) {
			// Media Player: Go Home (if the OE is on), Wake Up (if the OE is off)
			// Home: Wake Up (if the OE is off)
			//send_spi_request(PRESS_TOUCH_HEX_VALUE);
		}
#endif
	}
	else if(aw933xx.event.long_press)
	{
		PRINTF("[Touch] long_press \n");
	}
	else if(aw933xx.event.super_long_press)
	{
		PRINTF("[Touch] super_long_press \n");
	}
	else if(aw933xx.event.right_wareds)
	{
		PRINTF("[Touch] slide_right \n");
#if SOC_SPI_ENABLE
		if (Novatek_boot_completed && !get_music_status() && (ss_get_state() == USAGE_STATE_MENU)) {
			//send_spi_request(FORWARD_SLIDE_HEX_VALUE);
		}
#endif
		if (ss_get_state() == USAGE_STATE_MEDIA_PLAYER) {
			// Volume down
			ChangeMasterVolumeLevel15_UpDown(0); // pass zero or negative value to decrease volume
			PRINTF("[Touch] Volume down\r\n");

		}
	}
	else if(aw933xx.event.left_wareds)
	{
		PRINTF("[Touch] slide_left \n");
#if SOC_SPI_ENABLE
		if (Novatek_boot_completed && !get_music_status() && (ss_get_state() == USAGE_STATE_MENU)) {
			//send_spi_request(BACK_SLIDE_HEX_VALUE);
		}
#endif
		if (ss_get_state() == USAGE_STATE_MEDIA_PLAYER) {
			// Volume up
			ChangeMasterVolumeLevel15_UpDown(1); // pass positive value to increase volume
			PRINTF("[Touch] Volume up\r\n");
		}
	}

#endif
}

void hal_touch_ewd608_init (void)
{
	uint16_t fw_ver = 0;
	int rc = elan_touch_get_fw_version(&fw_ver);
	if (rc == kStatus_Success) {
	    PRINTF("[Touch]FW version raw: 0x%04X (%u)\n", fw_ver, fw_ver);
	} else {
		PRINTF("[Touch]Read FW version failed, rc=%d\n", rc);
	}
}

void hal_touch_ewd608_handler (void)
{
#if TOUCH_EWD608_ENABLE
	if (hal_touch_ewd608_get_gesture_state == 0) {
		return;

	} else if (hal_touch_ewd608_get_gesture_state == 2) {
		int rc = hal_i2c_mem_read_impl(EKTF_I2C_ADDR_7BIT, data_reg, buf, EWD_FRAME_MAX_LEN);

		if (rc == kStatus_Success) {
			elan_parse_and_report_data(buf, EWD_FRAME_MAX_LEN);

		} else {
			PRINTF("[Touch] I2C read failed: %d \r\n", rc);
		}
		hal_touch_ewd608_get_gesture_state = 0;
		has_touch_event = 0;

	} else {
		hal_touch_ewd608_get_gesture_state++;
	}
#endif
}

void hal_touch_ewd608_set_state (void)
{
#if TOUCH_EWD608_ENABLE
	if (!hal_touch_ewd608_get_gesture_state) {
		hal_touch_ewd608_get_gesture_state = 1;
	}
#endif
}

void touch_post_event(void *param)
{
    has_touch_event = 1;
}

uint8_t hal_touch_has_new_event(void)
{
	return has_touch_event;
}

#endif
