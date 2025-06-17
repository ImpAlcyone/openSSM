#include <ncurses.h>
#include <stdarg.h>

void win_printf(WINDOW *win, int y, int x, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    mvwprintw(win, y, x, fmt, args);
    va_end(args);
}
