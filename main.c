#define _GNU_SOURCE

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

#include "ssm.h"
#include "config.h"
#include "log.h"
#include "measure.h"

int main(int argc, char *argv[]){

    int opt = 0;
    char *comport = "/dev/ttyUSB0";
    char *config_file = "config/signals.conf";
    char *log_file = "ecu_log.csv";

    


    while ((opt = getopt(argc, argv, "c:d:l:")) != -1 )
    {
        switch(opt)
        {
            case 'c': config_file = optarg;
                      break;
            case 'd': comport = optarg;
                      break;
            case 'l': log_file = optarg;
                      break;
            default : printf("Usage: ecuscan [-d serial device]\n");
                      exit(0);
        }
    }
   
}
