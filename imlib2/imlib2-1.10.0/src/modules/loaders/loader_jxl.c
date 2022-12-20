#include "config.h"
#include "Imlib2_Loader.h"

#define MAX_RUNNERS 4           /* Maybe set to Ncpu/2? */

#include <jxl/decode.h>
#if MAX_RUNNERS > 0
#include <jxl/thread_parallel_runner.h>
#endif

#define DBG_PFX "LDR-jxl"

static const char  *const _formats[] = { "jxl" };

static void
_scanline_cb(void *opaque, size_t x, size_t y,
             size_t num_pixels, const void *pixels)
{
   ImlibImage         *im = opaque;
   const uint8_t      *pix = pixels;
   uint32_t           *ptr;
   size_t              i;

   DL("%s: x,y=%ld,%ld len=%lu\n", __func__, x, y, num_pixels);

   ptr = im->data + (im->w * y) + x;

   /* libjxl outputs ABGR pixels (stream order RGBA) - convert to ARGB */
   for (i = 0; i < num_pixels; i++, pix += 4)
      *ptr++ = PIXEL_ARGB(pix[3], pix[0], pix[1], pix[2]);

   /* Due to possible multithreading it's probably best not do do
    * progress calbacks here. */
}

static int
_load(ImlibImage * im, int load_data)
{
   static const JxlPixelFormat pbuf_fmt = {
      .num_channels = 4,
      .data_type = JXL_TYPE_UINT8,
      .endianness = JXL_NATIVE_ENDIAN,
   };

   int                 rc;
   JxlDecoderStatus    jst;
   JxlDecoder         *dec;
   JxlBasicInfo        info;
   JxlFrameHeader      fhdr;
   int                 frame, delay_unit;
   ImlibImageFrame    *pf;

#if MAX_RUNNERS > 0
   size_t              n_runners;
   JxlParallelRunner  *runner = NULL;
#endif

   rc = LOAD_FAIL;
   dec = NULL;

   switch (JxlSignatureCheck(im->fi->fdata, 128))
     {
     default:
//   case JXL_SIG_NOT_ENOUGH_BYTES:
//   case JXL_SIG_INVALID:
        goto quit;
     case JXL_SIG_CODESTREAM:
     case JXL_SIG_CONTAINER:
        break;
     }

   rc = LOAD_BADIMAGE;          /* Format accepted */

   dec = JxlDecoderCreate(NULL);
   if (!dec)
      goto quit;

#if MAX_RUNNERS > 0
   n_runners = JxlThreadParallelRunnerDefaultNumWorkerThreads();
   if (n_runners > MAX_RUNNERS)
      n_runners = MAX_RUNNERS;
   D("n_runners = %ld\n", n_runners);
   runner = JxlThreadParallelRunnerCreate(NULL, n_runners);
   if (!runner)
      goto quit;

   jst = JxlDecoderSetParallelRunner(dec, JxlThreadParallelRunner, runner);
   if (jst != JXL_DEC_SUCCESS)
      goto quit;
#endif /* MAX_RUNNERS */

   jst =
      JxlDecoderSubscribeEvents(dec,
                                JXL_DEC_BASIC_INFO | JXL_DEC_FRAME |
                                JXL_DEC_FULL_IMAGE);
   if (jst != JXL_DEC_SUCCESS)
      goto quit;

   jst = JxlDecoderSetInput(dec, im->fi->fdata, im->fi->fsize);
   if (jst != JXL_DEC_SUCCESS)
      goto quit;

   delay_unit = 0;
   frame = im->frame;
   pf = NULL;

   for (;;)
     {
        jst = JxlDecoderProcessInput(dec);
        DL("Event 0x%04x\n", jst);
        switch (jst)
          {
          default:
             D("Unexpected status: %d\n", jst);
             goto quit;

          case JXL_DEC_BASIC_INFO:
             jst = JxlDecoderGetBasicInfo(dec, &info);
             if (jst != JXL_DEC_SUCCESS)
                goto quit;

             D("WxH=%dx%d alpha=%d bps=%d nc=%d+%d\n", info.xsize, info.ysize,
               info.alpha_bits, info.bits_per_sample,
               info.num_color_channels, info.num_extra_channels);
             if (info.have_animation)
               {
                  D("anim=%d loops=%u tps_num/den=%u/%u\n", info.have_animation,
                    info.animation.num_loops, info.animation.tps_numerator,
                    info.animation.tps_denominator);
                  delay_unit = info.animation.tps_denominator * 1000;
                  if (info.animation.tps_numerator > 0)
                     delay_unit /= info.animation.tps_numerator;
               }

             if (!IMAGE_DIMENSIONS_OK(info.xsize, info.ysize))
                goto quit;

             im->w = info.xsize;
             im->h = info.ysize;
             im->has_alpha = info.alpha_bits > 0;

             if (frame > 0)
               {
                  if (info.have_animation)
                    {
                       pf = __imlib_GetFrame(im);
                       if (!pf)
                          QUIT_WITH_RC(LOAD_OOM);
                       pf->frame_count = 1234567890;    // FIXME - Hack
                       pf->loop_count = info.animation.num_loops;
                       pf->frame_flags |= FF_IMAGE_ANIMATED;
                       pf->canvas_w = info.xsize;
                       pf->canvas_h = info.ysize;

                       D("Canvas WxH=%dx%d frames=%d repeat=%d\n",
                         pf->canvas_w, pf->canvas_h,
                         pf->frame_count, pf->loop_count);

                       if (frame > 1 && pf->frame_count > 0
                           && frame > pf->frame_count)
                          QUIT_WITH_RC(LOAD_BADFRAME);
                    }

                  if (frame > 1)
                    {
                       /* Fast forward to desired frame */
                       JxlDecoderSkipFrames(dec, frame - 1);
                    }
               }

             if (!load_data)
                QUIT_WITH_RC(LOAD_SUCCESS);
             break;

          case JXL_DEC_NEED_IMAGE_OUT_BUFFER:
             if (!__imlib_AllocateData(im))
                QUIT_WITH_RC(LOAD_OOM);

             jst = JxlDecoderSetImageOutCallback(dec, &pbuf_fmt,
                                                 _scanline_cb, im);
             if (jst != JXL_DEC_SUCCESS)
                goto quit;
             break;

          case JXL_DEC_FRAME:
             if (!pf)
                break;
             JxlDecoderGetFrameHeader(dec, &fhdr);
             if (fhdr.is_last)
                pf->frame_count = frame;
             pf->frame_delay = fhdr.duration * delay_unit;
             D("Frame duration=%d tc=%08x nl=%d last=%d\n",
               pf->frame_delay, fhdr.timecode, fhdr.name_length, fhdr.is_last);
             break;

          case JXL_DEC_FULL_IMAGE:
             goto done;
          }
     }
 done:

   if (im->lc)
      __imlib_LoadProgress(im, 0, 0, im->w, im->h);

   rc = LOAD_SUCCESS;

 quit:
#if MAX_RUNNERS > 0
   if (runner)
      JxlThreadParallelRunnerDestroy(runner);
#endif
   if (dec)
      JxlDecoderDestroy(dec);

   return rc;
}

IMLIB_LOADER(_formats, _load, NULL);
