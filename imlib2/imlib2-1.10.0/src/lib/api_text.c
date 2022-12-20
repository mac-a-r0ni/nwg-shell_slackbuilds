#include "config.h"
#include <Imlib2.h>
#include "common.h"

#include <math.h>

#include "api.h"
#include "file.h"
#include "image.h"
#include "font.h"

EAPI                Imlib_Font
imlib_load_font(const char *font_name)
{
   return __imlib_font_load_joined(font_name);
}

EAPI void
imlib_free_font(void)
{
   CHECK_PARAM_POINTER("font", ctx->font);
   __imlib_font_free(ctx->font);
   ctx->font = NULL;
}

EAPI void
imlib_context_set_font(Imlib_Font font)
{
   ctx->font = font;
}

EAPI                Imlib_Font
imlib_context_get_font(void)
{
   return ctx->font;
}

EAPI void
imlib_context_set_direction(Imlib_Text_Direction direction)
{
   ctx->direction = direction;
}

EAPI                Imlib_Text_Direction
imlib_context_get_direction(void)
{
   return ctx->direction;
}

EAPI void
imlib_context_set_angle(double angle)
{
   ctx->angle = angle;
}

EAPI double
imlib_context_get_angle(void)
{
   return ctx->angle;
}

EAPI int
imlib_insert_font_into_fallback_chain(Imlib_Font font, Imlib_Font fallback_font)
{
   CHECK_PARAM_POINTER_RETURN("font", font, 1);
   CHECK_PARAM_POINTER_RETURN("fallback_font", fallback_font, 1);
   return __imlib_font_insert_into_fallback_chain_imp(font, fallback_font);
}

EAPI void
imlib_remove_font_from_fallback_chain(Imlib_Font fallback_font)
{
   CHECK_PARAM_POINTER("fallback_font", fallback_font);
   __imlib_font_remove_from_fallback_chain_imp(fallback_font);
}

EAPI                Imlib_Font
imlib_get_prev_font_in_fallback_chain(Imlib_Font fn)
{
   CHECK_PARAM_POINTER_RETURN("fn", fn, 0);
   return ((ImlibFont *) fn)->fallback_prev;
}

EAPI                Imlib_Font
imlib_get_next_font_in_fallback_chain(Imlib_Font fn)
{
   CHECK_PARAM_POINTER_RETURN("fn", fn, 0);
   return ((ImlibFont *) fn)->fallback_next;
}

EAPI void
imlib_text_draw(int x, int y, const char *text)
{
   imlib_text_draw_with_return_metrics(x, y, text, NULL, NULL, NULL, NULL);
}

EAPI void
imlib_text_draw_with_return_metrics(int x, int y, const char *text,
                                    int *width_return, int *height_return,
                                    int *horizontal_advance_return,
                                    int *vertical_advance_return)
{
   ImlibImage         *im;
   ImlibFont          *fn;
   int                 dir;

   CHECK_PARAM_POINTER("font", ctx->font);
   CHECK_PARAM_POINTER("image", ctx->image);
   CHECK_PARAM_POINTER("text", text);
   CAST_IMAGE(im, ctx->image);
   if (__imlib_LoadImageData(im))
      return;
   fn = (ImlibFont *) ctx->font;
   __imlib_DirtyImage(im);

   dir = ctx->direction;
   if (ctx->direction == IMLIB_TEXT_TO_ANGLE && ctx->angle == 0.0)
      dir = IMLIB_TEXT_TO_RIGHT;

   __imlib_render_str(im, fn, x, y, text, ctx->pixel, dir,
                      ctx->angle, width_return, height_return, 0,
                      horizontal_advance_return, vertical_advance_return,
                      ctx->operation,
                      ctx->cliprect.x, ctx->cliprect.y,
                      ctx->cliprect.w, ctx->cliprect.h);
}

EAPI void
imlib_get_text_size(const char *text, int *width_return, int *height_return)
{
   ImlibFont          *fn;
   int                 w, h;
   int                 dir;

   CHECK_PARAM_POINTER("font", ctx->font);
   CHECK_PARAM_POINTER("text", text);
   fn = (ImlibFont *) ctx->font;

   dir = ctx->direction;
   if (ctx->direction == IMLIB_TEXT_TO_ANGLE && ctx->angle == 0.0)
      dir = IMLIB_TEXT_TO_RIGHT;

   __imlib_font_query_size(fn, text, &w, &h);

   switch (dir)
     {
     case IMLIB_TEXT_TO_RIGHT:
     case IMLIB_TEXT_TO_LEFT:
        if (width_return)
           *width_return = w;
        if (height_return)
           *height_return = h;
        break;
     case IMLIB_TEXT_TO_DOWN:
     case IMLIB_TEXT_TO_UP:
        if (width_return)
           *width_return = h;
        if (height_return)
           *height_return = w;
        break;
     case IMLIB_TEXT_TO_ANGLE:
        if (width_return || height_return)
          {
             double              sa, ca;

             sa = sin(ctx->angle);
             ca = cos(ctx->angle);

             if (width_return)
               {
                  double              x1, x2, xt;

                  x1 = x2 = 0.0;
                  xt = ca * w;
                  if (xt < x1)
                     x1 = xt;
                  if (xt > x2)
                     x2 = xt;
                  xt = -(sa * h);
                  if (xt < x1)
                     x1 = xt;
                  if (xt > x2)
                     x2 = xt;
                  xt = ca * w - sa * h;
                  if (xt < x1)
                     x1 = xt;
                  if (xt > x2)
                     x2 = xt;
                  *width_return = (int)(x2 - x1);
               }
             if (height_return)
               {
                  double              y1, y2, yt;

                  y1 = y2 = 0.0;
                  yt = sa * w;
                  if (yt < y1)
                     y1 = yt;
                  if (yt > y2)
                     y2 = yt;
                  yt = ca * h;
                  if (yt < y1)
                     y1 = yt;
                  if (yt > y2)
                     y2 = yt;
                  yt = sa * w + ca * h;
                  if (yt < y1)
                     y1 = yt;
                  if (yt > y2)
                     y2 = yt;
                  *height_return = (int)(y2 - y1);
               }
          }
        break;
     default:
        break;
     }
}

EAPI void
imlib_get_text_advance(const char *text, int *horizontal_advance_return,
                       int *vertical_advance_return)
{
   ImlibFont          *fn;
   int                 w, h;

   CHECK_PARAM_POINTER("font", ctx->font);
   CHECK_PARAM_POINTER("text", text);
   fn = (ImlibFont *) ctx->font;
   __imlib_font_query_advance(fn, text, &w, &h);
   if (horizontal_advance_return)
      *horizontal_advance_return = w;
   if (vertical_advance_return)
      *vertical_advance_return = h;
}

EAPI int
imlib_get_text_inset(const char *text)
{
   ImlibFont          *fn;

   CHECK_PARAM_POINTER_RETURN("font", ctx->font, 0);
   CHECK_PARAM_POINTER_RETURN("text", text, 0);
   fn = (ImlibFont *) ctx->font;
   return __imlib_font_query_inset(fn, text);
}

EAPI void
imlib_add_path_to_font_path(const char *path)
{
   CHECK_PARAM_POINTER("path", path);
   if (!__imlib_font_path_exists(path))
      __imlib_font_add_font_path(path);
}

EAPI void
imlib_remove_path_from_font_path(const char *path)
{
   CHECK_PARAM_POINTER("path", path);
   __imlib_font_del_font_path(path);
}

EAPI char         **
imlib_list_font_path(int *number_return)
{
   CHECK_PARAM_POINTER_RETURN("number_return", number_return, NULL);
   return __imlib_font_list_font_path(number_return);
}

EAPI int
imlib_text_get_index_and_location(const char *text, int x, int y,
                                  int *char_x_return, int *char_y_return,
                                  int *char_width_return,
                                  int *char_height_return)
{
   ImlibFont          *fn;
   int                 w, h, cx, cy, cw, ch, cp, xx, yy;
   int                 dir;

   CHECK_PARAM_POINTER_RETURN("font", ctx->font, -1);
   CHECK_PARAM_POINTER_RETURN("text", text, -1);
   fn = (ImlibFont *) ctx->font;

   dir = ctx->direction;
   if (ctx->direction == IMLIB_TEXT_TO_ANGLE && ctx->angle == 0.0)
      dir = IMLIB_TEXT_TO_RIGHT;

   imlib_get_text_size(text, &w, &h);

   switch (dir)
     {
     case IMLIB_TEXT_TO_RIGHT:
        xx = x;
        yy = y;
        break;
     case IMLIB_TEXT_TO_LEFT:
        xx = w - x;
        yy = h - y;
        break;
     case IMLIB_TEXT_TO_DOWN:
        xx = y;
        yy = w - x;
        break;
     case IMLIB_TEXT_TO_UP:
        xx = h - y;
        yy = x;
        break;
     default:
        return -1;
     }

   cp = __imlib_font_query_text_at_pos(fn, text, xx, yy, &cx, &cy, &cw, &ch);

   switch (dir)
     {
     case IMLIB_TEXT_TO_RIGHT:
        if (char_x_return)
           *char_x_return = cx;
        if (char_y_return)
           *char_y_return = cy;
        if (char_width_return)
           *char_width_return = cw;
        if (char_height_return)
           *char_height_return = ch;
        return cp;
        break;
     case IMLIB_TEXT_TO_LEFT:
        cx = 1 + w - cx - cw;
        if (char_x_return)
           *char_x_return = cx;
        if (char_y_return)
           *char_y_return = cy;
        if (char_width_return)
           *char_width_return = cw;
        if (char_height_return)
           *char_height_return = ch;
        return cp;
        break;
     case IMLIB_TEXT_TO_DOWN:
        if (char_x_return)
           *char_x_return = cy;
        if (char_y_return)
           *char_y_return = cx;
        if (char_width_return)
           *char_width_return = ch;
        if (char_height_return)
           *char_height_return = cw;
        return cp;
        break;
     case IMLIB_TEXT_TO_UP:
        cy = 1 + h - cy - ch;
        if (char_x_return)
           *char_x_return = cy;
        if (char_y_return)
           *char_y_return = cx;
        if (char_width_return)
           *char_width_return = ch;
        if (char_height_return)
           *char_height_return = cw;
        return cp;
        break;
     default:
        return -1;
        break;
     }
   return -1;
}

EAPI void
imlib_text_get_location_at_index(const char *text, int index,
                                 int *char_x_return, int *char_y_return,
                                 int *char_width_return,
                                 int *char_height_return)
{
   ImlibFont          *fn;
   int                 cx, cy, cw, ch, w, h;

   CHECK_PARAM_POINTER("font", ctx->font);
   CHECK_PARAM_POINTER("text", text);
   fn = (ImlibFont *) ctx->font;

   __imlib_font_query_char_coords(fn, text, index, &cx, &cy, &cw, &ch);

   w = h = 0;
   imlib_get_text_size(text, &w, &h);

   switch (ctx->direction)
     {
     case IMLIB_TEXT_TO_RIGHT:
        if (char_x_return)
           *char_x_return = cx;
        if (char_y_return)
           *char_y_return = cy;
        if (char_width_return)
           *char_width_return = cw;
        if (char_height_return)
           *char_height_return = ch;
        return;
        break;
     case IMLIB_TEXT_TO_LEFT:
        cx = 1 + w - cx - cw;
        if (char_x_return)
           *char_x_return = cx;
        if (char_y_return)
           *char_y_return = cy;
        if (char_width_return)
           *char_width_return = cw;
        if (char_height_return)
           *char_height_return = ch;
        return;
        break;
     case IMLIB_TEXT_TO_DOWN:
        if (char_x_return)
           *char_x_return = cy;
        if (char_y_return)
           *char_y_return = cx;
        if (char_width_return)
           *char_width_return = ch;
        if (char_height_return)
           *char_height_return = cw;
        return;
        break;
     case IMLIB_TEXT_TO_UP:
        cy = 1 + h - cy - ch;
        if (char_x_return)
           *char_x_return = cy;
        if (char_y_return)
           *char_y_return = cx;
        if (char_width_return)
           *char_width_return = ch;
        if (char_height_return)
           *char_height_return = cw;
        return;
        break;
     default:
        return;
        break;
     }
}

EAPI char         **
imlib_list_fonts(int *number_return)
{
   CHECK_PARAM_POINTER_RETURN("number_return", number_return, NULL);
   return __imlib_font_list_fonts(number_return);
}

EAPI void
imlib_free_font_list(char **font_list, int number)
{
   __imlib_FileFreeDirList(font_list, number);
}

EAPI int
imlib_get_font_cache_size(void)
{
   return __imlib_font_cache_get();
}

EAPI void
imlib_set_font_cache_size(int bytes)
{
   __imlib_font_cache_set(bytes);
}

EAPI void
imlib_flush_font_cache(void)
{
   __imlib_font_flush();
}

EAPI int
imlib_get_font_ascent(void)
{
   CHECK_PARAM_POINTER_RETURN("font", ctx->font, 0);
   return __imlib_font_ascent_get(ctx->font);
}

EAPI int
imlib_get_font_descent(void)
{
   CHECK_PARAM_POINTER_RETURN("font", ctx->font, 0);
   return __imlib_font_descent_get(ctx->font);
}

EAPI int
imlib_get_maximum_font_ascent(void)
{
   CHECK_PARAM_POINTER_RETURN("font", ctx->font, 0);
   return __imlib_font_max_ascent_get(ctx->font);
}

EAPI int
imlib_get_maximum_font_descent(void)
{
   CHECK_PARAM_POINTER_RETURN("font", ctx->font, 0);
   return __imlib_font_max_descent_get(ctx->font);
}
