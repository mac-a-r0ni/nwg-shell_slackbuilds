#include "config.h"
#include <Imlib2.h>
#include "common.h"

#include <stdarg.h>
#include <stdio.h>

#include "api.h"
#include "image.h"
#include "dynamic_filters.h"
#include "filter.h"

EAPI void
imlib_image_filter(void)
{
   ImlibImage         *im;

   CHECK_PARAM_POINTER("image", ctx->image);
   CHECK_PARAM_POINTER("filter", ctx->filter);
   CAST_IMAGE(im, ctx->image);
   if (__imlib_LoadImageData(im))
      return;
   __imlib_DirtyImage(im);
   __imlib_FilterImage(im, (ImlibFilter *) ctx->filter);
}

EAPI                Imlib_Filter
imlib_create_filter(int initsize)
{
   return __imlib_CreateFilter(initsize);
}

EAPI void
imlib_free_filter(void)
{
   CHECK_PARAM_POINTER("filter", ctx->filter);
   __imlib_FreeFilter((ImlibFilter *) ctx->filter);
   ctx->filter = NULL;
}

EAPI void
imlib_context_set_filter(Imlib_Filter filter)
{
   ctx->filter = filter;
}

EAPI                Imlib_Filter
imlib_context_get_filter(void)
{
   return ctx->filter;
}

EAPI void
imlib_filter_set(int xoff, int yoff, int a, int r, int g, int b)
{
   ImlibFilter        *fil;

   CHECK_PARAM_POINTER("filter", ctx->filter);
   fil = (ImlibFilter *) ctx->filter;
   __imlib_FilterSetColor(&fil->alpha, xoff, yoff, a, 0, 0, 0);
   __imlib_FilterSetColor(&fil->red, xoff, yoff, 0, r, 0, 0);
   __imlib_FilterSetColor(&fil->green, xoff, yoff, 0, 0, g, 0);
   __imlib_FilterSetColor(&fil->blue, xoff, yoff, 0, 0, 0, b);
}

EAPI void
imlib_filter_set_alpha(int xoff, int yoff, int a, int r, int g, int b)
{
   ImlibFilter        *fil;

   CHECK_PARAM_POINTER("filter", ctx->filter);
   fil = (ImlibFilter *) ctx->filter;
   __imlib_FilterSetColor(&fil->alpha, xoff, yoff, a, r, g, b);
}

EAPI void
imlib_filter_set_red(int xoff, int yoff, int a, int r, int g, int b)
{
   ImlibFilter        *fil;

   CHECK_PARAM_POINTER("filter", ctx->filter);
   fil = (ImlibFilter *) ctx->filter;
   __imlib_FilterSetColor(&fil->red, xoff, yoff, a, r, g, b);
}

EAPI void
imlib_filter_set_green(int xoff, int yoff, int a, int r, int g, int b)
{
   ImlibFilter        *fil;

   CHECK_PARAM_POINTER("filter", ctx->filter);
   fil = (ImlibFilter *) ctx->filter;
   __imlib_FilterSetColor(&fil->green, xoff, yoff, a, r, g, b);
}

EAPI void
imlib_filter_set_blue(int xoff, int yoff, int a, int r, int g, int b)
{
   ImlibFilter        *fil;

   CHECK_PARAM_POINTER("filter", ctx->filter);
   fil = (ImlibFilter *) ctx->filter;
   __imlib_FilterSetColor(&fil->blue, xoff, yoff, a, r, g, b);
}

EAPI void
imlib_filter_constants(int a, int r, int g, int b)
{
   CHECK_PARAM_POINTER("filter", ctx->filter);
   __imlib_FilterConstants((ImlibFilter *) ctx->filter, a, r, g, b);
}

EAPI void
imlib_filter_divisors(int a, int r, int g, int b)
{
   CHECK_PARAM_POINTER("filter", ctx->filter);
   __imlib_FilterDivisors((ImlibFilter *) ctx->filter, a, r, g, b);
}

EAPI void
imlib_apply_filter(const char *script, ...)
{
   va_list             param_list;
   ImlibImage         *im;

   __imlib_dynamic_filters_init();
   CAST_IMAGE(im, ctx->image);
   if (__imlib_LoadImageData(im))
      return;
   __imlib_DirtyImage(im);
   va_start(param_list, script);
   __imlib_script_parse(im, script, param_list);
   va_end(param_list);
}
