/*
* cpmapn.h                                                  Version 5.3.0
*
* Console Function to Portal Shell Function Unmapping File.
*
* Copyright (c) 2023-2024 Micro Digital Inc.
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