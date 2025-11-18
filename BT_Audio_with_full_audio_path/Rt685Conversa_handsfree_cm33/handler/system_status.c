/*
 * system_status.c
 *
 *  Created on: 2025年10月23日
 *      Author: 11301026
 */


#include "system_status.h"
#include "spi_handler.h"

volatile uint8_t System_Status = 0; //Send system status to Novatek

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
