/*
 * Loader for HEIF images.
 *
 * Only loads the primary image for any file, whether it be a still image or an
 * image sequence.
 */
#include "config.h"
#include "Imlib2_Loader.h"

#include <libheif/heif.h>

static const char  *const _formats[] =
   { "heif", "heifs", "heic", "heics", "avci", "avcs", "avif", "avifs" };

#define HEIF_BYTES_TO_CHECK 12L
#define HEIF_8BIT_TO_PIXEL_ARGB(plane, has_alpha) \
   PIXEL_ARGB((has_alpha) ? (plane)[3] : 0xff, (plane)[0], (plane)[1], (plane)[2])

static int
_load(ImlibImage * im, int load_data)
{
   int                 rc;
   int                 img_has_alpha;
   int                 stride = 0;
   int                 bytes_per_px;
   int                 y, x;
   struct heif_error   error;
   struct heif_context *ctx = NULL;
   struct heif_image_handle *img_handle = NULL;
   struct heif_image  *img_data = NULL;
   struct heif_decoding_options *decode_opts = NULL;
   uint32_t           *ptr;
   const uint8_t      *img_plane = NULL;

   rc = LOAD_FAIL;

   /* input data needs to be atleast 12 bytes */
   if (im->fi->fsize < HEIF_BYTES_TO_CHECK)
      return rc;

   /* check signature */
   switch (heif_check_filetype(im->fi->fdata, im->fi->fsize))
     {
     case heif_filetype_no:
     case heif_filetype_yes_unsupported:
        goto quit;

        /* Have to let heif_filetype_maybe through because mif1 brand gives
         * heif_filetype_maybe on check */
     case heif_filetype_maybe:
     case heif_filetype_yes_supported:
        break;
     }

   ctx = heif_context_alloc();
   if (!ctx)
      goto quit;

   error = heif_context_read_from_memory_without_copy(ctx, im->fi->fdata,
                                                      im->fi->fsize, NULL);
   if (error.code != heif_error_Ok)
      goto quit;

   error = heif_context_get_primary_image_handle(ctx, &img_handle);
   if (error.code != heif_error_Ok)
      goto quit;

   rc = LOAD_BADIMAGE;          /* Format accepted */

   /* Freeing heif_context, since we got primary image handle */
   heif_context_free(ctx);
   ctx = NULL;

   im->w = heif_image_handle_get_width(img_handle);
   im->h = heif_image_handle_get_height(img_handle);
   if (!IMAGE_DIMENSIONS_OK(im->w, im->h))
      goto quit;

   img_has_alpha = heif_image_handle_has_alpha_channel(img_handle);
   im->has_alpha = img_has_alpha;

   if (!load_data)
     {
        rc = LOAD_SUCCESS;
        goto quit;
     }

   /* load data */

   /* Set decoding option to convert HDR to 8-bit if libheif>=1.7.0 and
    * successful allocation of heif_decoding_options */
#if LIBHEIF_HAVE_VERSION(1, 7, 0)
   decode_opts = heif_decoding_options_alloc();
   if (decode_opts)
      decode_opts->convert_hdr_to_8bit = 1;
#endif
   error =
      heif_decode_image(img_handle, &img_data, heif_colorspace_RGB,
                        img_has_alpha ? heif_chroma_interleaved_RGBA :
                        heif_chroma_interleaved_RGB, decode_opts);
   heif_decoding_options_free(decode_opts);
   decode_opts = NULL;
   if (error.code != heif_error_Ok)
      goto quit;

   im->w = heif_image_get_width(img_data, heif_channel_interleaved);
   im->h = heif_image_get_height(img_data, heif_channel_interleaved);
   if (!IMAGE_DIMENSIONS_OK(im->w, im->h))
      goto quit;
   ptr = __imlib_AllocateData(im);
   if (!ptr)
      goto quit;

   img_plane =
      heif_image_get_plane_readonly(img_data, heif_channel_interleaved,
                                    &stride);
   if (!img_plane)
      goto quit;

   /* Divide the number of bits per pixel by 8, always rounding up */
   bytes_per_px =
      (heif_image_get_bits_per_pixel(img_data, heif_channel_interleaved) +
       7) >> 3;
   /* If somehow bytes_per_pixel < 1, set it to 1 */
   bytes_per_px = bytes_per_px < 1 ? 1 : bytes_per_px;

   /* Correct the stride, since img_plane will be incremented after each pixel */
   stride -= im->w * bytes_per_px;
   for (y = 0; y != im->h; y++, img_plane += stride)
     {
        for (x = 0; x != im->w; x++, img_plane += bytes_per_px)
           *(ptr++) = HEIF_8BIT_TO_PIXEL_ARGB(img_plane, img_has_alpha);

        /* Report progress of each row. */
        if (im->lc && __imlib_LoadProgressRows(im, y, 1))
          {
             rc = LOAD_BREAK;
             goto quit;
          }
     }
   rc = LOAD_SUCCESS;

 quit:
   /* Free memory if it is still allocated.
    * Working this way means explicitly setting pointers to NULL if they were
    * freed beforehand to avoid freeing twice. */
   /* decode_opts was freed as soon as decoding was complete */
   heif_image_release(img_data);
   heif_image_handle_release(img_handle);
   heif_context_free(ctx);
   heif_decoding_options_free(decode_opts);

   return rc;
}

IMLIB_LOADER(_formats, _load, NULL);
