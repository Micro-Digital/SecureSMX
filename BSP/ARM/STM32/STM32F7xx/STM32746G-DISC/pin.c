/*
* pin.c                                                     Version 5.4.0
*
* Pin configuration for STMicro STM32F7xx. Add to or modify as desired.
* Note that SMX modules such as smxFS, smxNS, and smxUSB do the pin
* configuration they need in their drivers, so not all pin config is
* centralized here. This makes those drivers self-contained.
*
* Copyright (c) 2009-2025 Micro Digital Inc.
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
* Author: XiaoYi Chen
*
*****************************************************************************/

#include "bbase.h"
#include "bsp.h"

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
   __HAL_RCC_GPIOB_CLK_ENABLE();

   /* Select SysClk as source of USART1 clocks */
   RCC_PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
   RCC_PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_SYSCLK;
   HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphClkInit);

   /* Enable USARTx clock */
   __HAL_RCC_USART1_CLK_ENABLE();

   /* UART TX/RX GPIO pin configuration */
   GPIO_InitStruct.Pin       = GPIO_PIN_9;
   GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
   GPIO_InitStruct.Pull      = GPIO_PULLUP;
   GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;
   GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
   HAL_GPIO_Init(GPIOA, &GPIO_InitStruct); 

   GPIO_InitStruct.Pin       = GPIO_PIN_7;
   HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);  
#endif
}

