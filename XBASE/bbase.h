/*
* bbase.h                                                   Version 5.4.0
*
* smxBase Master Include File
*
* Copyright (c) 2009-2025 Micro Digital Inc.
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

