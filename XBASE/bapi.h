/*
* bapi.h                                                    Version 5.4.0
*
* smxBase API.
*
* Copyright (c) 2004-2025 Micro Digital Inc.
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
* Authors: David Moore, Ralph Moore
*
*****************************************************************************/

#ifndef SB_BAPI_H
#define SB_BAPI_H

/*===========================================================================*
*                                smxBase API                                 *
*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

#if defined(SMXAWARE)
bool     smxaware_init(void);
void     sa_PrintInit(void);
void     sa_Print(const char* string);
void     sa_PrintVals(const char* s, long val1, long val2);
#else
#define  sa_PrintInit()
#define  sa_Print(x)
#define  sa_PrintVals(x, y, z)
#endif

bool     sb_BlockPoolCreate(u8* p, PCB_PTR pool, u16 num, u16 size,
                                                              const char* name);
bool     sb_BlockPoolCreateDAR(SB_DCB* dar, PCB_PTR pool, u16 num, u16 size,
                                                   u16 align, const char* name);
u8*      sb_BlockPoolDelete(PCB_PTR pool);
u32      sb_BlockPoolPeek(PCB_PTR pool, SB_PK_PAR par);
u8*      sb_BlockGet(PCB_PTR pool, u16 clrsz);
bool     sb_BlockRel(PCB_PTR pool, u8* bp, u16 clrsz);

#if (SB_CFG_CON)
void     sb_ConClearEndOfLine(u32 col, u32 row, u32 len);
void     sb_ConClearEndOfLineUnp(u32 col, u32 row, u32 len);
void     sb_ConClearLine(u32 row);
void     sb_ConClearScreen(void);
void     sb_ConClearScreenUnp(void);
void     sb_ConCursorOff(void);
void     sb_ConCursorOn(void);
bool     sb_ConDbgMsgMode(void);
void     sb_ConDbgMsgModeSet(bool enable);
void     sb_ConInit(void);
u32      sb_ConOutPort(void);
int      sb_ConPutChar(char ch);
int      sb_ConPutString(const char* in_string);
void     sb_ConScroll(void);
void     sb_ConWriteChar(u32 col, u32 row, u32 F_color, u32 B_color, u32 blink, char ch);
void     sb_ConWriteCharUnp(u32 col, u32 row, u32 F_color, u32 B_color, u32 blink, char ch);
void     sb_ConWriteCounter(u32 col, u32 row, u32 F_color, u32 B_color, u32 ctr, u32 radix);
void     sb_ConWriteString(u32 col, u32 row, u32 F_color, u32 B_color, u32 blink, const char* in_string);
void     sb_ConWriteStringUnp(u32 col, u32 row, u32 F_color, u32 B_color, u32 blink, const char* in_string);
void     sb_ConWriteStringNum(u32 col, u32 row, u32 F_color, u32 B_color, u32 blink, const char* in_string, u32 num);
#else
#define  sb_ConClearEndOfLine(col, row, len) {}
#define  sb_ConClearEndOfLineUnp(col, row, len) {}
#define  sb_ConClearLine(row) {}
#define  sb_ConClearScreen() {}
#define  sb_ConClearScreenUnp() {}
#define  sb_ConCursorOff() {}
#define  sb_ConCursorOn() {}
#define  sb_ConDbgMsgMode() false
#define  sb_ConDbgMsgModeSet(enable) {}
#define  sb_ConInit() {}
#define  sb_ConOutPort() {}
#define  sb_ConPutChar(ch) {}
#define  sb_ConPutString(in_string) {}
#define  sb_ConScroll() {}
#define  sb_ConWriteChar(col, row, F_color, B_color, blink, ch) {}
#define  sb_ConWriteCharUnp(col, row, F_color, B_color, blink, ch) {}
#define  sb_ConWriteCounter(col, row, F_color, B_color, ctr, radix) {}
#define  sb_ConWriteString(col, row, F_color, B_color, blink, in_string) {}
#define  sb_ConWriteStringUnp(col, row, F_color, B_color, blink, in_string) {}
#define  sb_ConWriteStringNum(col, row, F_color, B_color, blink, in_string, num) {}
#endif

u8*      sb_DARAlloc(SB_DCB_PTR dar, u32 sz, u32 align);
bool     sb_DARFreeLast(SB_DCB_PTR dar);
bool     sb_DARInit(SB_DCB_PTR dar, u8* pi, u32 sz, bool fill, u32 fillval);
void     sb_GetLocalTime(DATETIME* pDateTime);
void     sb_HFM(void);
u32      sb_Peek(SB_PK_PAR par);
void     sb_MsgDisplay(void);
void     sb_MsgOut(u8 mtype, const char* mp);
void     sb_MSFill(void);
void     sb_MSFillResv(u32 resv);
u32      sb_MSScan(void);
u32      sb_PowerDown(u32 power_mode);
u32      sb_read32_unaligned(u8* addr);
void     sb_TMInit(void);
void     sb_TMStart(u32* pts);
void     sb_TMEnd(u32 ts, u32* ptm);
void     sb_TMLsr(void);
bool     sb_UFM(void);
void     sb_write32_unaligned(u8* addr, u32 val);

#ifdef __cplusplus
}
#endif

/*===========================================================================*
*                              smxBase MACROS                                *
*===========================================================================*/

#define  sb_MsgConstDisplay(mtype, mp) \
         { \
            sb_MsgOut(mtype, mp); \
            sb_MsgDisplay(); \
         }

#define  sb_MsgVarDisplay(mtype, mp) \
         { \
            sb_MsgOut(mtype, mp); \
            sb_MsgDisplay(); \
         }

/* utility macros */
#define  sb_BCD_BYTE_TO_DECIMAL(num) (((num) & 0x0F) + ((((num) & 0xF0)>>4) * 10))
#define  sb_DECIMAL_TO_BCD_BYTE(num) (((((num)%100)/10)<<4) | ((num)%10))   /* max 99; higher values truncated (e.g. 100 --> 00) */
#define  sb_INVERT_U16(v16)          (u16)(((uint)((v16) & 0x00FF) << 8) | ((uint)((v16) & 0xFF00) >> 8))
#define  sb_INVERT_U32(v32)          (u32)(((u32)(v32) << 24) | (((u32)(v32) << 8) & 0x00FF0000) | (((u32)(v32) >> 8) & 0x0000FF00L) | ((u32)(v32) >> 24))
#define  sb_MIN(a, b)                (((a) > (b)) ? (b) : (a))
#define  sb_MAX(a, b)                (((a) > (b)) ? (a) : (b))
#define  sb_LOU16(l)                 ((u16)(l))
#define  sb_HIU16(l)                 ((u16)(((l) & 0xFFFF0000L)>>16))
#define  sb_MAKEU32(h,l)             ((u32)(((u32)(h)<<16) | (l)))

/* time measurement macros */
#if SB_CFG_TM
#define  sb_TM_INIT()                {sb_TMInit(); /*<1>*/ \
                                     sb_TMInit();}
#define  sb_TM_START(p)              sb_TMStart(p);
#define  sb_TM_END(p, q)             sb_TMEnd(p, q);
#define  sb_TM_LSR()                 sb_TMLsr();
#else
#define  sb_TM_INIT()
#define  sb_TM_START(p)
#define  sb_TM_END(p, q)
#define  sb_TM_LSR()
#endif

/*===========================================================================*
*                                 BSP API                                    *
*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/* interrupt handling functions and macros */
bool     sb_IntCtrlInit(void);
void     sb_IntStateRestore(CPU_FL prev_state);
CPU_FL   sb_IntStateSaveDisable(void);
bool     sb_IntTrapVectSet(int int_num, ISR_PTR isr_ptr);
ISR_PTR  sb_IntVectGet(int int_num, u32* extra_info);
bool     sb_IntVectSet(int int_num, ISR_PTR isr_ptr);
bool     sb_IRQClear(int irq_num);
bool     sb_IRQConfig(int irq_num);
bool     sb_IRQEnd(int irq_num);
bool     sb_IRQMask(int irq_num);
bool     sb_IRQUnmask(int irq_num);
bool     sb_IRQsMask(void);
bool     sb_IRQsUnmask(void);
int      sb_IRQToInt(int irq_num);
ISR_PTR  sb_IRQVectGet(int irq_num, u32* extra_info);
bool     sb_IRQVectSet(int irq_num, ISR_PTR isr_ptr);
bool     sb_ISRInstall(int irq_num, uint par, ISRC_PTR fun, const char* name);

#define  sb_INT_DISABLE_S(s)  s = sb_IntStateSaveDisable()
#define  sb_INT_ENABLE_R(s)   sb_IntStateRestore(s)

/* memory functions */
void*    sb_DMABufferAlloc(uint num_bytes);
bool     sb_DMABufferFree(void* buf);

/* time functions */
void     sb_DelayUsec(u32 num);
u32      sb_PtimeGet(void);
bool     sb_StimeSet(void);
bool     sb_TickInit(void);
bool     sb_TickIntEnable(void);

#define  sb_DelayMsec(num) sb_DelayUsec(1000*(num))

/* misc functions */
bool     sb_ConsoleInInit(void);
bool     sb_ConsoleOutInit(void);
bool     sb_EVBInit(void);
void     sb_Exit(int retcode);
bool     sb_PeripheralsInit(void);
void     sb_Reboot(void);

void     sb_HWInitAtMain(void);

/* LCD / OLED */
void     sb_LCDInit(void);
void     sb_LCDWriteString(const char* str, uint line, u32 color, u32 attr);

/* LED */
void     sb_LEDInit(void);
void     sb_LEDWrite7Seg(u32 val);
void     sb_LEDWrite7SegNum(int num);
void     sb_LEDWriteRow(u32 val);

/* UART */
#if SB_CFG_UARTI
void     sb_UartInit(u32 parity, u32 dbit, u32 sbit);
#else
char     sb_UartGetCharPoll(void);
void     sb_UartInit(u8 port, u32 baudrate);
#endif

void     sb_UartOpen(u32 parity, u32 dbit, u32 sbit);
void     sb_UartOutData(u8 *psrc, u32 len);

/* Missing C library functions -- see bbase.c */
char*    itoa(int val, char *str, int radix);
char*    ltoa(long val, char *str, int radix);
int      putchar(int c);
void     reverse(char* s);
char*    ultoa(unsigned long val, char *str, int radix);

#ifdef __cplusplus
}
#endif

/* Notes:
   1. The first call to TMInit() primes the caches. 
*/
#endif /* SB_BAPI_H */
