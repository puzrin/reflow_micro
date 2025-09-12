#pragma once

#include <stdint.h>
#include <vector>

class EepromStore {
public:
    static constexpr std::size_t MAX_SIZE = 256; // 24C02
    static constexpr std::size_t PAGE_SIZE = 8; // safe value

    bool read(std::vector<uint8_t>& data);
    bool write(const std::vector<uint8_t>& data);

private:
    bool ee_read_at(uint16_t addr, uint8_t* buf, size_t len);
    bool ee_write_at(uint16_t addr, const uint8_t* buf, size_t len);
};
