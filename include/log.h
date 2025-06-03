#ifndef LOG_H
#define LOG_H

// functions
void set_start_time(struct timeval *start);
void set_current_time(struct timeval *now);
void build_logfile_header(int signalCount, const SignalConfig_t *signals);
int open_logfile(int romId);
void write_log_line(int signalCount, SignalConfig_t *signals, int *measbuffer);
void close_logfile(void);

#endif //LOG_H