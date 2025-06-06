#ifndef CONFIG_H
#define CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

// defines
#define MAX_LABELCOUNT 32
#define MAX_LABELLENGTH 128
#define MAX_UNITLENGTH 16
#define MAX_ADDRESSLENGTH 10
#define MAX_FORMATLENGTH 16
#define MAX_CONVERSIONLENGTH 16
#define MAX_OUTPUTFORMATLENGTH 512
#define MAX_OUTPUTLINELENGTH 1024


// structs
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


// functions
int load_signal_config(const char *filename, SignalConfig_t *signals);
int find_signal_index(const char *label, SignalConfig_t *signals, int signalCount);


#ifdef __cplusplus
}
#endif

#endif // CONFIG_H