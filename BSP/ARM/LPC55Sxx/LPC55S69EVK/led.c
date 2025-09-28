/*
* led.c                                                     Version 5.4.0
*
* Routines to write to the RGB LED on the NXP LPC55S69-EVK Board.
*
* Copyright (c) 2006-2025 Micro Digital Inc.
* All rights reserved. www.smxrtos.com
*
* This software, documentation, and accompanying materials are made available
* under the Apache License, Version 2.0. You may not use this file except in
* compliance with the License. http://www.apache.org/licenses/LICENSE-2.0
*
* SPDX-License-Identifier: Apache-2.0
*
* This Work is protected by patents listed in smx.h. A patent license is
* granted according to the License above. This entire comment block must be
* preserved in all copies of this file.
*
* Support services are offered by MDI. Inquire at support@smxrtos.com.
*
* Author: Xingsheng Wan
*
*****************************************************************************/

#include "bbase.h"
#include "bsp.h"
#include "led.h"

#if SMX_CFG_SSMX
#pragma section_prefix = ".ucom"
#endif


/* Initializes all LEDs. */

void sb_LEDInit(void)
{
   /* An User controlled RGB LED (D8) is provided on the Board, located on the right
      hand edge. The LEDs in this device are controlled by LPC55Sxx ports P1_4
      (Blue), P1_6 (Red), P1_7 (Green) with the LEDs being illuminated when the
      respective LED is pulled low.
   */

   GPIO_PortInit(GPIO, 1);

   gpio_pin_config_t LED_config = {
      .pinDirection = kGPIO_DigitalOutput,
      .outputLogic = 0U
   };

   /* Enables the clock for the I/O controller.: Enable Clock. */
   CLOCK_EnableClock(kCLOCK_Iocon);

   /* Initialize GPIO functionality on pin PIO1_4, PIO1_6, PIO1_7.  */
   GPIO_PinInit(GPIO, 1, 4, &LED_config);
   GPIO_PinInit(GPIO, 1, 6, &LED_config);
   GPIO_PinInit(GPIO, 1, 7, &LED_config);
    
   const uint32_t LED_mux = (/* Pin is configured as function 0 */
                               IOCON_FUNC0 |
                               /* Selects pull-up function */
                               IOCON_MODE_PULLUP |
                               /* Standard mode, output slew rate control is enabled */
                               IOCON_SLEW_STANDARD |
                               /* Input function is not inverted */
                               0x0 |
                               /* Enables digital function */
                               IOCON_DIGITAL_EN |
                               /* Open drain is disabled */
                               0x0);
   /* PORT1 PIN4 (coords: 1) is configured as PIO1_4, PIO1_6, PIO1_7 */
   IOCON_PinMuxSet(IOCON, 1, 4, LED_mux);
   IOCON_PinMuxSet(IOCON, 1, 6, LED_mux);
   IOCON_PinMuxSet(IOCON, 1, 7, LED_mux);
   sb_LEDWriteRow(0);
}


/* Writes hex number (2 digits). */
void sb_LEDWrite7SegNum(int num)
{
   (void)num;
   /* no 7-segment LED */
}


/* Writes any pattern passed in, to both LEDs (see defines in LED.H). */
void sb_LEDWrite7Seg(u32 val)
{
   (void)val;
   /* no 7-segment LED */
}


/* Light LEDs in row of LEDs. Bits in val indicate which to light. */
void sb_LEDWriteRow(u32 val)
{
   /* P1_4 (Blue), P1_6 (Red), P1_7 (Green) */
   GPIO->DIRCLR[1] = ((~val << 4) & 0x10) | ((~val << 5) & 0xC0);
   GPIO->DIRSET[1] = (((val) << 4) & 0x10) | (((val) << 5) & 0xC0);
}

