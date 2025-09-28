#include <ncurses.h>
#include <stdint.h>
#include <string.h>

#include "ssm.h"
#include "plot.h"
#include "measure.h"
#include "config.h"
#include "config_ui.h"
#include "log.h"

static int x_max, y_max;
static int init_state = 1;
static int signal_count = 0;
static int romId = 0x0;
static int vis_mode = 0;
static int conn_mode = 0;
static int log_mode = 0;
static int vis_req = 0;
static int conn_req = 0;
static int log_req = 0;
static int redraw = 0;
static SignalConfig_t signals[MAX_LABELCOUNT]; 

static int init_measure_ui(char *config_file)
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
static void draw_screen(void)
{
    getmaxyx(stdscr, y_max, x_max); // Get the number of rows and columns
    int y_mid = y_max >> 1;
    int x_mid = x_max >> 1;
    char *conn_messages[] = {
        "No Connection",
        "Connected",
        "Connecting Failed"};
    char *conn_error [] = {
        "Press 'C' again to retry connecting!"};
    char *vis_messages[] = {
        "Visualization off",
        "Visualization on"};
    char *log_messages[] = {
        "Logging off",
        "Logging on",
        "Logging Failed"};
    clear();
    white_on_black();
    
    switch (conn_mode)
    {
        case 0: white_on_black(); break;
        case 1: black_on_red();
            if(0 != romId){
                move(1, 0);
                printw("RomID = %06X", romId);
            }
            black_on_green();
            break;
        case 2: black_on_red();
            mvprintw(y_max - 2,
            x_mid - ((int)strlen(conn_error[0]) >> 1),
            conn_error[0]); 
            break;
    }
    mvprintw(y_max - 6,
            x_mid - ((int)strlen(conn_messages[conn_mode]) >> 1),
            conn_messages[conn_mode]); 
    
    switch (vis_mode)
    {
        case 0: white_on_black(); break;
        case 1: black_on_green(); break;
    }
    mvprintw(y_max - 5,
            x_mid - ((int)strlen(vis_messages[vis_mode]) >> 1),
            vis_messages[vis_mode]);

    switch (log_mode)
    {
        case 0: white_on_black(); break;
        case 1: black_on_green(); break;
        case 2: black_on_red();   break;
    }
     mvprintw(y_max - 4,
        x_mid - ((int)strlen(log_messages[log_mode]) >> 1),
        log_messages[log_mode]);

    white_on_black();
    
    move(25,1);
    printw("Q:Quit\t\tC:Toggle Connection\tV: Toggle Visualisation\t\tL:Toggle Logging");
    refresh();
    redraw = 0;
}

static void connect_ecu(char *comport)
{
    if(conn_req && conn_mode != 1){
        if ((ssm_open(comport)) != 0)
        {
            conn_mode = 2;
            conn_req = 0;
        }
        else
        {
            conn_mode = 1;
            if(ssm_romid_ecu(&romId) != 0)
            {
                romId = 0x0;
                conn_mode = 2;
                conn_req = 0;
            }
        }
    }
    if(!conn_req && 1 == conn_mode)
    {
        ssm_close();
        conn_mode = 0;
        log_mode = 0;
    }
}

static void log_ecu(char *log_file)
{
    switch (log_req)
    {
        case 0:
            if(get_log_file != NULL)
            {
                log_mode = close_logfile();
            }            
            break;
        case 1:
            if (1 == conn_mode && (1 == log_req) && (get_log_file == NULL))
            {
                if(log_file[0] != '\0' && get_logfileName_state())
                {
                    set_logfile_name(log_file);
                }
                log_mode = open_logfile(romId);
            }
            break;        
        default:
            break;
    }
   
}

void run_measure_tab(ui_config_t *p_ui_config) 
{
    if(init_state)
    {
        init_state = init_measure_ui(p_ui_config->config_file);
    }
    
    if(redraw){draw_screen();};
}

void user_input_measure(int *keypress, ui_config_t *p_ui_config)
{   
    if(init_state) {return;}; 
    redraw = 1;
    int _keypress = *keypress & 0xDF;
    switch (_keypress)
    {
    case 'C':
        conn_req = (conn_req + 1) % 2;
        connect_ecu(p_ui_config->comport);
        redraw = 1;
        break;
    case 'V':
        vis_req = (vis_req + 1) % 2;    
        redraw = 1;    
        break;
    case 'L':
        if(log_mode < 2)
        {
            log_req = (log_req + 1) % 2;
            log_ecu(p_ui_config->log_file);
            redraw = 1;
        }
        break;    
    default:
        redraw = 0;
        break;
    }
}

