#define _GNU_SOURCE

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "config.h"


int load_signal_config(const char *filename, SignalConfig_t *signals)
{    
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


    while ((readCharCount = getline(&line, &len, file)) != -1 && signalCount < MAX_LABELCOUNT) {
        
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
        signals[signalCount].loggingEnabled = (uint8_t)atoi(&loggingEnabled);
        strncpy(signals[signalCount].label, label, MAX_LABELLENGTH);
        strncpy(signals[signalCount].unit, unit, MAX_UNITLENGTH);
        uint32_t tempAddress = 0x0;
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

int find_signal_index(const char *label, SignalConfig_t *signals, int signalCount)
{
    for (int idx = 0; idx < signalCount; idx++) {
        if (strcmp(signals[idx].label, label) == 0) {
            return idx;
        }
    }
    return -1; // not found
}