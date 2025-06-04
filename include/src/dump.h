#ifndef DUMP_H
#define DUMP_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//functions
void set_romfile_name(char *ext_romfileName);
int init();
int poll_and_write(uint8_t *data, int start, int end, int *romid);

#ifdef __cplusplus
}
#endif

#endif //DUMP_H