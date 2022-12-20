#ifndef __SCALE
#define __SCALE 1

#include "types.h"

typedef struct _imlib_scale_info ImlibScaleInfo;

ImlibScaleInfo     *__imlib_CalcScaleInfo(ImlibImage * im,
                                          int sw, int sh,
                                          int dw, int dh, char aa);
ImlibScaleInfo     *__imlib_FreeScaleInfo(ImlibScaleInfo * isi);
void                __imlib_ScaleSampleRGBA(ImlibScaleInfo * isi,
                                            uint32_t * dest, int dxx, int dyy,
                                            int dx, int dy, int dw, int dh,
                                            int dow);
void                __imlib_ScaleAARGBA(ImlibScaleInfo * isi, uint32_t * dest,
                                        int dxx, int dyy, int dx, int dy,
                                        int dw, int dh, int dow, int sow);
void                __imlib_ScaleAARGB(ImlibScaleInfo * isi, uint32_t * dest,
                                       int dxx, int dyy, int dx, int dy, int dw,
                                       int dh, int dow, int sow);

#ifdef DO_MMX_ASM
void                __imlib_Scale_mmx_AARGBA(ImlibScaleInfo * isi,
                                             uint32_t * dest,
                                             int dxx, int dyy, int dx, int dy,
                                             int dw, int dh, int dow, int sow);
#endif

#endif
