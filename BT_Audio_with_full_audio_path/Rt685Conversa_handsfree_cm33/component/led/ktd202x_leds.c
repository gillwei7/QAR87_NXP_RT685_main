


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
	ret = ktd202x_write(0x09, LED_CURRENT_CH4, 1);
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

// 寫 Ramp 時間：low nibble=Trise code, high nibble=Tfall code
static inline status_t ktd202x_set_ramp(uint8_t rise_code, uint8_t fall_code) {
    uint8_t reg5 = ((fall_code & 0x0F) << 4) | (rise_code & 0x0F);
    return ktd202x_write(0x05, reg5, 1);            // Reg5
}

// 設定 Ramp 速度倍率：Reg0[6:5] = 00:1x, 01:2x慢, 10:4x慢, 11:8x快
static inline status_t ktd202x_set_ramp_scale(uint8_t scale2bits) {
    // 讀改寫 Reg0（若你沒有 read API，先保存本地鏡像或直接覆蓋其他位保持預設）
    // 這裡假設其他位保持 0：TimerSlot=Slot1、EnCtrl=Always ON、Test=0
    uint8_t reg0 = (scale2bits & 0x03) << 5;        // Reg0[6:5]
    return ktd202x_write(0x00, reg0, 1);            // Reg0
}

// 設定週期與 ON 百分比（Ton%），選 PWM1 或 PWM2
static inline status_t ktd202x_set_period_and_on(uint8_t period_code, uint8_t on_percent_code, bool use_pwm1) {
    // Reg1：週期 + Ramp 型態（若要線性，將 bit7 置 1）
    status_t ret = ktd202x_write(0x01, period_code, 1);     // Reg1
    if (ret != kStatus_Success) return ret;
    // Reg2/Reg3：PWM1 / PWM2 的 ON 百分比
    return ktd202x_write(use_pwm1 ? 0x02 : 0x03, on_percent_code, 1);
}

// ch3（綠）呼吸：period_ms 對應 period_code；rise/fall 用 nibble code；Ton% 用百分比碼
int32_t ktd202x_ch3_led_breathe(uint8_t period_code,
                                uint8_t rise_code,
                                uint8_t fall_code,
                                uint8_t on_percent_code,
                                uint8_t ramp_scale_2bits,
                                bool use_pwm1,
                                uint8_t current_level /* e.g., LED_CURRENT_CH3 */) {
    status_t ret;

    // 1) 電流（亮度）
    ret = ktd202x_write(0x08, current_level, 1);     // Reg8: Ch3 Iout
    if (ret != kStatus_Success) return ret;

    // 2) Ramp 設定
    ret = ktd202x_set_ramp(rise_code, fall_code);    // Reg5
    if (ret != kStatus_Success) return ret;
    ret = ktd202x_set_ramp_scale(ramp_scale_2bits);  // Reg0[6:5]
    if (ret != kStatus_Success) return ret;

    // 3) 週期 + ON 百分比
    ret = ktd202x_set_period_and_on(period_code, on_percent_code, use_pwm1); // Reg1 + Reg2/3
    if (ret != kStatus_Success) return ret;

    // 4) 通道模式：先清再設（避免位元殘留）
    // Ch3 的 2-bit 模式位元是 Reg4 的 [5:4]，你的 led_status 用對應遮罩 0x30
    led_status &= ~0x30;                              // 清 Ch3 模式
    led_status |= (use_pwm1 ? 0x20 : 0x30);           // 10:PWM1 或 11:PWM2
    ret = ktd202x_write(0x04, led_status, 1);         // Reg4
    if (ret != kStatus_Success) return ret;

    //LOG_DBG("[KTD202x] ch3 breathe: period=0x%02X rise=0x%X fall=0x%X on%%=0x%02X mode=%s\r\n",
    //        period_code, rise_code, fall_code, on_percent_code, use_pwm1 ? "PWM1" : "PWM2");
    PRINTF("[KTD2027] ktd202x_ch3_led_breathe \r\n");

    return ret;
}


// === 紅燈（Ch2）呼吸 ===
// period_code      : Reg1 週期碼（64ms 起，每步 +256ms，最長約 8s）
// rise_code/fall_code : Reg5 的 Trise/Tfall 檔位（各 4bit；未套倍率前約 128ms 級距）
// on_percent_code  : Reg2/Reg3 的 ON 時間百分比碼（Ton 含 Trise）
// ramp_scale_2bits : Reg0[6:5] 的倍率（00=1×、01=2×慢、10=4×慢、11=8×快）
// use_pwm1         : true=用 PWM1（寫 Reg2），false=用 PWM2（寫 Reg3）
// current_level    : 紅燈目標電流（例：LED_CURRENT_CH2；建議先用 0x80~0x9F 測亮度）

int32_t ktd202x_ch2_led_breathe(uint8_t period_code,
                                uint8_t rise_code,
                                uint8_t fall_code,
                                uint8_t on_percent_code,
                                uint8_t ramp_scale_2bits,
                                bool use_pwm1,
                                uint8_t current_level /* e.g., LED_CURRENT_CH2 */)
{
    status_t ret;

    // 1) 設定紅燈亮度電流（Reg7）
    ret = ktd202x_write(0x07, current_level, 1);  // Ch2 Iout
    if (ret != kStatus_Success) return ret;

    // 2) Ramp 設定（Reg5 + Reg0[6:5]）
    //    注意：Reg5 低 4bit 是 Trise， 高 4bit 是 Tfall
    uint8_t reg5 = ((fall_code & 0x0F) << 4) | (rise_code & 0x0F);
    ret = ktd202x_write(0x05, reg5, 1);           // Reg5: Ramp
    if (ret != kStatus_Success) return ret;

    // Reg0[6:5] 設倍率；此處假設其他位保持預設（TimerSlot=Slot1、EnCtrl=Always ON）
    uint8_t reg0 = (ramp_scale_2bits & 0x03) << 5;
    ret = ktd202x_write(0x00, reg0, 1);           // Reg0: Ramp Scaling
    if (ret != kStatus_Success) return ret;

    // 3) 週期與 ON 百分比（Reg1 + Reg2/Reg3）
    ret = ktd202x_write(0x01, period_code, 1);    // Reg1: Period（若需線性 ramp，將 Reg1[7] 置 1）
    if (ret != kStatus_Success) return ret;

    ret = ktd202x_write(use_pwm1 ? 0x02 : 0x03, on_percent_code, 1); // Reg2/Reg3: ON%
    if (ret != kStatus_Success) return ret;

    // 4) 通道模式（Reg4）：先清再設，避免殘留
    //    紅燈（Ch2）的模式遮罩是 0x0C（00:OFF, 01:ON, 10:PWM1, 11:PWM2）
    led_status &= ~0x0C;                          // 清 Ch2 模式位
    led_status |= (use_pwm1 ? 0x08 : 0x0C);       // 設為 PWM1 或 PWM2
    ret = ktd202x_write(0x04, led_status, 1);     // Reg4: Channel Control
    if (ret != kStatus_Success) return ret;

    //LOG_DBG("[KTD202x] ch2 breathe: period=0x%02X rise=0x%X fall=0x%X on%%=0x%02X mode=%s\r\n",
    //        period_code, rise_code, fall_code, on_percent_code, use_pwm1 ? "PWM1" : "PWM2");
    PRINTF("[KTD2027] ktd202x_ch2_led_breathe \r\n");

    return ret;
}


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
