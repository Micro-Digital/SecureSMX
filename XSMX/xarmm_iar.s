;
; xarmm_iar.s  (IAR version)                                Version 5.4.0
;
; ARM-M (e.g. Cortex-M) porting routines used by macros in xarmm.h
; that could not be implemented in the compiler's inline assembler.
; Also smx_PendSV_Handler/smx_PreSched.
;
; Copyright (c) 2008-2025 Micro Digital Inc.
; All rights reserved. www.smxrtos.com
;
; This software, documentation, and accompanying materials are made available
; under the Apache License, Version 2.0. You may not use this file except in
; compliance with the License. http://www.apache.org/licenses/LICENSE-2.0
;
; SPDX-License-Identifier: Apache-2.0
;
; This Work is protected by patents listed in smx.h. A patent license is
; granted according to the License above. This entire comment block must be
; preserved in all copies of this file.
;
; Support services are offered by MDI. Inquire at support@smxrtos.com.
;
; Authors: David Moore, Ralph Moore
;
;*****************************************************************************

         #include "xarmm_iar.inc"
         #include "bcfg.h"

         EXTERN   mp_MPULoad
         EXTERN   msp_sav
         EXTERN   sb_handler_en
         EXTERN   sb_TMLsr
         EXTERN   sb_UFM
         EXTERN   smx_clsr
         EXTERN   smx_ct
         EXTERN   smx_ctstart
         EXTERN   smx_EM
         EXTERN   smx_EVBLogLSRRet
         EXTERN   smx_lqctr
         EXTERN   smx_mstop
         EXTERN   smx_psp_sav
         EXTERN   smx_RTC_LSRStart
         EXTERN   smx_rqtop
         EXTERN   smx_sched
         EXTERN   smx_RTC_LSREnd
         EXTERN   smx_RTC_TaskStartID
         EXTERN   smx_autostop
         EXTERN   smx_SchedRunLSRs
         EXTERN   smx_SchedRunTasks
         EXTERN   smx_srnest
         EXTERN   smx_sst
         EXTERN   smx_sstp

         EXTERN   smxu_SchedAutoStopLSR

         PUBLIC   smx_SFModPC
         PUBLIC   smx_InMS
         PUBLIC   smx_GetPSR
         PUBLIC   smx_MakeFrame
         PUBLIC   smx_PendSV_Handler
         PUBLIC   smx_SVC_Handler
         PUBLIC   smx_SwitchStacks
         PUBLIC   smx_SwitchToNewStack
         PUBLIC   smx_SwitchToPSP
         PUBLIC   smx_UF_Handler
         #if SB_CPU_ARMM8
         PUBLIC   smx_MSSet
         PUBLIC   smx_TSOvfl
         #endif
         #if SMX_CFG_SSMX
         PUBLIC   mp_MPULoad_M8
         PUBLIC   smx_MakeFrameLSR
         PUBLIC   smx_StartSafeLSR
         PUBLIC   smx_SchedAutoStopLSR
         SECTION  `.sys.text`:CODE:NOROOT(2)
         #else
         SECTION  CODE:CODE:NOROOT(2)
         #endif
         THUMB

smx_GetPSR:
         push     {lr}
         mrs      r0, psr
         pop      {pc}

         ; psp = process stack pointer. Scheduler runs in PendSV exception
         ; and exceptions use main stack pointer (msp), so sp = msp.

smx_SwitchToNewStack:
         ldr      r0, =smx_ct                   ; get smx_ct address
         ldr      r0, [r0]                      ; get smx_ct tcb ptr
         ldr      r0, [r0, #SMX_TCB_OFFS_SBP]
         msr      psp, r0                       ; switch to a new stack
         bx       lr

smx_SwitchStacks:
         ldr      r0, =smx_ct                   ; get smx_ct address
         ldr      r0, [r0]                      ; get smx_ct tcb ptr
         ldr      r0, [r0, #SMX_TCB_OFFS_SP]
         msr      psp, r0                       ; switch stack pointer
         bx       lr

smx_SwitchToPSP:
         mov      r0, sp                        ; r0 = sp (msp)
         msr      psp, r0                       ; psp = sp
         mov      r0, #0x2
         msr      CONTROL, r0                   ; CONTROL[1] = 1 for 2-stack model
         isb
         bx       lr

smx_InMS:
         mrs      r0, psp                       ; r0 = psp
         mov      r1, sp                        ; r1 = sp
         sub      r0, r0, r1                    ; test if equal
         bx       lr

#if SB_CPU_ARMM8
smx_MSSet:
         mrs      r0, MSPLIM                    ; r0 = MSPLIM
         add      r0, r0, #12
         pop      {r1, r2}
         msr      msp, r0                       ; msp =MSPLIM
         push     {r1, r2}                      ; move up return regs
         bx       lr

smx_TSOvfl:
         mrs      r0, PSPLIM                    ; r0 = PSPLIM         
         mrs      r1, psp                       ; r1 = psp
         cmp      r0, r1                        ; test if psp < PSPLIM
         bpl      tso1                          ; task stack overflow
         mov      r0, #0                        ; main stack overflow
tso1:    bx       lr
#endif

smx_SFModPC:
         mrs      r1, msp                       ; r1 = main stack ptr
         add      r2, r1, #32                   ; r2 -> PC in stack frame
         ldr      r1, [r2]                      ; r1 = PC
         add      r1, r1, r0                    ; r1 = PC + r0
         str      r1, [r2]                      ; save new PC in stack frame
         bx       lr
         
; Make exception stack frame:
;        PSP   -> R0  = smx_ct->rv (C task) or smx_ct->thisptr (C++ task)
;        +4       R1  = ? (C task) or smx_ct->rv (C++ task)
;        +8       R2  = ? (? = don't care)
;        +12      R3  = ?
;        +16      R12 = ?
;        +20      LR  = address of smx_SchedAutoStop() if ptask
;        +20      LR  = address of smxu_SchedAutoStop() if utask
;        +24      PC  = smx_ct->fun (= task main function = task entry point)
;        +28      PSR = 0x01000000 (only T (Thumb) bit set)

smx_MakeFrame:
         push     {lr}
         mrs      r3, psp                       ; r3 = task stack
         sub      r3, r3, #32                   ; reserve space on task stack
         msr      psp, r3                       ;   for fake exception frame
         ldr      r12, =smx_ct
         ldr      r12, [r12]                    ; r12 -> smx_ct
         ldr      r0, [r12, #SMX_TCB_OFFS_THISPTR] ; r0 = smx_ct->thisptr
         ldr      r1, [r12, #SMX_TCB_OFFS_RV] ; r1 = smx_ct->rv
         cmp      r0, #0
         beq      mf_ncpp                       ; not c++
         stmia    r3, {r0-r1}                   ; psp -> thisptr, rv
         b        mf_cpp
mf_ncpp: str      r1, [r3]                      ; psp -> rv
mf_cpp:  ldr      r0, =smx_autostop             ; r0 = autostop pointer
         ldr      r0, [r0]
         ldr      r1, [r12, #SMX_TCB_OFFS_FUN]  ; r1 = smx_ct->fun
         ldr      r2, =0x01000000               ; r2 = Thumb
         add      r3, r3, #20
         stmia    r3, {r0-r2}                   ; psp+20 -> autostop, fun, psp
         pop      {pc}                          ; return

#if SMX_CFG_SSMX

; load MPU for ARMM8
mp_MPULoad_M8:
         ldr      r10, =0xe000ed98     ; r10 = RNR
         mov      r12, #MP_MPU_STATSZ
         str      r12, [r10]           ; RNR = MP_MPU_STATSZ
         mov      r12, #MP_MPU_ACTVSZ  ; r12 = MP_MPU_ACTVSZ   
         ldr      r1, =0xe000ed9c      ; r1 = RBAR
         cmp      r12, #4
         bmi      m1                   ; less than 4 slots to load

         ; load 4 MPU slots at a time
m0:      ldmia    r0!, {r2-r9}         ; read next 4 regions from MPA
         stmia    r1, {r2-r9}          ; load next 4 MPU slots <6>
         ldr      r2, [r10]            ; update RNR
         add      r2, r2, #4
         str      r2, [r10]
         subs     r12, R12, #4
         cmp      r12, #4
         bhs      m0                   ; load next 4 slots

         ; test for slots left
m1:      cmp      r12, #0
         beq      md                   ; 0
         cmp      r12, #2
         bhi      m3                   ; 3        
         beq      m2                   ; 2
                                       ; 1

         ldmia    r0, {r2-r3}          ; read next region of MPA
         stmia    r1, {r2-r3}          ; load last MPU slot
         b        md                   ; done
m2:      ldmia    r0, {r2-r5}          ; read next 2 regions of MPA
         stmia    r1, {r2-r5}          ; load last 2 MPU slots
         b        md                   ; done
m3:      ldmia    r0, {r2-r7}          ; read next 3 regions of MPA
         stmia    r1, {r2-r7}          ; load last 3 MPU slots 
md:      bx       lr                   ; done


; Make LSR exception stack frame:
;        PSP   -> R0  = par
;        +4       R1  = ?
;        +8       R2  = ? 
;        +12      R3  = ?
;        +16      R12 = ?
;        +20      LR  = smx_SchedAutoStopLSR if pmode
;        +20      LR  = smxu_SchedAutoStopLSR if umode
;        +24      PC  = smx_clsr->fun
;        +28      PSR = 0x01000000 (only T (Thumb) bit set)

smx_MakeFrameLSR: 
         push     {lr}                        
         ldr      r12, [r1, #SMX_LCB_OFFS_SBP]  ; r1 -> lcb 
                                                ; r12 = lsr stack bottom pointer
         sub      r12, r12, #32                 ; reserve space on lsr stack
                                                ;   for exception frame
         str      r0, [r12]                     ; save par in ex frame
         mov      r0, r1                        ; r0 -> lcb
         ldr      r1, [r0, #SMX_LCB_OFFS_FLAGS] ; test lsr->flags.umode
         ands     r1, r1, #SMX_LCB_FLAGS_UMODE
         ite      eq
         ldreq    r1, =smx_SchedAutoStopLSR     ; r1 = lr if pmode
         ldrne    r1, =smxu_SchedAutoStopLSR    ; r1 = lr if umode
         ldr      r2, [r0]                      ; r2 = lsr function
         ldr      r3, =0x01000000               ; r3 = Thumb bit
         add      r12, r12, #20
         stmia    r12, {r1-r3}                  ; load lr, fun, and psp at sp+20
                                                ;   into exception frame
         pop      {pc}

smx_StartSafeLSR:
         push     {lr}

         ; save psp stack pointer
         ldr      r1, =smx_psp_sav
         mrs      r2, psp
         str      r2, [r1]

         ; make exception frame <9>
         ldr      r1, =smx_clsr                 ; r1 = smx_clsr address
         ldr      r1, [r1]                      ; r1 -> LCB
         bl       smx_MakeFrameLSR

         ; switch psp to LSR stack pointer
         ldr      r2, [r0, #SMX_LCB_OFFS_SBP]   ; r0 ->LCB, r2 = lsr sbp
         sub      r2, r2, #32                   ; r2 -> stack frame
         msr      psp, r2 

         ; test mode
         ldr      r1, [r0, #SMX_LCB_OFFS_FLAGS] ; r1 = lsr flags
         ands     r1, r1, #SMX_LCB_FLAGS_UMODE
         bne      lsr1

         ; pmode: BR = 0 and CONTROL = 0
         smx_MPU_BR_OFF 
         mov      r1, #0
         msr      CONTROL, r1
         isb
         b        lsr2

lsr1:    ; umode: BR = SMX_CFG_MPU_BR_EN and CONTROL = 1
        #if !SMX_CFG_MPU_BR_EN
         smx_MPU_BR_OFF
        #endif
         mov      r1, #1
         msr      CONTROL, r1
         isb
lsr2:    pop      {pc}

; smx_AutoStopLSR() for ARMM is called when a safe LSR returns from its last }.
; smxu_LSRAutoStop() is called when a uLSR returns from its last }; it invokes 
; svc LS, which switches the processor to handler mode and jumps to here via 
; smx_sst[LS]. Calling of smx_ or smxu_ is determined by smx_MakeFrameLSR() in 
; xarmm_iar.s. Defined here in assembly rather than in C in scheduler to avoid
; a compiler code generation issue.

smx_SchedAutoStopLSR:
         bl       sb_TMLsr                      ; end of LSR time measurement
         smx_RTC_LSR_END                        ; end of LSR runtime count
         ldr.n    r1, =smx_clsr
         ldr      r0, [r1]
        #if SMX_CFG_EVB
         bl       smx_EVBLogLSRRet              ; log LSR return
        #endif
         ldr.n    r1, =smx_clsr
         movs     r0, #0                        ; clear smx_clsr
         str      r0, [r1]
         cpsie    i                             ; enable interrupts
         movs.w   r0, #0x10000000
         ldr.n    r1, =ARMM_NVIC_INT_CTRL
         str      r0, [r1]                      ; trigger PSVH
         isb
         pop      {pc}                          ; needed but not executed
                                                ; do not step; run to smx_PendSV_Handler below
#endif /* SMX_CFG_SSMX */

; smx PendSV handler (PSVH) is triggered from smx_SSRExit(), smx_SSRExitIF(),
; smx_ISR_EXIT(), or smx_SchedAutoStop. It calls the LSR scheduler and/or 
; task scheduler (sched). Both always return to PSVH, even if ct has changed. 
; This handler cannot be written in C due to compiler optimization problems.

smx_PendSV_Handler:
         smx_MPU_BR_ON              ; needed if sys_code not in MPU <2>
         sb_INT_DISABLE             ; for safety -- may not be necessary

         ; test for changed psp if smx_psp_sav != 0
         ldr      r1, =smx_psp_sav
         ldr      r0, [r1]          ; r0 = smx_psp_sav
         cbz      r0, psv_0         ; <7>

         ; restore psp and clear smx_psp_sav <10>
         msr      psp, r0
         mov      r2, #0
         str      r2, [r1]

psv_0:   push     {lr}              ; save EXC_RETURN
        #if SB_CFG_TM
         push     {r0}              ; <7>
         bl       sb_TMLsr          ; end of safe LSR time measurement (uLSR, pLSR)
         pop      {r0}
        #endif

         ; call LSR scheduler if smx_lqctr > 0
         ldr      r3, =smx_lqctr
         ldr      r2, [r3]
         cbz      r2, psv_1         ; skip LSR scheduler if lqctr == 0
         sub      sp, sp, #4        ; 8-byte stack align for ATPCS (due to push lr)
         bl       smx_SchedRunLSRs  ; r0 = smx_psp_sav from above
         add      sp, sp, #4
         cbz      r0, psv_1         ; if tLSR continue

         ; run safe LSR via exception return
        #if SMX_CFG_PROFILE
         bl       smx_RTC_LSRStart
        #endif
         cpsid    f
         sb_INT_ENABLE
         pop      {pc}  

         ; test smx_sched <1>
psv_1:   ldr      r3, =smx_sched   
         ldr      r2, [r3]
         tst      r2, #SMX_CT_STOP
         bne      psv_2             ; smx_sched = STOP
         tst      r2, #SMX_CT_DELETE
         bne      psv_2a            ; smx_sched = DELETE
        #if SMX_CFG_SSMX
         cbz      r2, psv_1a        ; smx_sched = 0 
        #else
         cbz      r2, psv_3         ; bypass task scheduler
        #endif

         ; smx_sched = SUSP or TEST 
         ldr      r1, =smx_ct
         ldr      r1, [r1]          ; r1 = smx_ct
         ldr      r2, =smx_rqtop
         ldr      r2, [r2]
         ldr      r2, [r2]          ; r2 = top task
         cmp      r2, r1
         bne      psv_1b            ; smx_ct != top task, call scheduler

         ; continue smx_ct since it is top task
         mov      r2, #0
         str      r2, [r3]          ; clear smx_sched

        #if SMX_CFG_SSMX
         ; reload MPU if it was changed by a safe LSR <8>
psv_1a:  cmp      r0, #0       
         it       ne
         blne     mp_MPULoad
         b        psv_3             ; bypass task scheduler
        #endif

         ; save non-volatile registers
psv_1b:  sb_INT_ENABLE
         ldr      r1, [r1, #SMX_TCB_OFFS_SBP] ; get ct RSA pointer
         stmia    r1, {r4-r11}
         sb_INT_DISABLE

         ; save task stack pointer in smx_ct->sp, save exret, and call sched.
psv_2:   ldr      r0, =smx_ct
         ldr      r0, [r0]                         ; r0 = smx_ct
         mrs      r1, psp                          ; r1 = psp
         str      r1, [r0, #SMX_TCB_OFFS_SP]       ; smx_ct->sp = psp
         pop      {lr}                             ; lr = EXC_RETURN
         strb     lr, [r0, #SMX_TCB_OFFS_EXRET]    ; save last lr byte in exret

psv_2a:  BL       smx_SchedRunTasks

         ldr      r0, =smx_ct                    ; r0 = smx_ct
         ldr      r0, [r0]

         ; test for task start. interrupts are disabled.
         ldr      r1, =smx_ctstart
         ldrb     r2, [r1]                      ; get ctstart
         cbz      r2, psv_3a                    ; task resume

         ; task start: clear ctstart and change exret to 0xFD
         mov      r2, #0
         strb     r2, [r1]                      ; clear ctstart
         mov      r2, #0xFD
         strb     r2, [r0, #SMX_TCB_OFFS_EXRET]
         b        psv_3c                        ; skip resume functions

psv_3a:
#if SMX_CFG_SSMX
         ; if ct->flags.rv_r0, copy ct->rv to exception frame r0 <11>
         ldr      r3, [r0, #SMX_TCB_OFFS_FLAGS] ; r3 = ct->flags
         tst      r3, #SMX_TCB_FLAGS_RV_R0
         beq      psv_3b                        ; ct->flags.rv_r0 == 0
         ldr      r1, [r0, #SMX_TCB_OFFS_RV]    ; r1 = ct->rv                       
         mrs      r2, psp
         str      r1, [r2]                      ; put ct->rv into exframe r0
         bic      r3, #SMX_TCB_FLAGS_RV_R0      ; clear rv_r0
         str      r3, [r0, #SMX_TCB_OFFS_FLAGS]
#endif
         ; restore non-volatile registers for resume
psv_3b:  ldr      r1, [r0, #SMX_TCB_OFFS_SBP]   ; get ct RSA pointer
         ldmia    r1, {r4-r11}

         ; make EXC_RETURN from exret and push it in ms
psv_3c:  ldrsb    lr, [r0, #SMX_TCB_OFFS_EXRET]
         push     {lr}

psv_3:   ; end of task scheduler bypass for a continued task
         ; clear smx_srnest
         ldr      r1, =smx_srnest
         mov      r2, #0
         str      r2, [r1] 

#if defined(SMX_DEBUG) || defined(SMXAWARE)
         ; clear smx_ct->susploc (task suspend location)
         ldr      r0, =smx_ct
         ldr      r0, [r0]
         str      r2, [r0, #SMX_TCB_OFFS_SUSPLOC] ; clear susploc
#endif
         ; mark task start for profiling and runtime limiting.
         smx_RTC_TASK_START_ID

#if SMX_CFG_MPU_ENABLE
         ; set BR OFF for pmode and CONTROL nPriv for umode
         ldr      r0, =smx_ct
         ldr      r0, [r0]
         ldr      r1, [r0, #SMX_TCB_OFFS_FLAGS]
         ands     r1, r1, #SMX_TCB_FLAGS_UMODE
         bne      psv_4
         smx_MPU_BR_OFF                         ; BR OFF 
         ; if Hard Fault here, probably sys_code region omitted from ct's MPA
         mov      r1, #0
         msr      CONTROL, r1                   ; ms, pmode
         isb
         b        psv_5                                                

psv_4:
#if !SMX_CFG_MPU_BR_EN
         ; set BR OFF for umode only in special case. see this option in .inc
         smx_MPU_BR_OFF
#endif
         mov      r1, #1
         msr      CONTROL, r1                   ; ms, umode
         isb
#endif   ;SMX_CFG_MPU_ENABLE

         ; Since FAULTMASK is cleared on exception exit, the following results
         ; in keeping interrupts disabled until after exception exit.
psv_5:   cpsid    f
         sb_INT_ENABLE
         POP      {pc}  ; exception return via EXC_RETURN

#if SMX_CFG_SSMX
#if SMX_CFG_DIAG
         EXTERN   smx_sst_ctr
         EXTERN   smx_svc_ctr
#endif

; smx_SVC_Handler
;  1. sp = msp in handler mode and psp -> exception stack frame.
;  2. The following code is optimized for a processor with pipelining and 0 
;     wait state SRAM.
;  3. Do not make SVC calls from handler mode <3>. 

smx_SVC_Handler:
         push     {lr}

#if SMX_CFG_DIAG
         ; increment smx_svc_ctr
         ldr      r0, =smx_svc_ctr
         ldr      r1, [r0]
         add      r1, r1, #1
         str      r1, [r0]
#endif
         ; get n from svc n in exception stack frame
         mrs      r0, psp
         ldr      r2, [r0, #24]
         ldrh     r2, [r2, #-2]  ; r2 = svc n
         and      r2, r2, #0xFF  ; r2 = n. if invalid see <6>
       
         ; abort if n >= sst lim
         ldr      r12, =smx_sstp 
         ldr      r12, [r12]     ; r12 -> current sst
         ldr      r1, [r12]      ; r1 = sst lim
         cmp      r2, r1
         bpl      svce

         ; if num par <= 4 skip copying parameters 5-7 
         ldr      r1, [r0, #16]  ; r1 = >4 par flag
         tst      r1, #1          
         beq      svc0

         ; find location of parameter 7 in task stack
         mov      r1, #44        ; basic exception frame size -4 + 16
         ldr      r3, [r0, #28]  ; test for alignment pad present
         tst      r3, #0x200
         it       ne
         addne    r1, #4         ; add alignment pad
         tst      lr, #0x10      ; test for floating point registers present
         it       eq
         addeq    r1, #72        ; add floating point registers size

         ; copy parameter 7 or dummy parameter from task stack to main stack
         ldr      r3, [r0, r1]
         str      r3, [sp, #-4]!

         ; copy parameter 6 or dummy parameter from task stack to main stack
         sub      r1, r1, #4
         ldr      r3, [r0, r1]
         str      r3, [sp, #-4]!

         ; copy parameter 5 from task stack to main stack
         sub      r1, r1, #4
         ldr      r3, [r0, r1]
         str      r3, [sp, #-4]!

svc0:
#if SMX_CFG_DIAG
         ; increment system service counter if smx_sstp == smx_sst
         ; (using main system service table not custom sst)
         ldr      r1, =smx_sstp
         ldr      r1, [r1]              ; r1 = smx_sstp
         ldr      lr, =smx_sst          ; lr = smx_sst addr
         cmp      r1, lr
         bne      svc0a                 ; smx_sstp != smx_sst so skip counting
         ldr      r1, =smx_sst_ctr
         ldr      r1, [r1]              ; r1 -> first slot in counter array
         ldr      lr, [r1, r2, LSL#2]   ; read counter at slot n
         add      lr, lr, #1
         str      lr, [r1, r2, LSL#2]
svc0a:
#endif
         ; fetch service address and load r0-r3 from exception stack frame <4>
         ldr      lr, [r12, r2, LSL#2]  ; lr -> service
         mov      r12, r0
         ldmia    r12, {r0-r3}

         ; CALL SERVICE
         BLX      LR
         ; return from service

         ; put return value in top of stack frame
         mrs      lr, psp
         str      r0, [lr]

         ; adjust main stack if par 5-7 were loaded into it
         ldr      r12, [lr, #16]
         tst      r12, #1
         it       ne
         addne    sp, sp, #12

#if defined(SMX_DEBUG) || defined(SMXAWARE)
         ; Save exception frame lr in smx_ct->susploc <5>
         ldr      r12, [lr, #20]
         ldr      lr, =smx_ct
         ldr      lr, [lr]
         str      r12, [lr, #SMX_TCB_OFFS_SUSPLOC]
#endif

         ; LSR flyback if lqctr > 0 
         sb_INT_DISABLE
         ldr      r1, =smx_lqctr
         ldr      r1, [r1]
         cbz      r1, svc1
         ldr      r1, =smx_srnest
         mov      r2, #1
         str      r2, [r1]
         ldr      r0, =ARMM_NVIC_INT_CTRL 
         ldr      r1, =ARMM_FL_PENDSVSET
         str      r1, [r0]

         ; return via EXC_RETURN
svc1:    cpsid    f
         sb_INT_ENABLE

         POP      {PC}

         ; error
svce:    mov      r0, #SMXE_PRIV_VIOL  
         mov      r1, #0
         bl       smx_EM
         pop      {pc}

#else /* !SMX_CFG_SSMX */
; EM SVC Handler.
smx_SVC_Handler:
         push     {lr}
         bl       smx_EM
         pop      {pc}
#endif

smx_UF_Handler:
         ; test sb_handler_en
         ldr.n    r0, =sb_handler_en
         ldrb     r0, [r0]
         cmp      r0, #0
         bne.n    ufh1           
         bkpt     #0x0                    ; halt

#if SB_CPU_ARMM8
         ; move MSPLIM to top of main stack to avoid LOCKUP             
ufh1:    ldr.n    r0, =smx_mstop
         ldr      r0, [r0]
         msr      MSPLIM, r0
         push     {lr}                    ; save EXC_RETURN
#else
ufh1:
#endif
         ; go to sb_UFM()
         bl       sb_UFM
         bkpt     #0x0                    ; halt

; Other fault handlers are in xarmm.c.

        END

; Notes:
; 1. Necessary if smx_PreSched() entered due to smx_lqctr > 0.
; 2. An LSR runs in the context of the task that was running when the ISR
;    interrupted, which could be any task, so data accessed by the LSR is
;    likely not to be accessible by the regions in that task's MPA. See
;    note 5 in xarmm.h about this problem for the ISR itself.
; 3. SVC_Handler must not be called when the processor is in handler mode.
;    This is true for exceptions and interrupts, as well as LSRs and the
;    scheduler, which run from the PendSV handler. These must not make SVC
;    calls nor call functions that do. They must not follow #include "xapiu.h".
;    This can cause strange behavior on the processor, and also SVC_Handler
;    expects that the stack frame is on the process stack not the main stack.
; 4. Volatile registers must be loaded from the stack frame because the SVC
;    handler could be interrupted by a higher-priority exception which might
;    change them.
; 5. ct does not actually wait prior to function return, so it is ok to save
;    susploc after the service return.
; 6. If n is invalid, it may be because the exception frame was saved on the
;    main stack due to using SVC from handler mode rather than thread mode,
;    which is not permitted (see 3). Return from SVC_Handler to see what
;    function made the call. Use #include "xapip.h" ahead of ISRs, LSRs,
;    and other privileged code.
; 7. r0 != 0 means that a uLSR or pLSR ran last.
;    r0 == 0 means that a tLSR ran last.
; 8. If a uLSR or a pLSR ran last and smx_ct is continuing, its MPA must be
;    reloaded into the MPU. 
; 9. Necessary because the exception frame PC is changed by the PendSV exception
;    following the last run of this LSR.
;10. For safe LSRs, smx_SchedAutoStopLSR() does not restore r4 because the 
;    PendSV exception prevents its epilogue from running.
;11. Necessary for a call via SVCH() which caused smx_ct to be suspended. The
;    value in smx_ct->rv has been overwritten by the SSR causing smx_ct to be
;    resumed. For a call not via SVCH(), when smx_ct resumes, it executes the
;    tail of smx_SSRExit(), which calls GetCTRV() to do the same thing as here.
