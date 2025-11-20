/******************************************************************************
 * @file     glf70302.c
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

#include "board.h"
#include "glf70302_driver.h"
#include "fsl_debug_console.h"
#include "stdio.h"

#if 0
static void HAL_Delay(unsigned int ms)
{
	SDK_DelayAtLeastUs(ms * 1000, CLOCK_GetFreq(kCLOCK_CoreSysClk));
	//vTaskDelay(pdMS_TO_TICKS(ms));
}

uint8_t glf70302_read_one_byte(uint8_t regaddr)
{
    uint8_t rx;
    status_t st = BOARD_I3C_Receive(
									BOARD_PMIC_I3C_BASEADDR,
									GLF70302_ADDR,
									regaddr,
									1,
									&rx,
									1);
    return rx;

}

uint16_t glf70302_read_two_bytes(uint8_t regaddr)
{
    uint8_t rx[2];
    status_t st = BOARD_I3C_Receive(
									BOARD_PMIC_I3C_BASEADDR,
									GLF70302_ADDR,
									regaddr,
									1,
									rx,
									2);
    uint16_t val = ((uint16_t)rx[1] << 8) | (uint16_t)(rx[0] & 0xFF);
	return val;
}

void glf70302_write_one_byte(uint8_t regaddr,uint8_t regdata)
{
    (void)BOARD_I3C_Send(
    					BOARD_PMIC_I3C_BASEADDR,
						GLF70302_ADDR,
						regaddr,
						1,
						&regdata,
						1);

	}

void glf70302_write_two_byte(uint8_t regaddr,uint16_t regdata)
{

    uint8_t tx[2];
    tx[0] = (uint8_t)(regdata & 0xFF);
    tx[1] = (uint8_t)(regdata >> 8);

    (void)BOARD_I3C_Send(
    					BOARD_PMIC_I3C_BASEADDR,
						GLF70302_ADDR,
						regaddr,
						1,
						tx,
						2);

	}

void glf70302_write_page(uint16_t addr,uint8_t *pData,uint8_t size)
{
	uint8_t databuffer[68],i;
	glf70302_write_two_byte(0x12,addr);
	glf70302_write_one_byte(0x10,0x05);
	HAL_Delay(4);
	databuffer[0] = 0x08;
	databuffer[1] = 0x00;
	databuffer[2] = 0xC0;
	databuffer[3] = 0x07;
	for(i=0;i<size;i++)
	{
		databuffer[i+4] = pData[i];
	}
	for(i=size;i<64;i++) //Fill 0x00 if the pData less than 64 btyes.
	{
		databuffer[i+4] = 0x00;
	}
	//HAL_I2C_Mem_Write(&hi2c2,GLF70302_ADDR,0x10,I2C_MEMADD_SIZE_8BIT,databuffer, 68, 0xfff);
    (void)BOARD_I3C_Send(
    					BOARD_PMIC_I3C_BASEADDR,
						GLF70302_ADDR,
						0x10,
						1,
						databuffer,
						68);
	glf70302_write_two_byte(0x12,addr);
	glf70302_write_one_byte(0x10,0x06);
	HAL_Delay(4);
	}

void glf70302_write_one_byte_clken(uint8_t regaddr,uint8_t regdata)
{
	glf70302_regclken();
	glf70302_write_one_byte(regaddr,regdata);
	glf70302_regclkdis();
	}

int8_t glf70302_read_ntctemp()
{
	return (int8_t)glf70302_read_one_byte(GLF70302REG_NTCTEMP);
	}

int8_t glf70302_read_dietemp()
{
	return (int8_t)glf70302_read_one_byte(GLF70302REG_DIETEMP);
	}

uint8_t glf70302_read_soc_()
{
	return glf70302_read_one_byte(GLF70302REG_SOC);
	}

uint8_t glf70302_read_soh()
{
	return glf70302_read_one_byte(GLF70302REG_SOH);
	}

uint16_t glf70302_read_vbat()
{
	return glf70302_read_two_bytes(GLF70302REG_VBAT);
	}

int16_t glf70302_read_ibat()
{
	uint16_t ibat;
	ibat = glf70302_read_two_bytes(GLF70302REG_IBAT);
	if(ibat == 0x8000)
		ibat = 0;
	return (int16_t)ibat;
	}

uint16_t glf70302_readcyclecount()
{
	return glf70302_read_two_bytes(GLF70302REG_CYCLE);
	}

uint16_t glf70302_readid()
{
	return glf70302_read_two_bytes(GLF70302REG_CHIPID);
	}

void glf70302_enter_shutdown()
{
	glf70302_write_one_byte(GLF70302REG_CFGA,0x41);
	}

void glf70302_exit_shutdown()
{
	glf70302_write_one_byte(GLF70302REG_CFGA,0x40);//退出shutdown可能丢失ACK，如果发生I2C通信错误则重发即可
	}

void glf70302_regclken()
{
	glf70302_write_one_byte(GLF70302REG_MEMSET,0x10);
	}

void glf70302_regclkdis()
{
	glf70302_write_one_byte(GLF70302REG_MEMSET,0x00);
	}

void glf70302_unseal()
{
	glf70302_write_one_byte(GLF70302REG_MEMSET,0x1F);
	HAL_Delay(1);
	glf70302_write_one_byte(0x86,0x00);   //disable watchdog
	glf70302_write_two_byte(0x14,0xA8C1);
	glf70302_write_two_byte(0x16,0x1623);
	glf70302_write_one_byte(0x10,0x24);

	glf70302_write_two_byte(0x14,0x4824);
	glf70302_write_two_byte(0x16,0x1564);
	glf70302_write_one_byte(0x10,0x23);
	glf70302_write_one_byte(0x10,0x20);

//如果不需要校准电流值，则不要解锁校准区域FLAHS擦写权限，避免误操作
#ifdef SYCALENABLE
	glf70302_write_two_byte(0x14,0xD02F);
	glf70302_write_two_byte(0x16,0xAA65);
	glf70302_write_one_byte(0x10,0x22);
#endif

	HAL_Delay(1);
	//uint8_t memstatus = glf70302_read_one_byte(0x11);
	//PRINTF("memstatus = 0x%x\r\n",memstatus);
	}

void glf70302_reset()
{
	glf70302_enter_shutdown();
	glf70302_exit_shutdown();
	HAL_Delay(4);
	}

void glf70302_writetemp(int8_t hosttemperature)
{
	glf70302_write_one_byte_clken(GLF70302REG_TEMPHOST,hosttemperature);
	}

void glf70302_write_flhword(uint16_t flashaddr,uint32_t flashdata)
{
	uint16_t flashdataL,flashdataH;
	flashdataL = flashdata & 0xFFFF;
	flashdataH = flashdata >> 16;
	glf70302_write_two_byte(0x12,flashaddr);
	glf70302_write_two_byte(0x14,flashdataL);
	glf70302_write_two_byte(0x16,flashdataH);
	glf70302_write_one_byte(0x10,0x02);
	HAL_Delay(8);
	}

uint32_t glf70302_read_flhword(uint16_t flashaddr)
{
	uint16_t flashdataL,flashdataH;
	uint32_t flashdata;
	glf70302_write_two_byte(0x12,flashaddr);
	glf70302_write_one_byte(0x10,0x01);
	flashdataL = glf70302_read_two_bytes(0x14);
	flashdataH = glf70302_read_two_bytes(0x16);
	flashdata = (flashdataH << 16) + flashdataL;
	return flashdata;
	}

uint32_t profileprogram(uint8_t *pData,uint16_t crcresult)
{
	uint16_t i,flhaddr,crcdata;
	glf70302_unseal();
#ifdef	I2CPROCONT
	PRINTF("program DataFlash by page\r\n");
	glf70302_write_page(0x3E40,pData,64);
	glf70302_write_page(0x3E80,(pData+64),64);
	glf70302_write_page(0x3EC0,(pData+128),32);
#else
	PRINTF("program DataFlash by word\r\n");
	flhaddr=GLF70302_PROFILEADDR;
	for(i=0;i<GLF70302_PFOFILESIZE;i=i+4) //将参数写入flash,先写入buffer然后每64字节写入一次Flash
	{
		glf70302_write_two_byte(0x12,flhaddr+i);
		glf70302_write_one_byte(0x14,pData[i]);
		glf70302_write_one_byte(0x15,pData[i+1]);
		glf70302_write_one_byte(0x16,pData[i+2]);
		glf70302_write_one_byte(0x17,pData[i+3]);

		if((i==60)||(i==124)||(i==(GLF70302_PFOFILESIZE-4)))
		{
			glf70302_write_two_byte(0x12,flhaddr+i+0x3800);
			glf70302_write_one_byte(0x10,0x02);
			HAL_Delay(8);
		}
		else
		{
			glf70302_write_one_byte(0x10,0x03);
		}
	}
#endif
	glf70302_write_one_byte(0x12,GLF70302_PROFILEPAGE); //计算校验值
	glf70302_write_one_byte(0x13,GLF70302_PROFILEPAGESIZE);
	glf70302_write_one_byte(0x10,0x07);
	HAL_Delay(1);
	crcdata = glf70302_read_two_bytes(0x14);
	glf70302_reset();
	//PRINTF("CRC= 0x%x\r\n",crcdata);
	if(crcdata == crcresult)
		return 0;
	else
		return 1;
	}


uint32_t glf70302_ibatcal(uint16_t ibatgain,int8_t ibatoffset)
{
	uint32_t databuff;
	glf70302_unseal();
	databuff = glf70302_read_flhword(0x3E04);
	databuff = (databuff&0xFFFF0000)|ibatgain;
	glf70302_write_flhword(0x3E04,databuff);

	databuff = glf70302_read_flhword(0x3E08);
	databuff = (databuff&0xFFFFFF00)|(ibatoffset&0x000000FF);
	glf70302_write_flhword(0x3E08,databuff);
	glf70302_reset();

	uint16_t igainprog 		= glf70302_read_two_bytes(GLF70302REG_IGAIN);
	int8_t	 ioffsetprog 	= glf70302_read_one_byte(GLF70302REG_IOFFSET);
	if((igainprog == ibatgain) && (ioffsetprog == ibatoffset))
		return 0;
	else
		return 1;

	}

void glf70302_init()
{
	uint8_t updateflag1,updateflag2;
	glf70302_exit_shutdown();//确保芯片不在shuttudown状态
	HAL_Delay(5); //上电或者退出shutdown进行初始化前需要等待5mS时间
	updateflag1 = glf70302_read_one_byte(GLF70302REG_DFVER);
	updateflag2 = glf70302_read_one_byte(GLF70302REG_DFVERSAVE);
	if((updateflag1 != DFVERSION) && (updateflag2 != DFVERSION))
	{
#ifdef SYCALENABLE
		if(glf70302_ibatcal(ibatgaintrimed,ibatoffsettrimed) == 0x00)
			PRINTF("calibrated Successfully\r\n ");
		else
			PRINTF("calibrated failed\r\n");
#endif
		if(profileprogram(config_profile_info,profilecrc)==0x00)
		{
			glf70302_write_one_byte(GLF70302REG_DFVERSAVE,DFVERSION); //将profile版本号复制一份到GLF70302REG_DFVERSAVE寄存器，避免后续修改GLF70302REG_SOCLOW导致主控重新上电误判需要重新烧录
			PRINTF("Programmed Successfully\r\n ");
		}
		else
			PRINTF("Programmed failed\r\n");
	}
	else
	{
		glf70302_write_one_byte(GLF70302REG_DFVERSAVE,DFVERSION); //将profile版本号复制一份到GLF70302REG_DFVERSAVE寄存器，避免后续修改GLF70302REG_SOCLOW导致主控重新上电误判需要重新烧录
		PRINTF("The bat profile has been programmed\r\n");
	}

#ifdef SYLOGENABLE
	uint8_t regdevcfg2;
	regdevcfg2 = glf70302_read_one_byte(GLF70302REG_DEVCFG2);
	regdevcfg2 = regdevcfg2 | 0x01;
	glf70302_write_one_byte_clken(GLF70302REG_DEVCFG2,regdevcfg2);
#endif

#ifdef SYHOSTTEMP
	uint8_t regset;
	regset = glf70302_read_one_byte(GLF70302REG_SET);
	regset = regset | 0x01;
	glf70302_write_one_byte_clken(GLF70302REG_SET,regset);
#endif
	//glf70302_write_one_byte_clken(GLF70302REG_SOCLOW,0x05);//如果需要使用SOC_LOW中断在此处重新配置
	//HAL_Delay(140);//需要等待延时140mS才能读取第一次BAT/IBAT/Temp/SOC
	}

void glf70302_polling()
{
	int8_t		ntctemp;
	int8_t 		dietemp;
	uint8_t 	soc;
	uint8_t 	soh;
	uint16_t 	vbat;
	int16_t 	ibat;
	uint16_t 	cycle;
	ntctemp = glf70302_read_ntctemp();
	soc 	= glf70302_read_soc_();
	vbat 	= glf70302_read_vbat();
	ibat	= glf70302_read_ibat();
	dietemp	= glf70302_read_dietemp();
	cycle	= glf70302_readcyclecount();
	soh 	= glf70302_read_soh();
	PRINTF("ntctemp = %dC, dietemp = %dC, vbat = %dmV, ibat = %dmA, soc = %d%%, cycle = %d, soh =%d%%\r\n",ntctemp,dietemp,vbat,ibat,soc,cycle,soh);
	}

void glf70302_shutdown_withsave()
{
    uint8_t  glf70302_socbf = glf70302_read_one_byte(GLF70302REG_SOC);
    uint16_t glf70302_ocvbf = glf70302_read_two_bytes(GLF70302REG_OCV);
    // host save glf70302_socbf and glf70302_ocvbf to host memory
    PRINTF("before shutdown ocv = %d, soc = %d\r\n",glf70302_ocvbf,glf70302_socbf);
    glf70302_enter_shutdown();
	}

void glf70302_wake_withload(uint8_t sochost,uint16_t ocvhost,uint8_t socthd)
{
	glf70302_exit_shutdown();
	HAL_Delay(130);
	uint8_t socnew = glf70302_read_one_byte(GLF70302REG_SOC);
	if(((socnew - sochost) < socthd) && ((sochost - socnew) < socthd))  //如果SOC和shutdown前差异不大，说明shutdown时间短，可继续用上次保存数据。如果差异大则说明shutdown时间长，应使用ADC采集的数据
	{
		glf70302_write_one_byte(0x19,0x11);   //enable clk
		glf70302_write_one_byte(0x1B,sochost);
		uint8_t ocvhostL = ocvhost & 0xFF;
		uint8_t ocvhostH = ocvhost >> 8;
		glf70302_write_one_byte(0x22,ocvhostL);
		glf70302_write_one_byte(0x23,ocvhostH);
		glf70302_write_one_byte(GLF70302REG_SET,0x0A); //use host soc & vbat
		glf70302_write_two_byte(0x14,0x4824);
		glf70302_write_two_byte(0x16,0x1564);
		glf70302_write_one_byte(0x10,0x23);  //unseal
		glf70302_write_one_byte(GLF70302REG_COMM,0x01); //mcu reset
		glf70302_write_one_byte(GLF70302REG_COMM,0x03); //mcu wake
		HAL_Delay(1);
		glf70302_write_one_byte(GLF70302REG_COMM,0x03); //mcu wake
		HAL_Delay(2);
		glf70302_write_one_byte(GLF70302REG_SET,0x00); //use host soc & vbat
		glf70302_write_one_byte(0x10,0x26); //seal
		glf70302_write_one_byte(0x19,0x00);
        uint8_t  socload = glf70302_read_one_byte(GLF70302REG_SOC);
        uint16_t ocvload = glf70302_read_two_bytes(GLF70302REG_OCV);
        PRINTF("soc reload,soc = %d, ocv =%d\r\n",socload,ocvload);
		}
	else
		PRINTF("use new soc \r\n");
	}
#endif
