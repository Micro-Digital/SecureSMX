/*
* bspm.c                                                    Version 5.4.0
*
* Board Support Package API Module for ARM-M processors.
* All are similar so just use conditionals here for differences.
*
* See XBASE\bbsp.h.
*
* Copyright (c) 2001-2025 Micro Digital Inc.
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
* Author: David Moore
*
* Notes:
*
* 1. IRQ Numbering Convention: IRQs are numbered according to the bit
*    positions in the NVIC interrupt enable, pending, and active registers.
*
* 2. IRQ Priorities: 00 to FF (00 is the highest)
*    a. Processor may support as few as 3 bits of 8-bit priority, and
*       they are from most significant bit not least. So, if 3-bit
*       priority is supported by processor, bits 7:5 are used and 4:0
*       are unused. In this case there are only these 8 priorities:
*       0x00, 0x20, 0x40, 0x60, 0x80, 0xA0, 0xC0, 0xE0.
*       SB_IRQ_PRI_BITS must be set to reflect the processor's priority bit usage.
*    b. If exceptions have the same priority, the lower exception number
*       has higher priority. (e.g. exception 4 has higher priority than 5
*       if both are set to same priority).
*
* USER: We recommend that you delete conditional sections for other
*       processors to make this module simpler and easier to read.
*
*****************************************************************************/

#include "bbase.h"
#include "bsp.h"

#if SB_LCD && SB_LCD_DEMO
#include "lcd.h"
#endif

#if SMX_CFG_SSMX
#pragma section_prefix = ".sys"
#endif

extern SB_IRQ_REC sb_irq_table[SB_IRQ_NUM];  /* Moved to irqtable.c in BSP directory. */
extern const intvec_elem __vector_table[];

/* Global Variables */
bool sb_handler_en     = false;  /* enable fault handlers */
bool sb_tick_init_done = false;  /* sb_TickInit() sets */
/*
   IMPORTANT: These must match config of timer used for tick.
   Since SysTick is built into ARMM core, these settings are the same
   for all ARMM processors.
*/
const u32 sb_ticktmr_clkhz = SB_CPU_HZ;
const u32 sb_ticktmr_cntpt = SB_TICK_TMR_COUNTS_PER_TICK;

#pragma default_variable_attributes = @ ".ucom.rodata"
const u32 sbu_ticktmr_cntpt = SB_TICK_TMR_COUNTS_PER_TICK;
#pragma default_variable_attributes =

/* Local Variables */
static CPU_FL saved_int_state;  /* stores interrupt flag state before sb_IRQsMask() */
static u32    saved_int_mask[SB_IRQ_MASK_REGS];  /* stores the previous contents of the interrupt mask registers, which shows which interrupts are enabled */
static u32    saved_tickint_flag;  /* stores the previous SysTick TICKINT flag */

#ifdef __cplusplus
extern "C" {
#endif

/* External Functions */

void ClockConfig(void);  /* clock.c (some BSPs; inline here for others) */

/* Local Functions */

#if SMX_CFG_SSMX
bool sb_IRQPermCheck(int irq_num);
#endif

#ifdef __cplusplus
}
#endif


/*
   Routines are grouped by functional area. Complementary routines are
   adjacent.
*/


/*------ sb_ConsoleInInit(void)
*
* Documented in smxBase User's Guide.
*
* Differences from Spec: none
*
----------------------------------------------------------------------------*/

bool sb_ConsoleInInit(void)
{
   return(true);
}


/*------ sb_ConsoleOutInit(void)
*
* Documented in smxBase User's Guide.
*
* Differences from Spec: none
*
* Note: sb_PeripheralsInit() must run first to initialize UART.
*
----------------------------------------------------------------------------*/

bool sb_ConsoleOutInit(void)
{
  #if (SB_CFG_CON)
   sb_ConInit();
  #endif

   return(true);
}


/*------ sb_IntCtrlInit(void)
*
* Documented in smxBase User's Guide.
*
* Differences from Spec: none
*
----------------------------------------------------------------------------*/

bool sb_IntCtrlInit(void)
{
   int i;
   u32 pri_1108, pri_1512;  /* used to init priority register below */

   /* Enable Usage Fault, Bus Fault, and Memory Management Fault. */
   /* Otherwise, all generate Hard Fault. */
   *ARMM_NVIC_SYSHAN_CTRL_STATE |= 0x00070000;

   /* Set NVIC for no sub priorities (PRIGROUP = 0).
      0x05FA is an access key to allow writing this register.
      USER: You may want to comment out this line if chip vendor code runs
      before and sets this. Note that the value it sets may be different
      but equivalent, for the number of priority bits implemented on the
      processor. Vendor code may run after this that overrides it.
   */
   *ARMM_NVIC_APPINT_RST_CTRL = 0x05FA0000;

   /* Set STKALIGN bit for 8-byte exception stack alignment for AAPCS. */
   *ARMM_NVIC_CONFIG_CTRL |= 0x200;

   /* Set priorities for System Exceptions (4-15). 0-3 are fixed.
      See discussion about priorities in comment at top of this file.
      We set most to 0 and let their exception number decide which is
      higher priority. You can change these as desired.
      1. SysTick should be the highest maskable priority so it has higher
         priority than user ISRs (interrupts 16-239).
      2. SVC is set to the second-lowest priority, just above PendSV.
      3. PendSV must have lowest priority. We set it to 0xFF realizing low
         bits are ignored. For example if using a 3-bit priority, it becomes 0xE0.
         The PendSV priority is tested against the SB_IRQ_PRI_BITS mask as a check
         to ensure SB_IRQ_PRI_BITS is set correctly for new BSPs.
   */
#if SB_ARMM_DISABLE_WITH_BASEPRI
   pri_1108 = (0xFFUL - SB_IRQ_PRI_INCR) << 24;            /* sets SVC priority */
   pri_1512 = 0x00FF0000 | (SB_ARMM_BASEPRI_VALUE << 24);  /* OR sets SysTick priority */
#else
   pri_1108 = (0xFFUL - SB_IRQ_PRI_INCR) << 24;  /* sets SVC priority */
   pri_1512 = 0x00FF0000;
#endif
   *ARMM_NVIC_SYSEXC_PRI_0704 = 0x00000000;  /* --, Usage Fault, Bus Fault, MemManage Fault */
   *ARMM_NVIC_SYSEXC_PRI_1108 = pri_1108;    /* SVCall, --, --, -- */
   *ARMM_NVIC_SYSEXC_PRI_1512 = pri_1512;    /* SysTick, PendSV, --, Debug Mon */

#if defined(SMX_DEBUG)
   /* Safety check to ensure SB_IRQ_PRI_BITS is set right for new BSPs. */
   if ((*ARMM_NVIC_SYSEXC_PRI_1512 & 0x00FF0000) != (SB_IRQ_PRI_BITS << 16))
      sb_DEBUGTRAP()  /* Fix SB_IRQ_PRI_BITS in bsp.h if this triggers. */
#endif

   /* Set all interrupt priorities to the lowest value. At reset all are 0
      which is the highest priority, and it is non-maskable if BASEPRI is
      set (as we do by default). If the user forgets to call sb_IRQConfig(),
      this causes Bus Faults, Usage Faults, and other failures because
      critical sections get interrupted. The user could modify this to set
      priorities to the values in sb_irq_table[]. We don't because we have
      sb_irq_table[] initialized with priorities for products the user may
      not have. It is ok to write all FF's since unused low bits are ignored,
      and it is ok to clear the whole last u32 even if only part is used.
   */
   for (i = 0; i < (SB_IRQ_MAX+1+3)/4; i++)
   {
      ARMM_NVIC_IRQ_PRI_BASE[i] = 0xFFFFFFFF;
   }

   /* Clear interrupt pending in all registers in case set somehow. */
   for (i = 0; i < SB_IRQ_MASK_REGS; i++)  /* (number of mask registers same as number pending, active, etc) */
   {
      ARMM_NVIC_IRQ_CLR_PEND_BASE[i] = 0xFFFFFFFF;
   }

   return(true);
}


/*------ sb_PeripheralsInit(void)
*
* Documented in smxBase User's Guide.
*
* Differences from Spec: none
*
----------------------------------------------------------------------------*/

bool sb_PeripheralsInit(void)
{
   /* Call peripheral init routines. Add others, as desired. Note that
      middleware modules do this init in their drivers, not here.
   */

#if !(defined(SMX_STM32CUBEMX) && defined(SMX_TXPORT))
   sb_LEDInit();   /* LEDs varies per board. See led.c in BSP dir. */
#endif

#if SB_LCD && SB_LCD_DEMO
   sb_LCDInit();  /* See lcd.c in BSP dir. */
#endif

   /* SysTick is used for the smx tick and is initialized by sb_TickInit(). */

   /* Initialize other timers, UARTs, etc here */

#if (SB_CFG_CON)
   /* Initialize UARTs for terminal. Usually these use the same port. */
#if SB_CFG_UARTI
   sb_UartOpen(0, 8, 0);
#else

#if defined(SB_CPU_STM32F7XX) || defined(SB_CPU_STM32H7XX) || defined(SB_CPU_STM32U5XX)
   sb_UartInit(SB_CON_OUT_PORT, SB_CON_BAUD);

#elif defined(SB_BRD_NXP_LPC55S69_EVK)
  #if (SB_CON_IN_PORT == 0)
   Uart0Init(SB_CON_BAUD);
  #elif (SB_CON_IN_PORT == 3)
   Uart3Init(SB_CON_BAUD);
  #endif

#else
#error Add UART init for your processor here in bspm.c.
#endif
#endif /* SB_CFG_UARTI */
#endif /* SB_CFG_CON */

   return(true);
}


/*------ sb_TickInit(void)
*
* Documented in smxBase User's Guide.
*
* Differences from Spec:
* 1. Does not enable generation of tick interrupt since SysTick is not
*    in the range of IRQs, so it cannot be masked by sb_IRQMask() or
*    sb_IRQsMask. sb_TickIntEnable() will enable the interrupt.
*
----------------------------------------------------------------------------*/

#if (defined(SMX_STM32CUBEMX) && defined(SMX_TXPORT))
bool sb_TickInit(void) {} /* already done in startup code for TXPort */
#else
bool sb_TickInit(void)
{
   /* Note: If anything is changed here, ensure the tick globals are set
            correctly at the top of this file.
   */

#if defined(SB_CPU_LPC55SXX)
    /* Call ClockConfig() here because it uses the global variables. It
       cannot be called in startup.c since these variables are initialized
       after __low_level_init finishes.
    */
   ClockConfig();
#endif

   /* We use SysTick, which is a built-in System Exception, not one of
      the user interrupts (we call IRQs), so it is not in sb_irq_table[],
      and we do what is below instead of our usual sequence
      (sb_IRQVectSet(), sb_IRQConfig(), sb_IRQUnmask()).
      Priority of System Exceptions is set in sb_IntCtrlInit().
      SysTick counts in processor clocks, so it is simple to set the
      reload value.
   */
   *ARMM_NVIC_SYSTICK_CTRL = 0;       /* ensure SysTick is stopped */
   *ARMM_NVIC_INT_CTRL = 0x02000000;  /* clear SysTick pending status */
   sb_IntVectSet(15, smx_TickISR);    /* Int 15 not IRQ 15 */
   *ARMM_NVIC_SYSTICK_RELOAD = SB_TICK_TMR_COUNTS_PER_TICK-1;

   /* Don't enable interrupt here. See comment above this function. */
   *ARMM_NVIC_SYSTICK_CTRL   = 0x5; /* use core clock, enable */

   sb_tick_init_done = true;
   return(true);
}
#endif


/*------ sb_TickIntEnable(void)
*
* Documented in smxBase User's Guide.
*
* Differences from Spec: none
*
* Notes:
* 1. Normally sb_TickInit() enables generation of the tick interrupt.
*    This function is only needed in the case that the tick cannot be
*    masked by sb_IRQMask() or sb_IRQsMask() because it is not in the
*    range of IRQs or for some other reason.
*
----------------------------------------------------------------------------*/

bool sb_TickIntEnable(void)
{
   *ARMM_NVIC_SYSTICK_CTRL = 0x7; /* use core clock, gen interrupt, enable */
   return(true);
}


/*------ sb_IntStateRestore(prev_state)
*
* Documented in smxBase User's Guide.
*
* Differences from Spec: none
*
----------------------------------------------------------------------------*/

void sb_IntStateRestore(CPU_FL prev_state)
{
   #if SB_ARMM_DISABLE_WITH_BASEPRI
    __set_BASEPRI(prev_state);
   #else
    __set_PRIMASK(prev_state);
   #endif
}


/*------ sb_IntStateSaveDisable(void)
*
* Documented in smxBase User's Guide.
*
* Differences from Spec: none
*
* Notes:
* 1. The value returned has nothing to do with the processor flags
*    register, for this processor, but CPU_FL is large enough.
*
----------------------------------------------------------------------------*/

CPU_FL sb_IntStateSaveDisable(void)
{
   CPU_FL prev_state;

   #if SB_ARMM_DISABLE_WITH_BASEPRI
    prev_state = __get_BASEPRI();
   #else
    prev_state = __get_PRIMASK();
   #endif

    sb_INT_DISABLE();
    return (prev_state);
}


/*------ sb_IntVectGet(int_num, extra_info)
*
* Documented in smxBase User's Guide.
*
* Differences from Spec: none
*
----------------------------------------------------------------------------*/

ISR_PTR sb_IntVectGet(int int_num, u32 * extra_info)
{
   (void)extra_info;

   if (int_num < SB_INT_MIN || int_num > SB_INT_MAX)
      return(0);

   return (*((ISR_PTR *)__section_begin("EVT") + int_num));
}


/*------ sb_IntVectSet(int_num, isr_ptr)
*
* Documented in smxBase User's Guide.
*
* Differences from Spec: none
*
----------------------------------------------------------------------------*/

bool sb_IntVectSet(int int_num, ISR_PTR isr_ptr)
{
   if (int_num < SB_INT_MIN || int_num > SB_INT_MAX)
      return(false);

   *((ISR_PTR *)__section_begin("EVT") + int_num) = isr_ptr;

   return(true);
}


/*------ sb_IntTrapVectSet(int_num, isr_ptr)
*
* Documented in smxBase User's Guide.
*
* Differences from Spec: none
*
----------------------------------------------------------------------------*/

bool sb_IntTrapVectSet(int int_num, ISR_PTR isr_ptr)
{
   return (sb_IntVectSet(int_num, isr_ptr));  /* no special handling needed for traps; all vectors hooked the same way */
}


/*------ sb_IRQConfig(irq_num)
*
* Documented in smxBase User's Guide.
*
* Differences from Spec: none
*
----------------------------------------------------------------------------*/

bool sb_IRQConfig(int irq_num)
{
   bool retval = true;

   if (irq_num < SB_IRQ_MIN || irq_num > SB_IRQ_MAX)
      return(false);

   /* Check priority of this entry in sb_irq_table[] and return false if
      any of the unused low bits are set, but go ahead and set the priority.
      The register masks the low bits so ok to set with unmasked value.
   */
   if ((sb_irq_table[irq_num].pri & ~SB_IRQ_PRI_BITS) != 0)
      retval = false;

   /* Set priority. (Notice it's a byte pointer.) */
   *(((vu8 *)ARMM_NVIC_IRQ_PRI_BASE) + irq_num) = sb_irq_table[irq_num].pri;

   return(retval);
}


/*------ sb_IRQVectGet(irq_num, extra_info)
*
* Documented in smxBase User's Guide.
*
* Differences from Spec: none
*
----------------------------------------------------------------------------*/

ISR_PTR sb_IRQVectGet(int irq_num, u32 * extra_info)
{
   return(sb_IntVectGet(sb_IRQToInt(irq_num), 0));
}


/*------ sb_IRQVectSet(irq_num, isr_ptr)
*
* Documented in smxBase User's Guide.
*
* Differences from Spec: none
*
----------------------------------------------------------------------------*/

bool sb_IRQVectSet(int irq_num, ISR_PTR isr_ptr)
{
   return(sb_IntVectSet(sb_IRQToInt(irq_num), isr_ptr));
}


/*------ sb_IRQMask(irq_num)
*
* Documented in smxBase User's Guide.
*
* Differences from Spec: none
*
----------------------------------------------------------------------------*/

bool sb_IRQMask(int irq_num)
{
   if (irq_num < SB_IRQ_MIN || irq_num > SB_IRQ_MAX)
      return(false);

  #if SMX_CFG_SSMX
   if (!sb_IRQPermCheck(irq_num))
   {
      sb_DEBUGTRAP();
      return(false);
   }
  #endif

   /* Atomic write, not read-modify-write. */
   /* & 31 is same as % 32 (mod) but faster */
   *(ARMM_NVIC_IRQ_CLR_EN_BASE + (irq_num / 32)) = 1 << (irq_num & 31);
   return(true);
}


/*------ sb_IRQUnmask(irq_num)
*
* Documented in smxBase User's Guide.
*
* Differences from Spec: none
*
----------------------------------------------------------------------------*/

bool sb_IRQUnmask(int irq_num)
{
   if (irq_num < SB_IRQ_MIN || irq_num > SB_IRQ_MAX)
      return(false);

  #if SMX_CFG_SSMX
   if (!sb_IRQPermCheck(irq_num))
   {
      sb_DEBUGTRAP();
      return(false);
   }
  #endif

   /* Atomic writes, not read-modify-write. */
   /* & 31 is same as % 32 (mod) but faster */
// *(ARMM_NVIC_IRQ_CLR_PEND_BASE + (irq_num / 32)) = 1 << (irq_num & 31); /* Clear pending if somehow set */
   *(ARMM_NVIC_IRQ_SET_EN_BASE   + (irq_num / 32)) = 1 << (irq_num & 31); /* Enable */
   return(true);
}


/*------ sb_IRQsMask(void)
*
* Documented in smxBase User's Guide.
*
* Differences from Spec: none
*
* Notes:
* 1. sb_IRQsMask()/sb_IRQsUnmask() are implemented the normal way,
*    to mask interrupts at the interrupt controller. An alternative
*    might have been to use PRIMASK or BASEPRI (whichever isn't used
*    by sb_INT_DISABLE/ENABLE()), but these inhibit PendSV, so the
*    first smx task switch would not occur and the app would just exit.
*    The code below masks all interrupts at the interrupt controller
*    Unfortunately, there is no mask all bit as on some, so a loop
*    is needed (once per 32 interrupts). Most processors should have
*    far fewer than 240 IRQs, so only a few loops should be needed.
*    Also, smx only needs this in 1 place during init, so the
*    inefficiency is not a problem, unless you use it in your code too.
*
----------------------------------------------------------------------------*/

bool sb_IRQsMask(void)
{
   int i;

   /* DISABLE interrupts after saving current interrupt state */
   saved_int_state = sb_IntStateSaveDisable();

   /* Save and mask each interrupt mask register for all IRQs. */
   for (i = 0; i < SB_IRQ_MASK_REGS; i++)
   {
      saved_int_mask[i] = ARMM_NVIC_IRQ_CLR_EN_BASE[i];
      ARMM_NVIC_IRQ_CLR_EN_BASE[i] = 0xFFFFFFFF;
   }

   /* Save and disable SysTick interrupt. It is not a normal IRQ so it
      is not part of the masks set above. */
   saved_tickint_flag = *ARMM_NVIC_SYSTICK_CTRL & 0x2;
   *ARMM_NVIC_SYSTICK_CTRL &= ~0x2;

   /* Interrupts are still DISABLED. sb_IRQsUnmask() will restore
      the previous interrupt state.
   */
   return(true);
}


/*------ sb_IRQsUnmask(void)
*
* Documented in smxBase User's Guide.
*
* Differences from Spec: none
*
* Notes: See notes for sb_IRQsMask().
*
----------------------------------------------------------------------------*/

bool sb_IRQsUnmask(void)
{
   int i;

   /* DISABLE interrupts. The original interrupt state was saved in
      sb_IRQsMask() to be restored below.
   */
   sb_INT_DISABLE();

   /* Unmask interrupts that were previously unmasked. */
   for (i = 0; i < SB_IRQ_MASK_REGS; i++)
   {
      ARMM_NVIC_IRQ_SET_EN_BASE[i] = saved_int_mask[i];
   }

   /* Reenable SysTick interrupt if it was previously enabled. It is not
      a normal IRQ so it is not part of the masks cleared above. */
   *ARMM_NVIC_SYSTICK_CTRL |= saved_tickint_flag;

   /* ENABLE interrupts if they were enabled before the call to sb_IntStateSaveDisable()
      in sb_IRQsMask().
   */
   sb_IntStateRestore(saved_int_state);

   return(true);
}


#pragma diag_suppress=Pe550  /* avoid warning about dummy variable set but not used */

/*------ sb_IRQClear(irq_num)
*
* Documented in smxBase User's Guide.
*
* Differences from Spec: none
*
----------------------------------------------------------------------------*/

bool sb_IRQClear(int irq_num)
{
   /* Not needed for ARM-M ISRs because it is done automatically by
      the NVIC when the interrupt is serviced. Yiu p. 180.
      However, it is needed to clear a pending interrupt in some cases,
      such as in a driver before unmasking interrupts if masked awhile.
   */
   if (irq_num < SB_IRQ_MIN || irq_num > SB_IRQ_MAX)
      return(false);

  #if SMX_CFG_SSMX
   if (!sb_IRQPermCheck(irq_num))
   {
      sb_DEBUGTRAP();
      return(false);
   }
  #endif

   /* Atomic write, not read-modify-write. */
   /* & 31 is same as % 32 (mod) but faster */
   *(ARMM_NVIC_IRQ_CLR_PEND_BASE + (irq_num / 32)) = 1 << (irq_num & 31);
   return(true);
}


/*------ sb_IRQEnd(irq_num)
*
* Documented in smxBase User's Guide.
*
* Differences from Spec: none
*
----------------------------------------------------------------------------*/

bool sb_IRQEnd(int irq_num)
{
   /* Not needed for ARM-M. */
   (void)irq_num;
   return(true);
}

#pragma diag_default=Pe550  /* restore warning */


/*------ sb_IRQToInt(irq_num)
*
* Documented in smxBase User's Guide.
*
* Differences from Spec: none
*
* Notes:
* 1. IRQ numbers have a simple mapping to vector numbers. They follow
*    the first 16 system exceptions.
*
----------------------------------------------------------------------------*/

int sb_IRQToInt(int irq_num)
{
   if (irq_num >= SB_IRQ_MIN && irq_num <= SB_IRQ_MAX)
      return(irq_num + 16);
   else
      return(-1);
}


#if 0 //USER: Enable if needed and modify to meet any requirements for DMA.

/*------ sb_DMABufferAlloc(num_bytes)
*
* Documented in smxBase User's Guide.
*
* Differences from Spec: none
*
----------------------------------------------------------------------------*/

void * sb_DMABufferAlloc(uint num_bytes)
{
   return(smx_HeapMalloc(num_bytes));
}


/*------ sb_DMABufferFree(buf)
*
* Documented in smxBase User's Guide.
*
* Differences from Spec: none
*
----------------------------------------------------------------------------*/

bool sb_DMABufferFree(void *buf)
{
   return(smx_HeapFree(buf));
}

#endif /* 0/1 */


/*------ sb_PtimeGet(void)
*
* Documented in smxBase User's Guide.
*
* Differences from Spec: none
*
----------------------------------------------------------------------------*/
u32 sb_PtimeGet(void)
{
   return ((SB_TICK_TMR_COUNTS_PER_TICK-1) - (*ARMM_NVIC_SYSTICK_CURRENT & 0x00FFFFFF));
}


#if SMX_CFG_EVB
/*------ sb_EVBInit(void)
*
* Documented in smxBase User's Guide.
*
* Differences from Spec: none
*
----------------------------------------------------------------------------*/
bool sb_EVBInit(void)
{
   /* Nothing to do. */
   return true;
}
#endif


/*------ sb_Exit(retcode)
*
* Documented in smxBase User's Guide.
*
* Differences from Spec: none
*
----------------------------------------------------------------------------*/

void sb_Exit(int retcode)
{
   (void)retcode;
   sb_Reboot();  /* No environment to exit to. Could also infinite loop. */
}


/*------ sb_Reboot(void)
*
* Documented in smxBase User's Guide.
*
* Differences from Spec: none
*
----------------------------------------------------------------------------*/

void sb_Reboot(void)
{
   sb_IRQsMask();  /* mask all interrupts */

   //TODO: Implement reboot
   while (1)
   {
      sb_BREAKPOINT();
   }
}


/********************* Platform-Specific BSP functions **********************/
/*
   These functions provide services that are specific to this
   particular target, or they have argument lists that vary for
   each target, so they are not part of the standard API in bbsp.h.

   This is where the user should add any other needed functions.
*/

/*------ sb_IRQTableEntryWrite(irq_num, pri)
*
* Changes an entry dynamically in the sb_irq_table. Generally, the
* sb_irq_table should be initialized statically and left alone, but
* this function is provided in case there is a need to change it
* while running. After calling this, call sb_IRQConfig() to make
* the change in the hardware.
*
* This function is primarily provided for assembly access, since
* it may not be possible to access a C structure in assembly.
* Even for C, it is good style to call this function rather than
* modifying the structure directly.
*
* USER: Create additional functions if you wish to read/write
* individual fields of an sb_irq_table[] record. We have not created
* additional functions because most users will not need/want them.
* Most will set the hardware once using the values in sb_irq_table,
* above.
*
* Parameters
*    irq_num       indexes into sb_irq_table[]
*    pri           interrupt priority
*
* Returns
*    true          ok
*    false         fail
*
----------------------------------------------------------------------------*/

bool sb_IRQTableEntryWrite(int irq_num, int pri)
{
   /* sb_IRQConfig() checks the fields; here we just need to ensure
      irq_num is in range so we don't write outside sb_irq_table[].
   */
   if (irq_num < SB_IRQ_MIN || irq_num > SB_IRQ_MAX)
      return(false);

   sb_irq_table[irq_num].pri = (u8)pri;

   return(true);
}

/************ Local functions used by the BSP API routines above ************/
/*
   These functions are intended for use by this file, but since some
   may be useful to the application, we don't declare them static.
   The set of functions defined here will be unique to each BSP.
   In order to gain access, add the prototypes for these functions
   to smxmain.h or directly to the .c module that uses them.
*/

#if SMX_CFG_SSMX
bool sb_IRQPermCheck(int irq_num)
{
   IRQ_PERM *p;

   /* if in SVC and ct is utask check IRQ permissions in TCB <1> */
   if (((__get_PSR() & 0xFF) == 0x0B) && (smx_ct->flags.umode == 1))
   {
      if (smx_ct->irq == NULL)
         return false;
      for (p = smx_ct->irq; *(u16*)p != 0xFFFF; p++)
      {
         if (irq_num >= p->irqmin && irq_num <= p->irqmax)
            return true;
      }
      return false;
   }
   else
      return true;
}
#endif

/*
   Notes:
   1. IRQ Mask/Unmask permissions check: Checking ct umode flag is
      insufficient, since we could be in an ISR that interrupted ct.
      Also, checking if we're in SVC is insufficient, since a ptask
      may do a SVC call due to calling a common routine used in umode.
      If we're not in SVC, we're in privileged code (either an ISR
      or a ptask), so we do not do the privilege check. Otherwise,
      if we're in a SVC call, it means we're in a task, so check
      if that task is a utask, and if so do permission checks.
*/
