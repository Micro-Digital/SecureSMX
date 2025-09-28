/*
* xtypes.h                                                  Version 5.4.0
*
* smx Data Types
*
* Copyright (c) 1989-2025 Micro Digital Inc.
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

#ifndef SMX_XTYPES_H
#define SMX_XTYPES_H

/*===========================================================================*
*                    CONTROL BLOCKS AND OTHER STRUCTURES                     *
*===========================================================================*/

/* These must be ahead of the struct defs below to avoid forward references. */
typedef struct BCB*   BCB_PTR;
typedef struct CB*    CB_PTR;
typedef struct EGCB*  EGCB_PTR;
typedef struct EQCB*  EQCB_PTR;
typedef struct LCB*   LCB_PTR;
typedef struct MCB*   MCB_PTR;
typedef struct MUCB*  MUCB_PTR;
typedef struct PICB*  PICB_PTR;
typedef struct TCB*   TCB_PTR;
typedef struct TMRCB* TMRCB_PTR;
typedef struct XCB*   XCB_PTR;

typedef struct BCB {       /* BLOCK CONTROL BLOCK */
   u8*         bp;            /* block pointer or free list link */
   PCB_PTR     ph;            /* block pool handle, NULL if none */
   void*       onr;           /* owner (task, LSR, or NULL) */
   BCB_PTR*    bhp;           /* block handle pointer */
} BCB, *BCB_PTR;

typedef struct CB {        /* GENERIC CONTROL BLOCK */
   CB_PTR      fl;            /* forward link */
   CB_PTR      bl;            /* backward link */
   SMX_CBTYPE  cbtype;        /* control block type */
   u8          pad8;
   u16         pad16;
   const char* name;          /* name */
} CB, *CB_PTR;

typedef struct CPS {       /* COARSE PROFILE STRUCTURE */
   bool        rdy;
   u32         idle;
   u32         work;
   u32         ovh;
} CPS;

typedef struct EGCB {      /* EVENT GROUP CONTROL BLOCK */
   TCB_PTR     fl;            /* forward link */
   TCB_PTR     bl;            /* backward link */
   SMX_CBTYPE  cbtype;        /* control block type (SMX_CB_EF) */
   u8          pad1;
   u16         pad2;
   u32         flags;         /* event flags */
   CBF_PTR     cbfun;         /* callback function */
   const char* name;          /* name */
   EGCB_PTR*   eghp;          /* event group handle pointer */
} EGCB, *EGCB_PTR;

typedef struct EQCB {      /* EVENT QUEUE CONTROL BLOCK */
   TCB_PTR     fl;            /* forward link */
   TCB_PTR     bl;            /* backward link */
   SMX_CBTYPE  cbtype;        /* control block type */
   u8          pad1;
   u8          pad2;
   u8          pad3;
   CBF_PTR     cbfun;         /* callback function */
   const char* name;          /* name */
   EQCB_PTR*   eqhp;          /* event queue handle pointer */
} EQCB, *EQCB_PTR;

typedef struct EREC {      /* ERROR RECORD FORMAT */
   u32         etime;         /* time of occurrence */
   u16         pad1;
   u8          pad2;
   SMX_ERRNO   err;           /* error number */
   void*       handle;        /* who caused or encountered the error */
} EREC, *EREC_PTR;

typedef struct HTREC {     /* HANDLE TABLE RECORD */
   void*       h;             /* handle */
   const char* name;          /* object name */
} HTREC, *HTREC_PTR;

typedef struct LQC {       /* LSR QUEUE CELL */
   LCB_PTR     lsr;           /* pointer to LSR control block */
   u32         par;           /* parameter to pass to LSR */
} LQC, *LQC_PTR;

/*
   IMPORTANT: If LCB is changed, change offsets in assembly .inc files.
*/
typedef struct LCB {       /* LSR CONTROL BLOCK */
   FUN_PTR     fun;           /* +00 LSR function pointer */
   LCB_PTR*    lhp;           /* +04 LSR handle pointer */
   SMX_CBTYPE  cbtype;        /* +08 control block type */
   struct {                   /* +09 flags */
      u8       trust : 1;        /* trusted LSR */
      u8       umode : 1;        /* run LSR in umode */
      u8       nolog : 1;        /* don't log in EVB */
   } flags;
   SMX_ERRNO   err;           /* +10 last error for this LSR */
#if SMX_CFG_SSMX
   u8          mpasz;         /* +11 MPA size */
#else
   u8          pad;           /* +11 */
#endif
   const char* name;          /* +12 name */
   TCB_PTR     htask;         /* +16 host task handle */
   u8*         stp;           /* +20 stack top pointer */
   u8*         sbp;           /* +24 stack bottom pointer */
#if SMX_CFG_SSMX
   MPR*        mpap;          /* +28 MPA pointer for LSR */
   MPR         sr;            /* +32 stack memory region */
#endif
} LCB, *LCB_PTR;

typedef struct MCB {       /* MESSAGE CONTROL BLOCK */
   CB_PTR      fl;            /* forward link */
   CB_PTR      bl;            /* backward link */
   SMX_CBTYPE  cbtype;        /* control block type */
   u8          pri;           /* message priority */
   u8          priv;          /* privilege level */
   u8          rpx;           /* reply index */
   u8*         bp;            /* block pointer */
   u32         bs;            /* block source */
   TCB_PTR     onr;           /* owner handle */
   MCB_PTR*    mhp;           /* message handle pointer */
#if SMX_CFG_SSMX            /* PMSG ADDED FIELDS */
  #if SB_CPU_ARMM7
   u32         rasr;          /* MPU rasr field */
   u16         pad2;
  #elif SB_CPU_ARMM8
   u32         rlar;          /* MPU rlar field */
   u8          shapxn;        /* SH, AP, and XN fields for v8 rbar */
   u8          pad2;
  #endif
   struct {                   /* pmsg control */
      u16      hsn : 4;          /* host slot number */
      u16      osn : 4;          /* owner slot number */
      u16      bnd : 1;          /* bound */
      u16      sb  : 1;          /* system block. pmsg is in mheap */
   } con;
   TCB_PTR     host;          /* host */
#endif
} MCB, *MCB_PTR;

typedef struct MUCB {      /* MUTEX CONTROL BLOCK */
   TCB_PTR     fl;            /* forward link */
   TCB_PTR     bl;            /* backward link */
   SMX_CBTYPE  cbtype;        /* control block type */
   u8          pad;
   u8          pi;            /* priority inheritance enabled */
   u8          ceil;          /* ceiling priority */
   const char* name;          /* name */
   TCB_PTR     onr;           /* owner */
   MUCB_PTR    molp;          /* next mutex in mutex owned list */
   u32         ncnt;          /* nesting count */
   MUCB_PTR*   muhp;          /* mutex handle pointer */
} MUCB;

/* PCB -- see bdef.h */

typedef struct PICB {      /* PIPE CONTROL BLOCK */
   TCB_PTR     fl;            /* forward link */
   TCB_PTR     bl;            /* backward link */
   SMX_CBTYPE  cbtype;        /* control block type */
   struct {                   /* flags */
      u8       full : 1;      /* pipe is full */
   } flags;
   u8          width;         /* pipe width (bytes) */
   u8          length;        /* pipe length (cells) */
   const char* name;          /* name */
   u8*         bi;            /* start of buffer */
   u8*         rp;            /* pipe read pointer */
   u8*         wp;            /* pipe write pointer */
   u8*         bx;            /* end of buffer */
   CBF_PTR     cbfun;         /* callback function */
   PICB_PTR*   php;           /* pipe handle pointer */
} PICB, *PICB_PTR;

typedef struct RQCB {      /* READY QUEUE CONTROL BLOCK */
   TCB_PTR     fl;            /* forward link */
   TCB_PTR     bl;            /* backward link */
   SMX_CBTYPE  cbtype;        /* control block type */
   u8          pad1;
   u8          pad2;
   u8          tplim : 7;     /* task priority lower limit */
   u8          tq    : 1;     /* task queue present (msb) */
   const char* name;          /* name */
} RQCB, *RQCB_PTR;

typedef struct SCB {       /* SEMAPHORE CONTROL BLOCK */
   TCB_PTR     fl;            /* forward link */
   TCB_PTR     bl;            /* backward link */
   SMX_CBTYPE  cbtype;        /* control block type */
   SMX_SEM_MODE mode;         /* operating mode */
   u8          count;         /* signal count */
   u8          lim;           /* count limit or threshold */
   CBF_PTR     cbfun;         /* callback function */
   const char* name;          /* name */
   SCB_PTR*    shp;           /* semaphore handle pointer */
} SCB, *SCB_PTR;

#define SMX_TCB_OFFS_SP     32   /* offset to TCB.sp field */
#define SMX_TCB_OFFS_SBP    36   /* offset to TCB.sbp field */
/*
   IMPORTANT: If TCB is changed, change offsets above and in assembly .inc files.
   Grep all XSMX files for each to find all places to change,
   including comments tagging where literals are used in the code.
   Offsets are numbered below as a reminder. Also change NULL in xglob.c.
*/

typedef struct TCB {       /* TASK CONTROL BLOCK */
   CB_PTR      fl;            /* +00 forward link */
   CB_PTR      bl;            /* +04 backward link */
   SMX_CBTYPE  cbtype;        /* +08 control block type */
   SMX_STATE   state;         /*     task state */
   SMX_ERRNO   err;           /*     last error for this task <1> */
   u8          indx;          /*     index in TCB array */
   const char* name;          /* +12 task name */
   u8          prinorm;       /* +16 normal priority */
   u8          pri;           /*     current priority */
   u8          pritmo;        /*     timeout priority */
   u8          exret;         /*     low byte of EXC_RETURN (ARMM) */
   struct {                   /* +20 flags */
      u32      ef_and : 1;       /* task waiting on AND of event flags */
      u32      ef_andor : 1;     /* task waiting on AND/OR of event flags */
      u32      hookd : 1;        /* entry and exit routines are hooked */
      u32      in_eq : 1;        /* task is in an event queue */
      u32      in_prq : 1;       /* in priority queue (except rq) */
      u32      mtx_wait : 1;     /* task waiting on mutex */
      u32      pipe_front : 1;   /* put packet to pipe front if 1, to back if 0 */
      u32      pipe_put : 1;     /* task waiting to put packet to pipe */
      u32      rv_r0 : 1;        /* copy ct->rv to r0 in exframe on task stack <2> */
      u32      stk_chk  : 1;     /* stack check enabled */
      u32      stk_hwmv : 1;     /* stack high water mark is valid */
      u32      stk_ovfl : 1;     /* stack has overflowed */
      u32      stk_perm : 1;     /* stack is permanent */
      u32      stk_preall : 1;   /* stack was preallocated */
      u32      strt_lockd : 1;   /* start task locked */
      u32      tok_ok : 1;       /* token test passed */
      u32      umode : 1;        /* unprivileged mode <2> */
      u32      priv_fixed : 1;   /* task privilege is fixed */
   } flags;
   u8*         spp;           /* +24 stack pad pointer */
   u8*         stp;           /* +28 stack top pointer -- last usable word */
   u8*         sp;            /* +32 stack pointer */
   u8*         sbp;           /* +36 stack bottom (initial) pointer and RSA pointer */
   u16         shwm;          /* +40 stack high-water mark */
   u16         ssz;           /*     stack size (not including pad, for comparison to shwm) */
   u32         rv;            /* +44 return value from last call */
   u32         sv;            /* +48 suspend/stop value or EG test flags */
   u32         sv2;           /* +52 suspend/stop value 2 or EG post-clear flags */

   FUN_PTR     fun;           /* +56 task main function pointer */
   CBF2_PTR    cbfun;         /* +60 callback function pointer */
   TCB_PTR*    thp;           /* +64 task handle pointer */
   void*       thisptr;       /* +68 this pointer for C++ class */

   MUCB_PTR    molp;          /* +72 mutex owned list pointer */
   void*       susploc;       /* +76 task suspend location, if suspended (for debug) */
   u8          srnest;        /* +80 srnest when enter PendSVH/PreSched from SSRExitInt() */
   u8          hn;            /*     heap number for perm stack */
   u8          priv;          /*     privilege level */
   u8          pad1;
   u32         rtc;           /* +84 runtime counter */
   u32         rtlim;         /* +88 runtime limit (or ptr to top parent's rtlim) */
   u32         rtlimctr;      /* +92 runtime limit counter (or ptr to top parent's rtlimctr) */

  #if SMX_CFG_SSMX
   TCB_PTR     parent;        /* +96 parent task */
   IRQ_PERM*   irq;           /*+100 IRQs task is allowed to mask, etc. */
   MPA*        mpatp;         /*+104 MPA template pointer for this task */
   MPR*        mpap;          /*+108 MPA pointer for this task */
   u8          mpasz;         /*+112 MPA size */
   u8          dsn;           /*     dual slot number (high nibble is aux slot) */
   u8          idle_ctr;      /*     counts idle passes per RTL frame */
   u8          pad2;
   u32*        tap;           /*+116 token array pointer */ 
  #endif

  #if defined(SMX_TXPORT)
   u32*        afp;           /* actual flags pointer */
  #endif
} TCB;

typedef struct TMRCB {     /* TIMER CONTROL BLOCK */
   TMRCB_PTR   fl;            /* forward link */
   TMRCB_PTR   bl;            /* backward link (unused) */
   SMX_CBTYPE  cbtype;        /* control block type */
   struct {                   /* flags */
      SMX_TMR_PS  state : 1;  /* pulse state (LO/HI) */
      SMX_TMR_OPT opt   : 2;  /* LSR parameter option */
      u8          os    : 1;  /* one shot timer (used by FRPort) */
      u8          act   : 1;  /* active timer (used by FRPort) */
   } flags;
   u16         count;         /* number of timeouts since last start */
   const char* name;          /* name */
   u32         diffcnt;       /* difference count from preceding timer */
   u32         nxtdly;        /* next delay */
   u32         period;        /* period for cyclic timer */
   u32         width;         /* pulse width */
   LCB_PTR     lsr;           /* LSR to be invoked at timeout */
   u32         par;           /* parameter to LSR */
   TMRCB_PTR*  tmhp;          /* timer handle pointer */
   TCB_PTR     onr;           /* timer owner handle */
} TMRCB, *TMRCB_PTR;

typedef struct XCB {       /* EXCHANGE CONTROL BLOCK */
   CB_PTR      fl;            /* forward link */
   CB_PTR      bl;            /* backward link */
   SMX_CBTYPE  cbtype;        /* control block type */
   SMX_XMODE   mode;          /* exchange mode */
   u8          pad;
   struct {                   /* flags */
      u8       mq : 1;        /* message queue present */
      u8       tq : 1;        /* task queue present */
      u8       pi : 1;        /* priority inheritance */
   } flags;
   TCB_PTR     onr;           /* owner if pi */
   CBF_PTR     cbfun;         /* callback function */
   const char* name;          /* name */
   XCB_PTR*    xhp;           /* exchange handle pointer */
} XCB, *XCB_PTR;


/* Notes:
   1. Do not move. See note 1 in xglob.c.
   2. Change SMX_TCB_FLAGS_RV_R0 or SMX_TCB_FLAGS_UMODE in xarmm_iar.inc if 
      flag position changes.
*/
#endif /* SMX_XTYPES_H */
