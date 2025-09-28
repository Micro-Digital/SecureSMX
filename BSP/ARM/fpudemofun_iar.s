;
; fpudemofun.s                                              Version 5.4.0
;
; IAR cannot support to accessing FPU registers, so these two assembly
; routines were created to read and write FPU s1-s31 registers.
;
; Copyright (c) 2010-2025 Micro Digital Inc.
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
; Author: Xingsheng Wan
;
;*****************************************************************************

         RSEG     CODE:CODE:NOROOT(2)
         CODE32

         PUBLIC   read_fpu_registers
         PUBLIC   write_fpu_registers
         PUBLIC   clear_fpu_registers

clear_fpu_registers:
         MOV      R2, LR
         MOV      R0, #0
         VMOV.32  S31, R0
         BL       write_fpu_registers
         BX       R2

write_fpu_registers:
         VMOV.32  R1, S31          ; Get last value

         ADD      R1, R1, R0
         VMOV.32  S0, R1
         ADD      R1, R1, R0
         VMOV.32  S1, R1
         ADD      R1, R1, R0
         VMOV.32  S2, R1
         ADD      R1, R1, R0
         VMOV.32  S3, R1
         ADD      R1, R1, R0
         VMOV.32  S4, R1
         ADD      R1, R1, R0
         VMOV.32  S5, R1
         ADD      R1, R1, R0
         VMOV.32  S6, R1
         ADD      R1, R1, R0
         VMOV.32  S7, R1
         ADD      R1, R1, R0
         VMOV.32  S8, R1
         ADD      R1, R1, R0
         VMOV.32  S9, R1
         ADD      R1, R1, R0
         VMOV.32  S10, R1
         ADD      R1, R1, R0
         VMOV.32  S11, R1
         ADD      R1, R1, R0
         VMOV.32  S12, R1
         ADD      R1, R1, R0
         VMOV.32  S13, R1
         ADD      R1, R1, R0
         VMOV.32  S14, R1
         ADD      R1, R1, R0
         VMOV.32  S15, R1
         ADD      R1, R1, R0
         VMOV.32  S16, R1
         ADD      R1, R1, R0
         VMOV.32  S17, R1
         ADD      R1, R1, R0
         VMOV.32  S18, R1
         ADD      R1, R1, R0
         VMOV.32  S19, R1
         ADD      R1, R1, R0
         VMOV.32  S20, R1
         ADD      R1, R1, R0
         VMOV.32  S21, R1
         ADD      R1, R1, R0
         VMOV.32  S22, R1
         ADD      R1, R1, R0
         VMOV.32  S23, R1
         ADD      R1, R1, R0
         VMOV.32  S24, R1
         ADD      R1, R1, R0
         VMOV.32  S25, R1
         ADD      R1, R1, R0
         VMOV.32  S26, R1
         ADD      R1, R1, R0
         VMOV.32  S27, R1
         ADD      R1, R1, R0
         VMOV.32  S28, R1
         ADD      R1, R1, R0
         VMOV.32  S29, R1
         ADD      R1, R1, R0
         VMOV.32  S30, R1
         ADD      R1, R1, R0
         VMOV.32  S31, R1

         BX       LR

read_fpu_registers:
         VMOV.32  R1, S0
         STR      R1, [R0]
         VMOV.32  R1, S1
         STR      R1, [R0, #4]
         VMOV.32  R1, S2
         STR      R1, [R0, #8]
         VMOV.32  R1, S3
         STR      R1, [R0, #12]
         VMOV.32  R1, S4
         STR      R1, [R0, #16]
         VMOV.32  R1, S5
         STR      R1, [R0, #20]
         VMOV.32  R1, S6
         STR      R1, [R0, #24]
         VMOV.32  R1, S7
         STR      R1, [R0, #28]
         VMOV.32  R1, S8
         STR      R1, [R0, #32]
         VMOV.32  R1, S9
         STR      R1, [R0, #36]
         VMOV.32  R1, S10
         STR      R1, [R0, #40]
         VMOV.32  R1, S11
         STR      R1, [R0, #44]
         VMOV.32  R1, S12
         STR      R1, [R0, #48]
         VMOV.32  R1, S13
         STR      R1, [R0, #52]
         VMOV.32  R1, S14
         STR      R1, [R0, #56]
         VMOV.32  R1, S15
         STR      R1, [R0, #60]
         VMOV.32  R1, S16
         STR      R1, [R0, #64]
         VMOV.32  R1, S17
         STR      R1, [R0, #68]
         VMOV.32  R1, S18
         STR      R1, [R0, #72]
         VMOV.32  R1, S19
         STR      R1, [R0, #76]
         VMOV.32  R1, S20
         STR      R1, [R0, #80]
         VMOV.32  R1, S21
         STR      R1, [R0, #84]
         VMOV.32  R1, S22
         STR      R1, [R0, #88]
         VMOV.32  R1, S23
         STR      R1, [R0, #92]
         VMOV.32  R1, S24
         STR      R1, [R0, #96]
         VMOV.32  R1, S25
         STR      R1, [R0, #100]
         VMOV.32  R1, S26
         STR      R1, [R0, #104]
         VMOV.32  R1, S27
         STR      R1, [R0, #108]
         VMOV.32  R1, S28
         STR      R1, [R0, #112]
         VMOV.32  R1, S29
         STR      R1, [R0, #116]
         VMOV.32  R1, S30
         STR      R1, [R0, #120]
         VMOV.32  R1, S31
         STR      R1, [R0, #124]
         VSTM.32  R0!,{s0-s31}
         BX       LR

         END

