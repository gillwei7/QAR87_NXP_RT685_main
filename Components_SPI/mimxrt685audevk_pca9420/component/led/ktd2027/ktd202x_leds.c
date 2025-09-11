


#include "ktd202x_leds.h"
#include "board.h"
#include "fsl_debug_console.h"

#define KTD_DEBUG
#ifdef KTD_DEBUG 
#define LOG_DBG(fmt, args...)   PRINTF("[%s]:: " fmt, ## args);
#else
#define LOG_DBG(fmt, args...)
#endif

#define KTD2026_ADDR 0x30

uint8_t ledstate = LED_OFF;
uint8_t led_status = 0;

//settings
#define TOTAL_TIME_START_COUNT       0x01
#define TOTAL_TIME_START       348 //Where is this
#define TOTAL_TIME_STEP         128

#define TIME_PERCENT_START_COUNT   0X00
#define TIME_PERCENT_STEP (4)


static int32_t ktd202x_write(uint8_t address, uint8_t value, uint32_t length)
{
    // PRINTF("address : %02x\n", address);
    // PRINTF("value:%02x\n", value);
//    uint8_t data[2];
//
//    data[0] = address;
//    data[1] = value;
    //int32_t ret = hal_i2c1_master_transmit(KTD2026_ADDR, data, length + 1);  // data is the start pointer of our array
    status_t ret = BOARD_I3C_Send(BOARD_PMIC_I3C_BASEADDR, KTD2026_ADDR, address, 1, &value, length);
    if(ret != kStatus_Success)
	{

    	PRINTF("[KTD2027] error writing register 0x%02X with value 0x%02X (status: %d)\n", address, value, ret);

	}
//    else
//    {
//
//    	PRINTF("[KTD2027] success writing register 0x%02X with value 0x%02X \n", address, value);
//    }
	return ret;
}

void ktd202x_reset(void)
{
	uint8_t val0 = 0x1F;
	ktd202x_write(0x00, val0, 1);
	PRINTF("[KTD2027] ktd202x_reset \n");
}

int ktd202x_probe(void)
{
	int err = 0;
	int i = 0;
	
	uint8_t val0 = 0x19;

	PRINTF("[KTD2027] start probe!\n");
	ktd202x_reset();
	ktd202x_write(0x00, val0, 1);
	ktd202x_write(0x04, led_status, 1);//turn off leds
	PRINTF("[KTD2027] led status = %d\n", led_status);
	return err;
}

/* Ch1: Blue */
int32_t ktd202x_ch1_led_on(uint8_t level) //blue
{
	int32_t ret;

	led_status = led_status|0x01;
	led_status = led_status & (~0x02) ;

	ret = ktd202x_write(0x06, level, 1);
	
	ret = ktd202x_write(0x04, led_status, 1);
	
	PRINTF("[KTD2027] ktd202x_ch1_led_on led_status = 0x%x, led_current=<0x%x>  \n",  led_status, level);
	return ret;
}

/* Ch2: Red */
int32_t ktd202x_ch2_led_on(uint8_t level) //red
{
	int32_t ret;

	led_status = led_status|0x04;
	led_status = led_status & (~0x08) ;

	ret = ktd202x_write(0x07, level, 1);
	
	ret = ktd202x_write(0x04, led_status, 1);
	
	PRINTF("[KTD2027] ktd202x_ch2_led_on led_status = 0x%x, led_current=<0x%x>  \n", led_status, level);
	return ret;
}

/* Ch3: Green */
int32_t ktd202x_ch3_led_on(uint8_t level) //green
{
	int32_t ret;

	led_status = led_status |0x10;
	led_status = led_status & (~0x20) ;

	ret =ktd202x_write(0x08, level, 1);
	
	ret =ktd202x_write(0x04, led_status, 1);
	
	PRINTF("[KTD2027] ktd202x_ch3_led_on led_status  = 0x%x , led_current =<0x%x> \n", led_status, level);
	return ret;
}

#ifdef KTD2027
int32_t ktd202x_ch4_led_on(uint8_t level)
{
	int32_t ret;

	ktd202x_reset();

	led_status = led_status|0x40;
	led_status = led_status & (~0x80);

	// ret = ktd202x_write(0x00, 0x1E, 1);
	// delay_ms(200);
	// ret = ktd202x_write(0x04, 0x00, 1);
	// if (ret != kStatus_Success) {
	// 	return ret;
	// }

	ret = ktd202x_write(0x09, level, 1);
	if (ret != kStatus_Success) {
		return ret;
	}

	ret = ktd202x_write(0x04, led_status, 1);
	if (ret != kStatus_Success) {
		return ret;
	}

	PRINTF("[KTD2027] ktd202x_ch4_led_on led_status  = 0x%x , led_current =<0x%x> \n", led_status, level);
	return ret;
}
#endif

//turn off leds
int32_t ktd202x_led_off(void){
	int32_t ret;
	led_status=0;

	ktd202x_reset();

	PRINTF("[KTD2027] ktd202x_led_off \n");

	//close light
	ret = ktd202x_write(0x06, led_status, 1);//set current is 0.125mA
	if (ret != kStatus_Success) {
		return ret;
	}
	ret = ktd202x_write(0x07, led_status, 1);//set current is 0.125mA
	if (ret != kStatus_Success) {
		return ret;
	}
	ret = ktd202x_write(0x08, led_status, 1);//set current is 0.125mA
	if (ret != kStatus_Success) {
		return ret;
	}
	ret = ktd202x_write(0x04, led_status, 1);//turn off leds
	if (ret != kStatus_Success) {
		return ret;
	}
	return ret;
}

/* Ch1: Blue */
int32_t ktd202x_ch1_led_off(void)
{
	int32_t ret;
	led_status = led_status & (~0x01);
	//ktd202x_write(ktd202x_i2c, 0x00, 0x08);
	ret = ktd202x_write(0x06, 0x00, 1);
	
	ret = ktd202x_write(0x04, led_status, 1);
	
	PRINTF("[KTD2027] 1 led off led_status = 0x%x  \n",  led_status);
	return ret;
}

int32_t ktd202x_ch2_led_off(void)
{
	int32_t ret;
	led_status = led_status & (~0x0C);

	ret = ktd202x_write(0x07, 0x00, 1);
	if (ret != kStatus_Success) {
		return ret;
	}
	ret = ktd202x_write(0x04, led_status, 1);
	if (ret != kStatus_Success) {
		return ret;
	}
	PRINTF("[KTD2027] 2 led off led_status = 0x%x  \n",  led_status);
	return ret;
}

int32_t ktd202x_ch3_led_off(void)
{
	int32_t ret;
	led_status = led_status & (~0x10);

	ret = ktd202x_write(0x08, 0x00, 1);
	if (ret != kStatus_Success) {
		return ret;
	}
	ret = ktd202x_write(0x04, led_status, 1);
	if (ret != kStatus_Success) {
		return ret;
	}
	PRINTF("[KTD2027] 3 led off led_status = 0x%x  \n",  led_status);
	return ret;
}

#ifdef KTD2027
int32_t ktd202x_ch4_led_off(void)
{
	int32_t ret;
	led_status = led_status & (~0xC0);

	ret = ktd202x_write(0x09, 0x00, 1);
	if (ret != kStatus_Success) {
		PRINTF("[KTD2027] ktd202x_write(0x09, 0x00, 1) failed\n");
		return ret;
	}
	ret = ktd202x_write(0x04, led_status, 1);
	if (ret != kStatus_Success) {
		PRINTF("[KTD2027] ktd202x_write(0x04, 0x%x, 1) failed\n",  led_status);
		return ret;
	}
	PRINTF("[KTD2027] led_status = 0x%x\n",  led_status);
	return ret;
}
#endif

#if 0
int32_t ktd202x_read(uint8_t address, uint8_t value, uint32_t length)
{
	uint8_t data_read[16] = {0};
	// data_read[0] = address;
	// data_read[1] = value;
	
    int32_t ret = hal_i2c1_master_transmit(KTD2026_ADDR, &address, 1);  // data is the start pointer of our array
    delay_ms(1000);
	PRINTF("===============register transmit finish================\n");
	ret = hal_i2c1_master_receive(KTD2026_ADDR, &data_read[0], length);
	for(uint8_t idx = 0; idx < 16; idx++)
	{
		PRINTF("read data:%02x\n", data_read[idx]);
	}
	return ret;
}
#endif

int ktd202x_translate_timer(unsigned long delay_on, unsigned long delay_off)
{
	int time_count = 0;
	int percent_count = 0;	
	if(delay_on==0 && delay_off ==0)
		return -1;
	
	if((delay_on + delay_off)< 348)
	{
		time_count = 1;
	}
	else
	{
		time_count = 
				(delay_on + delay_off - TOTAL_TIME_START)/TOTAL_TIME_STEP + TOTAL_TIME_START_COUNT + 1;
	}

	percent_count = 
			(delay_on*100/(delay_on + delay_off))*10 /TIME_PERCENT_STEP + TIME_PERCENT_START_COUNT + 1;
			//every timestep is 0.4 
	ktd202x_write(0x01, time_count, 1);
	ktd202x_write(0x02, percent_count, 1);
	ktd202x_write(0x03, percent_count, 1); //PWM2 Timer
	
	PRINTF("[KTD2027] time_count = %x  percent_count = %x   \n",  time_count, percent_count);
	
	return 0;
}


int32_t ktd202x_ch1_led_blink(unsigned long delay_on, unsigned long delay_off, uint8_t timer_slot)
{
	int ret=0;

	if (timer_slot == TIM_1) {
		led_status = led_status | 0x02; //Tim1
	} else if (timer_slot == TIM_2) {
		led_status = led_status | 0x03; //Tim2
	}

	ret = ktd202x_translate_timer(delay_on, delay_off);
	if(ret < 0)
	{
		ret = ktd202x_ch1_led_off();
		if (ret != kStatus_Success) {
			return ret;
		}
		return ret;
	}
	ret = ktd202x_write(0x05, 0x00, 1); //timer basis default
	if (ret != kStatus_Success) {
		return ret;
		PRINTF("[KTD2027] REG 05 failed");
	}
	else
	{
		// PRINTF("OK\n");
	}
	ret = ktd202x_write(0x06, LED_CURRENT_CH1, 1);
	if (ret != kStatus_Success) {
		return ret;
	}
	ret = ktd202x_write(0x04, led_status, 1); //pwm 1
	if (ret != kStatus_Success) {
		PRINTF("[KTD2027] PWM1 open failed");
		return ret;
	}
	PRINTF("[KTD2027] ktd202x_ch1_led_blink \n");

	return ret;
}

/* Ch2: Red */
int32_t ktd202x_ch2_led_blink(unsigned long delay_on, unsigned long delay_off, uint8_t timer_slot)
{
	int ret=0;

	if (timer_slot == TIM_1) {
		led_status = led_status | 0x08; //Tim1
	} else if (timer_slot == TIM_2) {
		led_status = led_status | 0x0C; //Tim2
	}

	ret = ktd202x_translate_timer(delay_on, delay_off);
	if(ret < 0)
	{
		ret = ktd202x_ch2_led_off();
		if (ret != kStatus_Success) {
			return ret;
		}
		return ret;
	}
	//ktd202x_write(ktd202x_i2c, 0x00, 0x18);
	ret = ktd202x_write(0x05, 0x00, 1);
	if (ret != kStatus_Success) {
		return ret;
	}
	ret = ktd202x_write(0x07, LED_CURRENT_CH2, 1);
	if (ret != kStatus_Success) {
		return ret;
	}
	ret = ktd202x_write(0x04, led_status, 1);
	if (ret != kStatus_Success) {
		return ret;
	}
	PRINTF("[KTD2027] ktd202x_ch2_led_blink \n");

	return ret;
}

/* Ch3: Green */
int32_t ktd202x_ch3_led_blink(unsigned long delay_on, unsigned long delay_off, uint8_t timer_slot)
{
	int ret=0;

	if (timer_slot == TIM_1) {
		led_status = led_status | 0x20; //Tim1
	} else if (timer_slot == TIM_2) {
		led_status = led_status | 0x30; //Tim2
	}

	ret = ktd202x_translate_timer(delay_on, delay_off);
	if(ret < 0)
	{
		ret = ktd202x_ch3_led_off();
		if (ret != kStatus_Success) {
			return ret;
		}
		return ret;
	}
	//ktd202x_write(ktd202x_i2c, 0x00, 0x18);
	ret = ktd202x_write(0x05, 0x00, 1);
	if (ret != kStatus_Success) {
		return ret;
	}
	ret = ktd202x_write(0x08, LED_CURRENT_CH3, 1);
	if (ret != kStatus_Success) {
		return ret;
	}
	ret = ktd202x_write(0x04, led_status, 1);
	if (ret != kStatus_Success) {
		return ret;
	}
	PRINTF("[KTD2027] ktd202x_ch3_led_blink \n");

	return ret;
}

#ifdef KTD2027
int32_t ktd202x_ch4_led_blink(unsigned long delay_on, unsigned long delay_off, uint8_t timer_slot)
{
	int ret = 0;

	if (timer_slot == TIM_1) {
		led_status = led_status | 0x80; //Tim1
	} else if (timer_slot == TIM_2) {
		led_status = led_status | 0xC0; //Tim2
	}

	ret = ktd202x_translate_timer(delay_on, delay_off);
	if(ret < 0)
	{
		ret = ktd202x_ch4_led_off();
		if (ret != kStatus_Success) {
			return ret;
		}
		return ret;
	}
	//ktd202x_write(ktd202x_i2c, 0x00, 0x18);
	ret = ktd202x_write(0x05, 0x00, 1);
	if (ret != kStatus_Success) {
		return ret;
	}
	ret = ktd202x_write(0x09, LED_CURRENT, 1);
	if (ret != kStatus_Success) {
		return ret;
	}
	ret = ktd202x_write(0x04, led_status, 1);
	if (ret != kStatus_Success) {
		return ret;
	}
	PRINTF("[KTD2027] ktd202x_ch4_led_blink \n");
	return ret;

}
#endif

#if 0
void LEDcontrol(uint8_t  newLEDState )
{
	int32_t ret;
	int level = 0x9F;
	unsigned long delay_on = 500;
	unsigned long delay_off = 500;
    if( ledstate == newLEDState )
    {
    	return;
    }

	ret = ktd202x_led_off();

	if( newLEDState == BLUE_BLINK)
	{
		ret = ktd202x_ch1_led_blink(delay_on, delay_off);
		PRINTF("[KTD2027] Turn blue blink \r\n");
	}
	else if( newLEDState == RED_BLINK)
	{
		ret = ktd202x_ch2_led_blink(delay_on, delay_off);
		PRINTF("[KTD2027] Turn red blink \r\n");
	}
	else if( newLEDState == GREEN_BLINK)
	{
		ret = ktd202x_ch3_led_blink(delay_on, delay_off);
		PRINTF("[KTD2027] Turn green blink \r\n");
	}
	
	else if( newLEDState == BLUE_ON)
	{
		ret = ktd202x_ch1_led_on(level);
		PRINTF("[KTD2027] blue on\n");
	}
	else if( newLEDState == RED_ON)
	{
		ret = ktd202x_ch2_led_on(level);
		PRINTF("[KTD2027] red on\n");
	}
	else if( newLEDState == GREEN_ON)
	{
		ret = ktd202x_ch3_led_on(level);
		PRINTF("[KTD2027] green on\n");
	}
	#if KTD2027
	else if(newLEDState == WHITE_BLINK)
	{
		ret = ktd202x_ch4_led_blink(delay_on, delay_off);
		PRINTF("[KTD2027] Turn white blink \r\n");
	}
	else if(newLEDState == WHITE_ON)
	{
		ret = ktd202x_ch4_led_on(delay_on, delay_off);
		PRINTF("[KTD2027] Turn white blink \r\n");
	}
	#endif
    if (ret == kStatus_Success) {
    	ledstate = newLEDState;
    }
}
#endif
