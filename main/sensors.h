#ifndef SENSORS_H
#define SENSORS_H

#include <stdbool.h>
#include <stdint.h>

#define LED_BUILTIN GPIO_NUM_8

#define ONE_WIRE_PIN GPIO_NUM_9

#define VBUS_ADC_PIN GPIO_NUM_2
#define ADC_UNIT_ID ADC_UNIT_1
#define ADC_CHANNEL ADC_CHANNEL_4  // GPIO4
#define ADC_ATTEN ADC_ATTEN_DB_12  // jusqu'à ~3.3V
#define POWER_ADC_PIN GPIO_NUM_0

#define DISABLE_DEEP_SLEEP_PIN GPIO_NUM_6
#define RESET_BLE_PIN GPIO_NUM_7

#define DIP_SWITCH_PINS_COUNT 5
static const int DIP_SWITCH_PINS[DIP_SWITCH_PINS_COUNT] = {10, 3, 5, 1, 4};
void init_sensors(void);

float read_temperature(void);
float read_voltage(void);

bool is_deep_sleep_enabled(void);
bool is_reset_ble_pressed(void);

uint8_t read_dip_switch(void);

void disable_inputs(void);

uint8_t get_UID(void);

#endif  // SENSORS_H