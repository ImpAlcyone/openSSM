//
//    **                                                **
//   ******************************************************
//  ***  THIS PROGRAM IS THE PROPERTY OF ALCYONE LIMITED ***
//   ******************************************************
//    **                                                **
//
//  ecuscan.c by Phil Skuse
//  Copyright (C) Alcyone Limited 2008.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//

#define _GNU_SOURCE

#include <curses.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>

#include "ssm.h"

#define MAX_LABELCOUNT 32
#define MAX_LABELLENGTH 128
#define MAX_UNITLENGTH 16
#define MAX_ADDRESSLENGTH 10
#define MAX_FORMATLENGTH 16
#define MAX_CONVERSIONLENGTH 16
#define MAX_OUTPUTFORMATLENGTH 512
#define MAX_OUTPUTLINELENGTH 1024

int celsius=1,kilometers=1,logmode=0,rc;
FILE *logh=NULL;
struct timeval start,now;


/*=================================*/
/* Functions to read configuration */
/*=================================*/

typedef struct {
    uint8_t loggingEnabled;
    char label[MAX_LABELLENGTH];
    char unit[MAX_UNITLENGTH];
    uint16_t address;
    char format[MAX_FORMATLENGTH];
    int conversionOffset;
    int conversionMulFactor;
    int conversionDivFactor;
} SignalConfig_t;

int load_signal_config(const char *filename, SignalConfig_t *signals, int max_label_count) {
    
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open signal config file");
        return -1;
    }

    
    char *line = NULL;
    size_t len = 0;
    ssize_t readCharCount;
    int signalCount = 0;
    size_t max_field_charCount = 0;

    char loggingEnabled = '\0';
    char label[MAX_LABELLENGTH] = {'\0'};
    char unit[MAX_UNITLENGTH] = {'\0'};
    char address[MAX_ADDRESSLENGTH] = {'\0'};
    char format[MAX_FORMATLENGTH] = {'\0'};
    char convOffset[MAX_CONVERSIONLENGTH] = {'\0'};
    char convMulFac[MAX_CONVERSIONLENGTH] = {'\0'};
    char convDivFac[MAX_CONVERSIONLENGTH] = {'\0'};
    uint64_t writeCount = 0;
    int fieldCounter = 0;
    uint8_t changeField = 1;
    char *pCurrentWriteChar = NULL;


    while ((readCharCount = getline(&line, &len, file)) != -1 && signalCount < max_label_count) {
        
        // Skip header or comments
        if (line[0] == '#') continue;

        // reset 
        loggingEnabled = '\0';
        memset(label, '\0', sizeof(label));
        memset(unit, '\0', sizeof(unit));
        memset(address, '\0', sizeof(address));
        memset(format, '\0', sizeof(format));
        memset(convOffset, '\0', sizeof(convOffset));
        memset(convMulFac, '\0', sizeof(convMulFac));
        memset(convDivFac, '\0', sizeof(convDivFac));
        writeCount = 0;
        fieldCounter = 0;
        changeField = 1;
        pCurrentWriteChar = NULL;

        // loop the current line characters
        for(int writeCharIdx = 0; writeCharIdx < readCharCount; writeCharIdx++){
            if(line[writeCharIdx] == ','){
                fieldCounter++;
                changeField = 1;
                continue;
                // skip commas, change to next struct field
            }else if(line[writeCharIdx] == '\n'){
                break;
                // exit loop, read next line
            }

            if(changeField){
                writeCount = 0;
                switch (fieldCounter)
                {
                    case 0:
                        pCurrentWriteChar = &loggingEnabled;
                        max_field_charCount = 1;
                        break;
                    
                    case 1:
                        pCurrentWriteChar = label;
                        max_field_charCount = MAX_LABELLENGTH;
                        break;

                    case 2:
                        pCurrentWriteChar = unit;
                        max_field_charCount = MAX_UNITLENGTH;
                        break;

                    case 3:
                        pCurrentWriteChar = address;
                        max_field_charCount = MAX_ADDRESSLENGTH;
                        break;

                    case 4:
                        pCurrentWriteChar = format;
                        max_field_charCount = MAX_FORMATLENGTH;
                        break;

                    case 5:
                        pCurrentWriteChar = convOffset;
                        max_field_charCount = MAX_CONVERSIONLENGTH;
                        break;

                    case 6:
                        pCurrentWriteChar = convMulFac;
                        max_field_charCount = MAX_CONVERSIONLENGTH;
                        break;

                    case 7:
                        pCurrentWriteChar = convDivFac;
                        max_field_charCount = MAX_CONVERSIONLENGTH;
                        break;

                    default:
                        break;
                }
                changeField = 0;
            }
            if(writeCount >= max_field_charCount){
                //don't write to buffer if counter is greater than buffer size
                continue;
            }
            if(pCurrentWriteChar != NULL){
                // write current character to corresponding buffer
                *pCurrentWriteChar = line[writeCharIdx];
                // increase pointer to destination string character
                pCurrentWriteChar++;
                writeCount++;
            }
        }

        // write values to current signal
        signals[signalCount].loggingEnabled = (u_int8_t)atoi(&loggingEnabled);
        strncpy(signals[signalCount].label, label, MAX_LABELLENGTH);
        strncpy(signals[signalCount].unit, unit, MAX_UNITLENGTH);
        u_int tempAddress = 0x0;
        sscanf(address, "%X", &tempAddress);
        signals[signalCount].address = (uint16_t)tempAddress;
        strncpy(signals[signalCount].format, format, MAX_FORMATLENGTH);
        signals[signalCount].conversionOffset = atoi(convOffset);
        signals[signalCount].conversionMulFactor = atoi(convMulFac);
        signals[signalCount].conversionDivFactor = atoi(convDivFac);

                                                
        signalCount++;
    }

    free(line);
    fclose(file);
    return signalCount;
}

int find_signal_index(const char *label, int signalCount, SignalConfig_t *signals) {
    for (int idx = 0; idx < signalCount; idx++) {
        if (strcmp(signals[idx].label, label) == 0) {
            return idx;
        }
    }
    return -1; // not found
}


/*=================================*/
/* Functions to handle colour text */
/*=================================*/

void setup_colours()
{
    start_color();
    init_pair(1,COLOR_WHITE,COLOR_BLACK);
    init_pair(2,COLOR_BLACK,COLOR_GREEN);
    init_pair(3,COLOR_BLACK,COLOR_YELLOW);
    init_pair(4,COLOR_BLACK,COLOR_RED);
    init_pair(5,COLOR_BLACK,COLOR_CYAN);
}

void white_on_black()
{
	attron(COLOR_PAIR(1));
}

void black_on_green()
{
	attron(COLOR_PAIR(2));
}

void black_on_yellow()
{
	attron(COLOR_PAIR(3));
}

void black_on_red()
{
	attron(COLOR_PAIR(4));
}

void black_on_cyan()
{
	attron(COLOR_PAIR(5));
}

/*===============================================*/

void draw_screen(uint8_t *visAcv, uint8_t *connAcv, int *romId)
{
    clear();
    white_on_black();
    move(0,31);
    printw("Subaru ECU Utility");
    move(1,28);
    printw("========================");

    
    switch (*connAcv)
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
    
        

    switch (*visAcv)
    {
        case 0: white_on_black(); break;
        case 1: black_on_green(); break;
    }
    move(20,35);
    printw(" Visualization ");

    switch (logmode)
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

void screen_too_small()
{
    move(0,0);
    white_on_black();
    printw("Please enlarge your terminal: 80x25 minimum required.");
    refresh();
}

/*===============================================*/
/* bar() draws a bar graph of a given percentage */
/*===============================================*/

void bar(int percent)
{
    int x,y;
    black_on_green();
    if (percent>100) percent=100;
    for (x=0;x<percent/5;x++) printw(" ");
    black_on_yellow();
    for (y=x;y<20;y++) printw(" ");
}

void absbar(int value, int min, int max){
    {
    int x,y,percent;

    if (value < ((min+max)>>1)){
        black_on_red();
    }else{
        black_on_green();
    }
        if (value < 130) 
            black_on_green();

    percent=((value+min)*100)/(min+max);
    if (percent>100) percent=100;

    for (x=0;x<percent/5;x++) printw(" ");
    black_on_yellow();
    for (y=x;y<20;y++) printw(" ");
}
}

void Tempbar(int tempC)
{
    int x,y,percent;

    if (tempC < 50)
        black_on_cyan();
    else
        if (tempC < 130) 
            black_on_green();
        else 
            black_on_red();

    percent=((tempC+50)*100)/255;
    if (percent>100) percent=100;

    for (x=0;x<percent/5;x++) printw(" ");
    black_on_yellow();
    for (y=x;y<20;y++) printw(" ");
}

void display_data(int signalCount, SignalConfig_t *signals ,int *measbuffer)
{
    int data;
    int currentIdx = -1;
    
   
    /*----------*/
    /* Read KMH */
    /*----------*/
    
    currentIdx = find_signal_index("vehicleSpeed", signalCount, signals);
    if(currentIdx == -1){
        data = 0;
    }else{
        data = measbuffer[currentIdx];
    }

   
    move(4,8);
    white_on_black();
    printw("Vehicle Speed");

    move(5,5);
    bar((data*100)/180);

    move(5,26);
    white_on_black();
    printw("%3d km/h ",data);
   
    refresh();

    /*----------*/
    /* Read RPM */
    /*----------*/

    currentIdx = find_signal_index("engineSpeed", signalCount, signals);
    if(currentIdx == -1){
        data = 0;
    }else{
        data = measbuffer[currentIdx];
    }
   
    move(7,8);
    white_on_black();
    printw("Engine Speed");

    move(8,5);
    bar(data*100/9000);

    move(8,26);
    white_on_black();
    printw(" %4d rpm",data);
   
    refresh();

    /*----------*/
    /* Read TPS */
    /*----------*/

    currentIdx = find_signal_index("throttlePosition", signalCount, signals);
    if(currentIdx == -1){
        data = 0;
    }else{
        data = measbuffer[currentIdx];
    }
    
    move(10,7);
    white_on_black();
    printw("Throttle Position");
    int tpsV = ((data*255)*5)/255;
    move(11,5);
    bar(data);

    move(11,26);
    white_on_black();
    printw(" %1d.%02dV (%3d%%)",tpsV/100,tpsV%100,data);

    refresh();

    /*----------------------*/
    /* Read Idle Valve Duty */
    /*----------------------*/

    currentIdx = find_signal_index("ISUDutyValve", signalCount, signals);
    if(currentIdx == -1){
        data = 0;
    }else{
        data = measbuffer[currentIdx];
    }
     
    move(13,8);
    white_on_black();
    printw("Idle Valve Duty");

    move(14,5);
    bar(data);

    move(14,26);
    white_on_black();
    printw(" %3d%% ",data);
   
    refresh();

    /*---------------------------*/
    /* Read Injector Pulse width */
    /*---------------------------*/

    currentIdx = find_signal_index("injectorPulseWidth", signalCount, signals);
   if(currentIdx == -1){
        data = 0;
    }else{
        data = measbuffer[currentIdx];
    }

    move(16,8);
    white_on_black();
    printw("Injector Pulse Width");

    move(17,5);
    bar((data)*100/65);

    move(17,26);
    white_on_black();
    printw(" %4dms ",data);

    refresh();

    /*-----------*/
    /* Read Temp */
    /*-----------*/

    currentIdx = find_signal_index("coolantTemp", signalCount, signals);
    if(currentIdx == -1){
        data = 0;
    }else{
        data = measbuffer[currentIdx];
    }
   
    move(4,47);
    white_on_black();
    printw("Coolant Temperature");

    move(5,47);
    Tempbar(data);

    move(5,68);
    white_on_black();
    if (celsius) printw(" %3d C ",data);

    refresh();

    /*-----------------------*/
    /* Read Ignition advance */
    /*-----------------------*/

    currentIdx = find_signal_index("ignitionAdvance", signalCount, signals);
    if(currentIdx == -1){
        data = 0;
    }else{
        data = measbuffer[currentIdx];
    }
    
    move(7,49);
    white_on_black();
    printw("Ignition Advance");

    move(8,47);
    bar(data*100/255);

    move(8,68);
    white_on_black();
    printw(" %3d degCrk ",data); 
  
    refresh();

      
    /*-----------------------*/
    /* Read Knock correction */
    /*-----------------------*/

    currentIdx = find_signal_index("knockCorrection", signalCount, signals);
    if(currentIdx == -1){
        data = 0;
    }else{
        data = measbuffer[currentIdx];
    }
    
    move(10,49);
    white_on_black();
    printw("Knock correction");

    move(11,47);
    bar(data*100/255);

    move(11,68);
    white_on_black();
    printw(" %3d - ",data); 
  
    refresh();

    /*-----------------------*/
    /* Read Engine Load      */
    /*-----------------------*/

    currentIdx = find_signal_index("engineLoad", signalCount, signals);
    if(currentIdx == -1){
        data = 0;
    }else{
        data = measbuffer[currentIdx];
    }
    
    move(13,49);
    white_on_black();
    printw("Engine Load");

    move(14,47);
    bar(data*100/255);

    move(14,68);
    white_on_black();
    printw(" %3d - ",data); 
  
    refresh();

    /*-----------------------*/
    /* Read Lambda correction*/
    /*-----------------------*/

    currentIdx = find_signal_index("AFCorrection", signalCount, signals);
    if(currentIdx == -1){
        data = 0;
    }else{
        data = measbuffer[currentIdx];
    }
    
    move(16,49);
    white_on_black();
    printw("AFR correction");

    move(17,47);
    bar(((data*128)/100)+128);

    move(17,68);
    white_on_black();
    printw(" %3d%% ",data); 
  
    refresh();
}
    
void measure_data(int signalCount, SignalConfig_t *signals, int *measbuffer){
    
    uint8_t rawData = 0;
    uint32_t elapsed_ms = 0;

    for (int pollIdx = 0; pollIdx < signalCount; pollIdx++) {
        if (!signals[pollIdx].loggingEnabled) {
            continue;
        }

        if ((rc = ssm_query_ecu(signals[pollIdx].address, &rawData, 1)) != 0) {
            continue; // failed read is skipped
        }

        // Update the buffer for this signal only
        measbuffer[pollIdx] = ((rawData + signals[pollIdx].conversionOffset) * 
                               signals[pollIdx].conversionMulFactor) / 
                               signals[pollIdx].conversionDivFactor;

        if (logmode == 1) {
            // Get time with milliseconds precision
            gettimeofday(&now, NULL);
            elapsed_ms = (now.tv_sec - start.tv_sec) * 1000U +
                         (now.tv_usec - start.tv_usec) / 1000U;

            char logLine[MAX_OUTPUTLINELENGTH] = {'\0'};
            char *p = logLine;
            size_t remaining = sizeof(logLine);
            int written;

            // Write timestamp
            written = snprintf(p, remaining, "%u.%03u", 
                elapsed_ms / 1000U, 
                elapsed_ms % 1000U);
            if (written < 0 || (size_t)written >= remaining) {
                continue;
            }
            p += written;
            remaining -= written;

            // Add all values from measbuffer in consistent column order
            for (int logIdx = 0; logIdx < signalCount; logIdx++) {
                if (!signals[logIdx].loggingEnabled)
                    continue;

                // Add comma
                written = snprintf(p, remaining, ",");
                if (written < 0 || (size_t)written >= remaining) break;
                p += written;
                remaining -= written;

                // Format value
                written = snprintf(p, remaining,
                     signals[logIdx].format,
                      measbuffer[logIdx]);
                if (written < 0 || (size_t)written >= remaining) break;
                p += written;
                remaining -= written;
            }

            // Newline
            snprintf(p, remaining, "\n");

            // Write to log
            rc = fputs(logLine, logh);
            if (rc < 0) logmode = 2;

            move(23, 25);
            printw("Logging since %02d:%02d:%02d ", 
                (elapsed_ms/3600000U), 
                (elapsed_ms/60000U)%60,
                (elapsed_ms/1000U)%60);
        }
    }
}    
   
 
void build_logfile_header(int signalCount, const SignalConfig_t *signals, char logfileHeader[2][MAX_OUTPUTLINELENGTH]) {

    const char *pLabel = NULL;
    const char *pUnit = NULL;
    char *timeLabel = "time";
    char *timeUnit = "s";
    uint8_t labelDone = 0;
    uint8_t unitDone = 0; 
    uint writeLabelCharIdx = 0;
    uint writeUnitCharIdx = 0;
    uint readCharIdx = 0;


    for (int signalIdx = -1; signalIdx < signalCount; signalIdx++) {
       
        if(-1 == signalIdx){
            pLabel = timeLabel;
            pUnit = timeUnit;
        }else{
            if (0 == signals[signalIdx].loggingEnabled){
                continue;
                // only write signals to header if they are to be logged
            }
            pLabel = signals[signalIdx].label;
            pUnit = signals[signalIdx].unit;
        }
        labelDone = 0; 
        unitDone = 0;
        readCharIdx = 0;
        
        while(writeLabelCharIdx < MAX_OUTPUTLINELENGTH - 2 || writeUnitCharIdx < MAX_OUTPUTLINELENGTH - 2){
            
            if(pLabel[readCharIdx] != '\0' && readCharIdx < MAX_LABELLENGTH){
                logfileHeader[0][writeLabelCharIdx] = pLabel[readCharIdx];
                writeLabelCharIdx++;
            }else if(labelDone == 0){
                logfileHeader[0][writeLabelCharIdx] = ',';
                writeLabelCharIdx++;
                labelDone = 1;
            }

            if(pUnit[readCharIdx] != '\0' && readCharIdx < MAX_UNITLENGTH){
                logfileHeader[1][writeUnitCharIdx] = pUnit[readCharIdx];
                writeUnitCharIdx++;
            }else if(unitDone == 0){
                logfileHeader[1][writeUnitCharIdx] = ',';
                writeUnitCharIdx++;
                unitDone = 1;
            }
            
            readCharIdx++;

            if(unitDone && labelDone){
                break;
                // exit while loop if both are finished
            }
        }
    }

    logfileHeader[0][writeLabelCharIdx - 1] = '\n';
    logfileHeader[0][writeLabelCharIdx] = '\0';
    logfileHeader[1][writeUnitCharIdx - 1] = '\n';
    logfileHeader[1][writeUnitCharIdx] = '\0';
}

// Function to get the current timestamp as a formatted string
void get_time_string(char *time_str, size_t max_len, struct timeval *start) {
    struct tm *tm_info;
    char time_buf[32];

    tm_info = localtime(&start->tv_sec);

    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d_%H-%M-%S", tm_info);
    snprintf(time_str, max_len, "%s", time_buf);
}


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
    
    signalCount = load_signal_config(configfile, signals, MAX_LABELCOUNT);
    if (signalCount <= 0) {
        fprintf(stderr, "No valid signals loaded from config file.\n");
        exit(1);
    }


    build_logfile_header(signalCount, signals, logfileHeader);
    
    mkdir("log", 0755);  // creates log/ if it doesn't exist (UNIX)

    initscr();
    curs_set(0);
    noecho();
    setup_colours();
    nodelay(stdscr,FALSE);

    gettimeofday(&start,NULL);

    while((keypress & 0xDF) != 'Q')
    {   
        if(1 == connectionActive){
            measure_data(signalCount, signals, measBuffer);
        }

        getmaxyx(stdscr,height,width);
        if ((height < 25) || (width < 80))
        {
            if (0 == need_redraw)
            {
                clear();
                need_redraw = 1;
            }
            screen_too_small();
    	}
        else 
        {   
            if(connectReq && 1 != connectionActive){
                if ((rc=ssm_open(comport)) != 0)
                    {
                        connectionActive = 2;
                        connectReq = 0;
                }else{
                    connectionActive = 1;
                    if(0 != ssm_romid_ecu(&romId)){
                        romId = 0x0;
                    }
                    nodelay(stdscr, TRUE);
                }
                need_redraw=1;
            }
            if(!connectReq && 1 == connectionActive){
                ssm_close();
                connectionActive = 0;
                logmode = 0;
                nodelay(stdscr, FALSE);
                need_redraw=1;
            }
            
            if(visualizeReq && 1 == connectionActive){
                display_data(signalCount, signals, measBuffer);
                if(!visualizeActive){
                    visualizeActive = 1;
                    need_redraw=1;
                }
            }else{
                if(1 == visualizeActive){
                    visualizeActive = 0;
                    need_redraw = 1;
                }
            }


            if(1 == need_redraw){
                draw_screen(&visualizeActive, &connectionActive, &romId);
                need_redraw = 0;
            }

        }
        keypress=getch();

        if ((keypress & 0xDF) == 'L' && (logmode < 2)) {
            logmode=(logmode+1) % 2;
        }
        if ((keypress & 0xDF) == 'V' && (visualizeReq < 2)) {
            visualizeReq=(visualizeReq+1) % 2;
        }
        if ((keypress & 0xDF) == 'C' && (connectReq < 2)) {
            connectReq=(connectReq+1) % 2;
        }

        if (connectionActive && (logmode==1) && (logh == NULL))
        {       
            // get current time again to start logging from 0
            gettimeofday(&start,NULL);

            // Get current time for logfile naming
            get_time_string(time_str, sizeof(time_str), &start);

            // Modify logfile name with the current time
            snprintf(logfile, sizeof(logfile), "log/%s_%08X_ecuscan.csv", time_str, romId);

            logh=fopen(logfile,"a");
            if (logh == NULL) logmode=2;
            rc = fputs(logfileHeader[0], logh);
            if (rc<0) logmode=2;
            rc = fputs(logfileHeader[1], logh);
            if (rc<0) logmode=2;
            need_redraw = 1;
        }
        if ((0 == logmode) && (logh != NULL))
        {
            fclose(logh);
            logh=NULL;
        }
    }
    if (logh != NULL) fclose(logh);
    endwin();
    ssm_close();
}
