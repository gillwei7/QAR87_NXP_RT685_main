/*
 * hal_amp.c
 *
 *  Created on: Oct 21, 2025
 *      Author: Lydia
 */
#if UsingQAR87Board == 1
#include "hal_amp.h"
#include "WorkStateManager.h"
#include "system_status.h"

volatile amp_event_t g_amp_event = AMP_EVT_STOP;
static uint8_t has_amp_event = 0;

/* prof_name: 1. "Music", 2. "Receiver" */
static void hal_amp_aw88166_left_start(char *prof_name) {
    start_aw88166_pa(AW_DEV_0, prof_name);
}

static void hal_amp_aw88166_right_start(char *prof_name) {
    start_aw88166_pa(AW_DEV_1, prof_name);
}

static void hal_amp_aw88166_left_stop(void) {
    close_aw88166_pa(AW_DEV_0);

}
static void hal_amp_aw88166_right_stop(void) {
    close_aw88166_pa(AW_DEV_1);

}

// The AMP must be enabled after I2S, so it can be enabled within the handler
static void hal_amp_aw88166_all_start_music (void) {
	hal_amp_aw88166_left_start("Music");
	hal_amp_aw88166_right_start("Music");
	set_amp_status(AMP_STATUS_MUSIC);
	AmpState=AmpState_ConfiguredAndActive;
	PRINTF("AMP_EVT_MUSIC_START done\r\n");
}

static void hal_amp_aw88166_all_start_receiver (void) {
	hal_amp_aw88166_left_start("Receiver");
	hal_amp_aw88166_right_start("Receiver");
	set_amp_status(AMP_STATUS_RECEIVER);
	AmpState=AmpState_ConfiguredAndActive;
	PRINTF("AMP_EVT_RECEIVER_START done\r\n");
}

//The AMP must be disabled before I2S
static void hal_amp_aw88166_all_stop (void) {
	hal_amp_aw88166_left_stop();
	hal_amp_aw88166_right_stop();
	set_amp_status(AMP_STATUS_OFF);
	AmpState=AmpState_UnConfigured;
	PRINTF("AMP_EVT_STOP done\r\n");
}

void hal_amp_aw88166_power_on(void) {
    GPIO_PinWrite(GPIO, GPIO_AMP_RESET_R_PORT, GPIO_AMP_RESET_R_PIN, 1);
}

void hal_amp_aw88166_power_off(void) {
    GPIO_PinWrite(GPIO, GPIO_AMP_RESET_R_PORT, GPIO_AMP_RESET_R_PIN, 0);
}

void hal_amp_aw88166_init(void) {
    init_aw88166();
    PRINTF("[AMP][AW88166] init OK\r\n");
    hal_amp_aw88166_all_stop();
}

void hal_amp_aw88166_handler (void) {
#if AMP_AW88166_ENABLE
		switch (g_amp_event) {
			case AMP_EVT_MUSIC:
				hal_amp_aw88166_all_start_music();
				break;
			case AMP_EVT_RECEIVER:
				hal_amp_aw88166_all_start_receiver();
				break;
			case AMP_EVT_STOP:
				hal_amp_aw88166_all_stop();
				break;
			default:
				break;
		}
		has_amp_event = 0;
#endif

}


void amp_post_event(amp_event_t e)
{
    g_amp_event = e;
#if 0
    if (i2c_event_group) {
        xEventGroupSetBits(i2c_event_group, AMP_EVENT_BIT);
    }
#endif
    has_amp_event = 1;
}

uint8_t hal_amp_has_new_event(void)
{
	return has_amp_event;
}


#endif
