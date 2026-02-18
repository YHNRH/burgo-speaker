#include "wake_word.h"
#include "esp_log.h"
#include "esp_wn_iface.h"
// #include "hilexin.h"
// #include "hiesp.h"
#include "esp_wn_models.h"   // содержит esp_wn_handle_from_name

#include "esp_afe_sr_iface.h"
#include "esp_afe_sr_models.h"

#include "model_path.h"

static const char *TAG = "WAKE_WORD";
static const esp_wn_iface_t *wakenet = NULL;
static model_iface_data_t *model_data = NULL;

static esp_afe_sr_data_t *afe_data = NULL;
static esp_afe_sr_iface_t *afe_handle = NULL;

static int chunk_size = 0;

esp_err_t wake_word_init(const char *model_name) {

    srmodel_list_t *models = esp_srmodel_init("model");
    char *sys_model_name = esp_srmodel_filter(models, ESP_WN_PREFIX, model_name);

    // 1. Получаем интерфейс модели по имени
    wakenet = esp_wn_handle_from_name(sys_model_name);
    if (!wakenet) {
        ESP_LOGE(TAG, "Model '%s' not found!", sys_model_name);
        return ESP_FAIL;
    }

    // 2. Создаём экземпляр модели с порогом обнаружения
    model_data = wakenet->create(sys_model_name, DET_MODE_90);
    if (!model_data) {
        ESP_LOGE(TAG, "Failed to create model instance");
        return ESP_FAIL;
    }

    chunk_size = wakenet->get_samp_chunksize(model_data);
    ESP_LOGI(TAG, "Wake word initialized. Model: %s, chunk size: %d", sys_model_name, chunk_size);
    return ESP_OK;
}

bool wake_word_process(int16_t *samples, size_t sample_count) {
    if (!model_data || !wakenet) {
        return false;
    }
    if (sample_count != chunk_size) {
        ESP_LOGW(TAG, "Sample count mismatch: expected %d, got %d", chunk_size, sample_count);
        return false;
    }
    int state = wakenet->detect(model_data, samples);
    return (state == WAKENET_DETECTED);
}

int wake_word_get_chunk_size(void) {
    return chunk_size;
}