#include <esp_crc.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "eeprom_store.hpp"
#include "components/i2c_io.hpp"

namespace {
    constexpr uint16_t MAGIC = 0x42DA;
    constexpr uint8_t EEPROM_I2C_ADDR = 0x50; // 24C02 default address

    struct __attribute__((packed)) Header {
        uint16_t magic;
        uint16_t size;
        uint32_t crc32;
    };

    static_assert(sizeof(Header) == 8, "Header must be exactly 8 bytes");
}

bool EepromStore::read(etl::ivector<uint8_t>& data) {
    // Read header
    Header header;
    if (!ee_read_at(0, reinterpret_cast<uint8_t*>(&header), sizeof(header))) {
        return false; // Hardware error
    }

    // Validate magic
    if (header.magic != MAGIC) {
        data.clear(); // EEPROM is clean, return zero size vector
        return true;
    }

    // Sanity check size
    if (header.size > MAX_SIZE - sizeof(Header)) {
        data.clear(); // Invalid data, return zero size vector
        return true;
    }

    // Read data
    data.resize(header.size);
    if (header.size > 0) {
        if (!ee_read_at(sizeof(Header), data.data(), header.size)) {
            return false; // Hardware error
        }
    }

    // Verify CRC32
    uint32_t calc_crc = esp_crc32_le(0, data.data(), header.size);
    if (calc_crc != header.crc32) {
        data.clear(); // Data corrupted
        return true;
    }

    return true;
}

bool EepromStore::write(const etl::ivector<uint8_t>& data) {
    // Check size constraints
    if (data.size() > MAX_SIZE - sizeof(Header)) {
        return false;
    }

    // Prepare header
    Header header;
    header.magic = MAGIC;
    header.size = static_cast<uint16_t>(data.size());
    header.crc32 = esp_crc32_le(0, data.data(), data.size());

    // Write header
    if (!ee_write_at(0, reinterpret_cast<const uint8_t*>(&header), sizeof(header))) {
        return false;
    }

    // Write data
    if (data.size() > 0) {
        if (!ee_write_at(sizeof(Header), data.data(), data.size())) {
            return false;
        }
    }

    return true;
}

bool EepromStore::probe() {
    uint8_t dummy = 0;
    return ee_read_at(0, &dummy, 1);
}

bool EepromStore::ee_read_at(uint16_t addr, uint8_t* buf, size_t len) {
    size_t bytes_left = len;
    uint16_t current_addr = addr;
    uint8_t* current_buf = buf;

    while (bytes_left > 0) {
        size_t chunk_size = (bytes_left > PAGE_SIZE) ? PAGE_SIZE : bytes_left;

        if (!i2c_read_block(EEPROM_I2C_ADDR, current_addr, current_buf, chunk_size)) {
            return false;
        }

        current_addr += chunk_size;
        current_buf += chunk_size;
        bytes_left -= chunk_size;
    }

    return true;
}

bool EepromStore::ee_write_at(uint16_t addr, const uint8_t* buf, size_t len) {
    size_t bytes_left = len;
    uint16_t current_addr = addr;
    const uint8_t* current_buf = buf;

    while (bytes_left > 0) {
        // Calculate how many bytes we can write until end of current page
        size_t bytes_until_page_end = PAGE_SIZE - (current_addr % PAGE_SIZE);
        size_t chunk_size = (bytes_left < bytes_until_page_end) ? bytes_left : bytes_until_page_end;

        if (!i2c_write_block(EEPROM_I2C_ADDR, current_addr, current_buf, chunk_size)) {
            return false;
        }

        current_addr += chunk_size;
        current_buf += chunk_size;
        bytes_left -= chunk_size;

        vTaskDelay(pdMS_TO_TICKS(10)); // EEPROM write delay
    }

    return true;
}
