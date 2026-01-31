/*
* bbase.h                                                   Version 6.0.0
*
* smxBase Master Include File
*
* Copyright (c) 2009-2026 Micro Digital Inc.
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

#ifndef  SB_BBASE_H
#define  SB_BBASE_H

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "bcfg.h"          /* base configuration */
#include "xcfg.h"          /* smx configuration */

#if defined(SMX_FRPORT_TEST)
#include "frcfg.h"         /* frport test configuration */
#elif defined(SMX_TXPORT_TEST)
#include "txcfg.h"         /* txport test configuration */
#elif defined (SMX_TSMX)
#include "tcfg.h"          /* tsmx configuration */
#else
#include "acfg.h"          /* application configuration */
#endif

#include "bdef.h"          /* definitions */
#include "barmm.h"         /* ARMM macros and definitions */
#include "bapi.h"          /* API */

#if SMX_CFG_SSMX
#include "mparmm.h"        /* MPU ARMM definitions */
#include "mpudef.h"        /* MPU definitions */
#endif

#include "smx.h"

#endif /* SB_BBASE_H */

