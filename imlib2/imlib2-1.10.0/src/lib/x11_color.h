#ifndef X11_COLOR_H
#define X11_COLOR_H 1

#include <X11/Xlib.h>
#include "types.h"

typedef enum {
   PAL_TYPE_332,                /* 0 */
   PAL_TYPE_232,                /* 1 */
   PAL_TYPE_222,                /* 2 */
   PAL_TYPE_221,                /* 3 */
   PAL_TYPE_121,                /* 4 */
   PAL_TYPE_111,                /* 5 */
   PAL_TYPE_1,                  /* 6 */
   PAL_TYPE_666,                /* 7 */
} ImlibPalType;

extern unsigned short _max_colors;

int                 __imlib_XActualDepth(Display * d, Visual * v);
Visual             *__imlib_BestVisual(Display * d, int screen,
                                       int *depth_return);

uint8_t            *__imlib_AllocColorTable(Display * d, Colormap cmap,
                                            unsigned char *type_return,
                                            Visual * v);

#endif /* X11_COLOR_H */
