;
; xarmm_iar.inc  (IAR version)                              Version 6.0.0
;
; smx Definitions, Globals, and Macros for ARM-M assembly files.
; Minimal subset of the definitions in C header files.
;
; Copyright (c) 1998-2026 Micro Digital Inc.
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

#include "barmm_iar.inc"

SMX                     EQU   1
SMX_VERSION             EQU   0x0600  ; Change xdef.h and string in scheduler too.

;============================================================================;
;                             SMX CONFIGURATION                              ;
;============================================================================;

; NOTE: Set to match definitions in bcfg.h and xcfg.h. See explanations there.

SB_CFG_TM               EQU   1
SMX_CFG_DIAG            EQU   1
SMX_CFG_EVB             EQU   1
SMX_CFG_PROFILE         EQU   1

SMX_CFG_SSMX            EQU   1

#if SMX_CFG_SSMX
SMX_CFG_MPU_ENABLE      EQU   1
SMX_CFG_MPU_BR_EN       EQU   1
SMX_CFG_RTLIM           EQU   1
#else
SMX_CFG_MPU_ENABLE      EQU   0
SMX_CFG_MPU_BR_EN       EQU   0
SMX_CFG_RTLIM           EQU   0
#endif

;============================================================================;
;                                 CONSTANTS                                  ;
;============================================================================;

; NOTE: These must match corresponding definitions in xdef.h and xtypes.h.

SMX_CT_DELETE           EQU   0x100
SMX_CT_STOP             EQU   0x80

SMX_LCB_OFFS_FLAGS      EQU    8     ; actually byte 9 but flag values skip low byte (cbtype)
SMX_LCB_OFFS_SBP        EQU   24
SMX_LCB_FLAGS_UMODE     EQU   0x0200 ; byte 1 flags, byte 0 cbtype

SMX_TCB_OFFS_EXRET      EQU   19
SMX_TCB_OFFS_FLAGS      EQU   20
SMX_TCB_OFFS_SPP        EQU   24
SMX_TCB_OFFS_SP         EQU   32
SMX_TCB_OFFS_SBP        EQU   36
SMX_TCB_OFFS_RV         EQU   44
SMX_TCB_OFFS_FUN        EQU   56
SMX_TCB_OFFS_THISPTR    EQU   68
SMX_TCB_OFFS_SUSPLOC    EQU   76
SMX_TCB_OFFS_DAF        EQU   120

SMX_TCB_FLAGS_RV_R0     EQU   0x00000100
SMX_TCB_FLAGS_UMODE     EQU   0x00010000
SMX_TCB_FLAGS_G4PAR     EQU   0x00040000
SMX_TCB_FLAGS_DA_ENTER  EQU   0x00080000
SMX_TCB_FLAGS_DA_RUN    EQU   0x00100000
SMX_TCB_FLAGS_DA_EXIT   EQU   0x00200000

; Errors
SMXE_PRIV_VIOL          EQU   2

; Following must agree with mparmm.h
#if (defined(SMX_TSMX) && SB_CPU_ARMM7)
MP_MPU_ACTVSZ           EQU   6  /* TSMX setting; don't change */
MP_MPU_STATSZ           EQU   2  /* TSMX setting; don't change */
#else
MP_MPU_ACTVSZ           EQU   8
MP_MPU_STATSZ           EQU   0
#endif


;============================================================================;
;                                   MACROS                                   ;
;============================================================================;

#if SMX_CFG_SSMX && SMX_CFG_MPU_ENABLE
smx_MPU_BR_ON: MACRO
         MOVS     R0,#5
         LDR.N    R1,=ARMM_MPU_CTRL
         STR      R0,[R1]
         DSB      
         ISB
         ENDM
 
smx_MPU_BR_OFF: MACRO
         DMB      
         MOVS     R0,#1
         LDR.N    R1,=ARMM_MPU_CTRL
         STR      R0,[R1, #0]
         DSB      
         ISB
         ENDM
#else
smx_MPU_BR_ON: MACRO
         ENDM
smx_MPU_BR_OFF: MACRO
         ENDM
#endif

#if SMX_CFG_PROFILE || SMX_CFG_RTLIM
smx_RTC_TASK_START_ID: MACRO
         BL       smx_RTC_TaskStartID
         ENDM
smx_RTC_LSR_END: MACRO
         BL       smx_RTC_LSREnd
         ENDM
#else
smx_RTC_TASK_START_ID: MACRO
         ENDM
smx_RTC_LSR_END: MACRO
         ENDM
#endif

; Notes:
;  1. Normally, BR is on in umode so that an ISR can access sys_code and
;     sys_data. However, if the sys_code and sys_data are in static MPU slots,
;     BR can be off in umode. Note BR has no effect in umode, but does in ISRs, 
;     which run in hmode.
