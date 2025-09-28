/*
* tportls.c                                                 Version 5.4.0
*
* Tunnel portal server functions and objects.
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
*  mp_TPortalCreate()
*
*  Creates server xchg, loads portal name and sxchg handle into authorized client 
*  portal structures, and loads sxchg and name into portal server structure.
*  Notes: 
*     1. Must be called from pmode.
*     2. ARMM7: dsn is a single slot number.
*     3. ARMM8: if portal task is a ptask, dsn must be a dual slot number.
*/
bool mp_TPortalCreate(TPSS** pshp, TPCS** pclp, u32 pclsz, u8 dsn, 
                                       const char* pname, const char* sxname)
{
   u32   i;
   bool  pass;
   TPSS* psh;

   /* verify current task has create permission */
   pass = smx_TOKEN_TEST(smx_ct, (u32)pshp, SMX_PRIV_HI);
   if (pass)
   {
      psh = *pshp;
      mp_PORTAL_LOG4(MP_ID_TPORTAL_CREATE, (u32)pshp, (u32)pclp, pclsz, dsn);  /* <1> */
      psh->sxchg = smx_MsgXchgCreate(SMX_XCHG_PASS, sxname ? sxname : "sxchg", NULL, SMX_PI);
      if (psh->sxchg == NULL)
      {
         /* smx reported error: SMXE_WRONG_MODE or SMXE_OUT_OF_XCBS */
         mp_PORTAL_RET(MP_ID_TPORTAL_CREATE, false);
         return false;
      }

      /* load portal name and sxchg into portal client structures */
      for (i = pclsz; i > 0; i--, pclp++)
      {
         (*pclp)->pname = pname;
         (*pclp)->sxchg = psh->sxchg;
      }
      psh->pname = pname;
      psh->dsn = dsn;
      psh->pshp = pshp;
      smx_HT_ADD(psh, pname);  /* for smxAware */
      mp_PORTAL_RET(MP_ID_TPORTAL_CREATE, true);
   }
   return pass;
}

/*
*  mp_TPortalDelete()
*
*  Stops server task, deletes server xchg, clears portal name and sxchg handle 
*  in each authorized portal client structure, closes portal, and clears portal 
*  server structure.
*  Note: must be called from pmode.
*/
bool mp_TPortalDelete(TPSS* psh, TPCS** pclp, u32 pclsz)
{
   u32  i;
   bool pass;

   pass = smx_TOKEN_TEST(smx_ct, (u32)psh->pshp, SMX_PRIV_HI);
   if (pass)
   {
      mp_PORTAL_LOG3(MP_ID_TPORTAL_DELETE, (u32)psh, (u32)pclp, pclsz);
      smx_TaskStop(psh->stask, SMX_TMO_INF);

      if (!smx_MsgXchgDelete(&psh->sxchg))
      {
         /* smx reported error: SMXE_INV_XCB */
         mp_PORTAL_RET(MP_ID_TPORTAL_DELETE, false);
         return false;
      }
      /* clear portal name, sxchg, and open in portal client structures */
      for (i = pclsz; i > 0; i--, pclp++)
      {
         (*pclp)->pname = "none";
         (*pclp)->sxchg = NULL;
         (*pclp)->open  = 0;
      }
      /* clear TPSS upper fields */
      psh->pname = "none";
      psh->pmsg = NULL;
      psh->csem = NULL;
      psh->ssem = NULL;

      /* clear TPSS: open, mhp, shp, mdp, and mdsz */
      psh->open = false;
      memset((void*)&psh->mhp, 0, sizeof(TPSS)-MP_TPSS_MHP_OFFSET);
      smx_HT_DELETE(psh);  /* for smxAware */
      mp_PORTAL_RET(MP_ID_TPORTAL_DELETE, true);
   }
   return pass;
}

/* portal error manager */
void mp_PortalEM(PS* ph, SMX_ERRNO errno, SMX_ERRNO* ep)
{
   char    pem[40]; /* portal error message buffer */
   CPU_FL  istate;
   EREC*   p;

   if (ep != NULL)
      *ep = errno;

   if (ph->pname)
   {
      strcpy(pem, ph->pname);
      strcat(pem, " ");
      strcat(pem, smx_errmsgs[errno]);
   }
   else
      strcpy(pem, smx_errmsgs[errno]);

   /* display error message */
   sb_MsgOut(SB_MSG_ERR, pem);

   smx_errno = errno;
   smx_errctr++;
   smx_errctrs[errno]++;

   /* save error information in smx error buffer if smx_eben */
   if (smx_eben)
   {
      istate = sb_IntStateSaveDisable();
      p = smx_ebn > smx_ebx ? smx_ebi : smx_ebn;
      p->etime  = smx_etime;
      p->err    = errno;
      p->handle = (void*)ph;
      smx_ebn   = p + 1;
      sb_IntStateRestore(istate);
   }
   /* save error information smx event buffer if SMX_EVB_EN_PERR */
   mp_EVB_LOG_PORTAL_ERROR(errno, smx_ct, ph);
   smx_EMHook(errno, NULL, 0);
}

/*
* mp_PortalLog()
*
*  Logs portal function entry into EVB.
*/
void mp_PortalLog(u32 id, u32 p1, u32 p2, u32 p3, u32 p4, u32 p5, u32 p6)
{
   u32  n;
   u32* p;
   CPU_FL istate = sb_IntStateSaveDisable();
   if (smx_evben & SMX_EVB_EN_PORTAL)
   {
      n = (id & 0x0000F000)>>12;
      n = (n > 6) ? 6 : n;
      p = (smx_evbn > smx_evbx) ? smx_evbi : smx_evbn;
      *p++ = (0x55550004 + n) | SMX_EVB_RT_PORTAL;
      *p++ = (smx_clsr != NULL) ? (u32)smx_clsr : (u32)smx_ct;
      *p++ = id;
      p += n;
      *p = sb_PtimeGet();
      smx_evbn = p+1;
      switch (n)
      {
         /* deliberate fall-through */
         case 6:
            *--p = p6;
         case 5:
            *--p = p5;
         case 4:
            *--p = p4;
         case 3:
            *--p = p3;
         case 2:
            *--p = p2;
         case 1:
            *--p = p1;
         case 0:
         default:
            break;
      }
   }
   sb_IntStateRestore(istate);
}

/*
* mp_PortalRet()
*
*  Logs portal function return into EVB.
*/
void mp_PortalRet(u32 id, u32 rv)
{
   u32* p;
   CPU_FL istate = sb_IntStateSaveDisable();
   if (smx_evben & SMX_EVB_EN_PORTAL)
   {
      p = (smx_evbn > smx_evbx) ? smx_evbi : smx_evbn;
      *p++ = 0x55550005 | SMX_EVB_RT_PORTAL_RET;
      *p++ = sb_PtimeGet();
      *p++ = (smx_clsr != NULL) ? (u32)smx_clsr : (u32)smx_ct;
      *p++ = smx_sched;
      *p++ = (rv);
      smx_evbn = p;
   }
   sb_IntStateRestore(istate);
}

#include "xapiu.h"
#pragma default_function_attributes = @ ".ucom.text"

static void mp_TPortalCallServerFunc(TPSS* psh);

/*
*  mp_TPortalServer()
*
*  Called by portal task to perform portal server functions. Waits at sxchg
*  for portal OPEN pmsg to open portal at server end. Signals csem when 
*  commands are done; waits at ssem for more commands. When portal is closed
*  by client, waits at sxchg for next OPEN pmsg. To stop portal server, stop 
*  or delete the portal task.
*/

void mp_TPortalServer(TPSS* psh, u32 stmo)
{
   u32      dsz;  /* data size */
   TPMH*    mhp;  /* msg header ptr */
   TOMH*    omsp; /* portal open msg ptr */
   u32      sz;   /* data size to go */

   /* get next pmsg */
   while (psh->pmsg = smx_PMsgReceive(psh->sxchg, (u8**)&mhp, psh->dsn, SMX_TMO_INF, 0))
   {
      if (mhp->type == TUNNEL)
      {
         mp_PORTAL_LOG2(MP_ID_TPORTAL_SERVER, (u32)mhp->type, (u32)mhp->cmd);
         if (mhp->cmd == OPEN) /* first command must be OPEN */
         {
            do /* continuously loop while open */
            {
               /* perform operations */
               switch (mhp->cmd)
               {
                  case OPEN:
                     omsp     = (TOMH*)mhp;
                     psh->mhp = mhp;
                     psh->shp = (u32*)mhp + sizeof(TPMH)/4;
                     psh->mdp = (u8*)mhp + omsp->thsz;
                     psh->mdsz = omsp->mdsz;
                     psh->csem = omsp->csem;
                     psh->ssem = omsp->ssem;
                     psh->open = true;
                     break;
                  case SEND:    /* from client */
                     mp_TPortalCallServerFunc(psh);
                     mhp->con.sod = false;
                     mhp->con.eod = false;
                     break;
                  case RECEIVE: /* to client */
                     if (mhp->con.sod)
                        sz = mhp->rqsz;
                     dsz = (sz > psh->mdsz ? psh->mdsz : sz);
                     mhp->mdsz = dsz;
                     mp_TPortalCallServerFunc(psh);
                     sz -= dsz;
                     mhp->con.sod = false;
                     if (sz == 0)
                        mhp->con.eod = true; /* all blocks sent */
                     break;
                  case CLOSE:
                     psh->open = false;
                     smx_MsgXchgClear(psh->sxchg); /*<2>*/
                     break;
                  default:
                     mp_PortalEM((PS*)psh, SPE_INV_CMD, &mhp->errno);
               }
               if (psh->csem)
                  smx_SemSignal(psh->csem);
              /* if portal is open, wait for client request */
            } while (psh->open && smx_SemTest(psh->ssem, stmo));

            /* if ssem timeout: close portal and report server timeout */
            if (psh->open)
            {
               psh->open = false;
               mp_PortalEM((PS*)psh, SPE_SERVER_TMO, &mhp->errno);
            }
         }
         else
         {
            mp_PortalEM((PS*)psh, SPE_INV_CMD, &mhp->errno); /* invalid command */
         }
      }
      else if (mhp->type == FREEMSG)
      {
         mp_PORTAL_LOG2(MP_ID_TPORTAL_SERVER, (u32)mhp->type, (u32)mhp->cmd);
         psh->mhp = mhp;
         psh->shp = (u32*)mhp + sizeof(TPMH)/4;

         /* perform operation */
         switch (mhp->cmd)
         {
            case CONTROL:
               mp_TPortalCallServerFunc(psh);
               break;
            default:
               mp_PortalEM((PS*)psh, SPE_INV_CMD, &mhp->errno);
         }
      }
      else
         mp_PortalEM((PS*)psh, SPE_INV_TYPE, &mhp->errno); /* invalid pmsg type */

      /* Initialize TPSS for next request */             
      psh->pmsg = NULL;
      psh->csem = NULL;
      psh->ssem = NULL;
      /* clear TPSS open, mhp, shp, mdp, mdsz */
      psh->open = false;
      memset((void*)&psh->mhp, 0, sizeof(TPSS)-MP_TPSS_MHP_OFFSET);
      mp_PORTAL_RET(MP_ID_TPORTAL_SERVER, (u32)psh->pmsg);
   }
}

/* Define portal server functions */
#if MW_FATFS_DEMO
void fp_server(TPSS* ph);
#else
#define fp_server(psh) mp_PortalEM((PS*)psh, SPE_INV_SID, &psh->mhp->errno);
#endif

#if defined(SMX_TSMX)
void tp_pserver(TPSS* psh);
void tp_userver(TPSS* psh);
#else
#define tp_pserver(psh) mp_PortalEM((PS*)psh, SPE_INV_SID, &psh->mhp->errno);
#define tp_userver(psh) mp_PortalEM((PS*)psh, SPE_INV_SID, &psh->mhp->errno);
#endif 

/* Call server function based upon server id */
static void mp_TPortalCallServerFunc(TPSS* psh)
{
   switch (psh->sid)
   {
     #if MW_FATFS_DEMO
      case FP:
         fp_server(psh);
         break;
     #endif
      case TSTPP:
         tp_pserver(psh);
         break;
      case TSTUP:
         tp_userver(psh);
         break;
      default:
         mp_PortalEM((PS*)psh, SPE_INV_SID, &psh->mhp->errno);
   }
}

#endif /* SMX_CFG_PORTAL */

/* 
   Notes:
   1. SVC handler limits mp_PortalLog() calls to 7 pars: call ID plus 6 pars.
   2. Pending messages have to be cleared when the portal is closed since they
      may have been freed by the time smx_PMsgReceive() is called at the top,
      so they have no region, which causes MMF trying to check mhp->type.
*/
