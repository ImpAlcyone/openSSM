//
//    **                                                **
//   ******************************************************
//  ***  THIS PROGRAM IS THE PROPERTY OF ALCYONE LIMITED ***
//   ******************************************************
//    **                                                **
//
//  tcuscan.c by Phil Skuse
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


#include <curses.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/time.h>

int celsius=1,kilometers=0,logmode=0,rc;
FILE *logh=NULL;
struct timeval start,now;

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
    move(0,24);
    printw("Subaru 4EAT Transmission Utility");
    move(1,24);
    printw("================================");
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

void ATFbar(int tempF)
{
    int x,y,percent;

    if (tempF < 50)
        black_on_cyan();
    else
        if (tempF < 230) 
            black_on_green();
        else 
            black_on_red();

    percent=(tempF*100)/255;
    if (percent>100) percent=100;

    for (x=0;x<percent/5;x++) printw(" ");
    black_on_yellow();
    for (y=x;y<20;y++) printw(" ");
}

void display_data()
{
    unsigned char data;
    unsigned char stick,fwd=0,man=0,pwr=0,crs=0,abs=0,brk=0,gear=0,mode=0;
    unsigned char sol1=0,sol2=0,sol3=0;
    int rc,DutyA,DutyB,DutyC,ATFtempC,ATFtempF,tps,rpm,temp;
    int vss1kmh,vss1mph,vss2kmh,vss2mph,batt,atmos,vss1,vss2,wtf21;

    /*------------------------------------------*/
    /* Read FWD, Power, Cruise and ABS Switches */
    /*------------------------------------------*/

    if ((rc=ssm_query_tcu(0x0011,&data,1)) != 0)
    {
        printf("ssm_query_tcu() returned %d\n",rc);
        ssm_close();
        exit(7);
    }
    if (data & 0x80) fwd=1; else fwd=0;
    if (data & 0x40) pwr=1; else pwr=0;
    if (data & 0x20) crs=1; else crs=0;
    if (data & 0x10) abs=1; else abs=0;

    move(22,13);
    if (fwd == 1) black_on_red(); else white_on_black();
    printw(" FWD ");

    move(22,23);
    if (pwr == 1) black_on_green(); else white_on_black();
    printw(" PWR ");

    move(22,43);
    if (crs == 1) black_on_green(); else white_on_black();
    printw(" CRS ");

    move(22,63);
    if (abs == 1) black_on_red(); else white_on_black();
    printw(" ABS ");

    refresh();

    /*------------------------------*/
    /* Read Manual Switch and Stick */
    /*------------------------------*/

    if ((rc=ssm_query_tcu(0x0012,&data,1)) != 0)
    {
        printf("ssm_query_tcu() returned %d\n",rc);
        ssm_close();
        exit(7);
    }
    if (data & 0x40) man=1; else man=0;
    switch (data & 0x3F)
    {
        case 0x20: stick='1'; break;
        case 0x10: stick='2'; break;
        case 0x08: stick='3'; break;
        case 0x04: stick='D'; break;
        case 0x02: stick='R'; break;
        case 0x01: stick='N'; break;
        default  : stick=' '; break;
    }

    move(4,38);
    if (stick == 'R') black_on_green(); else white_on_black();
    printw(" R ");

    move(5,37);
    if (stick == 'N') black_on_yellow(); else white_on_black();
    printw(" N/P ");

    move(6,38);
    if (stick == 'D') black_on_green(); else white_on_black();
    printw(" D ");

    move(7,38);
    if (stick == '3') black_on_green(); else white_on_black();
    printw(" 3 ");

    move(8,38);
    if (stick == '2') black_on_green(); else white_on_black();
    printw(" 2 ");

    move(9,38);
    if (stick == '1') black_on_green(); else white_on_black();
    printw(" 1 ");

    move(22,33);
    if (man == 1) black_on_yellow(); else white_on_black();
    printw(" MAN ");

    refresh();

    /*-------------------*/
    /* Read Brake Switch */
    /*-------------------*/

    if ((rc=ssm_query_tcu(0x0013,&data,1)) != 0)
    {
        printf("ssm_query_tcu() returned %d\n",rc);
        ssm_close();
        exit(7);
    }
    if (data & 0x04) brk=1; else brk=0;

    move(22,53);
    if (brk == 1) black_on_yellow(); else white_on_black();
    printw(" BRK ");

    refresh();

    /*-------------------*/
    /* Read Current Gear */
    /*-------------------*/

    if ((rc=ssm_query_tcu(0x004E,&data,1)) != 0)
    {
        printf("ssm_query_tcu() returned %d\n",rc);
        ssm_close();
        exit(7);
    }
    if (data < 4) gear=data+1; else gear=0;

    move(20,12);
    if (gear == 1) black_on_cyan(); else white_on_black();
    printw(" 1ST ");

    move(20,22);
    if (gear == 2) black_on_cyan(); else white_on_black();
    printw(" 2ND ");

    move(20,32);
    if (gear == 3) black_on_cyan(); else white_on_black();
    printw(" 3RD ");

    move(20,42);
    if (gear == 4) black_on_cyan(); else white_on_black();
    printw(" 4TH ");

    refresh();

    /*--------------------*/
    /* Read Shift Pattern */
    /*--------------------*/

    if ((rc=ssm_query_tcu(0x005A,&data,1)) != 0)
    {
        printf("ssm_query_tcu() returned %d\n",rc);
        ssm_close();
        exit(7);
    }
    if (data >0) mode=1; else mode=0;

    move(20,52);
    if (mode == 0) black_on_yellow(); else white_on_black();
    printw(" NORMAL ");

    move(20,62);
    if (mode == 1) black_on_green(); else white_on_black();
    printw(" POWER ");

    refresh();

    /*----------------------*/
    /* Read SolA Duty Cycle */
    /*----------------------*/

    /*-----------------------------------------------------------------*/
    /* The TCU returns the duty cycle as a number between 0 and 200    */
    /* We multiply this by 5 and store an integer between 0 and 1000.  */
    /* A decimal place is assumed before that last digit               */
    /* eg. 1000 is 100.0 percent                                       */
    /*-----------------------------------------------------------------*/

    if ((rc=ssm_query_tcu(0x00B3,&data,1)) != 0)
    {
        printf("ssm_query_tcu() returned %d\n",rc);
        ssm_close();
        exit(7);
    }
    DutyA=data*5;

    move(4,8);
    white_on_black();
    printw("Duty Solenoid A");

    move(5,5);
    bar(DutyA/10);

    move(5,26);
    white_on_black();
    printw(" %3d.%1d%% ",DutyA/10,DutyA%10);

    refresh();

    /*----------------------*/
    /* Read SolB Duty Cycle */
    /*----------------------*/

    if ((rc=ssm_query_tcu(0x00B4,&data,1)) != 0)
    {
        printf("ssm_query_tcu() returned %d\n",rc);
        ssm_close();
        exit(7);
    }
    DutyB=data*5;

    move(7,8);
    white_on_black();
    printw("Duty Solenoid B");

    move(8,5);
    bar(DutyB/10);

    move(8,26);
    white_on_black();
    printw(" %3d.%1d%% ",DutyB/10,DutyB%10);

    refresh();

    /*----------------------*/
    /* Read SolC Duty Cycle */
    /*----------------------*/

    if ((rc=ssm_query_tcu(0x00B5,&data,1)) != 0)
    {
        printf("ssm_query_tcu() returned %d\n",rc);
        ssm_close();
        exit(7);
    }
    DutyC=data*5;

    move(10,8);
    white_on_black();
    printw("Duty Solenoid C");

    move(11,5);
    bar(DutyC/10);

    move(11,26);
    white_on_black();
    printw(" %3d.%1d%% ",DutyC/10,DutyC%10);

    refresh();

    /*----------------------*/
    /* Read Shift Solenoids */
    /*----------------------*/

    if ((rc=ssm_query_tcu(0x0060,&data,1)) != 0)
    {
        printf("ssm_query_tcu() returned %d\n",rc);
        ssm_close();
        exit(7);
    }
    if (data & 0x80) sol3=1; else sol3=0;
    if (data & 0x20) sol2=1; else sol2=0;
    if (data & 0x10) sol1=1; else sol1=0;

    move(13,8);
    white_on_black();
    printw("Shift Solenoids");

    move(14,9);
    if (sol1 == 1) black_on_yellow(); else white_on_black();
    printw(" 1 ");

    move(14,14);
    if (sol2 == 1) black_on_yellow(); else white_on_black();
    printw(" 2 ");

    move(14,19);
    if (sol3 == 1) black_on_yellow(); else white_on_black();
    printw(" 3 ");

    refresh();

    /*----------*/
    /* ATF Temp */
    /*----------*/

    if ((rc=ssm_query_tcu(0x0017,&data,1)) != 0)
    {
        printf("ssm_query_tcu() returned %d\n",rc);
        ssm_close();
        exit(7);
    }

    ATFtempF=data;
    ATFtempC=((ATFtempF-32)*5)/9;

    move(4,49);
    white_on_black();
    printw("ATF Temperature");

    move(5,47);
    ATFbar(ATFtempF);

    move(5,68);
    white_on_black();
    if (celsius) printw(" %+3d C ",ATFtempC); else printw(" %+3d F ",ATFtempF);

    refresh();

    /*-------------------*/
    /* Throttle Position */
    /*-------------------*/

    if ((rc=ssm_query_tcu(0x0046,&data,1)) != 0)
    {
        printf("ssm_query_tcu() returned %d\n",rc);
        ssm_close();
        exit(7);
    }
    tps=(data*100)/255;

    move(7,48);
    white_on_black();
    printw("Throttle Position");

    move(8,47);
    bar(tps);

    move(8,67);
    white_on_black();
    printw(" %3d%% ",tps);

    refresh();

    /*-----*/
    /* RPM */
    /*-----*/

    if ((rc=ssm_query_tcu(0x003B,&data,1)) != 0)
    {
        printf("ssm_query_tcu() returned %d\n",rc);
        ssm_close();
        exit(7);
    }
    rpm=(data*250);

    move(10,51);
    white_on_black();
    printw("Engine RPM");

    move(11,47);
    bar((rpm*100)/9000);

    move(11,68);
    white_on_black();
    printw(" %4d ",rpm);

    refresh();

    /*---------------*/
    /* Speed Sensors */
    /*---------------*/

    if ((rc=ssm_query_tcu(0x0019,&data,1)) != 0)
    {
        printf("ssm_query_tcu() returned %d\n",rc);
        ssm_close();
        exit(7);
    }
    vss1kmh=data;
    vss1mph=(data*5)/8;

    if ((rc=ssm_query_tcu(0x001A,&data,1)) != 0)
    {
        printf("ssm_query_tcu() returned %d\n",rc);
        ssm_close();
        exit(7);
    }
    vss2kmh=data;
    vss2mph=(data*5)/8;

    move(13,50);
    white_on_black();
    printw("Speed Sensors");

    move(14,47);
    bar(vss1kmh/3);
    move(15,47);
    bar(vss2kmh/3);

    white_on_black();
    move(14,68);
    if (kilometers) printw("1 %3d km/h ",vss1kmh); else printw("1 %3d mph ",vss1mph);
    move(15,68);
    if (kilometers) printw("2 %3d km/h ",vss2kmh); else printw("2 %3d mph ",vss2mph);

    /*---------*/
    /* Battery */
    /*---------*/

    if ((rc=ssm_query_tcu(0x0016,&data,1)) != 0)
    {
        printf("ssm_query_tcu() returned %d\n",rc);
        ssm_close();
        exit(7);
    }
    batt=((data*8)/10);

    move(17,47);
    white_on_black();
    printw("Battery Voltage:      %2d.%1d V ",batt/10,batt%10);

    refresh();

    /*----------------------*/
    /* Atmospheric Pressure */
    /*----------------------*/

    if ((rc=ssm_query_tcu(0x0015,&data,1)) != 0)
    {
        printf("ssm_query_tcu() returned %d\n",rc);
        ssm_close();
        exit(7);
    }
    atmos=(data*8);

    move(18,47);
    white_on_black();
    printw("Atmospheric Pressure: %4d mmHg ",atmos);

    refresh();

    /* Unknown 0x21 */

    if ((rc=ssm_query_tcu(0x0021,&data,1)) != 0)
    {
        printf("ssm_query_tcu() returned %d\n",rc);
        ssm_close();
        exit(7);
    }
    wtf21=data;
    if ((rc=ssm_query_tcu(0x0022,&data,1)) != 0)
    {
        printf("ssm_query_tcu() returned %d\n",rc);
        ssm_close();
        exit(7);
    }
    wtf21=(wtf21 << 8)+data;

    /*---------*/
    /* Logmode */
    /*---------*/

    switch (logmode)
    {
        case 0: white_on_black(); break;
        case 1: black_on_green(); break;
        case 2: black_on_red();   break;
    }

    move(12,35);
    printw(" Logging ");

    if (logmode == 1)
    {
        gettimeofday(&now,NULL);
	if (celsius) temp=ATFtempC; else temp=ATFtempF;
	if (kilometers)
        {
            vss1=vss1kmh;
            vss2=vss2kmh;
        }
        else 
        {
            vss1=vss1mph;
            vss2=vss2mph;
        }
        rc=fprintf(logh,"%0d,%1d,%1d,%1d,%1d,%1d,%1d,%c,%1d,%1d,%03d.%1d,%03d.%1d,%03d.%1d,%1d,%1d,%1d,%03d,%03d,%04d,%03d,%03d,%02d.%1d,%04d,%2.2x\n",(now.tv_sec-start.tv_sec),fwd,pwr,crs,abs,man,brk,stick,gear,mode,DutyA/10,DutyA%10,DutyB/10,DutyB%10,DutyC/10,DutyC%10,sol1,sol2,sol3,temp,tps,rpm,vss1,vss2,batt/10,batt%10,atmos,wtf21);
        if (rc<0) logmode=2;
    }
}

int main(int argc, char *argv[])
{
    char rc, opt;
    char *comport="/dev/ttyS0";
    char *logfile="tcuscan.csv";
    int height,width,need_redraw=1,keypress=0;

    gettimeofday(&start,NULL);

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
            rc=fprintf(logh,"time,fwd,pwr,crs,abs,man,brk,stick,gear,mode,DutyA,DutyB,DutyC,sol1,sol2,sol3,temp,tps,rpm,vss1,vss2,batt,atmos\n");
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
