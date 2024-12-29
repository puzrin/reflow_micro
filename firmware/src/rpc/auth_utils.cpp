#include <string>
#include <array>
#include <mbedtls/md.h>
#include "esp_system.h"

auto bin2hex(const uint8_t* data, size_t length) -> std::string {
    const char* hex_chars = "0123456789ABCDEF";
    std::string hex;

    for (size_t i = 0; i < length; ++i) {
        hex += hex_chars[(data[i] >> 4) & 0xF];
        hex += hex_chars[data[i] & 0xF];
    }

    return hex;
}

auto hexchar2num(char c) -> uint8_t {
    if (c >= '0' && c <= '9') { return static_cast<uint8_t>(c - '0'); }
    if (c >= 'A' && c <= 'F') { return static_cast<uint8_t>(c - 'A' + 10); }
    if (c >= 'a' && c <= 'f') { return static_cast<uint8_t>(c - 'a' + 10); }
    return 0;
}

void hex2bin(const std::string& hex, uint8_t* out, size_t length) {
    size_t out_index = 0;
    uint8_t value = 0;

    for (size_t i = 0; i < hex.length() && out_index < length; ++i) {
        value = static_cast<uint8_t>(value << 4 | hexchar2num(hex[i]));
        if (i % 2 != 0) {
            out[out_index++] = value;
            value = 0;
        }
    }
}

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
