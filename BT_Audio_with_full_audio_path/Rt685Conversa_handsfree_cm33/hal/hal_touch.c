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
#include "touch_handler.h"


extern volatile struct aw933xx_dev aw933xx;
extern uint8_t Novatek_boot_completed;

static uint8_t has_touch_event = 0;

#if TOUCH_EWD608_ENABLE
static uint8_t hal_touch_ewd608_get_gesture_state = 0;
static const uint8_t data_reg = 0xC0;
static uint8_t buf[EWD_FRAME_MAX_LEN];
#endif

#if TOUCH_AW93305_ENABLE
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
        	set_touch_gesture(TOUCH_GESTURE_SINGLE_TAP);
		}
		else if(btn_event==2)
		{
        	set_touch_gesture(TOUCH_GESTURE_SINGLE_DOUBLE_TAP);
		}

	}
	else if(aw933xx.event.press)
	{
		PRINTF("[Touch] press \n");
    	set_touch_gesture(TOUCH_GESTURE_SINGLE_PRESS_HOLD);
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
		PRINTF("[Touch] slide_right (backward) \n");
    	set_touch_gesture(TOUCH_GESTURE_SINGLE_BACKWARD);
	}
	else if(aw933xx.event.left_wareds)
	{
		PRINTF("[Touch] slide_left (forward) \n");
    	set_touch_gesture(TOUCH_GESTURE_SINGLE_FORWARD);
	}

#endif
}
#endif

#if TOUCH_EWD608_ENABLE
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
#endif

void touch_post_event(void *param)
{
    has_touch_event = 1;
}

uint8_t hal_touch_has_new_event(void)
{
	return has_touch_event;
}

#endif
