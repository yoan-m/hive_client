#include "scale.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gpioconfig.h"
#include "hx711.h"
#include "nvs.h"

static const char* TAG = "SCALE";
#define NVS_NAMESPACE "scale"

static hx711_t scale = {
    .dout = SDA_PIN,
    .pd_sck = SCL_PIN,
    .gain = HX711_GAIN_A_128,  // canal A gain 128
};

int32_t offset;
float scale_factor;

void init_scale(void) {
  esp_log_level_set(TAG, ESP_LOG_INFO);
  ESP_LOGI(TAG, "Initialisation balance HX711");
  return;
  ESP_ERROR_CHECK(hx711_init(&scale));
  bool ok = load_calibration(&offset, &scale_factor);
  if (!ok || is_tare_scale_pressed()) {
    if (!ok) {
      ESP_LOGW(TAG, "Pas de calibration, lancement calibration manuelle");
    } else {
      ESP_LOGI(TAG, "Calibration existante, mais tare demandé");
    }
    tare_scale();
  }

  ESP_LOGI(TAG, "HX711 initialisé");
}

float read_weight(void) {
  int32_t raw = 0;
  esp_err_t err;

  // Vérifier que le capteur est prêt
  if (!is_scale_ready()) {
    ESP_LOGW(TAG, "HX711 non prêt");
    return 0.0f;
  }

  err = hx711_read_average(&scale, 10, &raw);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Erreur lecture HX711");
    return 0.0f;
  }

  if (scale_factor == 0.0f) {
    ESP_LOGE(TAG, "scale_factor invalide (0)");
    return 0.0f;
  }

  float poids = ((float)(raw - offset)) / scale_factor;

  ESP_LOGI(TAG, "Raw: %ld | Poids: %.3f kg", raw, poids);

  return poids;
}

bool is_scale_ready(void) { return (hx711_wait(&scale, 1000) == ESP_OK); }

void tare_scale(void) {
  // ---- TARE (zéro) ----
  int32_t offset;
  hx711_read_average(&scale, 20, &offset);
  ESP_LOGI(TAG, "Offset (à vide) : %ld", offset);

  vTaskDelay(pdMS_TO_TICKS(5000));
  // ---- Pause manuelle ----
  wait_for_button();

  // ---- Mesure du poids connu ----
  int32_t raw_1kg;
  hx711_read_average(&scale, 20, &raw_1kg);
  ESP_LOGI(TAG, "Raw avec 1kg : %ld", raw_1kg);

  // ---- Calcul facteur d'échelle ----
  float scale_factor = (float)(raw_1kg - offset) / 1.0;
  ESP_LOGI(TAG, "Facteur d'échelle : %f par kg", scale_factor);

  // Save calibration

  save_calibration(offset, scale_factor);

  // ---- Lecture continue ----
  for (int i = 0; i < 10; i++) {
    read_weight();
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void wait_for_button(void) {
  ESP_LOGI(TAG, "Mettre un poids de 1kg sur la balance.");
  ESP_LOGI(TAG, "Appuyez sur le bouton pour continuer...");
  while (gpio_get_level(TARE_SCALE_PIN) ==
         1) {  // pull-up = 1 tant que pas appuyé
    vTaskDelay(pdMS_TO_TICKS(50));
  }
  vTaskDelay(pdMS_TO_TICKS(200));  // debounce
}

void save_calibration(int32_t offset, float scale) {
  nvs_handle_t nvs;
  ESP_ERROR_CHECK(nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs));

  ESP_ERROR_CHECK(nvs_set_i32(nvs, "offset", offset));
  ESP_ERROR_CHECK(nvs_set_blob(nvs, "scale", &scale, sizeof(scale)));

  ESP_ERROR_CHECK(nvs_commit(nvs));
  nvs_close(nvs);

  ESP_LOGI("NVS", "Calibration sauvegardée");
}

bool load_calibration(int32_t* offset, float* scale) {
  nvs_handle_t nvs;
  esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs);
  if (err != ESP_OK) return false;

  err = nvs_get_i32(nvs, "offset", offset);
  if (err != ESP_OK) {
    nvs_close(nvs);
    return false;
  }

  size_t len = sizeof(float);
  err = nvs_get_blob(nvs, "scale", scale, &len);
  nvs_close(nvs);

  if (err != ESP_OK || len != sizeof(float)) return false;

  ESP_LOGI("NVS", "Calibration chargée: offset=%ld scale=%f", *offset, *scale);
  return true;
}