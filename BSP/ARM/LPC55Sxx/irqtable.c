/*
* irqtable.c                                                Version 5.4.0
*
* sb_irq_table[] for NXP LPC55Sxx. Normally in bsp.c, but here in BSP dir since
* all ARM-M share the same bsp.c.
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
* Author: Xingsheng Wan
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

   Priority Values: See note at top of bsp.c. This processor uses 3-bit
   priority (bits 7:5) so valid priorities are only:

      0x00, 0x20, 0x40, 0x60, 0x80, 0xA0, 0xC0, 0xE0

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
   {  0xFF  },  /*  0   WDT, BOD, FLASH */
   {  0xFF  },  /*  1   SDMA0 */
   {  0xFF  },  /*  2   GPIO_GLOBALINT0 */
   {  0xFF  },  /*  3   GPIO_GLOBALINT1 */
   {  0xFF  },  /*  4   GPIO_INT0_IRQ0 */
   {  0xFF  },  /*  5   GPIO_INT0_IRQ1 */
   {  0xFF  },  /*  6   GPIO_INT0_IRQ2 */
   {  0xFF  },  /*  7   GPIO_INT0_IRQ3 */
   {  0xFF  },  /*  8   UTICK */
   {  0xFF  },  /*  9   MRT */
   {  0xFF  },  /* 10   CTIMER0 */
   {  0xFF  },  /* 11   CTIMER1 */
   {  0xFF  },  /* 12   SCT */
   {  0xFF  },  /* 13   CTIMER3 */
   {  0xFF  },  /* 14   Flexcomm Interface 0 */
   {  0xFF  },  /* 15   Flexcomm Interface 1 */
   {  0xFF  },  /* 16   Flexcomm Interface 2 */
   {  0xFF  },  /* 17   Flexcomm Interface 3 */
   {  0xFF  },  /* 18   Flexcomm Interface 4 */
   {  0xFF  },  /* 19   Flexcomm Interface 5 */
   {  0xFF  },  /* 20   Flexcomm Interface 6 */
   {  0xFF  },  /* 21   Flexcomm Interface 7 */
   {  0xFF  },  /* 22   ADC */
   {  0xFF  },  /* 23   Reserved */
   {  0xFF  },  /* 24   ACMP */
   {  0xFF  },  /* 25   Reserved */
   {  0xFF  },  /* 26   Reserved */
   {  0xFF  },  /* 27   USB0_NEEDCLK */
   {  0xFF  },  /* 28   USB0 */
   {  0xFF  },  /* 29   RTC */
   {  0xFF  },  /* 30   Reserved */
   {  0xFF  },  /* 31   MAILBOX */
   {  0xFF  },  /* 32   GPIO_INT0_IRQ4 */
   {  0xFF  },  /* 33   GPIO_INT0_IRQ5 */
   {  0xFF  },  /* 34   GPIO_INT0_IRQ6 */
   {  0xFF  },  /* 35   GPIO_INT0_IRQ7 */
   {  0xFF  },  /* 36   CTIMER2 */
   {  0xFF  },  /* 37   CTIMER4 */
   {  0xFF  },  /* 38   OSEVTIMER */
   {  0xFF  },  /* 39   Reserved */
   {  0xFF  },  /* 40   Reserved */
   {  0xFF  },  /* 41   Reserved */
   {  0xFF  },  /* 42   SDIO */
   {  0xFF  },  /* 43   Reserved */
   {  0xFF  },  /* 44   Reserved */
   {  0xFF  },  /* 45   Reserved */
   {  0xFF  },  /* 46   USB1_PHY */
   {  0xFF  },  /* 47   USB1 */
   {  0xFF  },  /* 48   USB1_NEEDCLK */
   {  0xFF  },  /* 49   HYPERVISOR */
   {  0xFF  },  /* 50   SGPIO_INT0_IRQ0 */
   {  0xFF  },  /* 51   SGPIO_INT0_IRQ1 */
   {  0xFF  },  /* 52   PLU */
   {  0xFF  },  /* 53   SEC_VIO, SECURE_VIOLATION, SEC_VIOLATION */
   {  0xFF  },  /* 54   HASH */
   {  0xFF  },  /* 55   CASPER */
   {  0xFF  },  /* 56   PUF */
   {  0xFF  },  /* 57   PQ */
   {  0xFF  },  /* 58   SDMA1 */
   {  0xFF  },  /* 59   HS_SPI */
};

#define UNUSED_IRQ_REC 0xFF  /* use same value in sb_irq_table[] above */

#if SMX_CFG_SSMX
/* IRQs permitted to be masked/unmasked by utasks */ //USER: Adjust as needed for your system. Comment out unneeded IRQs.

#if SB_CFG_UARTI
const IRQ_PERM sb_irq_perm_uart[] = {
   {14, 14},      /* USART0 for terminal output */
   {0xFF, 0xFF},  /* terminator */
};
#endif

#endif /* SMX_CFG_SSMX */
