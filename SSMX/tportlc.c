/*
* tportlc.c                                                 Version 6.0.0
*
* Tunnel portal client functions and objects.
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

#if SMX_CFG_PORTAL
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
bool mp_TPortalOpen(TPCS* pch, u32 msz, u32 thsz, u32 tmo, 
                                       const char* ssname, const char* csname)
{
   bool  csem_cre = false;       /* csem created now */
   TPMH* mhp  = pch->mhp;        /* message header pointer */
   TOMH* omsp = (TOMH*)pch->mhp; /* OPEN message pointer */

   mp_PORTAL_LOG6(MP_ID_TPORTAL_OPEN, (u32)pch, msz, thsz, tmo, (u32)ssname, (u32)csname);

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

   /* create client semaphore for portal unless it already exists <1> */
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

   /* create server semaphore for portal unless it already exists <1> */
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
   omsp->cmd   = CLOSE; /*<3>*/
   omsp->con.eod  = false;
   omsp->con.sod  = false;
   omsp->errno   = SMXE_OK;
   omsp->mdsz  = msz - thsz;
   omsp->rqsz  = 0;
   omsp->csem  = pch->csem;
   omsp->ssem  = pch->ssem;
   omsp->thsz  = thsz;
   omsp->cmd   = OPEN;

   /* ensure csem clear in case still set from prior use of portal */
   if (!csem_cre)
   {
      smx_SemClear(pch->csem);
   }
   smx_PMsgSendB(pch->pmsg, pch->sxchg, smx_ct->pri, 0);
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
*  mp_TPortalClose()
*
*  Sends CLOSE command to server. Allows server to close connection at its end,
*  if not already closed, then closes connection at client's end. 
*/
bool mp_TPortalClose(TPCS* pch)
{  
   TPMH* mhp = pch->mhp; /* portal msg header pointer */

   mp_PORTAL_LOG1(MP_ID_TPORTAL_CLOSE, (u32)pch);

   /* send CLOSE command to server */
   if (pch->sxchg != NULL)
   {
      mhp->cmd = CLOSE;
      smx_SemSignal(pch->ssem);
      smx_TaskBump(SMX_CT, SMX_PRI_NOCHG); /* allow server to close <4> */
   }

   /* clear TPCS from open down */
   memset((void*)&pch->open, 0, 20);

   /* restore client priority if it was promoted */
   smx_ct->pri = smx_ct->prinorm;

   /* delete semaphores */
   if (!smx_SemDelete(&pch->csem) || !smx_SemDelete(&pch->ssem))
   {
      mp_PORTAL_RET(MP_ID_TPORTAL_CLOSE, false);
      return false; /* error = SMXE_INV_SCB */
   }
   else
   {
      mp_PORTAL_RET(MP_ID_TPORTAL_CLOSE, true);
      return true;
   }
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
   TPMH* mhp  = pch->mhp;  /* message header pointer */
   u32   dsz;              /* data size to transfer to cbuf */

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

   mhp->cmd = RECEIVE;  /* <2> */
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
   mhp->cmd = SEND;  /* <2> */
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
   1. Allows reopening portal if closed or to change pmsg.
   2. Have to set cmd before checking pch->open because client shell already
      has changed shp fields, so we can't leave msg type set to whatever it
      was before, which could be OPEN, which server might then process.
   3. mp_TPortalClose does not remove pmsg from sxchg if the connection was
      already closed at its end. If the server preempts the client while it is
      creating an OPEN message, this causes the server to remove the pmsg and 
      to close an already closed connection.
   4. It is safest to run the server before pmsg is changed.
*/
