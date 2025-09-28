/*
* fportls.c                                                 Version 5.4.0
*
* Free message portal server functions and objects.
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
*  mp_FPortalCreate()
*
*  Creates server sxchg, loads sxchg handle and portal name into authorized 
*  portal client structures and loads sxchg and name into portal server structure.
*  Note: must be called from pmode. 
*/
bool mp_FPortalCreate(FPSS** pshp, FPCS** pclp, u32 pclsz, u8 ssn, 
                                       const char* pname, const char* sxname)
{
   u32   i;
   bool  pass;
   FPSS* psh;

   /* verify current task has create permission */
   pass = smx_TOKEN_TEST(smx_ct, (u32)pshp, SMX_PRIV_HI);
   if (pass)
   {
      psh = *pshp;
      mp_PORTAL_LOG6(MP_ID_FPORTAL_CREATE, (u32)psh, (u32)pclp, pclsz, ssn, (u32)pname, (u32)sxname);  /* <1> */
      psh->sxchg = smx_MsgXchgCreate(SMX_XCHG_PASS, sxname ? sxname : "sxchg", NULL, SMX_PI);
      if (psh->sxchg == NULL)
      {
         /* smx reported error: SMXE_WRONG_MODE or SMXE_OUT_OF_XCBS */
         mp_PORTAL_RET(MP_ID_FPORTAL_CREATE, false);
         return false;
      }

      /* load portal name and sxchg into portal client structures */
      for (i = pclsz; i > 0; i--, pclp++)
      {
         (*pclp)->pname = pname;
         (*pclp)->sxchg = psh->sxchg;
      }
      psh->pname = pname;
      psh->ssn = ssn;
      psh->pshp = (FPSS**)*pshp;
      smx_HT_ADD(psh, pname);  /* for smxAware */
      mp_PORTAL_RET(MP_ID_FPORTAL_CREATE, true);
   }
   return pass;
}

/*
*  mp_FPortalDelete()
*
*  Stops server task, deletes server sxchg, clears portal name and sxchg 
*  handles in authorized client portal structures, and closes client portals.
*  Clears portal server structure.
*  Note: must be called from pmode. xsn is slot number for the task deleting
*  this portal.
*/
bool mp_FPortalDelete(FPSS* psh, FPCS** pclp, u32 pclsz, u8 xsn)
{
   u32  i;
   bool pass;

   pass = smx_TOKEN_TEST(smx_ct, (u32)psh->pshp, SMX_PRIV_HI);
   if (pass)
   {
      mp_PORTAL_LOG4(MP_ID_FPORTAL_DELETE, (u32)psh, (u32)pclp, pclsz, xsn);
      smx_TaskStop(psh->stask, SMX_TMO_INF);

      if (!smx_MsgXchgDelete(&psh->sxchg))
      {
         /* smx reported error: SMXE_INV_XCB */
         mp_PORTAL_RET(MP_ID_FPORTAL_DELETE, false);
         return false;
      }

      /* clear portal name and sxchg in portal client structures and close */
      for (i = pclsz; i > 0; i--, pclp++)
      {
         (*pclp)->pname = "none";
         (*pclp)->sxchg = NULL;
         if ((*pclp)->open)
            mp_FPortalClose(*pclp, xsn);
      }

      /* clear FPSS */
      psh->pname = "none";
      psh->sxchg = NULL;
      memset((void*)&psh->errno, 0, sizeof(FPSS)-MP_FPSS_ERRNUM_OFFSET);
      smx_HT_DELETE(psh);  /* for smxAware */
      mp_PORTAL_RET(MP_ID_FPORTAL_DELETE, true);
   }
   return pass;
}
#endif /* SMX_CFG_PORTAL */

/* Notes:
   1. mp_PortalLog() called via SVC which smx limits to 7 pars, which are
      call ID plus 6 pars.
*/
