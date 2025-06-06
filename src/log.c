
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>

#include "config.h"
#include "log.h"


static FILE *logfile = NULL;
static char logfileName[MAX_FILENAME_LENGTH] = {'\0'};
static char logfileHeader[2][MAX_OUTPUTLINELENGTH] = {'\0'};
static int logmode = 0;
static struct timeval _start;
static struct timeval _now;

static void get_time_string(char *time_str, size_t max_len, struct timeval *_start);

static inline int min_int(int a, int b)
{
    return (a < b) ? a : b;
}

void set_start_time(struct timeval *start)
{
    _start = *start;
}

void set_current_time(struct timeval *now)
{
    _now = *now;
}

void set_logmode(int ext_logmode)
{
    logmode = ext_logmode;
}

void set_logfile_name(char *ext_logfileName)
{
    int size = 0;
    size = min_int((int)strlen(ext_logfileName), (int)MAX_FILENAME_LENGTH);
    memcpy(logfileName, ext_logfileName, size);
}

void build_logfile_header(const SignalConfig_t *signals, int signalCount)
{

    const char *pLabel = NULL;
    const char *pUnit = NULL;
    char *timeLabel = "time";
    char *timeUnit = "s";
    uint8_t labelDone = 0;
    uint8_t unitDone = 0; 
    uint32_t writeLabelCharIdx = 0;
    uint32_t writeUnitCharIdx = 0;
    uint32_t readCharIdx = 0;


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
            
            if(pLabel[readCharIdx] != '\0' && readCharIdx < MAX_LABELLENGTH &&
               labelDone != 1){
                logfileHeader[0][writeLabelCharIdx] = pLabel[readCharIdx];
                writeLabelCharIdx++;
            }else if(labelDone == 0){
                logfileHeader[0][writeLabelCharIdx] = ',';
                writeLabelCharIdx++;
                labelDone = 1;
            }

            if(pUnit[readCharIdx] != '\0' && readCharIdx < MAX_UNITLENGTH &&
               unitDone != 1){
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

int open_logfile(int romId)
{ 
    char time_str[32];
    int rc = 0;    

    if(logfileName[0] == '\0')
    {
        // get current time again to _start logging from 0
        gettimeofday(&_start,NULL);

        // Get current time for logfile naming
        get_time_string(time_str, sizeof(time_str), &_start);

        // Modify logfile name with the current time
        snprintf(logfileName, sizeof(logfile), "log/%s_%08X_ecuscan.csv", time_str, romId);
    }

    logfile=fopen(logfileName,"w");
    if (logfile == NULL) logmode=2;
    rc = fputs(logfileHeader[0], logfile);
    if (rc<0) logmode=2;
    rc = fputs(logfileHeader[1], logfile);
    if (rc<0) logmode=2;

    return logmode;
}

void write_log_line(int signalCount, SignalConfig_t *signals, int *measbuffer)
{
    uint32_t elapsed_ms = 0;
    int rc = 0;
    
    // Get time with milliseconds precision
    gettimeofday(&_now, NULL);
    elapsed_ms = (_now.tv_sec - _start.tv_sec) * 1000U +
                    (_now.tv_usec - _start.tv_usec) / 1000U;

    char logLine[MAX_OUTPUTLINELENGTH] = {'\0'};
    char *p = logLine;
    size_t remaining = sizeof(logLine);
    int written;

    // Write timestamp
    written = snprintf(p, remaining, "%u.%03u", 
        elapsed_ms / 1000U, 
        elapsed_ms % 1000U);
    if (written < 0 || (size_t)written >= remaining) {
        return;
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
    rc = fputs(logLine, logfile);
    if (rc < 0) logmode = 2;

}
        
void close_logfile(void)
{
    if(logfile != NULL){
        fclose(logfile);
        logfile=NULL;
    }
}

static void get_time_string(char *time_str, size_t max_len, struct timeval *_start)
{
    struct tm *tm_info;
    char time_buf[32];

    tm_info = localtime(&_start->tv_sec);

    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d_%H-%M-%S", tm_info);
    snprintf(time_str, max_len, "%s", time_buf);
}