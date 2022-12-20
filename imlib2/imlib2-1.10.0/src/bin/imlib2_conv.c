/*
 * Convert images between formats, using Imlib2's API.
 * Defaults to jpg's.
 */
#include "config.h"
#ifndef X_DISPLAY_MISSING
#define X_DISPLAY_MISSING
#endif
#include <Imlib2.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define DEBUG 0
#if DEBUG
#define Dprintf(fmt...)  printf(fmt)
#else
#define Dprintf(fmt...)
#endif

#define HELP \
   "Usage:\n" \
   "  imlib2_conv [OPTIONS] [ input-file output-file[.fmt] ]\n" \
   "    <fmt> defaults to jpg if not provided.\n" \
   "\n" \
   "OPTIONS:\n" \
   "  -h            : Show this help\n" \
   "  -i key=value  : Attach tag with integer value for saver\n" \
   "  -j key=string : Attach tag with string value for saver\n"

static void
usage(void)
{
   printf(HELP);
}

static void
data_free_cb(void *im, void *data)
{
   Dprintf("%s: im=%p data=%p\n", __func__, im, data);
   free(data);
}

/*
 * Attach tag = key/value pair to current image
 */
static void
data_attach(int type, char *arg)
{
   char               *p;

   p = strchr(arg, '=');
   if (!p)
      return;                   /* No value - just ignore */

   *p++ = '\0';

   switch (type)
     {
     default:
        break;                  /* Should not be possible - ignore */
     case 'i':                 /* Integer parameter */
        Dprintf("%s: Set '%s' = %d\n", __func__, arg, atoi(p));
        imlib_image_attach_data_value(arg, NULL, atoi(p), NULL);
        break;
     case 'j':                 /* String parameter */
        p = strdup(p);
        Dprintf("%s: Set '%s' = '%s' (%p)\n", __func__, arg, p, p);
        imlib_image_attach_data_value(arg, p, 0, data_free_cb);
        break;
     }
}

int
main(int argc, char **argv)
{
   int                 opt, err;
   const char         *fin, *fout;
   char               *dot;
   Imlib_Image         im;

   while ((opt = getopt(argc, argv, "hi:j:")) != -1)
     {
        switch (opt)
          {
          default:
          case 'h':
             usage();
             exit(0);
          case 'i':
          case 'j':
             break;             /* Ignore this time around */
          }
     }

   if (argc - optind < 2)
     {
        usage();
        exit(1);
     }

   fin = argv[optind];
   fout = argv[optind + 1];

   im = imlib_load_image_with_errno_return(fin, &err);
   if (!im)
     {
        fprintf(stderr, "*** Error %d:'%s' loading image: '%s'\n",
                err, imlib_strerror(err), fin);
        return 1;
     }

   Dprintf("%s: im=%p\n", __func__, im);
   imlib_context_set_image(im);

   /* Re-parse options to attach parameters to be used by savers */
   optind = 1;
   while ((opt = getopt(argc, argv, "hi:j:")) != -1)
     {
        switch (opt)
          {
          default:
             break;
          case 'i':            /* Attach integer parameter */
          case 'j':            /* Attach string parameter */
             data_attach(opt, optarg);
             break;
          }
     }

   /* hopefully the last one will be the one we want.. */
   dot = strrchr(fout, '.');

   /* if there's a format, snarf it and set the format. */
   if (dot && *(dot + 1))
      imlib_image_set_format(dot + 1);
   else
      imlib_image_set_format("jpg");

   imlib_save_image_with_errno_return(fout, &err);
   if (err)
      fprintf(stderr, "*** Error %d:'%s' saving image: '%s'\n",
              err, imlib_strerror(err), fout);

#if DEBUG
   imlib_free_image_and_decache();
#endif

   return err;
}
