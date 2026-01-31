/*
* bdef.h                                                    Version 6.0.0
*
* smxBase Definitions.
*
* Copyright (c) 2004-2026 Micro Digital Inc.
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
* Authors: David Moore, Ralph Moore
*
*****************************************************************************/

#ifndef SB_BDEF_H
#define SB_BDEF_H

#define SB_VERSION 0x0600
/*
   Version numbers are of the form XX.X.X. Using the hex scheme above,
   digits up to 15 (0xF) can be represented.
*/

/*===========================================================================*
*                               BASIC DEFINES                                *
*===========================================================================*/

#ifdef  NULL
#undef  NULL
#endif
#define NULL                     (void*)0    /*<1>*/
#define OFF                      0
#define ON                       1
#define SB_STK_FILL_VAL          0x55555555  /* stack fill value to check usage */
#define SB_TMO_INF               0xFFFFFFFF  /* infinite timeout */
#define STATIC                   static

/* compiler macros */
#define __inline__               inline
#define __interdecl
#define __interrupt
#define __packed                 __packed
#define __packed_gnu
#define __packed_pragma          1
#define __short_enum_attr
#define __unaligned

/*===========================================================================*
*                             BASIC DATA TYPES                               *
*===========================================================================*/

typedef  const char              cch;
typedef  const char*             ccp;

#if (SB_CFG_FIXED_WIDTH_TYPES)
#include <stdint.h>
typedef  int8_t                  s8;
typedef  int16_t                 s16;
typedef  int32_t                 s32;
typedef  uint8_t                 u8;
typedef  uint16_t                u16;
typedef  uint32_t                u32;
#else
typedef  signed   char           s8;
typedef  signed   short int      s16;
typedef  signed   long int       s32;
typedef  unsigned char           u8;
typedef  unsigned short int      u16;
typedef  unsigned long int       u32;
#endif

typedef  u32                     uint;

typedef  volatile s8             vs8;
typedef  volatile s16            vs16;
typedef  volatile s32            vs32;
typedef  volatile u8             vu8;
typedef  volatile u16            vu16;
typedef  volatile u32            vu32;

#if (SB_CFG_FIXED_WIDTH_TYPES)
typedef  int64_t                 s64;
typedef  uint64_t                u64;
#else
typedef  signed   long long int  s64;
typedef  unsigned long long int  u64;
#endif
typedef  volatile s64            vs64;
typedef  volatile u64            vu64;

typedef  float                   f32;
typedef  double                  f64;
typedef  volatile bool           vbool;

#ifndef __cplusplus /*<2>*/
#define  bool    _Bool
#define  true    1
#define  false   0
#endif

/*===========================================================================*
*                             OTHER DATA TYPES                               *
*===========================================================================*/

/* function pointer types */
typedef  bool (*CBF_PTR)(u32);
typedef  bool (*CBF2_PTR)(u32, u32);
typedef  void (*FUN_PTR)(u32);
typedef  void (*FUNV_PTR)(void);
typedef  void (*ISR_PTR)(void);
typedef  int  (*ISRC_PTR)(uint);
typedef  void (*LSR_PTR)(u32);

typedef struct
{
   u16 wYear;
   u16 wMonth;
   u16 wDay;
   u16 wHour;
   u16 wMinute;
   u16 wSecond;
   u16 wMilliseconds;
} DATETIME;

/* control block types */
typedef enum {
         SB_CB_NULL,       /* 0 */
         SB_CB_PCB,        /* 1 Must be same as SMX_CB_PCB */
         SB_CB_DCB         /* 2 */
} __short_enum_attr SB_CBTYPE;

/* smxBase error types -- see SMX_ERRNO in xdef.h */

/* smxBase peek parameters */
typedef enum {
         SB_PK_BP,
         SB_PK_COUNT,
         SB_PK_ERROR,
         SB_PK_FIRST,
         SB_PK_FLAGS,
         SB_PK_FREE,
         SB_PK_INDEX,
         SB_PK_LAST,
         SB_PK_LIMIT,
         SB_PK_MAX,
         SB_PK_MIN,
         SB_PK_MODE,
         SB_PK_MSG,
         SB_PK_NAME,
         SB_PK_NEXT,
         SB_PK_NUM,
         SB_PK_NUM_USED,
         SB_PK_POOL,
         SB_PK_SIZE,
         SB_PK_STATE,
         SB_PK_TICK_INIT_DONE,
         SB_PK_TICKTMR_CLKHZ,
         SB_PK_TICKTMR_CNTPT,
} SB_PK_PAR;

/*===========================================================================*
*                              CONTROL BLOCKS                                *
*===========================================================================*/

#if defined(SCB)  /* defined in ARM CMSIS core_*.h */
#undef SCB  
#endif

typedef struct SCB*  SCB_PTR;
typedef struct PCB*  PCB_PTR;

typedef struct SB_DCB {    /* DYNAMICALLY ALLOCATED REGION CONTROL BLOCK */
   u8 *        pi;            /* pointer to first word of DAR */
   u8 *        pl;            /* pointer to last block allocated */
   u8 *        pn;            /* pointer to next free block */
   u8 *        px;            /* pointer to last word of DAR */
} SB_DCB, *SB_DCB_PTR;

typedef struct PCB {       /* POOL CONTROL BLOCK -- shared with smx */
   SB_CBTYPE   cbtype;        /* control block type = SB_CB_PCB */
   u8          pad1;
   u16         num;           /* number of blocks in pool */
   u16         num_used;      /* number of blocks used */
   u16         size;          /* block size */
   u8*         pi;            /* pointer to first block */
   u8*         pn;            /* pointer to next free block */
   u8*         px;            /* pointer to last block */
   const char* name;          /* pool name */
   SCB_PTR     sem;           /* resource semaphore */
   PCB_PTR*    php;           /* pool handle pointer */
} PCB, *PCB_PTR;

/*===========================================================================*
*                                 CONSTANTS                                  *
*===========================================================================*/

/* ANSI terminal foreground and background colors (ISO 6429) */
#define SB_CLR_BLACK          0x00
#define SB_CLR_RED            0x01
#define SB_CLR_GREEN          0x02
#define SB_CLR_YELLOW         0x03
#define SB_CLR_BLUE           0x04
#define SB_CLR_MAGENTA        0x05
#define SB_CLR_CYAN           0x06
#define SB_CLR_WHITE          0x07

/* map these onto colors above. */
#define SB_CLR_LIGHTRED       0x01
#define SB_CLR_LIGHTGREEN     0x02
#define SB_CLR_BROWN          0x03
#define SB_CLR_LIGHTBLUE      0x04
#define SB_CLR_LIGHTMAGENTA   0x05
#define SB_CLR_LIGHTCYAN      0x06
#define SB_CLR_LIGHTGRAY      0x07
#define SB_CLR_DARKGRAY       0x07

/* console */
#define SB_CON_ROWS_MAX       24    /* number of text rows, (1 to 24) */
#define SB_CON_COLS_MAX       80    /* number of text columns, (1 to 80) */
#define SB_CON_BLINK          5     /* blink attribute */

/* display */
#define SB_DISP_XMIN          40
#define SB_DISP_XMAX          (SB_CON_COLS_MAX)
#define SB_DISP_LL            (SB_DISP_XMAX - SB_DISP_XMIN - 1)

#if defined(SMX_TSMX)
#define SB_DISP_YMIN          0
#else
#define SB_DISP_YMIN          1
#endif

#define SB_DISP_YMAX      (SB_CON_ROWS_MAX-1)

/* message output types */
#define SB_MSG_ERR            0     /* Errors. Possibly catastrophic. */
#define SB_MSG_WARN           2     /* Warnings. May still run properly. */
#define SB_MSG_INFO           4     /* Informational message (status/debug). */

/*===========================================================================*
*                             GLOBAL VARIABLES                               *
*===========================================================================*/

extern bool       sb_handler_en;       /* enable fault handlers */
extern u8*        sb_isp;              /* interrupt stack pointer */
extern u32        sb_tltsel;           /* LSR type selector for sb_TMLsr() */
extern u32        sb_te[4];            /* ending times for time measurements */
extern bool       sb_tick_init_done;   /* tick initialization done */
extern const u32  sb_ticktmr_clkhz;    /* tick timer/counter clock in Hz */
extern const u32  sb_ticktmr_cntpt;    /* tick timer/counter counts per tick */
extern u32        sb_TMCal;            /* TM calibration for pmode */
extern u32        sb_ts1;              /* starting time for time measurement 1 */
extern u32        sb_ts2;              /* starting time for time measurement 2 */
extern const u32  sbu_ticktmr_cntpt;   /* tick timer/counter counts per tick for umode */

/* Notes:
   1. Use NULL for pointers; use explicit 0 for numeric values.
      This is mostly for good style/readability. In the past this was
      more needed to avoid compiler warnings. Some C compilers required
      defining this as ((void *)0), but for C++ 0 is ok.
   2. bool is a pre-defined type for C++ and its size is controlled by the
      compiler. For C we have to define it.
*/
#endif /* SB_BDEF_H */
