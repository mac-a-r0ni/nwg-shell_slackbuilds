#ifndef __FILTER
#define __FILTER 1

typedef struct {
   int                 xoff, yoff;
   int                 a, r, g, b;
} ImlibFilterPixel;

typedef struct {
   int                 size, entries;
   int                 div, cons;
   ImlibFilterPixel   *pixels;
} ImlibFilterColor;

typedef struct {
   ImlibFilterColor    alpha, red, green, blue;
} ImlibFilter;

ImlibFilter        *__imlib_CreateFilter(int size);
void                __imlib_FreeFilter(ImlibFilter * fil);
void                __imlib_FilterSet(ImlibFilterColor * fil, int x, int y,
                                      int a, int r, int g, int b);
void                __imlib_FilterSetColor(ImlibFilterColor * fil, int x, int y,
                                           int a, int r, int g, int b);
void                __imlib_FilterDivisors(ImlibFilter * fil,
                                           int a, int r, int g, int b);
void                __imlib_FilterConstants(ImlibFilter * fil,
                                            int a, int r, int g, int b);
void                __imlib_FilterImage(ImlibImage * im, ImlibFilter * fil);

#endif
