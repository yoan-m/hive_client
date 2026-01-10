#ifndef SCALE_H
#define SCALE_H

#include <stdint.h>
#include <stdbool.h>

#define SDA_PIN GPIO_NUM_20
#define SCL_PIN GPIO_NUM_21

#define TARE_SCALE_PIN GPIO_NUM_8

float read_weight(void);
bool is_scale_ready(void);
bool is_tare_scale_pressed(void);
void tare_scale(void);
void save_calibration(int32_t offset, float scale);
bool load_calibration(int32_t *offset, float *scale);
void wait_for_button(void);
void init_scale(void);
#endif // SCALE_H