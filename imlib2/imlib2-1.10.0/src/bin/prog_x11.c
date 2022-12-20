/*
 * Common program functionality
 */
#include "config.h"
#include <Imlib2.h>

#include <stdio.h>
#include <X11/Xlib.h>

#include "prog_x11.h"

Display            *disp = NULL;

static Atom         ATOM_WM_DELETE_WINDOW = None;
static Atom         ATOM_WM_PROTOCOLS = None;

int
prog_x11_init(void)
{
   disp = XOpenDisplay(NULL);
   if (!disp)
     {
        fprintf(stderr, "Cannot open display\n");
        return 1;
     }

   imlib_context_set_display(disp);
   imlib_context_set_visual(DefaultVisual(disp, DefaultScreen(disp)));
   imlib_context_set_colormap(DefaultColormap(disp, DefaultScreen(disp)));

   return 0;
}

Window
prog_x11_create_window(const char *name, int w, int h)
{
   Window              win;
   int                 x, y;

   x = y = 0;

   win = XCreateSimpleWindow(disp, DefaultRootWindow(disp),
                             x, y, w, h, 0, 0, 0);

   XSelectInput(disp, win, KeyPressMask |
                ButtonPressMask | ButtonReleaseMask | ButtonMotionMask |
                PointerMotionMask | ExposureMask);

   XStoreName(disp, win, name);

   ATOM_WM_PROTOCOLS = XInternAtom(disp, "WM_PROTOCOLS", False);
   ATOM_WM_DELETE_WINDOW = XInternAtom(disp, "WM_DELETE_WINDOW", False);
   XSetWMProtocols(disp, win, &ATOM_WM_DELETE_WINDOW, 1);

   return win;
}

int
prog_x11_event(XEvent * ev)
{
   switch (ev->type)
     {
     default:
        break;
     case ClientMessage:
        if (ev->xclient.message_type == ATOM_WM_PROTOCOLS &&
            ev->xclient.data.l[0] == (long)ATOM_WM_DELETE_WINDOW)
           return 1;
        break;
     }

   return 0;
}
