#include <array>
#include <string>

std::string bin2hex(const uint8_t* data, size_t length);
void hex2bin(const std::string& hex, uint8_t* out, size_t length);
std::array<uint8_t, 32> hmac_sha256(const std::array<uint8_t, 32>& message, const std::array<uint8_t, 32>& key);
std::array<uint8_t, 6> get_own_mac();
std::array<uint8_t, 32> create_secret();
