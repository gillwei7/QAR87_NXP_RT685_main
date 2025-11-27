/*
 * system_statuss.c
 *
 *  Created on: 2025年10月23日
 *      Author: 11301026
 */


#include "system_status.h"
#include "spi_handler.h"

volatile uint8_t System_Status = 0; //Send system status to Novatek
volatile SystemStatus ss = {0};

volatile usage_status_t current_usage_status = USAGE_STATUS_HOME;

extern QueueHandle_t      spi_request_queue;

void usage_status_change(uint8_t status)
{
	PRINTF("[System] Usage Status change: %d to %d \r\n",current_usage_status,status);
	current_usage_status = status;
	uint8_t v = USAGE_STATUS_HEX_VALUE + current_usage_status;
    (void)xQueueSend(spi_request_queue, &v, 0);

}

/* ====== BLE/HA/BT/MIC：開關與讀取 ====== */
  void ss_ble_on()  { ss.flags |=  SS_BLE_BIT; System_Status=1; }
  void ss_ble_off() { ss.flags &= ~SS_BLE_BIT; System_Status=1;}
  bool ss_ble_is_on() { return (ss.flags & SS_BLE_BIT) != 0; }

  void ss_ha_on()   { ss.flags |=  SS_HA_BIT; System_Status=1;}
  void ss_ha_off()  { ss.flags &= ~SS_HA_BIT; System_Status=1;}
  bool ss_ha_is_on()  { return (ss.flags & SS_HA_BIT) != 0; }

  void ss_bt_on()   { ss.flags |=  SS_BT_BIT; System_Status=1;}
  void ss_bt_off()  { ss.flags &= ~SS_BT_BIT; System_Status=1;}
  bool ss_bt_is_on()  { return (ss.flags & SS_BT_BIT) != 0; }

  void ss_mic_on()  { ss.flags |=  SS_MIC_BIT; System_Status=1;}
  void ss_mic_off() { ss.flags &= ~SS_MIC_BIT; System_Status=1;}
  bool ss_mic_is_on() { return (ss.flags & SS_MIC_BIT) != 0; }


/* ====== Layer：設定與讀取 ====== */
  void     ss_set_layer(uint8_t layer) { ss.layer = layer; System_Status=1;}
  uint8_t  ss_get_layer()          { return ss.layer; }

/* ====== 充電與電量：設定與讀取 ====== */
  void ss_set_charging(bool on) {
    if (on) ss.batt |= SS_CHARGER_BIT; else ss.batt &= ~SS_CHARGER_BIT;
    System_Status=1;
}
  bool ss_is_charging() {
    return (ss.batt & SS_CHARGER_BIT) != 0;
}

  void ss_set_battery(uint8_t percent) {
    if (percent > 100) percent = 100; // clamp 到 0..100
    ss.batt = (uint8_t)((ss.batt & SS_CHARGER_BIT) | (percent << SS_LEVEL_SHIFT));
    System_Status=1;
}
  uint8_t ss_get_battery() {
    return (uint8_t)((ss.batt & SS_LEVEL_MASK) >> SS_LEVEL_SHIFT);
}


