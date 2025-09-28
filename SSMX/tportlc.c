/*
* tportlc.c                                                 Version 5.4.0
*
* Tunnel portal client functions and objects.
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

#include "xapiu.h"

#pragma diag_suppress=Ta168  /* override warning */
#pragma section_prefix = ".ucom"

/*
*  mp_TPortalClose()
*
*  Sends CLOSE command to server and waits for ack, unless portal has been
*  deleted. Clears msg hdr and TPCS after handles. Deletes csem and ssem and
*  returns true unless a semaphore fails to delete.
*/
bool mp_TPortalClose(TPCS* pch, u32 tmo)
{  
   TPMH* mhp = pch->mhp; /* portal msg header pointer */

   mp_PORTAL_LOG2(MP_ID_TPORTAL_CLOSE, (u32)pch, tmo);

   /* send CLOSE command to server to close portal and wait for ack */
   if (pch->sxchg != NULL)
   {
      mhp->cmd = CLOSE;
      /* if client marked closed due to timeout, mark open and clear csem 
         so following operations work */
      if (!pch->open)
      {
         pch->open = true;
         smx_SemClear(pch->csem);
      }
      smx_SemSignal(pch->ssem);
      smx_SemTest(pch->csem, tmo);
   }

   /* clear pmsg header and TPCS: open, shp, mdp, mdsz, and data */
   memset((void*)mhp, 0, sizeof(TPMH));
   pch->open = false;
   memset((void*)&pch->shp, 0, sizeof(TPCS)-MP_TPCS_MHP_OFFSET);

   /* delete semaphores */
   if (!smx_SemDelete(&pch->csem) || !smx_SemDelete(&pch->ssem))
   {
      mp_PORTAL_RET(MP_ID_TPORTAL_CLOSE, false);
      return false; /* error = SMXE_INV_SCB */
   }
   else
   {
      smx_HT_DELETE(pch);  /* for smxAware */
      mp_PORTAL_RET(MP_ID_TPORTAL_CLOSE, true);
      return true;
   }
}

/*
*  mp_TPortalOpen()
*
*  Opens an existing portal for a client. Aborts if pmsg does not exist or
*  portal is already open. Creates csem and ssem for the portal, loads pmsg 
*  parameters into the portal client structure, initializes an OPEN command 
*  pmsg, and sends it to sxchg. mdsz is the size of the block used for
*  data transfers (not including header sizes). Total header size, thsz = pmsg
*  header size + service header size. msz = thsz + mdsz.
*
*  Note: Must be preceded with:
*  1. Get msg of size >= msz.
*  2. pch->mhp = msg->bp (done via par to smx_PMsgGetHeap()).
*  3. pch->sxchg and pch->name loaded (done by TPortalCreate()).
*/
bool mp_TPortalOpen(TPCS* pch, u32 msz, u32 thsz, u8 pri, u32 tmo, 
                                       const char* ssname, const char* csname)
{
   TPMH* mhp  = pch->mhp;        /* message header pointer */
   TOMH* omsp = (TOMH*)pch->mhp; /* OPEN message pointer */
   bool  csem_cre = false;       /* csem created now */

   mp_PORTAL_LOG6(MP_ID_TPORTAL_OPEN, (u32)pch, msz, thsz, pri, tmo, (u32)ssname);  /* <1> */

   /* abort if portal does not exist */
   if (pch->sxchg == NULL)
   {
      mp_PortalEM((PS*)pch, SPE_PORTAL_NEXIST, &mhp->errno);
      mp_PORTAL_RET(MP_ID_TPORTAL_OPEN, false);
      return false;
   }

   /* abort if pmsg does not exist */
   if (pch->pmsg == NULL)
   {
      mp_PortalEM((PS*)pch, SPE_NO_PMSG, NULL);
      mp_PORTAL_RET(MP_ID_TPORTAL_OPEN, false);
      return false;
   }

   /* create client semaphore for portal unless it already exists <2> */
   if (pch->csem == NULL)
   {
      pch->csem = smx_SemCreate(SMX_SEM_EVENT, 1, csname ? csname : "csem", 0);
      if (pch->csem == NULL)
      {
         mp_PORTAL_RET(MP_ID_TPORTAL_OPEN, false);
         return false; /* SMXE_INV_PAR or SMXE_OUT_OF_SCBS */
      }
      csem_cre = true;
   }

   /* create server semaphore for portal unless it already exists <2> */
   if (pch->ssem == NULL)
   {
      pch->ssem = smx_SemCreate(SMX_SEM_EVENT, 1, ssname ? ssname : "ssem", 0);
      if (pch->ssem == NULL)
      {
         smx_SemDelete(&pch->csem);
         mp_PORTAL_RET(MP_ID_TPORTAL_OPEN, false);
         return false; /* SMXE_INV_PAR or SMXE_OUT_OF_SCBS */
      }
   }

   /* finish loading client portal structure */
   pch->mdp  = (u8*)pch->mhp + thsz;
   pch->shp  = (u32*)pch->mhp + sizeof(TPMH)/4;
   pch->mdsz = msz - thsz;

   /* load OPEN msg */
   omsp->type  = TUNNEL;
   omsp->cmd   = OPEN;
   omsp->con.eod  = false;
   omsp->con.sod  = false;
   omsp->errno   = SMXE_OK;
   omsp->mdsz  = msz - thsz;
   omsp->rqsz  = 0;
   omsp->csem  = pch->csem;
   omsp->ssem  = pch->ssem;
   omsp->thsz  = thsz;

   /* ensure csem clear in case still set from prior use of portal */
   if (!csem_cre)
   {
      smx_SemClear(pch->csem);
   }
   /* send bound msg to portal xchg and wait for ack */
   smx_PMsgSendB(pch->pmsg, pch->sxchg, pri, 0);
   if (smx_SemTest(pch->csem, tmo))
   {
      pch->open = true;
      mp_PORTAL_RET(MP_ID_TPORTAL_OPEN, true);
      return true;
   }
   mp_PORTAL_RET(MP_ID_TPORTAL_OPEN, false);
   return false;
}

/*
*  mp_TPortalReceive()
*
*  Allows client to receive data blocks from server. Sends RECEIVE command, 
*  clears eod, sets sod, passes rqsz via mhp, and signals ssem. Waits on csem 
*  for data block from server, then if dp is !NULL copies data block to user 
*  buffer. If !eod, updates dp and signals ssem for next block. If eod, 
*  resets it, and returns true. If portal is not open or is closed returns 
*  false and reports it. In NO_COPY mode returns false and reports INV_SZ if 
*  rqsz is too large. 
*/
bool mp_TPortalReceive(TPCS* pch, u8* dp, u32 rqsz, u32 tmo)
{
   TPMH* mhp  = pch->mhp;     /* message header pointer */
   u32   dsz;                 /* data size to transfer to cbuf */

   mp_PORTAL_LOG4(MP_ID_TPORTAL_RECEIVE, (u32)pch, (u32)dp, rqsz, tmo);

   if (mhp == 0)
   {
      mp_PortalEM((PS*)pch, SPE_PORTAL_NOPEN, NULL);
      mp_PORTAL_RET(MP_ID_TPORTAL_RECEIVE, false);
      return false;
   }
   mhp->cmpsz = 0;

   /* abort if no-copy and rqsz is larger than pmsg data block */
   if (dp == NULL && rqsz > pch->mdsz)
   {
      mp_PortalEM((PS*)pch, SPE_INV_SZ, &mhp->errno);
      mp_PORTAL_RET(MP_ID_TPORTAL_RECEIVE, false);
      return false;
   }

   mhp->cmd = RECEIVE;  /* <3> */
   if (pch->open)
   {
      /* send RECEIVE command */
      mhp->con.eod = false;
      mhp->con.sod = true;
      mhp->errno  = SMXE_OK;
      mhp->rqsz    = rqsz;
      smx_SemSignal(pch->ssem);

      /* wait for first data block from server */
      if (!smx_SemTest(pch->csem, tmo))   /* timeout */
      {
         mp_PortalEM((PS*)pch, SPE_CLIENT_TMO, &mhp->errno);
         mp_PORTAL_RET(MP_ID_TPORTAL_RECEIVE, false);
         pch->open = false;
      }
      else if (mhp->errno == SPE_TRANS_INC)  /* transfer incomplete */
      {
         mp_PortalEM((PS*)pch, SPE_TRANS_INC, &mhp->errno);
         mp_PORTAL_RET(MP_ID_TPORTAL_RECEIVE, false);
         return false;
      }
   }
   else  /* portal not open */
   {
      mp_PortalEM((PS*)pch, SPE_PORTAL_NOPEN, &mhp->errno);
      mp_PORTAL_RET(MP_ID_TPORTAL_RECEIVE, false);
      return false;
   }

   /* accept data blocks from server */
   while (pch->open)
   {
      if (dp != NULL)
      {
         /* copy block to cbuf */
         dsz = mhp->mdsz;
         memcpy(dp, pch->mdp, dsz);
         mhp->cmpsz += dsz;
      }
      if (dp != NULL && !mhp->con.eod)
      {
         /* request next data block */
         dp += dsz;
         smx_SemSignal(pch->ssem);

         /* wait for next data block from server */
         if (!smx_SemTest(pch->csem, tmo))   /* timeout */
         {
            mp_PortalEM((PS*)pch, SPE_CLIENT_TMO, &mhp->errno);
            mp_PORTAL_RET(MP_ID_TPORTAL_RECEIVE, false);
            pch->open = false;
         }
         else if (mhp->errno == SPE_TRANS_INC)  /* transfer incomplete */
         {
            mp_PortalEM((PS*)pch, SPE_TRANS_INC, &mhp->errno);
            mp_PORTAL_RET(MP_ID_TPORTAL_RECEIVE, false);
            return false;
         }
      }
      else  /* receive is complete */
      {
         if (dp == NULL)  /* no-copy */
            mhp->cmpsz = pch->mdsz;
         mhp->con.eod = false;
         mp_PORTAL_RET(MP_ID_TPORTAL_RECEIVE, true);
         return true;
      }
   }
   mp_PortalEM((PS*)pch, SPE_PORTAL_CLOSED, &mhp->errno);   /* portal was closed */
   mp_PORTAL_RET(MP_ID_TPORTAL_RECEIVE, false);
   return false;
}

/*
*  mp_TPortalSend()
*
*  Allows client to send data blocks or commands to server. If dp == NULL, 
*  sends rqsz bytes in pmsg data block, signals ssem, and returns true. If dp
*  != NULL, transfers up to mdsz bytes in pmsg data block, signals ssem,
*  and waits at csem for ack from server that it completed the operation. 
*  Continues until rqsz bytes have been sent and returns true. Returns false 
*  if csem times out, portal is not open, transmission error occurs, or rqsz 
*  is too large for no-copy op.
*/
bool mp_TPortalSend(TPCS* pch, u8* dp, u32 rqsz, u32 tmo)
{
   u32   mdsz = pch->mdsz;   /* message data block size */
   TPMH* mhp  = pch->mhp;    /* message header pointer */
   u32   sz   = rqsz;        /* size left to send */

   mp_PORTAL_LOG4(MP_ID_TPORTAL_SEND, (u32)pch, (u32)dp, rqsz, tmo);

   if (mhp == 0)
   {
      mp_PortalEM((PS*)pch, SPE_PORTAL_NOPEN, NULL);
      mp_PORTAL_RET(MP_ID_TPORTAL_SEND, false);
      return false;
   }

   mhp->cmpsz = 0;
   mhp->cmd = SEND;  /* <3> */
   if (pch->open)
   {
      /* send SEND command */
      if (dp == NULL)   /* no-copy */
      {
         if (sz <= mdsz)
         {
            /* no-copy send single block */
            mhp->mdsz = sz;
            mhp->rqsz = sz;
            mhp->con.sod = true;
            mhp->con.eod = true;
            mhp->errno  = SMXE_OK;
            smx_SemSignal(pch->ssem);         /* notify server */
            /* wait for server done ack */
            if (!smx_SemTest(pch->csem, tmo))   /* timeout */
            {
               mp_PortalEM((PS*)pch, SPE_CLIENT_TMO, &mhp->errno);
               mp_PORTAL_RET(MP_ID_TPORTAL_SEND, false);
               pch->open = false;
               return false;
            }
            else if (mhp->errno == SPE_TRANS_INC)  /* transfer incomplete */
            {
               mp_PortalEM((PS*)pch, SPE_TRANS_INC, &mhp->errno);
               mp_PORTAL_RET(MP_ID_TPORTAL_SEND, false);
               return false;
            }
            else  /* transmission complete */
            {
               mhp->cmpsz = mdsz;
               mp_PORTAL_RET(MP_ID_TPORTAL_SEND, true);
               return true;
            }
         }
         else
         {
            mp_PortalEM((PS*)pch, SPE_INV_SZ, &mhp->errno);  /* size too big */
            mp_PORTAL_RET(MP_ID_TPORTAL_SEND, false);
            return false;
         }
      }

      /* copy and send one or more blocks */
      mhp->con.sod = true;
      mhp->con.eod = false;
      mhp->errno  = SMXE_OK;
      mhp->rqsz    = sz;
      do 
      {
         if (sz > mdsz)
         {
            /* copy and send next data block */
            memcpy(pch->mdp, dp, mdsz);
            mhp->mdsz = mdsz;
            dp += mdsz;
            sz -= mdsz;
            smx_SemSignal(pch->ssem);           /* notify server */
            /* wait for server done ack */
            if (!smx_SemTest(pch->csem, tmo))   /* timeout */
            {
               mp_PortalEM((PS*)pch, SPE_CLIENT_TMO, &mhp->errno);
               mp_PORTAL_RET(MP_ID_TPORTAL_SEND, false);
               pch->open = false;
               return false;
            }
            else if (mhp->errno == SPE_TRANS_INC)  /* transfer incomplete */
            {
               mp_PortalEM((PS*)pch, SPE_TRANS_INC, &mhp->errno);
               mp_PORTAL_RET(MP_ID_TPORTAL_SEND, false);
               return false;
            }
            mhp->cmpsz += mdsz;
         }
         else
         {
            /* copy and send final data block */
            memcpy(pch->mdp, dp, sz);
            mhp->mdsz    = sz;
            mhp->con.eod = true;
            sz = 0;
            smx_SemSignal(pch->ssem);           /* notify server */
            /* wait for server done ack */
            if (!smx_SemTest(pch->csem, tmo))   /* timeout */
            {
               mp_PortalEM((PS*)pch, SPE_CLIENT_TMO, &mhp->errno);
               mp_PORTAL_RET(MP_ID_TPORTAL_SEND, false);
               pch->open = false;
               return false;
            }
            else if (mhp->errno == SPE_TRANS_INC)  /* transfer incomplete */
            {
               mp_PortalEM((PS*)pch, SPE_TRANS_INC, &mhp->errno);
               mp_PORTAL_RET(MP_ID_TPORTAL_SEND, false);
               return false;
            }
            mhp->cmpsz += mhp->mdsz;
         }
         if (sz == 0)
         {
            mp_PORTAL_RET(MP_ID_TPORTAL_SEND, true);
            return true;
         }
      } while (pch->open);
   }
   mp_PortalEM((PS*)pch, SPE_PORTAL_CLOSED, &mhp->errno);   /* portal was closed */
   mp_PORTAL_RET(MP_ID_TPORTAL_SEND, false);
   return false;
}
#endif /* SMX_CFG_PORTAL */

/* Notes:
   1. SVC calls are limited to 7 parameters, so call ID and only the first 6
      parameters are passed.
   2. Allows reopening portal if closed or to change pmsg.
   3. Have to set cmd before checking pch->open because client shell already
      has changed shp fields, so we can't leave msg type set to whatever it
      was before, which could be OPEN, which server may then process.
*/
