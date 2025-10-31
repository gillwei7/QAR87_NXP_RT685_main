/*
 * ktd202x_leds.h
 *
 *  Created on: 2022年12月26日
 *      Author: 10608152
 */
#ifndef KTD202X_LEDS_H_
#define KTD202X_LEDS_H_

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#define KTD2027
#define PLATFORM_MODE 1

#define LED_OFF      0x00
#define BLUE_BLINK 0x01
#define GREEN_BLINK  0x02
#define RED_BLINK    0x03
#define BLUE_ON    0x04
#define GREEN_ON     0x05
#define RED_ON     0x06
#ifdef KTD2027
#define WHITE_ON     0x07
#define WHITE_BLINK     0x08
#endif


#define LED_CURRENT_CH1       0x09	// BLUE  0x1F: 4mA, 0x1B: 3.5mA, 0x17: 3mA, 0x14: 2.625mA, 0x13: 2.5mA, 0x12: 2.375mA, 0x0F: 2mA, 0x0E: 1.875mA, 0x0D: 1.75mA 0x0C: 1.625mA, 0x0B: 1.5mA 0x0A: 1.375mA, 0x07: 1mA     (20mA * 10mcd / 50mcd  = 4mA)
#define LED_CURRENT_CH1_WHITE 0x01	// BLUE  0x1F: 4mA, 0x1B: 3.5mA, 0x17: 3mA, 0x14: 2.625mA, 0x13: 2.5mA, 0x12: 2.375mA, 0x0F: 2mA, 0x0E: 1.875mA, 0x0D: 1.75mA 0x0C: 1.625mA, 0x0B: 1.5mA 0x0A: 1.375mA, 0x07: 1mA     (20mA * 10mcd / 50mcd  = 4mA)
#define LED_CURRENT_CH2       0x07	// RED   0x0F: 2mA      (20mA * 10mcd / 100mcd = 2mA)
#define LED_CURRENT_CH2_WHITE 0x03	// RED   0x0F: 2mA      (20mA * 10mcd / 100mcd = 2mA)
#define LED_CURRENT_CH2_ORG   0x0E	// RED   0x0F: 2mA      (20mA * 10mcd / 100mcd = 2mA)
#define LED_CURRENT_CH3       0x03	// GREEN 0x08: 1.125mA, 0x07: 1mA, 0x05: 0.75mA, 0x04: 0.625mA  (20mA * 10mcd / 180mcd = 1.11mA)
#define LED_CURRENT_CH3_WHITE 0x01	// GREEN 0x08: 1.125mA, 0x07: 1mA, 0x05: 0.75mA, 0x04: 0.625mA  (20mA * 10mcd / 180mcd = 1.11mA)
#define LED_CURRENT_CH3_ORG   0x00	// GREEN 0x08: 1.125mA, 0x04: 0.625mA, 0x01: 0.25mA, 0x00: 0.125mA  (20mA * 10mcd / 180mcd = 1.11mA)
#define LED_CURRENT_CH4       0x05	// WHITE 0x4F: 10mA, 0x37 7mA, 0x17: 3mA, 0x13: 2.5mA, 0x09: 1.25mA


#define TIM_1     0x00
#define TIM_2     0x01

#define PERIOD_CODE_1P5S     0x06
#define RISE_CODE_600MS      0x03
#define FALL_CODE_600MS      0x03
#define RAMP_SCALE_2X_SLOW   0x10    // 2×慢（若太慢，改 0x00；若還太快，改 0x10:4×慢）
#define ON_PERCENT_60        0x9A    // 約 60% 對應的百分比碼（表列含 0.4% 步進）

#if PLATFORM_MODE
int ktd202x_probe(void);

int32_t ktd202x_ch1_led_on(uint8_t level);
int32_t ktd202x_ch2_led_on(uint8_t level);
int32_t ktd202x_ch3_led_on(uint8_t level);
int32_t ktd202x_ch4_led_on(uint8_t level);

int32_t ktd202x_led_off(void);
int32_t ktd202x_ch1_led_off(void);
int32_t ktd202x_ch2_led_off(void);
int32_t ktd202x_ch3_led_off(void);
int32_t ktd202x_ch4_led_off(void);

int32_t ktd202x_ch1_led_blink(unsigned long delay_on, unsigned long delay_off, uint8_t timer_slot);
int32_t ktd202x_ch2_led_blink(unsigned long delay_on, unsigned long delay_off, uint8_t timer_slot);
int32_t ktd202x_ch3_led_blink(unsigned long delay_on, unsigned long delay_off, uint8_t timer_slot);
int32_t ktd202x_ch4_led_blink(unsigned long delay_on, unsigned long delay_off, uint8_t timer_slot);

void LEDcontrol(uint8_t  newLEDState ); //control status

int32_t ktd202x_ch2_led_breathe(uint8_t period_code,
                                uint8_t rise_code,
                                uint8_t fall_code,
                                uint8_t on_percent_code,
                                uint8_t ramp_scale_2bits,
                                bool use_pwm1,
                                uint8_t current_level );
int32_t ktd202x_ch3_led_breathe(uint8_t period_code,
                                uint8_t rise_code,
                                uint8_t fall_code,
                                uint8_t on_percent_code,
                                uint8_t ramp_scale_2bits,
                                bool use_pwm1,
                                uint8_t current_level);

#else
int ktd202x_probe(void);
uint8_t LEDstate(uint8_t SRCharging, uint8_t SRFull, uint8_t CRLowpower,
		uint8_t CRCharging, uint8_t CRFull);
void LEDcontrol(uint8_t  newLEDState );
void LED_handler(void);
#endif /* KTD202X_LEDS_H_ */
#endif
