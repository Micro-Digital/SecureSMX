/*
* bcon.c                                                    Version 6.0.0
*
* Console display routines. Implemented to send characters to a terminal
* via serial port.
*
* Copyright (c) 1998-2026 Micro Digital Inc.
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
* Authors: David Moore, A. C. Verbeck, Ralph Moore
*
* Notes:
*
*   1. 0,0 is the top left corner for this API. See note in bcon.h.
*
*   2. Use Unp (unprotected) versions of functions from ISRs and LSRs
*      since the normal versions call an SSR, which is not allowed from
*      ISRs, and they also get a mutex or test a semaphore with a timeout,
*      both of which are not allowed from LSRs.
*
*   3. Use functions such as itoa to convert numbers to strings.
*      Then use sb_ConWriteString() or sb_ConPutString() to output strings
*
*****************************************************************************/

#include "bbase.h"
#include "bsp.h"

#if (SB_CFG_CON)

#if SMX_CFG_SSMX
#pragma section_prefix = ".cp"
#include "xapiu.h"
#endif

#define USE_COLOR 1  /* enable color */

bool         con_out_dbg_mode;   /* output text messages w/o formatting */

/* variables */
static char  ANSI_buf[12];
static u8    con_out_byte;       /* single byte out */
static uint  con_out_is_init;    /* prevents display until console initialized */
static MUCB* con_out_mtx;
static u8    spaces[SB_CON_COLS_MAX];

/* string literals */
const char CLR_END_OF_LINE[]  = "\x1b[K";
const char CLR_SCREEN[]       = "\x1b[2J";
const char CURSOR_OFF[]       = "\x1b[?25l";
const char CURSOR_ON[]        = "\x1b[?25h";
const char GMODE_RESET[]      = "\x1b[0m";

/* internal subroutines */
static void ANSI_Cmd(const char *cmd);
static void ANSI_GModeSet(u32 gm);
static void ANSI_GotoXY(u32 x, u32 y);

/*============================================================================
                            CONSOLE FUNCTIONS
============================================================================*/

/*
*  sb_ConInit() does any necessary initialization and then sets con_out_is_init.
*  If the display routines in this file are output via serial port, we
*  assume the UART has already been initialized before this is called.
*/
void sb_ConInit(void)
{
 #if (SB_CFG_CON)
   con_out_mtx = smx_MutexCreate(1, 0, "con_out_mtx", &con_out_mtx);
   if (!con_out_mtx)
      return;
   for (u32 i = 0; i < SB_CON_COLS_MAX; i++)
   {
      spaces[i] = ' ';
   }
   ANSI_Cmd(CURSOR_OFF);
   ANSI_Cmd(GMODE_RESET);
   ANSI_Cmd(CLR_SCREEN);
   con_out_is_init = true;
 #endif
}

/*
*  sb_ConOutPort() returns console output port.
*/
u32 sb_ConOutPort(void)
{
   return SB_CON_OUT_PORT;
}

/*
*  sb_ConClearEndOfLine() clears to the end of the line within the left
*  panel or right panel of the display, unless it exceeds the panel the
*  message started in. col, row, and len are starting column, row, and
*  length of the message just displayed.
*/
void sb_ConClearEndOfLine(u32 col, u32 row, u32 len)
{
   if (!con_out_is_init) return;

   smx_MutexGet(con_out_mtx, SMX_TMO_INF);
   sb_ConClearEndOfLineUnp(col, row, len);
   smx_MutexRel(con_out_mtx);
}

/*
*  sb_ConClearEndOfLineUnp() is unprotected version. See note 2.
*/
void sb_ConClearEndOfLineUnp(u32 col, u32 row, u32 len)
{
   u32 start, end;

   if (!con_out_is_init) return;

  #if (SB_DISP_YMIN != 0)
   if (col < SB_DISP_XMIN && row >= SB_DISP_YMIN)  /* in left panel */
  #else
   if (col < SB_DISP_XMIN)     /* in left panel */
  #endif
      end = SB_DISP_XMIN - 1;  /* -1 is to avoid clearing '*' line marker */
   else                                            /* in right panel */
      end = SB_CON_COLS_MAX;
   start = col+len;
   if (start < end)
   {
      ANSI_GotoXY(row, start);
      sb_UartOutData((u8*)&spaces, end-start);
   }
}

/*
*  sb_ConClearLine() can be used to clear a line (row) of the screen.
*/
void sb_ConClearLine(u32 row)
{
   if (!con_out_is_init) return;
   smx_MutexGet(con_out_mtx, SMX_TMO_INF);

   /* goto beginning of line and clear to end */
   ANSI_GotoXY(row,0);
   ANSI_GModeSet(SB_CLR_BLACK+40);
   ANSI_Cmd(CLR_END_OF_LINE);
   smx_MutexRel(con_out_mtx);
}

/*
*  sb_ConClearScreen() clears the screen.
*/
void sb_ConClearScreen(void)
{
   if (!con_out_is_init) return;

   smx_MutexGet(con_out_mtx, SMX_TMO_INF);
   sb_ConClearScreenUnp();
   smx_MutexRel(con_out_mtx);
}

/*
*  sb_ConClearScreenUnp() is unprotected version. See note 2.
*/
void sb_ConClearScreenUnp(void)
{
   if (!con_out_is_init) return;

   ANSI_GotoXY(0,0);
   #if (USE_COLOR == 0)
   ANSI_GModeSet(SB_CLR_WHITE+30);
   #else
   ANSI_GModeSet(SB_CLR_LIGHTGRAY+30);
   #endif
   ANSI_GModeSet(SB_CLR_BLACK+40);
   ANSI_Cmd(CLR_SCREEN);
}

/*
*  sb_ConCursorOn/Off
*/
void sb_ConCursorOff(void)
{
   ANSI_Cmd(CURSOR_OFF);
}

void sb_ConCursorOn(void)
{
   ANSI_Cmd(CURSOR_ON);
}

/*
*  sb_ConScroll() moves every char + attribute up one line. The first line
*  is discarded. The last line is cleared.
*/
void sb_ConScroll(void)
{
   /* implement */
}

/*
*  sb_ConPutChar is like putchar()
*/
int sb_ConPutChar(char ch)
{
   static u32 col = 0;
   static u32 row = 0;

   if (!con_out_is_init) return(false);
   smx_MutexGet(con_out_mtx, SMX_TMO_INF);
   sb_ConWriteCharUnp(col++, row, SB_CLR_LIGHTGRAY, SB_CLR_BLACK, !SB_CON_BLINK, ch);
   if (ch == '\n' || col >= SB_CON_COLS_MAX)
   {
      row = (row == SB_CON_ROWS_MAX-1) ? row : row+1;
      col = 0;
   }
   smx_MutexRel(con_out_mtx);
   return(ch);
}

/*
*  sb_ConPutString is like puts()
*/
int sb_ConPutString(const char* in_string)
{
   static u32 row = 0;

   if (!con_out_is_init) return(false);

   smx_MutexGet(con_out_mtx, SMX_TMO_INF);
   sb_ConWriteStringUnp(0, row, SB_CLR_LIGHTGRAY, SB_CLR_BLACK, !SB_CON_BLINK, in_string);
   sb_ConWriteCharUnp(0, row, SB_CLR_LIGHTGRAY, SB_CLR_BLACK, !SB_CON_BLINK, '\n');
   row = (row == SB_CON_ROWS_MAX-1) ? row : row+1;
   smx_MutexRel(con_out_mtx);

   return(true);
}

/*
*  sb_ConWriteChar() writes a character to the operation screen at the loca-
*  tion specified by col and row.  F_color is the color of the character
*  itself and B_color is the color of the space around the character.
*  blink should be SB_CON_BLINK if the text is to blink, and !SB_CON_BLINK if not.
*/
void sb_ConWriteChar(u32 col, u32 row, u32 F_color, u32 B_color, u32 blink, char ch)
{
   if (!con_out_is_init) return;

   smx_MutexGet(con_out_mtx, SMX_TMO_INF);
   sb_ConWriteCharUnp(col, row, F_color, B_color, blink, ch);
   smx_MutexRel(con_out_mtx);
}

/*
*  sb_ConWriteCharUnp() is unprotected version. See note 2.
*/
void sb_ConWriteCharUnp(u32 col, u32 row, u32 F_color, u32 B_color, u32 blink, char ch)
{
   if (!con_out_is_init) return;

  #if (USE_COLOR == 0)
   F_color = SB_CLR_WHITE;
   B_color = SB_CLR_BLACK;
  #endif
   if (!con_out_dbg_mode)
   {
      ANSI_GotoXY(row,col);
      ANSI_GModeSet(F_color+30);
      ANSI_GModeSet(B_color+40);
      if (blink)
         ANSI_GModeSet(SB_CON_BLINK);
   }
   con_out_byte = ch;
   sb_UartOutData(&con_out_byte, 1);
   if (!con_out_dbg_mode)
   {
      if (blink)
         ANSI_Cmd(GMODE_RESET);
   }
}

/*
*  sb_ConWriteString() writes a string to the operation screen at the location
*  specified by col and row.  F_color is the color of the string, and B_color
*  is the color of the background. blink should be SB_CON_BLINK if the string
*  is to blink, and !SB_CON_BLINK if not. The last parameter is a pointer to
*  the null-terminated string. sb_ConWriteString goes to the next line if the
*  current line is filled.
*/
void sb_ConWriteString(u32 col, u32 row, u32 F_color, u32 B_color,
                                         u32 blink, const char* in_string)
{
   smx_MutexGet(con_out_mtx, SMX_TMO_INF);
   sb_ConWriteStringUnp(col, row, F_color, B_color, blink, in_string);
   smx_MutexRel(con_out_mtx);
}

/*
*  sb_ConWriteStringUnp() is unprotected version. See note 2.
*/
void sb_ConWriteStringUnp(u32 col, u32 row, u32 F_color, u32 B_color,
                                         u32 blink, const char* in_string)
{
   u32  len;

   if (!con_out_is_init) return;

  #if (USE_COLOR == 0)
   F_color = SB_CLR_WHITE;
   B_color = SB_CLR_BLACK;
  #endif
   if (!con_out_dbg_mode)
   {
      ANSI_GotoXY(row,col);
      ANSI_GModeSet(F_color+30);
      ANSI_GModeSet(B_color+40);
      if (blink)
         ANSI_GModeSet(SB_CON_BLINK);
   }

   len = strlen(in_string);
   sb_UartOutData((u8*)in_string, len);

   if (!con_out_dbg_mode)
   {
      if (blink)
         ANSI_Cmd(GMODE_RESET);

      /* Clear to end of line. */
      sb_ConClearEndOfLineUnp(col, row, strlen(in_string));
   }
   else
   {
      len = 2;
      sb_UartOutData((u8*)"\r\n", len);
   }
}

/*
*  sb_ConWriteStringNum() is like sb_ConWriteString() but writes the indicated
*  number of characters. Does not stop at a NUL character.
*/
void sb_ConWriteStringNum(u32 col, u32 row, u32 F_color, u32 B_color,
                              u32 blink, const char* in_string, u32 num)
{
   if (!con_out_is_init) return;

  #if (USE_COLOR == 0)
   F_color = SB_CLR_WHITE;
   B_color = SB_CLR_BLACK;
  #endif

   smx_MutexGet(con_out_mtx, SMX_TMO_INF);
   if (!con_out_dbg_mode)
   {
      ANSI_GotoXY(row,col);
      ANSI_GModeSet(F_color+30);
      ANSI_GModeSet(B_color+40);
      if (blink)
         ANSI_GModeSet(SB_CON_BLINK);
   }

   sb_UartOutData((u8*)in_string, num);

   if (!con_out_dbg_mode)
   {
      if (blink)
         ANSI_Cmd(GMODE_RESET);

      /* Clear to end of line. */
      sb_ConClearEndOfLineUnp(col, row, num);
   }
   smx_MutexRel(con_out_mtx);
}

/*
*  sb_ConWriteCounter() writes a 6-digit counter to the screen. Used by
*  various demo tasks. (When the counter exceeds 6 digits, counter display
*  moves to the right.)
*/

void sb_ConWriteCounter(u32 col, u32 row, u32 F_color, u32 B_color, u32 ctr, u32 radix)
{
   char buffer[11];
   u32  i, j;

   ultoa(ctr, buffer, radix);

   for (i = 0; buffer[i] && i < 6; i++) {}  /* find end of string */
   for (; i < 6; i++)                       /* right align */
   {
      for (j = i+1; j > 0; j--)
         buffer[j] = buffer[j-1];
      buffer[j] = '0';
   }

   sb_ConWriteString(col,row,F_color,B_color,!SB_CON_BLINK,buffer);
}

/*
*  sb_ConDbgMsgMode() returns true if the console is in debug message mode.
*/
bool sb_ConDbgMsgMode(void)
{
   return con_out_dbg_mode;
}

/*
*  sb_ConDbgMsgModeSet() enables the debug message mode of console output.
*  This omits ANSI codes for positioning, to enable saving a clean log file
*  in the terminal emulator.
*/
void sb_ConDbgMsgModeSet(bool enable)
{
   con_out_dbg_mode = enable;
}

/*
* ANSI Terminal Functions
*
* Description:
*    These send ANSI codes to the terminal to control cursor position,
*    color, etc.
*
* Parameters:
*    (None)
*
* Returns:
*    (None)
*
* Notes:
*    We do not check con_out_is_init because it is checked in all functions
*    (above) that call these.
*/

static void ANSI_Cmd(const char* cmd)
{
   u32 len = strlen(cmd);
   sb_UartOutData((u8*)cmd, len);
}

static void ANSI_GotoXY(u32 x, u32 y)
{
   u32 i = 0;
   x++; y++;   /* convert to terminal coordinates */

   /* output ESC sequence: 0x1b[x;yH0 */
   ANSI_buf[i++] = 0x1b;
   ANSI_buf[i++] = 0x5b;
   itoa(x, ANSI_buf+i, 10);
   i = strlen(ANSI_buf);
   ANSI_buf[i++] = ';';
   itoa(y, ANSI_buf+i, 10);
   i = strlen(ANSI_buf);
   ANSI_buf[i++] = 'H';
   ANSI_buf[i] = 0;
   ANSI_Cmd(ANSI_buf);
}

static void ANSI_GModeSet(u32 gmode)
{
   u32 i = 0;
   u32 g = gmode;

   /* output ESC sequence: 0x1b[gm0 */
   ANSI_buf[i++] = 0x1b;
   ANSI_buf[i++] = 0x5b;
   itoa(g, ANSI_buf+i, 10);
   i = strlen(ANSI_buf);
   ANSI_buf[i++] = 'm';
   ANSI_buf[i] = 0;
   ANSI_Cmd(ANSI_buf);
}
#endif /* SB_CFG_CON */

