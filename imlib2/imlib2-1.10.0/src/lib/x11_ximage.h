#ifndef X11_XIMAGE_H
#define X11_XIMAGE_H 1

#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>

void                __imlib_SetXImageCacheCountMax(Display * d, int num);
int                 __imlib_GetXImageCacheCountMax(Display * d);
int                 __imlib_GetXImageCacheCountUsed(Display * d);
void                __imlib_SetXImageCacheSizeMax(Display * d, int num);
int                 __imlib_GetXImageCacheSizeMax(Display * d);
int                 __imlib_GetXImageCacheSizeUsed(Display * d);
void                __imlib_FlushXImage(Display * d);
void                __imlib_ConsumeXImage(Display * d, XImage * xim);
XImage             *__imlib_ProduceXImage(Display * d, Visual * v, int depth,
                                          int w, int h, char *shared);
XImage             *__imlib_ShmGetXImage(Display * d, Visual * v, Drawable draw,
                                         int depth, int x, int y, int w, int h,
                                         XShmSegmentInfo * si);
void                __imlib_ShmDestroyXImage(Display * d, XImage * xim,
                                             XShmSegmentInfo * si);

#endif /* X11_XIMAGE_H */
