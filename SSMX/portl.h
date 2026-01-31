/*
* portl.h                                                   Version 6.0.0
*
* Portal definitions.
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

#ifndef MP_PORTL_H
#define MP_PORTL_H

#if SMX_CFG_PORTAL

/*===========================================================================*
*                              Portal Typedefs                               *
*===========================================================================*/
#define  PERRNO   SMX_ERRNO

/* portal types */
typedef enum {
         NOTYPE,
         FREEMSG, /* free message portal */
         TUNNEL,  /* tunnel portal */
} PTYPE;

/* portal commands */
typedef enum {NOP, OPEN, SEND, RECEIVE, CLOSE, CONTROL} PCMD;

/* server function numbers */
typedef enum {
         NOSVR,
         CP,      /* console portal */
         FP,      /* FatFs portal */
         TSTPP,   /* tsmx test pmode portal */
         TSTUP,   /* tsmx test umode portal */
} SID;

/* pmsg headers */
typedef struct TPMH {       /* TUNNEL PORTAL MESSAGE HEADER */
   PTYPE       type;          /* portal type */
   PCMD        cmd;           /* portal command */
   struct {                   /* portal control */
      u8       eod  : 1;         /* end of data */
      u8       sod  : 1;         /* start of data */
   } con;
   PERRNO      errno;         /* portal error number */
   u32         mdsz;          /* pmsg data size */
   u32         rqsz;          /* request size */
   u32         cmpsz;         /* completed size */
} TPMH;

typedef struct TOMH {       /* TUNNEL OPEN MESSAGE HEADER */
   PTYPE       type;          /* portal type */
   PCMD        cmd;           /* portal command */
   struct {                   /* portal control */
      u8       eod  : 1;         /* end of data */
      u8       sod  : 1;         /* start of data */
   } con;
   PERRNO      errno;         /* portal error number */
   u32         mdsz;          /* pmsg data size */
   u32         rqsz;          /* request size */
   /* open info */
   SCB*        csem;          /* client semaphore */
   SCB*        ssem;          /* server semaphore */
   u32         thsz;          /* total header size */
} TOMH;

/*===========================================================================*
*                             Portal Structures                              *
*===========================================================================*/

typedef struct PS {        /* GENERIC PORTAL STRUCTURE */
   const char* pname;         /* portal name */
   XCB*        sxchg;         /* server exchange */
} PS;

#define MP_FPCS_ERRNUM_OFFSET 16

typedef struct FPCS {      /* FREE PMSG PORTAL CLIENT STRUCTURE */
   const char* pname;         /* portal name */
   XCB*        sxchg;         /* server exchange (alias) */
   MCB*        pmsg;          /* pmsg handle if held by ct */
   XCB*        rxchg;         /* resource exchange */
   PERRNO      errno;         /* error number of last error */
   u8          csn;           /* client slot number for pmsg */
   u8          pri;           /* pmsg priority */
   u8          num;           /* number of pmsgs existing for this client */
   u8          open;          /* portal is open <2> */
   u8          pad1;
   u16         pad2;
   u32         tmo;           /* timeout for rxchg */
   u32         data;          /* client data or pointer to it (for shell use) */
} FPCS;

#define MP_FPSS_ERRNUM_OFFSET 12

typedef struct FPSS {      /* FREE PMSG PORTAL SERVER STRUCTURE */
   const char* pname;         /* portal name */
   XCB*        sxchg;         /* server exchange */
   TCB*        stask;         /* server task */
   PERRNO      errno;         /* error number of last error */
   u8          ssn;           /* server slot number for portal */
   u16         pad;
   void*       shp;           /* service header ptr */
   FPSS**      pshp;          /* free portal structure handle pointer */
} FPSS;

typedef struct TPCS {       /* TUNNEL PORTAL CLIENT STRUCTURE */
   const char* pname;         /* portal name <1> */
   XCB*        sxchg;         /* server exchange (alias) <1> */
   MCB*        pmsg;          /* portal msg */
   SCB*        csem;          /* client semaphore */
   SCB*        ssem;          /* server semaphore */
   TPMH*       mhp;           /* message header ptr <4> */
   u8          open;          /* portal is open <2> */
   u8          pad1;
   u16         pad2;
   void*       shp;           /* service header ptr */
   u8*         mdp;           /* message data ptr -- same as TPSS */
   u32         mdsz;          /* message data size */
   u32         data;          /* client data or pointer to it (for shell use) */
} TPCS;

#define MP_TPSS_MHP_OFFSET 28

typedef struct TPSS {      /* TUNNEL PORTAL SERVER STRUCTURE */
   const char* pname;         /* portal name */
   XCB*        sxchg;         /* server exchange */
   MCB*        pmsg;          /* portal msg */
   SCB*        csem;          /* client semaphore (alias) */
   SCB*        ssem;          /* server semaphore (alias) */
   TCB*        stask;         /* server task */
   u8          dsn;           /* dual server slot number for portal */
   SID         sid;           /* server ID */
   u8          ssid;          /* subserver ID */ 
   u8          open;          /* portal is open */
   TPMH*       mhp;           /* message header ptr -- same as TPCS */
   void*       shp;           /* service header ptr */
   u8*         mdp;           /* message data ptr */
   u32         mdsz;          /* message data size */
   TPSS**      pshp;          /* tunnel portal structure handle pointer */
} TPSS;

/*===========================================================================*
*                       Portal API And Other Routines                        *
*===========================================================================*/

void  mp_SetDAF(u32 n); /* set deferred action function */

#ifdef __cplusplus
extern "C" {

/*------------------- for C++ with default parameters ----------------------*/

/* client */
bool  mp_FPortalClose(FPCS* pch, u8 xsn=0);
bool  mp_FPortalOpen(FPCS* pch, u8 csn, u32 msz, u32 nmsg, 
                                  u32 tmo=SMX_TMO_INF, const char* rxname=NULL);
MCB*  mp_FPortalReceive(FPCS* pch, u8** dpp=NULL);
bool  mp_FPortalSend(FPCS* pch, MCB* pmsg);
bool  mp_FTPortalSend(FPCS* pch, u8* bp, MCB* pmsg);

#define  mp_TPortalCall(pch, tmo) mp_TPortalSend(pch, 0, 0, tmo)
bool  mp_TPortalClose(TPCS* pch);
bool  mp_TPortalOpen(TPCS* pch, u32 msz, u32 thsz, u32 tmo=SMX_TMO_INF,
                              const char* ssname=NULL, const char* csname=NULL);
bool  mp_TPortalReceive(TPCS* pch, u8* dp, u32 rqsz, u32 tmo=0);
bool  mp_TPortalSend(TPCS* pch, u8* dp=NULL, u32 rqsz=0, u32 tmo=0);

/* server */
bool  mp_FPortalCreate(FPSS** pshp, FPCS** pclp, u32 pclsz, u8 ssn, 
                                const char* pname=NULL, const char* sxname=NULL);
bool  mp_FPortalDelete(FPSS* pshp, FPCS** pclp, u32 pclsz, u8 xsn=0);
bool  mp_TPortalCreate(TPSS** pshp, TPCS** pclp, u32 pclsz, u8 dsn, 
                                const char* pname=NULL, const char* sxname=NULL);
bool  mp_TPortalDelete(TPSS* pshp, TPCS** pclp, u32 pclsz);
void  mp_TPortalServer(TPSS* pshp, u32 stmo);

/* general */
void  mp_PortalEM(PS* pch, PERRNO errno, PERRNO* ep);
void  mp_PortalLog(u32 id, u32 p1=0, u32 p2=0, u32 p3=0, u32 p4=0, u32 p5=0, u32 p6=0);
void  mp_PortalRet(u32 id, u32 rv);
}
#else  /*--------------- for C without default parameters -------------------*/

/* Client */
bool  mp_FPortalClose(FPCS* pch, u8 xsn);
bool  mp_FPortalOpen(FPCS* pch, u8 csn, u32 msz, u32 nmsg, u32 tmo, const char* rxname);
MCB*  mp_FPortalReceive(FPCS* pch, u8** dpp);
bool  mp_FPortalSend(FPCS* pch, MCB* pmsg);
bool  mp_FTPortalSend(FPCS* pch, u8* bp, MCB* pmsg);

#define  mp_TPortalCall(pch, tmo) mp_TPortalSend(pch, 0, 0, tmo)
bool  mp_TPortalClose(TPCS* pch);
bool  mp_TPortalOpen(TPCS* pch, u32 msz, u32 thsz, u32 tmo,
                              const char* ssname, const char* csname);
bool  mp_TPortalReceive(TPCS* pch, u8* dp, u32 rqsz, u32 tmo);
bool  mp_TPortalSend(TPCS* pch, u8* dp, u32 rqsz, u32 tmo);

/* Server */
bool  mp_FPortalCreate(FPSS** pshp, FPCS** pclp, u32 pclsz, u8 ssn, 
                                const char* pname, const char* sxname);
bool  mp_FPortalDelete(FPSS* psh, FPCS** pclp, u32 pclsz, u8 xsn);
bool  mp_TPortalCreate(TPSS** pshp, TPCS** pclp, u32 pclsz, u8 dsn, 
                                const char* pname, const char* sxname);
bool  mp_TPortalDelete(TPSS* psh, TPCS** pclp, u32 pclsz);
void  mp_TPortalServer(TPSS* psh, u32 stmo);

/* General */
void  mp_PortalEM(PS* pch, PERRNO errno, PERRNO* ep);
void  mp_PortalLog(u32 id, u32 p1, u32 p2, u32 p3, u32 p4, u32 p5, u32 p6);
void  mp_PortalRet(u32 id, u32 rv);
#endif /* __cplusplus */

/*===========================================================================*
*                                Portal Macros                               *
*===========================================================================*/

#define MP_NPCL(pcl) (sizeof(pcl)/sizeof(u32*))

/* service header load macros */
#define mp_SHL0(id, e) \
   { \
      shp->fid = id; \
      shp->ret = e; /* preload ret val in case portal operation itself fails */ \
      mp_PTL_CALLER_SHL(); \
   }

#define mp_SHL1(id, par1, e) \
   { \
      shp->fid = id; \
      shp->p1  = par1; \
      shp->ret = e; \
      mp_PTL_CALLER_SHL(); \
   }

#define mp_SHL2(id, par1, par2, e) \
   { \
      shp->fid = id; \
      shp->p1  = par1; \
      shp->p2  = par2; \
      shp->ret = e; \
      mp_PTL_CALLER_SHL(); \
   }

#define mp_SHL3(id, par1, par2, par3, e) \
   { \
      shp->fid = id; \
      shp->p1  = par1; \
      shp->p2  = par2; \
      shp->p3  = par3; \
      shp->ret = e; \
      mp_PTL_CALLER_SHL(); \
   }

#define mp_SHL4(id, par1, par2, par3, par4, e) \
   { \
      shp->fid = id; \
      shp->p1  = par1; \
      shp->p2  = par2; \
      shp->p3  = par3; \
      shp->p4  = par4; \
      shp->ret = e; \
      mp_PTL_CALLER_SHL(); \
   }

#define mp_SHL5(id, par1, par2, par3, par4, par5, e) \
   { \
      shp->fid = id; \
      shp->p1  = par1; \
      shp->p2  = par2; \
      shp->p3  = par3; \
      shp->p4  = par4; \
      shp->p5  = par5; \
      shp->ret = e; \
      mp_PTL_CALLER_SHL(); \
   }

#define mp_SHL6(id, par1, par2, par3, par4, par5, par6, e) \
   { \
      shp->fid = id; \
      shp->p1  = par1; \
      shp->p2  = par2; \
      shp->p3  = par3; \
      shp->p4  = par4; \
      shp->p5  = par5; \
      shp->p6  = par6; \
      shp->ret = e; \
      mp_PTL_CALLER_SHL(); \
   }

#if defined(SMX_DEBUG)
#define mp_PTL_CALLER_SAV()   void* caller = (void*)__get_LR()
#define mp_PTL_CALLER_SHL()   shp->caller = caller;
#else
#define mp_PTL_CALLER_SAV()
#define mp_PTL_CALLER_SHL()
#endif

#if SMX_CFG_EVB
/*===========================================================================*
*                                Event Buffer                                *
*===========================================================================*/

/* Non-SVC IDs (>= 0x100). See xdef.h for ID format. */
#define  MP_ID_FPORTAL_CLOSE              0x02002100
#define  MP_ID_FPORTAL_CREATE             0x02006101
#define  MP_ID_FPORTAL_DELETE             0x02004102
#define  MP_ID_FPORTAL_OPEN               0x02006103
#define  MP_ID_FPORTAL_RECEIVE            0x02002104
#define  MP_ID_FPORTAL_SEND               0x02002105
#define  MP_ID_FTPORTAL_SEND              0x02003106
#define  MP_ID_TPORTAL_CLOSE              0x02001107
#define  MP_ID_TPORTAL_CREATE             0x02006108
#define  MP_ID_TPORTAL_DELETE             0x02003109
#define  MP_ID_TPORTAL_OPEN               0x0200610A
#define  MP_ID_TPORTAL_RECEIVE            0x0200410B
#define  MP_ID_TPORTAL_SEND               0x0200410C
#define  MP_ID_TPORTAL_SERVER             0x0200210D  /*<3>*/

/* EVB portal EVB logging macros */
#define mp_EVB_LOG_PORTAL_ERROR(errno, task, ph) \
   { \
      if (smx_evben & SMX_EVB_EN_PERR) \
      { \
         u32 istate, *p; \
         istate = sb_IntStateSaveDisable(); \
         p = (smx_evbn > smx_evbx) ? smx_evbi : smx_evbn; \
         *p++ = 0x55550005 | SMX_EVB_RT_PERR; \
         *p++ = sb_PtimeGet(); \
         *p++ = smx_etime; \
         *p++ = (u32)(errno); \
         *p++ = (u32)(task); \
         *p++ = (u32)(ph); \
         smx_evbn = p; \
         sb_IntStateRestore(istate); \
      } \
   }

#define  mp_PORTAL_LOG0(id) \
                  mp_PortalLog(id, 0, 0, 0, 0, 0, 0)
#define  mp_PORTAL_LOG1(id, p1) \
                  mp_PortalLog(id, (u32)p1, 0, 0, 0, 0, 0)
#define  mp_PORTAL_LOG2(id, p1, p2) \
                  mp_PortalLog(id, (u32)p1, (u32)p2, 0, 0, 0, 0)
#define  mp_PORTAL_LOG3(id, p1, p2, p3) \
                  mp_PortalLog(id, (u32)p1, (u32)p2, (u32)p3, 0, 0, 0)
#define  mp_PORTAL_LOG4(id, p1, p2, p3, p4) \
                  mp_PortalLog(id, (u32)p1, (u32)p2, (u32)p3, (u32)p4, 0, 0)
#define  mp_PORTAL_LOG5(id, p1, p2, p3, p4, p5) \
                  mp_PortalLog(id, (u32)p1, (u32)p2, (u32)p3, (u32)p4, (u32)p5, 0)
#define  mp_PORTAL_LOG6(id, p1, p2, p3, p4, p5, p6) \
                  mp_PortalLog(id, (u32)p1, (u32)p2, (u32)p3, (u32)p4, (u32)p5, (u32)p6)
#define  mp_PORTAL_RET(id, rv) \
                  mp_PortalRet(id, rv)
#endif /* SMX_CFG_EVB */

/*
   Notes:
   1. Must be in the same position in all portal structures.
   2. Must be in the same position in TPCS and FPCS.
   3. MP_ID_TPORTAL_SERVER indicates 2 pars, but it logs values not pars,
      like USER events.
   4. Do not clear as long as pmsg is valid.
*/
#endif /* SMX_CFG_PORTAL */
#endif /* MP_PORTL_H */