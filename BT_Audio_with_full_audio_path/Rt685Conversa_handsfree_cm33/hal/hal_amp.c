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


void hal_amp_aw88166_power_on(void) {
    GPIO_PinWrite(GPIO, GPIO_AMP_RESET_R_PORT, GPIO_AMP_RESET_R_PIN, 1);
}

void hal_amp_aw88166_power_off(void) {
    GPIO_PinWrite(GPIO, GPIO_AMP_RESET_R_PORT, GPIO_AMP_RESET_R_PIN, 0);
}

void hal_amp_aw88166_init(void) {
    init_aw88166();
    PRINTF("[AMP][AW88166] init OK\r\n");
}

/* prof_name: 1. "Music", 2. "Receiver" */
void hal_amp_aw88166_left_start(char *prof_name) {
    start_aw88166_pa(AW_DEV_0, prof_name);
}

void hal_amp_aw88166_right_start(char *prof_name) {
    start_aw88166_pa(AW_DEV_1, prof_name);
}

void hal_amp_aw88166_left_stop(void) {
    close_aw88166_pa(AW_DEV_0);

}
void hal_amp_aw88166_right_stop(void) {
    close_aw88166_pa(AW_DEV_1);

}

void hal_amp_aw88166_handler (void) {
#if AMP_AW88166_ENABLE
		switch (g_amp_event) {
			case AMP_EVT_MUSIC:
				hal_amp_aw88166_left_start("Music");
				hal_amp_aw88166_right_start("Music");
				AmpState=AmpState_ConfiguredAndActive;
				set_amp_status(AMP_STATUS_MUSIC);
				PRINTF("AMP_EVT_MUSIC_START done\r\n");
				break;
			case AMP_EVT_RECEIVER:
				hal_amp_aw88166_left_start("Receiver");
				hal_amp_aw88166_right_start("Receiver");
				AmpState=AmpState_ConfiguredAndActive;
				PRINTF("AMP_EVT_RECEIVER_START done\r\n");
				set_amp_status(AMP_STATUS_RECEIVER);
				break;
			case AMP_EVT_STOP:
				hal_amp_aw88166_left_stop();
				hal_amp_aw88166_right_stop();
				AmpState=AmpState_UnConfigured;
				PRINTF("AMP_EVT_STOP done\r\n");
				set_amp_status(AMP_STATUS_OFF);
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
