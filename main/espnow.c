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

static volatile bool sync_received = false;
static int64_t sleep_us = FALLBACK_SLEEP_US;
static const char* TAG = "ESPNOW";
#define NVS_NAMESPACE "espnow"
static uint8_t server_mac[ESP_NOW_ETH_ALEN] = {0x7C, 0x2C, 0x67,
                                               0xC7, 0x91, 0x00};
uint8_t broadcast_mac[ESP_NOW_ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void init_wifi(void) {
  ESP_LOGI(TAG, "Initialisation WiFi...");
  ESP_ERROR_CHECK(nvs_flash_init());
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
  ESP_ERROR_CHECK(esp_wifi_set_channel(NORMAL_CHANNEL, WIFI_SECOND_CHAN_NONE));

  uint8_t mac[6];
  esp_wifi_get_mac(WIFI_IF_STA, mac);
  ESP_LOGI(TAG, "MAC %02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2],
           mac[3], mac[4], mac[5]);
}

void on_data_sent(const esp_now_send_info_t* info,
                  esp_now_send_status_t status) {
  ESP_LOGI(TAG, "Message envoyé: %s",
           status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL");
}

void on_data_recv(const esp_now_recv_info_t* info, const uint8_t* data,
                  int len) {
  ESP_LOGI(TAG, "Message reçu");
  if (data[0] != MSG_SYNC) return;
  memcpy(server_mac, info->src_addr, ESP_NOW_ETH_ALEN);
  ESP_LOGI(TAG, "Serveur MAC %02X:%02X:%02X:%02X:%02X:%02X", server_mac[0],
           server_mac[1], server_mac[2], server_mac[3], server_mac[4],
           server_mac[5]);

  esp_now_peer_info_t peer = {0};

  memcpy(peer.peer_addr, server_mac, ESP_NOW_ETH_ALEN);
  peer.channel = NORMAL_CHANNEL;
  peer.ifidx = WIFI_IF_STA;
  peer.encrypt = false;

  ESP_ERROR_CHECK(esp_now_add_peer(&peer));
  ESP_LOGI(TAG, "Sync ok");
  sync_msg_t msg;
  memcpy(&msg, data, sizeof(msg));

  sleep_us = msg.cycle_us;
  sync_received = true;
}

void init_espnow(int64_t* sleep_us_ptr) {
  sleep_us = *sleep_us_ptr;
  ESP_LOGI(TAG, "Initialisation ESP-NOW");
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

void wait_sync() {
  ESP_LOGI(TAG, "Attente de synchronisation...");
  int64_t start = esp_timer_get_time();
  while (!sync_received &&
         (esp_timer_get_time() - start) < SYNC_TIMEOUT_MS * 1000) {
    vTaskDelay(pdMS_TO_TICKS(20));
  }

  int slot = get_UID() % NB_SLOTS;
  vTaskDelay(pdMS_TO_TICKS(slot * SLOT_MS));
}

void send_data_tdma(client_msg_t* msg) {
  ESP_LOGI(TAG, "Envoi des données en TDMA...");

  // --- Vérifie que le serveur est enregistré comme peer ---
  if (!esp_now_is_peer_exist(server_mac)) {
    esp_now_peer_info_t peer = {0};
    memcpy(peer.peer_addr, server_mac, ESP_NOW_ETH_ALEN);
    peer.channel = NORMAL_CHANNEL;
    peer.ifidx = WIFI_IF_STA;
    peer.encrypt = false;

    esp_err_t add_err = esp_now_add_peer(&peer);
    if (add_err == ESP_OK) {
      ESP_LOGI(TAG, "Peer serveur ajouté avec succès");
    } else if (add_err == ESP_ERR_ESPNOW_EXIST) {
      ESP_LOGI(TAG, "Peer serveur existait déjà");
    } else {
      ESP_LOGE(TAG, "Erreur ajout peer serveur: %s", esp_err_to_name(add_err));
    }

    // Pause courte pour que le peer soit actif (esp32-C3)
    vTaskDelay(pdMS_TO_TICKS(20));
  }

  // --- Envoi broadcast pour prévenir les autres clients ---
  sync_msg_t sync = {.type = MSG_SYNC, .cycle_us = 42};
  esp_err_t err = esp_now_send(broadcast_mac, (uint8_t*)&sync, sizeof(sync));
  ESP_LOGI(TAG, "Broadcast esp_now_send returned: %s", esp_err_to_name(err));

  // --- Pause pour laisser le broadcast passer ---
  vTaskDelay(pdMS_TO_TICKS(50));

  // --- Envoi unicast au serveur ---
  esp_err_t err2 = esp_now_send(server_mac, (uint8_t*)&sync, sizeof(sync));
  ESP_LOGI(TAG, "Unicast serveur esp_now_send returned: %s",
           esp_err_to_name(err2));
}
