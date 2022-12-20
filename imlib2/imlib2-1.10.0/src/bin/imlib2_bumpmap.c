#include "config.h"
#include <Imlib2.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>

#include "prog_x11.h"

Window              win;

int
main(int argc, char **argv)
{
   int                 w, h, x, y;
   Imlib_Image         im_bg;
   XEvent              ev;
   KeySym              keysym;

   /**
    * Initialization according to options
    */
   printf("Initialising\n");

   prog_x11_init();
   win = prog_x11_create_window("imlib2_bumpmap", 100, 100);

   /**
    * Start rendering
    */
   printf("Rendering\n");
   imlib_context_set_drawable(win);
   imlib_context_set_dither(1);
   imlib_context_set_blend(0);
   imlib_context_set_color_modifier(NULL);

   im_bg = imlib_load_image(PACKAGE_DATA_DIR "/data/images/imlib2.png");
   imlib_context_set_image(im_bg);
   w = imlib_image_get_width();
   h = imlib_image_get_height();

   XResizeWindow(disp, win, w, h);
   XMapWindow(disp, win);
   XSync(disp, False);

   x = -9999;
   y = -9999;
   while (1)
     {
        Imlib_Image        *temp, *temp2;

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
                  keysym = XLookupKeysym(&ev.xkey, 0);
                  if (keysym == XK_q || keysym == XK_Escape)
                     goto quit;
                  break;
               case ButtonRelease:
                  goto quit;
               case MotionNotify:
                  x = ev.xmotion.x;
                  y = ev.xmotion.y;
                  break;
               }
          }
        while (XPending(disp));

        imlib_context_set_blend(0);
        imlib_context_set_image(im_bg);
        temp = imlib_clone_image();
        imlib_context_set_image(temp);

        /*    imlib_blend_image_onto_image(im_bg, 0,
         * 0, 0, w, h,
         * 0, 0, w, h);
         * first = 0; */

        imlib_apply_filter
           ("bump_map_point(x=[],y=[],map=" PACKAGE_DATA_DIR
            "/data/images/imlib2.png);", &x, &y);

        temp2 = im_bg;
        im_bg = temp;
        imlib_context_set_image(im_bg);
        imlib_render_image_on_drawable(0, 0);
        im_bg = temp2;
        imlib_context_set_image(temp);
        imlib_free_image();
     }

 quit:
   return 0;
}
