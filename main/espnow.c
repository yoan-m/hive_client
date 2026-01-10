#include "espnow.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_netif.h"
#include "esp_now.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "sensors.h"

static int64_t sleep_us = -1;

static const char* TAG = "ESPNOW";
#define NVS_NAMESPACE "espnow"

void wifi_init(void) {
  ESP_ERROR_CHECK(esp_netif_init());
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_start());

  esp_wifi_set_ps(WIFI_PS_NONE);
  esp_wifi_set_channel(NORMAL_CHANNEL, WIFI_SECOND_CHAN_NONE);

  uint8_t mac[6];
  esp_wifi_get_mac(WIFI_IF_STA, mac);
  ESP_LOGI(TAG, "MAC %02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2],
           mac[3], mac[4], mac[5]);
}

void on_data_sent(const esp_now_send_info_t* info,
                  esp_now_send_status_t status) {
  ESP_LOGI(TAG, "ESP-NOW send: %s",
           status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL");
}

void on_data_recv(const esp_now_recv_info_t* info, const uint8_t* data,
                  int len) {
  if (len != sizeof(server_msg_t)) return;

  server_msg_t msg;
  memcpy(&msg, data, sizeof(msg));

  if (msg.uid != get_UID() && msg.uid != 0xFFFFFFFF) return;
  /*if (msg.type != 1) {
    set_registered_in_espnow(true);
  }*/

  sleep_us = msg.sleep_us;
}

void espnow_init() {
  ESP_ERROR_CHECK(esp_now_init());
  ESP_ERROR_CHECK(esp_now_register_send_cb(on_data_sent));
  ESP_ERROR_CHECK(esp_now_register_recv_cb(on_data_recv));

  /* broadcast peer */
  esp_now_peer_info_t bcast = {
      .peer_addr = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
      .channel = NORMAL_CHANNEL,
      .ifidx = WIFI_IF_STA,
      .encrypt = false};
  ESP_ERROR_CHECK(esp_now_add_peer(&bcast));
}

void wait_sync(const int64_t* sleep_us_ptr) {
  sleep_us = *sleep_us_ptr;
  int64_t start = esp_timer_get_time();
  while (sleep_us < 0 && (esp_timer_get_time() - start) < MAX_WAIT_MS * 1000) {
    vTaskDelay(pdMS_TO_TICKS(50));
  }

  if (sleep_us < 0) sleep_us = 30LL * 60 * 1000000;
}

void send_data_tdma(client_data_t* msg) {
  int slot = get_UID() % NB_SLOTS;
  vTaskDelay(pdMS_TO_TICKS(slot * SLOT_MS));
  esp_now_send(NULL, (uint8_t*)msg, sizeof(*msg));
}

/*bool is_registered_in_espnow(void) {
  nvs_handle_t nvs;
  esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs);
  if (err != ESP_OK) return false;
  int8_t paired;
  err = nvs_get_i8(nvs, "paired", &paired);
  if (err != ESP_OK) {
    nvs_close(nvs);
    return false;
  }
  return paired != 0;
}

void set_registered_in_espnow(bool paired) {
  nvs_handle_t nvs;
  ESP_ERROR_CHECK(nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs));
  ESP_ERROR_CHECK(nvs_set_i8(nvs, "paired", paired ? 1 : 0));
  ESP_ERROR_CHECK(nvs_commit(nvs));
  nvs_close(nvs);
}*/