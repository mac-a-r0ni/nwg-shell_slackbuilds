#include "config.h"
#include "Imlib2_Loader.h"

#include <zlib.h>

static const char  *const _formats[] = { "gz" };

#define OUTBUF_SIZE 16484

static int
uncompress_file(const void *fdata, unsigned int fsize, int dest)
{
   int                 ok;
   z_stream            strm = { 0 };;
   unsigned char       outbuf[OUTBUF_SIZE];
   int                 ret = 1, bytes;

   ok = 0;

   ret = inflateInit2(&strm, 15 + 32);
   if (ret != Z_OK)
      return ok;

   strm.next_in = (void *)fdata;
   strm.avail_in = fsize;

   for (;;)
     {
        strm.next_out = outbuf;
        strm.avail_out = sizeof(outbuf);

        ret = inflate(&strm, 0);

        if (ret != Z_OK && ret != Z_STREAM_END)
           goto quit;

        bytes = sizeof(outbuf) - strm.avail_out;
        if (write(dest, outbuf, bytes) != bytes)
           goto quit;

        if (ret == Z_STREAM_END)
           break;
     }

   ok = 1;

 quit:
   inflateEnd(&strm);

   return ok;
}

static int
_load(ImlibImage * im, int load_data)
{

   return decompress_load(im, load_data, _formats, ARRAY_SIZE(_formats),
                          uncompress_file);
}

IMLIB_LOADER(_formats, _load, NULL);
