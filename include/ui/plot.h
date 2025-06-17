#ifndef PLOT_H
#define PLOT_H

#include <ncurses.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize the plot system (optional: called once)
void plot_init(WINDOW *win, int max_y);

// Draw a single data point in the plot area
void plot_draw_point(WINDOW *win, int x, int y, int max_y, chtype ch);

// Draw a connected line of points
void plot_draw_series(WINDOW *win, const int *data, int len, int max_value, int max_y);
#ifdef __cplusplus
}
#endif

#endif // PLOT_UI
