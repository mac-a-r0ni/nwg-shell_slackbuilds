#ifndef __ROTATE
#define __ROTATE 1

#include "types.h"

/*\ Calc precision \*/
#define _ROTATE_PREC 12
#define _ROTATE_PREC_MAX (1 << _ROTATE_PREC)
#define _ROTATE_PREC_BITS (_ROTATE_PREC_MAX - 1)

void                __imlib_RotateSample(uint32_t * src, uint32_t * dest,
                                         int sow, int sw, int sh,
                                         int dow, int dw, int dh,
                                         int x, int y,
                                         int dxh, int dyh, int dxv, int dyv);
void                __imlib_RotateAA(uint32_t * src, uint32_t * dest,
                                     int sow, int sw, int sh,
                                     int dow, int dw, int dh,
                                     int x, int y, int dx, int dy,
                                     int dxv, int dyv);
void                __imlib_BlendImageToImageSkewed(ImlibImage * im_src,
                                                    ImlibImage * im_dst,
                                                    char aa, char blend,
                                                    char merge_alpha,
                                                    int ssx, int ssy,
                                                    int ssw, int ssh,
                                                    int ddx, int ddy,
                                                    int hsx, int hsy,
                                                    int vsx, int vsy,
                                                    ImlibColorModifier * cm,
                                                    ImlibOp op,
                                                    int clx, int cly,
                                                    int clw, int clh);

#ifdef DO_MMX_ASM
void                __imlib_mmx_RotateAA(uint32_t * src, uint32_t * dest,
                                         int sow, int sw, int sh, int dow,
                                         int dw, int dh, int x, int y, int dx,
                                         int dy, int dxv, int dyv);
#endif
#endif
