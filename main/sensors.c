#include "sensors.h"

#include "driver/gpio.h"
#include "ds18b20.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "onewire_bus.h"
#include "scale.h"
#include "sdkconfig.h"

static const char* TAG = "SENSORS";

uint8_t UID = 0x00;
/* =========================
 * GPIO DEFINITIONS
 * ========================= */
/*
#define LED_BUILTIN GPIO_NUM_8
#define DISABLE_DEEP_SLEEP_PIN GPIO_NUM_3
#define RESET_BLE_PIN GPIO_NUM_4
#define TARE_SCALE_PIN GPIO_NUM_5

#define POWER_ADC_PIN GPIO_NUM_6
#define VBUS_ADC_CHANNEL ADC_CHANNEL_2 // adapte à ton schéma

#define DIP_SWITCH_PINS_COUNT 3
static const gpio_num_t dip_pins[DIP_SWITCH_PINS_COUNT] = {
    GPIO_NUM_10,
    GPIO_NUM_11,
    GPIO_NUM_12};*/

/* =========================
 * ADC
 * ========================= */

static ds18b20_device_handle_t ds18b20;

static adc_oneshot_unit_handle_t adc_handle;

static adc_cali_handle_t cali_handle;

/* =========================
 * INIT
 * ========================= */

static void blink(void) {
  for (int i = 0; i < 2; i++) {
    gpio_set_level(LED_BUILTIN, 0);
    vTaskDelay(pdMS_TO_TICKS(200));
    gpio_set_level(LED_BUILTIN, 1);
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

void init_temperature(void) {
  // ===== Installation du bus 1-Wire =====
  onewire_bus_handle_t bus = NULL;

  onewire_bus_config_t bus_config = {
      .bus_gpio_num = ONE_WIRE_PIN,
      .flags =
          {
              .en_pull_up =
                  true,  // pull-up interne si pas de résistance externe
          },
  };

  onewire_bus_rmt_config_t rmt_config = {
      .max_rx_bytes = 10,
  };

  ESP_ERROR_CHECK(onewire_new_bus_rmt(&bus_config, &rmt_config, &bus));
  ESP_LOGI(TAG, "1-Wire bus installé sur GPIO %d", ONE_WIRE_PIN);

  // ===== Recherche du PREMIER device =====
  onewire_device_iter_handle_t iter = NULL;
  onewire_device_t dev;

  ESP_ERROR_CHECK(onewire_new_device_iter(bus, &iter));

  if (onewire_device_iter_get_next(iter, &dev) != ESP_OK) {
    ESP_LOGE(TAG, "Aucun device OneWire trouvé");
    onewire_del_device_iter(iter);
    return;
  }

  onewire_del_device_iter(iter);

  // ===== Création du handle DS18B20 =====
  ds18b20_config_t ds_cfg = {};

  ESP_ERROR_CHECK(ds18b20_new_device_from_enumeration(&dev, &ds_cfg, &ds18b20));

  ESP_LOGI(TAG, "DS18B20 détecté");
}

void init_deep_sleep_pin(void) {
  ESP_LOGI(TAG, "Initialisation pin deep sleep");
  gpio_config_t in_pullup = {
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_ENABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
  };

  in_pullup.pin_bit_mask = 1ULL << DISABLE_DEEP_SLEEP_PIN;
  gpio_config(&in_pullup);
  ESP_LOGI(TAG, "Deep sleep pin initialized");
}

void init_led_pin(void) {
  ESP_LOGI(TAG, "Initialisation pin LED");
  gpio_config_t led_cfg = {
      .pin_bit_mask = 1ULL << LED_BUILTIN,
      .mode = GPIO_MODE_OUTPUT,
  };
  gpio_config(&led_cfg);
  gpio_set_level(LED_BUILTIN, 1);
  blink();
  ESP_LOGI(TAG, "LED pin initialized");
}

void init_adc(void) {
  ESP_LOGI(TAG, "Initialisation ADC");
  gpio_config_t adc_power = {
      .pin_bit_mask = 1ULL << POWER_ADC_PIN,
      .mode = GPIO_MODE_OUTPUT,
  };
  gpio_config(&adc_power);
  gpio_set_level(POWER_ADC_PIN, 0);

  // -------- ADC init --------
  adc_oneshot_unit_init_cfg_t unit_cfg = {
      .unit_id = ADC_UNIT_ID,
  };
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &adc_handle));

  adc_oneshot_chan_cfg_t chan_cfg = {
      .bitwidth = ADC_BITWIDTH_DEFAULT,
      .atten = ADC_ATTEN,
  };
  ESP_ERROR_CHECK(
      adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &chan_cfg));

  // -------- Calibration --------
  adc_cali_curve_fitting_config_t cali_cfg = {
      .atten = ADC_ATTEN,
      .bitwidth = ADC_BITWIDTH_DEFAULT,
      .unit_id = ADC_UNIT_ID,
  };
  ESP_ERROR_CHECK(
      adc_cali_create_scheme_curve_fitting(&cali_cfg, &cali_handle));

  ESP_LOGI(TAG, "ADC prêt");
}

void init_buttons(void) {
  ESP_LOGI(TAG, "Initialisation boutons");
  gpio_config_t in_pullup = {
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_ENABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
  };

  in_pullup.pin_bit_mask = (1ULL << RESET_BLE_PIN) | (1ULL << TARE_SCALE_PIN);
  gpio_config(&in_pullup);
  ESP_LOGI(TAG, "Buttons initialized");
}

void init_dip_switches(void) {
  ESP_LOGI(TAG, "Initialisation DIP switches");
  gpio_config_t in_pullup = {
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_ENABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
  };

  uint64_t dip_mask = 0;
  for (int i = 0; i < DIP_SWITCH_PINS_COUNT; i++) {
    dip_mask |= 1ULL << DIP_SWITCH_PINS[i];
  }
  in_pullup.pin_bit_mask = dip_mask;
  gpio_config(&in_pullup);
  ESP_LOGI(TAG, "DIP switches initialized");
}

void init_sensors(void) {
  ESP_LOGI(TAG, "Initialisation des capteurs...");
  init_deep_sleep_pin();

  init_led_pin();

  init_adc();

  init_buttons();

  init_dip_switches();
  read_dip_switch();

  init_temperature();

  ESP_LOGI(TAG, "Sensors initialized");
}

/* =========================
 * READ FUNCTIONS
 * ========================= */

float read_temperature(void) {
  ESP_ERROR_CHECK(ds18b20_trigger_temperature_conversion(ds18b20));
  // vTaskDelay(pdMS_TO_TICKS(800));

  float temperature;
  ESP_ERROR_CHECK(ds18b20_get_temperature(ds18b20, &temperature));

  ESP_LOGI(TAG, "Température: %.2f °C", temperature);

  return temperature;
}

bool is_deep_sleep_enabled(void) {
  return gpio_get_level(DISABLE_DEEP_SLEEP_PIN) == 1;
}

bool is_reset_ble_pressed(void) { return gpio_get_level(RESET_BLE_PIN) == 0; }

bool is_tare_scale_pressed(void) { return gpio_get_level(TARE_SCALE_PIN) == 0; }

float read_voltage(void) {
  gpio_set_level(POWER_ADC_PIN, 1);
  vTaskDelay(pdMS_TO_TICKS(200));

  int raw;
  int mv;

  ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL, &raw));
  ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali_handle, raw, &mv));

  int voltage = raw * (3.3 / 4095.0) * 1000;  // Convertir en tension ADC
  int batteryVoltage =
      voltage / (680.0 / (100.0 + 680.0));  // Ajustement avec le pont diviseur
  ESP_LOGI(TAG, "RAW=%d  Voltage=%d mV RealVoltage=%d mV batteryVoltage=%d mV",
           raw, mv, voltage, batteryVoltage);

  gpio_set_level(POWER_ADC_PIN, 0);
  return mv;
}

uint8_t read_dip_switch(void) {
  uint8_t id = 0;
  for (int i = 0; i < DIP_SWITCH_PINS_COUNT; i++) {
    if (gpio_get_level(DIP_SWITCH_PINS[i]) == 0) {
      id |= (1 << i);
    }
  }
  UID = id + 1;
  return UID;
}

void disable_input(gpio_num_t gpio) { gpio_reset_pin(gpio); }

void disable_inputs(void) {
  for (int i = 0; i < DIP_SWITCH_PINS_COUNT; i++) {
    disable_input(DIP_SWITCH_PINS[i]);
  }
  disable_input(RESET_BLE_PIN);
  disable_input(TARE_SCALE_PIN);
  disable_input(DISABLE_DEEP_SLEEP_PIN);
}

uint8_t get_UID(void) { return UID; }
