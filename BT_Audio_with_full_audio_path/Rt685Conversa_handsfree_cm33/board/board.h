/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef _BOARD_H_
#define _BOARD_H_

#include "clock_config.h"
#include "fsl_common.h"
#include "fsl_reset.h"
#include "fsl_gpio.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/*! @brief The board name */
#define BOARD_NAME      "MIMXRT685-EVK"
#define BOARD_I3C_CODEC (1)

/*! @brief The UART to use for debug messages. */
#if UsingQAR87Board == 1

#define BOARD_DEBUG_UART_TYPE     kSerialPort_Uart
	#if Using_UART5ToPrint
#define BOARD_DEBUG_UART_BASEADDR (uint32_t) USART5
#define BOARD_DEBUG_UART_INSTANCE 5U
#define BOARD_DEBUG_UART_CLK_FREQ CLOCK_GetFlexCommClkFreq(5U)
#define BOARD_DEBUG_UART_FRG_CLK \
    (&(const clock_frg_clk_config_t){5, kCLOCK_FrgPllDiv, 255, 0}) /*!< Select FRG0 mux as frg_pll */
#define BOARD_DEBUG_UART_CLK_ATTACH kFRG_to_FLEXCOMM5
#define BOARD_DEBUG_UART_RST        kFC5_RST_SHIFT_RSTn
#define BOARD_DEBUG_UART_CLKSRC     kCLOCK_Flexcomm5
#define BOARD_UART_IRQ_HANDLER      FLEXCOMM5_IRQHandler
#define BOARD_UART_IRQ              FLEXCOMM5_IRQn
	#endif
	#if Using_UART2ToPrint
#define BOARD_DEBUG_UART_BASEADDR (uint32_t) USART2
#define BOARD_DEBUG_UART_INSTANCE 2U
#define BOARD_DEBUG_UART_CLK_FREQ CLOCK_GetFlexCommClkFreq(2U)
#define BOARD_DEBUG_UART_FRG_CLK \
    (&(const clock_frg_clk_config_t){2, kCLOCK_FrgPllDiv, 255, 0}) /*!< Select FRG0 mux as frg_pll */
#define BOARD_DEBUG_UART_CLK_ATTACH kFRG_to_FLEXCOMM2
#define BOARD_DEBUG_UART_RST        kFC2_RST_SHIFT_RSTn
#define BOARD_DEBUG_UART_CLKSRC     kCLOCK_Flexcomm2
#define BOARD_UART_IRQ_HANDLER      FLEXCOMM2_IRQHandler
#define BOARD_UART_IRQ              FLEXCOMM2_IRQn
#endif
#else

#define BOARD_DEBUG_UART_TYPE     kSerialPort_Uart
#define BOARD_DEBUG_UART_BASEADDR (uint32_t) USART0
#define BOARD_DEBUG_UART_INSTANCE 0U
#define BOARD_DEBUG_UART_CLK_FREQ CLOCK_GetFlexCommClkFreq(0U)
#define BOARD_DEBUG_UART_FRG_CLK \
    (&(const clock_frg_clk_config_t){0, kCLOCK_FrgPllDiv, 255, 0}) /*!< Select FRG0 mux as frg_pll */
#define BOARD_DEBUG_UART_CLK_ATTACH kFRG_to_FLEXCOMM0
#define BOARD_DEBUG_UART_RST        kFC0_RST_SHIFT_RSTn
#define BOARD_DEBUG_UART_CLKSRC     kCLOCK_Flexcomm0
#define BOARD_UART_IRQ_HANDLER      FLEXCOMM0_IRQHandler
#define BOARD_UART_IRQ              FLEXCOMM0_IRQn
#endif
#ifndef BOARD_DEBUG_UART_BAUDRATE
//#define BOARD_DEBUG_UART_BAUDRATE 115200
#define BOARD_DEBUG_UART_BAUDRATE 2000000
#endif /* BOARD_DEBUG_UART_BAUDRATE */

#define BOARD_FLEXSPI_PSRAM FLEXSPI
#ifndef BOARD_ENABLE_PSRAM_CACHE
#define BOARD_ENABLE_PSRAM_CACHE 1
#endif

#if UsingQAR87Board == 1
#define BOARD_CODEC_I2C_BASEADDR   I2C2
#define BOARD_CODEC_I2C_CLOCK_FREQ CLOCK_GetFlexCommClkFreq(2U)
#define BOARD_CODEC_I2C_INSTANCE   2

#define BOARD_PMIC_I3C_BASEADDR   I3C
#define BOARD_PMIC_I3C_CLOCK_FREQ CLOCK_GetI3cClkFreq()

#define BOARD_PMIC_I2C_BASEADDR   I2C4
#define BOARD_PMIC_I2C_CLOCK_FREQ CLOCK_GetFlexCommClkFreq(4U)
#else
#if BOARD_I3C_CODEC && (defined(SDK_I3C_BASED_COMPONENT_USED) && SDK_I3C_BASED_COMPONENT_USED)
#define BOARD_CODEC_I2C_BASEADDR   I3C
#define BOARD_CODEC_I2C_CLOCK_FREQ CLOCK_GetI3cClkFreq()
#define BOARD_CODEC_I2C_INSTANCE   0
#else
#define BOARD_CODEC_I2C_BASEADDR   I2C4
#define BOARD_CODEC_I2C_CLOCK_FREQ CLOCK_GetFlexCommClkFreq(4U)
#define BOARD_CODEC_I2C_INSTANCE   4
#endif


#define BOARD_PMIC_I2C_BASEADDR   I2C15
#define BOARD_PMIC_I2C_CLOCK_FREQ CLOCK_GetFlexCommClkFreq(15U)
#endif //#if UsingQAR87Board == 1

#define BOARD_ACCEL_I2C_BASEADDR   I2C2
#define BOARD_ACCEL_I2C_ADDR       0x1E
#define BOARD_ACCEL_I2C_CLOCK_FREQ CLOCK_GetFlexCommClkFreq(2U)

/* Board led color mapping */
#define LOGIC_LED_ON  1U
#define LOGIC_LED_OFF 0U

#ifndef BOARD_LED_RED_GPIO
#define BOARD_LED_RED_GPIO GPIO
#endif
#define BOARD_LED_RED_GPIO_PORT 0U
#ifndef BOARD_LED_RED_GPIO_PIN
#define BOARD_LED_RED_GPIO_PIN 31U
#endif

#ifndef BOARD_LED_GREEN_GPIO
#define BOARD_LED_GREEN_GPIO GPIO
#endif
#define BOARD_LED_GREEN_GPIO_PORT 0U
#ifndef BOARD_LED_GREEN_GPIO_PIN
#define BOARD_LED_GREEN_GPIO_PIN 14U
#endif
#ifndef BOARD_LED_BLUE_GPIO
#define BOARD_LED_BLUE_GPIO GPIO
#endif
#define BOARD_LED_BLUE_GPIO_PORT 0U
#ifndef BOARD_LED_BLUE_GPIO_PIN
#define BOARD_LED_BLUE_GPIO_PIN 26U
#endif

#ifndef BOARD_FLASH_RESET_GPIO
#define BOARD_FLASH_RESET_GPIO GPIO
#endif
#ifndef BOARD_FLASH_RESET_GPIO_PORT
#define BOARD_FLASH_RESET_GPIO_PORT 2U
#endif
#ifndef BOARD_FLASH_RESET_GPIO_PIN
#define BOARD_FLASH_RESET_GPIO_PIN 12U
#endif

/* Board microphone defines */
#ifndef BOARD_DMIC_NUM
#define BOARD_DMIC_NUM 2
#endif

#define LED_RED_INIT(output)                                                          \
    GPIO_PinInit(BOARD_LED_RED_GPIO, BOARD_LED_RED_GPIO_PORT, BOARD_LED_RED_GPIO_PIN, \
                 &(gpio_pin_config_t){kGPIO_DigitalOutput, (output)}) /*!< Enable target LED_RED */
#define LED_RED_ON()                                          \
    GPIO_PortSet(BOARD_LED_RED_GPIO, BOARD_LED_RED_GPIO_PORT, \
                 1U << BOARD_LED_RED_GPIO_PIN) /*!< Turn on target LED_RED */
#define LED_RED_OFF()                                           \
    GPIO_PortClear(BOARD_LED_RED_GPIO, BOARD_LED_RED_GPIO_PORT, \
                   1U << BOARD_LED_RED_GPIO_PIN) /*!< Turn off target LED_RED */
#define LED_RED_TOGGLE()                                         \
    GPIO_PortToggle(BOARD_LED_RED_GPIO, BOARD_LED_RED_GPIO_PORT, \
                    1U << BOARD_LED_RED_GPIO_PIN) /*!< Toggle on target LED_RED */

#define LED_GREEN_INIT(output)                                                              \
    GPIO_PinInit(BOARD_LED_GREEN_GPIO, BOARD_LED_GREEN_GPIO_PORT, BOARD_LED_GREEN_GPIO_PIN, \
                 &(gpio_pin_config_t){kGPIO_DigitalOutput, (output)}) /*!< Enable target LED_GREEN */
#define LED_GREEN_ON()                                            \
    GPIO_PortSet(BOARD_LED_GREEN_GPIO, BOARD_LED_GREEN_GPIO_PORT, \
                 1U << BOARD_LED_GREEN_GPIO_PIN) /*!< Turn on target LED_GREEN */
#define LED_GREEN_OFF()                                             \
    GPIO_PortClear(BOARD_LED_GREEN_GPIO, BOARD_LED_GREEN_GPIO_PORT, \
                   1U << BOARD_LED_GREEN_GPIO_PIN) /*!< Turn off target LED_GREEN */
#define LED_GREEN_TOGGLE()                                           \
    GPIO_PortToggle(BOARD_LED_GREEN_GPIO, BOARD_LED_GREEN_GPIO_PORT, \
                    1U << BOARD_LED_GREEN_GPIO_PIN) /*!< Toggle on target LED_GREEN */

#define LED_BLUE_INIT(output)                                                            \
    GPIO_PinInit(BOARD_LED_BLUE_GPIO, BOARD_LED_BLUE_GPIO_PORT, BOARD_LED_BLUE_GPIO_PIN, \
                 &(gpio_pin_config_t){kGPIO_DigitalOutput, (output)}) /*!< Enable target LED_BLUE */
#define LED_BLUE_ON()                                           \
    GPIO_PortSet(BOARD_LED_BLUE_GPIO, BOARD_LED_BLUE_GPIO_PORT, \
                 1U << BOARD_LED_BLUE_GPIO_PIN) /*!< Turn on target LED_BLUE */
#define LED_BLUE_OFF()                                            \
    GPIO_PortClear(BOARD_LED_BLUE_GPIO, BOARD_LED_BLUE_GPIO_PORT, \
                   1U << BOARD_LED_BLUE_GPIO_PIN) /*!< Turn off target LED_BLUE */
#define LED_BLUE_TOGGLE()                                          \
    GPIO_PortToggle(BOARD_LED_BLUE_GPIO, BOARD_LED_BLUE_GPIO_PORT, \
                    1U << BOARD_LED_BLUE_GPIO_PIN) /*!< Toggle on target LED_BLUE */

/* Board SW PIN */
#ifndef BOARD_SW1_GPIO
#define BOARD_SW1_GPIO GPIO
#endif
#define BOARD_SW1_GPIO_PORT 1U
#ifndef BOARD_SW1_GPIO_PIN
#define BOARD_SW1_GPIO_PIN 1U
#endif

#ifndef BOARD_SW2_GPIO
#define BOARD_SW2_GPIO GPIO
#endif
#define BOARD_SW2_GPIO_PORT 0U
#ifndef BOARD_SW2_GPIO_PIN
#define BOARD_SW2_GPIO_PIN 10U
#endif

/* USDHC configuration */
#define BOARD_SD_SUPPORT_180V          (1)
#define BOARD_USDHC_CD_GPIO_BASE       GPIO
#define BOARD_USDHC_CD_GPIO_PORT       (2)
#define BOARD_USDHC_CD_GPIO_PIN        (9)
#define BOARD_SD_POWER_RESET_GPIO      (GPIO)
#define BOARD_SD_POWER_RESET_GPIO_PORT (2)
#define BOARD_SD_POWER_RESET_GPIO_PIN  (10)

/* Card detect handled by uSDHC, no GPIO interrupt */
#define BOARD_SD_DETECT_TYPE              kSDMMCHOST_DetectCardByHostCD
#define BOARD_USDHC_CD_PORT_IRQ           USDHC0_IRQn
#define BOARD_USDHC_CD_STATUS()           0
#define BOARD_USDHC_CD_INTERRUPT_STATUS() 0
#define BOARD_USDHC_CD_CLEAR_INTERRUPT(flag)
#define BOARD_USDHC_CD_GPIO_INIT()

#define BOARD_HAS_SDCARD                 (1U)
#define BOARD_USDHC_CARD_INSERT_CD_LEVEL (0U)

#define BOARD_USDHC_MMCCARD_POWER_CONTROL_INIT()
#define BOARD_USDHC_MMCCARD_POWER_CONTROL(state)
#define BOARD_USDHC_SDCARD_POWER_CONTROL_INIT()                                                                \
    {                                                                                                          \
        GPIO_PortInit(BOARD_SD_POWER_RESET_GPIO, BOARD_SD_POWER_RESET_GPIO_PORT);                              \
        GPIO_PinInit(BOARD_SD_POWER_RESET_GPIO, BOARD_SD_POWER_RESET_GPIO_PORT, BOARD_SD_POWER_RESET_GPIO_PIN, \
                     &(gpio_pin_config_t){kGPIO_DigitalOutput, 0});                                            \
    }

#define BOARD_MMC_SUPPORT_8BIT_BUS 0

#define BOARD_USDHC_SDCARD_POWER_CONTROL(state)                                                                        \
    (state ?                                                                                                           \
         GPIO_PortSet(BOARD_SD_POWER_RESET_GPIO, BOARD_SD_POWER_RESET_GPIO_PORT, 1 << BOARD_SD_POWER_RESET_GPIO_PIN) : \
         GPIO_PortClear(BOARD_SD_POWER_RESET_GPIO, BOARD_SD_POWER_RESET_GPIO_PORT,                                     \
                        1 << BOARD_SD_POWER_RESET_GPIO_PIN))

#define BOARD_USDHC0_BASEADDR USDHC0

#define BOARD_USDHC0_CLK_FREQ CLOCK_GetSdioClkFreq(0)

#define BOARD_USDHC_SWITCH_VOLTAGE_FUNCTION 1U

#define BOARD_SD_HOST_BASEADDR BOARD_USDHC0_BASEADDR
#define BOARD_SD_HOST_CLK_FREQ BOARD_USDHC0_CLK_FREQ
#define BOARD_SD_HOST_IRQ      USDHC0_IRQn

#define BOARD_SD_Pin_Config(speed, strength)

/* USB PHY condfiguration */
#define BOARD_USB_PHY_D_CAL     (0x0CU)
#define BOARD_USB_PHY_TXCAL45DP (0x06U)
#define BOARD_USB_PHY_TXCAL45DM (0x06U)

#define BOARD_FLASH_SIZE (0x4000000U)

/* Display. */
#define BOARD_LCD_DC_GPIO      GPIO
#define BOARD_LCD_DC_GPIO_PORT 1U
#define BOARD_LCD_DC_GPIO_PIN  7U

#if UsingQAR87Board == 1
#define BOARD_BT_UART_BASEADDR USART0
#define BOARD_BT_UART_INSTANCE 0
#define BOARD_BT_UART_BAUDRATE 3000000
#define BOARD_BT_UART_CLK_FREQ CLOCK_GetFlexCommClkFreq(0U)
#define BOARD_BT_UART_FRG_CLK \
    (&(const clock_frg_clk_config_t){0, kCLOCK_FrgPllDiv, 255, 4}) /*!< Select FRG0 mux as frg_pll */
#define BOARD_BT_UART_CLK_ATTACH  kFRG_to_FLEXCOMM0
#define BOARD_BT_UART_RST         kFC0_RST_SHIFT_RSTn
#define BOARD_BT_UART_IRQ         FLEXCOMM0_IRQn
#define BOARD_BT_UART_IRQ_HANDLER FLEXCOMM0_IRQHandler
#define BOARD_BT_UART_CLKSRC      kCLOCK_Flexcomm0
#else
#define BOARD_BT_UART_BASEADDR USART4
#define BOARD_BT_UART_INSTANCE 4
#define BOARD_BT_UART_BAUDRATE 3000000
#define BOARD_BT_UART_CLK_FREQ CLOCK_GetFlexCommClkFreq(4U)
#define BOARD_BT_UART_FRG_CLK \
    (&(const clock_frg_clk_config_t){4, kCLOCK_FrgPllDiv, 255, 4}) /*!< Select FRG0 mux as frg_pll */
#define BOARD_BT_UART_CLK_ATTACH  kFRG_to_FLEXCOMM4
#define BOARD_BT_UART_RST         kFC4_RST_SHIFT_RSTn
#define BOARD_BT_UART_IRQ         FLEXCOMM4_IRQn
#define BOARD_BT_UART_IRQ_HANDLER FLEXCOMM4_IRQHandler
#define BOARD_BT_UART_CLKSRC      kCLOCK_Flexcomm4
#endif

/* ERPC SPI configuration */
#define ERPC_BOARD_SPI_SLAVE_READY_USE_GPIO (1)
#define ERPC_BOARD_SPI_BASEADDR             SPI14_BASE
#define ERPC_BOARD_SPI_BAUDRATE             500000U
#define ERPC_BOARD_SPI_CLKSRC               kCLOCK_Flexcomm14
#define ERPC_BOARD_SPI_CLK_FREQ             CLOCK_GetFlexCommClkFreq(14)
#define ERPC_BOARD_SPI_INT_GPIO             GPIO
#define ERPC_BOARD_SPI_INT_PORT             1U
#define ERPC_BOARD_SPI_INT_PIN              16U
#define ERPC_BOARD_SPI_INT_PIN_IRQ          PIN_INT1_IRQn
#define ERPC_BOARD_SPI_INT_PIN_IRQ_HANDLER  PIN_INT1_IRQHandler
#define ERPC_BOARD_SPI_INT_GPIO_LPC         (1)

/* ERPC I2C configuration */
#define ERPC_BOARD_I2C_BASEADDR            I2C2_BASE
#define ERPC_BOARD_I2C_BAUDRATE            100000U
#define ERPC_BOARD_I2C_CLKSRC              kCLOCK_Flexcomm2
#define ERPC_BOARD_I2C_CLK_FREQ            CLOCK_GetFlexCommClkFreq(2)
#define ERPC_BOARD_I2C_INT_GPIO            GPIO
#define ERPC_BOARD_I2C_INT_PORT            1U
#define ERPC_BOARD_I2C_INT_PIN             16U
#define ERPC_BOARD_I2C_INT_PIN_IRQ         PIN_INT1_IRQn
#define ERPC_BOARD_I2C_INT_PIN_IRQ_HANDLER PIN_INT1_IRQHandler


/* QAR87 Board */
#if UsingQAR87Board == 1

/* GPIO Port */
#define GPIO0_PORT                    0U
#define GPIO1_PORT                    1U
#define GPIO2_PORT                    2U

/* AMP */
#define GPIO_AMP_RESET_R_PORT         0U      //AMP_RESET_PORT
#define GPIO_AMP_RESET_R_PIN          0U      //AMP_RESET_PIN

/* Touch */
#define NXP_TOUCH_INT_PORT            1U      //TOUCH_INT_PORT
#define NXP_TOUCH_INT_PIN             8U      //TOUCH_INT_PIN

/* Charger */
#if UsingQAR87BoardHwVersion1 == 1 // Actual Board
#define CHG_INT_N_R_PORT              0U      //CHARG_INT_PORT
#define CHG_INT_N_R_PIN               21U     //CHARG_INT_PIN
#endif
#if UsingQAR87BoardHwVersion1 == 0 // Dev Board
#define CHG_INT_N_R_PORT              1U      //CHARG_INT_PORT
#define CHG_INT_N_R_PIN               15U     //CHARG_INT_PIN
#endif

/* Gauge */
#define FG_INT_GLF70302_PORT          2U      //GAUGE_INT_PORT
#define FG_INT_GLF70302_PIN           18U     //GAUGE_INT_PIN

/* SoC */
#define NXP_532_PWR_PMIC1_PORT        0U      //PWR_SW1_PORT / NT98532 POWER ON
#define NXP_532_PWR_PMIC1_PIN         6U      //PWR_SW1_PIN
#define AP533_RST_N_PORT              0U      //RESET553_N_PORT
#define AP533_RST_N_PIN               27U     //RESET553_N_PIN
#define HS_SPI_2_RDY_PORT             1U
#define HS_SPI_2_RDY_PIN              10U
#define AP533_WAKEUP_N_PORT           2U
#define AP533_WAKEUP_N_PIN            15U

/* Buttons */
#define BUTTON_PINT_BASE              PINT
#define POWERKEY_PINT_PIN_INT0_SRC    kINPUTMUX_GpioPort0Pin5ToPintsel  /* Power Key */
#define FUNKEY_PINT_PIN_INT1_SRC      kINPUTMUX_GpioPort0Pin26ToPintsel   /* Fun Key */
#define NXP_BQ_MR_N_PORT              0U      //POWER_KEY_PORT
#define NXP_BQ_MR_N_PIN               5U      //POWER_KEY_PIN
#define FUN_KEY_PORT                  0U      //FUN_KEY1_N_PORT
#define FUN_KEY_PIN                   26U     //FUN_KEY1_N_PIN

/* Proximity */
#if UsingQAR87BoardHwVersion1 == 1 // Actual Board
#define PROX1_INT_N_PORT              2U
#define PROX1_INT_N_PIN               14U
#endif

/* USB Switch */
#if UsingQAR87BoardHwVersion1 == 1 // Actual Board
#define NXP_532_USB_SWITCH_PORT       0U
#define NXP_532_USB_SWITCH_PIN        10U
#define USB_SWDIO_SWITCH_PORT         2U
#define USB_SWDIO_SWITCH_PIN          19U

#endif
#define VBUS20_PORT                   0U
#define VBUS20_PIN                    19U
#define OVP_FAULT_N_PORT              0U
#define OVP_FAULT_N_PIN               20U

/* BT / Wi-Fi */
#if UsingQAR87BoardHwVersion1 == 1 // Actual Board
#define BT_RST_PORT                   0U
#define BT_RST_PIN                    11U
#define NXP_PDN_RF_PORT               3U
#define NXP_PDN_RF_PIN                26U
#endif
#if UsingQAR87BoardHwVersion1 == 0 // Dev Board
#define PWR_EN_IW611_PORT             3U
#define PWR_EN_IW611_PIN              26U
#endif
#define BT_WAKE_OUT_PORT              0U
#define BT_WAKE_OUT_PIN               25U
#define BT_WAKE_IN_PORT               1U
#define BT_WAKE_IN_PIN                2U

/* PIMC */
#define PCA9422_AMUX_PORT             0U
#define PCA9422_AMUX_PIN              13U
#define PCA9422_685_STBY_MODE1_PORT   0U
#define PCA9422_685_STBY_MODE1_PIN    24U
#define PCA9422_685_DVS_CTR1_PORT     0U
#define PCA9422_685_DVS_CTR1_PIN      29U
#define PCA9422_685_DVS_CTR0_PORT     1U
#define PCA9422_685_DVS_CTR0_PIN      9U
#define PCA9422_685_SLEEP_MODE0_PORT  2U
#define PCA9422_685_SLEEP_MODE0_PIN   27U
#define PCA9422_INTB_PORT             2U
#define PCA9422_INTB_PIN              28U
#define PCA9422_685_DVS_CTR2_PORT     2U
#define PCA9422_685_DVS_CTR2_PIN      31U

/* HW config */
#if UsingQAR87BoardHwVersion1 == 0 // Dev Board
#define HW_CONFIG_01_PORT             0U
#define HW_CONFIG_01_PIN              10U
#define HW_CONFIG_02_PORT             0U
#define HW_CONFIG_02_PIN              11U
#define HW_CONFIG_03_PORT             0U
#define HW_CONFIG_03_PIN              12U
#endif

#define POWER_KEY_PINT_CH  		      0      // PINT channel
#define FUN_KEY_PINT_CH  		      1      // PINT channel

#define SOC_SPI_SLAVE                 SPI14
#define SOC_SPI_SLAVE_IRQ             FLEXCOMM14_IRQn
#define SPI_SLAVE_IRQHandler          FLEXCOMM14_IRQHandler

#define SOC_SPI_SSEL                  0
#define SOC_SPI_SPOL                  kSPI_SpolActiveAllLow


#define PMIC_GLF70583_ENABLE          1
#define PMIC_PCA9422_ENABLE           1
#define TOUCH_AW93305_ENABLE          1
#define CHG_BQ25618_ENABLE            1
#define FG_GLF70302_ENABLE            0
#define LED_KTD2027_ENABLE            1
#define AMP_AW88166_ENABLE            1
#define SOC_SPI_ENABLE                1

#endif

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/*******************************************************************************
 * API
 ******************************************************************************/

void BOARD_InitDebugConsole(void);
status_t BOARD_InitPsRam(void);
void BOARD_FlexspiClockSafeConfig(void);
AT_QUICKACCESS_SECTION_CODE(void BOARD_SetFlexspiClock(uint32_t src, uint32_t divider));
AT_QUICKACCESS_SECTION_CODE(void BOARD_DeinitXip(FLEXSPI_Type *base));
AT_QUICKACCESS_SECTION_CODE(void BOARD_InitXip(FLEXSPI_Type *base));
AT_QUICKACCESS_SECTION_CODE(void BOARD_SetDeepSleepPinConfig(void));
AT_QUICKACCESS_SECTION_CODE(void BOARD_RestoreDeepSleepPinConfig(void));
AT_QUICKACCESS_SECTION_CODE(void BOARD_EnterDeepSleep(const uint32_t exclude_from_pd[4]));
AT_QUICKACCESS_SECTION_CODE(void BOARD_EnterDeepPowerDown(const uint32_t exclude_from_pd[4]));
#if defined(SDK_I2C_BASED_COMPONENT_USED) && SDK_I2C_BASED_COMPONENT_USED
void BOARD_I2C_Init(I2C_Type *base, uint32_t clkSrc_Hz);
status_t BOARD_I2C_Send(I2C_Type *base,
                        uint8_t deviceAddress,
                        uint32_t subAddress,
                        uint8_t subaddressSize,
                        uint8_t *txBuff,
                        uint8_t txBuffSize);
status_t BOARD_I2C_Receive(I2C_Type *base,
                           uint8_t deviceAddress,
                           uint32_t subAddress,
                           uint8_t subaddressSize,
                           uint8_t *rxBuff,
                           uint8_t rxBuffSize);
#endif
#if defined(SDK_I3C_BASED_COMPONENT_USED) && SDK_I3C_BASED_COMPONENT_USED
void BOARD_I3C_Init(I3C_Type *base, uint32_t clkSrc_Hz);
status_t BOARD_I3C_Send(I3C_Type *base,
                        uint8_t deviceAddress,
                        uint32_t subAddress,
                        uint8_t subaddressSize,
                        uint8_t *txBuff,
                        uint8_t txBuffSize);
status_t BOARD_I3C_Receive(I3C_Type *base,
                           uint8_t deviceAddress,
                           uint32_t subAddress,
                           uint8_t subaddressSize,
                           uint8_t *rxBuff,
                           uint8_t rxBuffSize);
#endif
#if defined BOARD_USE_CODEC
void BOARD_Codec_I2C_Init(void);
status_t BOARD_Codec_I2C_Send(
    uint8_t deviceAddress, uint32_t subAddress, uint8_t subAddressSize, const uint8_t *txBuff, uint8_t txBuffSize);
status_t BOARD_Codec_I2C_Receive(
    uint8_t deviceAddress, uint32_t subAddress, uint8_t subAddressSize, uint8_t *rxBuff, uint8_t rxBuffSize);
#endif
#if defined(SDK_I2C_BASED_COMPONENT_USED) && SDK_I2C_BASED_COMPONENT_USED
void BOARD_PMIC_I2C_Init(void);
status_t BOARD_PMIC_I2C_Send(
    uint8_t deviceAddress, uint32_t subAddress, uint8_t subAddressSize, const uint8_t *txBuff, uint8_t txBuffSize);
status_t BOARD_PMIC_I2C_Receive(
    uint8_t deviceAddress, uint32_t subAddress, uint8_t subAddressSize, uint8_t *rxBuff, uint8_t rxBuffSize);

void BOARD_Accel_I2C_Init(void);
status_t BOARD_Accel_I2C_Send(uint8_t deviceAddress, uint32_t subAddress, uint8_t subaddressSize, uint32_t txBuff);
status_t BOARD_Accel_I2C_Receive(
    uint8_t deviceAddress, uint32_t subAddress, uint8_t subaddressSize, uint8_t *rxBuff, uint8_t rxBuffSize);

#endif /* SDK_I2C_BASED_COMPONENT_USED */
#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* _BOARD_H_ */
