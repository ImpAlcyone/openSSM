#include <ncurses.h>

/*=================================*/
/* Functions to handle colour text */
/*=================================*/

void setup_colours(void)
{
    start_color();
    init_pair(1,COLOR_WHITE,COLOR_BLACK);
    init_pair(2,COLOR_BLACK,COLOR_GREEN);
    init_pair(3,COLOR_BLACK,COLOR_YELLOW);
    init_pair(4,COLOR_BLACK,COLOR_RED);
    init_pair(5,COLOR_BLACK,COLOR_CYAN);
}

void white_on_black(void)
{
	attron(COLOR_PAIR(1));
}

void black_on_green(void)
{
	attron(COLOR_PAIR(2));
}

void black_on_yellow(void)
{
	attron(COLOR_PAIR(3));
}

void black_on_red(void)
{
	attron(COLOR_PAIR(4));
}

void black_on_cyan(void)
{
	attron(COLOR_PAIR(5));
}

/*===============================================*/