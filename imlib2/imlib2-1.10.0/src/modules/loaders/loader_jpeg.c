#include "config.h"
#include "Imlib2_Loader.h"

#include <jpeglib.h>
#include <setjmp.h>
#include "exif.h"

#define DBG_PFX "LDR-jpg"

static const char  *const _formats[] = { "jpg", "jpeg", "jfif", "jfi" };

typedef struct {
   struct jpeg_error_mgr jem;
   sigjmp_buf          setjmp_buffer;
   uint8_t            *data;
} ImLib_JPEG_data;

static void
_JPEGFatalErrorHandler(j_common_ptr jcp)
{
   ImLib_JPEG_data    *jd = (ImLib_JPEG_data *) jcp->err;

#if 0
   jcp->err->output_message(jcp);
#endif
   siglongjmp(jd->setjmp_buffer, 1);
}

static void
_JPEGErrorHandler(j_common_ptr jcp)
{
#if 0
   ImLib_JPEG_data    *jd = (ImLib_JPEG_data *) jcp->err;

   jcp->err->output_message(jcp);
   siglongjmp(jd->setjmp_buffer, 1);
#endif
}

static void
_JPEGErrorHandler2(j_common_ptr jcp, int msg_level)
{
#if 0
   ImLib_JPEG_data    *jd = (ImLib_JPEG_data *) jcp->err;

   jcp->err->output_message(jcp);
   siglongjmp(jd->setjmp_buffer, 1);
#endif
}

static struct jpeg_error_mgr *
_jdata_init(ImLib_JPEG_data * jd)
{
   struct jpeg_error_mgr *jem;

   jem = jpeg_std_error(&jd->jem);

   jd->jem.error_exit = _JPEGFatalErrorHandler;
   jd->jem.emit_message = _JPEGErrorHandler2;
   jd->jem.output_message = _JPEGErrorHandler;

   jd->data = NULL;

   return jem;
}

static int
_load(ImlibImage * im, int load_data)
{
   int                 w, h, rc;
   struct jpeg_decompress_struct jds;
   ImLib_JPEG_data     jdata;
   uint8_t            *ptr, *line[16];
   uint32_t           *ptr2;
   int                 x, y, l, scans, inc;
   ExifInfo            ei = { 0 };

   rc = LOAD_FAIL;

   /* set up error handling */
   jds.err = _jdata_init(&jdata);
   if (sigsetjmp(jdata.setjmp_buffer, 1))
      QUIT_WITH_RC(LOAD_FAIL);

   jpeg_create_decompress(&jds);
   jpeg_mem_src(&jds, im->fi->fdata, im->fi->fsize);
   jpeg_save_markers(&jds, JPEG_APP0 + 1, 256);
   jpeg_read_header(&jds, TRUE);

   rc = LOAD_BADIMAGE;          /* Format accepted */

   /* Get orientation */
   ei.orientation = ORIENT_TOPLEFT;

   if (jds.marker_list)
     {
        jpeg_saved_marker_ptr m = jds.marker_list;

        D("Markers: %p: m=%02x len=%d/%d\n", m,
          m->marker, m->original_length, m->data_length);

        exif_parse(m->data, m->data_length, &ei);
     }

   w = jds.image_width;
   h = jds.image_height;
   if (!IMAGE_DIMENSIONS_OK(w, h))
      goto quit;

   if (ei.swap_wh)
     {
        im->w = h;
        im->h = w;
     }
   else
     {
        im->w = w;
        im->h = h;
     }

   if (!load_data)
      QUIT_WITH_RC(LOAD_SUCCESS);

   /* Load data */

   jds.do_fancy_upsampling = FALSE;
   jds.do_block_smoothing = FALSE;
   jpeg_start_decompress(&jds);

   if ((jds.rec_outbuf_height > 16) || (jds.output_components <= 0))
      goto quit;

   jdata.data = malloc(w * 16 * jds.output_components);
   if (!jdata.data)
      QUIT_WITH_RC(LOAD_OOM);

   /* must set the im->data member before callign progress function */
   ptr2 = __imlib_AllocateData(im);
   if (!ptr2)
      QUIT_WITH_RC(LOAD_OOM);

   for (y = 0; y < jds.rec_outbuf_height; y++)
      line[y] = jdata.data + (y * w * jds.output_components);

   for (l = 0; l < h; l += jds.rec_outbuf_height)
     {
        jpeg_read_scanlines(&jds, line, jds.rec_outbuf_height);

        scans = jds.rec_outbuf_height;
        if ((h - l) < scans)
           scans = h - l;

        for (y = 0; y < scans; y++)
          {
             ptr = line[y];

             switch (ei.orientation)
               {
               default:
               case ORIENT_TOPLEFT:
                  ptr2 = im->data + (l + y) * w;
                  inc = 1;
                  break;
               case ORIENT_TOPRIGHT:
                  ptr2 = im->data + (l + y) * w + w - 1;
                  inc = -1;
                  break;
               case ORIENT_BOTRIGHT:
                  ptr2 = im->data + (h - 1 - (l + y)) * w + w - 1;
                  inc = -1;
                  break;
               case ORIENT_BOTLEFT:
                  ptr2 = im->data + (h - 1 - (l + y)) * w;
                  inc = 1;
                  break;
               case ORIENT_LEFTTOP:
                  ptr2 = im->data + (l + y);
                  inc = h;
                  break;
               case ORIENT_RIGHTTOP:
                  ptr2 = im->data + (h - 1 - (l + y));
                  inc = h;
                  break;
               case ORIENT_RIGHTBOT:
                  ptr2 = im->data + (h - 1 - (l + y)) + (w - 1) * h;
                  inc = -h;
                  break;
               case ORIENT_LEFTBOT:
                  ptr2 = im->data + (l + y) + (w - 1) * h;
                  inc = -h;
                  break;
               }
             D("l,s,y=%d,%d, %d - x,y=%4ld,%4ld\n", l, y, l + y,
               (ptr2 - im->data) % im->w, (ptr2 - im->data) / im->w);

             switch (jds.out_color_space)
               {
               default:
                  goto quit;
               case JCS_GRAYSCALE:
                  for (x = 0; x < w; x++)
                    {
                       *ptr2 = PIXEL_ARGB(0xff, ptr[0], ptr[0], ptr[0]);
                       ptr++;
                       ptr2 += inc;
                    }
                  break;
               case JCS_RGB:
                  for (x = 0; x < w; x++)
                    {
                       *ptr2 = PIXEL_ARGB(0xff, ptr[0], ptr[1], ptr[2]);
                       ptr += jds.output_components;
                       ptr2 += inc;
                    }
                  break;
               case JCS_CMYK:
                  for (x = 0; x < w; x++)
                    {
                       *ptr2 = PIXEL_ARGB(0xff, ptr[0] * ptr[3] / 255,
                                          ptr[1] * ptr[3] / 255,
                                          ptr[2] * ptr[3] / 255);
                       ptr += jds.output_components;
                       ptr2 += inc;
                    }
                  break;
               }
          }

        if (ei.orientation != ORIENT_TOPLEFT &&
            ei.orientation != ORIENT_TOPRIGHT)
           continue;

        if (im->lc && __imlib_LoadProgressRows(im, l, scans))
           QUIT_WITH_RC(LOAD_BREAK);
     }
   if (ei.orientation != ORIENT_TOPLEFT && ei.orientation != ORIENT_TOPRIGHT)
     {
        if (im->lc)
           __imlib_LoadProgressRows(im, 0, im->h);
     }

   jpeg_finish_decompress(&jds);

   rc = LOAD_SUCCESS;

 quit:
   jpeg_destroy_decompress(&jds);
   free(jdata.data);

   return rc;
}

static int
_save(ImlibImage * im)
{
   int                 rc;
   struct jpeg_compress_struct jcs;
   ImLib_JPEG_data     jdata;
   FILE               *f;
   uint8_t            *buf;
   uint32_t           *ptr;
   JSAMPROW           *jbuf;
   int                 y, quality, compression;
   ImlibImageTag      *tag;
   int                 i, j;

   /* allocate a small buffer to convert image data */
   buf = malloc(im->w * 3 * sizeof(uint8_t));
   if (!buf)
      return LOAD_FAIL;

   rc = LOAD_FAIL;

   f = fopen(im->fi->name, "wb");
   if (!f)
      goto quit;

   /* set up error handling */
   jcs.err = _jdata_init(&jdata);
   if (sigsetjmp(jdata.setjmp_buffer, 1))
      goto quit;

   /* setup compress params */
   jpeg_create_compress(&jcs);
   jpeg_stdio_dest(&jcs, f);
   jcs.image_width = im->w;
   jcs.image_height = im->h;
   jcs.input_components = 3;
   jcs.in_color_space = JCS_RGB;

   /* look for tags attached to image to get extra parameters like quality */
   /* settigns etc. - this is the "api" to hint for extra information for */
   /* saver modules */

   /* compression */
   compression = 2;
   tag = __imlib_GetTag(im, "compression");
   if (tag)
     {
        compression = tag->val;
        if (compression < 0)
           compression = 0;
        if (compression > 9)
           compression = 9;
     }
   /* convert to quality */
   quality = (9 - compression) * 10;
   quality = quality * 10 / 9;
   /* quality */
   tag = __imlib_GetTag(im, "quality");
   if (tag)
      quality = tag->val;
   if (quality < 1)
      quality = 1;
   if (quality > 100)
      quality = 100;

   /* set up jepg compression parameters */
   jpeg_set_defaults(&jcs);
   jpeg_set_quality(&jcs, quality, TRUE);

   /* progressive */
   if ((tag = __imlib_GetTag(im, "interlacing")) && tag->val)
      jpeg_simple_progression(&jcs);

   jpeg_start_compress(&jcs, TRUE);
   /* get the start pointer */
   ptr = im->data;
   /* go one scanline at a time... and save */
   for (y = 0; jcs.next_scanline < jcs.image_height; y++)
     {
        /* convcert scaline from ARGB to RGB packed */
        for (j = 0, i = 0; i < im->w; i++)
          {
             uint32_t            pixel = *ptr++;

             buf[j++] = PIXEL_R(pixel);
             buf[j++] = PIXEL_G(pixel);
             buf[j++] = PIXEL_B(pixel);
          }
        /* write scanline */
        jbuf = (JSAMPROW *) (&buf);
        jpeg_write_scanlines(&jcs, jbuf, 1);

        if (im->lc && __imlib_LoadProgressRows(im, y, 1))
           QUIT_WITH_RC(LOAD_BREAK);
     }

   rc = LOAD_SUCCESS;

 quit:
   /* finish off */
   jpeg_finish_compress(&jcs);
   jpeg_destroy_compress(&jcs);
   free(buf);
   fclose(f);

   return rc;
}

IMLIB_LOADER(_formats, _load, _save);
