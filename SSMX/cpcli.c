/*
* cpcli.c                                                   Version 6.0.0
*
* Console Portal Client File.
*
* Copyright (c) 2019-2026 Micro Digital Inc.
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
*****************************************************************************/

#include "xsmx.h"
#include "xapiu.h"

#if CP_PORTAL

#if SMX_CFG_SSMX
#pragma diag_suppress=Ta168 /* ignore warning that next line overrides existing prefix */
#pragma section_prefix = ".ucom"
#endif

/*===========================================================================*
*                          Console Portal Shells                             *
*===========================================================================*/

void sbp_ConClearScreen(FPCS* pch)
{
   mp_PTL_CALLER_SAV();
   CPSH* shp;
   MCB*  pmsg;

   if (pmsg = mp_FPortalReceive(pch, (u8**)&shp))
   {
      mp_SHL0(CP_CLR_SCREEN, 0);
      mp_FPortalSend(pch, pmsg);
   }
}

void sbp_ConClearScreenUnp(FPCS* pch)
{
   mp_PTL_CALLER_SAV();
   CPSH* shp;
   MCB*  pmsg;

   if (pmsg = mp_FPortalReceive(pch, (u8**)&shp))
   {
      mp_SHL0(CP_CLR_SCREEN_UNP, 0);
      mp_FPortalSend(pch, pmsg);
   }
}

bool sbp_ConDbgMsgMode(FPCS* pch)
{
   mp_PTL_CALLER_SAV();
   CPSH* shp;
   MCB*  pmsg;
   bool  mode = false;

   if (pmsg = mp_FPortalReceive(pch, (u8**)&shp))
   {
      mp_SHL0(CP_DBG_MODE, 0);
      mp_FPortalSend(pch, pmsg);
      pmsg = mp_FPortalReceive(pch, (u8**)&shp);
      mode = shp->ret;
      smx_PMsgReply(pmsg);
   }
   return(mode);
}

void sbp_ConDbgMsgModeSet(bool enable, FPCS* pch)
{
   mp_PTL_CALLER_SAV();
   CPSH* shp;
   MCB*  pmsg;

   if (pmsg = mp_FPortalReceive(pch, (u8**)&shp))
   {
      mp_SHL1(CP_DBG_MODE_SET, enable, 0);
      mp_FPortalSend(pch, pmsg);
   }
}

int sbp_ConPutChar(char ch, FPCS* pch)
{
   mp_PTL_CALLER_SAV();
   CPSH* shp;
   MCB*  pmsg;

   if (pmsg = mp_FPortalReceive(pch, (u8**)&shp))
   {
      mp_SHL1(CP_PUT_CHAR, ch, 0);
      mp_FPortalSend(pch, pmsg);
   }
   return(ch);
}

void sbp_ConWriteChar(u32 col, u32 row, u32 F_color, u32 B_color, u32 blink, 
                                                            char ch, FPCS* pch)
{
   mp_PTL_CALLER_SAV();
   CPSH* shp;
   MCB*  pmsg;
   u32   p1;

   if (pmsg = mp_FPortalReceive(pch, (u8**)&shp))
   {
      p1 = (col<<24) + (row<<16) + (F_color<<8) + B_color;
      p1 |= blink > 0 ? 0x80000000 : 0;
      mp_SHL2(CP_WRITE_CHAR, p1, ch, 0);
      mp_FPortalSend(pch, pmsg);
   }
}

void sbp_ConWriteString(u32 col, u32 row, u32 F_color, u32 B_color,
                                   u32 blink, const char *in_string, FPCS* pch)
{
   mp_PTL_CALLER_SAV();
   char* dp;
   CPSH* shp;
   MCB*  pmsg;
   u32   p1;

   if (pmsg = mp_FPortalReceive(pch, (u8**)&shp))
   {
      p1 = (col<<24) + (row<<16) + (F_color<<8) + B_color;
      p1 |= blink > 0 ? 0x80000000 : 0;
      mp_SHL1(CP_WRITE_STRING, p1, 0);
      dp = (char*)shp + sizeof(CPSH);
      strcpy(dp, in_string);
      shp->dp = (u32)dp;
      mp_FPortalSend(pch, pmsg);
   }
}

void sbp_ConWriteStringNum(u32 col, u32 row, u32 F_color, u32 B_color,
                           u32 blink, const char *in_string, u32 num, FPCS* pch)
{
   mp_PTL_CALLER_SAV();
   char* dp;
   CPSH* shp;
   MCB*  pmsg;
   u32   p1;

   if (pmsg = mp_FPortalReceive(pch, (u8**)&shp))
   {
      p1 = (col<<24) + (row<<16) + (F_color<<8) + B_color;
      p1 |= blink > 0 ? 0x80000000 : 0;
      mp_SHL2(CP_WRITE_STRING_NUM, p1, num, 0);
      dp = (char*)shp + sizeof(CPSH);
      strcpy(dp, in_string);
      shp->dp = (u32)dp;
      mp_FPortalSend(pch, pmsg);
   }
}
#endif /* CP_PORTAL */