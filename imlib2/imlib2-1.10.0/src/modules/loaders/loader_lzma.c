#include "config.h"
#include "Imlib2_Loader.h"

#include <lzma.h>

static const char  *const _formats[] = { "xz", "lzma" };

#define OUTBUF_SIZE 16484

static int
uncompress_file(const void *fdata, unsigned int fsize, int dest)
{
   int                 ok;
   lzma_stream         strm = LZMA_STREAM_INIT;
   lzma_ret            ret;
   uint8_t             outbuf[OUTBUF_SIZE];
   ssize_t             bytes;

   ok = 0;

   ret = lzma_auto_decoder(&strm, UINT64_MAX, 0);
   if (ret != LZMA_OK)
      return ok;

   strm.next_in = fdata;
   strm.avail_in = fsize;

   for (;;)
     {
        strm.next_out = outbuf;
        strm.avail_out = sizeof(outbuf);

        ret = lzma_code(&strm, 0);

        if (ret != LZMA_OK && ret != LZMA_STREAM_END)
           goto quit;

        bytes = sizeof(outbuf) - strm.avail_out;
        if (write(dest, outbuf, bytes) != bytes)
           goto quit;

        if (ret == LZMA_STREAM_END)
           break;
     }

   ok = 1;

 quit:
   lzma_end(&strm);

   return ok;
}

static int
_load(ImlibImage * im, int load_data)
{

   return decompress_load(im, load_data, _formats, ARRAY_SIZE(_formats),
                          uncompress_file);
}

IMLIB_LOADER(_formats, _load, NULL);
