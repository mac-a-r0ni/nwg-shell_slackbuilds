#include "config.h"
#include "Imlib2_Loader.h"

#include <libspectre/spectre.h>

#define DBG_PFX "LDR-ps"

static const char  *const _formats[] = { "ps", "eps" };

static int
_load(ImlibImage * im, int load_data)
{
   int                 rc;
   SpectreDocument    *spdoc;
   SpectrePage        *sppage;
   SpectreStatus       spst;
   int                 frame, fcount;
   int                 w, h;
   SpectreRenderContext *sprc;
   unsigned char      *pdata;
   int                 rowlen;
   unsigned char      *src;
   uint32_t           *dst;
   int                 i, j;
   ImlibImageFrame    *pf;

   rc = LOAD_FAIL;
   spdoc = NULL;
   sppage = NULL;
   sprc = NULL;

   /* Signature check */
   if (memcmp(im->fi->fdata, "%!PS", 4) != 0)
      goto quit;

   spdoc = spectre_document_new();
   if (!spdoc)
      goto quit;

   spectre_document_load(spdoc, im->fi->name);
   spst = spectre_document_status(spdoc);
   if (spst != SPECTRE_STATUS_SUCCESS)
     {
        D("spectre_document_load: %s\n", spectre_status_to_string(spst));
        goto quit;
     }

   rc = LOAD_BADIMAGE;          /* Format accepted */

   frame = im->frame;
   if (frame > 0)
     {
        fcount = spectre_document_get_n_pages(spdoc);
        D("Pages=%d\n", fcount);
        if (frame > 1 && frame > fcount)
           QUIT_WITH_RC(LOAD_BADFRAME);

        pf = __imlib_GetFrame(im);
        if (!pf)
           QUIT_WITH_RC(LOAD_OOM);
        pf->frame_count = fcount;
     }

   sppage = spectre_document_get_page(spdoc, frame - 1);
   spst = spectre_document_status(spdoc);
   if (spst != SPECTRE_STATUS_SUCCESS)
     {
        D("spectre_document_get_page: %s\n", spectre_status_to_string(spst));
        goto quit;
     }

   spectre_page_get_size(sppage, &w, &h);

   D("WxH=%dx%d pages=%d fmt=%s level=%d eps=%d\n", w, h,
     spectre_document_get_n_pages(spdoc),
     spectre_document_get_format(spdoc),
     spectre_document_get_language_level(spdoc),
     spectre_document_is_eps(spdoc));
   im->w = w;
   im->h = h;

   if (!IMAGE_DIMENSIONS_OK(im->w, im->h))
      goto quit;

   if (!load_data)
      QUIT_WITH_RC(LOAD_SUCCESS);

   /* Load data */

   if (!__imlib_AllocateData(im))
      QUIT_WITH_RC(LOAD_OOM);

   sprc = spectre_render_context_new();
   if (!sprc)
      QUIT_WITH_RC(LOAD_OOM);

   rowlen = 0;
   spectre_page_render(sppage, sprc, &pdata, &rowlen);
   spst = spectre_page_status(sppage);
   if (spst != SPECTRE_STATUS_SUCCESS)
     {
        D("spectre_render_context_set_page_size: %s\n",
          spectre_status_to_string(spst));
        goto quit;
     }

   spectre_render_context_set_page_size(sprc, im->w, im->h);
   spst = spectre_page_status(sppage);
   if (spst != SPECTRE_STATUS_SUCCESS)
     {
        D("spectre_render_context_set_page_size: %s\n",
          spectre_status_to_string(spst));
        goto quit;
     }

   src = pdata;
   dst = im->data;

   D("rowlen=%d (%d)\n", rowlen, 4 * im->w);

   for (i = 0; i < im->h; i++)
     {
        src = pdata + i * rowlen;
        for (j = 0; j < im->w; j++, src += 4)
           *dst++ = PIXEL_ARGB(0xff, src[2], src[1], src[0]);

        if (im->lc && __imlib_LoadProgressRows(im, i, 1))
           QUIT_WITH_RC(LOAD_BREAK);
     }

   rc = LOAD_SUCCESS;

 quit:
   if (sprc)
      spectre_render_context_free(sprc);
   if (sppage)
      spectre_page_free(sppage);
   if (spdoc)
      spectre_document_free(spdoc);

   return rc;
}

IMLIB_LOADER(_formats, _load, NULL);
