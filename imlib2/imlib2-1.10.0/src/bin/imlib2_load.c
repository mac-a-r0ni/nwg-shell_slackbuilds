#include "config.h"
#ifndef X_DISPLAY_MISSING
#define X_DISPLAY_MISSING
#endif
#include <Imlib2.h>

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#if USE_MONOTONIC_CLOCK
#include <time.h>
#else
#include <sys/time.h>
#endif
#include <sys/mman.h>
#include <sys/stat.h>

#define PROG_NAME "imlib2_load"

#define LOAD_DEFER	0
#define LOAD_NODATA	1
#define LOAD_IMMED	2
#define LOAD_FROM_FD	3
#define LOAD_FROM_MEM	4

static char         progress_called;
static FILE        *fout;

#define Vprintf(fmt...)  if (verbose)      fprintf(fout, fmt)
#define V2printf(fmt...) if (verbose >= 2) fprintf(fout, fmt)

#define HELP \
   "Usage:\n" \
   "  imlib2_load [OPTIONS] FILE...\n" \
   "OPTIONS:\n" \
   "  -c  : Enable image caching\n" \
   "  -e  : Break on error\n" \
   "  -f  : Load with imlib_load_image_fd()\n" \
   "  -i  : Load image immediately (don't defer data loading)\n" \
   "  -j  : Load image header only\n" \
   "  -m  : Load with imlib_load_image_mem()\n" \
   "  -n N: Repeat load N times\n" \
   "  -p  : Check that progress is called\n" \
   "  -v  : Increase verbosity\n" \
   "  -x  : Print to stderr\n"

static void
usage(void)
{
   printf(HELP);
}

static unsigned int
time_us(void)
{
#if USE_MONOTONIC_CLOCK
   struct timespec     ts;

   clock_gettime(CLOCK_MONOTONIC, &ts);

   return (unsigned int)(ts.tv_sec * 1000000 + ts.tv_nsec / 1000);
#else
   struct timeval      timev;

   gettimeofday(&timev, NULL);

   return (unsigned int)(timev.tv_sec * 1000000 + timev.tv_usec);
#endif
}

static Imlib_Image *
image_load_fd(const char *file, int *perr)
{
   Imlib_Image        *im;
   int                 fd;
   const char         *ext;

   ext = strchr(file, '.');
   if (ext)
      ext += 1;
   else
      ext = file;

   fd = open(file, O_RDONLY);
   if (fd < 0)
     {
        *perr = errno;
        return NULL;
     }

   im = imlib_load_image_fd(fd, ext);

   return im;
}

static Imlib_Image *
image_load_mem(const char *file, int *perr)
{
   Imlib_Image        *im;
   int                 fd, err;
   const char         *ext;
   struct stat         st;
   void               *fdata;

   ext = strchr(file, '.');
   if (ext)
      ext += 1;
   else
      ext = file;

   err = stat(file, &st);
   if (err)
      goto bail;

   im = NULL;
   fd = -1;
   fdata = MAP_FAILED;

   fd = open(file, O_RDONLY);
   if (fd < 0)
      goto bail;

   fdata = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
   close(fd);
   if (fdata == MAP_FAILED)
      goto bail;

   im = imlib_load_image_mem(ext, fdata, st.st_size);

 quit:
   if (fdata != MAP_FAILED)
      munmap(fdata, st.st_size);
   return im;
 bail:
   *perr = errno;
   goto quit;
}

static int
progress(Imlib_Image im, char percent, int update_x, int update_y,
         int update_w, int update_h)
{
   progress_called = 1;
   return 1;                    /* Continue */
}

int
main(int argc, char **argv)
{
   int                 opt;
   Imlib_Image         im;
   int                 err;
   unsigned int        t0;
   double              dt;
   char                nbuf[4096];
   int                 frame;
   int                 verbose;
   bool                check_progress;
   int                 break_on_error;
   bool                show_time;
   int                 load_cnt, cnt;
   int                 load_mode;
   bool                opt_cache;

   fout = stdout;
   verbose = 0;
   check_progress = false;
   break_on_error = 0;
   show_time = false;
   load_cnt = 1;
   load_mode = LOAD_DEFER;
   opt_cache = false;

   while ((opt = getopt(argc, argv, "cefijmn:pvx")) != -1)
     {
        switch (opt)
          {
          case 'c':
             opt_cache = true;
             break;
          case 'e':
             break_on_error += 1;
             break;
          case 'f':
             load_mode = LOAD_FROM_FD;
             break;
          case 'i':
             load_mode = LOAD_IMMED;
             break;
          case 'j':
             load_mode = LOAD_NODATA;
             break;
          case 'm':
             load_mode = LOAD_FROM_MEM;
             break;
          case 'n':
             load_cnt = atoi(optarg);
             show_time = true;
             verbose = 1;
             break;
          case 'p':
             check_progress = true;
             load_mode = LOAD_IMMED;
             verbose = 1;
             break;
          case 'v':
             verbose += 1;
             break;
          case 'x':
             fout = stderr;
             break;
          }
     }

   argc -= optind;
   argv += optind;

   if (argc <= 0)
     {
        usage();
        return 1;
     }

   if (check_progress)
     {
        imlib_context_set_progress_function(progress);
        imlib_context_set_progress_granularity(10);
     }

   if (load_cnt < 0)
      load_cnt = 1;
   t0 = 0;

   for (; argc > 0; argc--, argv++)
     {
        progress_called = 0;

        Vprintf("Loading image: '%s'\n", argv[0]);

        if (show_time)
           t0 = time_us();

        for (cnt = 0; cnt < load_cnt; cnt++)
          {
             err = 0;

             switch (load_mode)
               {
               case LOAD_IMMED:
                  im = imlib_load_image_immediately(argv[0]);
                  break;
               case LOAD_FROM_FD:
                  im = image_load_fd(argv[0], &err);
                  break;
               case LOAD_FROM_MEM:
                  im = image_load_mem(argv[0], &err);
                  break;
               case LOAD_DEFER:
               case LOAD_NODATA:
               default:
                  frame = -1;
                  sscanf(argv[0], "%[^%]%%%d", nbuf, &frame);

                  if (frame >= 0)
                     im = imlib_load_image_frame(nbuf, frame);
                  else
                     im = imlib_load_image(argv[0]);
                  break;
               }

             if (!im)
               {
                  if (err == 0)
                     err = imlib_get_error();
                  fprintf(fout, "*** Error %d:'%s' loading image: '%s'\n",
                          err, imlib_strerror(err), argv[0]);

                  if (break_on_error & 2)
                     goto quit;
                  goto next;
               }

             imlib_context_set_image(im);
             V2printf("- Image: fmt=%s WxH=%dx%d\n", imlib_image_format(),
                      imlib_image_get_width(), imlib_image_get_height());

             if (load_mode == LOAD_DEFER)
                imlib_image_get_data_for_reading_only();

             if (opt_cache)
                imlib_free_image();
             else
                imlib_free_image_and_decache();
          }

        if (show_time)
          {
             dt = 1e-3 * (time_us() - t0);
             printf("Elapsed time: %.3f ms (%.3f ms per load)\n",
                    dt, dt / load_cnt);
          }

        if (check_progress && !progress_called)
          {
             fprintf(fout, "*** No progress during image load\n");
             if (break_on_error & 1)
                goto quit;
          }
      next:
        ;
     }
 quit:

   return 0;
}
