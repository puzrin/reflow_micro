#include <array>
#include <string>

auto bin2hex(const uint8_t* data, size_t length) -> std::string;
void hex2bin(const std::string& hex, uint8_t* out, size_t length);
auto hmac_sha256(const std::array<uint8_t, 32>& message, const std::array<uint8_t, 32>& key) -> std::array<uint8_t, 32>;
auto get_own_mac() -> std::array<uint8_t, 6>;
auto create_secret() -> std::array<uint8_t, 32>;
