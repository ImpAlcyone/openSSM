#ifndef HELPER_UI_H
#define HELPER_UI_H

#include <ncurses.h>


#ifdef __cplusplus
extern "C" {
#endif

void win_printf(WINDOW *win, int y, int x, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif // HELPER_UI_H
