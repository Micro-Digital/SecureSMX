;
; barmm_iar.inc  (IAR version)                              Version 5.4.0
;
; ARMM (e.g. Cortex-M) definitions and macros.
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
; Author: David Moore
;
;*****************************************************************************

SB_ARMM_DISABLE_WITH_BASEPRI  equ   0           ; Change barmm.h too.
SB_ARMM_BASEPRI_VALUE         equ   0x20        ; Change barmm.h too.

ARMM_FL_PENDSVSET             equ   0x10000000
ARMM_MPU_CTRL                 equ   0xE000ED94
ARMM_NVIC_INT_CTRL            equ   0xE000ED04

sb_INT_DISABLE MACRO
         #if SB_ARMM_DISABLE_WITH_BASEPRI       ; disable interrupts
         mov      r2, #SB_ARMM_BASEPRI_VALUE    ; CAUTION: changes r2
         msr      BASEPRI, r2                   ; BASEPRI = SB_ARMM_BASEPRI_VALUE
         #else
         cpsid    i                             ; PRIMASK = 1
         #endif
         ENDM

sb_INT_ENABLE MACRO
         #if SB_ARMM_DISABLE_WITH_BASEPRI
         mov      r2, #0                        ; CAUTION: changes r2
         msr      BASEPRI, r2                   ; BASEPRI = 0 no interrupts masked
         #else
         cpsie    i                             ; PRIMASK = 0 interrupts enabled
         #endif
         ENDM

