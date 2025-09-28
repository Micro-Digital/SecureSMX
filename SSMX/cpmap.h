/*
* cpmap.h                                                   Version 5.4.0
*
* Console Function to Portal Shell Function Map File.
*
* Copyright (c) 2019-2025 Micro Digital Inc.
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
*****************************************************************************/

#ifndef SMX_CPMAP_H
#define SMX_CPMAP_H

#if CP_PORTAL

/* Note: The CP_PCH macro permits adding a needed parameter to the portal
   versions of console services that is not in the non-portal versions. It must 
   be defined ahead of console service calls if CP_PORTAL is true. Can #undef 
   and #define it again to change it for a section of code that uses a 
   different portal client structure.
*/
/*===========================================================================*
*                          sb to sbp Mapping Macros                          *
*===========================================================================*/

#if (SB_CFG_CON)
#define sb_ConClearScreen(void)     sbp_ConClearScreen(CP_PCH)
#define sb_ConClearScreenUnp(void)  sbp_ConClearScreenUnp(CP_PCH)
#define sb_ConDbgMsgMode(void)      sbp_ConDbgMsgMode(CP_PCH)
#define sb_ConDbgMsgModeSet(enable) sbp_ConDbgMsgModeSet(enable, CP_PCH)
#define sb_ConPutChar(ch)           sbp_ConPutChar(ch, CP_PCH)

#define sb_ConWriteChar(col, row, F_color, B_color, blink, ch) \
               sbp_ConWriteChar(col, row, F_color, B_color, blink, ch, CP_PCH)
#define sb_ConWriteString(col, row, F_color, B_color, blink, in_string) \
               sbp_ConWriteString(col, row, F_color, B_color, blink, in_string, CP_PCH)
#define sb_ConWriteStringNum(col, row, F_color, B_color, blink, in_string, num) \
               sbp_ConWriteStringNum(col, row, F_color, B_color, blink, in_string, num, CP_PCH)
#endif

#undef SMX_CPMAPN_H /* allow cpmapn.h to be included again */
#endif /* CP_PORTAL */
#endif /* SMX_CPMAP_H */