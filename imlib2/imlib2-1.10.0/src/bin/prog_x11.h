#ifndef PROG_X11_H
#define PROG_X11_H

#include <X11/Xlib.h>

extern Display     *disp;

int                 prog_x11_init(void);
Window              prog_x11_create_window(const char *name, int w, int h);
int                 prog_x11_event(XEvent * ev);

#endif /* PROG_X11_H */
