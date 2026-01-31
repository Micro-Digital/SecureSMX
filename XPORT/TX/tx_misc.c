/*
* tx_misc.c                                                 Version 6.0.0
*
* TXPort Miscellaneous Functions
*
* Copyright (c) 2024-2026 Micro Digital Inc.
* All rights reserved. www.smxrtos.com
*
* SPDX-License-Identifier: GPL-2.0-only OR LicenseRef-MDI-Commercial
*
* This software, documentation, and accompanying materials are made available
* under a dual license, either GPLv2 or Commercial. You may not use this file
* except in compliance with either License. GPLv2 is at www.gnu.org/licenses.
* It does not permit the incorporation of this code into proprietary programs.
*
* Commercial license and support services are available from Micro Digital.
* Inquire at support@smxrtos.com.
*
* This Work embodies patents listed in smx.h. A patent license is hereby
* granted to use these patents in this Work and Derivative Works, except in
* another RTOS or OS.
*
* This entire comment block must be preserved in all copies of this file.
*
* Author: Ralph Moore
*
* For ThreadX/Azure Kernel V6.1
*
*****************************************************************************/

#include "xsmx.h"
#include "tx_api.h"

UINT tx_interrupt_control(UINT new_posture)
{
#if SB_ARMM_DISABLE_WITH_BASEPRI
UINT  prev_state = __get_BASEPRI();
#else
UINT  prev_state = __get_PRIMASK();
#endif

   if (new_posture == TX_INT_DISABLE)
      sb_INT_DISABLE();
   if (new_posture == TX_INT_ENABLE)
      sb_INT_ENABLE();

   return prev_state;
}

ULONG  tx_time_get(VOID)
{
   return smx_etime;
}

VOID  tx_time_set(ULONG new_time)
{
   smx_etime = new_time;
}
      