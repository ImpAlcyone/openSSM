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

    char rc, opt;
    char *comport="/dev/ttyUSB0";
    int height,width,need_redraw=1,keypress=0;
    char logfile[256]="log/ecuscan.csv";
    char *configfile="config/signals.conf";
    int signalCount = 0;
    char logfileHeader[2][MAX_OUTPUTLINELENGTH] = {'\0'};
    SignalConfig_t signals[MAX_LABELCOUNT];
    int measBuffer[MAX_LABELCOUNT] = {0};
    char time_str[32];
    int connectReq = 0;
    int visualizeReq = 0; 
    int logReq = 0;
    uint8_t connectionActive = 0;
    uint8_t visualizeActive = 0;
    int romId = 0x0;


    while ((opt = getopt (argc, argv, "d:")) != -1 )
    {
        switch(opt)
        {
            case 'd': comport=optarg;
                      break;
            default : printf("Usage: ecuscan [-d serial device]\n");
                      exit(0);
        }
    }
    
    signalCount = load_signal_config(configfile, signals);
    if (signalCount <= 0) {
        fprintf(stderr, "No valid signals loaded from config file.\n");
        return EXIT_FAILURE;
    }


    build_logfile_header(signals, signalCount);
    
    while((keypress & 0xDF) != 'Q')
    {   
        if(1 == connectionActive){
            measure_data(signalCount, signals, measBuffer);
        }

        if(connectReq && 1 != connectionActive){
            if ((rc=ssm_open(comport)) != 0)
                {
                    connectionActive = 2;
                    connectReq = 0;
            }else{
                connectionActive = 1;
                if(0 != ssm_romid_ecu(&romId)){
                    romId = 0x0;
                    connectionActive = 2;
                    connectReq = 0;
                }
            }
        }
        if(!connectReq && 1 == connectionActive){
            ssm_close();
            connectionActive = 0;
            close_logfile();
            set_logReq(0);
        }
            
            if(visualizeReq && 1 == connectionActive){
                //display_data(signalCount, signals, measBuffer);
                if(!visualizeActive){
                    visualizeActive = 1;
                }
            }else{
                if(1 == visualizeActive){
                    visualizeActive = 0;
                }
            }

            /*
            if(1 == need_redraw){
                draw_screen(&visualizeActive, &connectionActive, &romId);
                need_redraw = 0;
            }
                */

        }
        keypress=getch();

        if ((keypress & 0xDF) == 'L' && (logReq < 2)) {
            logReq=(logReq+1) % 2;
            set_logmode(logReq);
        }
        if ((keypress & 0xDF) == 'V' && (visualizeReq < 2)) {
            visualizeReq=(visualizeReq+1) % 2;
        }
        if ((keypress & 0xDF) == 'C' && (connectReq < 2)) {
            connectReq=(connectReq+1) % 2;
        }

        if (1 == connectionActive && (logReq==1))
        { 
            
        }
    close_logfile();
    ssm_close();
}
