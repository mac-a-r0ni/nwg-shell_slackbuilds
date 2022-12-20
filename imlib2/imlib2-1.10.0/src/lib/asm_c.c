#include "common.h"

#include <stdlib.h>

#include "asm_c.h"

#if defined(DO_MMX_ASM) || defined(DO_AMD64_ASM)
#if DO_MMX_ASM
#define CPUID_MMX (1 << 23)
#define CPUID_XMM (1 << 25)
int                 __imlib_get_cpuid(void);
#endif

int
__imlib_do_asm(void)
{
   static char         _cpu_can_asm = -1;

   if (_cpu_can_asm < 0)
     {
        if (getenv("IMLIB2_ASM_OFF"))
           _cpu_can_asm = 0;
        else
#if DO_MMX_ASM
           _cpu_can_asm = !!(__imlib_get_cpuid() & CPUID_MMX);
#elif DO_AMD64_ASM
           _cpu_can_asm = 1;    // instruction set is always present
#endif
     }

   return _cpu_can_asm;
}

#endif
