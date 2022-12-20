#ifndef IMLIB2_DEDUG_H
#define IMLIB2_DEDUG_H

#if IMLIB2_DEBUG

#define D(fmt...)     if (__imlib_debug)     __imlib_printf(DBG_PFX, fmt)
#define DC(M, fmt...) if (__imlib_debug & M) __imlib_printf(DBG_PFX, fmt)

#define DBG_FILE	0x0001
#define DBG_LOAD	0x0002
#define DBG_LDR 	0x0004
#define DBG_LDR2	0x0008

#if __LOADER_COMMON_H
#undef D
#define D(fmt...)  DC(DBG_LDR, fmt)
#define DL(fmt...) DC(DBG_LDR2, fmt)
#endif

extern unsigned int __imlib_debug;

__PRINTF_2__ void   __imlib_printf(const char *pfx, const char *fmt, ...);

unsigned int        __imlib_time_us(void);

#else

#define D(fmt...)
#define DC(fmt...)
#if __LOADER_COMMON_H
#define DL(fmt...)
#endif

#endif /* IMLIB2_DEBUG */

#endif /* IMLIB2_DEDUG_H */
