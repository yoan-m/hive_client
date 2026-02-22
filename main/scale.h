#ifndef SCALE_H
#define SCALE_H

#include <stdbool.h>
#include <stdint.h>

float read_weight(void);
bool is_scale_ready(void);
bool is_tare_scale_pressed(void);
void tare_scale(void);
void save_calibration(int32_t offset, float scale);
bool load_calibration(int32_t* offset, float* scale);
void wait_for_button(void);
void init_scale(void);
#endif  // SCALE_H