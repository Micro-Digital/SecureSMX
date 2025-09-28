/*
* xarmm.h                                                   Version 5.4.0
*
* ARMM (e.g. Cortex-M) definitions and macros, including C scheduler
* macros. The smx Porting Guide documents these macros.
*
* Copyright (c) 2008-2025 Micro Digital Inc.
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
* Authors: David Moore, Ralph Moore
*
*****************************************************************************/

#ifndef SMX_XARMM_H
#define SMX_XARMM_H

#ifdef __cplusplus
extern "C" {
#endif

void     smx_MakeFrame(void);
void     smx_MSSet(void);
void     smx_SFModPC(s32 n);           /* modify PC in stack frame */
bool     smx_TSOvfl(void);             /* task stack overflow (ARMM8) */

void     smx_StartSafeLSR(u32 par);
void     smx_SchedAutoStop(void);
void     smx_SchedAutoStopLSR(void);
void     smxu_SchedAutoStop(void);
void     smxu_SchedAutoStopLSR(void);

#if SMX_CFG_SSMX
void     smx_BROff(void);
void     smx_BROn(void);
void     smx_BRRestoreOff(void);
void     smx_BRSaveOn(void);
void     smx_PBlockRelSlot(u8 sn);
bool     smx_TaskOpPermit(TCB_PTR task);
#endif

#if SMX_CFG_SSMX && SMX_CFG_MPU_ENABLE
#define  smx_MPU_BR_OFF()         smx_BROff();
#define  smx_MPU_BR_ON()          smx_BROn();
#define  smx_MPU_BR_RESTORE_OFF() smx_BRRestoreOff();
#define  smx_MPU_BR_SAVE_ON()     smx_BRSaveOn();
#else
#define  smx_MPU_BR_OFF()
#define  smx_MPU_BR_ON()
#define  smx_MPU_BR_RESTORE_OFF()
#define  smx_MPU_BR_SAVE_ON()
#endif

#ifdef __cplusplus
}
#endif

#define smx_ISR_ENTER() \
   { \
      smx_SAVE_SUSPLOC_ISR();   /* must be done before any call since uses LR */ \
      smx_ISREnter(); \
   }

#define smx_ISR_EXIT()  smx_ISRExit();


#if defined(__ICCARM__)
/*===========================================================================*
*                               IAR ARM C/C++                                *
*===========================================================================*/
/*
   Notes:
   1. Support for inline assembly in the IAR compiler is weak, so we
      simply map these macros onto routines in xarmm_iar.s. Specifically,
      a. It does not support the pseudoinstruction "LDR r0, =global_var".
      b. It does not allow using temporary registers or local variables.
      c. It requires saving any registers used in the macro. By making
         the macros do a function call, we can use the volatile registers
         because the compiler knows not to expect them to be preserved
         across a function call. The compiler does not have a way to
         let us tell it to pick a temporary register we can use, and it
         also can't access autovariables by name.
      IAR's documentation of inline assembly recommends against its use
      and points out it is likely the code could break in new releases
      of the compiler. It suggests that calling assembly routines may
      also have better performance.
*/

/* Scheduler Macros */

/* In xarmm_iar.s */
#ifdef __cplusplus
extern "C" {
#endif
extern void mp_MPULoad_M8(u32* mp);
extern void smx_SwitchToNewStack(void);
extern void smx_SwitchStacks(void);
extern void smx_SwitchToPSP(void);
#ifdef __cplusplus
}
#endif

#define SMX_RSA_SIZE  32  /* register save area size above top of stack */

#define smx_SWITCH_TO_NEW_STACK() smx_SwitchToNewStack();
#define smx_SWITCH_STACKS()       smx_SwitchStacks();

/* trigger smx_PendSV_Handler() */
#define smx_PENDSVH()            {*ARMM_NVIC_INT_CTRL = ARMM_FL_PENDSVSET; \
                                  sb_INT_ENABLE(); \
                                  __ISB(); }  /* <1> */

/* Macros to save the task suspend location (where it will resume).
   Called from ISR_ENTER() and SSR_ENTER(). For debug. */

#if defined(__ICCARM__)   /* IAR */
#if defined(SMX_DEBUG) || defined(SMXAWARE)
#define smx_PROCESS_STACK 0x0004

#define smx_SAVE_SUSPLOC() \
   { \
      if (smx_ct->susploc == 0) \
      { \
         smx_ct->susploc = (void*) __get_LR(); \
      } \
   }

#define smx_SAVE_SUSPLOC_ISR() \
   { \
      if (smx_ct->susploc == 0) \
      { \
         u32* stackPointer; \
         if (__get_LR() & smx_PROCESS_STACK)         /* find EXC_RETURN's return stack */ \
            stackPointer = (u32*) __get_PSP();       /* read the Process Stack Pointer */ \
         else \
            stackPointer = (u32*) __get_MSP();       /* read the Main Stack Pointer */ \
         smx_ct->susploc = (void*) stackPointer[6];  /* return PC is the 6th item in the exception stack frame */ \
      } \
   }

#define smx_CLEAR_SUSPLOC() \
   { \
      smx_ct->susploc = 0; \
   }
#else
#define smx_SAVE_SUSPLOC()
#define smx_SAVE_SUSPLOC_ISR()
#define smx_CLEAR_SUSPLOC()
#endif

#endif

#else
#error Define the porting macros for your compiler here in xarmm.h.

#endif  /* compiler checks */
/*==========================================================================*/

/*
Notes:

1. smx_PENDSVH(): A memory barrier instruction is needed after setting the
   PENDSV flag to ensure the exception happens immediately, not one or more
   instructions below, which could cause a problem, such as if GetCTRV()
   (which follows in smx_SSRExit()) is inlined. In this case, the return from
   exception would return in the middle of it, not at its first line, causing
   smx_ct->rv not to be actually read. The ENABLE() (or a function call if
   GetCTRV() were not inlined) seems to act as a memory barrier instruction,
   but we cannot rely on this and must use an explicit ISB.
*/

#endif /* SMX_XARMM_H */
