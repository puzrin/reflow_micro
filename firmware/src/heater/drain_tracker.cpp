#include "drain_tracker.hpp"

#include "components/i2c_io.hpp"
#include "logger.hpp"

DrainTracker drain_tracker;

void DrainTracker::setup() {
    i2c_init();
    if (!ina226_init()) {
        APP_LOGE("DrainTracker: INA226 init failed");
    }
}

void DrainTracker::collect_data() {
    uint16_t adc_v_raw;
    uint16_t adc_i_raw;

    if (ina226_read_reg16(0x02, adc_v_raw) &&
        ina226_read_reg16(0x04, adc_i_raw))
    {
        adc_buffer[adc_count % ADC_FILTER_SIZE] = {
            .v_raw = adc_v_raw,
            .i_raw = static_cast<int16_t>(adc_i_raw)
        };
        adc_count++;
    }
}

void DrainTracker::process_collected_data() {
    if (adc_count == 0) {
        return;
    }

    uint32_t count = adc_count < ADC_FILTER_SIZE ? adc_count : ADC_FILTER_SIZE;
    uint32_t v_sum{0};
    int32_t i_sum{0};

    for (uint32_t i = 0; i < count; i++) {
        v_sum += adc_buffer[i].v_raw;
        i_sum += adc_buffer[i].i_raw;
    }

    if (i_sum < 0) { i_sum = 0; }
    // Vbus = V_raw * 1.25 LSB (per datasheet)
    uint32_t peak_mv = ((v_sum / count) * 5 + 2) / 4; // x1.25, rounded
    // Shunt current already in mA
    uint32_t peak_ma = static_cast<uint32_t>(i_sum / static_cast<int32_t>(count));
    // Thresholds for real measurements, not PD limits.
    bool load_valid = (peak_ma >= 300 && peak_mv >= 4000);

    xSemaphoreTake(info_lock, portMAX_DELAY);
    info.peak_mv = peak_mv;
    info.peak_ma = peak_ma;
    info.load_valid = load_valid;
    xSemaphoreGive(info_lock);

    adc_count = 0;
}

void DrainTracker::clear_collected_data() {
    adc_count = 0;
}

void DrainTracker::reset() {
    clear_collected_data();

    xSemaphoreTake(info_lock, portMAX_DELAY);
    info = DRAIN_INFO{};
    xSemaphoreGive(info_lock);
}

DrainTracker::DRAIN_INFO DrainTracker::get_info() const {
    DRAIN_INFO copy;
    xSemaphoreTake(info_lock, portMAX_DELAY);
    copy = info;
    xSemaphoreGive(info_lock);
    return copy;
}

bool DrainTracker::ina226_init() {
    // CONFIG (0x00) = 0x0207
    // - AVG=001 (x2)
    // - VBUSCT=000 (140 us)
    // - VSHCT=000 (140 us),
    // - MODE=111 (Shunt+Bus, Continuous)
    //
    // ADC runs independently of PWM; continuous mode with ~560 us per full cycle
    // (2-sample average) keeps conversions faster than our ~1 ms polling tick,
    // so reads are fresh.
    if (!ina226_write_reg16(0x00, 0x0207)) {
        APP_LOGE("INA226: Failed to write CONFIG");
        return false;
    }

    // CALIBRATION (0x05) = 0x0200
    // Current_LSB = 1 mA & Rshunt=10 mOhm:
    // Cal = 0.00512 / (Current_LSB[A] * Rshunt[ohm])
    //     = 0.00512 / (0.001 * 0.01) = 512 = 0x0200.
    if (!ina226_write_reg16(0x05, 0x0200)) {
        APP_LOGE("INA226: Failed to write CALIBRATION");
        return false;
    }

    uint16_t id;

    if (!ina226_read_reg16(0xFE, id)) {
        APP_LOGE("INA226: Failed to read Manufacturer ID");
        return false;
    }
    if (id != 0x5449) {
        APP_LOGE("INA226: Invalid Manufacturer ID 0x{:04X}", id);
        return false;
    }

    if (!ina226_read_reg16(0xFF, id)) {
        APP_LOGE("INA226: Failed to read Die ID");
        return false;
    }
    if ((id >> 4) != 0x226) {
        APP_LOGE("INA226: Invalid Die ID 0x{:04X}", id);
        return false;
    }

    return true;
}

bool DrainTracker::ina226_read_reg16(uint8_t reg, uint16_t &data) {
    uint8_t buf[2];

    if (!i2c_read_block(INA226_ADDR, reg, buf, 2)) { return false; }

    data = (static_cast<uint16_t>(buf[0]) << 8) | static_cast<uint16_t>(buf[1]);
    return true;
}

bool DrainTracker::ina226_write_reg16(uint8_t reg, uint16_t data) {
    const uint8_t buf[2] = {
        static_cast<uint8_t>(data >> 8),
        static_cast<uint8_t>(data & 0xFF)
    };
    return i2c_write_block(INA226_ADDR, reg, buf, 2);
}
