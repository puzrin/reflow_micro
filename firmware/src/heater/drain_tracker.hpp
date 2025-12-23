#pragma once

#include <stdint.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

class DrainTracker {
public:
    struct DRAIN_INFO {
        uint32_t peak_mv = 0;
        uint32_t peak_ma = 0;
        bool load_valid = false;
    };

    void setup();
    void collect_data();
    void process_collected_data();
    void clear_collected_data();
    void reset();

    DRAIN_INFO get_info() const;

private:
    static constexpr uint32_t ADC_FILTER_SIZE = 8;
    struct ADC_ITEM { uint16_t v_raw; int16_t i_raw; };

    static constexpr uint8_t INA226_ADDR = 0x40;

    bool ina226_init();
    bool ina226_read_reg16(uint8_t reg, uint16_t &data);
    bool ina226_write_reg16(uint8_t reg, uint16_t data);

    ADC_ITEM adc_buffer[ADC_FILTER_SIZE]{};
    uint32_t adc_count{0};

    mutable SemaphoreHandle_t info_lock{xSemaphoreCreateMutex()};
    DRAIN_INFO info{};
};

extern DrainTracker drain_tracker;
