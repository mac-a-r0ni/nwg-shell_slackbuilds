/*
 * ANI loader
 *
 * File format:
 * https://en.wikipedia.org/wiki/Resource_Interchange_File_Format
 * https://www.daubnet.com/en/file-format-ani
 */
#include "config.h"
#include "Imlib2_Loader.h"

#if IMLIB2_DEBUG
#define Dx(fmt...) if (__imlib_debug & DBG_LDR) __imlib_printf(NULL, fmt)
#else
#define Dx(fmt...)
#endif

#define DBG_PFX "LDR-ani"

#define T(a,b,c,d) ((a << 0) | (b << 8) | (c << 16) | (d << 24))

#define RIFF_TYPE_RIFF	T('R', 'I', 'F', 'F')
#define RIFF_TYPE_LIST	T('L', 'I', 'S', 'T')
#define RIFF_TYPE_INAM	T('I', 'N', 'A', 'M')
#define RIFF_TYPE_IART	T('I', 'A', 'R', 'T')
#define RIFF_TYPE_icon	T('i', 'c', 'o', 'n')
#define RIFF_TYPE_anih	T('a', 'n', 'i', 'h')
#define RIFF_TYPE_rate	T('r', 'a', 't', 'e')
#define RIFF_TYPE_seq 	T('s', 'e', 'q', ' ')

#define RIFF_NAME_ACON	T('A', 'C', 'O', 'N')

static const char  *const _formats[] = { "ani" };

typedef struct {
   unsigned char       nest;
   int                 nframes, nfsteps;
   uint32_t           *rates, *seq;
} riff_ctx_t;

typedef struct {
   uint32_t            size;    // Size of chunk data (=36)
   uint32_t            frames;  // Number of frames in file
   uint32_t            steps;   // Number of steps in animation sequence
   uint32_t            width;   // Image width (raw data only?)
   uint32_t            height;  // Image height (raw data only?)
   uint32_t            bpp;     // Bits per pixel (raw data only?)
   uint32_t            planes;  // N. planes (raw data only?)
   uint32_t            rate;    // Default rate in 1/60s
   uint32_t            flags;   // Flags: ANIH_FLAG_...
} anih_data_t;

#define ANIH_FLAG_ICO	0x01    // Frames are icons or cursors (otherwiwe raw - bmp?)
#define ANIH_FLAG_SEQ	0x02    // Image contains seq chunk

static int
_load_embedded(ImlibImage * im, int load_data, const char *data,
               unsigned int size)
{
   int                 rc;
   ImlibLoader        *loader;
   int                 frame;
   void               *lc;

   loader = __imlib_FindBestLoader(NULL, "ico", 0);
   if (!loader)
      return LOAD_FAIL;

   /* Disable frame and progress handling in sub-loader */
   frame = im->frame;
   lc = im->lc;
   im->frame = 0;
   im->lc = NULL;

   rc = __imlib_LoadEmbeddedMem(loader, im, load_data, data, size);

   im->frame = frame;
   im->lc = lc;

   if (rc == LOAD_SUCCESS && im->lc)
      __imlib_LoadProgress(im, 0, 0, im->w, im->h);

   return rc;
}

#define LE32(p) (SWAP_LE_32(*((const uint32_t*)(p))))
#define OFFS(p) ((const char*)(p) - (const char*)im->fi->fdata)

static int
_riff_parse(ImlibImage * im, riff_ctx_t * ctx, const char *fdata,
            unsigned int fsize, const char *fptr)
{
   int                 rc;
   unsigned int        type;
   int                 size, avail;
   int                 fcount, i;
   ImlibImageFrame    *pf;

   rc = LOAD_FAIL;
   ctx->nest += 1;

   pf = NULL;
   fcount = 0;

   for (; rc == 0; fptr += 8 + size)
     {
        avail = fdata + fsize - fptr;   /* Bytes left in chunk */

        if (avail <= 0)
           break;               /* Normal end (= 0) */
        if (avail < 8)
          {
             D("%5lu: %*s Chunk: %.4s premature end\n",
               OFFS(fptr), ctx->nest, "", fptr);
             break;
          }

        type = LE32(fptr);
        size = LE32(fptr + 4);

        D("%5lu: %*s Chunk: %.4s size %u: ",
          OFFS(fptr), ctx->nest, "", fptr, size);

        if (ctx->nest == 1 && fptr == fdata)
          {
             Dx("\n");
             /* First chunk of file */
             if (type != RIFF_TYPE_RIFF || (LE32(fptr + 8)) != RIFF_NAME_ACON)
                return LOAD_FAIL;
             size = 4;
             continue;
          }

        if (avail < 8 + size)
          {
             Dx("incorrect size\n");
             rc = LOAD_BADFILE;
             break;
          }

        switch (type)
          {
          default:
          case RIFF_TYPE_RIFF:
             Dx("\n");
             break;
          case RIFF_TYPE_INAM:
          case RIFF_TYPE_IART:
             Dx("'%.*s'\n", size, fptr + 8);
             break;
          case RIFF_TYPE_LIST:
             Dx("'%.*s'\n", 4, fptr + 8);
             rc = _riff_parse(im, ctx, fptr + 12, size - 4, fptr + 12);
             break;
          case RIFF_TYPE_icon:
             Dx("\n");
             fcount++;
             if (im->frame > 0)
               {
                  i = (ctx->seq) ?
                     (int)SWAP_LE_32(ctx->seq[im->frame - 1]) + 1 : im->frame;
                  if (i != fcount)
                     break;
               }
             if (pf && ctx->rates)
                pf->frame_delay =
                   (1000 * SWAP_LE_32(ctx->rates[im->frame - 1])) / 60;
             rc = _load_embedded(im, 1, fptr + 8, size);
             break;
          case RIFF_TYPE_anih:
#define AH ((const anih_data_t*)(fptr + 8))
/**INDENT-OFF**/
             Dx("sz=%u nf=%u/%u WxH=%ux%u bc=%u np=%u dr=%u fl=%u\n",
	        SWAP_LE_32(AH->size), SWAP_LE_32(AH->frames), SWAP_LE_32(AH->steps),
	        SWAP_LE_32(AH->width), SWAP_LE_32(AH->height),
	        SWAP_LE_32(AH->bpp), SWAP_LE_32(AH->planes),
	        SWAP_LE_32(AH->rate), SWAP_LE_32(AH->flags));
/**INDENT-ON**/
             ctx->nframes = SWAP_LE_32(AH->frames);
             ctx->nfsteps = SWAP_LE_32(AH->steps);
             if (im->frame <= 0)
                break;
             if (ctx->nfsteps < ctx->nframes)
                ctx->nfsteps = ctx->nframes;
             if (im->frame > ctx->nfsteps)
                return LOAD_BADFRAME;
             pf = __imlib_GetFrame(im);
             if (!pf)
               {
                  rc = LOAD_OOM;
                  break;
               }
             pf->frame_count = ctx->nfsteps;
             if (ctx->nframes > 1)
                pf->frame_flags = FF_IMAGE_ANIMATED;
             pf->frame_delay = (1000 * SWAP_LE_32(AH->rate)) / 60;
             break;
          case RIFF_TYPE_rate:
             ctx->rates = (uint32_t *) (fptr + 8);
             if ((int)size != 4 * ctx->nfsteps)
               {
                  D("rate chunk size mismatch: %d != %d\n", size,
                    4 * ctx->nfsteps);
                  break;
               }
#if IMLIB2_DEBUG
             for (i = 0; i < ctx->nfsteps; i++)
                Dx(" %d", SWAP_LE_32(ctx->rates[i]));
#endif
             Dx("\n");
             break;
          case RIFF_TYPE_seq:
             ctx->seq = (uint32_t *) (fptr + 8);
             if ((int)size != 4 * ctx->nfsteps)
               {
                  D("seq chunk size mismatch: %d != %d\n", size,
                    4 * ctx->nfsteps);
                  break;
               }
#if IMLIB2_DEBUG
             for (i = 0; i < ctx->nfsteps; i++)
                Dx(" %d", SWAP_LE_32(ctx->seq[i]));
#endif
             Dx("\n");
             break;
          }
        size = (size + 1) & ~1;
     }

   ctx->nest -= 1;

   return rc;
}

static int
_load(ImlibImage * im, int load_data)
{
   int                 rc;
   riff_ctx_t          ctx = { };

   rc = _riff_parse(im, &ctx, im->fi->fdata, im->fi->fsize, im->fi->fdata);

   return rc;
}

IMLIB_LOADER(_formats, _load, NULL);
