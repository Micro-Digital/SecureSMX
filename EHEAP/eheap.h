/*
* eheap.h                                                   Version 6.0.0
*
* Embedded Heap Functions
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
* This Work embodies patents listed here. A patent license is hereby granted
* to use these patents in this Work and Derivative Works, except in another
* RTOS or OS.
* US Patents 10,318,198, 11,010,070, and one or more patents pending.
*
* This entire comment block must be preserved in all copies of this file.
*
* Author: Ralph Moore
*
*****************************************************************************/

#ifndef EHEAP_H
#define EHEAP_H

#include <stdint.h>
#include <string.h>

/*===========================================================================*
                               CONFIGURATION
*===========================================================================*/

#define EH_ALIGN        1  /* enable aligned allocations */
#define EH_BP           1  /* 8- and 12-byte block pool enable */
#define EH_MAX_AN       12 /* maximum alignment = 4096 */
#define EH_NUM_FENCES   2  /* fence words above and below data block. <4> */
#define EH_NUM_HEAPS    6  /* number of heaps supported */
#define EH_SAFE         1  /* enable safety checks */
#define EH_SS_MERGE     1  /* enable spare space merge */
#define EH_STATS        0  /* enable statistics */

#if defined(__IAR_SYSTEMS_ICC__)
#define USE_CLZ 1
#include <intrinsics.h>
#else
#define USE_CLZ 0
#endif

/*===========================================================================*
                              BASIC DEFINITIONS
*===========================================================================*/

typedef  int8_t      s8;
typedef  int16_t     s16;
typedef  int32_t     s32;
typedef  uint8_t     u8;
typedef  uint16_t    u16;
typedef  uint32_t    u32;

#ifdef   NULL
#undef   NULL
#endif
#define  NULL        0

#define  OFF         0
#define  ON          1

/*===========================================================================*
                                 CONSTANTS
*===========================================================================*/

#define EH_FREE         0x0      /* free chunk */
#define EH_INUSE        0x1      /* inuse chunk */
#define EH_DEBUG        0x2      /* debug chunk */
#define EH_SSP          0x4      /* spare space pointer exists */
#define EH_FLAGS        0x7      /* ~ used to clear above flags */
#define EH_MIN_FRAG (32 + EH_CHK_OVH)  /* Minimum fragment size after splitting. <8> */
#define EH_R            0x100    /* MPU region flag */

/* Heap Fill Patterns */
#define EH_DATA_FILL    0xDDDDDDDD
#define EH_FENCE_FILL   0xAAAAAAA3  /* NOTE: bits 1 & 0 must = 1 */
#define EH_FREE_FILL    0xEEEEEEEE
#define EH_DTC_FILL     0xCCCCCCCC

/* Peek Parameters */
typedef enum {
   EH_PK_AUTO,
   EH_PK_BINNO,
   EH_PK_BP,
   EH_PK_BS_FWD,
   EH_PK_CP,
   EH_PK_COUNT,
   EH_PK_DEBUG,
   EH_PK_END,
   EH_PK_FILL,
   EH_PK_FIRST,
   EH_PK_HS_FWD,
   EH_PK_INIT,
   EH_PK_LAST,
   EH_PK_MERGE,
   EH_PK_NEXT,
   EH_PK_NEXT_FREE,
   EH_PK_ONR,
   EH_PK_PREV,
   EH_PK_PREV_FREE,
   EH_PK_SIZE,
   EH_PK_SPACE,
   EH_PK_TIME,
   EH_PK_TYPE,
   EH_PK_USE_DC
} EH_PK_PAR;

/* Set Parameters */
typedef enum {
   EH_ST_AUTO,
   EH_ST_DEBUG,
   EH_ST_FILL,
   EH_ST_MERGE,
   EH_ST_END
} EH_ST_PAR;

/*===========================================================================*
                               ERROR NUMBERS
*===========================================================================*/

typedef enum {
               EH_OK,
               EH_ALREADY_INIT,
               EH_HEAP_BRKN,
               EH_HEAP_FIXED,
               EH_HEAP_ERROR,
               EH_HEAP_FENCE_BRKN,
               EH_INSUFF_HEAP,
               EH_INV_CCB,
               EH_INV_PAR,
               EH_RECOVER,
               EH_TOO_MANY_HEAPS,
               EH_WRONG_HEAP,
} EH_ERRNO;
/*__short_enum_attr*/

/*===========================================================================*
                                 TYPEDEFS
*===========================================================================*/

typedef struct CCB*  CCB_PTR;

typedef struct BPCB {  /* BLOCK POOL CONTROL BLOCK */
   u32      num_blks;      /* number of blocks in pool */
   u32      inuse;         /* blocks in use of this size */
   u32      maxuse;        /* maximum blocks in use of this size */
   void*    pi;            /* pointer to pool start block */
   void*    pn;            /* pointer to next free block = NULL if none */
   void*    px;            /* pointer to pool end block */
} BPCB, *BPCB_PTR;

typedef struct CCB {    /* CHUNK FREE CONTROL BLOCK */
   CCB_PTR  fl;            /* forward link */
   CCB_PTR  blf;           /* backward link | flags */
   u32      sz;            /* chunk size */
   CCB_PTR  ffl;           /* free forward link */
   CCB_PTR  fbl;           /* free backward link */
   u32      binx8;         /* bin number << 3. last 3 bits = 0 */
} CCB, *CCB_PTR;

typedef struct CDCB {   /* CHUNK DEBUG CONTROL BLOCK */
   CCB_PTR  fl;            /* forward link */
   CCB_PTR  blf;           /* backward link | flags */
   u32      sz;            /* chunk size */
   u32      time;          /* time of allocation (etime) */
   u32      onr;           /* task or LSR that allocated chunk */
   u32      fence;         /* = EH_FENCE_FILL */
} CDCB, *CDCB_PTR;

typedef struct CICB {   /* CHUNK INUSE CONTROL BLOCK */
   CCB_PTR  fl;            /* forward link */
   CCB_PTR  blf;           /* backward link | flags */
} CICB, *CICB_PTR;

typedef struct HBCB {   /* HEAP BIN CONTROL BLOCK */
   CCB_PTR  ffl;           /* free forward link */
   CCB_PTR  fbl;           /* free backward link */
} HBCB, *HBCB_PTR;

/* user-alterable heap mode flags */
#define EH_CM     0x0002   /* cmerge */
#define EH_DBM    0x0008   /* debug mode */
#define EH_FILL   0x0010   /* fill */
#define EH_AM     0x0020   /* amerge */
#define EH_HFR    0x0400   /* heap failure report */  
#define EH_AR     0x0800   /* auto recover */
#define EH_ED     0x1000   /* error detection excl alloc and free */
#define EH_EDA    0x2000   /* error detection all */
#define EH_EM     0x4000   /* error manager */
#define EH_PRE    0x8000   /* preemption protection */

/* combined heap mode flags */
#define EH_NORM   (EH_AM | EH_EDA | EH_EM | EH_PRE) /* normal operation */
#define EH_DBOP   (EH_NORM | EH_FILL | EH_HFR)      /* debug operation */

typedef union HMODE { /* HEAP MODE FLAGS */
   struct {  
      u32      init     : 1;  /* heap has been initialized */
      u32      cmerge   : 1;  /* free chunk merge mode */
      u32      use_dc   : 1;  /* use donor chunk */
      u32      debug    : 1;  /* heap is in debug mode */
      u32      fill     : 1;  /* enable heap fill */
      u32      amerge   : 1;  /* enables automatic control of chunk merge */
      u32      fbbs     : 1;  /* first bin back scan */
      u32      hs_fwd   : 1;  /* heap scan forward */
      u32      bs_fwd   : 1;  /* bin scan forward */
      u32      realloc  : 1;  /* in eh_Realloc() */
      u32      hf_rpt   : 1;  /* enable heap failure report */
      u32      auto_rec : 1;  /* enable auto-recovery */
      u32      ed_en    : 2;  /* error detection enable:
                                    0  none
                                    1  all but allocation and free
                                    2  all
                                    3  all */
      u32      em_en    : 1;  /* enable error manager */
      u32      pre      : 1;  /* preemption protection present */
      } fl;
   u32 wd;                    /* to load or clear */
} HMODE;

typedef struct EHV {    /* EHEAP VARIABLES */
   const char* name;       /* heap name (optional) */
#if defined(SMX)
   MUCB*     mtx;          /* access control mutex handle */
#else
   void*     acop;         /* access control object pointer */
#endif
   CCB_PTR   bfp;          /* bin fix pointer */
   HBCB_PTR  binp;         /* bin pointer */
   u32       bmap;         /* bin map */
   u32       bsmap;        /* bin sort map */
   bool      bsort;        /* bin sort flag */
   CCB_PTR   bsp;          /* bin scan pointer */
   u32*      bszap;        /* bin size array pointer <1> */
   void*     cbp;          /* current block pointer for eh_Realloc() */
   CCB_PTR   ccp;          /* current chunk pointer */
   u32       csbin;        /* current sort bin (none if = -1) */
   CCB_PTR   dcp;          /* donor chunk pointer */
   EH_ERRNO  errno;        /* error number */
   CCB_PTR   hfp;          /* heap fix pointer */  
   u32       hhwm;         /* heap high water mark */
   u32       hsz;          /* heap size including extensions, if any */
   CCB_PTR   hsp;          /* heap scan pointer */
   u32       hused;        /* heap used in bytes */
   CCB_PTR   ltp;          /* last turtle pointer */
   void    (*mgr)(u32);    /* heap manager function */
   HMODE     mode;         /* heap mode flags */
   CCB_PTR   ncp;          /* next chunk pointer <2> */
   CCB_PTR   pcp;          /* previous chunk pointer <2> */
   CICB_PTR  pi;           /* pointer to heap start chunk, sc */
   CICB_PTR  px;           /* pointer to heap end chunk, ec */
   u32       retries;      /* recovery retries */
   u32       sba_top;      /* small bin array top bin number */
   u32       sba_top_sz;   /* small bin array top bin size */
   u32       tbsz;         /* top bin size (sum of chunks) */
   CCB_PTR   tcp;          /* top chunk pointer */
   u32       top_bin;      /* bin array top bin number */
#if EH_BP || defined(SMXAWARE)
   void*     fhcp;         /* first heap chunk pointer */
   BPCB_PTR  bpcbp;        /* block pool control block array pointer */
#endif
#if EH_STATS || defined(SMXAWARE)
   u32*      bnump;        /* bin number pointer */
   u32*      bsump;        /* bin sum pointer */
#endif
} EHV, *EHV_PTR;

/*===========================================================================*
                               VARIABLES
*===========================================================================*/

extern EHV_PTR eh_hvp[EH_NUM_HEAPS]; /* pointers to eh variable structures */
extern u32     eh_hvpn;              /* next available eh_hvp slot */

/*===========================================================================*
                                 MACROS
*===========================================================================*/

#define EH_BP_OFFS  (eh_hvp[hn]->mode.fl.debug*(sizeof(CDCB) + 4*EH_NUM_FENCES - 8) + 8)
#define EH_CHK_OVH  (eh_hvp[hn]->mode.fl.debug*(sizeof(CDCB) + 8*EH_NUM_FENCES - 8) + 8) /*<3>*/
#define EH_IS_FREE(cp)  !((u32)(cp->blf)&EH_INUSE)

/*============================================================================
                          API FUNCTION PROTOTYPES
============================================================================*/

#ifdef __cplusplus

/*------------------- for C++ with default parameters ----------------------*/

u32      eh_BinPeek(u32 binno, EH_PK_PAR par, u32 hn=0);
bool     eh_BinScan(u32 binno, u32 fnum, u32 bnum, u32 hn=0);
bool     eh_BinSeed(u32 num, u32 bsz, u32 hn=0);
bool     eh_BinSort(u32 binno, u32 fnum, u32 hn=0);
void*    eh_Calloc(u32 num, u32 sz, u32 an=0, u32 hn=0);
u32      eh_ChunkPeek(void* vp, EH_PK_PAR par, u32 hn=0);
bool     eh_Extend(u32 xsz, u8* xp, u32 hn=0);
bool     eh_Free(void* bp, u32 hn=0);
u32      eh_Init(u32 sz, u32 dcsz, u8* hp, EHV_PTR vp, u32* bszap, HBCB* binp, 
                                             u32 mode, const char* name=NULL);
void*    eh_Malloc(u32 sz, u32 an=0, u32 hn=0);
u32      eh_Peek(EH_PK_PAR par, u32 hn=0);
void*    eh_Realloc(void* cbp, u32 sz, u32 an=0, u32 hn=0);
bool     eh_Recover(u32 sz, u32 num, u32 an=0, u32 hn=0);
bool     eh_Scan(CCB_PTR cp, u32 fnum, u32 bnum, u32 hn=0);
bool     eh_Set(EH_ST_PAR par, u32 val, u32 hn=0);

#else  /*--------------- for C without default parameters -------------------*/

u32      eh_BinPeek(u32 binno, EH_PK_PAR par, u32 hn);
bool     eh_BinScan(u32 binno, u32 fnum, u32 bnum, u32 hn);
bool     eh_BinSeed(u32 num, u32 bsz, u32 hn);
bool     eh_BinSort(u32 binno, u32 fnum, u32 hn);
void*    eh_Calloc(u32 num, u32 sz, u32 an, u32 hn);
u32      eh_ChunkPeek(void* vp, EH_PK_PAR par, u32 hn);
bool     eh_Extend(u32 xsz, u8* xp, u32 hn);
bool     eh_Free(void* bp, u32 hn);
u32      eh_Init(u32 sz, u32 dcsz, u8* hp, EHV_PTR vp, u32* bszap, HBCB* binp, 
                                                  u32 mode, const char* name);
void*    eh_Malloc(u32 sz, u32 an, u32 hn);
u32      eh_Peek(EH_PK_PAR par, u32 hn);
void*    eh_Realloc(void* cbp, u32 sz, u32 an, u32 hn);
bool     eh_Recover(u32 sz, u32 num, u32 an, u32 hn);
bool     eh_Scan(CCB_PTR cp, u32 fnum, u32 bnum, u32 hn);
bool     eh_Set(EH_ST_PAR par, u32 val, u32 hn);
#endif /* __cplusplus */

/*===========================================================================*
                                   MISC
*===========================================================================*/

#if !defined(SMX)
bool     eh_HeapsInit(void);  /* app callback to init heaps */
#endif

extern u32 eh_onr(void);
extern u32 eh_time(void);

/* Notes:
   1. eh_hvp[hn]->bszap points at an array, such as binsz[], which stores
      minimum bin sizes for heap hn. It is defined in user code (e.g. theap.c).
      It must be in RAM for tsmx, but can be in either ROM or RAM for
      applications. ROM is safer, but RAM is likely to be faster.
   2. If NULL, indicates to echunk_unfree() that this chunk was not merged.
   3. Includes fences after the block in a debug chunk.
   4. EH_NUM_FENCES must be a multiples of 2 to preserve 8-byte alignment of 
      all chunks and blocks.
*/
#endif /* EHEAP_H */