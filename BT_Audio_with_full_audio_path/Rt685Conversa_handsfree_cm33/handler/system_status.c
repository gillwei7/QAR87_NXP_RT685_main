/*
 * system_status.c
 *
 *  Created on: 2025年10月23日
 *      Author: 11301026
 */


#include "system_status.h"
#include "spi_handler.h"
#include "WorkStateManager.h"

volatile uint8_t System_Status = 0; //Send system status to Novatek

volatile usage_state_t current_usage_state = USAGE_STATE_HOME;

extern QueueHandle_t      spi_request_queue;

void ss_get_state(void)
{
	return current_usage_state;
}

void ss_set_state(uint8_t state)
{
	PRINTF("[System] Usage State change: %d to %d \r\n",current_usage_state,state);
	if (current_usage_state == state) {
		return;
	}


	if (state == USAGE_STATE_HOME) {
		if (current_usage_state == USAGE_STATE_MUSIC_PLAYER) {
			RequestToGetOutofA2dpPlay = 1;
			current_usage_state = state;
			send_state_to_spi();

		} else if (current_usage_state == USAGE_STATE_MEDIA_PLAYER) {
			RequestToGetOutofMediaPlayer = 1;
			current_usage_state = state;
			send_state_to_spi();

		} else if (current_usage_state == USAGE_STATE_VIDEO_RECORDING) {
			RequestToGetOutofVideoRecording = 1;
			current_usage_state = state;
			send_state_to_spi();

		} else if (current_usage_state == USAGE_STATE_VIDEO_AI) {
			RequestToGetOutofVideoAI = 1;
			current_usage_state = state;
			send_state_to_spi();

		} else if (current_usage_state == USAGE_STATE_TRANSLATION) {
			RequestToGetOutofTranslation = 1;
			current_usage_state = state;
			send_state_to_spi();

		} else if (current_usage_state == USAGE_STATE_TAKE_PHOTO) {
			RequestToGetOutofTakePhoto = 1;
			current_usage_state = state;
			send_state_to_spi();

		} else if (current_usage_state == USAGE_STATE_ABOUT) {
			RequestToGetOutofAbout = 1;
			current_usage_state = state;
			send_state_to_spi();

		} else if (current_usage_state == USAGE_STATE_MENU) {
			RequestToGetOutofMenu = 1;
			current_usage_state = state;
			send_state_to_spi();

		}

	} else if (state == USAGE_STATE_MENU && current_usage_state == USAGE_STATE_HOME) {
		RequestToGetIntoMenu = 1;
		current_usage_state = state;
		send_state_to_spi();

	} else if (state == USAGE_STATE_ABOUT && (current_usage_state == USAGE_STATE_HOME || current_usage_state == USAGE_STATE_MENU)) {
		if (current_usage_state == USAGE_STATE_MENU) {
			RequestToGetOutofMenu = 1;
		}
		RequestToGetIntoAbout = 1;
		current_usage_state = state;
		send_state_to_spi();

	} else if (state == USAGE_STATE_MUSIC_PLAYER && (current_usage_state == USAGE_STATE_HOME || current_usage_state == USAGE_STATE_MENU)) {
		if (current_usage_state == USAGE_STATE_MENU) {
			RequestToGetOutofMenu = 1;
		}
		RequestToGetIntoA2dpPlay = 1;
		current_usage_state = state;
		send_state_to_spi();

	} else if (state == USAGE_STATE_MEDIA_PLAYER && (current_usage_state == USAGE_STATE_HOME || current_usage_state == USAGE_STATE_MENU)) {
		if (current_usage_state == USAGE_STATE_MENU) {
			RequestToGetOutofMenu = 1;
		}
		RequestToGetIntoMediaPlayer = 1;
		current_usage_state = state;
		send_state_to_spi();

	} else if (state == USAGE_STATE_VIDEO_RECORDING && (current_usage_state == USAGE_STATE_HOME || current_usage_state == USAGE_STATE_MENU)) {
		if (current_usage_state == USAGE_STATE_MENU) {
			RequestToGetOutofMenu = 1;
		}
		RequestToGetIntoVideoRecording = 1;
		current_usage_state = state;
		send_state_to_spi();

	} else if (state == USAGE_STATE_TAKE_PHOTO && (current_usage_state == USAGE_STATE_HOME || current_usage_state == USAGE_STATE_MENU)) {
		if (current_usage_state == USAGE_STATE_MENU) {
			RequestToGetOutofMenu = 1;
		}
		RequestToGetIntoTakePhoto = 1;
		current_usage_state = state;
		send_state_to_spi();

	} else if (state == USAGE_STATE_VIDEO_AI && (current_usage_state == USAGE_STATE_HOME || current_usage_state == USAGE_STATE_MENU)) {
		if (current_usage_state == USAGE_STATE_MENU) {
			RequestToGetOutofMenu = 1;
		}
		RequestToGetIntoVideoAI = 1;
		current_usage_state = state;
		send_state_to_spi();

	} else if (state == USAGE_STATE_TRANSLATION && (current_usage_state == USAGE_STATE_HOME || current_usage_state == USAGE_STATE_MENU)) {
		if (current_usage_state == USAGE_STATE_MENU) {
			RequestToGetOutofMenu = 1;
		}
		RequestToGetIntoTranslation = 1;
		current_usage_state = state;
		send_state_to_spi();

	}

}

void send_state_to_spi(void) // send state to soc if both audio path and state are ready
{
	PRINTF("[System] Usage State change: %d to %d \r\n",current_usage_state,state);
	uint8_t v = USAGE_STATE_HEX_VALUE + current_usage_state;
	(void)xQueueSend(spi_request_queue, &v, 0);
}

/* ====== BLE/HA/BT/MIC：開關與讀取 ====== */
  void ss_ble_on(SystemStatus* s)  { s->flags |=  SS_BLE_BIT; System_Status=1; }
  void ss_ble_off(SystemStatus* s) { s->flags &= ~SS_BLE_BIT; System_Status=1;}
  bool ss_ble_is_on(const SystemStatus* s) { return (s->flags & SS_BLE_BIT) != 0; }

  void ss_ha_on(SystemStatus* s)   { s->flags |=  SS_HA_BIT; System_Status=1;}
  void ss_ha_off(SystemStatus* s)  { s->flags &= ~SS_HA_BIT; System_Status=1;}
  bool ss_ha_is_on(const SystemStatus* s)  { return (s->flags & SS_HA_BIT) != 0; }

  void ss_bt_on(SystemStatus* s)   { s->flags |=  SS_BT_BIT; System_Status=1;}
  void ss_bt_off(SystemStatus* s)  { s->flags &= ~SS_BT_BIT; System_Status=1;}
  bool ss_bt_is_on(const SystemStatus* s)  { return (s->flags & SS_BT_BIT) != 0; }

  void ss_mic_on(SystemStatus* s)  { s->flags |=  SS_MIC_BIT; System_Status=1;}
  void ss_mic_off(SystemStatus* s) { s->flags &= ~SS_MIC_BIT; System_Status=1;}
  bool ss_mic_is_on(const SystemStatus* s) { return (s->flags & SS_MIC_BIT) != 0; }


/* ====== Layer：設定與讀取 ====== */
  void     ss_set_layer(SystemStatus* s, uint8_t layer) { s->layer = layer; System_Status=1;}
  uint8_t  ss_get_layer(const SystemStatus* s)          { return s->layer; }

/* ====== 充電與電量：設定與讀取 ====== */
  void ss_set_charging(SystemStatus* s, bool on) {
    if (on) s->batt |= SS_CHARGER_BIT; else s->batt &= ~SS_CHARGER_BIT;
    System_Status=1;
}
  bool ss_is_charging(const SystemStatus* s) {
    return (s->batt & SS_CHARGER_BIT) != 0;
}

  void ss_set_battery(SystemStatus* s, uint8_t percent) {
    if (percent > 100) percent = 100; // clamp 到 0..100
    s->batt = (uint8_t)((s->batt & SS_CHARGER_BIT) | (percent << SS_LEVEL_SHIFT));
    System_Status=1;
}
  uint8_t ss_get_battery(const SystemStatus* s) {
    return (uint8_t)((s->batt & SS_LEVEL_MASK) >> SS_LEVEL_SHIFT);
}
