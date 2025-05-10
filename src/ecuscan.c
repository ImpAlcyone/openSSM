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

#define MAX_LABELLENGTH 128
#define MAX_UNITLENGTH 16
#define MAX_LABELCOUNT 32

int celsius=1,kilometers=1,logmode=0,rc;
FILE *logh=NULL;
struct timeval start,now;


/*=================================*/
/* Functions to read configuration */
/*=================================*/

typedef struct {
    char label[MAX_LABELLENGTH];
    char unit[MAX_UNITLENGTH];
    uint16_t address;
} SignalConfig;

SignalConfig signals[MAX_LABELCOUNT];

int load_signal_config(const char *filename, SignalConfig *signals, int max_label_count) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open data map file");
        return -1;
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int count = 0;

    while ((read = getline(&line, &len, file)) != -1 && count < max_label_count) {
        // Skip header or comments
        if (line[0] == '#' || strncmp(line, "Label", 5) == 0) continue;

        // Remove newline if present
        line[strcspn(line, "\r\n")] = 0;

        char *label = strtok(line, ",");
        char *unit = strtok(NULL, ",");
        char *addr_str = strtok(NULL, ",");

        if (label && unit && addr_str) {
            strncpy(signals[count].label, label, sizeof(signals[count].label) - 1);
            strncpy(signals[count].unit, unit, sizeof(signals[count].unit) - 1);
            unsigned int addr;
            if (sscanf(addr_str, "%x", &addr) == 1) {
                signals[count].address = (uint16_t)addr;
            }
            count++;
        }
    }

    free(line);
    fclose(file);
    return count;
}

int find_signal_index(const char *label, int signalCount, SignalConfig *signals) {
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

void draw_screen()
{
    clear();
    white_on_black();
    move(0,28);
    printw("Subaru ECU Utility");
    move(1,28);
    printw("=======================");
    move(24,1);
    printw("Q:Quit   T:Toggle C/F   M:Toggle MPH/KMH   L:Toggle Logging");
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

void display_data()
{
    unsigned char data;
    int rc,tps,rpm,maf,batt,O2Avg,lambda;
    int kmh,mph,tempC,tempF,speed,temp;
    int currentIdx = -1;
    uint16_t address = 0x0;

    /*----------*/
    /* Read KMH */
    /*----------*/
    
    currentIdx = find_signal_index("vehicleSpeed", MAX_LABELCOUNT, &signals);
    if(currentIdx == -1){
        address = 0x0;
    }else{
        address = signals[currentIdx].address;
    }

    if ((rc=ssm_query_ecu(address,&data,1)) != 0)
    {
        printf("ssm_query_ecu() returned %d\n",rc);
        ssm_close();
        exit(7);
    }

    kmh=data*2;
    mph=(kmh*5)/8;

    move(4,8);
    white_on_black();
    printw("Vehicle Speed");

    move(5,5);
    bar((mph*100)/180);

    move(5,26);
    white_on_black();
    if (kilometers) printw("%3d km/h ",kmh); else printw("%3d mph ",mph);

    refresh();

    /*----------*/
    /* Read RPM */
    /*----------*/

    currentIdx = find_signal_index("engineSpeed", MAX_LABELCOUNT, &signals);
    if(currentIdx == -1){
        address = 0x0;
    }else{
        address = signals[currentIdx].address;
    }

    if ((rc=ssm_query_ecu(address,&data,1)) != 0)
    {
        printf("ssm_query_ecu() returned %d\n",rc);
        ssm_close();
        exit(7);
    }
    rpm=data*25;

    move(7,8);
    white_on_black();
    printw("Engine Speed");

    move(8,5);
    bar(rpm*100/9000);

    move(8,26);
    white_on_black();
    printw(" %4d rpm",rpm);

    refresh();

    /*----------*/
    /* Read TPS */
    /*----------*/

    currentIdx = find_signal_index("throttlePosition", MAX_LABELCOUNT, &signals);
    if(currentIdx == -1){
        address = 0x0;
    }else{
        address = signals[currentIdx].address;
    }


    if ((rc=ssm_query_ecu(address,&data,1)) != 0)
    {
        printf("ssm_query_ecu() returned %d\n",rc);
        ssm_close();
        exit(7);
    }
    tps=(data*500)/255;

    move(10,7);
    white_on_black();
    printw("Throttle Position");

    move(11,5);
    bar(tps/5);

    move(11,26);
    white_on_black();
    printw(" %1d.%02dV (%3d%%)",tps/100,tps%100,tps/5);

    refresh();

    /*----------*/
    /* Read MAF */
    /*----------*/

    currentIdx = find_signal_index("airflowSensor", MAX_LABELCOUNT, &signals);
    if(currentIdx == -1){
        address = 0x0;
    }else{
        address = signals[currentIdx].address;
    }

    if ((rc=ssm_query_ecu(address,&data,1)) != 0)
    {
        printf("ssm_query_ecu() returned %d\n",rc);
        ssm_close();
        exit(7);
    }
    
    maf=(data*500)/255;

    move(13,8);
    white_on_black();
    printw("Airflow Sensor");

    move(14,5);
    bar(maf/5);

    move(14,26);
    white_on_black();
    printw(" %1d.%02dV ",maf/100,maf%100);

    refresh();

    /*-----------*/
    /* Read Batt */
    /*-----------*/

    currentIdx = find_signal_index("batteryVoltage", MAX_LABELCOUNT, &signals);
    if(currentIdx == -1){
        address = 0x0;
    }else{
        address = signals[currentIdx].address;
    }


    if ((rc=ssm_query_ecu(address,&data,1)) != 0)
    {
        printf("ssm_query_ecu() returned %d\n",rc);
        ssm_close();
        exit(7);
    }
    
    batt=(data*8);

    move(16,8);
    white_on_black();
    printw("Battery Voltage");

    move(17,5);
    bar(batt/20);

    move(17,26);
    white_on_black();
    printw(" %2d.%02dV ",batt/100,batt%100);

    refresh();

    /*-----------*/
    /* Read Temp */
    /*-----------*/

    currentIdx = find_signal_index("coolantTemp", MAX_LABELCOUNT, &signals);
    if(currentIdx == -1){
        address = 0x0;
    }else{
        address = signals[currentIdx].address;
    }

    if ((rc=ssm_query_ecu(address,&data,1)) != 0)
    {
        printf("ssm_query_ecu() returned %d\n",rc);
        ssm_close();
        exit(7);
    }
    
    tempC=(data-50);
    tempF=(tempC*9)/5+32;

    move(4,47);
    white_on_black();
    printw("Coolant Temperature");

    move(5,47);
    Tempbar(tempC);

    move(5,68);
    white_on_black();
    if (celsius) printw(" %3d C ",tempC); else printw(" %3d F ",tempF);

    refresh();

    /*------------*/
    /* Read O2Avg */
    /*------------*/

    currentIdx = find_signal_index("O2Average", MAX_LABELCOUNT, &signals);
    if(currentIdx == -1){
        address = 0x0;
    }else{
        address = signals[currentIdx].address;
    }

    if ((rc=ssm_query_ecu(address,&data,1)) != 0)
    {
        printf("ssm_query_ecu() returned %d\n",rc);
        ssm_close();
        exit(7);
    }
    
    O2Avg=(data*5000)/512;

    move(7,49);
    white_on_black();
    printw("O2 Sensor Average");

    move(8,47);
    bar(O2Avg/25);

    move(8,68);
    white_on_black();
    printw(" %3d mV ",O2Avg); 

    refresh();

    /*------------------*/
    /* Calculate Lambda */
    /*------------------*/

    // Fake narrowband lambda: 0.88 to 1.12 scaled by 100
    lambda = 112 - ((O2Avg - 100) * 24) / 800;

    // Clamp
    if (lambda < 88) lambda = 88;
    if (lambda > 112) lambda = 112;

    move(10,49);
    white_on_black();
    printw("Lambda");

    move(11,47);
    bar(lambda/25);

    move(11,68);
    white_on_black();
    printw(" %d.%02d - ", lambda/100, lambda%100); 

    refresh();

    /*---------*/
    /* Logmode */
    /*---------*/

    switch (logmode)
    {
        case 0: white_on_black(); break;
        case 1: black_on_green(); break;
        case 2: black_on_red();   break;
    }

    move(19,35);
    printw(" Logging ");

    if (logmode == 1)
    {
        gettimeofday(&now,NULL);
        if (celsius) temp=tempC; else temp=tempF;
        if (kilometers) speed=kmh; else speed=mph;
        rc=fprintf(logh,"%d,%d,%d,%d,%d.%02d,%d.%02d,%d,%d,%d.%02d\n",\
        (now.tv_sec-start.tv_sec),speed,rpm,tps/5,maf/100,maf%100,batt/100,batt%100,temp,O2Avg,lambda/100,lambda%100);
        if (rc<0) logmode=2;
    }


}

int main(int argc, char *argv[])
{
    char rc, opt;
    char *comport="/dev/ttyUSB0";
    int height,width,need_redraw=1,keypress=0;
    char *logfile="log/ecuscan.csv";
    char *configfile="config/signals.conf";
    int signalCount = 0;


    while ((opt = getopt (argc, argv, "d:")) != -1 )
    {
        switch(opt)
        {
            case 'd': comport=optarg;
                      break;
            default : printf("Usage: tcuscan [-d serial device]\n");
                      exit(0);
        }
    }

    
    signalCount = load_signal_config(configfile, &signals, MAX_LABELCOUNT);

    mkdir("log", 0755);  // creates log/ if it doesn't exist (UNIX)



    if ((rc=ssm_open(comport)) != 0)
    {
        printf("ssm_open() returned %d\n",rc);
        perror("Error Details");
        exit(1);
    }

    initscr();
    curs_set(0);
    noecho();
    setup_colours();
    nodelay(stdscr,TRUE);

    gettimeofday(&start,NULL);

    while((keypress & 0xDF) != 'Q')
    {
        getmaxyx(stdscr,height,width);
        if ((height < 25) || (width < 80))
        {
            if (need_redraw == 0)
            {
                clear();
                need_redraw=1;
            }
            screen_too_small();
    	}
        else 
        {
            if (need_redraw == 1)
            {
                draw_screen();
                need_redraw=0;
            }
            display_data();
        }
        keypress=getch();
        if ((keypress & 0xDF) == 'T') celsius    = (celsius    + 1) % 2;
        if ((keypress & 0xDF) == 'M') kilometers = (kilometers + 1) % 2;
        if ((keypress & 0xDF) == 'L' && (logmode < 2)) logmode=(logmode+1) % 2;

        if ((logmode==1) && (logh == NULL))
        {   
            logh=fopen(logfile,"a");
            if (logh == NULL) logmode=2;
            rc=fprintf(logh,"time,speed,rpm,tps,maf,batt,temp,O2Avg,lambda\n");
            if (rc<0) logmode=2;
        }
        if ((logmode==0) && (logh != NULL))
        {
            fclose(logh);
            logh=NULL;
        }
    }
    if (logh != NULL) fclose(logh);
    endwin();
    ssm_close();
}
