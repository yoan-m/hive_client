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
TimerHandle_t sleep_timer;
TaskHandle_t main_task_handle = NULL;
static int64_t sleep_us = FALLBACK_SLEEP_US;

void timer_callback(TimerHandle_t xTimer) {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  vTaskNotifyGiveFromISR(main_task_handle, &xHigherPriorityTaskWoken);
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/* =========================
 * MAIN
 * ========================= */

void app_main(void) {
  main_task_handle = xTaskGetCurrentTaskHandle();
  esp_log_level_set("*", ESP_LOG_WARN);
  esp_log_level_set(TAG, ESP_LOG_INFO);

  init_nvs();

  init_sensors();

  init_scale();

  if (is_tare_scale_pressed()) {
    ESP_LOGI(TAG, "Tare scale pressed");
    tare_scale();
  } else {
    ESP_LOGI(TAG, "Tare scale not pressed");
  }

  uint8_t uid = get_UID();
  uint16_t battery_mv = read_voltage();
  float temp = read_temperature();
  float weight = read_weight();
  is_deep_sleep_enabled();
  is_tare_scale_pressed();
  disable_inputs();

  ESP_LOGI(TAG, "🚀 Démarrage serveur ESP-NOW...");
  init_wifi();
  vTaskDelay(pdMS_TO_TICKS(100));
  init_espnow();
  vTaskDelay(pdMS_TO_TICKS(500));
  ESP_LOGI(TAG, "✅ Serveur prêt à recevoir les messages !");

  wait_sync();
  send_data_tdma(uid, weight * 10, temp * 10, battery_mv);

  TimerHandle_t sleep_timer = xTimerCreate(
      "sleep_timer", pdMS_TO_TICKS(SLOT_MS), pdFALSE, NULL, timer_callback);

  xTimerStart(sleep_timer, 0);

  // Attente notification
  ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

  if (is_deep_sleep_enabled()) {
    ESP_LOGI(TAG, "Dodo pour %lld us", sleep_us);

    esp_now_deinit();
    esp_wifi_stop();

    esp_sleep_enable_timer_wakeup(sleep_us);
    esp_deep_sleep_start();
  } else {
    ESP_LOGI(TAG, "Deep sleep désactivé");
  }
}
