/*
* irqtable.c                                                Version 6.0.0
*
* sb_irq_table[] for STM32F7xxxx. Normally in bsp.c, but here in BSP dir since
* all ARM-M share the same bsp.c.
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
* Author: David Moore, Xingsheng Wan, XiaoYi Chen
*
*****************************************************************************/

#include "bbase.h"
#include "bsp.h"

#if SMX_CFG_SSMX
#pragma section_prefix = ".sys"
#endif

/* IRQ Table */
/*
   This is the single place where all IRQ information should be configured.
   Having it all here helps avoid double-assignment of priorities and
   vector numbers. If you need the ability to change these dynamically
   and re-set them in the hardware, call sb_IRQTableEntryWrite() to change
   the entry and then sb_IRQConfig() to write it to the hardware.

   Priority Values: See note at top of bsp.c. This processor uses 4-bit
   priority (bits 7:4) so valid priorities are only:

      0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70,
      0x80, 0x90, 0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xF0

   Interrupt controller init is done in sb_IntCtrlInit() in bsp.c.

   IMPORTANT: Do not use priorities higher (lower value) than
   SB_ARMM_BASEPRI_VALUE (barmm.h) except for special non-smx ISRs
   that do not use smx_ISR_ENTER/EXIT() nor invoke LSRs. These top priorities
   are non-maskable and reserved for special uses such as motor control
   that must not be delayed and that do not interact with the multitasking
   system.
*/
SB_IRQ_REC sb_irq_table[SB_IRQ_NUM] =
{
   /* pri          IRQ  Summary */
   /* ---          ---  ------- */
   {  0xFF  },  /*  0   Window Watchdog */
   {  0xFF  },  /*  1   PVD through EXTI Line detection */
   {  0xFF  },  /*  2   Tamper and TimeStamp interrupts through the EXTI Line */
   {  0xFF  },  /*  3   RTC Wakeup interrupt through the EXTI Line */
   {  0xFF  },  /*  4   Flash global */
   {  0xFF  },  /*  5   RCC global */
   {  0xFF  },  /*  6   EXTI Line0 */
   {  0xFF  },  /*  7   EXTI Line1 */
   {  0xFF  },  /*  8   EXTI Line2 */
   {  0xFF  },  /*  9   EXTI Line3 */
   {  0xFF  },  /* 10   EXTI Line4 */
   {  0xFF  },  /* 11   DMA1 Stream 0 global */
   {  0xFF  },  /* 12   DMA1 Stream 1 global */
   {  0xFF  },  /* 13   DMA1 Stream 2 global */
   {  0xFF  },  /* 14   DMA1 Stream 3 global */
   {  0xFF  },  /* 15   DMA1 Stream 4 global */
   {  0xFF  },  /* 16   DMA1 Stream 5 global */
   {  0xFF  },  /* 17   DMA1 Stream 6 global */
   {  0xFF  },  /* 18   ADC1, ADC2, and ADC3 global */
   {  0xFF  },  /* 19   CAN1 TX */
   {  0xFF  },  /* 20   CAN1 RX0 */
   {  0xFF  },  /* 21   CAN1 RX1 */
   {  0xFF  },  /* 22   CAN1 SCE */
   {  0xFF  },  /* 23   EXTI Line[9:5] */
   {  0xFF  },  /* 24   TIM1 Break and TIM9 global */
   {  0xFF  },  /* 25   TIM1 Update and TIM10 global */
   {  0xFF  },  /* 26   TIM1 Trigger and Commutation and TIM11 global */
   {  0xFF  },  /* 27   TIM1 Capture Compare */
   {  0xFF  },  /* 28   TIM2 global */
   {  0xFF  },  /* 29   TIM3 global */
   {  0xFF  },  /* 30   TIM4 global */
   {  0xFF  },  /* 31   I2C1 Event */
   {  0xFF  },  /* 32   I2C1 Error */
   {  0xFF  },  /* 33   I2C2 Event */
   {  0xFF  },  /* 34   I2C2 Error */
   {  0xFF  },  /* 35   SPI1 global */
   {  0xFF  },  /* 36   SPI2 global */
   {  0xA0  },  /* 37   USART1 global */
   {  0xFF  },  /* 38   USART2 global */
   {  0xFF  },  /* 39   USART3 global */
   {  0xFF  },  /* 40   EXTI Line[15:10] */
   {  0xFF  },  /* 41   RTC Alarm (A and B) through EXTI Line */
   {  0xFF  },  /* 42   USB OTG FS Wakeup through EXTI Line */
   {  0xFF  },  /* 43   TIM8 Break and TIM12 global */
   {  0xFF  },  /* 44   TIM8 Update and TIM13 global */
   {  0xFF  },  /* 45   TIM8 Trigger and Commutation and TIM14 global */
   {  0xFF  },  /* 46   TIM8 Capture Compare */
   {  0xFF  },  /* 47   DMA1 Stream7 */
   {  0xFF  },  /* 48   FSMC global */
   {  0xFF  },  /* 49   SDIO global */
   {  0xFF  },  /* 50   TIM5 global */
   {  0xFF  },  /* 51   SPI3 global */
   {  0xFF  },  /* 52   UART4 global */
   {  0xFF  },  /* 53   UART5 global */
   {  0xFF  },  /* 54   TIM6 global and DAC1 and DAC2 underrun error */
   {  0xFF  },  /* 55   TIM7 global */
   {  0xFF  },  /* 56   DMA2 Stream 0 global */
   {  0xFF  },  /* 57   DMA2 Stream 1 global */
   {  0xFF  },  /* 58   DMA2 Stream 2 global */
   {  0xFF  },  /* 59   DMA2 Stream 3 global */
   {  0xFF  },  /* 60   DMA2 Stream 4 global */
   {  0xFF  },  /* 61   Ethernet global */
   {  0xFF  },  /* 62   Ethernet Wakeup through EXTI Line */
   {  0xFF  },  /* 63   CAN2 TX */
   {  0xFF  },  /* 64   CAN2 RX0 */
   {  0xFF  },  /* 65   CAN2 RX1 */
   {  0xFF  },  /* 66   CAN2 SCE */
   {  0xFF  },  /* 67   USB OTG FS global */
   {  0xFF  },  /* 68   DMA2 Stream 5 global */
   {  0xFF  },  /* 69   DMA2 Stream 6 global */
   {  0xFF  },  /* 70   DMA2 Stream 7 global */
   {  0xFF  },  /* 71   USART6 global */
   {  0xFF  },  /* 72   I2C3 event */
   {  0xFF  },  /* 73   I2C3 error */
   {  0xFF  },  /* 74   USB OTG HS End Point 1 Out global */
   {  0xFF  },  /* 75   USB OTG HS End Point 1 In global */
   {  0xFF  },  /* 76   USB OTG HS Wakeup through EXTI */
   {  0xFF  },  /* 77   USB OTG HS global */
   {  0xFF  },  /* 78   DCMI global */
   {  0xFF  },  /* 79   CRYP crypto global */
   {  0xFF  },  /* 80   Hash and Rng global */
   {  0xFF  },  /* 81   FPU global */
   {  0xFF  },  /* 82   UART 7 global */
   {  0xFF  },  /* 83   UART 8 global */
   {  0xFF  },  /* 84   SPI 4 global */
   {  0xFF  },  /* 85   SPI 5 global */
   {  0xFF  },  /* 86   SPI 6 global */
   {  0xFF  },  /* 87   SAI1 global */
   {  0xFF  },  /* 88   LTDC global */
   {  0xFF  },  /* 89   LTDC global Error */
   {  0xFF  },  /* 90   DMA2D global */
   {  0xFF  },  /* 91   SAI2 global */
   {  0xFF  },  /* 92   QuadSPI global */
   {  0xFF  },  /* 93   LP Timer1 global */
   {  0xFF  },  /* 94   HDMI-CEC global */
   {  0xFF  },  /* 95   I2C4 event */
   {  0xFF  },  /* 96   I2C4Error */
   {  0xFF  },  /* 97   SPDIF global */
#if defined(SB_CPU_STM32F76X) || defined(SB_CPU_STM32F77X)
   {  0xFF  },  /* 98   DSIHOST */
   {  0xFF  },  /* 99   DFSDM1_FLT0 */
   {  0xFF  },  /* 100  DFSDM1_FLT1 */
   {  0xFF  },  /* 101  DFSDM1_FLT2 */
   {  0xFF  },  /* 102  DFSDM1_FLT3 */
   {  0xFF  },  /* 103  SDMMC2 */
   {  0xFF  },  /* 104  CAN3_TX */
   {  0xFF  },  /* 105  CAN3_RX0 */
   {  0xFF  },  /* 106  CAN3_RX1 */
   {  0xFF  },  /* 107  CAN3_SCE */
   {  0xFF  },  /* 108  JPEG */
   {  0xFF  },  /* 109  MDIOS */ 
#endif
};
#define UNUSED_IRQ_REC 0xFF  /* use same value in sb_irq_table[] above */

#if SMX_CFG_SSMX
/* IRQs permitted to be masked/unmasked by utasks */ 
//USER: Adjust as needed for your system. Comment out unneeded IRQs.

#if SB_CFG_UARTI
const IRQ_PERM sb_irq_perm_uart[] = {
   {37, 37},      /* USART1 for terminal output */
   {0xFF, 0xFF},  /* terminator */
};
#endif

#endif /* SMX_CFG_SSMX */
