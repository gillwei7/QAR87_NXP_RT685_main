/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* FreeRTOS kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "event_groups.h"
#include "semphr.h"
#include "fsl_pint.h"

/* Freescale includes. */
#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "board.h"
#include "app.h"

#include "spi_handler.h"
#include "button_handler.h"
#include "i2_component_handler.h"
#include "i2s_handler.h"

#include <string.h>
#include <stdlib.h>

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define USE_EVENT 1
#define USE_SEMAPHORE 0

/*****************************************************************************************************/

/* ===== external I2C handlers ===== */
extern EventGroupHandle_t i2c_event_group ;
extern SemaphoreHandle_t  i2c_mutex       ;
extern TaskHandle_t       sI2CTaskHandle  ;

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/* ===== external SPI handlers ===== */
extern QueueHandle_t spi_request_queue ;
extern EventGroupHandle_t spi_event_group ;
extern SemaphoreHandle_t spi_semaphore ;

/*******************************************************************************
 * Code
 ******************************************************************************/

void GPIO_INTA_DriverIRQHandler(void)
{

	BaseType_t xHPW = pdFALSE;

	uint32_t status_1 = GPIO_PortGetInterruptStatus(GPIO, GPIO1_PORT, kGPIO_InterruptA);
	uint32_t status_2 = GPIO_PortGetInterruptStatus(GPIO, GPIO2_PORT, kGPIO_InterruptA);

    if (status_1 & (1 << TOUCH_INT_PIN)) { //Touch
        GPIO_PinDisableInterrupt(GPIO, TOUCH_INT_PORT, TOUCH_INT_PIN, kGPIO_InterruptA);
        GPIO_PinClearInterruptFlag(GPIO, TOUCH_INT_PORT, TOUCH_INT_PIN, kGPIO_InterruptA);

        if (i2c_event_group)
        {
            xEventGroupSetBitsFromISR(i2c_event_group, TOUCH_EVENT_BIT, &xHPW);
        }

        //PRINTF("[Debug] TOUCH_GPIO_INTA_IRQHandler \r\n");
    }
    if (status_1 & (1 << CHARG_INT_PIN)) { //Charger
    	GPIO_PinDisableInterrupt(GPIO, CHARG_INT_PORT, CHARG_INT_PIN, kGPIO_InterruptA);
        GPIO_PinClearInterruptFlag(GPIO, CHARG_INT_PORT, CHARG_INT_PIN, kGPIO_InterruptA);

        if (i2c_event_group)
        {
            xEventGroupSetBitsFromISR(i2c_event_group, CHARGER_EVENT_BIT, &xHPW);
        }

        //PRINTF("[Debug] CHARG_INT_INTA_IRQHandler \r\n");
    }
    if (status_2 & (1 << GAUGE_INT_PIN)) { //Gauge

    	GPIO_PinDisableInterrupt(GPIO, GAUGE_INT_PORT, GAUGE_INT_PIN, kGPIO_InterruptA);
        GPIO_PinClearInterruptFlag(GPIO, GAUGE_INT_PORT, GAUGE_INT_PIN, kGPIO_InterruptA);

        if (i2c_event_group)
        {
            xEventGroupSetBitsFromISR(i2c_event_group, GAUGE_EVENT_BIT, &xHPW);
        }

        //PRINTF("[Debug] GAUGE_INT_PIN_INTA_IRQHandler \r\n");
    }

    portYIELD_FROM_ISR(xHPW);
    SDK_ISR_EXIT_BARRIER;
}

/**
 * @brief 主程式進入點。
 * @details 負責初始化硬體、時鐘、GPIO、I2C 和 SPI 等周邊設備。
 * 同時，它會建立 FreeRTOS 所需的事件組、佇列和各個任務 (如按鈕處理、SPI 處理等)，
 * 最後啟動 FreeRTOS 排程器，將控制權交給作業系統。
 */
int main(void)
{
	BOARD_InitHardware();

	BOARD_I3C_Init(BOARD_PMIC_I3C_BASEADDR, BOARD_PMIC_I3C_CLOCK_FREQ);

#if USE_EVENT
    spi_event_group = xEventGroupCreate();
    if (spi_event_group == NULL)
    {
        PRINTF("Failed to create event group\r\n");
        while (1);
    }
#endif

#if USE_SEMAPHORE
    spi_semaphore = xSemaphoreCreateBinary();
    if (spi_semaphore == NULL)
    {
        PRINTF("Failed to create semaphore\r\n");
        while (1);
    }
#endif

    /* <<< NEW: 建立訊息佇列 >>> */
    // 佇列長度為 10，每個訊息的大小是一個 uint8_t
    spi_request_queue = xQueueCreate(10, sizeof(uint8_t));
    if (spi_request_queue == NULL)
    {
        PRINTF("Failed to create spi_request_queue\r\n");
        while (1);
    }
#if SCAN_I2C_ADDRESS_ENABLE
    Scan_I2C_Devices(BOARD_PMIC_I3C_BASEADDR);
#endif

	/* Init GPIO */
    GPIO_PortInit(GPIO, GPIO0_PORT);
    GPIO_PortInit(GPIO, GPIO2_PORT);
    gpio_pin_config_t output_int_config = {kGPIO_DigitalOutput, 0,};
    GPIO_PinInit(GPIO, 2, 15, &output_int_config);
    GPIO_PinWrite(GPIO, 2, 15, 0);  // 預設為低位

	GPIO_PinInit(GPIO, PWR_SW1_PORT, PWR_SW1_PIN, &output_int_config);
    GPIO_PinWrite(GPIO, PWR_SW1_PORT, PWR_SW1_PIN, 0);
    GPIO_PinInit(GPIO, RESET553_N_PORT, RESET553_N_PIN, &output_int_config);

    /* Define the init structure for the input switch pin */
    gpio_pin_config_t sw_config    = {kGPIO_DigitalInput, 0};
    gpio_interrupt_config_t config = {kGPIO_PinIntEnableEdge, kGPIO_PinIntEnableLowOrFall};
    NVIC_SetPriority(GPIO_INTA_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
    EnableIRQ(GPIO_INTA_IRQn);

    /* AMP GPIO */
    gpio_pin_config_t amp_config = { kGPIO_DigitalOutput, 1, };
	GPIO_PinInit(GPIO, AMP_RESET_PORT, AMP_RESET_PIN, &amp_config);
    /* Touch INT GPIO*/
    GPIO_PinInit(GPIO, TOUCH_INT_PORT, TOUCH_INT_PIN, &sw_config);
    GPIO_SetPinInterruptConfig(GPIO, TOUCH_INT_PORT, TOUCH_INT_PIN, &config);
    GPIO_PinEnableInterrupt(GPIO, TOUCH_INT_PORT, TOUCH_INT_PIN, kGPIO_InterruptA);
    /* Charger INT GPIO */
    GPIO_PinInit(GPIO, CHARG_INT_PORT, CHARG_INT_PIN, &sw_config);
    GPIO_SetPinInterruptConfig(GPIO, CHARG_INT_PORT, CHARG_INT_PIN, &config);
    GPIO_PinEnableInterrupt(GPIO, CHARG_INT_PORT, CHARG_INT_PIN, kGPIO_InterruptA);
    /* Gauge INT GPIO */
    GPIO_PinInit(GPIO, GAUGE_INT_PORT, GAUGE_INT_PIN, &sw_config);
    GPIO_SetPinInterruptConfig(GPIO, GAUGE_INT_PORT, GAUGE_INT_PIN, &config);
    GPIO_PinEnableInterrupt(GPIO, GAUGE_INT_PORT, GAUGE_INT_PIN, kGPIO_InterruptA);

    /* Initialize PINT */ /* Init FUN_KEY1 & Power_Key*/
	PINT_Init(EXAMPLE_PINT_BASE);
	NVIC_SetPriority(PIN_INT0_IRQn + FUN_KEY1_PINT_CH, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
	NVIC_SetPriority(PIN_INT0_IRQn + POWER_KEY_PINT_CH, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);

	PINT_PinInterruptConfig(EXAMPLE_PINT_BASE, kPINT_PinInt0, kPINT_PinIntEnableBothEdges, pint_intr_callback);
	PINT_EnableCallbackByIndex(EXAMPLE_PINT_BASE, kPINT_PinInt0);
	PINT_PinInterruptConfig(EXAMPLE_PINT_BASE, kPINT_PinInt1, kPINT_PinIntEnableBothEdges, pint_intr_callback);
	PINT_EnableCallbackByIndex(EXAMPLE_PINT_BASE, kPINT_PinInt1);

	/* init SPI peripheral */
    spi_slave_config_t slave_config = {0};
    SPI_SlaveGetDefaultConfig(&slave_config);
    slave_config.sselPol = (spi_spol_t)EXAMPLE_SPI_SPOL;
    SPI_SlaveInit(EXAMPLE_SPI_SLAVE, &slave_config);

    NVIC_SetPriority(EXAMPLE_SPI_SLAVE_IRQ, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
    EnableIRQ(EXAMPLE_SPI_SLAVE_IRQ);

	/* Init I2C Component */
    Init_I2C_Component();

	/* I2S Init */
	Init_I2S();

	/* ===== A. 建立 I2C EventGroup 與 Mutex ===== */
	i2c_event_group = xEventGroupCreate();
	configASSERT(i2c_event_group);

	i2c_mutex = xSemaphoreCreateMutex();
	configASSERT(i2c_mutex);


	/* 建立 tasks */
    /* <<< MODIFIED: 建立 spi_handler_task 來取代舊的 console_task >>> */
    if (xTaskCreate(spi_handler_task, "SPI_HANDLER", configMINIMAL_STACK_SIZE + 500, NULL,
                    tskIDLE_PRIORITY + 2, NULL) != pdPASS)
    {
        PRINTF("Task creation failed!.\r\n");
        while (1);
    }

//    if (xTaskCreate(spi_task, "SPI_TASK", configMINIMAL_STACK_SIZE + 100, NULL,
//                    tskIDLE_PRIORITY + 1, NULL) != pdPASS)
//    {
//        PRINTF("Task creation failed!.\r\n");
//        while (1);
//    }

    if (xTaskCreate(passive_spi_handler_task, "PASSIVE", configMINIMAL_STACK_SIZE + 200, NULL,
                    tskIDLE_PRIORITY + 3, NULL) != pdPASS)
    {
        PRINTF("Task creation failed!.\r\n");
        while (1);
    }

	if (xTaskCreate(button_task, "BUTTON", configMINIMAL_STACK_SIZE + 100, NULL, tskIDLE_PRIORITY + 2, NULL)!= pdPASS)
    {
        PRINTF(" BUTTON Task creation failed!.\r\n");
        while (1);
    }
	if (xTaskCreate(power_key_task, "POWER_KEY", configMINIMAL_STACK_SIZE + 100, NULL, tskIDLE_PRIORITY + 2, NULL)!= pdPASS)
    {
        PRINTF(" POWER_KEY Task creation failed!.\r\n");
        while (1);
    }
	/* ===== C. 建立 I2C_Task（建議比 passive_handler_task 略高，避免事件延遲） ===== */
	if (xTaskCreate(I2C_Task, "I2C_TASK",
	                configMINIMAL_STACK_SIZE + 256,
	                NULL,
	                tskIDLE_PRIORITY + 3,
	                &sI2CTaskHandle) != pdPASS)
	{
	    PRINTF("I2C_TASK creation failed!\r\n");
	    while (1) { ; }
	}


	vTaskStartScheduler();



    for (;;);
}
