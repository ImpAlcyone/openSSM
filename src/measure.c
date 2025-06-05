#include <stdint.h>

#include "config.h"
#include "measure.h"
#include "ssm.h"


void measure_data(SignalConfig_t *signals, int *measbuffer, int signalCount)
{    
    uint8_t rawData = 0;
    uint32_t elapsed_ms = 0;
    int ssm_return = 0;

    for (int pollIdx = 0; pollIdx < signalCount; pollIdx++) {
        if (!signals[pollIdx].loggingEnabled) {
            continue; //skip signal that is not to be logged
        }

        ssm_return = ssm_query_ecu(signals[pollIdx].address, &rawData, 1);

        if (ssm_return != 0) {
            continue; // failed read is skipped
        }

        // Update the buffer for this signal only
        measbuffer[pollIdx] = ((rawData + signals[pollIdx].conversionOffset) * 
                               signals[pollIdx].conversionMulFactor) / 
                               signals[pollIdx].conversionDivFactor;
        }
}
 