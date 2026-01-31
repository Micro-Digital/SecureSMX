;
; fpudemofunm.s                                             Version 6.0.0
;
; IAR cannot support to accessing FPU registers, so these two assembly
; routines were created to read and write FPU s1-s31 registers.
;
; Copyright (c) 2010-2026 Micro Digital Inc.
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
; Author: Xingsheng Wan
;
;*****************************************************************************

         #include "xarmm_iar.inc"

FPU_NUM_REGS      EQU   32    ; normal case
;FPU_NUM_REGS     EQU   16    ; for CM4/M7 auto save if no sw save of s16-s31

#if (SMX_CFG_SSMX)
         SECTION  `.fpu.text`:CODE:NOROOT(2)
#else
         SECTION  CODE:CODE:NOROOT(2)
#endif
         THUMB

         PUBLIC   read_fpu_registers
         PUBLIC   write_fpu_registers
         PUBLIC   clear_fpu_registers

clear_fpu_registers:
         MOV      R2, LR
         MOV      R0, #0
#if (FPU_NUM_REGS == 16)
         VMOV     S15, R0
#else
         VMOV     S31, R0
#endif
         BL       write_fpu_registers
         BX       R2

write_fpu_registers:
#if (FPU_NUM_REGS == 16)
         VMOV     R1, S15        ; Get last value
#else
         VMOV     R1, S31        ; Get last value
#endif
         ADD      R1, R1, R0
         VMOV     S0, R1
         ADD      R1, R1, R0
         VMOV     S1, R1
         ADD      R1, R1, R0
         VMOV     S2, R1
         ADD      R1, R1, R0
         VMOV     S3, R1
         ADD      R1, R1, R0
         VMOV     S4, R1
         ADD      R1, R1, R0
         VMOV     S5, R1
         ADD      R1, R1, R0
         VMOV     S6, R1
         ADD      R1, R1, R0
         VMOV     S7, R1
         ADD      R1, R1, R0
         VMOV     S8, R1
         ADD      R1, R1, R0
         VMOV     S9, R1
         ADD      R1, R1, R0
         VMOV     S10, R1
         ADD      R1, R1, R0
         VMOV     S11, R1
         ADD      R1, R1, R0
         VMOV     S12, R1
         ADD      R1, R1, R0
         VMOV     S13, R1
         ADD      R1, R1, R0
         VMOV     S14, R1
         ADD      R1, R1, R0
         VMOV     S15, R1
         ADD      R1, R1, R0
#if (FPU_NUM_REGS != 16)
         VMOV     S16, R1
         ADD      R1, R1, R0
         VMOV     S17, R1
         ADD      R1, R1, R0
         VMOV     S18, R1
         ADD      R1, R1, R0
         VMOV     S19, R1
         ADD      R1, R1, R0
         VMOV     S20, R1
         ADD      R1, R1, R0
         VMOV     S21, R1
         ADD      R1, R1, R0
         VMOV     S22, R1
         ADD      R1, R1, R0
         VMOV     S23, R1
         ADD      R1, R1, R0
         VMOV     S24, R1
         ADD      R1, R1, R0
         VMOV     S25, R1
         ADD      R1, R1, R0
         VMOV     S26, R1
         ADD      R1, R1, R0
         VMOV     S27, R1
         ADD      R1, R1, R0
         VMOV     S28, R1
         ADD      R1, R1, R0
         VMOV     S29, R1
         ADD      R1, R1, R0
         VMOV     S30, R1
         ADD      R1, R1, R0
         VMOV     S31, R1
#endif
         BX       LR

read_fpu_registers:
         VMOV     R1, S0
         STR      R1, [R0]
         VMOV     R1, S1
         STR      R1, [R0, #4]
         VMOV     R1, S2
         STR      R1, [R0, #8]
         VMOV     R1, S3
         STR      R1, [R0, #12]
         VMOV     R1, S4
         STR      R1, [R0, #16]
         VMOV     R1, S5
         STR      R1, [R0, #20]
         VMOV     R1, S6
         STR      R1, [R0, #24]
         VMOV     R1, S7
         STR      R1, [R0, #28]
         VMOV     R1, S8
         STR      R1, [R0, #32]
         VMOV     R1, S9
         STR      R1, [R0, #36]
         VMOV     R1, S10
         STR      R1, [R0, #40]
         VMOV     R1, S11
         STR      R1, [R0, #44]
         VMOV     R1, S12
         STR      R1, [R0, #48]
         VMOV     R1, S13
         STR      R1, [R0, #52]
         VMOV     R1, S14
         STR      R1, [R0, #56]
         VMOV     R1, S15
         STR      R1, [R0, #60]
#if (FPU_NUM_REGS != 16)
         VMOV     R1, S16
         STR      R1, [R0, #64]
         VMOV     R1, S17
         STR      R1, [R0, #68]
         VMOV     R1, S18
         STR      R1, [R0, #72]
         VMOV     R1, S19
         STR      R1, [R0, #76]
         VMOV     R1, S20
         STR      R1, [R0, #80]
         VMOV     R1, S21
         STR      R1, [R0, #84]
         VMOV     R1, S22
         STR      R1, [R0, #88]
         VMOV     R1, S23
         STR      R1, [R0, #92]
         VMOV     R1, S24
         STR      R1, [R0, #96]
         VMOV     R1, S25
         STR      R1, [R0, #100]
         VMOV     R1, S26
         STR      R1, [R0, #104]
         VMOV     R1, S27
         STR      R1, [R0, #108]
         VMOV     R1, S28
         STR      R1, [R0, #112]
         VMOV     R1, S29
         STR      R1, [R0, #116]
         VMOV     R1, S30
         STR      R1, [R0, #120]
         VMOV     R1, S31
         STR      R1, [R0, #124]
#endif
#if (FPU_NUM_REGS == 16)
         VSTM.32  R0!,{s0-s15}
#else
         VSTM.32  R0!,{s0-s31}
#endif
         BX       LR

         END
