#include <string.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_now.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "espnow.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "memory.h"
#include "nvs_flash.h"
#include "scale.h"
#include "sensors.h"

static const char* TAG = "HIVE_CLIENT";
static int64_t sleep_us = -1;

/* =========================
 * MAIN
 * ========================= */

void app_main(void) {
  init_sensors();

  nvs_init();
  wifi_init();
  espnow_init();
  init_scale();

  if (is_tare_scale_pressed()) {
    ESP_LOGI(TAG, "Tare scale pressed");
    tare_scale();
  } else {
    ESP_LOGI(TAG, "Tare scale not pressed");
  }
  /*bool registered = is_registered_in_espnow();
  bool force_pair = is_reset_ble_pressed();
  if (!registered) {
    ESP_LOGI(TAG, "Pas encore enregistré en ESPNOW, enregistrement...");
  } else if (force_pair) {
    ESP_LOGI(
        TAG,
        "Bouton de reset pressé, forçant le réenregistrement en ESPNOW...");
  }*/

  wait_sync(&sleep_us);

  client_data_t msg = {.uid = get_UID(),
                       .weight = read_weight(),
                       .temperature = read_temperature(),
                       .battery = read_voltage()};

  send_data_tdma(&msg);
  disable_inputs();
  esp_sleep_enable_timer_wakeup(sleep_us);
  esp_deep_sleep_start();
}
