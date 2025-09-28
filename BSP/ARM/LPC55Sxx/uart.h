/*
* uart.h                                                    Version 5.4.0
*
* Defines for UART on NXP LPC55Sxx boards.
* Currently supported:  NXP LPC55S69-EVK Board
*
* Copyright (c) 2005-2020 Micro Digital Inc.
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
