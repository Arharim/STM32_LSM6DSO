/**
 * @file gpio_config.h
 * @brief GPIO pin configuration constants
 * @author STM32_LSM6DSO Project
 *
 * This header contains all GPIO pin assignments and mode constants.
 * Modify these values to adapt to different hardware configurations.
 */

#ifndef HAL_GPIO_CONFIG_H
#define HAL_GPIO_CONFIG_H

#include <stdint.h>

/*============================================================================
 * SPI Pin Configuration
 *===========================================================================*/
#define SPI_CS_PIN   (4U) /**< SPI Chip Select pin (PA4) */
#define SPI_SCK_PIN  (5U) /**< SPI Clock pin (PA5) */
#define SPI_MISO_PIN (6U) /**< SPI Master In Slave Out pin (PA6) */
#define SPI_MOSI_PIN (7U) /**< SPI Master Out Slave In pin (PA7) */

/*============================================================================
 * UART Pin Configuration
 *===========================================================================*/
#define UART_TX_PIN (2U) /**< UART Transmit pin (PA2) */
#define UART_RX_PIN (3U) /**< UART Receive pin (PA3) */

/*============================================================================
 * GPIO Mode Constants (CRL/CRH register values)
 *===========================================================================*/
#define GPIO_MODE_INPUT        (0U) /**< Input mode (reset state) */
#define GPIO_MODE_OUTPUT_10MHZ (1U) /**< Output mode, max speed 10 MHz */
#define GPIO_MODE_OUTPUT_2MHZ  (2U) /**< Output mode, max speed 2 MHz */
#define GPIO_MODE_OUTPUT_50MHZ (3U) /**< Output mode, max speed 50 MHz */

/*============================================================================
 * GPIO Configuration Constants (CRL/CRH register values)
 *===========================================================================*/
#define GPIO_CNF_INPUT_ANALOG (0U << 2) /**< Analog input */
#define GPIO_CNF_INPUT_FLOAT  (1U << 2) /**< Floating input */
#define GPIO_CNF_INPUT_PULL   (2U << 2) /**< Input with pull-up/pull-down */
#define GPIO_CNF_OUTPUT_PP    (0U << 2) /**< General purpose output push-pull */
#define GPIO_CNF_OUTPUT_OD    (1U << 2) /**< General purpose output open-drain */
#define GPIO_CNF_AF_PP        (2U << 2) /**< Alternate function output push-pull */
#define GPIO_CNF_AF_OD        (3U << 2) /**< Alternate function output open-drain */

/*============================================================================
 * PWM Pin Configuration (Servo motors)
 *===========================================================================*/
#define PWM_TIM1_CH1_PIN (8U)  /**< TIM1 CH1 - PA8 */
#define PWM_TIM1_CH2_PIN (9U)  /**< TIM1 CH2 - PA9 */
#define PWM_TIM1_CH3_PIN (10U) /**< TIM1 CH3 - PA10 */
#define PWM_TIM1_CH4_PIN (11U) /**< TIM1 CH4 - PA11 */
#define PWM_TIM4_CH1_PIN (6U)  /**< TIM4 CH1 - PB6 */
#define PWM_TIM4_CH2_PIN (7U)  /**< TIM4 CH2 - PB7 */

/*============================================================================
 * GPIO Port Default Configurations
 *===========================================================================*/
/**
 * @brief Default CRL/CRH configuration for input with pull-down
 *
 * Each nibble: MODE=00 (input), CNF=10 (pull-up/down)
 * With ODR=0, this configures pull-down
 */
#define GPIOB_CRL_PULLDOWN_CFG (0x88888888UL)
#define GPIOB_CRH_PULLDOWN_CFG (0x88888888UL)
#define GPIOC_CRL_PULLDOWN_CFG (0x88888888UL)
#define GPIOC_CRH_PULLDOWN_CFG (0x88888888UL)

#endif
