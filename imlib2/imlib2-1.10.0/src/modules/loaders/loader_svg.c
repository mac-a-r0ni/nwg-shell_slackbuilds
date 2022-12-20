#include "config.h"
#include "Imlib2_Loader.h"

#include <math.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcomment"
#include <librsvg/rsvg.h>
#pragma GCC diagnostic pop

#define DBG_PFX "LDR-svg"

static const char  *const _formats[] = { "svg" };

#define DPI 96

#define MATCHSTR(ptr, len, str) (len >= sizeof(str) && memcmp(ptr, str, sizeof(str) - 1) == 0)
#define FINDSTR(ptr, len, str) (memmem(ptr, len, str, sizeof(str) - 1) != 0)

static int
_sig_check(const unsigned char *buf, unsigned int len)
{
   /* May also be compressed? - forget for now */

   if (len > 4096)
      len = 4096;
   if (MATCHSTR(buf, len, "<svg"))
      return 0;
   if (!MATCHSTR(buf, len, "<?xml version=") &&
       !MATCHSTR(buf, len, "<!--") && !MATCHSTR(buf, len, "<!DOCTYPE svg"))
      return 1;
   return FINDSTR(buf, len, "<svg") ? 0 : 1;
}

#if LIBRSVG_CHECK_VERSION(2, 46, 0)

static double
u2pix(double x, int unit)
{
   switch (unit)
     {
     default:
     case RSVG_UNIT_PERCENT:   /* 0  percentage values where 1.0 means 100% */
        return 0;               /* Size should be determined otherwise */
     case RSVG_UNIT_PX:        /* 1  pixels */
     case RSVG_UNIT_EM:        /* 2  em, or the current font size */
     case RSVG_UNIT_EX:        /* 3  x-height of the current font */
        return x;
     case RSVG_UNIT_IN:        /* 4  inches */
        return x * DPI;
     case RSVG_UNIT_CM:        /* 5  centimeters */
        return x * DPI / 2.54;
     case RSVG_UNIT_MM:        /* 6  millimeters */
        return x * DPI / 25.4;
     case RSVG_UNIT_PT:        /* 7  points, or 1/72 inch */
        return x * DPI / 72;
     case RSVG_UNIT_PC:        /* 8  picas, or 1/6 inch (12 points) */
        return x * DPI / 6;
     }
}

#endif /* LIBRSVG need 2.46 */

static void
_handle_error(GError * error)
{
   D("librsvg2: %s\n", error->message);
   g_error_free(error);
}

static int
_load(ImlibImage * im, int load_data)
{
   int                 rc;
   RsvgHandle         *rsvg;
   GError             *error;
   gboolean            ok;
   cairo_surface_t    *surface;
   cairo_t            *cr;

   rc = LOAD_FAIL;
   error = NULL;
   rsvg = NULL;
   surface = NULL;
   cr = NULL;

   /* Signature check */
   if (_sig_check(im->fi->fdata, im->fi->fsize))
      goto quit;

   rsvg = rsvg_handle_new_from_data(im->fi->fdata, im->fi->fsize, &error);
   if (!rsvg)
      goto quit;

   rc = LOAD_BADIMAGE;          /* Format accepted */

#if LIBRSVG_CHECK_VERSION(2, 46, 0)
   {
      gboolean            out_has_width, out_has_height, out_has_viewbox;
      RsvgLength          out_width = { }, out_height = { };
      RsvgRectangle       out_viewbox = { };
      rsvg_handle_get_intrinsic_dimensions(rsvg,
                                           &out_has_width,
                                           &out_width,
                                           &out_has_height,
                                           &out_height,
                                           &out_has_viewbox, &out_viewbox);
      D("WH:%d%d %.1fx%.1f (%d/%d: %.1fx%.1f) VB:%d %.1f,%.1f %.1fx%.1f\n",
        out_has_width, out_has_height,
        out_width.length, out_height.length, out_width.unit, out_height.unit,
        u2pix(out_width.length, out_width.unit),
        u2pix(out_height.length, out_height.unit),
        out_has_viewbox,
        out_viewbox.x, out_viewbox.y, out_viewbox.width, out_viewbox.height);

      if (out_has_width && out_has_height)
        {
           im->w = lrint(u2pix(out_width.length, out_width.unit));
           im->h = lrint(u2pix(out_height.length, out_height.unit));
           D("Choose rsvg_handle_get_intrinsic_dimensions width/height\n");
#if !IMLIB2_DEBUG
           if (im->w > 0 && im->h > 0)
              goto got_size;
#endif
        }

      if (out_has_viewbox && (im->w <= 0 || im->h <= 0))
        {
           im->w = lrint(out_viewbox.width);
           im->h = lrint(out_viewbox.height);
           D("Choose rsvg_handle_get_intrinsic_dimensions viewbox\n");
#if !IMLIB2_DEBUG
           if (im->w > 0 && im->h > 0)
              goto got_size;
#endif
        }
   }
#endif /* LIBRSVG need 2.46 */

#if 0
#if LIBRSVG_CHECK_VERSION(2, 52, 0)
   {
      gdouble             dw = 0, dh = 0;

      ok = rsvg_handle_get_intrinsic_size_in_pixels(rsvg, &dw, &dh);
      D("ok=%d WxH=%.1fx%.1f\n", ok, dw, dh);
      if (ok && (im->w <= 0 || im->w <= 0))
        {
           im->w = lrint(dw);
           im->h = lrint(dh);
           D("Choose rsvg_handle_get_intrinsic_size_in_pixels width/height\n");
#if !IMLIB2_DEBUG
           if (im->w > 0 && im->h > 0)
              goto got_size;
#endif
        }
   }
#endif
#endif /* LIBRSVG need 2.52 */

#if LIBRSVG_CHECK_VERSION(2, 46, 0)
   {
      RsvgRectangle       out_ink_rect = { }, out_logical_rect = { };

      ok = rsvg_handle_get_geometry_for_element(rsvg, NULL,
                                                &out_ink_rect,
                                                &out_logical_rect, &error);
      D("ok=%d Ink: %.1f,%.1f %.1fx%.1f Log: %.1f,%.1f %.1fx%.1f\n", ok,
        out_ink_rect.x, out_ink_rect.y, out_ink_rect.width, out_ink_rect.height,
        out_logical_rect.x, out_logical_rect.y, out_logical_rect.width,
        out_logical_rect.height);
      if (ok && (im->w <= 0 || im->w <= 0))
        {
           im->w = lrint(out_ink_rect.width);
           im->h = lrint(out_ink_rect.height);
           D("Choose rsvg_handle_get_geometry_for_element ink rect width/height\n");
#if !IMLIB2_DEBUG
           if (im->w > 0 && im->h > 0)
              goto got_size;
#endif
        }
   }
#endif /* LIBRSVG need 2.46 */

#if !IMLIB2_DEBUG
 got_size:
#endif
   if (!IMAGE_DIMENSIONS_OK(im->w, im->h))
      goto quit;

   im->has_alpha = 1;

   if (!load_data)
      QUIT_WITH_RC(LOAD_SUCCESS);

   /* Load data */

   if (!__imlib_AllocateData(im))
      QUIT_WITH_RC(LOAD_OOM);

   memset(im->data, 0, im->w * im->h * sizeof(uint32_t));
   surface =
      cairo_image_surface_create_for_data((void *)im->data, CAIRO_FORMAT_ARGB32,
                                          im->w, im->h,
                                          im->w * sizeof(uint32_t));;
   if (!surface)
      QUIT_WITH_RC(LOAD_OOM);

   cr = cairo_create(surface);
   if (!cr)
      QUIT_WITH_RC(LOAD_OOM);

#if LIBRSVG_CHECK_VERSION(2, 46, 0)
   {
      RsvgRectangle       cvb;

      cvb.x = cvb.y = 0;
      cvb.width = im->w;
      cvb.height = im->h;
      rsvg_handle_render_document(rsvg, cr, &cvb, &error);
   }
#endif /* LIBRSVG need 2.46 */

   if (im->lc)
      __imlib_LoadProgress(im, 0, 0, im->w, im->h);

   rc = LOAD_SUCCESS;

 quit:
   if (error)
      _handle_error(error);
   if (surface)
      cairo_surface_destroy(surface);
   if (cr)
      cairo_destroy(cr);
   if (rsvg)
      g_object_unref(rsvg);

   return rc;
}

IMLIB_LOADER(_formats, _load, NULL);
