#include "common.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "debug.h"

#if IMLIB2_DEBUG

__EXPORT__ unsigned int __imlib_debug = 0;

static FILE        *opt_fout = NULL;

__attribute__((constructor))
     static void         _debug_init(void)
{
   const char         *s;
   int                 p1, p2;

   opt_fout = stdout;

   s = getenv("IMLIB2_DEBUG");
   if (!s)
      return;

   p1 = p2 = 0;
   sscanf(s, "%i:%i", &p1, &p2);

   __imlib_debug = p1;
   opt_fout = (p2) ? stderr : stdout;
}

#if USE_MONOTONIC_CLOCK
#include <time.h>
#else
#include <sys/time.h>
#endif

__EXPORT__ unsigned int
__imlib_time_us(void)
{
#if USE_MONOTONIC_CLOCK
   struct timespec     ts;

   clock_gettime(CLOCK_MONOTONIC, &ts);

   return (unsigned int)(ts.tv_sec * 1000000 + ts.tv_nsec / 1000);
#else
   struct timeval      timev;

   gettimeofday(&timev, NULL);

   return (unsigned int)(timev.tv_sec * 1000000 + timev.tv_usec);
#endif
}

__EXPORT__ void
__imlib_printf(const char *pfx, const char *fmt, ...)
{
   char                fmtx[1024];
   va_list             args;

   va_start(args, fmt);

   if (pfx)
     {
        snprintf(fmtx, sizeof(fmtx), "%-4s: %s", pfx, fmt);
        fmt = fmtx;
     }
   vfprintf(opt_fout, fmt, args);
   va_end(args);
}

#endif /* IMLIB2_DEBUG */
