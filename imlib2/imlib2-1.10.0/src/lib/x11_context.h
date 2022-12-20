#ifndef X11_CONTEXT_H
#define X11_CONTEXT_H 1

#include <X11/Xlib.h>
#include "types.h"

typedef struct _Context {
   int                 last_use;
   Display            *display;
   Visual             *visual;
   Colormap            colormap;
   int                 depth;
   struct _Context    *next;

   uint8_t            *palette;
   unsigned char       palette_type;
   void               *r_dither;
   void               *g_dither;
   void               *b_dither;
} Context;

void                __imlib_SetMaxContexts(int num);
int                 __imlib_GetMaxContexts(void);
void                __imlib_FlushContexts(void);
Context            *__imlib_FindContext(Display * d, Visual * v, Colormap c,
                                        int depth);
Context            *__imlib_NewContext(Display * d, Visual * v, Colormap c,
                                       int depth);
Context            *__imlib_GetContext(Display * d, Visual * v, Colormap c,
                                       int depth);

#endif /* X11_CONTEXT_H */
