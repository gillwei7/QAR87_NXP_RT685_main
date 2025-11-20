/******************************************************************************
 * @file     glf70302.h
 * @brief
 * All Rights Reserved
 * History: Introduce the difference from previous version.
 * Revision          Date           Author            Desc
 * 0.1            2024/11/21        cyb        build this file
 * 0.2			  2024/11/22	    cyb        Add shutdown_withsave() and wake_withload function
 * 0.3			  2025/01/17	    cyb        Add NTC trim function
 * 0.4			  2025/06/23	    cyb        Remove NTC trim function(AB version has supported different type NTC)
                                               Add SOH read function
 * 0.5			  2025/07/25	    cyb        Use REG62 to save the dataflash version
 											   Add optional flash page program
  * 0.6			  2025/11/07	    cyb		   Add exitshutdown command and delay in glf70302_init()
 ******************************************************************************/

#ifndef SRC_GLF70302_H_
#define SRC_GLF70302_H_

#define GLF70302_ADDR			0x55

#define GLF70302REG_OCV		0x00
#define GLF70302REG_NTCTEMP	0x08
#define GLF70302REG_DIETEMP	0x0D
#define GLF70302REG_SOC		0x09
#define GLF70302REG_SOH		0x0A
#define GLF70302REG_CYCLE		0x06
#define GLF70302REG_VBAT		0x3C
#define GLF70302REG_IBAT		0x3A
#define GLF70302REG_CHIPID	0x40

#define GLF70302REG_TEMPHOST	0x21

#define GLF70302REG_TEMPHIGH	0x81
#define GLF70302REG_TEMPLOW	0x82
#define GLF70302REG_SOCLOW	0x83
#define GLF70302REG_DFVER		0x83	//bat profile和SOCLOW寄存器复用,默认值为0x14
#define GLF70302REG_ADCCFG1	0x84
#define GLF70302REG_ADCCFG2	0x85
#define GLF70302REG_DEVCFG1	0x86
#define GLF70302REG_DEVCFG2	0x87

#define GLF70302REG_CFGA		0x60
#define GLF70302REG_DFVERSAVE 0x62
#define GLF70302REG_MEMSET	0x19
#define GLF70302REG_SET		0x18
#define GLF70302REG_COMM		0x20

#define GLF70302REG_IGAIN		0xA4
#define GLF70302REG_IOFFSET	0xA8

#define GLF70302_PFOFILESIZE	160
#define GLF70302_PROFILEADDR	0x640
#define GLF70302_PROFILEPAGE	249
#define GLF70302_PROFILEPAGESIZE	3

/*********** The following configurations need to be adjusted according to the project. **************/
#define DFVERSION	0x01
//#define SYLOGENABLE
//#define SYCALENABLE
//#define SYHOSTTEMP
//#define I2CPROCONT

typedef struct {
    uint8_t soc;         // 電量百分比 (%)
    uint16_t voltage;    // 電壓 (mV)
    int16_t current;     // 電流 (mA)
    int8_t temperature;  // 溫度 (°C)
} BatteryInfo;


static uint16_t ibatgaintrimed = 3015;
static int8_t 	ibatoffsettrimed = 3;

static uint8_t config_profile_info[GLF70302_PFOFILESIZE] = {
		0x3d,0x2d,0x00,0x01,0xe1,0xbc,0x49,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x7d,0x00,0x83,0x68,0x01,0x5f,0xfb,0x68,0x01,0x20,0x07,0x52,0x0d,0x0a,0x00,
		0x30,0x00,0x60,0x00,0x40,0x11,0x33,0x00,0x1a,0x00,0x10,0x00,0x07,0x00,0x58,0x0e,
		0x34,0x0d,0xcf,0x0d,0x2b,0x0e,0x61,0x0e,0x6a,0x0e,0x6d,0x0e,0x70,0x0e,0x7a,0x0e,
		0x87,0x0e,0x93,0x0e,0x9d,0x0e,0xb1,0x0e,0xc8,0x0e,0xe0,0x0e,0xf1,0x0e,0x03,0x0f,
		0x19,0x0f,0x30,0x0f,0x4c,0x0f,0x6e,0x0f,0xaa,0x0f,0xd7,0x0f,0x00,0x10,0x2d,0x10,
		0x5a,0x10,0x89,0x10,0xb8,0x10,0xe6,0x10,0x0e,0x11,0x30,0x11,0x51,0x11,0x14,0x00,
		0x2a,0x00,0x50,0x00,0x7e,0x00,0xb6,0x00,0x38,0x01,0x4d,0x01,0x99,0x01,0xf6,0x01,
		0x2d,0x19,0x0f,0x0a,0x05,0x00,0xfb,0xf6,0xf1,0x08,0x04,0x06,0x05,0x07,0x08,0x07,
		0x08,0x08,0x08,0x08,0x08,0x0a,0x10,0x48,0x00,0x00,0x03,0x00,0xd0,0x02,0x00,0x00,
};

static uint16_t profilecrc = 0xC86A;

/***********************************/

  uint8_t 		glf70302_read_one_byte(uint8_t regaddr);
  uint16_t 	glf70302_read_two_bytes(uint8_t regaddr);
  void 		glf70302_write_one_byte(uint8_t regaddr,uint8_t regdata);
  void 		glf70302_write_two_byte(uint8_t regaddr,uint16_t regdata);
  void 		glf70302_write_one_byte_clken(uint8_t regaddr,uint8_t regdata);
  void 		glf70301_write_page(uint16_t addr,uint8_t *pData,uint8_t size);
  int8_t 		glf70302_read_ntctemp();
  int8_t 		glf70302_read_dietemp();
  uint8_t 		glf70302_read_soc_();
  uint8_t 		glf70302_read_soh();
  uint16_t 	glf70302_read_vbat();
  int16_t 		glf70302_read_ibat();
  uint16_t 	glf70302_readcyclecount();
  uint16_t 	glf70302_readid();
  void 		glf70302_enter_shutdown();
  void 		glf70302_exit_shutdown();
  void 		glf70302_regclken();
  void			glf70302_regclkdis();
  void 		glf70302_unseal();
  void 		glf70302_reset();
  void 		glf70302_writetemp(int8_t hosttemperature);
  void 		glf70302_write_flhword(uint16_t flashaddr,uint32_t flashdata);
  uint32_t 	glf70302_read_flhword(uint16_t flashaddr);
  uint32_t 	profileprogram(uint8_t *pData,uint16_t crcresult);
  uint32_t 	glf70302_ibatcal(uint16_t ibatgain,int8_t ibatoffset);
  void 		glf70302_init();
  void 		glf70302_polling(BatteryInfo *info);
  void 		glf70302_shutdown_withsave();
  void 		glf70302_wake_withload(uint8_t sochost,uint16_t ocvhost,uint8_t socthd);

#endif

