#define _GNU_SOURCE

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#include "main_ui.h"
#include "config_ui.h"

static void init_ui_config(ui_config_t *p_ui_config)
{
    strncpy(p_ui_config->comport, "/dev/ttyUSB0", MAX_FILEPATH_LENGTH);
    strncpy(p_ui_config->config_file, "config/signals.conf", MAX_FILEPATH_LENGTH);
    memset(p_ui_config->log_file, '\0', MAX_FILEPATH_LENGTH);
}

int main(int argc, char *argv[]){

    int opt = 0;
    ui_config_t ui_config;
    init_ui_config(&ui_config);

    while ((opt = getopt(argc, argv, "c:d:l:")) != -1 )
    {
        switch(opt)
        {
            case 'c':
                memset(ui_config.config_file, '\0', MAX_FILEPATH_LENGTH);
                strncpy(ui_config.config_file, optarg, MAX_FILEPATH_LENGTH);
                break;
            case 'd': 
                memset(ui_config.comport, '\0', MAX_FILEPATH_LENGTH);
                strncpy(ui_config.comport, optarg, MAX_FILEPATH_LENGTH);
                break;
            case 'l': 
                strncpy(ui_config.log_file, optarg, MAX_FILEPATH_LENGTH);
                break;
            default : printf("Usage: ecuscan [-d serial device]\n");
                      exit(0);
        }
    }

    run_main_ui(&ui_config);

    return EXIT_SUCCESS;
   
}
