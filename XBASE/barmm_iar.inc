;
; barmm_iar.inc  (IAR version)                              Version 6.0.0
;
; ARMM (e.g. Cortex-M) definitions and macros.
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
; Author: David Moore
;
;*****************************************************************************

SB_ARMM_DISABLE_WITH_BASEPRI  equ   0           ; Change barmm.h too.
SB_ARMM_BASEPRI_VALUE         equ   0x20        ; Change barmm.h too.

ARMM_FL_PENDSVSET             equ   0x10000000
ARMM_MPU_CTRL                 equ   0xE000ED94
ARMM_MPU_RNR                  equ   0xE000ED98
ARMM_MPU_RBAR                 equ   0xE000ED9C
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

