#ifndef MEASURE_UI_H
#define MEASURE_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config_ui.h"

void run_measure_tab(ui_config_t *p_ui_config);
void user_input_measure(int *keypress, ui_config_t *p_ui_config);

#ifdef __cplusplus
}
#endif

#endif // MEASURE_UI_H
