/*
* xdef.h                                                    Version 6.0.0
*
* smx constants, error numbers, and SSR IDs.
*
* This file defines general constants for use by all files.
*
* Copyright (c) 1989-2026 Micro Digital Inc.
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
* Authors: Ralph Moore, David Moore
*
*****************************************************************************/

#ifndef SMX_XDEF_H
#define SMX_XDEF_H

/*===========================================================================*
*                                 CONSTANTS                                  *
*===========================================================================*/

#define  SMX                           /* used in eheap and third-party code */
#define  SMX_VERSION       0x0600      /* change in xarmm_iar.inc <1> */

/* control block types */ 
typedef enum {
   SMX_CB_NULL,      /* 0x00 */
   SMX_CB_PCB,       /* 0x01 placeholder, not used */
   SMX_CB_BCB,       /* 0x02 */
   SMX_CB_MCB,       /* 0x03 */
   SMX_CB_PIPE,      /* 0x04 */ /* BQ <3> */
   SMX_CB_EG,        /* 0x05 */ /* BQ */
   SMX_CB_TQ,        /* 0x06 */
   SMX_CB_TMR,       /* 0x07 */
   SMX_CB_RQ,        /* 0x08 */ /* BQ */
   SMX_CB_SUBLV,     /* 0x09 */ /* BQ */
   SMX_CB_XCHG,      /* 0x0A */ /* BQ */
   SMX_CB_EQ,        /* 0x0B */ /* BQ */
   SMX_CB_SEM,       /* 0x0C */ /* BQ */
   SMX_CB_MTX,       /* 0x0D */ /* BQ */
   SMX_CB_TASK,      /* 0x0E */
   SMX_CB_PCBH,      /* 0x0F */
   SMX_CB_LSR,       /* 0x10 */
} __short_enum_attr SMX_CBTYPE; /*<2>*/

/* task callback function modes */
#define  SMX_CBF_ENTER     1     /* cbfun enter */
#define  SMX_CBF_EXIT      2     /* cbfun exit */
#define  SMX_CBF_START     3     /* cbfun start */
#define  SMX_CBF_STOP      4     /* cbfun stop */
#define  SMX_CBF_INIT      5     /* cbfun initialize */
#define  SMX_CBF_DELETE    6     /* cbfun delete */

/* scheduler modes */
typedef enum {
   SMX_CT_NOP,       /* no action */
   SMX_CT_TEST     = 0x20,       /* test ct */
   SMX_CT_SUSP     = 0x40,       /* suspend ct */
   SMX_CT_STOP     = 0x80,       /* stop ct <5> */
   SMX_CT_DELETE   = 0x100,      /* delete ct <5> */
   SMX_CT_TOP      = 0x7FFFFFFF  /* force 32-bit enum */
} SMX_SCHED;

/* event group flags */
#define  SMX_EF_OR         0           /* OR flag */
#define  SMX_EF_AND        1           /* AND flag */
#define  SMX_EF_ANDOR      2           /* ANDOR flag */

/* task and LSR create flags */
#define  SMX_FL_NONE       0x00000000
#define  SMX_FL_STRT_LOCKD 0x00000010
#define  SMX_FL_UMODE      0x00000020
#define  SMX_FL_CHILD      0x00000040
#define  SMX_FL_TRUST      0x00000080
#define  SMX_FL_NOLOG      0x00000100

/* peek parameters */
typedef enum {
   SMX_PK_BP,
   SMX_PK_CEIL,
   SMX_PK_COUNT,
   SMX_PK_DELAY,
   SMX_PK_DIFF_CNT,
   SMX_PK_ERROR,
   SMX_PK_ETIME,
   SMX_PK_ETIME_MS,
   SMX_PK_FILL,
   SMX_PK_FIRST,
   SMX_PK_FLAGS,
   SMX_PK_FREE,
   SMX_PK_FULL,
   SMX_PK_FUN,
   SMX_PK_HN,
   SMX_PK_INDEX,
   SMX_PK_INIT,
   SMX_PK_LAST,
   SMX_PK_LENGTH,
   SMX_PK_LIMIT,
   SMX_PK_LOCK,
   SMX_PK_LPAR,
   SMX_PK_LSR,
   SMX_PK_MAX,
   SMX_PK_MAX_DELAY,
   SMX_PK_MIN,
   SMX_PK_MODE,
   SMX_PK_MOLP,
   SMX_PK_MSG,
   SMX_PK_MTX,
   SMX_PK_NAME,
   SMX_PK_NCNT,
   SMX_PK_NEXT,
   SMX_PK_NUM,
   SMX_PK_NUMPKTS,
   SMX_PK_NUMTASKS,
   SMX_PK_ONR,
   SMX_PK_OPT,
   SMX_PK_PARENT,
   SMX_PK_PERIOD,
   SMX_PK_PI,
   SMX_PK_POOL,
   SMX_PK_PREV,
   SMX_PK_PRI,
   SMX_PK_PRINORM,
   SMX_PK_PRIV,
   SMX_PK_PULSE,
   SMX_PK_REPLY,
   SMX_PK_RTC,
   SMX_PK_RTLIM,
   SMX_PK_RTLIMCTR,
   SMX_PK_SEC,
   SMX_PK_SIZE,
   SMX_PK_STATE,
   SMX_PK_STIME,
   SMX_PK_TASK,
   SMX_PK_TIME_LEFT,
   SMX_PK_TLSP,
   SMX_PK_TMO,
   SMX_PK_UMODE,
   SMX_PK_XCHG,
   SMX_PK_WIDTH,
   SMX_PK_END
} SMX_PK_PAR;

/* pipe direction */
typedef enum {
   SMX_PUT_TO_BACK,
   SMX_PUT_TO_FRONT,
} SMX_PIPE_MODE;

/* semaphore types */
typedef enum {
   SMX_SEM_NULL,
   SMX_SEM_RSRC,     /* resource */
   SMX_SEM_EVENT,    /* event */
   SMX_SEM_THRES,    /* threshold */
   SMX_SEM_GATE,     /* gate */
   SMX_SEM_RDR,      /* reader (future) */
   SMX_SEM_WRTR      /* writer (future) */
} __short_enum_attr SMX_SEM_MODE;

/* set parameters */
typedef enum {
   SMX_ST_CBFUN,
   SMX_ST_FUN,
   SMX_ST_IRQ,
   SMX_ST_PRITMO,
   SMX_ST_PRIV,
   SMX_ST_RTLIM,
   SMX_ST_STK_CK,
   SMX_ST_STRT_LOCKD,
   SMX_ST_TAP,
   SMX_ST_UMODE
} SMX_ST_PAR;

/* task states */
typedef enum {
   SMX_TASK_NULL,
   SMX_TASK_WAIT  = 0x33,
   SMX_TASK_READY = 0x66,
   SMX_TASK_RUN   = 0x99,
   SMX_TASK_DEL   = 0xCC
} __short_enum_attr SMX_STATE;

/* timeouts */
#define  SMX_TMO_DFLT      10*(SMX_TICKS_PER_SEC)  /* default timeout */
#define  SMX_TMO_INF       0xFFFFFFFF  /* infinite timeout */
#define  SMX_TMO_NOCHG     0xFFFFFFFE  /* no change to task timer */
#define  SMX_TMO_NOWAIT    0           /* no timeout */

/* Timer pulse state */
typedef enum {
   SMX_TMR_LO,       /* low pulse */
   SMX_TMR_HI        /* high pulse */
} __short_enum_attr SMX_TMR_PS;

/* timer LSR par options */
typedef enum {
   SMX_TMR_PAR,      /* par -> LSR */
   SMX_TMR_STATE,    /* state -> LSR */
   SMX_TMR_TIME,     /* time -> LSR */
   SMX_TMR_COUNT     /* count -> LSR */
} __short_enum_attr SMX_TMR_OPT;

/* exchange modes */
typedef enum {
   SMX_XCHG_NULL,    /* none */
   SMX_XCHG_NORM,    /* normal */
   SMX_XCHG_PASS,    /* priority pass */
   SMX_XCHG_BCST     /* broadcast */
} __short_enum_attr SMX_XMODE;

/* misc */
#define  SMX_CT   (TCB_PTR)0xFFFFFFFF  /* use for smx_ct when called from umode */
#define  SMX_FL_MSEC       0x80000000  /* delay msec instead of ticks */
#define  SMX_HEAP_RETRY    0xFFFFFFFE  /* recall heap function because got mutex */
#define  SMX_PRI_NOCHG     0xFF        /* no change to priority */
#define  SMX_PRIV_LO       0           /* low privilege token */
#define  SMX_PRIV_HI       1           /* high privilege token */
#define  SMX_SS_INUSE      0x80000000  /* system Stack in use */
#define  SMX_PI            1           /* priority inheritance */

/*===========================================================================*
*                            smx ERROR TYPES <4>                             *
*===========================================================================*/

/* smx error types */
typedef enum {
   SMXE_OK,
   SMXE_TMO,
   SMXE_PRIV_VIOL,      /* also defined in xarmm_iar.inc */

   SMXE_ABORT,
   SMXE_BLK_IN_USE,
   SMXE_BROKEN_Q,
   SMXE_CLIB_ABORT,
   SMXE_EXCESS_LOCKS,
   SMXE_EXCESS_UNLOCKS,

   SMXE_HEAP_ALRDY_INIT,
   SMXE_HEAP_BRKN,
   SMXE_HEAP_ERROR,
   SMXE_HEAP_FENCE_BRKN,
   SMXE_HEAP_FIXED,
   SMXE_HEAP_INIT_FAIL,
   SMXE_HEAP_RECOVER,
   SMXE_HEAP_TIMEOUT,

   SMXE_HT_DUP,
   SMXE_HT_FULL,

   SMXE_INIT_MOD_FAIL,
   SMXE_INSUFF_HEAP,
   SMXE_INSUFF_UNLOCKS,

   SMXE_INV_BCB,
   SMXE_INV_CCB,
   SMXE_INV_EGCB,
   SMXE_INV_EQCB,
   SMXE_INV_FUNC,
   SMXE_INV_LCB,
   SMXE_INV_MCB,
   SMXE_INV_MUCB,
   SMXE_INV_OP,
   SMXE_INV_PAR,
   SMXE_INV_PCB,
   SMXE_INV_PICB,
   SMXE_INV_PRI,

   SMXE_INV_SCB,
   SMXE_INV_TCB,
   SMXE_INV_TIME,
   SMXE_INV_TMRCB,
   SMXE_INV_XCB,

   SMXE_LQ_OVFL,
   SMXE_LSR_NOT_OWN_MTX,
   SMXE_MTX_ALRDY_FREE,
   SMXE_MTX_NON_ONR_REL,
   SMXE_NO_ISR,
   SMXE_NOT_MSG_ONR,

   SMXE_OP_NOT_ALLOWED,

   SMXE_OUT_OF_BCBS,
   SMXE_OUT_OF_EGCBS,
   SMXE_OUT_OF_EQCBS,
   SMXE_OUT_OF_LCBS,
   SMXE_OUT_OF_MCBS,
   SMXE_OUT_OF_MUCBS,
   SMXE_OUT_OF_PCBS,
   SMXE_OUT_OF_PICBS,
   SMXE_OUT_OF_SCBS,
   SMXE_OUT_OF_STKS,
   SMXE_OUT_OF_TCBS,
   SMXE_OUT_OF_TMRCBS,
   SMXE_OUT_OF_XCBS,

   SMXE_POOL_EMPTY,
   SMXE_Q_FIXED,
   SMXE_RQ_ERROR,

   SMXE_SEM_CTR_OVFL,
   SMXE_SMX_INIT_FAIL,
   SMXE_SSR_IN_ISR,
   SMXE_STK_OVFL,
   SMXE_MSTK_OVFL,
   SMXE_TOKEN_VIOL,
   SMXE_TOO_MANY_HEAPS,
   SMXE_UNKNOWN_SIZE,

   SMXE_WAIT_NOT_ALLOWED,
   SMXE_WRONG_HEAP,
   SMXE_WRONG_MODE,
   SMXE_WRONG_POOL,

   /* smxBase error types */
   SBE_INSUFF_DAR,
   SBE_INV_ALIGN,
   SBE_INV_BP,
   SBE_INV_DAR,
   SBE_INV_LSR,
   SBE_INV_OFFSET,
   SBE_INV_POOL,
   SBE_INV_PRI,
   SBE_INV_SIZE,
   SBE_LQ_OVERFLOW,
   SBE_OUT_OF_LCBS,
   SBE_OUT_OF_PCBS,
   SBE_PRIV_VIOL,
   SBE_DAR_INIT_FAIL,
   SBE_INV_PAR,

   /* hardware faults */
   SBE_CPU_BF_VIOL,
   SBE_CPU_HF_VIOL,
   SBE_CPU_MMF_VIOL,
   SBE_CPU_UF_VIOL,

   #if SMX_CFG_SSMX
   /* ssmx portal error types */
   SPE_CLIENT_TMO, 
   SPE_INV_TYPE, 
   SPE_INV_CMD, 
   SPE_INV_FCT, 
   SPE_INV_SID, 
   SPE_INV_SSID, 
   SPE_INV_SZ, 
   SPE_NO_PMSG, 
   SPE_PORTAL_CLOSED, 
   SPE_PORTAL_NEXIST, 
   SPE_PORTAL_NOPEN,    
   SPE_SERVER_TMO, 
   SPE_TRANS_ERR, 
   SPE_TRANS_INC,
   #endif

   SMX_NUM_ERRORS
} __short_enum_attr SMX_ERRNO;

/*===========================================================================*
*                                 SSR IDS                                    *
*===========================================================================*/
/*
   Format: 0xMMSSPIII

      M = Module (1 byte)
          01 = smx kernel and smxBase (bdef.h)
          (others for middleware modules defined elsewhere)
      S = SSR Group. Enables EVB logging by groups SG1-8 (bit n + 1).
      P = Number of Parameters (4 bits)
      I = Function ID (12 bits)

   Notes:

   1. IDs are needed for SSRs (including any non-API SSRs) in order to
      appear by name in smxAware traces. Pass ID 0 to smx_SSR_ENTERn()
      for don't care. IDs are also needed for any API functions to make
      accessible to smxDLM.
   2. LAST marks the last function number used. Move this marker as new
      functions are added (since they will be added in alphabetical order).
   3. There should not be gaps in function numbers because these will be
      used in jump tables. Gaps may occur if functions are deleted.
      Remember numbers are hexadecimal; be careful not to skip A-F.
   4. All SSRs are in SSR Group 1 (SG1) by default. Selected SSRs can be
      moved to SG1-8 in order to selectively enable or disable logging them.
      Logging is controlled by setting SG bits in smx_evben. SSRs put into
      SG0 (=0) will never be logged.
*/

#define  SMX_ID_MASK_MODULE               0xFF000000
#define  SMX_ID_MASK_SSRGRP               0x00FF0000
#define  SMX_ID_MASK_NUMPAR               0x0000F000
#define  SMX_ID_MASK_FUNCID               0x00000FFF

#define  SMX_ID_BLOCK_GET                 0x01014001
#define  SMX_ID_BLOCK_MAKE                0x01013002
#define  SMX_ID_BLOCK_PEEK                0x01012003
#define  SMX_ID_BLOCK_REL                 0x01012004
#define  SMX_ID_BLOCK_REL_ALL             0x01011005
#define  SMX_ID_BLOCK_UNMAKE              0x01012006

#define  SMX_ID_BP_CREATE                 0x0101500A
#define  SMX_ID_BP_DELETE                 0x0101100B
#define  SMX_ID_BP_PEEK                   0x0101200C

#define  SMX_ID_EF_PULSE                  0x01012010
#define  SMX_ID_EF_SET                    0x01013011
#define  SMX_ID_EF_TEST                   0x01015012
#define  SMX_ID_EF_TEST_STOP              0x01015013

#define  SMX_ID_EG_CLEAR                  0x01012018
#define  SMX_ID_EG_CREATE                 0x01013019
#define  SMX_ID_EG_DELETE                 0x0101101A
#define  SMX_ID_EG_PEEK                   0x0101201B
#define  SMX_ID_EG_SET                    0x0101401C

#define  SMX_ID_EQ_CLEAR                  0x01011020
#define  SMX_ID_EQ_COUNT                  0x01013021
#define  SMX_ID_EQ_COUNT_STOP             0x01013022
#define  SMX_ID_EQ_CREATE                 0x01020023
#define  SMX_ID_EQ_DELETE                 0x01011024
#define  SMX_ID_EQ_SIGNAL                 0x01011025
#define  SMX_ID_EQ_PEEK                   0x01012026
#define  SMX_ID_EQ_SET                    0x01014027

#define  SMX_ID_HEAP_BIN_PEEK             0x01013030
#define  SMX_ID_HEAP_BIN_SCAN             0x01804031
#define  SMX_ID_HEAP_BIN_SEED             0x01013032
#define  SMX_ID_HEAP_BIN_SORT             0x01803033
#define  SMX_ID_HEAP_CALLOC               0x01804034
#define  SMX_ID_HEAP_CHUNK_PEEK           0x01013035
#define  SMX_ID_HEAP_EXTEND               0x01013036
#define  SMX_ID_HEAP_FREE                 0x01802037
#define  SMX_ID_HEAP_INIT                 0x01017038
#define  SMX_ID_HEAP_MALLOC               0x0180303A
#define  SMX_ID_HEAP_PEEK                 0x0101203B
#define  SMX_ID_HEAP_REALLOC              0x0180403C
#define  SMX_ID_HEAP_RECOVER              0x0101203D
#define  SMX_ID_HEAP_SCAN                 0x0180403E
#define  SMX_ID_HEAP_SET                  0x0101303F

#define  SMX_ID_LSR_CREATE                0x01016048
#define  SMX_ID_LSR_INVOKE                0x01012049
#define  SMX_ID_LSRS_OFF                  0x0101004A
#define  SMX_ID_LSRS_ON                   0x0101004B
#define  SMX_ID_LSR_DELETE                0x0101104C

#define  SMX_ID_MSG_BUMP                  0x01012050
#define  SMX_ID_MSG_GET                   0x01014051
#define  SMX_ID_MSG_MAKE                  0x01013052
#define  SMX_ID_MSG_PEEK                  0x01012053
#define  SMX_ID_MSG_RECEIVE               0x01014054
#define  SMX_ID_MSG_RECEIVE_STOP          0x01014055
#define  SMX_ID_MSG_REL                   0x01012056
#define  SMX_ID_MSG_REL_ALL               0x01011057
#define  SMX_ID_MSG_SEND                  0x01014058
#define  SMX_ID_MSG_UNMAKE                0x01012059

#define  SMX_ID_MSG_XCHG_CLEAR            0x0101105A
#define  SMX_ID_MSG_XCHG_CREATE           0x0101405B
#define  SMX_ID_MSG_XCHG_DELETE           0x0101105C
#define  SMX_ID_MSG_XCHG_PEEK             0x0101205D
#define  SMX_ID_MSG_XCHG_SET              0x0101405E

#define  SMX_ID_MUTEX_CLEAR               0x01011060
#define  SMX_ID_MUTEX_CREATE              0x01014061
#define  SMX_ID_MUTEX_DELETE              0x01011062
#define  SMX_ID_MUTEX_FREE                0x01011063
#define  SMX_ID_MUTEX_GET                 0x01012064
#define  SMX_ID_MUTEX_GET_STOP            0x01012065
#define  SMX_ID_MUTEX_REL                 0x01011066
#define  SMX_ID_MUTEX_PEEK                0x01012067
#define  SMX_ID_MUTEX_SET                 0x01014068

#define  SMX_ID_PBLK_GET_HEAP             0x01015070
#define  SMX_ID_PBLK_GET_POOL             0x01014071
#define  SMX_ID_PBLK_MAKE                 0x01015072
#define  SMX_ID_PBLK_REL_HEAP             0x01013073
#define  SMX_ID_PBLK_REL_POOL             0x01014074

#define  SMX_ID_PMSG_GET_HEAP             0x01016078
#define  SMX_ID_PMSG_GET_POOL             0x01015079
#define  SMX_ID_PMSG_MAKE                 0x0101607A
#define  SMX_ID_PMSG_RECEIVE              0x0101507B
#define  SMX_ID_PMSG_RECEIVE_STOP         0x0101507C
#define  SMX_ID_PMSG_REL                  0x0101207D
#define  SMX_ID_PMSG_SEND                 0x0101407E

#define  SMX_ID_PIPE_CLEAR                0x01011080
#define  SMX_ID_PIPE_CREATE               0x01015081
#define  SMX_ID_PIPE_DELETE               0x01011082
#define  SMX_ID_PIPE_GET_PKT_WAIT         0x01013083
#define  SMX_ID_PIPE_GET_PKT_WAIT_STOP    0x01013084
#define  SMX_ID_PIPE_PEEK                 0x01012085
#define  SMX_ID_PIPE_PUT_PKT_WAIT         0x01014086
#define  SMX_ID_PIPE_PUT_PKT_WAIT_STOP    0x01014087
#define  SMX_ID_PIPE_RESUME               0x01011088
#define  SMX_ID_PIPE_SET                  0x01014089

#define  SMX_ID_SEM_CLEAR                 0x01011090
#define  SMX_ID_SEM_CREATE                0x01014091
#define  SMX_ID_SEM_DELETE                0x01011092
#define  SMX_ID_SEM_PEEK                  0x01012093
#define  SMX_ID_SEM_SET                   0x01014094
#define  SMX_ID_SEM_SIGNAL                0x01011095
#define  SMX_ID_SEM_TEST                  0x01012096
#define  SMX_ID_SEM_TEST_STOP             0x01012097

#define  SMX_ID_SYS_PEEK                  0x0101109A
#define  SMX_ID_SYS_POWER_DOWN            0x0100109B
#define  SMX_ID_SYS_WHAT_IS               0x0101109C

#define  SMX_ID_TASK_BUMP                 0x010120A0
#define  SMX_ID_TASK_CREATE               0x010170A1
#define  SMX_ID_TASK_DELETE               0x010110A2
#define  SMX_ID_TASK_LOCATE               0x010110A3
#define  SMX_ID_TASK_LOCK                 0x010100A4
#define  SMX_ID_TASK_LOCK_CLEAR           0x010200A5
#define  SMX_ID_TASK_PEEK                 0x010120A6
#define  SMX_ID_TASK_RESUME               0x010110A7
#define  SMX_ID_TASK_SET                  0x010140A8
#define  SMX_ID_TASK_SLEEP                0x010110A9
#define  SMX_ID_TASK_SLEEP_STOP           0x010110AA
#define  SMX_ID_TASK_START                0x010120AB
#define  SMX_ID_TASK_START_NEW            0x010140AC
#define  SMX_ID_TASK_STOP                 0x010120AD
#define  SMX_ID_TASK_SUSPEND              0x010120AE
#define  SMX_ID_TASK_UNLOCK               0x010100AF
#define  SMX_ID_TASK_UNLOCK_QUICK         0x010100B0

#define  SMX_ID_TIMER_DUP                 0x010130B4
#define  SMX_ID_TIMER_PEEK                0x010120B5
#define  SMX_ID_TIMER_RESET               0x010120B6
#define  SMX_ID_TIMER_SET_LSR             0x010140B7
#define  SMX_ID_TIMER_SET_PULSE           0x010130B8
#define  SMX_ID_TIMER_START               0x010150B9
#define  SMX_ID_TIMER_START_ABS           0x010150BA
#define  SMX_ID_TIMER_STOP                0x010120BB
#define  SMX_ID_END                       0x010100BC

/* Notes:
   1. Version numbers are of the form XX.X.X. Using the hex scheme above,
      digits up to 15 (0xF) can be represented. smxVersion is assigned
      this value in xglob.c. Defining this macro allows conditioning on
      the version number, if desired.
   2. 8-bit enums are necessary for some control block fields in order to 
      optimize control block sizes. If not available, control blocks will be 
      larger.
   3. Numbering of control block types is important for TEST macros. BQ marks 
      types tested by smx_TEST_BRKNQ().
   4. See Glossary in smx Reference Manual for more information on error types.
   5. Must also change in xarmm_iar.inc.
*/
#endif /* SMX_XDEF_H */
