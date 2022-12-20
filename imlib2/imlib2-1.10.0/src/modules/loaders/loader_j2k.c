#include "config.h"
#include "Imlib2_Loader.h"

#include <openjpeg.h>

#define DBG_PFX "LDR-j2k"

#define JP2_RFC3745_MAGIC "\x00\x00\x00\x0c\x6a\x50\x20\x20\x0d\x0a\x87\x0a"
#define JP2_MAGIC "\x0d\x0a\x87\x0a"
/* position 45: "\xff\x52" */
#define J2K_CODESTREAM_MAGIC "\xff\x4f\xff\x51"

static const char  *const _formats[] = { "jp2", "j2k" };

#if IMLIB2_DEBUG
static void
_j2k_cb(const char *type, const char *msg, void *data)
{
   DL("%s: %p: %s: %s", __func__, data, type, msg);
}

static void
_j2k_cb_info(const char *msg, void *data)
{
   _j2k_cb("info", msg, data);
}

static void
_j2k_cb_warn(const char *msg, void *data)
{
   _j2k_cb("warn", msg, data);
}

static void
_j2k_cb_err(const char *msg, void *data)
{
   _j2k_cb("err", msg, data);
}
#endif /*IMLIB2_DEBUG */

static struct {
   const unsigned char *data, *dptr;
   unsigned int        size;
} mdata;

static void
mm_init(const void *src, unsigned int size)
{
   mdata.data = mdata.dptr = src;
   mdata.size = size;
}

static              OPJ_SIZE_T
mm_read(void *dst, OPJ_SIZE_T len, void *data)
{
   DL("%s: len=%ld\n", __func__, len);

   if (mdata.dptr >= mdata.data + mdata.size)
      return -1;                /* Out of data */
   if (mdata.dptr + len > mdata.data + mdata.size)
      len = mdata.data + mdata.size - mdata.dptr;

   memcpy(dst, mdata.dptr, len);
   mdata.dptr += len;

   return len;
}

static              OPJ_OFF_T
mm_seek_cur(OPJ_OFF_T offs, void *data)
{
   DL("%s: offs=%ld\n", __func__, offs);

   if (mdata.dptr + offs > mdata.data + mdata.size)
      return 0;                 /* Out of data */

   mdata.dptr += offs;

   return mdata.dptr - mdata.data;
}

static              OPJ_BOOL
mm_seek_set(OPJ_OFF_T offs, void *data)
{
   DL("%s: offs=%ld\n", __func__, offs);

   if (offs > mdata.size)
      return OPJ_FALSE;         /* Out of data */

   mdata.dptr = mdata.data + offs;

   return OPJ_TRUE;
}

static int
_load(ImlibImage * im, int load_data)
{
   int                 rc;
   int                 ok;
   opj_dparameters_t   jparam;
   opj_codec_t        *jcodec;
   opj_stream_t       *jstream;
   opj_image_t        *jimage;
   OPJ_CODEC_FORMAT    jfmt;
   int                 i, j;
   uint32_t           *dst;
   OPJ_INT32          *pa, *pr, *pg, *pb;
   unsigned char       a, r, g, b;

   rc = LOAD_FAIL;
   jcodec = NULL;
   jstream = NULL;
   jimage = NULL;

   /* Signature check */
   if (im->fi->fsize < 12)
      goto quit;

   if (memcmp(im->fi->fdata, JP2_MAGIC, 4) == 0 ||
       memcmp(im->fi->fdata, JP2_RFC3745_MAGIC, 12) == 0)
      jfmt = OPJ_CODEC_JP2;
   else if (memcmp(im->fi->fdata, J2K_CODESTREAM_MAGIC, 4) == 0)
      jfmt = OPJ_CODEC_J2K;
   else
      goto quit;

   DL("format=%d\n", jfmt);

   memset(&jparam, 0, sizeof(opj_dparameters_t));
   opj_set_default_decoder_parameters(&jparam);

   jcodec = opj_create_decompress(jfmt);
   if (!jcodec)
      goto quit;

   rc = LOAD_BADIMAGE;          /* Format accepted */

#if IMLIB2_DEBUG
   opj_set_info_handler(jcodec, _j2k_cb_info, NULL);
   opj_set_warning_handler(jcodec, _j2k_cb_warn, NULL);
   opj_set_error_handler(jcodec, _j2k_cb_err, NULL);
#endif

   ok = opj_setup_decoder(jcodec, &jparam);
   if (!ok)
      goto quit;

   // May be set with OPJ_NUM_THREADS=number or ALL_CPUS
// opj_codec_set_threads(jcodec, 4);

   if (getenv("JP2_USE_FILE"))
     {
        jstream = opj_stream_create_default_file_stream(im->fi->name, OPJ_TRUE);
     }
   else
     {
        jstream = opj_stream_create(OPJ_J2K_STREAM_CHUNK_SIZE, OPJ_TRUE);
        if (!jstream)
           goto quit;

        mm_init(im->fi->fdata, im->fi->fsize);
        opj_stream_set_user_data(jstream, &mdata, NULL);
        opj_stream_set_user_data_length(jstream, im->fi->fsize);
        opj_stream_set_read_function(jstream, mm_read);
        opj_stream_set_skip_function(jstream, mm_seek_cur);
        opj_stream_set_seek_function(jstream, mm_seek_set);
     }

   opj_read_header(jstream, jcodec, &jimage);
   if (!jimage)
      goto quit;
   im->w = jimage->x1 - jimage->x0;
   im->h = jimage->y1 - jimage->y0;
   im->has_alpha = jimage->numcomps == 4 || jimage->numcomps == 2;
   D("WxH=%dx%d alpha=%d numcomp=%d colorspace=%d\n",
     im->w, im->h, im->has_alpha, jimage->numcomps, jimage->color_space);

   for (i = 0; i < (int)jimage->numcomps; i++)
     {
        DL("%d: dx/y=%d/%d wxh=%d,%d prec=%d sgnd=%d fact=%d\n", i,
           jimage->comps[i].dx, jimage->comps[i].dy,
           jimage->comps[i].w, jimage->comps[i].h,
           jimage->comps[i].prec,
           jimage->comps[i].sgnd, jimage->comps[i].factor);
        if (jimage->comps[0].dx != jimage->comps[i].dx ||
            jimage->comps[0].dy != jimage->comps[i].dy ||
            (int)jimage->comps[i].w != im->w ||
            (int)jimage->comps[i].h != im->h)
           goto quit;
     }

   if (!load_data)
      QUIT_WITH_RC(LOAD_SUCCESS);

   /* Load data */

   ok = opj_decode(jcodec, jstream, jimage);
   if (!ok)
      goto quit;

   ok = opj_end_decompress(jcodec, jstream);
   if (!ok)
      goto quit;

   if (!__imlib_AllocateData(im))
      QUIT_WITH_RC(LOAD_OOM);

   /* Ignoring color_space and data format details... */

   dst = im->data;
   pa = jimage->comps[0].data;  /* Avoid compiler warning */

   switch (jimage->numcomps)
     {
     default:
        goto quit;

     case 4:                   /* RGBA */
        pa = jimage->comps[3].data;
        /* FALLTHROUGH */
     case 3:                   /* RGB  */
        pr = jimage->comps[0].data;
        pg = jimage->comps[1].data;
        pb = jimage->comps[2].data;
        for (i = 0; i < im->h; i++)
          {
             for (j = 0; j < im->w; j++)
               {
                  r = *pr++;
                  g = *pg++;
                  b = *pb++;
                  a = (jimage->numcomps == 4) ? *pa++ : 0xff;

                  *dst++ = PIXEL_ARGB(a, r, g, b);
               }

             if (im->lc && __imlib_LoadProgressRows(im, i, 1))
                QUIT_WITH_RC(LOAD_BREAK);
          }
        break;

     case 2:                   /* Gray with A */
        pa = jimage->comps[1].data;
        /* FALLTHROUGH */
     case 1:                   /* Gray */
        pg = jimage->comps[0].data;
        for (i = 0; i < im->h; i++)
          {
             for (j = 0; j < im->w; j++)
               {
                  g = *pg++;
                  a = (jimage->numcomps == 2) ? *pa++ : 0xff;

                  *dst++ = PIXEL_ARGB(a, g, g, g);
               }

             if (im->lc && __imlib_LoadProgressRows(im, i, 1))
                QUIT_WITH_RC(LOAD_BREAK);
          }
        break;
     }

   rc = LOAD_SUCCESS;

 quit:
   if (jimage)
      opj_image_destroy(jimage);
   if (jstream)
      opj_stream_destroy(jstream);
   if (jcodec)
      opj_destroy_codec(jcodec);

   return rc;
}

IMLIB_LOADER(_formats, _load, NULL);
