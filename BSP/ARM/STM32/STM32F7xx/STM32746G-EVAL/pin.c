/*
* pin.c                                                     Version 6.0.0
*
* Pin configuration for STMicro STM32F7xx. Add to or modify as desired.
* Note that SMX modules such as smxFS, smxNS, and smxUSB do the pin
* configuration they need in their drivers, so not all pin config is
* centralized here. This makes those drivers self-contained.
*
* Copyright (c) 2009-2026 Micro Digital Inc.
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
* Author: XiaoYi Chen
*
*****************************************************************************/

#include "bbase.h"
#include "bsp.h"

#if SMX_CFG_SSMX
#pragma section_prefix = ".sys"
#endif

#ifdef __cplusplus
extern "C" {
#endif

void PinConfig(void);

#ifdef __cplusplus
}
#endif

void PinConfig(void)
{
#if !SB_USB_FS /* Don't init USART1 pins if using Full Speed USB controller */
   GPIO_InitTypeDef  GPIO_InitStruct;

   RCC_PeriphCLKInitTypeDef RCC_PeriphClkInit;

   /* Enable GPIO TX/RX clock */
   __HAL_RCC_GPIOA_CLK_ENABLE();

   /* Select SysClk as source of USART1 clocks */
   RCC_PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
   RCC_PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_SYSCLK;
   HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphClkInit);

   /* Enable USARTx clock */
   __HAL_RCC_USART1_CLK_ENABLE();

   /* UART TX/RX GPIO pin configuration */
   GPIO_InitStruct.Pin       = GPIO_PIN_9 | GPIO_PIN_10;
   GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
   GPIO_InitStruct.Pull      = GPIO_PULLUP;
   GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;
   GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
   HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);  
#endif
}
