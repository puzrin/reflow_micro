#include <string>
#include <array>
#include <psa/crypto.h>
#include "esp_random.h"
#include "esp_mac.h"

auto hmac_sha256(const std::array<uint8_t, 32>& message, const std::array<uint8_t, 32>& key) -> std::array<uint8_t, 32> {
    std::array<uint8_t, 32> output = {0};

    constexpr auto algorithm = PSA_ALG_HMAC(PSA_ALG_SHA_256);
    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_set_key_type(&attributes, PSA_KEY_TYPE_HMAC);
    psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_SIGN_MESSAGE);
    psa_set_key_algorithm(&attributes, algorithm);

    mbedtls_svc_key_id_t key_id{};
    auto status = psa_import_key(&attributes, key.data(), key.size(), &key_id);
    psa_reset_key_attributes(&attributes);
    if (status != PSA_SUCCESS) { return output; }

    size_t output_length = 0;
    status = psa_mac_compute(key_id, algorithm, message.data(), message.size(),
                             output.data(), output.size(), &output_length);
    auto destroy_status = psa_destroy_key(key_id);

    if (status != PSA_SUCCESS || destroy_status != PSA_SUCCESS || output_length != output.size()) {
        output.fill(0);
    }
    return output;
}

auto get_own_mac() -> std::array<uint8_t, 6> {
    std::array<uint8_t, 6> mac;
    //esp_read_mac(mac.data(), ESP_MAC_WIFI_STA);
    esp_efuse_mac_get_default(mac.data());
    return mac;
}

auto create_secret() -> std::array<uint8_t, 32> {
    std::array<uint8_t, 32> secret;

    for (size_t i = 0; i < secret.size(); i += 4) {
        auto random4b = esp_random();
        std::copy(reinterpret_cast<uint8_t*>(&random4b),
            reinterpret_cast<uint8_t*>(&random4b) + 4,
            secret.begin() + i);
    }

    return secret;
}
