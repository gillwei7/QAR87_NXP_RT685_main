/*
 * hal_common.c
 *
 *  Created on: Oct 21, 2025
 *      Author: Lydia
 */
#if UsingQAR87Board == 1
#include "hal_common.h"
#include "fsl_pint.h"
#include "fsl_spi.h"
#include "board.h"
#include "button_handler.h"
#include "i2c_component_handler.h"
#include "fsl_adapter_gpio.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#if SOC_SPI_ENABLE
#define USE_EVENT 1
#define USE_SEMAPHORE 0
#endif

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/* ===== external I2C handlers ===== */
extern EventGroupHandle_t i2c_event_group ;
extern SemaphoreHandle_t  i2c_mutex       ;
extern TaskHandle_t       sI2CTaskHandle  ;

/* ===== external SPI handlers ===== */
extern QueueHandle_t spi_request_queue ;
extern EventGroupHandle_t spi_event_group ;
extern SemaphoreHandle_t spi_semaphore ;

/*******************************************************************************
 * Variables
 ******************************************************************************/

GPIO_HANDLE_DEFINE(s_TouchIntGpioHandle);
GPIO_HANDLE_DEFINE(s_ChargerIntGpioHandle);
GPIO_HANDLE_DEFINE(s_GaugeIntGpioHandle);


/**
 * @description: Delay N ms
 * @paran:
 * @return {*}
 * @author: lmx
 * @param {u32} val：delay time，unit：ms
 */
void hal_delay_ms(uint32_t val)
{
//    if (val < 4294 && val > 0) {
//        SDK_DelayAtLeastUs(val * 1000U, CLOCK_GetFreq(kCLOCK_CoreSysClk));
//    }
//    volatile uint32_t i = 0;
//    volatile uint32_t j = 0;
//    for (j = 0; j < val; j++)
//	for (i = 0; i < 30000; i++)
//	{
//		__NOP();
//	}

	vTaskDelay(pdMS_TO_TICKS(val));
}

static void hal_gpio_port_init(void)
{
    GPIO_PortInit(GPIO, GPIO0_PORT);
    GPIO_PortInit(GPIO, GPIO1_PORT);
    GPIO_PortInit(GPIO, GPIO2_PORT);
}

static void hal_gpio_pin_init(void)
{
    /* Define the init structure for the input switch pin */
    gpio_pin_config_t input_pin_config    = {kGPIO_DigitalInput, 0};

    /* Define the init structure for the output low pin */
    gpio_pin_config_t output_low_pin_config = {kGPIO_DigitalOutput, 0};

    /* Define the init structure for the output low pin */
    gpio_pin_config_t output_high_pin_config = {kGPIO_DigitalOutput, 1};

    /* Init GPIO */
    /* SoC GPIO */
    GPIO_PinInit(GPIO, NXP_532_PWR_PMIC1_PORT, NXP_532_PWR_PMIC1_PIN, &output_low_pin_config);
    GPIO_PinInit(GPIO, AP533_RST_N_PORT, AP533_RST_N_PIN, &output_low_pin_config);
    GPIO_PinInit(GPIO, AP533_WAKEUP_N_PORT, AP533_WAKEUP_N_PIN, &output_low_pin_config);
    /* AMP GPIO */
    GPIO_PinInit(GPIO, GPIO_AMP_RESET_R_PORT, GPIO_AMP_RESET_R_PIN, &output_low_pin_config);
    /* HW Config GPIO */
    GPIO_PinInit(GPIO, HW_CONFIG_03_PORT, HW_CONFIG_03_PIN, &input_pin_config);
#if UsingQAR87BoardHwVersion == 1 // Actual Board
    /* USB Switch GPIO */
    GPIO_PinInit(GPIO, NXP_532_USB_SWITCH_PORT, NXP_532_USB_SWITCH_PIN, &output_low_pin_config);
    GPIO_PinInit(GPIO, USB_SWDIO_SWITCH_PORT, USB_SWDIO_SWITCH_PIN, &output_low_pin_config);
    /* BT Reset GPIO */
    GPIO_PinInit(GPIO, BT_RST_PORT, BT_RST_PIN, &output_high_pin_config);
#endif

    /* Initialize PINT */ /* Init FUN_KEY1 & Power_Key*/
    PINT_Init(BUTTON_PINT_BASE);
    NVIC_SetPriority(PIN_INT0_IRQn + FUN_KEY_PINT_CH, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
    NVIC_SetPriority(PIN_INT0_IRQn + POWER_KEY_PINT_CH, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
    PINT_PinInterruptConfig(BUTTON_PINT_BASE, kPINT_PinInt0, kPINT_PinIntEnableBothEdges, pint_intr_callback);
    PINT_EnableCallbackByIndex(BUTTON_PINT_BASE, kPINT_PinInt0);
    PINT_PinInterruptConfig(BUTTON_PINT_BASE, kPINT_PinInt1, kPINT_PinIntEnableBothEdges, pint_intr_callback);
    PINT_EnableCallbackByIndex(BUTTON_PINT_BASE, kPINT_PinInt1);

}

static void hal_gpio_interrupt_callback(void *param)
{
    BaseType_t xHPW = pdFALSE;

    EventBits_t uxBitsToSet = (EventBits_t)param;

    if (i2c_event_group)
    {
        xEventGroupSetBitsFromISR(i2c_event_group, uxBitsToSet, &xHPW);
    }
    portYIELD_FROM_ISR(xHPW);
    SDK_ISR_EXIT_BARRIER;

}

void hal_gpio_interrupt_init(void)
{
    hal_gpio_pin_config_t touch_int_config = {
        kHAL_GpioDirectionIn,
        0,
        NXP_TOUCH_INT_PORT,
        NXP_TOUCH_INT_PIN,
    };
    hal_gpio_pin_config_t charger_int_config = {
        kHAL_GpioDirectionIn,
        0,
        CHG_INT_N_R_PORT,
        CHG_INT_N_R_PIN,
    };

    hal_gpio_pin_config_t gauge_int_config = {
        kHAL_GpioDirectionIn,
        0,
        FG_INT_GLF70302_PORT,
        FG_INT_GLF70302_PIN,
    };

    /* workaround for calling GPIO_PortInit may reset the configuration already done for the port */
    CLOCK_EnableClock(kCLOCK_HsGpio0);
    RESET_ClearPeripheralReset(kHSGPIO0_RST_SHIFT_RSTn);
    CLOCK_EnableClock(kCLOCK_HsGpio1);
    RESET_ClearPeripheralReset(kHSGPIO1_RST_SHIFT_RSTn);
    CLOCK_EnableClock(kCLOCK_HsGpio2);
    RESET_ClearPeripheralReset(kHSGPIO2_RST_SHIFT_RSTn);

    HAL_GpioInit(s_TouchIntGpioHandle, &touch_int_config);
    HAL_GpioInit(s_ChargerIntGpioHandle, &charger_int_config);
    HAL_GpioInit(s_GaugeIntGpioHandle, &gauge_int_config);

    HAL_GpioSetTriggerMode(s_TouchIntGpioHandle, kHAL_GpioInterruptFallingEdge);
    HAL_GpioSetTriggerMode(s_ChargerIntGpioHandle, kHAL_GpioInterruptFallingEdge);
    HAL_GpioSetTriggerMode(s_GaugeIntGpioHandle, kHAL_GpioInterruptFallingEdge);

    HAL_GpioInstallCallback(s_TouchIntGpioHandle, hal_gpio_interrupt_callback, (void *)TOUCH_EVENT_BIT);
    HAL_GpioInstallCallback(s_ChargerIntGpioHandle, hal_gpio_interrupt_callback, (void *)CHARGER_EVENT_BIT);
    HAL_GpioInstallCallback(s_GaugeIntGpioHandle, hal_gpio_interrupt_callback, (void *)GAUGE_EVENT_BIT);

}

void hal_gpio_init(void)
{
    hal_gpio_port_init();
    hal_gpio_interrupt_init();
    hal_gpio_pin_init();
}

void hal_i3c_init(void)
{
    BOARD_I3C_Init(BOARD_PMIC_I3C_BASEADDR, BOARD_PMIC_I3C_CLOCK_FREQ);
    //hal_scan_i2c_devices(BOARD_PMIC_I3C_BASEADDR);
    i2c_event_group = xEventGroupCreate();
    configASSERT(i2c_event_group);

    i2c_mutex = xSemaphoreCreateMutex();
    configASSERT(i2c_mutex);

}

void hal_scan_i2c_devices(I3C_Type *base)
{
    uint8_t dummyData = 0x00;
    status_t result;

    PRINTF("[I2C]Scanning I2C addresses...\n");

    for (uint8_t addr = 0x08; addr <= 0x77; addr++) // I2C valid 7-bit address range
    {
        result = BOARD_I3C_Send(base, addr, 0x00, 0, &dummyData, 0);

        if (result == kStatus_Success)
        {
            PRINTF("[I2C]Device found at 0x%02X\n", addr);
        }
    }

    PRINTF("[I2C]Scan complete.\n");
}

void hal_spi_init(void)
{
#if SOC_SPI_ENABLE
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

    /* init SPI peripheral */
    spi_slave_config_t spi_slave_config = {0};
    SPI_SlaveGetDefaultConfig(&spi_slave_config);
    spi_slave_config.sselPol = (spi_spol_t)SOC_SPI_SPOL;
    SPI_SlaveInit(SOC_SPI_SLAVE, &spi_slave_config);

    NVIC_SetPriority(SOC_SPI_SLAVE_IRQ, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
    EnableIRQ(SOC_SPI_SLAVE_IRQ);
#endif
}

void hal_soc_enable(void)
{
    GPIO_PinWrite(GPIO, AP533_RST_N_PORT, AP533_RST_N_PIN, 1); //NT98532 Reset Pin
}

void hal_board_init(void)
{
	PRINTF("[System] Version= %s \n", HAL_MCU_APP_VERSION);

	hal_gpio_init();
	hal_i3c_init();
	hal_spi_init();
	Init_I2C_Component();
	hal_scan_i2c_devices(BOARD_PMIC_I3C_BASEADDR);
}

#endif
