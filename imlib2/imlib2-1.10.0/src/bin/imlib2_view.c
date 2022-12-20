#include "config.h"
#include <Imlib2.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <unistd.h>

#include "prog_x11.h"

#define MIN(a, b) ((a < b) ? a : b)
#define MAX(a, b) ((a > b) ? a : b)

typedef struct {
   int                 x, y;    /* Origin */
   int                 w, h;    /* Size   */
} rect_t;

static int          debug = 0;
static int          verbose = 0;
static Window       win;
static Pixmap       bg_pm = 0;
static int          image_width = 0, image_height = 0;
static int          window_width = 0, window_height = 0;
static Imlib_Image  bg_im = NULL;
static Imlib_Image  bg_im_clean = NULL;
static Imlib_Image  fg_im = NULL;       /* The animated image */
static Imlib_Image  im_curr = NULL;     /* The current image */

static bool         opt_cache = false;
static bool         opt_progr = true;   /* Render through progress callback */
static bool         opt_scale = false;
static double       opt_scale_x = 1.;
static double       opt_scale_y = 1.;
static double       opt_sgrab_x = 1.;
static double       opt_sgrab_y = 1.;
static char         opt_progress_granularity = 10;
static char         opt_progress_print = 0;
static int          opt_progress_delay = 0;

static Imlib_Frame_Info finfo;
static bool         multiframe = false; /* Image has multiple frames     */
static bool         fixedframe = false; /* We have selected single frame */
static bool         animated = false;   /* Image has animation sequence  */
static bool         animate = false;    /* Animation is active           */
static int          animloop = 0;       /* Animation loop count          */

#define Dprintf(fmt...)  if (debug)        printf(fmt)
#define Vprintf(fmt...)  if (verbose)      printf(fmt)
#define V2printf(fmt...) if (verbose >= 2) printf(fmt)

#define MAX_DIM	32767

#define SCALE_X(x) (int)(scale_x * (x) + .5)
#define SCALE_Y(x) (int)(scale_y * (x) + .5)

#define HELP \
   "Usage:\n" \
   "  imlib2_view [OPTIONS] {FILE | XID}...\n" \
   "OPTIONS:\n" \
   "  -c         : Enable image caching (implies -e)\n" \
   "  -d         : Enable debug\n" \
   "  -e         : Do rendering explicitly (not via progress callback)\n" \
   "  -g N       : Set progress granularity to N%% (default 10(%%))\n" \
   "  -l N       : Introduce N ms delay in progress callback (default 0)\n" \
   "  -p         : Print info in progress callback (default no)\n" \
   "  -s Sx[,Sy] : Set render x/y scaling factors to Sx,Sy (default 1.0)\n" \
   "  -S Sx[,Sy] : Set grab x/y scaling factors to Sx,Sy (default 1.0)\n" \
   "  -v         : Increase verbosity\n"

static void
usage(void)
{
   printf(HELP);
}

static void
bg_im_init(int w, int h)
{
   int                 x, y, onoff;

   if (bg_im)
     {
        imlib_context_set_image(bg_im);
        imlib_free_image_and_decache();
     }

   bg_im = imlib_create_image(w, h);

   imlib_context_set_image(bg_im);
   for (y = 0; y < h; y += 8)
     {
        onoff = (y / 8) & 0x1;
        for (x = 0; x < w; x += 8)
          {
             if (onoff)
                imlib_context_set_color(144, 144, 144, 255);
             else
                imlib_context_set_color(100, 100, 100, 255);
             imlib_image_fill_rectangle(x, y, 8, 8);
             onoff++;
             if (onoff == 2)
                onoff = 0;
          }
     }
}

static void
bg_pm_init(Window xwin, int w, int h)
{
   if (bg_pm)
      XFreePixmap(disp, bg_pm);

   bg_pm = XCreatePixmap(disp, xwin, w, h,
                         DefaultDepth(disp, DefaultScreen(disp)));
}

static void
bg_pm_redraw(int zx, int zy, double zoom, int aa)
{
   int                 sx, sy, sw, sh, dx, dy, dw, dh;

   if (zoom == 0.)
     {
        sx = sy = dx = dy = 0;
        sw = image_width;
        sh = image_height;
        dw = window_width;
        dh = window_height;
     }
   else if (zoom > 1.0)
     {
        dx = 0;
        dy = 0;
        dw = window_width;
        dh = window_height;

        sx = zx - (zx / zoom);
        sy = zy - (zy / zoom);
        sw = image_width / zoom;
        sh = image_height / zoom;
     }
   else
     {
        dx = zx - (zx * zoom);
        dy = zy - (zy * zoom);
        dw = window_width * zoom;
        dh = window_height * zoom;

        sx = 0;
        sy = 0;
        sw = image_width;
        sh = image_height;
     }

   imlib_context_set_anti_alias(aa);
   imlib_context_set_dither(aa);
   imlib_context_set_blend(0);
   imlib_context_set_drawable(bg_pm);
   imlib_context_set_image(bg_im);
   imlib_render_image_part_on_drawable_at_size(sx, sy, sw, sh, dx, dy, dw, dh);
   XClearWindow(disp, win);
}

static void
anim_init(int w, int h)
{
   if (fg_im)
     {
        imlib_context_set_image(fg_im);
        imlib_free_image_and_decache();
        fg_im = NULL;
     }

   if (bg_im_clean)
     {
        imlib_context_set_image(bg_im_clean);
        imlib_free_image_and_decache();
        bg_im_clean = NULL;
     }

   if (animated)
     {
        /* Create canvas for rendering of animated image */
        fg_im = imlib_create_image(w, h);
        imlib_context_set_image(fg_im);
        imlib_image_set_has_alpha(1);
        imlib_context_set_color(0, 0, 0, 0);
        imlib_image_fill_rectangle(0, 0, w, h);

        /* Keep a clean copy of background image around for clearing parts */
        imlib_context_set_image(bg_im);
        bg_im_clean = imlib_clone_image();
     }
}

static void
anim_update(Imlib_Image im, const rect_t * r_up, const rect_t * r_out,
            rect_t * r_dam, int flags)
{
   static const rect_t r_zero = { };
   static rect_t       r_prev = { };
   static Imlib_Image  im_prev = NULL;

   if (!im)
     {
        /* Cleanup */
        if (im_prev)
          {
             imlib_context_set_image(im_prev);
             imlib_free_image_and_decache();
             im_prev = NULL;
          }
        return;
     }

   imlib_context_set_image(fg_im);

   if (r_prev.w > 0)
     {
        /* "dispose" of (clear) previous area before rendering new */
        if (im_prev)
          {
             Dprintf(" Prev  %d,%d %dx%d\n",
                     r_prev.x, r_prev.y, r_prev.w, r_prev.h);
             imlib_context_set_blend(0);
             imlib_blend_image_onto_image(im_prev, 1,
                                          0, 0, r_prev.w, r_prev.h,
                                          r_prev.x, r_prev.y,
                                          r_prev.w, r_prev.h);
             imlib_context_set_image(im_prev);
             imlib_free_image_and_decache();
             imlib_context_set_image(fg_im);
             im_prev = NULL;
          }
        else
          {
             Dprintf(" Clear %d,%d %dx%d\n",
                     r_prev.x, r_prev.y, r_prev.w, r_prev.h);
             imlib_context_set_color(0, 0, 0, 0);
             imlib_image_fill_rectangle(r_prev.x, r_prev.y, r_prev.w, r_prev.h);
          }

        /* Damaged area is (cleared + new frame) */
        r_dam->x = MIN(r_prev.x, r_out->x);
        r_dam->y = MIN(r_prev.y, r_out->y);
        r_dam->w = MAX(r_prev.x + r_prev.w, r_out->x + r_out->w) - r_dam->x;
        r_dam->h = MAX(r_prev.y + r_prev.h, r_out->y + r_out->h) - r_dam->y;
     }
   else
     {
        *r_dam = *r_out;
     }

   if (flags & IMLIB_FRAME_DISPOSE_PREV)
     {
        Dprintf(" Save  %d,%d %dx%d\n", r_out->x, r_out->y, r_out->w, r_out->h);
        im_prev =
           imlib_create_cropped_image(r_out->x, r_out->y, r_out->w, r_out->h);
     }

   if (flags & (IMLIB_FRAME_DISPOSE_CLEAR | IMLIB_FRAME_DISPOSE_PREV))
      r_prev = *r_out;          /* Clear/revert next time around */
   else
      r_prev = r_zero;          /* No clearing before next frame */

   /* Render new frame on canvas */
   Dprintf(" Render %d,%d %dx%d -> %d,%d\n", r_up->x, r_up->y, r_up->w, r_up->h,
           r_out->x, r_out->y);
   if (flags & IMLIB_FRAME_BLEND)
      imlib_context_set_blend(1);
   else
      imlib_context_set_blend(0);
   imlib_blend_image_onto_image(im, 1, r_up->x, r_up->y, r_up->w, r_up->h,
                                r_out->x, r_out->y, r_up->w, r_up->h);
}

static void
free_image(Imlib_Image im)
{
   Dprintf(" Cache usage: %d/%d\n", imlib_get_cache_used(),
           imlib_get_cache_size());

   if (!im)
      return;

   imlib_context_set_image(im);
   if (opt_cache)
      imlib_free_image();
   else
      imlib_free_image_and_decache();
}

static int
progress(Imlib_Image im, char percent, int update_x, int update_y,
         int update_w, int update_h)
{
   static double       scale_x = 0., scale_y = 0.;
   int                 up_wx, up_wy, up_ww, up_wh;
   rect_t              r_up, r_out;

   if (opt_progress_print)
      printf("%s: %3d%% %4d,%4d %4dx%4d\n",
             __func__, percent, update_x, update_y, update_w, update_h);

   imlib_context_set_image(im);
   imlib_image_get_frame_info(&finfo);
   multiframe = finfo.frame_count > 1;
   animated = finfo.frame_flags & IMLIB_IMAGE_ANIMATED;

   if (update_w <= 0 || update_h <= 0)
     {
        /* Explicit rendering (not doing callbacks) */
        r_up.x = 0;
        r_up.y = 0;
        r_up.w = finfo.frame_w;
        r_up.h = finfo.frame_h;
     }
   else
     {
        r_up.x = update_x;
        r_up.y = update_y;
        r_up.w = update_w;
        r_up.h = update_h;
     }

   r_out = r_up;
   if (!fixedframe)
     {
        r_out.x += finfo.frame_x;
        r_out.y += finfo.frame_y;
     }

   Dprintf("U=%d,%d %dx%d F=%d,%d %dx%d O=%d,%d %dx%d\n",
           r_up.x, r_up.y, r_up.w, r_up.h,
           finfo.frame_x, finfo.frame_y, finfo.frame_w, finfo.frame_h,
           r_out.x, r_out.y, r_out.w, r_out.h);

   /* first time it's called */
   if (image_width == 0)
     {
        scale_x = opt_scale_x;
        scale_y = opt_scale_y;

        window_width = DisplayWidth(disp, DefaultScreen(disp));
        window_height = DisplayHeight(disp, DefaultScreen(disp));
        window_width -= 32;     /* Allow for decorations */
        window_height -= 32;
        Dprintf(" Screen WxH=%dx%d\n", window_width, window_height);

        image_width = fixedframe ? finfo.frame_w : finfo.canvas_w;
        image_height = fixedframe ? finfo.frame_h : finfo.canvas_h;

        if (!opt_scale &&
            (image_width > window_width || image_height > window_height))
          {
             scale_x = scale_y = 1.;
             while (window_width < SCALE_X(image_width) ||
                    window_height < SCALE_Y(image_height))
               {
                  scale_x *= .5;
                  scale_y = scale_x;
                  Dprintf(" scale WxH=%.3fx%.3f\n", scale_x, scale_y);
               }
          }

        window_width = SCALE_X(image_width);
        window_height = SCALE_Y(image_height);
        if (window_width > MAX_DIM)
          {
             window_width = MAX_DIM;
             scale_x = (double)MAX_DIM / image_width;
          }
        if (window_height > MAX_DIM)
          {
             window_height = MAX_DIM;
             scale_y = (double)MAX_DIM / image_height;
          }
        Dprintf(" Window WxH=%dx%d\n", window_width, window_height);

        V2printf(" Image  WxH=%dx%d fmt='%s'\n",
                 image_width, image_height, imlib_image_format());

        /* Initialize checkered background image */
        bg_im_init(image_width, image_height);
        /* Initialize window background pixmap */
        bg_pm_init(win, window_width, window_height);

        /* Paint entire background pixmap with initial pattern */
        imlib_context_set_drawable(bg_pm);
        imlib_context_set_image(bg_im);
        imlib_render_image_part_on_drawable_at_size(0, 0,
                                                    image_width, image_height,
                                                    0, 0,
                                                    window_width,
                                                    window_height);

        /* Set window background, resize, and show */
        XSetWindowBackgroundPixmap(disp, win, bg_pm);
        XResizeWindow(disp, win, window_width, window_height);
        XClearWindow(disp, win);
        XMapWindow(disp, win);
        XSync(disp, False);

        /* Initialize images used for animation */
        anim_init(image_width, image_height);
     }

   imlib_context_set_anti_alias(0);
   imlib_context_set_dither(0);

   if (animated)
     {
        rect_t              r_dam = { };

        /* Update animated "target" canvas image (fg_im) */
        anim_update(im, &r_up, &r_out, &r_dam, finfo.frame_flags);
        r_out = r_dam;          /* r_up is now the "damaged" (to be re-rendered) area */
        im = fg_im;

        /* Re-render checkered background where animated image has changes */
        imlib_context_set_image(bg_im);
        imlib_context_set_blend(0);
        imlib_blend_image_onto_image(bg_im_clean, 0,
                                     r_out.x, r_out.y, r_out.w, r_out.h,
                                     r_out.x, r_out.y, r_out.w, r_out.h);
     }

   /* Render image on background image */
   Dprintf(" Update %d,%d %dx%d\n", r_out.x, r_out.y, r_out.w, r_out.h);
   imlib_context_set_image(bg_im);
   imlib_context_set_blend(1);
   imlib_blend_image_onto_image(im, 1,
                                r_out.x, r_out.y, r_out.w, r_out.h,
                                r_out.x, r_out.y, r_out.w, r_out.h);

   /* Render image (part) (or updated canvas) on window background pixmap */
   up_wx = SCALE_X(r_out.x);
   up_wy = SCALE_Y(r_out.y);
   up_ww = SCALE_X(r_out.w);
   up_wh = SCALE_Y(r_out.h);
   Dprintf(" Paint  %d,%d %dx%d\n", up_wx, up_wy, up_ww, up_wh);
   imlib_context_set_blend(0);
   imlib_context_set_drawable(bg_pm);
   imlib_render_image_part_on_drawable_at_size(r_out.x, r_out.y, r_out.w,
                                               r_out.h, up_wx, up_wy, up_ww,
                                               up_wh);

   /* Update window */
   XClearArea(disp, win, up_wx, up_wy, up_ww, up_wh, False);
   XFlush(disp);

   if (opt_progress_delay > 0)
      usleep(opt_progress_delay);

   return 1;
}

static              Imlib_Image
load_image_frame(const char *name, int frame, int inc)
{
   Imlib_Image         im;

   free_image(im_curr);

   if (inc && finfo.frame_count > 0)
      frame = (finfo.frame_count + frame + inc - 1) % finfo.frame_count + 1;

   im = imlib_load_image_frame(name, frame);
   im_curr = im;
   if (!im)
      return im;

   if (!opt_progr)
     {
        /* No progress callback - render explicitly */
        progress(im, 100, 0, 0, 0, 0);
     }

   /* finfo should now be populated by imlib_image_get_frame_info()
    * called from progress() callback */

   if (multiframe)
     {
        Dprintf(" showing frame %d/%d (x,y=%d,%d, wxh=%dx%d T=%d)\n",
                finfo.frame_num, finfo.frame_count,
                finfo.frame_x, finfo.frame_y, finfo.frame_w, finfo.frame_h,
                finfo.frame_delay);
     }
   else
     {
        Dprintf(" showing image wxh=%dx%d\n", finfo.frame_w, finfo.frame_h);
     }

   return im;
}

static int
load_image(int no, const char *name)
{
   int                 err;
   Imlib_Image         im;
   char               *ptr;
   Drawable            draw;

   Vprintf("Show  %d: '%s'\n", no, name);

   anim_update(NULL, NULL, NULL, NULL, 0);      /* Clean up previous animation */

   image_width = 0;             /* Force redraw in progress() */

   draw = strtoul(name, &ptr, 0);
   if (*ptr != '\0')
      draw = 0;

   if (draw)
     {
        Window              rr;
        int                 x, y;
        unsigned int        w, h, bw;
        unsigned int        wo, ho;
        unsigned int        depth;
        int                 get_alpha = 1;

        XGetGeometry(disp, draw, &rr, &x, &y, &w, &h, &bw, &depth);

        imlib_context_set_drawable(draw);

        Vprintf("Drawable: %#lx: x,y: %d,%d  wxh=%ux%u  bw=%u  depth=%u\n",
                draw, x, y, w, h, bw, depth);

        wo = w * opt_sgrab_x;
        ho = h * opt_sgrab_y;
        im = imlib_create_scaled_image_from_drawable(None, 0, 0, w, h, wo, ho,
                                                     1, (get_alpha) ? 1 : 0);
        if (!im)
           return -1;

        progress(im, 100, 0, 0, wo, ho);

        imlib_context_set_image(im);
        imlib_free_image();     /* Grabbed image is never cached */

        err = 0;
     }
   else
     {
        char                nbuf[4096];
        const char         *s;
        int                 frame;

        frame = -1;
        sscanf(name, "%[^%]%%%n", nbuf, &frame);
        if (frame > 0)
          {
             s = name + frame;
             frame = atoi(s);
             animate = false;
             fixedframe = true;
          }
        else
          {
             frame = 1;
             animate = true;
             fixedframe = false;
          }
        animloop = 0;

        memset(&finfo, 0, sizeof(finfo));
        im = load_image_frame(nbuf, frame, 0);

        animate = animate && animated;

        err = !im;
     }

   return err;
}

int
main(int argc, char **argv)
{
   int                 opt, err;
   int                 no, inc;

   verbose = 0;

   while ((opt = getopt(argc, argv, "cdeg:l:ps:S:v")) != -1)
     {
        switch (opt)
          {
          case 'c':
             opt_cache = true;
             opt_progr = false; /* Cached images won't give progress callbacks */
             break;
          case 'd':
             debug += 1;
             break;
          case 'e':
             opt_progr = false;
             break;
          case 'g':
             opt_progress_granularity = atoi(optarg);
             break;
          case 'l':
             opt_progress_delay = 1000 * atoi(optarg);
             break;
          case 'p':
             opt_progress_print = 1;
             break;
          case 's':            /* Scale (window size wrt. image size) */
             opt_scale = true;
             opt_scale_y = 0.f;
             sscanf(optarg, "%lf,%lf", &opt_scale_x, &opt_scale_y);
             if (opt_scale_y == 0.f)
                opt_scale_y = opt_scale_x;
             break;
          case 'S':            /* Scale on grab */
             opt_sgrab_y = 0.f;
             sscanf(optarg, "%lf,%lf", &opt_sgrab_x, &opt_sgrab_y);
             if (opt_sgrab_y == 0.f)
                opt_sgrab_y = opt_sgrab_x;
             break;
          case 'v':
             verbose += 1;
             break;
          default:
             usage();
             return 1;
          }
     }

   argc -= optind;
   argv += optind;

   if (argc <= 0)
     {
        usage();
        return 1;
     }

   prog_x11_init();

   win = prog_x11_create_window("imlib2_view", 10, 10);

   if (opt_progr)
     {
        imlib_context_set_progress_function(progress);
        imlib_context_set_progress_granularity(opt_progress_granularity);
     }

   /* Raise cache size if we do caching (default 4M) */
   if (opt_cache)
      imlib_set_cache_size(32 * 1024 * 1024);

   for (no = 0; no < argc; no++)
     {
        err = load_image(no, argv[no]);
        if (!err)
           break;
     }
   if (no >= argc)
     {
        fprintf(stderr, "No loadable image\n");
        exit(0);
     }

   for (;;)
     {
        int                 x, y, b, count, fdsize, xfd, timeout;
        XEvent              ev;
        static int          zoom_mode = 0, zx, zy;
        static double       zoom = 1.0;
        struct timeval      tval;
        fd_set              fdset;
        KeySym              key;
        int                 no2;

        if (animate)
          {
             if (finfo.frame_num == 1)
                animloop++;
             if (finfo.loop_count == animloop &&
                 finfo.frame_num == finfo.frame_count)
                animate = false;
          }

        if (animate)
          {
             usleep(1e3 * finfo.frame_delay);
             load_image_frame(argv[no], finfo.frame_num, 1);
             if (!XPending(disp))
                continue;
          }

        timeout = 0;

        XFlush(disp);
        do
          {
             XNextEvent(disp, &ev);
             switch (ev.type)
               {
               default:
                  if (prog_x11_event(&ev))
                     goto quit;
                  break;
               case KeyPress:
                  while (XCheckTypedWindowEvent(disp, win, KeyPress, &ev))
                     ;
                  key = XLookupKeysym(&ev.xkey, 0);
                  switch (key)
                    {
                    default:
                       break;
                    case XK_q:
                    case XK_Escape:
                       goto quit;
                    case XK_r:
                       goto show_cur;
                    case XK_Right:
                       goto show_next;
                    case XK_Left:
                       goto show_prev;
                    case XK_p:
                       animate = animated && !animate;
                       break;
                    case XK_Up:
                       goto show_next_frame;
                    case XK_Down:
                       goto show_prev_frame;
                    }
                  break;
               case ButtonPress:
                  b = ev.xbutton.button;
                  x = ev.xbutton.x;
                  y = ev.xbutton.y;
                  if (b == 3)
                    {
                       zoom_mode = 1;
                       zx = x;
                       zy = y;
                       bg_pm_redraw(zx, zy, 0., 0);
                    }
                  break;
               case ButtonRelease:
                  b = ev.xbutton.button;
                  if (b == 3)
                     zoom_mode = 0;
                  if (b == 1)
                     goto show_next;
                  if (b == 2)
                     goto show_prev;
                  break;
               case MotionNotify:
                  while (XCheckTypedWindowEvent(disp, win, MotionNotify, &ev))
                     ;
                  x = ev.xmotion.x;
                  if (zoom_mode)
                    {
                       zoom = ((double)x - (double)zx) / 32.0;
                       if (zoom < 0)
                          zoom = 1.0 + ((zoom * 32.0) / ((double)(zx + 1)));
                       else
                          zoom += 1.0;
                       if (zoom <= 0.0001)
                          zoom = 0.0001;

                       bg_pm_redraw(zx, zy, zoom, 0);
                       timeout = 200000;        /* .2 s */
                    }
                  break;

                show_cur:
                  inc = 0;
                  goto show_next_prev;
                show_next:
                  inc = 1;
                  goto show_next_prev;
                show_prev:
                  inc = -1;
                  goto show_next_prev;
                show_next_prev:
                  no2 = no + inc;
                  if (no2 >= argc)
                     break;
                  if (no2 < 0)
                     break;
                  for (;;)
                    {
                       err = load_image(no2, argv[no2]);
                       if (err == 0)
                         {
                            zoom = 1.0;
                            zoom_mode = 0;
                            no = no2;
                            break;
                         }
                       Vprintf("*** Error loading image: %s\n", argv[no2]);
                       no2 += inc;
                       if (no2 >= argc)
                         {
                            no2 = argc - 2;
                            inc = -1;
                         }
                       else if (no2 < 0)
                         {
                            no2 = 1;
                            inc = 1;
                         }
                    }
                  break;
                show_next_frame:
                  inc = 1;
                  goto show_next_prev_frame;
                show_prev_frame:
                  inc = -1;
                  goto show_next_prev_frame;
                show_next_prev_frame:
                  if (!multiframe)
                     break;
                  if (!animated)
                     image_width = 0;   /* Reset window size */
                  load_image_frame(argv[no], finfo.frame_num, inc);
                  break;
               }
          }
        while (XPending(disp));

        if (multiframe)
           continue;

        xfd = ConnectionNumber(disp);
        fdsize = xfd + 1;
        FD_ZERO(&fdset);
        FD_SET(xfd, &fdset);

        if (timeout > 0)
          {
             tval.tv_sec = timeout / 1000000;
             tval.tv_usec = timeout - tval.tv_sec * 1000000;
             count = select(fdsize, &fdset, NULL, NULL, &tval);
          }
        else
          {
             count = select(fdsize, &fdset, NULL, NULL, NULL);
          }

        if (count < 0)
          {
             if ((errno == ENOMEM) || (errno == EINVAL) || (errno == EBADF))
                exit(1);
          }
        else if (count == 0)
          {
             /* "Final" (delayed) re-rendering with AA */
             bg_pm_redraw(zx, zy, zoom, 1);
          }
     }

 quit:
   return 0;
}
