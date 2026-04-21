/*
 * system_status.c
 *
 *  Created on: 2025年10月23日
 *      Author: 11301026
 */


#include "system_status.h"
#include "spi_handler.h"
#include "WorkStateManager.h"
#include "app_handsfree.h"
#include "board.h"
#include "hal_amp.h"
#include "ringtone_handler.h"
#include "scenario_state.h"
#include "porting.h"



volatile uint8_t System_Status = 0; //Send system status to Novatek
volatile SystemStatus ss = {0};

static uint8_t device_name[248] = "QAR88a_0000";
static uint8_t wifi_ssid[248] = "QAR88a_5G_0000";
static uint8_t set_device_name = 0;

static volatile uint8_t current_state_value = 0;
static volatile uint8_t capture_status = 0;
static volatile uint8_t recording_status = 0;
static volatile amp_status_t amp_status = AMP_STATUS_OFF;
static volatile uint8_t music_status = STATUS_OFF;
static volatile uint8_t audio_call_status = STATUS_OFF;

static volatile uint8_t is_turning_on_camera = 0;
static volatile uint8_t need_send_state = 0;
static volatile uint8_t need_send_music_status = 0;

static uint8_t bt_addr_0 = 0;
static uint8_t bt_addr_1 = 0;
static uint8_t bt_addr_2 = 0;
static uint8_t bt_addr_3 = 0;
static uint8_t bt_addr_4 = 0;
static uint8_t bt_addr_5 = 0;

extern QueueHandle_t      spi_request_queue;



// Only for A2DP
uint8_t get_music_status(void)
{
	return music_status;
}

void set_music_status(uint8_t status)
{
	music_status = status;
}

// Only for HFP
uint8_t get_audio_call_status(void)
{
	return audio_call_status;
}

void set_audio_call_status(uint8_t status)
{
	audio_call_status = status;
}

uint8_t get_amp_status(void)
{
	return amp_status;
}

void set_amp_status(uint8_t status)
{
	amp_status = status;
}

#if 0
void send_state_to_soc(void) // send state to soc if both audio path and state are ready
{
	PRINTF("[System] send_state_to_soc (%d) \r\n", get_scenario_state());

	if (need_send_state) {
		PRINTF("[System] need_send_state send_state_to_soc (%d) \r\n", get_scenario_state());
		current_state_value = USAGE_STATE_HEX_VALUE + get_scenario_state();
		//send_spi_request(current_state_value);
		need_send_state = 0;

	}
//	else if (need_send_music_status) {
//		PRINTF("[System] send_music_status_to_soc (%d) \r\n", music_status);
//		send_music_status_to_soc();
//		need_send_music_status = 0;
//	}
}


void send_music_status_to_soc(void)
{
	// Todo
	// To make novatek open music ui or open home ui
#if SOC_SPI_ENABLE
	if (music_status) {
		//send_spi_request(MUSIC_START_HEX_VALUE); //music start
	} else {
		//send_spi_request(MUSIC_STOP_HEX_VALUE); //music stop
	}
#endif
	//send_spi_request(MUSIC_START_HEX_VALUE); //music start
	//send_spi_request(MUSIC_STOP_HEX_VALUE); //music stop
}
#endif

void ss_set_camera_status(uint8_t status)
{
	if (status == STATUS_ON)
	{
		is_turning_on_camera = 1;
	}
}

void ss_set_capture_status(uint8_t status)
{
	if (status == STATUS_START)
	{
		capture_status = STATUS_ON;
        set_ringtone_state(Ringtone_PhotoCapture);
	}
	else if (status == STATUS_END)
	{
		capture_status = STATUS_OFF;

	}
}

uint8_t ss_get_capture_status(void)
{
	return capture_status;
}

void ss_set_recording_status(uint8_t status)
{
	if (status == STATUS_START)
	{
		recording_status = STATUS_ON;
	}
	else if (status == STATUS_END)
	{
		recording_status = STATUS_OFF;
	}
}

uint8_t ss_get_recording_status(void)
{
	return recording_status;
}

static void ss_set_device_name (void)
{
	snprintf(device_name, sizeof(device_name), "QAR88a_%02X%02X", bt_addr_1, bt_addr_0);
	snprintf(wifi_ssid, sizeof(wifi_ssid), "QAR88a_5G_%02X%02X", bt_addr_1, bt_addr_0);
	set_device_name = 1;
}

void ss_set_bt_addr (uint8_t addr_0, uint8_t addr_1, uint8_t addr_2, uint8_t addr_3, uint8_t addr_4, uint8_t addr_5)
{
	bt_addr_0 = addr_0;
	bt_addr_1 = addr_1;
	bt_addr_2 = addr_2;
	bt_addr_3 = addr_3;
	bt_addr_4 = addr_4;
	bt_addr_5 = addr_5;
	ss_set_device_name();
}

uint8_t has_set_device_name (void) {
	return set_device_name;
}

uint8_t * ss_get_device_name (void)
{
	PRINTF("Device name: %s\r\n", device_name);
	return device_name;
}

uint8_t * ss_get_wifi_ssid (void)
{
	PRINTF("Wi-Fi SSID: %s\r\n", wifi_ssid);
	return wifi_ssid;
}

void ss_print_bt_addr (void)
{
	PRINTF("BT Address: %02X:%02X:%02X:%02X:%02X:%02X\r\n", bt_addr_5, bt_addr_4, bt_addr_3, bt_addr_2, bt_addr_1, bt_addr_0);

}

/* ====== BLE/HA/BT/MIC：開關與讀取 ====== */
void ss_ble_on() {
	ss.flags |=  SS_BLE_BIT;
	System_Status=1;
#if SOC_SPI_ENABLE
	//send_spi_request(SYSTEM_STATUS_HEX_VALUE);
#endif
}
void ss_ble_off() {
	ss.flags &= ~SS_BLE_BIT;
	System_Status=1;
#if SOC_SPI_ENABLE
	//send_spi_request(SYSTEM_STATUS_HEX_VALUE);
#endif
}

bool ss_ble_is_on() {
	return (ss.flags & SS_BLE_BIT) != 0;
}

void ss_ha_on() {
	ss.flags |=  SS_HA_BIT;
	System_Status=1;
#if SOC_SPI_ENABLE
	//send_spi_request(SYSTEM_STATUS_HEX_VALUE);
#endif
}

void ss_ha_off() {
	ss.flags &= ~SS_HA_BIT;
	System_Status=1;
#if SOC_SPI_ENABLE
	//send_spi_request(SYSTEM_STATUS_HEX_VALUE);
#endif
}

bool ss_ha_is_on() {
	return (ss.flags & SS_HA_BIT) != 0;
}

void ss_bt_on() {
	ss.flags |=  SS_BT_BIT;
	System_Status=1;
#if SOC_SPI_ENABLE
	//send_spi_request(SYSTEM_STATUS_HEX_VALUE);
#endif
}

void ss_bt_off() {
	ss.flags &= ~SS_BT_BIT;
	System_Status=1;
#if SOC_SPI_ENABLE
	//send_spi_request(SYSTEM_STATUS_HEX_VALUE);
#endif
}

bool ss_bt_is_on() {
	return (ss.flags & SS_BT_BIT) != 0;
}

void ss_mic_on() {
	ss.flags |=  SS_MIC_BIT;
	System_Status=1;
#if SOC_SPI_ENABLE
	//send_spi_request(SYSTEM_STATUS_HEX_VALUE);
#endif
}

void ss_mic_off() {
	ss.flags &= ~SS_MIC_BIT;
	System_Status=1;
#if SOC_SPI_ENABLE
	//send_spi_request(SYSTEM_STATUS_HEX_VALUE);
#endif
}

bool ss_mic_is_on() {
	return (ss.flags & SS_MIC_BIT) != 0;
}


/* ====== Layer：設定與讀取 ====== */
void ss_set_layer(uint8_t layer) {
	ss.layer = layer; System_Status=1;
}

uint8_t ss_get_layer() {
	return ss.layer;
}

/* ====== 充電與電量：設定與讀取 ====== */
  void ss_set_charging(bool on) {
    if (on) {
    	ss.batt |= SS_CHARGER_BIT;
    }
    else {
    	ss.batt &= ~SS_CHARGER_BIT;
    }
    System_Status=1;
}
  bool ss_is_charging() {
    return (ss.batt & SS_CHARGER_BIT) != 0;
}

  void ss_set_battery(uint8_t percent) {
    if (percent > 100) {
    	percent = 100; // clamp 到 0..100
    }
    ss.batt = (uint8_t)((ss.batt & SS_CHARGER_BIT) | (percent << SS_LEVEL_SHIFT));
    System_Status=1;
}
  uint8_t ss_get_battery() {
    return (uint8_t)((ss.batt & SS_LEVEL_MASK) >> SS_LEVEL_SHIFT);
}


