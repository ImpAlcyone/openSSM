#ifndef CONFIG_UI_H
#define CONFIG_UI_H

/* Includes */

#ifdef __cplusplus
extern "C" {
#endif

/* defines */
#define MAX_FILEPATH_LENGTH 0xFFF

/* structs */
typedef struct
{
    char config_file[MAX_FILEPATH_LENGTH];
    char log_file[MAX_FILEPATH_LENGTH];
    char comport[MAX_FILEPATH_LENGTH];
} ui_config_t;

/* functions */

void setup_colours(void);
void white_on_black(void);
void black_on_green(void);
void black_on_yellow(void);
void black_on_red(void);
void black_on_cyan(void);


#ifdef __cplusplus
}
#endif

#endif // CONFIG_UI_H