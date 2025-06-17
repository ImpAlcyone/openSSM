// plot_ui.c
#include "plot.h"
#include <ncurses.h>
#include <stdlib.h>
#include <math.h>

// Optional initializer (currently unused but available for future state)
void plot_init(WINDOW *win, int max_y) {
    // Could be used to clear the region or draw axes if needed
    (void)win;
    (void)max_y;
}

// Internal line drawing helper
static void draw_line(WINDOW *win, int x0, int y0, int x1, int y1, chtype ch) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    while (1) {
        if (y0 >= 0 && x0 >= 0)
            mvwaddch(win, y0, x0, ch);
        if (x0 == x1 && y0 == y1)
            break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

// Draw a single point scaled by max_y (e.g. terminal plot height)
void plot_draw_point(WINDOW *win, int x, int y, int max_y, chtype ch) {
    int height, width;
    getmaxyx(win, height, width);
    int py = max_y - y;
    if (x >= 0 && x < width && py >= 0 && py < height) 
    {
        mvwaddch(win, py, x, ch);
    }
}

// Draw a full series with auto-scaling and line interpolation
void plot_draw_series(WINDOW *win, const int *data, int len, int max_value, int max_y) {
    int height, width;
    getmaxyx(win, height, width);
    float y_scale = (float)max_y / max_value;
    int x0, x1, y0, y1;

    for (int i = 1; i < len; ++i) {
        x0 = i - 1;
        x1 = i;

        y0 = max_y - (int)(data[i - 1] * y_scale);
        y1 = max_y - (int)(data[i] * y_scale);

        if (x0 >= 0 && x1 < width && y0 >= 0 && y0 < height && y1 >= 0 && y1 < height) {
            draw_line(win, x0, y0, x1, y1, '*');
        }
    }
}
