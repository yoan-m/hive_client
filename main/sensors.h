#ifndef SENSORS_H
#define SENSORS_H

#include <stdbool.h>
#include <stdint.h>

void init_sensors(void);

float read_temperature(void);
float read_voltage(void);

bool is_deep_sleep_enabled(void);

uint8_t read_dip_switch(void);

void disable_inputs(void);

uint8_t get_UID(void);

#endif  // SENSORS_H