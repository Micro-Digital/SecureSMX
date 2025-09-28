/*
* fportlc.c                                                 Version 5.4.0
*
* Free message portal client functions and objects.
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

#include "xsmx.h"

#if SMX_CFG_PORTAL

/*
*  mp_FPortalClose()
*
*  Closes a free message portal for a client. If rxchg, releases all pmsgs 
*  owned by portal. Waits up to pch->tmo ticks for each pmsg. Aborts if timeout 
*  and returns false. Then deletes rxchg. Clears FPCS after handles. 
*  xsn is slot number used by deleting task -- see mp FPortalDelete().
*/
bool mp_FPortalClose(FPCS* pch, u8 xsn)
{
   MCB*     pmsg;
   u8       sn, dsn;

   mp_PORTAL_LOG2(MP_ID_FPORTAL_CLOSE, (u32)pch, xsn);

   sn = (xsn == 0) ? pch->csn : xsn;

   /* release pmsgs and delete rxchg if created by mp_FPortalOpen() */
   if (pch->num)
   {
      if (pch->pmsg != NULL)
      {
         /* release pmsg held by portal */
         smx_PMsgRel(&pch->pmsg, 0);
         pch->num--;
      }
      while (pch->num)
      {
         /* release pmsgs at rxchg, wait up to pch->tmo for each */
         #if SB_CPU_ARMM7
         dsn = sn;
         #elif SB_CPU_ARMM8
         if (smx_TaskPeek(SMX_CT, SMX_PK_UMODE))
            dsn = sn;
         else
            dsn = (sn << 4) + sn; /*<2>*/
         #endif
         pmsg = smx_PMsgReceive(pch->rxchg, NULL, dsn, pch->tmo, 0);
         if (pmsg)
         {
            smx_PMsgRel(&pmsg, 0);
            pch->num--;
         }
         else
         {
            mp_PORTAL_RET(MP_ID_FPORTAL_CLOSE, false);
            return false;
         }
      }
      smx_MsgXchgDelete(&pch->rxchg);
   }
   /* clear FPCS after handles */
   memset((void*)&pch->errno, 0, sizeof(FPCS)-MP_FPCS_ERRNUM_OFFSET);
   smx_HT_DELETE(pch);  /* for smxAware */
   mp_PORTAL_RET(MP_ID_FPORTAL_CLOSE, true);
   return true;
}

/*
*  mp_FPortalOpen()
*
*  Opens a free message portal for a client. If nmsg > 0, creates rxchg, gets
*  up to nmsg pmsgs, and sends them to rxchg. Initializes FPCS fields, except
*  name and sxchg.
*/
bool mp_FPortalOpen(FPCS* pch, u8 csn, u32 msz, u32 nmsg, u8 pri, u32 tmo,
                                                            const char* rxname)
{
   u32      i = 0; /* number of pmsgs created */
   MCB*     pmsg;

   mp_PORTAL_LOG6(MP_ID_FPORTAL_OPEN, (u32)pch, csn, msz, nmsg, pri, tmo);  /* 6 max <1> */
   if (pch->open)
   {
      mp_PORTAL_RET(MP_ID_FPORTAL_OPEN, true);
      return true;
   }
   if (pch->sxchg == NULL)
   {
      mp_PortalEM((PS*)pch, SPE_PORTAL_NEXIST, &pch->errno);
      mp_PORTAL_RET(MP_ID_FPORTAL_OPEN, false);
      return false;
   }
   if (nmsg > 0)
   {
      /* create rxchg for portal */
      pch->rxchg = smx_MsgXchgCreate(SMX_XCHG_NORM, rxname ? rxname : "rxchg", 0);
      if (pch->rxchg == NULL)
      {
         mp_PORTAL_RET(MP_ID_FPORTAL_OPEN, false);
         return false;
      }

      /* get nmsg pmsgs from main heap and send to pch->rxchg */
      for (i = 0; i < nmsg; i++)
      {
         pmsg = smx_PMsgGetHeap(msz, NULL, csn, MP_DATARW, 0, 0);
         if (pmsg)
            smx_PMsgSend(pmsg, pch->rxchg, pri, pch->rxchg);
         else
         {
            /* delete rxchg and release all pmsgs obtained */
            smx_MsgXchgDelete(&pch->rxchg);
            mp_PORTAL_RET(MP_ID_FPORTAL_OPEN, false);
            return false;
         }
      }
   }
   /* finish loading client portal structure */
   pch->pmsg = NULL;
   pch->csn  = csn;
   pch->num  = i;
   pch->pri  = pri;
   pch->tmo  = tmo;
   pch->open = 1;
   mp_PORTAL_RET(MP_ID_FPORTAL_OPEN, true);
   return true;
}

/*
*  mp_FPortalReceive()
*
*  Allows client to receive free pmsg from pch->rxchg. If pmsg recieved, loads
*  pointer to pmsg data block into *dpp, if dpp != NULL, and returns pmsg handle.
*/
MCB* mp_FPortalReceive(FPCS* pch, u8** dpp)
{
   u8   dsn;
   MCB* pmsg;

   mp_PORTAL_LOG2(MP_ID_FPORTAL_RECEIVE, (u32)pch, (u32)dpp);
   if (!pch->open)
   {
      mp_PortalEM((PS*)pch, SPE_PORTAL_NOPEN, &pch->errno);
      mp_PORTAL_RET(MP_ID_FPORTAL_RECEIVE, NULL);
      return NULL;
   }
   pch->errno = SMXE_OK;
   #if SB_CPU_ARMM7
   dsn = pch->csn;
   #elif SB_CPU_ARMM8
   if (smx_TaskPeek(SMX_CT, SMX_PK_UMODE))
      dsn = pch->csn;
   else
      dsn = pch->csn << 4;    /*<2>*/
   #endif
   pmsg = smx_PMsgReceive(pch->rxchg, dpp, dsn, pch->tmo, 0);
   pch->pmsg = pmsg ? pmsg : NULL;
   mp_PORTAL_RET(MP_ID_FPORTAL_RECEIVE, (u32)pmsg);
   return pmsg;
}

/*
*  mp_FPortalSend()
*
*  Sends free pmsg from client to server.
*/
bool mp_FPortalSend(FPCS* pch, MCB* pmsg)
{
   bool ret;

   mp_PORTAL_LOG2(MP_ID_FPORTAL_SEND, (u32)pch, (u32)pmsg);
   if (!pch->open)
   {
      mp_PortalEM((PS*)pch, SPE_PORTAL_NOPEN, &pch->errno);
      mp_PORTAL_RET(MP_ID_FPORTAL_SEND, false);
      return false;
   }
   pch->errno = SMXE_OK;
   if (pch->pmsg == pmsg)
      pch->pmsg  = NULL;
   ret = smx_PMsgSend(pmsg, pch->sxchg, pch->pri, pch->rxchg);
   mp_PORTAL_RET(MP_ID_FPORTAL_SEND, ret);
   return ret;
}

/*
*  mp_FTPortalSend()
*
*  Sends free pmsg from client to tunnel portal server.
*/
bool mp_FTPortalSend(FPCS* pch, u8* bp, MCB* pmsg)
{
   bool  ret;
   TOMH* hp = (TOMH*)bp;

   mp_PORTAL_LOG3(MP_ID_FTPORTAL_SEND, (u32)pch, (u32)bp, (u32)pmsg);
   if (!pch->open)
   {
      mp_PortalEM((PS*)pch, SPE_PORTAL_NOPEN, &pch->errno);
      mp_PORTAL_RET(MP_ID_FTPORTAL_SEND, false);
      return false;
   }
   hp->type = FREEMSG;
   hp->cmd  = CONTROL;
   pch->errno = SMXE_OK;
   if (pch->pmsg == pmsg)
      pch->pmsg  = NULL;
   ret = smx_PMsgSend(pmsg, pch->sxchg, pch->pri, pch->rxchg);
   mp_PORTAL_RET(MP_ID_FTPORTAL_SEND, ret);
   return ret;
}
#endif /* SMX_CFG_PORTAL */

/* 
   Notes:
   1. mp_PortalLog() called via SVC which smx limits to 7 pars, which are
      the call ID plus 6 pars.
   2. For ARMM8, assumes that all pmsgs at rxchg have pmsg->con.sb = 1 and 
      thus their data blocks are in sys_data. This is because mp_FPortalOpen()
      gets all pmsgs from mheap.
*/
