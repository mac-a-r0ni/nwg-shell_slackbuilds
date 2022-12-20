#include "config.h"
#include "Imlib2_Loader.h"

#include <webp/decode.h>
#include <webp/demux.h>
#include <webp/encode.h>

#define DBG_PFX "LDR-webp"

static const char  *const _formats[] = { "webp" };

static int
_load(ImlibImage * im, int load_data)
{
   int                 rc;
   WebPData            webp_data;
   WebPDemuxer        *demux;
   WebPIterator        iter;
   int                 frame, fcount;
   ImlibImageFrame    *pf;

   rc = LOAD_FAIL;

   if (im->fi->fsize < 12)
      return rc;

   webp_data.bytes = im->fi->fdata;
   webp_data.size = im->fi->fsize;

   /* Init (includes signature check) */
   demux = WebPDemux(&webp_data);
   if (!demux)
      goto quit;

   rc = LOAD_BADIMAGE;          /* Format accepted */

   pf = NULL;
   frame = im->frame;
   if (frame > 0)
     {
        fcount = WebPDemuxGetI(demux, WEBP_FF_FRAME_COUNT);
        if (frame > 1 && frame > fcount)
           QUIT_WITH_RC(LOAD_BADFRAME);

        pf = __imlib_GetFrame(im);
        if (!pf)
           QUIT_WITH_RC(LOAD_OOM);
        pf->frame_count = fcount;
        pf->loop_count = WebPDemuxGetI(demux, WEBP_FF_LOOP_COUNT);
        if (pf->frame_count > 1)
           pf->frame_flags |= FF_IMAGE_ANIMATED;
        pf->canvas_w = WebPDemuxGetI(demux, WEBP_FF_CANVAS_WIDTH);
        pf->canvas_h = WebPDemuxGetI(demux, WEBP_FF_CANVAS_HEIGHT);

        D("Canvas WxH=%dx%d frames=%d repeat=%d\n",
          pf->canvas_w, pf->canvas_h, pf->frame_count, pf->loop_count);
     }
   else
     {
        frame = 1;
     }

   if (!WebPDemuxGetFrame(demux, frame, &iter))
      goto quit;

   WebPDemuxReleaseIterator(&iter);

   im->w = iter.width;
   im->h = iter.height;
   if (pf)
     {
        pf->frame_x = iter.x_offset;
        pf->frame_y = iter.y_offset;
        pf->frame_delay = iter.duration;
        if (iter.dispose_method == WEBP_MUX_DISPOSE_BACKGROUND)
           pf->frame_flags |= FF_FRAME_DISPOSE_CLEAR;
        if (iter.blend_method == WEBP_MUX_BLEND)
           pf->frame_flags |= FF_FRAME_BLEND;

        D("Canvas WxH=%dx%d frame=%d/%d X,Y=%d,%d WxH=%dx%d alpha=%d T=%d dm=%d co=%d bl=%d\n", //
          pf->canvas_w, pf->canvas_h, iter.frame_num, pf->frame_count,
          pf->frame_x, pf->frame_y, im->w, im->h, iter.has_alpha,
          pf->frame_delay, iter.dispose_method, iter.complete,
          iter.blend_method);
     }

   if (!IMAGE_DIMENSIONS_OK(im->w, im->h))
      goto quit;

   im->has_alpha = iter.has_alpha;

   if (!load_data)
      QUIT_WITH_RC(LOAD_SUCCESS);

   /* Load data */

   if (!__imlib_AllocateData(im))
      QUIT_WITH_RC(LOAD_OOM);

   if (WebPDecodeBGRAInto
       (iter.fragment.bytes, iter.fragment.size, (uint8_t *) im->data,
        sizeof(uint32_t) * im->w * im->h, im->w * 4) == NULL)
      goto quit;

   if (im->lc)
      __imlib_LoadProgress(im, 0, 0, im->w, im->h);

   rc = LOAD_SUCCESS;

 quit:
   if (demux)
      WebPDemuxDelete(demux);

   return rc;
}

static int
_save(ImlibImage * im)
{
   FILE               *f;
   int                 rc;
   ImlibImageTag      *quality_tag;
   float               quality;
   uint8_t            *fdata;
   size_t              encoded_size;

   f = fopen(im->fi->name, "wb");
   if (!f)
      return LOAD_FAIL;

   rc = LOAD_FAIL;
   fdata = NULL;

   quality = 75;
   quality_tag = __imlib_GetTag(im, "quality");
   if (quality_tag)
     {
        quality = quality_tag->val;
        if (quality < 0)
          {
             fprintf(stderr,
                     "Warning: 'quality' setting %f too low for WebP, using 0\n",
                     quality);
             quality = 0;
          }

        if (quality > 100)
          {
             fprintf(stderr,
                     "Warning, 'quality' setting %f too high for WebP, using 100\n",
                     quality);
             quality = 100;
          }
     }

   encoded_size = WebPEncodeBGRA((uint8_t *) im->data, im->w, im->h,
                                 im->w * 4, quality, &fdata);

   if (fwrite(fdata, encoded_size, 1, f) != 1)
      goto quit;

   rc = LOAD_SUCCESS;

 quit:
   if (fdata)
      WebPFree(fdata);
   fclose(f);

   return rc;
}

IMLIB_LOADER(_formats, _load, _save);
