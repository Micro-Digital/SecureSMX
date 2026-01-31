/*
* cpmapn.h                                                  Version 6.0.0
*
* Console Function to Portal Shell Function Unmapping File.
*
* Copyright (c) 2023-2026 Micro Digital Inc.
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
* Author: David Moore
*
*****************************************************************************/

#ifndef SMX_CPMAPN_H
#define SMX_CPMAPN_H

#if CP_PORTAL

/*===========================================================================*
*                         sb to sbp Unmapping Macros                         *
*===========================================================================*/

#if (SB_CFG_CON)
#undef sb_ConClearScreen
#undef sb_ConClearScreenUnp
#undef sb_ConDbgMsgMode
#undef sb_ConDbgMsgModeSet
#undef sb_ConPutChar
#undef sb_ConWriteChar
#undef sb_ConWriteString
#undef sb_ConWriteStringNum
#endif

#undef SMX_CPMAP_H /* allow cpmap.h to be included again */
#endif /* CP_PORTAL */
#endif /* SMX_CPMAPN_H */