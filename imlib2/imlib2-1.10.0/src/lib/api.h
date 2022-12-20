
#include "rgbadraw.h"

/* convenience macros */
#define   CAST_IMAGE(im, image) (im) = (ImlibImage *)(image)
#define   CHECK_PARAM_POINTER_RETURN(sparam, param, ret) \
if (!(param)) \
{ \
  fprintf(stderr, "***** Imlib2 Developer Warning ***** :\n" \
                  "\tThis program is calling the Imlib call:\n\n" \
                  "\t%s();\n\n" \
                  "\tWith the parameter:\n\n" \
                  "\t%s\n\n" \
                  "\tbeing NULL. Please fix your program.\n", __func__, sparam); \
  return ret; \
}

#define   CHECK_PARAM_POINTER(sparam, param) \
if (!(param)) \
{ \
  fprintf(stderr, "***** Imlib2 Developer Warning ***** :\n" \
                  "\tThis program is calling the Imlib call:\n\n" \
                  "\t%s();\n\n" \
                  "\tWith the parameter:\n\n" \
                  "\t%s\n\n" \
                  "\tbeing NULL. Please fix your program.\n", __func__, sparam); \
  return; \
}

/* internal typedefs for function pointers */
typedef void        (*Imlib_Internal_Progress_Function)(void *, char, int, int,
                                                        int, int);
typedef void        (*Imlib_Internal_Data_Destructor_Function)(void *, void *);

typedef struct {
#ifdef BUILD_X11
   Display            *display;
   Visual             *visual;
   Colormap            colormap;
   int                 depth;
   Drawable            drawable;
   Pixmap              mask;
#endif
   int                 error;
   char                anti_alias;
   char                dither;
   char                blend;
   Imlib_Color_Modifier color_modifier;
   ImlibOp             operation;
   Imlib_Color         color;
   uint32_t            pixel;
   Imlib_Color_Range   color_range;
   Imlib_Image         image;
   Imlib_Image_Data_Memory_Function image_data_memory_func;
   Imlib_Progress_Function progress_func;
   char                progress_granularity;
   char                dither_mask;
   int                 mask_alpha_threshold;
   Imlib_Rectangle     cliprect;
   int                 references;
   char                dirty;
#if ENABLE_FILTERS
   Imlib_Filter        filter;
#endif
#if ENABLE_TEXT
   Imlib_Font          font;
   Imlib_Text_Direction direction;
   double              angle;
#endif
} ImlibContext;

extern ImlibContext *ctx;
