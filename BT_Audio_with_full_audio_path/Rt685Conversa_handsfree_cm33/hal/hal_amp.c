/*
 * hal_amp.c
 *
 *  Created on: Oct 21, 2025
 *      Author: Lydia
 */
#if UsingQAR87Board == 1
#include "hal_amp.h"

void hal_amp_aw88166_power_on(void) {
    GPIO_PinWrite(GPIO, GPIO_AMP_RESET_R_PORT, GPIO_AMP_RESET_R_PIN, 1);
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

#endif
