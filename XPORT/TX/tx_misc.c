/*
* tx_misc.c                                                 Version 5.4.0
*
* TXPort Miscellaneous Functions
*
* Copyright (c) 1994-2025 Micro Digital Inc.
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
      