;
; xarmm_iar.s  (IAR version)                                Version 6.2.0
;
; ARM-M (e.g. Cortex-M) porting routines used by macros in xarmm.h
; that could not be implemented in the compiler's inline assembler.
; Also smx_PendSV_Handler/smx_PreSched.
;
; Copyright (c) 2008-2026 Micro Digital Inc.
; All rights reserved. www.smxrtos.com
;
; SPDX-License-Identifier: GPL-2.0-only OR LicenseRef-MDI-Commercial
;
; This software, documentation, and accompanying materials are made available
; under a dual license, either GPLv2 or Commercial. You may not use this file
; except in compliance with either License. GPLv2 is at www.gnu.org/licenses.
; It does not permit the incorporation of this code into proprietary programs.
;
; Commercial license and support services are available from Micro Digital.
; Inquire at support@smxrtos.com.
;
; This Work embodies patents listed in smx.h. A patent license is hereby
; granted to use these patents in this Work and Derivative Works, except in
; another RTOS or OS.
;
; This entire comment block must be preserved in all copies of this file.
;
; Authors: David Moore, Ralph Moore
;
;*****************************************************************************

         #include "xarmm_iar.inc"
         #include "bcfg.h"

         EXTERN   mp_MPULoad
         EXTERN   sb_handler_en
         EXTERN   sb_TMLsr
         EXTERN   sb_UFM
         EXTERN   smx_clsr
         EXTERN   smx_ct
         EXTERN   smx_EM
         EXTERN   smx_EVBLogLSRRet
         EXTERN   smx_EVBLogTaskResume
         EXTERN   smx_EVBLogTaskStart
         EXTERN   smx_lqctr
         EXTERN   smx_mstop
         EXTERN   smx_psp_sav
         EXTERN   smx_RTC_LSRStart
         EXTERN   smx_rqtop
         EXTERN   smx_sched
         EXTERN   smx_RTC_LSREnd
         EXTERN   smx_RTC_TaskStart
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
         PUBLIC   smx_UF_Handler
        #if SB_CPU_ARMM8
         PUBLIC   smx_MSSet
         PUBLIC   smx_TSOvfl
        #endif
        #if SMX_CFG_SSMX
         PUBLIC   mp_MPULoad_M8
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
;        +28      PSR = 0x01000000 = Thumb bit

smx_MakeFrame:
         push     {lr}
         mrs      r3, psp                       ; r3 = task stack
         sub      r3, r3, #32                   ; reserve space on task stack
         msr      psp, r3                       ;   for fake exception frame
         ldr      r12, =smx_ct
         ldr      r12, [r12]                    ; r12 -> smx_ct
         ldr      r0, [r12, #SMX_TCB_OFFS_THISPTR] ; r0 = smx_ct->thisptr
         ldr      r1, [r12, #SMX_TCB_OFFS_RV]   ; r1 = smx_ct->rv
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
         push     {r10, r12}
         mov      r12, #MP_MPU_STATSZ
         ldr      r10, =ARMM_MPU_RNR   ; r10 = RNR
         str      r12, [r10]           ; RNR = MP_MPU_STATSZ
         mov      r12, #MP_MPU_ACTVSZ  ; r12 = MP_MPU_ACTVSZ   
         ldr      r1, =ARMM_MPU_RBAR   ; r1 = RBAR
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
md:      pop      {r10, r12}
         bx       lr                   ; done

; LSR exception stack frame:
;        PSP   -> R0  = par
;        +4       R1  = ?
;        +8       R2  = ? 
;        +12      R3  = ?
;        +16      R12 = ?
;        +20      LR  = smx_SchedAutoStopLSR if pmode
;        +20      LR  = smxu_SchedAutoStopLSR if umode
;        +24      PC  = smx_clsr->fun
;        +28      PSR = 0x01000000 = Thumb bit

smx_StartSafeLSR:
         push     {lr}
         mrs      r2, psp
         ldr      r1, =smx_psp_sav              ; save psp stack pointer
         str      r2, [r1]
         ldr      r1, =smx_clsr                 ; r1 = smx_clsr address
         ldr      r1, [r1]                      ; r1 -> LCB

         ; make exception frame <9>                        
         ldr      r12, [r1, #SMX_LCB_OFFS_SBP]  ; r1 -> LCB 
                                                ; r12 = lsr stack bottom pointer
         sub      r12, r12, #32                 ; reserve space on lsr stack
                                                ;   for exception frame
         str      r0, [r12]                     ; save par in ex frame
         mov      r0, r1                        ; r0 -> LCB
         ldrb     lr, [r0, #SMX_LCB_OFFS_FLAGS] ; test lsr->flags.umode
         tst      lr, #SMX_LCB_FLAGS_UMODE
         ite      eq
         ldreq    r1, =smx_SchedAutoStopLSR     ; r1 = lr if pmode
         ldrne    r1, =smxu_SchedAutoStopLSR    ; r1 = lr if umode
         ldr      r2, [r0]                      ; r2 = lsr function
         ldr      r3, =0x01000000               ; r3 = Thumb bit
         add      r0, r12, #20
         stmia    r0, {r1-r3}                   ; load lr, fun, and psp at sp+20
                                                ;   into exception frame1
         msr      psp, r12                      ; psp -> exception frame
         pop      {pc}

; autostop for safe LSRs <2>
smx_SchedAutoStopLSR:
         smx_RTC_LSR_END                        ; end of LSR runtime period
        #if SMX_CFG_EVB
         ldr.n    r1, =smx_clsr
         ldr      r0, [r1]
         bl       smx_EVBLogLSRRet              ; log LSR return
        #endif
         movs     r0, #0                        ; clear smx_clsr
         ldr.n    r1, =smx_clsr
         str      r0, [r1]
         movs.w   r0, #0x10000000
         ldr.n    r1, =ARMM_NVIC_INT_CTRL
         str      r0, [r1]                      ; trigger PSVH
         isb
         pop      {pc}                          ; needed but not executed
                                                ; do not step; run to smx_PendSV_Handler below

; make new exception frame on task stack for exit from PSVH() to smx_RunDAF
smx_MakeFrameDAF:
         push     {lr}
         mrs      r12, psp                      ; r12 -> top of task stack
         sub      r12, r12, #32                 ; reserve space on task stack
         msr      psp, r12                      ;   for DAF exception frame
         ldr      r1, =smx_RunDAF               ; r1 = &RunDAF
         ldr      r2, =0x01000000               ; r2 = Thumb bit
         add      r12, r12, #24
         stmia    r12!, {r1,r2}                 ; psp+24 -> fun, psp
         pop      {pc}                          ; return

;  run deferred action function in pmode when called from umode, then trigger 
;  PSVH() to return to point of call in umode.
smx_RunDAF:
         ; initialize frame pointers
         mrs      r12, psp       ; r12 -> orig ex frame 
         sub      r0, r12, #32   ; r0  -> new ex frame above orig ex frame     

         ; if num par <= 4 skip copying par5-7 
         ldr      r3, [r12, #16] ; r3 = r12 from exception frame
         tst      r3, #1         ; r3 bit0 = >4 par flag
         beq      rd0

         ; find offset from r0 to par5 in task stack
         mov      r1, #36        ; basic exception frame size + 4
         ldr      r3, [r12, #28] ; test for alignment pad present
         tst      r3, #0x200
         it       ne
         addne    r1, #4         ; add alignment pad
         mrs      lr, msp        ; fetch original lr
         ldr      lr, [lr]
         tst      lr, #0x10      ; test for floating point registers present
         it       eq
         addeq    r1, #72        ; add floating point registers size

         ; copy par5-7 or dummy parameters above new ex frame in task stack
         add      lr, r1, r12    ; lr -> par5 in task stack
         ldmia    lr, {r1-r3}
         sub      r0, r0, #12
         msr      psp, r0        ; psp = top of task stack, in case of interrupt
         stmia    r0, {r1-r3}
         b        rd1

         ; psp = top of task stack, in case of interrupt
rd0:     msr      psp, r0

         ; load par 1 to 4 from orig ex frame into r0 to r3
rd1:     ldmia    r12, {r0-r3}                     ; r12 -> orig ex frame

         ldr      lr, =smx_ct
         ldr      lr, [lr]                         ; lr = smx_ct
         ldr      r12, [lr, #SMX_TCB_OFFS_FLAGS]
         and      r12, r12, #~SMX_TCB_FLAGS_DA_ENTER ; clear smx_ct->flags.da_enter
         orr      r12, r12, #SMX_TCB_FLAGS_DA_RUN  ; set smx_ct->flags.da_run
         str      r12, [lr, #SMX_TCB_OFFS_FLAGS]
         ldr      r12, [lr, #SMX_TCB_OFFS_DAF]     ; r12 -> deferred action function

         ; clear srnest
         push     {r4}
         mov      r4, #0
         ldr      lr, =smx_srnest
         str      r4, [lr]
         pop      {r4}

         BLX      r12  ; call deferred action function

         ; restore srnest
         mov      r2, #1
         ldr      lr, =smx_srnest
         str      r2, [lr]                         

         ldr      lr, =smx_ct
         ldr      lr, [lr]                         ; lr = smx_ct
         ldr      r12, [lr, #SMX_TCB_OFFS_FLAGS]
         tst      r12, #SMX_TCB_FLAGS_G4PAR
         mrs      r2, psp
         add      r2, r2, #32                      ; offset for new ex frame
         beq      rd2

         ; save  r0 in orig ex frame and set psp -> to it
         add      r2, r2, #12                      ; additional offset if > 4 par
rd2:     str      r0, [r2]
         msr      psp, r2                          ; psp -> orig ex frame

         ; update da flags
         and      r12, r12, #~SMX_TCB_FLAGS_DA_RUN ; clear smx_ct->flags.da_run
         and      r12, r12, #~SMX_TCB_FLAGS_G4PAR  ; clear smx_ct->flags.g4par
         orr      r12, r12, #SMX_TCB_FLAGS_DA_EXIT ; set smx_ct->flags.da_exit
         str      r12, [lr, #SMX_TCB_OFFS_FLAGS]
         mov      r1, #0
         str      r1, [lr, #SMX_TCB_OFFS_DAF]      ; clear smx_ct->daf

         ; make msp 8-byte aligned
         mrs      r0, msp
         add      r0, r0, #4
         msr      msp, r0

         ; trigger PSVH (sets PENDSVSET)                       
         ldr      r0, =ARMM_NVIC_INT_CTRL
         ldr      r1, =ARMM_FL_PENDSVSET
         str      r1, [r0]                         ; -> PSVH()
         isb
        #endif ; SMX_CFG_SSMX

         LTORG

; smx PendSV handler, PSVH(), is triggered from smx_SSRExit(), smx_SSRExitIF(),
; smx_ISR_EXIT(), smx_SchedAutoStop(), or smx_SchedAutoStopLSR(). It must be 
; entered via an exception call and exited via an exception return. It calls
; the LSR scheduler and/or task scheduler (sched). Both return to PSVH(),  
; even if current task has changed. PSVH() also runs safe LSRs and deferred 
; action functions. srnest > 1 allows interrupts to be enabled, except for a
; few short intervals.

smx_PendSV_Handler:                 ; PSVH()
         push     {lr}              ; save EXC_RETURN

        #if SMX_CFG_SSMX
         ; if sLSR has run, restore psp and end sLSR time measurement
         ldr      r1, =smx_psp_sav
         ldr      r0, [r1]          ; r0 = smx_psp_sav
         cbz      r0, psv0          ; <7>
         msr      psp, r0           ; restore psp
        #if SB_CPU_ARMM8
         ; restore PSPLIM for currnt task
         ldr      r0, =smx_ct
         ldr      r0, [r0]
         ldr      r0, [r0, #SMX_TCB_OFFS_SPP]
         msr      PSPLIM, R0
        #endif
         sb_TM_LSR                  ; end of sLSR time measurement
        #endif

         ; call LSR scheduler if smx_lqctr > 0
psv0:    sb_INT_DISABLE             ; minimize lsr latency
         ldr      r3, =smx_lqctr
         ldr      r2, [r3]

        #if SMX_CFG_SSMX
         cbz      r2, psv2          ; skip LSR scheduler if lqctr == 0
        #else
         cbz      r2, psv5          ; skip LSR scheduler if lqctr == 0
        #endif

         sub      sp, sp, #4        ; 8-byte msp alignment for call
         bl       smx_SchedRunLSRs  ; r0 > 0 if sLSR is ready to run next
         add      sp, sp, #4        ; restore msp
         
        #if SMX_CFG_SSMX
         cbz      r0, psv2          ; skip sLSR if r0 == 0
        #else
         cbz      r0, psv5          ; skip sLSR if r0 == 0
        #endif

        #if SMX_CFG_SSMX

         ; run sLSR
psv1:    sb_INT_ENABLE
         smx_RTC_LSR_START          ; start sLSR runtime/profile counters
         pop      {pc}              ; exception return to sLSR
                                    ; does not return to here <15>
         ; run deferred action function?
psv2:    sb_INT_ENABLE
         ldr      r0, =smx_ct
         ldr      r0, [r0]          ; r0 = smx_ct
         ldr      r12, [r0, #SMX_TCB_OFFS_FLAGS]
         tst      r12, #SMX_TCB_FLAGS_DA_ENTER
         beq      psv3              ; no
                                    ; yes
         ; run DAF
         BL       smx_MakeFrameDAF  ; make frame for exception call to PSVH()
         mov      lr, #0xFFFFFFFD   ; select thread mode and task stack
         push     {lr}
         mov      r1, #0
         msr      CONTROL, r1       ; select pmode
         pop      {pc}              ; exception return to smx_RunDAF()
                                    ; does not return to here <16>
         ; da_exit?
psv3:    tst      r12, #SMX_TCB_FLAGS_DA_EXIT
         beq      psv4              ; no
                                    ; yes
         ; clear da_exit flag
         and      r12, r12, #~SMX_TCB_FLAGS_DA_EXIT
         str      r12, [r0, #SMX_TCB_OFFS_FLAGS]

         ; restore psp to original value <10> 
         mrs      r1, psp
         add      r1, r1, #32
         msr      psp, r1
         b        psv13             ; bypass task scheduler

         ; did sLSR run last?
psv4:    ldr      r1, =smx_psp_sav  
         ldr      r2, [r1]
         cbz      r2, psv5          ; no
         mov      r2, #0            ; yes
         str      r2, [r1]          ; clear smx_psp_sav
         bl       mp_MPULoad        ; reload MPU for ct <8>
         ldr      r0, =smx_ct
         ldr      r0, [r0]          ; r0 = smx_ct

         ; smx_sched == NOP? <19>
         ldr      r1, =smx_sched   
         ldr      r1, [r1]
         cbnz     r1, psv5          ; no -- return to smx_SchedRunTasks()
         b        psv13             ; yes -- continue smx_ct  

        #endif ; SMX_CFG_SSMX

         ; determine sched action
psv5:    sb_INT_ENABLE
         ldr      r0, =smx_ct
         ldr      r0, [r0]          ; r0 = smx_ct
         ldr      r1, =smx_sched   
         ldr      r2, [r1]          ; r2 = sched
         tst      r2, #SMX_CT_STOP
         bne      psv7              ; smx_sched = STOP
         tst      r2, #SMX_CT_DELETE
         it       ne
         popne    {lr}
         bne      psv8              ; smx_sched = DELETE
         cbz      r2, psv13         ; bypass task scheduler

         ; is smx_ct top task? <18>
         ldr      r2, =smx_rqtop
         ldr      r2, [r2]
         ldr      r2, [r2]          ; r2 = top task
         cmp      r2, r0            ; r0 = smx_ct
         bne      psv6              ; no -- call scheduler
                                    ; yes
         ; continue smx_ct
         mov      r2, #0
         str      r2, [r1]          ; clear smx_sched
         b        psv13             ; bypass task scheduler

         ; save non-volatile registers
psv6:    ldr      r1, [r0, #SMX_TCB_OFFS_SBP] ; get ct RSA pointer
         stmia    r1, {r4-r11}

         ; save task stack pointer in smx_ct->sp and save exret
psv7:    mrs      r1, psp                       ; r1 = psp
         str      r1, [r0, #SMX_TCB_OFFS_SP]    ; smx_ct->sp = psp <13>
         pop      {lr}                          ; lr = EXC_RETURN
         strb     lr, [r0, #SMX_TCB_OFFS_EXRET] ; save last lr byte in exret

psv8:    BL       smx_SchedRunTasks   ; call task scheduler

        #if SMX_CFG_SSMX
         ; sLSR flyback?
         cbz      r0, psv9                      ; no
         mov      lr, #0xFFFFFFFD               ; yes
         push     {lr}            
         b        psv1                          
        #endif

psv9:    ldr      r0, =smx_ct
         ldr      r0, [r0]                      ; r0 = smx_ct

         ; if task start, set ct->exret = 0xFD <17>
         ldr      r1, [r0, #SMX_TCB_OFFS_SP]
         cbnz     r1, psv10
         mov      r2, #0xFD
         strb     r2, [r0, #SMX_TCB_OFFS_EXRET] 
         b        psv12                         ; skip resume code

psv10:   ; task resume

        #if SMX_CFG_SSMX
         ; ct->flags.rv_r0 == 1?
         ldr      r3, [r0, #SMX_TCB_OFFS_FLAGS] ; r3 = ct->flags
         tst      r3, #SMX_TCB_FLAGS_RV_R0
         beq      psv11                         ; no
                                                ; yes
         ; exframe r0 = smx_ct->rv and smx_ct->flags.rv_r0 = 0 <11>
         mrs      r2, psp                       ; r2 -> r0 in exframe
         ldr      r1, [r0, #SMX_TCB_OFFS_RV]    ; r1 = smx_ct->rv                      
         str      r1, [r2] 
         bic      r3, #SMX_TCB_FLAGS_RV_R0
         str      r3, [r0, #SMX_TCB_OFFS_FLAGS]
        #endif

         ; restore non-volatile registers
psv11:   ldr      r1, [r0, #SMX_TCB_OFFS_SBP]   ; get ct RSA pointer
         ldmia    r1, {r4-r11}

         ; make EXC_RETURN from ct->exret and push onto main stack 
psv12:   ldrsb    lr, [r0, #SMX_TCB_OFFS_EXRET]
         push     {lr}

psv13:   ; task scheduler bypass to here for continued tasks, deleted tasks, and DAF  
         mov      r2, #0                        ; pre-select pmode

       #if SMX_CFG_MPU_ENABLE
         ; is DAF running?
         ldr      r1, [r0, #SMX_TCB_OFFS_FLAGS] ; r1 = smx_ct->flags
         tst      r1, #SMX_TCB_FLAGS_DA_RUN
         bne      psv16                         ; yes 
                                                ; no
         ; is current task a utask?
         tst      r1, #SMX_TCB_FLAGS_UMODE
         it       ne
         movne    r2, #1                        ; pre-select umode                            
         bne      psv16                         ; yes -- leave BR ON <12>
         smx_MPU_BR_OFF                         ; no -- turn BR OFF <12>         
       #endif   ;SMX_CFG_MPU_ENABLE

psv16:   msr      CONTROL, r2                   ; select mode

         ; LSR flyback?
         sb_INT_DISABLE
         ldr      r2, =smx_lqctr
         ldr      r2, [r2]
         cbz      r2, psv17                     ; no
         sb_INT_ENABLE
         pop      {lr}                          ; yes -- adjust msp
         b        smx_PendSV_Handler            ; flyback to top of PSVH()

         ; clear smx_srnest
psv17:   mov      r2, #0
         ldr      r1, =smx_srnest
         str      r2, [r1]

        #if defined(SMX_DEBUG) || defined(SMXAWARE)
         ; clear task suspend location
         str      r2, [r0, #SMX_TCB_OFFS_SUSPLOC]
        #endif

         ldr      r1, [r0, #SMX_TCB_OFFS_SP]    ; r0 = smx_ct
         cbz      r1, psv18
         smx_EVB_LOG_TASK_RESUME
         b        psv19
psv18:   smx_EVB_LOG_TASK_START

psv19:   smx_RTC_TASK_START

         ; return to point of call via exception return <14>
         cpsid    f
         sb_INT_ENABLE
         POP      {pc}  


; smx_SVC_Handler
;  1. sp = msp in handler mode and psp -> exception stack frame.
;  2. The following code is optimized for a processor with pipelining and 0 
;     wait state SRAM.
;  3. Do not make SVC calls from handler mode <3>.

       #if SMX_CFG_SSMX
        #if SMX_CFG_DIAG
         EXTERN   smx_sst_ctr
         EXTERN   smx_svc_ctr
        #endif

smx_SVC_Handler:
         push     {lr}

        #if SMX_CFG_DIAG
         ; increment smx_svc_ctr
         ldr      r0, =smx_svc_ctr
         ldr      r1, [r0]
         add      r1, r1, #1
         str      r1, [r0]
        #endif
         ; get n from svc n in exception frame on task stack
         mrs      r0, psp        ; r0 -> ex frame
         ldr      r2, [r0, #24]
         ldrb     r2, [r2, #-2]  ; r2 = n. if invalid see <6> 
       
         ; abort if n >= sst lim
         ldr      r12, =smx_sstp 
         ldr      r12, [r12]     ; r12 -> current sst
         ldr      r1, [r12]      ; r1 = sst lim
         cmp      r2, r1
         bpl      svce

         ; if num par <= 4, skip copying parameters 5-7 
         ldr      r3, [r0, #16]  ; r3 = r12 in exception frame
         tst      r3, #1         ; r3 bit0 = >4 par flag
         beq      svc1

         ; if not deferred action function, copy parameters 5-7
         tst      r3, #2         ; r3 bit1 = temp da_enter flag
         beq      svc0

         ; set > 4 par flag in smx_ct
         ldr      lr, =smx_ct
         ldr      lr, [lr]
         ldr      r12, [lr, #SMX_TCB_OFFS_FLAGS]
         orr      r12, r12, #SMX_TCB_FLAGS_G4PAR
         str      r12, [lr, #SMX_TCB_OFFS_FLAGS]
         bne      svc1

         ; find offset from psp to location of parameter 7 in task stack
svc0:    mov      r1, #44        ; basic exception frame size -4 + 16
         ldr      r3, [r0, #28]  ; test for alignment pad present
         tst      r3, #0x200
         it       ne
         addne    r1, #4         ; add alignment pad
         tst      lr, #0x10      ; test for floating point registers present
         it       eq
         addeq    r1, #72        ; add floating point registers size

         ; copy par7 or dummy parameter from task stack to main stack
         ldr      r3, [r0, r1]
         str      r3, [sp, #-4]!

         ; copy par6 or dummy parameter above par7
         sub      r1, r1, #4
         ldr      r3, [r0, r1]
         str      r3, [sp, #-4]!

         ; copy par5 from task stack above par6
         sub      r1, r1, #4
         ldr      r3, [r0, r1]
         str      r3, [sp, #-4]!

         ; save ex frame pointer in top of task stack
         mrs      r3, psp
         str      r0, [r3, #-4]   

svc1:
        #if SMX_CFG_DIAG
         ; increment system service counter if smx_sstp == smx_sst
         ; (using main system service table not custom sst)
         ldr      r1, =smx_sstp
         ldr      r1, [r1]             ; r1 = smx_sstp
         ldr      lr, =smx_sst         ; lr = smx_sst addr
         cmp      r1, lr
         bne      svc2                 ; smx_sstp != smx_sst so skip counting
         ldr      r1, =smx_sst_ctr
         ldr      r1, [r1]             ; r1 -> first slot in counter array
         ldr      lr, [r1, r2, LSL#2]  ; read counter at slot n
         add      lr, lr, #1
         str      lr, [r1, r2, LSL#2]
svc2:
        #endif
         ; fetch service address
         ldr      r12, =smx_sstp 
         ldr      r12, [r12]           ; r12 -> current sst
         ldr      lr, [r12, r2, LSL#2] ; lr -> service (r2 = n)

         ; if deferred action function load n into r0
         tst      r3, #2         ; r3 bit1 = temp da_enter flag
         beq      svc3
         mov      r0, r2         ; r0 = n
         b        svc4

         ; load r0-r3 from exception stack frame <4>
svc3:    mov      r12, r0              ; r0 -> exception frame
         ldmia    r12, {r0-r3}

svc4:    BLX      LR    ; CALL SERVICE

         mrs      lr, psp              ; lr -> ex frame

         ; if deferred action function, skip saving r0 in exception frame
         ; and adjusting main stack
         ldr      r1, =smx_ct
         ldr      r1, [r1]
         ldr      r2, [r1, #SMX_TCB_OFFS_FLAGS]         
         tst      r2, #SMX_TCB_FLAGS_DA_ENTER
         bne      svc5 

         ; put return value in top of stack frame
         str      r0, [lr]

         ; adjust main stack if par 5-7 were loaded into it
         ldr      r12, [lr, #16]
         tst      r12, #1
         it       ne
         addne    sp, sp, #12
svc5:
        #if defined(SMX_DEBUG) || defined(SMXAWARE)
         ; Save exception frame lr in smx_ct->susploc <5>
         ldr      r12, [lr, #20]
         ldr      lr, =smx_ct
         ldr      lr, [lr]
         str      r12, [lr, #SMX_TCB_OFFS_SUSPLOC]
        #endif

         ; trigger PSVH() if lqctr > 0 <20>
         sb_INT_DISABLE
         ldr      r1, =smx_lqctr
         ldr      r1, [r1]
         cbz      r1, svc6
         mov      r2, #1
         ldr      r1, =smx_srnest
         str      r2, [r1]
         ldr      r0, =ARMM_NVIC_INT_CTRL 
         ldr      r1, =ARMM_FL_PENDSVSET
         str      r1, [r0]

svc6:    cpsid    f
         sb_INT_ENABLE
         POP      {PC}     ; return or tail-chain to PSVH() via EXC_RETURN

         ; abort because n is too large
svce:    mov      r0, #SMXE_PRIV_VIOL  
         mov      r1, #0
         bl       smx_EM
         pop      {pc}

       #else 
; EM SVC Handler.
smx_SVC_Handler:
         push     {lr}
         bl       smx_EM
         pop      {pc}
       #endif ; SMX_CFG_SSMX

; usage fault due to program error
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
;  1. Abbreviations: 
;        hmode    handler mode
;        pmode    privileged mode
;        umode    unprivileged mode
;        PSVH()   smx_PendSV_Handler()
;        SVCH()   smx_SVC_Handler()
;        sched    smx_SchedRunTasks()
;  2. When a safe LSR runs through its last } smx_SchedAutoStopLSR() is called 
;     if in pmode (pLSR), or smxu_SchedAutoStopLSR() is called if in umode 
;     (uLSR). This is determined by the exception frame used to run the LSR.
;     smxu_SchedAutoStopLSR() invokes svc LS, which calls smx_SchedAutoStopLSR()
;     via smx_sst[LS]. It triggers PSVH(), so control returns to PSVH() top, 
;     where psp is restored to the value before the sLSR ran and time 
;     measurement for the sLSR is recorded.
;  3. SVC_Handler, SVCH(), must not be called from hmode. This applies to
;     exception handlers, ISRs, and tLSRs. If #include "xapiu.h" is above,
;     add #include "xapip.h" before them. It is ok to call SVCH() from  
;     pmode tasks.
;  4. Volatile registers must be loaded from the stack frame because the SVC
;     handler could have been interrupted by a higher-priority exception which 
;     changed them.
;  5. ct does not actually wait prior to function return, so it is ok to save
;     susploc after the service return.
;  6. If n is invalid, see Note 3.
;  7. If r0 != 0 an sLSR ran last. If r0 == 0, a tLSR ran last.
;  8. If an sLSR ran last and smx_ct is continuing, its MPA must be reloaded 
;     into the MPU. 
;  9. The exception frame must be reloaded each time an sLSR is called because 
;     after an exception call, psp is at the bottom of the frame and the 
;     exception overwrites it.
; 10. psp -> exception frame made for transisition to PSVH() from RunDAF().
; 11. When ct is suspended via a direct call to an SSR and then is resumed by
;     another SSR, it returns to smx_SSRExit(), which calls GetCTRV() to 
;     return task->rv. But when ct is suspended by an SSR call via SVCH() and  
;     then is resumed, it does not return to smx_SSRExit(), but rather to the 
;     point of call via a PSVH() exception return. To deal with this,  
;     ct->flags.rv_ro is set in the SSRs that cause ct to suspend. Then, when
;     ct resumes, if ct->flags.rv_ro is set, the PSVH() tail loads
;     ct->rv into the r0 position of the exception frame created by the SVC 
;     exception. Thus r0 = task->rv following exception return.
; 12. BR must be off for ptasks, else MPU would not limit accesses. Since BR 
;     is off, sys_code and sys_data must be in the MPU to handle exceptions.
;     However if smx_ct is a utask, sys_code and sys_data may not be in the 
;     MPU, so smx_MPULoad() called by smx_SchedRunTasks() turns BR on.
; 13. Enables sched to test psp.
; 14. Since FAULTMASK is cleared on exception exit, the following results
;     in keeping interrupts disabled until after exception exit.
; 15. No further execution after this point. When sLSR is done, PSVH() 
;     exception is triggered and execution returns to beginning of PSVH().
; 16. No further execution after this point. When smx_RunDAF() is done
;     running the deferred action function, PSVH() exception is triggered and
;     execution returns to beginning of PSVH().
; 17. It is not necessary to restore floating point registers if a task is 
;     just starting.
; 18. If smx_sched == SMX_CT_TEST and smx_ct is still top task, sched is 
;     skipped to improve performance. If smx_sched == SMX_CT_SUSP, smx_ct is 
;     not in smx_rq so it cannot be the top task so sched is called. 
; 19. If smx_sched == NOP, no scheduler action is necessary, however smx_ct
;     must go through the PSVH() tail processing that it missed due to the sLSR 
;     flyback. If smx_sched != NOP, scheduler action is necessary, however
;     smx_ct has already gone through PSVH() head processing.
; 20. This is necessary if PSVH() has not already been triggered, because in 
;     that case SVCH() would return to the point of call, and waiting LSRs
;     would not run.