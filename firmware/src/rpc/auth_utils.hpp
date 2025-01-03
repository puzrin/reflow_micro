#include <array>
#include <string>

auto hmac_sha256(const std::array<uint8_t, 32>& message, const std::array<uint8_t, 32>& key) -> std::array<uint8_t, 32>;
auto get_own_mac() -> std::array<uint8_t, 6>;
auto create_secret() -> std::array<uint8_t, 32>;
