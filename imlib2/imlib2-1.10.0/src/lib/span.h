#ifndef __SPAN
#define __SPAN 1

#include "types.h"

typedef void        (*ImlibPointDrawFunction)(uint32_t, uint32_t *);

ImlibPointDrawFunction
__imlib_GetPointDrawFunction(ImlibOp op, char dst_alpha, char blend);

typedef void        (*ImlibSpanDrawFunction)(uint32_t, uint32_t *, int);

ImlibSpanDrawFunction
__imlib_GetSpanDrawFunction(ImlibOp op, char dst_alpha, char blend);

typedef void        (*ImlibShapedSpanDrawFunction)(uint8_t *, uint32_t,
                                                   uint32_t *, int);

ImlibShapedSpanDrawFunction
__imlib_GetShapedSpanDrawFunction(ImlibOp op, char dst_alpha, char blend);

#endif
