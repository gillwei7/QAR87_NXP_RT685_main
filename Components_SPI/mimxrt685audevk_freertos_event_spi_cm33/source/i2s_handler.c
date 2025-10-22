/*
 * i2s_handler.c
 *
 *  Created on: 2025年10月22日
 *      Author: 11301026
 */


#include "i2s_handler.h"
#include "music.h"

/*====== I2S prototype======*/
dma_handle_t s_DmaTxHandle;
i2s_config_t s_TxConfig;
i2s_dma_handle_t s_TxHandle;
i2s_transfer_t s_TxTransfer;

static void TxCallback(I2S_Type *base, i2s_dma_handle_t *handle, status_t completionStatus, void *userData)
{
    /* Enqueue the same original buffer all over again */
    i2s_transfer_t *transfer = (i2s_transfer_t *)userData;
    I2S_TxTransferSendDMA(base, handle, *transfer);
}
void StopSoundPlayback(void)
{
    PRINTF("[I2S1]Stopping sound playback\r\n");

    I2S_TransferAbortDMA(DEMO_I2S_TX_toAmp, &s_TxHandle);
}

void StartSoundPlayback(void)
{


    PRINTF("[I2S1]Setup looping playback of sine wave \r\n");

    s_TxTransfer.data     = &g_Music[0];
    s_TxTransfer.dataSize = sizeof(g_Music);

    I2S_TxTransferCreateHandleDMA(DEMO_I2S_TX_toAmp,  &s_TxHandle, &s_DmaTxHandle, TxCallback, (void *)&s_TxTransfer);
    I2S_TxTransferSendDMA(DEMO_I2S_TX_toAmp,  &s_TxHandle, s_TxTransfer);


}

void Init_I2S(void)
{
	PRINTF("[I2S]Configure I2S \r\n");

    I2S_TxGetDefaultConfig(&s_TxConfig);

    // I2S 32bits
    s_TxConfig.dataLength  = 32;
    s_TxConfig.frameLength = 64;
    s_TxConfig.divider     = 8;//(24576000U / 16000U / 32U / 2);//DEMO_I2S_CLOCK_DIVIDER;
    s_TxConfig.masterSlave = DEMO_I2S_TX_MODE;

    I2S_TxInit(DEMO_I2S_TX_toAmp, &s_TxConfig);

    DMA_Init(DEMO_DMA);

    DMA_EnableChannel(DEMO_DMA, DEMO_I2S_TX_CHANNEL_toAmp);
    DMA_SetChannelPriority(DEMO_DMA, DEMO_I2S_TX_CHANNEL_toAmp, kDMA_ChannelPriority3);
    DMA_CreateHandle(&s_DmaTxHandle, DEMO_DMA, DEMO_I2S_TX_CHANNEL_toAmp);
}
