#ifndef _AW_CAP_CONFIG_H_
#define _AW_CAP_CONFIG_H_

#define AW_DEBUG_LOG

#define AW_I2C_RETRIES				(5)
#define AW_I2C_RETRIES_DELAY		(2)

#define AW_CAP_CHIP_I2C_ADDR		(0x12)
#define AW_CHANNEL_NUM_MAX			(12)
#define AW_DEBUG_PRINTF			    (1)
#define AW_STM32H7_P				(1)

#define AW_BES2500_P				(0)
#if (defined(AW_BES2500_P) && (AW_BES2500_P == 1))
#define AW933XX_OS_USED							(1)
#define AW933XX_SPP_USED						(1)
#define AW_FLASH_USED						(1)
#endif
/*****************for aw93307 start*************/
#define AW933XX_CSX_TO_IRQ			(2) // CS2
/*****************for aw93307 end*************/





#endif /* _AW_CAP_CONFIG_H_ */
