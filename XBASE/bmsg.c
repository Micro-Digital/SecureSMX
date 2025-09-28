/*
* bmsg.c                                                    Version 5.4.0
*
* smxBase Message Display Functions.
*
* Copyright (c) 2012-2025 Micro Digital Inc.
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
* Authors: Ralph Moore, David Moore
*
*****************************************************************************/

#include "bbase.h"

#if SMX_CFG_SSMX
#pragma section_prefix = ".sys"
#endif

#define EOL    (SB_DISP_XMIN + SB_DISP_LL - 1)
#define MAXSZ  SB_CON_COLS_MAX + 1  /* max OMB msg length */
#define MLOST  1                    /* message lost */
#define NUL    0

/*============================================================================
                                VARIABLES
============================================================================*/

extern bool  con_out_dbg_mode; /* output text messages w/o formatting */

u32  disp_inuse;
u8   sb_omb[SB_SIZE_OMB];      /* output msg buffer */
bool sb_ombfull;
char sb_msgbuf[MAXSZ];         /* msg display buffer */

/*============================================================================
                           INITIALIZED VARIABLES
============================================================================*/

u8*  sb_ombep = (sb_omb + SB_SIZE_OMB - 1); /* omb end ptr */
u8*  sb_ombip = sb_omb; /* omb in ptr */
u8*  sb_ombop = sb_omb; /* omb out ptr */
u8*  sb_omblp = sb_omb; /* omb last msg ptr */
u32  x  = SB_DISP_XMIN; /* display x coordinate */
u32  y  = SB_DISP_YMIN; /* display y coordinate */
u32  yn = SB_DISP_YMIN; /* first y coordinate of new msg */
u32  yp = SB_DISP_YMIN; /* first y coordinate of previous msg */

/* sb_MsgOut()
   Copy strings to sb_omb for later display by sb_MsgDisplay(). Each msg in
   sb_omb has a 8-bit header followed by a NUL-terminated string. 
*/
void sb_MsgOut(u8 mtype, const char* mp)
{
   u32  isav;           /* interrupt status save */
   u32  len;            /* msg length including NUL */
   u32  slen;           /* split length to end of omb */
   bool trun = false;   /* msg truncated */

   isav = sb_IntStateSaveDisable();

   if (!sb_ombfull)
   {
      len = strlen(mp) + 1;

      /* truncate over-size string -- add NUL later */
      if (len > MAXSZ)
      {
         len = MAXSZ;
         trun = true;
      }

      /* if message will not fit, set ombfull and MLOST and abort */
      if ((sb_ombop != sb_ombip) && 
          (len) > ((SB_SIZE_OMB + sb_ombop - sb_ombip)%SB_SIZE_OMB))
      {
         sb_ombfull = true;
         *sb_omblp |= MLOST;
      }
      else
      {
         sb_omblp = sb_ombip;

         /* load msg type and clear MLOST flag <1> */
         *sb_ombip++ = mtype;

         /* if wrap-around, copy partial string to end of omb */
         if ((sb_ombip + len - 1) > sb_ombep)
         {
            slen = (sb_ombep - sb_ombip + 1);
            memcpy(sb_ombip, mp, slen);
            mp = mp + slen;
            len = len - slen;
            sb_ombip = sb_omb;
         }

         /* copy partial or full string to omb and advance sb_ombip */
         memcpy(sb_ombip, mp, len);
         sb_ombip = sb_ombip + len;
 
         /* NUL terminate truncated msg */
         if (trun)
            *(sb_ombip - 1) = NUL;

         /* wrap sb_ombip around */
         if (sb_ombip > sb_ombep)
            sb_ombip = sb_omb;

         /* set ombfull if pointers are equal */
         if (sb_ombip == sb_ombop)
         {
            sb_ombfull = true;
         }
      }
   }
   else
   {
      *sb_omblp |= MLOST;
   }
   sb_IntStateRestore(isav);
}

/* sb_MsgDisplay()
   Display all messages in sb_omb from sb_ombop to sb_ombip <2> 
*/
void sb_MsgDisplay(void)
{
   u32   isav;    /* interrupt state save */
   u32   len;     /* num bytes to NUL or to end of omb */
   u32   len2;    /* num bytes to NUL in wrapped part of msg */
   u8    mhdr;    /* msg header */
   u8*   mp;      /* message pointer */
   u8*   msp;     /* message scan pointer */
   bool  msglost; /* msg lost */

   isav = sb_IntStateSaveDisable();
   if (disp_inuse)
   {
      sb_IntStateRestore(isav);
      return;
   }
   disp_inuse = true;

   while ((sb_ombop != sb_ombip) || sb_ombfull)
   {
      /* LOAD NEXT MSG INTO sb_msgbuf */

      /* get msg header and move to start of msg */
      mhdr = *sb_ombop++;
      if (sb_ombop > sb_ombep)
         sb_ombop = sb_omb;

      /* determine len to first NUL or ombend + 1  */
      mp = sb_ombop;
      for (msp = mp; *msp != NUL && msp <= sb_ombep; msp++){}
      len = msp - mp;
      len2 = 0;

      /* copy full msg or unwrapped part of msg into strbuf */
      strncpy(sb_msgbuf, (const char*)mp, len);

      /* if msg wrapped, append wrapped part of msg to msg in strbuf  */
      if (msp > sb_ombep)
      {
         mp = sb_omb;
         for (msp = mp; *msp != NUL; msp++){}
         len2 = msp - mp;
         strncpy(sb_msgbuf + len, (const char*)mp, len2);
         sb_ombop = (mp + len2 + 1);
      }
      else
      {
         sb_ombop = (mp + len + 1);
      }
      if (sb_ombop > sb_ombep)
         sb_ombop = sb_omb;      /* stop output if sb_ombip = sb_omb */

      sb_ombfull = false;
      sb_IntStateRestore(isav);

     #if defined(SMXAWARE)
      sa_Print((const char*)mp);
     #endif

     #if (SB_CFG_CON)

      /* DISPLAY MSG IN sb_msgbuf */

      u32  color;             /* text display color */
      u32  dlen;              /* console display length */

      msglost = (mhdr & MLOST ? true : false);
      switch (mhdr & 0x6)
      {
         case SB_MSG_ERR:
            color = SB_CLR_LIGHTRED;
            break;
         case SB_MSG_WARN:
            color = SB_CLR_YELLOW;
            break;
         default:
            color = SB_CLR_LIGHTGREEN;
      }
      mp = (u8*)&sb_msgbuf;
      len += len2;
      yn = y;

      do
      {
         dlen = len > SB_DISP_LL ? SB_DISP_LL : len;
         sb_ConWriteStringNum(x, y, color, SB_CLR_BLACK, !SB_CON_BLINK, (const char*)mp, dlen);
         mp += dlen;
         len -= dlen;
         if(++y >= SB_DISP_YMAX)
            y = SB_DISP_YMIN;
      } while (len > 0);

      if (!con_out_dbg_mode)
      {
         /* display new line marker and clear old one */
         sb_ConWriteChar(x-1, yn, SB_CLR_WHITE, SB_CLR_BLACK, !SB_CON_BLINK, '*');
         if (yp != yn)
            sb_ConWriteChar(x-1, yp, SB_CLR_WHITE, SB_CLR_BLACK, !SB_CON_BLINK, ' ');
         yp = yn;

         /* lost message notification */
         if (msglost)
            sb_ConWriteChar(EOL, yn, SB_CLR_LIGHTRED, SB_CLR_BLACK, !SB_CON_BLINK, 'B');
      }
      else
      {
         /* lost message notification */
         sb_ConWriteStringNum(x, y, color, SB_CLR_BLACK, !SB_CON_BLINK, "\r\n", 2);
         if (msglost)
            sb_ConWriteString(x, y, SB_CLR_LIGHTRED, SB_CLR_BLACK, !SB_CON_BLINK, 
                                          "*** OMB Overflow ***");
      }
     #endif /* SB_CFG_CON */
      isav = sb_IntStateSaveDisable();
   }
   sb_IntStateRestore(isav);  /* restore interrupt enable/disable */
   disp_inuse = false;        /* permit reentry to this function */
}

/* Notes:
   1. Worst case: sb_ombip == sb_ombep, *sb_ombep = hdr, and in following
      partial load, slen = 0 so nothing happens.
   2. Must call from pmode since the sb_Con functions call smx_MutexGet() via 
      the SVC exception, which would cause an exception within an exception
      from umode, causing a Hard Fault. sb_MsgDisplay() is normally called
      from the idle task.
*/