/*
* uart.c                                                    Version 5.4.0
*
* Routines to send data through UART on NXP LPC55Sxx boards.
* Currently supported:  NXP LPC55S69-EVK Board
*
* Copyright (c) 2005-2025 Micro Digital Inc.
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
#include "uart.h"

#if SMX_CFG_SSMX
#pragma section_prefix = ".cp"
#endif

/* Initializes UART0. */
void Uart0Init(u32 baud)
{
   usart_config_t config;

   /* Enables the clock for the I/O controller.: Enable Clock. */
   CLOCK_EnableClock(kCLOCK_Iocon);

   const uint32_t port0_pin29_config = (/* Pin is configured as FC0_RXD_SDA_MOSI_DATA */
                                        IOCON_FUNC1 |
                                        /* No addition pin function */
                                        IOCON_MODE_INACT |
                                        /* Standard mode, output slew rate control is enabled */
                                        IOCON_SLEW_STANDARD |
                                        /* Input function is not inverted */
                                        0x0 |
                                        /* Enables digital function */
                                        IOCON_DIGITAL_EN |
                                        /* Open drain is disabled */
                                        0);
   /* PORT0 PIN29 (coords: 92) is configured as FC0_RXD_SDA_MOSI_DATA */
   IOCON_PinMuxSet(IOCON, 0U, 29U, port0_pin29_config);

   const uint32_t port0_pin30_config = (/* Pin is configured as FC0_TXD_SCL_MISO_WS */
                                        IOCON_FUNC1 |
                                        /* No addition pin function */
                                        IOCON_MODE_INACT |
                                        /* Standard mode, output slew rate control is enabled */
                                        IOCON_SLEW_STANDARD |
                                        /* Input function is not inverted */
                                        0x0 |
                                        /* Enables digital function */
                                        IOCON_DIGITAL_EN |
                                        /* Open drain is disabled */
                                        0);
   /* PORT0 PIN30 (coords: 94) is configured as FC0_TXD_SCL_MISO_WS */
   IOCON_PinMuxSet(IOCON, 0U, 30U, port0_pin30_config);
  
   USART_GetDefaultConfig(&config);
   config.baudRate_Bps = baud;
   config.enableTx     = true;
   config.enableRx     = true;

   USART_Init(USART0, &config, CLOCK_GetFlexCommClkFreq(0));
}

/* Send a char */
void Uart0PutChar(unsigned char chr)
{
   USART_WriteBlocking(USART0, &chr, 1);
}

/* Receive a char */
unsigned char Uart0GetChar(void)
{
   USART_Type *pUart;
   pUart = (USART_Type *)USART0;

   /* Waiting until RXD1 Receiver Data Ready. */
   //while (!(pUart->FIFOSTAT & USART_FIFOSTAT_RXNOTEMPTY_MASK)) {}

   /* Return char in  */
   return (pUart->FIFORD & 0xFF);
}

void sb_UartOutData(u8 *psrc, uint len)
{
   for (u32 i = 0; i < len; i++, psrc++)
      Uart0PutChar((unsigned char)*psrc);
}

char sb_UartGetCharPoll(void)
{
#if SB_CFG_UARTI
   char key;
   if (1 == sb_InByte(SB_CON_IN_PORT, (u8 *)&key, NULL, 0))
      return key;
#else
  #if (SB_CON_IN_PORT == 0)
   USART_Type *pUart;
   pUart = (USART_Type *)USART0;
   if (pUart->FIFOSTAT & USART_FIFOSTAT_RXNOTEMPTY_MASK)  /* if char available */
      return Uart0GetChar();
  #endif
#endif
   return 0;
}

