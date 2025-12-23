#pragma once

#include <stddef.h>
#include <stdint.h>
#include <etl/vector.h>

class EepromStore {
public:
    static constexpr size_t MAX_SIZE = 256; // 24C02
    static constexpr size_t PAGE_SIZE = 8; // safe value

    bool read(etl::ivector<uint8_t>& data);
    bool write(const etl::ivector<uint8_t>& data);
    bool probe();

private:
    bool ee_read_at(uint16_t addr, uint8_t* buf, size_t len);
    bool ee_write_at(uint16_t addr, const uint8_t* buf, size_t len);
};
