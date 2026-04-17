/*
 * touch_handler.c
 *
 *  Created on: Apr 16, 2026
 *      Author: Lydia
 */


#include "touch_handler.h"
#include "spi_command_set.h"
#include "spi_handler.h"
#include "system_status.h"
#include "WorkStateManager.h"
#include "scenario_state.h"
#include "ui_handler.h"

static volatile touch_gesture_t current_touch_gesture = TOUCH_GESTURE_NOTHING;
static volatile uint8_t has_new_gesture = 0;


static void touch_gesture_media_player_handler (void) {
	if (get_scenario_state() == SCENARIO_STATE_MEDIA_PLAYER) {
		if (current_touch_gesture == TOUCH_GESTURE_SINGLE_FORWARD) {
			ChangeMasterVolumeLevel15_UpDown(1); // pass positive value to increase volume
			PRINTF("[Touch] Volume up\r\n");
		} else if (current_touch_gesture == TOUCH_GESTURE_SINGLE_BACKWARD) {
			ChangeMasterVolumeLevel15_UpDown(0); // pass zero or negative value to decrease volume
			PRINTF("[Touch] Volume down\r\n");
		} else if (current_touch_gesture == TOUCH_GESTURE_SINGLE_TAP) {
			//TODO play and pause
		} else if (current_touch_gesture == TOUCH_GESTURE_SINGLE_DOUBLE_TAP) {
			//TODO next
		} else if (current_touch_gesture == TOUCH_GESTURE_SINGLE_PRESS_HOLD) {
			//TODO leave go to menu scenario
		}
	}
}

static void touch_gesture_music_handler (void) {
	if (get_scenario_state() == SCENARIO_STATE_MUSIC) {
		if (get_ui_view() == UI_VIEW_MUSIC_PLAYER) {
			if (current_touch_gesture == TOUCH_GESTURE_SINGLE_FORWARD) {
				ChangeMasterVolumeLevel15_UpDown(1); // pass positive value to increase volume
				PRINTF("[Touch] Volume up\r\n");
			} else if (current_touch_gesture == TOUCH_GESTURE_SINGLE_BACKWARD) {
				ChangeMasterVolumeLevel15_UpDown(0); // pass zero or negative value to decrease volume
				PRINTF("[Touch] Volume down\r\n");
			} else if (current_touch_gesture == TOUCH_GESTURE_SINGLE_TAP) {
				//TODO play and pause
			} else if (current_touch_gesture == TOUCH_GESTURE_SINGLE_DOUBLE_TAP) {
				//TODO next song
			} else if (current_touch_gesture == TOUCH_GESTURE_TWO_PRESS_HOLD) {
				//TODO leave and go to menu ui
			}

		} else if (get_ui_view() == UI_VIEW_MENU_MEDIA) {
			if (current_touch_gesture == TOUCH_GESTURE_SINGLE_FORWARD) {
				ChangeMasterVolumeLevel15_UpDown(1); // pass positive value to increase volume
				PRINTF("[Touch] Volume up\r\n");
			} else if (current_touch_gesture == TOUCH_GESTURE_SINGLE_BACKWARD) {
				ChangeMasterVolumeLevel15_UpDown(0); // pass zero or negative value to decrease volume
				PRINTF("[Touch] Volume down\r\n");
			} else if (current_touch_gesture == TOUCH_GESTURE_SINGLE_TAP) {
				//TODO play and pause
			} else if (current_touch_gesture == TOUCH_GESTURE_SINGLE_DOUBLE_TAP) {
				//TODO next song
			} else if (current_touch_gesture == TOUCH_GESTURE_TWO_PRESS_HOLD) {
				//TODO leave and go to music ui
			} else if (current_touch_gesture == TOUCH_GESTURE_TWO_FORWARD) {
				//TODO next app
			} else if (current_touch_gesture == TOUCH_GESTURE_TWO_BACKWARD) {
				//TODO previous app
			} else if (current_touch_gesture == TOUCH_GESTURE_TWO_TAP) {
				//TODO enter
			}
		} else if (get_ui_view() == UI_VIEW_MENU_ABOUT) {
			if (current_touch_gesture == TOUCH_GESTURE_SINGLE_FORWARD) {
				ChangeMasterVolumeLevel15_UpDown(1); // pass positive value to increase volume
				PRINTF("[Touch] Volume up\r\n");
			} else if (current_touch_gesture == TOUCH_GESTURE_SINGLE_BACKWARD) {
				ChangeMasterVolumeLevel15_UpDown(0); // pass zero or negative value to decrease volume
				PRINTF("[Touch] Volume down\r\n");
			} else if (current_touch_gesture == TOUCH_GESTURE_SINGLE_TAP) {
				//TODO play and pause
			} else if (current_touch_gesture == TOUCH_GESTURE_SINGLE_DOUBLE_TAP) {
				//TODO next song
			} else if (current_touch_gesture == TOUCH_GESTURE_TWO_PRESS_HOLD) {
				//TODO leave and go to music ui
			} else if (current_touch_gesture == TOUCH_GESTURE_TWO_FORWARD) {
				//TODO next app
			} else if (current_touch_gesture == TOUCH_GESTURE_TWO_BACKWARD) {
				//TODO previous app
			} else if (current_touch_gesture == TOUCH_GESTURE_TWO_TAP) {
				//TODO enter
			}

		} else if (get_ui_view() == UI_VIEW_ABOUT) {
			if (current_touch_gesture == TOUCH_GESTURE_SINGLE_FORWARD) {
				ChangeMasterVolumeLevel15_UpDown(1); // pass positive value to increase volume
				PRINTF("[Touch] Volume up\r\n");
			} else if (current_touch_gesture == TOUCH_GESTURE_SINGLE_BACKWARD) {
				ChangeMasterVolumeLevel15_UpDown(0); // pass zero or negative value to decrease volume
				PRINTF("[Touch] Volume down\r\n");
			} else if (current_touch_gesture == TOUCH_GESTURE_SINGLE_TAP) {
				//TODO play and pause
			} else if (current_touch_gesture == TOUCH_GESTURE_SINGLE_DOUBLE_TAP) {
				//TODO next song
			} else if (current_touch_gesture == TOUCH_GESTURE_TWO_PRESS_HOLD) {
				//TODO leave and go to music ui
			}
		}
	}
}

static void touch_gesture_audio_call_handler (void) {
	if (get_scenario_state() == SCENARIO_STATE_AUDIO_CALL) {
		if (get_ui_view() == UI_VIEW_AUDIO_CALL) {
			if (current_touch_gesture == TOUCH_GESTURE_SINGLE_FORWARD) {
				ChangeMasterVolumeLevel15_UpDown(1); // pass positive value to increase volume
				PRINTF("[Touch] Volume up\r\n");
			} else if (current_touch_gesture == TOUCH_GESTURE_SINGLE_BACKWARD) {
				ChangeMasterVolumeLevel15_UpDown(0); // pass zero or negative value to decrease volume
				PRINTF("[Touch] Volume down\r\n");
			} else if (current_touch_gesture == TOUCH_GESTURE_SINGLE_TAP) {
				//TODO answer
			} else if (current_touch_gesture == TOUCH_GESTURE_SINGLE_DOUBLE_TAP) {
				//TODO reject
			} else if (current_touch_gesture == TOUCH_GESTURE_TWO_PRESS_HOLD) {
				//TODO leave and go to menu ui
			}

		} else if (get_ui_view() == UI_VIEW_MENU_MEDIA) {
			if (current_touch_gesture == TOUCH_GESTURE_SINGLE_FORWARD) {
				ChangeMasterVolumeLevel15_UpDown(1); // pass positive value to increase volume
				PRINTF("[Touch] Volume up\r\n");
			} else if (current_touch_gesture == TOUCH_GESTURE_SINGLE_BACKWARD) {
				ChangeMasterVolumeLevel15_UpDown(0); // pass zero or negative value to decrease volume
				PRINTF("[Touch] Volume down\r\n");
			} else if (current_touch_gesture == TOUCH_GESTURE_SINGLE_TAP) {
				//TODO answer
			} else if (current_touch_gesture == TOUCH_GESTURE_SINGLE_DOUBLE_TAP) {
				//TODO reject
			} else if (current_touch_gesture == TOUCH_GESTURE_TWO_PRESS_HOLD) {
				//TODO leave and go to audio call ui
			} else if (current_touch_gesture == TOUCH_GESTURE_TWO_FORWARD) {
				//TODO next app
			} else if (current_touch_gesture == TOUCH_GESTURE_TWO_BACKWARD) {
				//TODO previous app
			} else if (current_touch_gesture == TOUCH_GESTURE_TWO_TAP) {
				//TODO enter
			}

		} else if (get_ui_view() == UI_VIEW_MENU_ABOUT) {
			if (current_touch_gesture == TOUCH_GESTURE_SINGLE_FORWARD) {
				ChangeMasterVolumeLevel15_UpDown(1); // pass positive value to increase volume
				PRINTF("[Touch] Volume up\r\n");
			} else if (current_touch_gesture == TOUCH_GESTURE_SINGLE_BACKWARD) {
				ChangeMasterVolumeLevel15_UpDown(0); // pass zero or negative value to decrease volume
				PRINTF("[Touch] Volume down\r\n");
			} else if (current_touch_gesture == TOUCH_GESTURE_SINGLE_TAP) {
				//TODO answer
			} else if (current_touch_gesture == TOUCH_GESTURE_SINGLE_DOUBLE_TAP) {
				//TODO reject
			} else if (current_touch_gesture == TOUCH_GESTURE_TWO_PRESS_HOLD) {
				//TODO leave and go to audio call ui
			} else if (current_touch_gesture == TOUCH_GESTURE_TWO_FORWARD) {
				//TODO next app
			} else if (current_touch_gesture == TOUCH_GESTURE_TWO_BACKWARD) {
				//TODO previous app
			} else if (current_touch_gesture == TOUCH_GESTURE_TWO_TAP) {
				//TODO enter
			}

		} else if (get_ui_view() == UI_VIEW_ABOUT) {
			if (current_touch_gesture == TOUCH_GESTURE_SINGLE_FORWARD) {
				ChangeMasterVolumeLevel15_UpDown(1); // pass positive value to increase volume
				PRINTF("[Touch] Volume up\r\n");
			} else if (current_touch_gesture == TOUCH_GESTURE_SINGLE_BACKWARD) {
				ChangeMasterVolumeLevel15_UpDown(0); // pass zero or negative value to decrease volume
				PRINTF("[Touch] Volume down\r\n");
			} else if (current_touch_gesture == TOUCH_GESTURE_SINGLE_TAP) {
				//TODO answer
			} else if (current_touch_gesture == TOUCH_GESTURE_SINGLE_DOUBLE_TAP) {
				//TODO reject
			} else if (current_touch_gesture == TOUCH_GESTURE_TWO_PRESS_HOLD) {
				//TODO leave and go to audio call ui
			}
		}
	}
}

static void touch_gesture_video_recording_handler (void) {
	if (get_scenario_state() == SCENARIO_STATE_VIDEO_RECORDING) {
		if (get_ui_view() == UI_VIEW_VIDEO_RECORDING) {
			if (current_touch_gesture == TOUCH_GESTURE_TWO_PRESS_HOLD) {
				//TODO leave and go to menu ui
			}
		} else if (get_ui_view() == UI_VIEW_MENU_MEDIA) {
			if (current_touch_gesture == TOUCH_GESTURE_TWO_PRESS_HOLD) {
				//TODO leave and go to video recording ui
			} else if (current_touch_gesture == TOUCH_GESTURE_TWO_FORWARD) {
				//TODO next app
			} else if (current_touch_gesture == TOUCH_GESTURE_TWO_BACKWARD) {
				//TODO previous app
			} else if (current_touch_gesture == TOUCH_GESTURE_TWO_TAP) {
				//TODO enter
			}
		} else if (get_ui_view() == UI_VIEW_MENU_ABOUT) {
			if (current_touch_gesture == TOUCH_GESTURE_TWO_PRESS_HOLD) {
				//TODO leave and go to video recording ui
			} else if (current_touch_gesture == TOUCH_GESTURE_TWO_FORWARD) {
				//TODO next app
			} else if (current_touch_gesture == TOUCH_GESTURE_TWO_BACKWARD) {
				//TODO previous app
			} else if (current_touch_gesture == TOUCH_GESTURE_TWO_TAP) {
				//TODO enter
			}

		} else if (get_ui_view() == UI_VIEW_ABOUT) {
			if (current_touch_gesture == TOUCH_GESTURE_TWO_PRESS_HOLD) {
				//TODO leave and go to video recording ui
			}
		}
	}
}

static void touch_gesture_home_handler (void) {
	if (get_scenario_state() == SCENARIO_STATE_HOME) {
		if (current_touch_gesture == TOUCH_GESTURE_TWO_PRESS_HOLD) {
			//TODO leave and go to menu scenario
		}
	}
}

static void touch_gesture_menu_handler (void) {
	if (get_scenario_state() == SCENARIO_STATE_MENU) {
		if (current_touch_gesture == TOUCH_GESTURE_TWO_PRESS_HOLD) {
			//TODO leave and go to home scenario
		} else if (current_touch_gesture == TOUCH_GESTURE_TWO_FORWARD) {
			//TODO next app
		} else if (current_touch_gesture == TOUCH_GESTURE_TWO_BACKWARD) {
			//TODO previous app
		} else if (current_touch_gesture == TOUCH_GESTURE_TWO_TAP) {
			//TODO enter
		}
	}
}

static void touch_gesture_settings_handler (void) {
	if (get_scenario_state() == SCENARIO_STATE_SETTINGS) {
		if (current_touch_gesture == TOUCH_GESTURE_TWO_PRESS_HOLD) {
			//TODO leave and go to menu scenario
		} else if (current_touch_gesture == TOUCH_GESTURE_TWO_FORWARD) {
			//TODO next setting
		} else if (current_touch_gesture == TOUCH_GESTURE_TWO_BACKWARD) {
			//TODO previous setting
		} else if (current_touch_gesture == TOUCH_GESTURE_TWO_TAP) {
			//TODO change status
		}
	}
}

static void touch_gesture_about_handler (void) {
	if (get_scenario_state() == SCENARIO_STATE_ABOUT) {
		if (current_touch_gesture == TOUCH_GESTURE_TWO_PRESS_HOLD) {
			//TODO leave and go to settings scenario
		}
	}
}

// For Video AI, Translation, Video Call
static void touch_gesture_video_ai_handler (void) {
	if (get_scenario_state() == SCENARIO_STATE_VIDEO_AI || get_scenario_state() == SCENARIO_STATE_TRANSLATION
			 || get_scenario_state() == SCENARIO_STATE_VIDEO_CALL) {
		if (current_touch_gesture == TOUCH_GESTURE_SINGLE_FORWARD) {
			ChangeMasterVolumeLevel15_UpDown(1); // pass positive value to increase volume
			PRINTF("[Touch] Volume up\r\n");
		} else if (current_touch_gesture == TOUCH_GESTURE_SINGLE_BACKWARD) {
            ChangeMasterVolumeLevel15_UpDown(0); // pass zero or negative value to decrease volume
            PRINTF("[Touch] Volume down\r\n");

		} else if (current_touch_gesture == TOUCH_GESTURE_TWO_PRESS_HOLD) {
			//TODO leave and go to menu scenario
		}
	}
}

void touch_gesture_handler (void) {
	if (!has_new_gesture) {
		return;
	}
	touch_gesture_media_player_handler();
	touch_gesture_music_handler();
	touch_gesture_audio_call_handler();
	touch_gesture_video_recording_handler();
	touch_gesture_home_handler();
	touch_gesture_menu_handler();
	touch_gesture_settings_handler();
	touch_gesture_about_handler();
	touch_gesture_video_ai_handler();

	has_new_gesture = 0;
	current_touch_gesture = TOUCH_GESTURE_NOTHING;
}

void set_touch_gesture (touch_gesture_t touch_gesture) {
	has_new_gesture = 1;
	current_touch_gesture = touch_gesture;
}
