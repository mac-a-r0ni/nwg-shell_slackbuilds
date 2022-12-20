#ifndef ASM_C_H
#define ASM_C_H 1

#if defined(DO_MMX_ASM) || defined(DO_AMD64_ASM)
int                 __imlib_do_asm(void);
#endif

#endif /* ASM_C_H */
