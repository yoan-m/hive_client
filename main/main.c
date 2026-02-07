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
static int64_t sleep_us = FALLBACK_SLEEP_US;

/* =========================
 * MAIN
 * ========================= */

void app_main(void) {
  init_sensors();

  init_nvs();
  nvs_flash_init();
  init_wifi();
  init_espnow(&sleep_us);
  init_scale();

  if (is_tare_scale_pressed()) {
    ESP_LOGI(TAG, "Tare scale pressed");
    tare_scale();
  } else {
    ESP_LOGI(TAG, "Tare scale not pressed");
  }

  client_msg_t msg = {.uid = get_UID(),
                      .weight = read_weight(),
                      .temperature = read_temperature(),
                      .battery = read_voltage()};

  // wait_sync();
  vTaskDelay(pdMS_TO_TICKS(1000));
  send_data_tdma(&msg);
  while (true) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
  /*if (is_deep_sleep_enabled()) {
    ESP_LOGI(TAG, "Dodo pour %lld us", sleep_us);

    disable_inputs();
    esp_sleep_enable_timer_wakeup(sleep_us);
    esp_deep_sleep_start();
  } else {
    ESP_LOGI(TAG, "Deep sleep désactivé");
  }*/
}
