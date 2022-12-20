#include "config.h"
#include "Imlib2_Loader.h"

#include <bzlib.h>

#define OUTBUF_SIZE 16384
#define INBUF_SIZE 1024

static const char  *const _formats[] = { "bz2" };

static int
uncompress_file(const void *fdata, unsigned int fsize, int dest)
{
   int                 ok;
   bz_stream           strm = { 0 };
   int                 ret, bytes;
   char                outbuf[OUTBUF_SIZE];

   ok = 0;

   ret = BZ2_bzDecompressInit(&strm, 0, 0);
   if (ret != BZ_OK)
      return ok;

   strm.next_in = (void *)fdata;
   strm.avail_in = fsize;

   for (;;)
     {
        strm.next_out = outbuf;
        strm.avail_out = sizeof(outbuf);

        ret = BZ2_bzDecompress(&strm);

        if (ret != BZ_OK && ret != BZ_STREAM_END)
           goto quit;

        bytes = sizeof(outbuf) - strm.avail_out;
        if (write(dest, outbuf, bytes) != bytes)
           goto quit;

        if (ret == BZ_STREAM_END)
           break;
     }

   ok = 1;

 quit:
   BZ2_bzDecompressEnd(&strm);

   return ok;
}

static int
_load(ImlibImage * im, int load_data)
{

   return decompress_load(im, load_data, _formats, ARRAY_SIZE(_formats),
                          uncompress_file);
}

IMLIB_LOADER(_formats, _load, NULL);
