#include "prefs.hpp"
#include <nvs_flash.h>
#include <nvs.h>
#include "logger.hpp"

bool AsyncPreferenceKV::write(const std::string& ns, const std::string& key, uint8_t* buffer, size_t length) {
    static bool nvs_initialized = false;

    if (!nvs_initialized) {
        esp_err_t ret = nvs_flash_init();
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
            DEBUG("NVS has no free pages, erasing storage");
            nvs_flash_erase();
            ret = nvs_flash_init();
        } else if (ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            DEBUG("NVS version mismatch, erasing storage");
            nvs_flash_erase();
            ret = nvs_flash_init();
        }
        if (ret != ESP_OK) {
            DEBUG("NVS initialization failed: {}", esp_err_to_name(ret));
            return false;
        }
        nvs_initialized = true;
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open(ns.c_str(), NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        DEBUG("Failed to open namespace '{}': {}", ns, esp_err_to_name(err));
        return false;
    }

    err = nvs_set_blob(handle, key.c_str(), buffer, length);
    if (err != ESP_OK) {
        DEBUG("Failed to write key '{}': {}", key, esp_err_to_name(err));
        nvs_close(handle);
        return false;
    }

    err = nvs_commit(handle);
    nvs_close(handle);
    if (err != ESP_OK) {
        DEBUG("Failed to commit NVS changes: {}", esp_err_to_name(err));
        return false;
    }

    return true;
}

bool AsyncPreferenceKV::read(const std::string& ns, const std::string& key, uint8_t* buffer, size_t max_length) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(ns.c_str(), NVS_READONLY, &handle);
    if (err != ESP_OK) { return false; }

    size_t required_size = max_length;
    err = nvs_get_blob(handle, key.c_str(), buffer, &required_size);
    nvs_close(handle);

    return (err == ESP_OK);
}

size_t AsyncPreferenceKV::length(const std::string& ns, const std::string& key) {
    nvs_handle_t handle;
    size_t len = 0;
    esp_err_t err = nvs_open(ns.c_str(), NVS_READONLY, &handle);
    if (err == ESP_OK) {
        err = nvs_get_blob(handle, key.c_str(), nullptr, &len);
        nvs_close(handle);
    }

    return (err == ESP_OK) ? len : 0;
}
