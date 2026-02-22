#ifndef GPIOSCONFIG_H
#define GPIOSCONFIG_H

#define LED_BUILTIN GPIO_NUM_2

#define ONE_WIRE_PIN GPIO_NUM_16

#define ADC_UNIT_ID ADC_UNIT_1
#define ADC_CHANNEL ADC_CHANNEL_3  // GPI39
#define ADC_ATTEN ADC_ATTEN_DB_12  // jusqu'à ~3.3V
#define POWER_ADC_PIN GPIO_NUM_18

#define DISABLE_DEEP_SLEEP_PIN GPIO_NUM_19

#define DIP_SWITCH_PINS_COUNT 6
static const int DIP_SWITCH_PINS[DIP_SWITCH_PINS_COUNT] = {23, 25, 26,
                                                           27, 32, 33};

#define SDA_PIN GPIO_NUM_21
#define SCL_PIN GPIO_NUM_22

#define TARE_SCALE_PIN GPIO_NUM_17

#endif