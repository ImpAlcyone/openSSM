// main_ui.c
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <ncurses.h>

#include "dump_ui.h"
#include "error_ui.h"
#include "measure_ui.h"

static int active_tab = 0;
static int active_tab_old = 0;
static int rows, cols;
static const int tab_count = 3;

static void draw_tab_bar(void) 
{
    const char *labels[] = {"ROM-Dump", "Errors", "Measure"};
    int x = 2;
    for (int i = 0; i < tab_count; ++i) {
        if (i == active_tab)
            attron(A_REVERSE);

        mvprintw(0, x, " %s ", labels[i]);

        if (i == active_tab)
            attroff(A_REVERSE);

        x += strlen(labels[i]) + 3;
    }
}

static int tab_cleanup(void)
{
    switch (active_tab_old)
    {
        case 0:
            /* code */
            break;
        case 1:
            /* code */
            break;
        case 2:
            /* code */
            break;
        default:
            break;
    }
}

int run_main_ui(void)
{
    int keypress = 0;
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    while (1) 
    {
        clear();
        draw_tab_bar();

        if(active_tab_old != active_tab)
        {
            tab_cleanup();
            active_tab_old = active_tab;
        }

        switch (active_tab) 
        {
            case 0: draw_dump_tab(); break;
            case 1: draw_error_tab(); break;
            case 2: run_measure_tab(); break;
        }

        refresh();

        keypress = getch();
        if(keypress == KEY_LEFT) 
        {
            active_tab = (active_tab + tab_count - 1) % tab_count;
        }
        else if(keypress == KEY_RIGHT) 
        {
            active_tab = (active_tab + 1) % tab_count;
        }
        else if(keypress == 'q') 
        {
            break;
        }
    }

    endwin();
}
