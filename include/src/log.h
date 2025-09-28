#ifndef LOG_H
#define LOG_H

#include <sys/time.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

//defines
#define MAX_FILENAME_LENGTH 0xFFFF

// functions
void set_start_time(struct timeval *start);
void set_current_time(struct timeval *now);
void set_logmode(int ext_logmode);
void set_logfile_name(char *ext_logfileName);
int get_logfileName_state(void);
FILE * get_log_file(void);
void build_logfile_header(const SignalConfig_t *signals, int signalCount);
int open_logfile(int romId);
int write_log_line(int signalCount, SignalConfig_t *signals, int *measbuffer);
int close_logfile(void);

#ifdef __cplusplus
}
#endif

#endif //LOG_H