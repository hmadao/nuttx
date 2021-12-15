/****************************************************************************
 * boards/arm/phy62xx/phy6222/src/phy6222.h
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

#ifndef __BOARDS_ARM_PHY6222_H
#define __BOARDS_ARM_PHY6222_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/compiler.h>
#include <stdint.h>

#include "gpio.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

/* How many SPI modules does this chip support? */

/* STM32F0Discovery GPIOs ***************************************************/

/* The STM32F0Discovery board has four LEDs.
 * Two of these are controlled by logic on the board and
 * are not available for software control:
 *
 * LD1 COM:   LD2 default status is red. LD2 turns to green to indicate
 *            that communications are in  progress between the PC and
 *            the ST-LINK/V2.
 * LD2 PWR:   Red LED indicates that the board is powered.
 *
 * And two LEDs can be controlled by software:
 *
 * User LD3:
 *     Green LED is a user LED connected to the I/O PB7 of the STM32L152 MCU.
 * User LD4:
 *      Blue LED is a user LED connected to the I/O PB6 of the STM32L152 MCU.
 *
 * The other side of the LED connects to ground so high value will illuminate
 * the LED.
 */

#define GPIO_LED1       (GPIO_OUTPUT | GPIO_PUSHPULL | GPIO_SPEED_MEDIUM | \
                         GPIO_OUTPUT_CLEAR | GPIO_PORTC | GPIO_PIN9)
#define GPIO_LED2       (GPIO_OUTPUT | GPIO_PUSHPULL | GPIO_SPEED_MEDIUM | \
                         GPIO_OUTPUT_CLEAR | GPIO_PORTC | GPIO_PIN8)

/* Button definitions *******************************************************/

/* The STM32F0Discovery supports two buttons; only one button is controllable
 * by software:
 *
 *   B1 USER:
 *     user and wake-up button connected to the I/O PA0 of the STM32F303VCT6.
 *   B2 RESET:
 *     pushbutton connected to NRST is used to RESET the STM32F303VCT6.
 *
 * NOTE that  EXTI interrupts are configured
 */

#define MIN_IRQBUTTON   BUTTON_USER
#define MAX_IRQBUTTON   BUTTON_USER
#define NUM_IRQBUTTONS  1

#define GPIO_BTN_USER   (GPIO_INPUT | GPIO_FLOAT | GPIO_EXTI | GPIO_PORTA | GPIO_PIN0)

/****************************************************************************
 * Public Types
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifndef __ASSEMBLY__

/****************************************************************************
 * Public Functions Definitions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32_bringup
 *
 * Description:
 *   Perform architecture-specific initialization
 *
 *   CONFIG_BOARD_LATE_INITIALIZE=y :
 *     Called from board_late_initialize().
 *
 *   CONFIG_BOARD_LATE_INITIALIZE=n && CONFIG_LIB_BOARDCTL=y :
 *     Called from the NSH library
 *
 ****************************************************************************/

int phy62xx_bringup(void);

#endif /* __ASSEMBLY__ */
#endif /* __BOARDS_ARM_PHY6222_H */