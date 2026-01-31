/*
* uart.h                                                    Version 6.0.0
*
* Defines for UART on NXP LPC55Sxx boards.
* Currently supported:  NXP LPC55S69-EVK Board
*
* Copyright (c) 2005-2026 Micro Digital Inc.
* All rights reserved. www.smxrtos.com
*
* SPDX-License-Identifier: GPL-2.0-only OR LicenseRef-MDI-Commercial
*
* This software, documentation, and accompanying materials are made available
* under a dual license, either GPLv2 or Commercial. You may not use this file
* except in compliance with either License. GPLv2 is at www.gnu.org/licenses.
* It does not permit the incorporation of this code into proprietary programs.
*
* Commercial license and support services are available from Micro Digital.
* Inquire at support@smxrtos.com.
*
* This Work embodies patents listed in smx.h. A patent license is hereby
* granted to use these patents in this Work and Derivative Works, except in
* another RTOS or OS.
*
* This entire comment block must be preserved in all copies of this file.
*
* Author: Xingsheng Wan
*
*****************************************************************************/

#ifndef SMX_UART_H
#define SMX_UART_H
#ifdef __cplusplus
extern "C" {
#endif

void Uart0Init(u32 baud);
void Uart0PutChar(unsigned char chr);
unsigned char Uart0GetChar(void);

#ifdef __cplusplus
}
#endif

#endif /* SMX_UART_H */
