#ifndef ESPNOW_H
#define ESPNOW_H

#include <stdbool.h>
#include <stdio.h>

#include "esp_now.h"

#define WIFI_CHANNEL 1
#define NB_SLOTS 16
#define SLOT_MS 500
#define NODE_ID 5                     // UNIQUE par client
#define MAX_WAIT_MS (30 * 60 * 1000)  // 30 min pour première synchro
#define NORMAL_CHANNEL 1
#define SYNC_LISTEN_MS 3000

typedef struct {
  uint8_t uid;
  float weight;
  float temperature;
  float battery;
} client_data_t;

typedef struct {
  uint32_t uid;  // destinataire
  uint8_t type;  // 1=PAIR , 2=SYNC
  int64_t sleep_us;
} server_msg_t;

void wifi_init(void);
void espnow_init(void);
void on_data_recv(const esp_now_recv_info_t* info, const uint8_t* data,
                  int len);
void on_data_sent(const esp_now_send_info_t* info,
                  esp_now_send_status_t status);
void wait_sync(const int64_t* sleep_us_ptr);
void send_data_tdma(client_data_t* msg);
/*bool is_registered_in_espnow(void);
bool register_in_espnow(void);*/
#endif  // ESPNOW_H