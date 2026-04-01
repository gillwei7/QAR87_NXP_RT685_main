/*
 * hal_sar.c
 *
 *  Created on: Mar 31, 2026
 *      Author: Lydia
 */
#if UsingQAR87Board == 1
#include "hal_sar.h"
#include "sx920x.h"


void hal_sar_init(void)
{
#if SAR_SX9204_ENABLE
    if (sx920x_init() == 0)
    {
        PRINTF("[SAR] Init Success! \r\n");

        if (sx920x_compensate() == 0)
        {
            PRINTF("[SAR] Compensation Done!\r\n");
        }
        else
        {
            PRINTF("[SAR] Compensation Failed!\r\n");
        }
    }
    else
    {
        PRINTF("[SAR] Init Failed!\r\n");
    }
#endif
}

void hal_sar_handler (void)
{
#if SAR_SX9204_ENABLE

    /*
    uint8_t prox_state = 0;
    // 收到中斷信號，開始執行 I2C 讀寫
    if (sx920x_read_state(&prox_state) == 0)
    {
	    PRINTF("SAR Task: State = 0x%02X\r\n", prox_state);
	    // 處理感測邏輯...
    }
    */
	sx920x_event_t evt;
	if (sx920x_poll_event(&evt) == 0 ) {
		PRINTF("[SAR] %s\r\n", sx920x_event_to_str_zh(evt));  // 顯示：接近/人體接近/人體遠離/遠離
		/* TODO: 根據 evt 做對應動作，例如：
		   - SX920X_EVT_BODY_CLOSE: 提高掃描頻率、喚醒系統
		   - SX920X_EVT_BODY_FAR  : 恢復省電
		*/
	}

#endif
}


#endif
