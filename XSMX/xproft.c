/*
* xproft.c                                                  Version 5.4.0
*
* Profiling functions using STM32F7 TIM2.
*
* Copyright (c) 2011-2025 Micro Digital Inc.
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
* Author: Ralph Moore
*
*****************************************************************************/

/* Alternative to using tick counter. Not fully tested. Positioning of some 
   start and end functions is different. */

#if SMX_CFG_PROFILE

/* start an ISR runtime period. called for base ISR */
void smx_RTC_ISRStart(void)
{
   u32 istate = sb_IntStateSaveDisable();
   *ARMM_TIM2_CR1 = 0;        /* stop counting */
   u32 d = *ARMM_TIM2_CNT;    /* get count */
   *ARMM_TIM2_CNT = 0;        /* clear counter */
   if (smx_pf == SMX_TASK)
   {
      smx_ct->rtc += d;       /* update smx_ct runtime count */

     #if SMX_CFG_RTLIM
      if (smx_ct->parent)
         *(u32*)smx_ct->rtlimctr += d; /* update top parent rtlimctr <1> */
      else
         smx_ct->rtlimctr += d;        /* update smx_ct->rtlimctr */
     #endif
   }
   if (smx_pf == SMX_LSR)
      smx_lsr_rtc += d;       /* update lsr runtime count */
   smx_pf = SMX_ISR;
   *ARMM_TIM2_CR1 |= 1;       /* start counting */
   sb_IntStateRestore(istate);
}

/* end runtime period. called for base ISR. interrupts disabled. */
void smx_RTC_ISREnd(void)
{
   *ARMM_TIM2_CR1 = 0;        /* stop counting */
   u32 d = *ARMM_TIM2_CNT;    /* get count */
   *ARMM_TIM2_CNT = 0;        /* clear counter */
   smx_isr_rtc += d;
   smx_pf = SMX_OVH;
}

/* start an LSR runtime period */
void smx_RTC_LSRStart(void)
{
   smx_pf = SMX_LSR;
   *ARMM_TIM2_CR1 |= 1;       /* start counting */
}

/* end an LSR runtime period */
void smx_RTC_LSREnd(void)
{
   *ARMM_TIM2_CR1 = 0;        /* stop counting */
   u32 d = *ARMM_TIM2_CNT;    /* get count */
   *ARMM_TIM2_CNT = 0;        /* clear counter */
   smx_lsr_rtc += d;
   smx_pf = SMX_OVH;
}
#else
void smx_RTC_ISRStart(void)
{}
void smx_RTC_ISREnd(void)
{}
void smx_RTC_LSRStart(void)
{}
void smx_RTC_LSREnd(void)
{}
#endif
#endif /* SMX_CFG_PROFILE */

#if SMX_CFG_PROFILE || SMX_CFG_RTLIM

/* start a task runtime period */
void smx_RTC_TaskStart(void)
{
   u32 istate = sb_IntStateSaveDisable();
   *ARMM_TIM2_CNT = 0;        /* clear counter */
   smx_pf = SMX_TASK;
   *ARMM_TIM2_CR1 |= 1;       /* start counting */
   sb_IntStateRestore(istate);
}

/* end a task runtime period */
void smx_RTC_TaskEnd(void)   /* interrupts enabled */
{
   sb_INT_DISABLE();
   *ARMM_TIM2_CR1 = 0;        /* stop counting */
   u32 d = *ARMM_TIM2_CNT;    /* get count */
   *ARMM_TIM2_CNT = 0;        /* clear counter */
   smx_ct->rtc += d;          /* update ct runtime count */

  #if SMX_CFG_RTLIM
   if (smx_ct->parent)
      *(u32*)smx_ct->rtlimctr += d; /* update top parent rtlimctr <1> */
   else
      smx_ct->rtlimctr += d;        /* update smx_ct->rtlimctr */
  #endif
   smx_pf = SMX_OVH;
   sb_INT_ENABLE();
}

/*------ sb_TimersInit(void)
*
*  Initilizes TIM2 for profiling and runtime limiting.
*
----------------------------------------------------------------------------*/

void sb_TimersInit(void)
{
   *ARMM_RCC_APB1ENR = 1;  /* enable TIM2 clock */
   *ARMM_TIM2_CR2 = 0;     /* mode 0 */
   *ARMM_TIM2_PSC = 50;    /* count = usec */
   *ARMM_TIM2_CNT = 0;     /* clear counter */
   *ARMM_TIM2_CR1 = 0;     /* count up, counter disabled */
   return;
}

#endif /* SMX_CFG_PROFILE || SMX_CFG_RTLIM */