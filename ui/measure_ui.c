#include <ncurses.h>
#include <stdint.h>

#include "plot.h"
#include "measure.h"
#include "config.h"
#include "config_ui.h"
#include "log.h"

static const char *config_file = "config/signals.conf";
static int x_max, y_max;
static int init_state = 1;
static int signal_count = 0;
static int vis_mode = 0;
static int conn_mode = 0;
static int log_mode = 0;
static SignalConfig_t signals[MAX_LABELCOUNT]; 

static int init_measure_ui(void)
{
    x_max = 0;
    y_max = 0;

    signal_count = load_signal_config(config_file, signals);
    if (signal_count <= 0) {
        fprintf(stderr, "No valid signals loaded from config file.\n");
        return 1;
    }

    build_logfile_header(signals, signal_count);

    return 0;
}



/* Draw basic measurement screen*/
 static void draw_screen(int *romId)
{
    clear();
    white_on_black();

    
    switch (conn_mode)
    {
        case 0: 
            white_on_black(); 
            move(19,36);
            printw("No Connection");
            break;
        case 1: 
            black_on_red();
            if(0 != *romId){
                move(0, 0);
                printw("RomID = %08X", *romId);
            }
            black_on_green();
            move(19,36);
            printw("  Connected  "); 
            break;
        case 2: 
            black_on_red();
            move(19,33);
            printw(" Connection Failed ");
            move(23, 25);
            printw("Press 'C' again to retry connecting!");
            break;
    }
    
        

    switch (vis_mode)
    {
        case 0: white_on_black(); break;
        case 1: black_on_green(); break;
    }
    move(20,35);
    printw(" Visualization ");

    switch (log_mode)
    {
        case 0: white_on_black(); break;
        case 1: black_on_green(); break;
        case 2: black_on_red();   break;
    }
    move(21,38);
    printw(" Logging ");


    white_on_black();
    move(25,1);
    printw("Q:Quit\t\tC:Toggle Connection\tV: Toggle Visualisation\t\tL:Toggle Logging");
    refresh();
}

void run_measure_tab(void) 
{
    if(init_state)
    {
        init_state = init_measure_ui();
    }
   getmaxyx(stdscr, y_max, x_max); // Get the number of rows and columns
}

