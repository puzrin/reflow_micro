#include <string>
#include <array>
#include <mbedtls/md.h>
#include "esp_system.h"

auto hmac_sha256(const std::array<uint8_t, 32>& message, const std::array<uint8_t, 32>& key) -> std::array<uint8_t, 32> {
    std::array<uint8_t, 32> output = {0};

    mbedtls_md_context_t ctx;
    const mbedtls_md_info_t *info;

    mbedtls_md_init(&ctx);
    info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    mbedtls_md_setup(&ctx, info, 1);

    if (mbedtls_md_get_size(info) != output.size()) {
        mbedtls_md_free(&ctx);
        return output;
    }

    mbedtls_md_hmac_starts(&ctx, key.data(), key.size());
    mbedtls_md_hmac_update(&ctx, message.data(), message.size());
    mbedtls_md_hmac_finish(&ctx, output.data());

    mbedtls_md_free(&ctx);
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
