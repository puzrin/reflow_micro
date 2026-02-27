#include "drain_tracker.hpp"

#include "components/i2c_io.hpp"
#include "logger.hpp"

DrainTracker drain_tracker;

void DrainTracker::setup() {
    i2c_init();
    if (!adc_ina_init()) {
        APP_LOGE("DrainTracker: INA init failed");
    }
}

void DrainTracker::collect_data(uint32_t ctx_idx) {
    uint16_t adc_v_raw;
    uint16_t adc_i_raw;
    uint8_t vbus_reg;
    uint8_t current_reg;

    if (adc_ina_chip == ADC_INA_CHIP::INA226) {
        vbus_reg = 0x02;
        current_reg = 0x04;
    } else if (adc_ina_chip == ADC_INA_CHIP::INA238) {
        vbus_reg = 0x05;
        current_reg = 0x07;
    } else {
        return;
    }

    if (!adc_ina_read_reg16(vbus_reg, adc_v_raw)) { return; }
    if (!adc_ina_read_reg16(current_reg, adc_i_raw)) { return; }

    adc_buffer[adc_count % ADC_FILTER_SIZE] = {
        .v_raw = adc_v_raw,
        .i_raw = static_cast<int16_t>(adc_i_raw),
        .ctx_idx = ctx_idx
    };
    adc_count++;
}

void DrainTracker::process_collected_data() {
    if (adc_count == 0) {
        return;
    }
    if (adc_ina_chip == ADC_INA_CHIP::Unknown) {
        adc_count = 0;
        return;
    }

    uint32_t count = adc_count < ADC_FILTER_SIZE ? adc_count : ADC_FILTER_SIZE;

    // Check that all samples have a consistent profile index.
    // If the index changed mid-collection, drop this batch; the next PWM cycle
    // will retry.
    uint32_t first_idx = adc_buffer[0].ctx_idx;
    for (uint32_t i = 1; i < count; i++) {
        if (adc_buffer[i].ctx_idx != first_idx) {
            adc_count = 0;
            return;
        }
    }

    uint32_t v_sum{0};
    int32_t i_sum{0};

    for (uint32_t i = 0; i < count; i++) {
        v_sum += adc_buffer[i].v_raw;
        i_sum += adc_buffer[i].i_raw;
    }

    if (i_sum < 0) { i_sum = 0; }
    uint32_t v_avg = v_sum / count;
    uint32_t peak_mv = 0;
    if (adc_ina_chip == ADC_INA_CHIP::INA238) {
        // INA238 VBUS LSB = 3.125 mV
        peak_mv = (v_avg * 3125 + 500) / 1000;
    } else {
        // INA226 VBUS LSB = 1.25 mV
        peak_mv = (v_avg * 5 + 2) / 4;
    }
    // Shunt current already in mA
    uint32_t peak_ma = static_cast<uint32_t>(i_sum / static_cast<int32_t>(count));
    // Thresholds for real measurements, not PD limits.
    bool load_valid = (peak_ma >= 300 && peak_mv >= 4000);

    xSemaphoreTake(info_lock, portMAX_DELAY);
    info.peak_mv = peak_mv;
    info.peak_ma = peak_ma;
    info.load_valid = load_valid;
    info.ctx_idx = first_idx;
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

bool DrainTracker::adc_ina_detect() {
    adc_ina_chip = ADC_INA_CHIP::Unknown;

    uint16_t manufacturer_id{0};
    uint16_t device_id{0};

    //
    // Try INA238 first:
    // - MANUFACTURER_ID = 0x5449 at 0x3E
    // - DEVICE_ID[15:4]  = 0x238  at 0x3F
    //
    if (!(adc_ina_read_reg16(0x3E, manufacturer_id) &&
          adc_ina_read_reg16(0x3F, device_id)))
    {
        APP_LOGE("INA238: I2C read error");
        return false;
    }

    if (manufacturer_id == 0x5449 && (device_id >> 4) == 0x238) {
        adc_ina_chip = ADC_INA_CHIP::INA238;
        return true;
    }

    //
    // INA226 fallback:
    // - MANUFACTURER_ID = 0x5449 at 0xFE
    // - DEVICE_ID[15:4]  = 0x226  at 0xFF
    //
    if (!(adc_ina_read_reg16(0xFE, manufacturer_id) &&
          adc_ina_read_reg16(0xFF, device_id)))
    {
        APP_LOGE("INA226: I2C read error");
        return false;
    }

    if (manufacturer_id == 0x5449 && (device_id >> 4) == 0x226) {
        adc_ina_chip = ADC_INA_CHIP::INA226;
        return true;
    }

    APP_LOGE("INAxxx: No supported INAxxx chip found.");
    return false;
}

bool DrainTracker::adc_ina_init() {
    if (!adc_ina_detect()) { return false; }

    if (adc_ina_chip == ADC_INA_CHIP::INA238) {
        // CONFIG (0x00): ADCRANGE=0, no delayed start
        if (!adc_ina_write_reg16(0x00, 0x0000)) {
            APP_LOGE("INA238: Failed to write CONFIG");
            return false;
        }

        // VBUSCT=VSHCT=3 (~280 us), AVG=0 (x1); MODE = Continuous shunt + bus
        // Effective full cycle is ~560 us: (280 + 280) * 1.
        // if (!adc_ina_write_reg16(0x01, 0xB6D8)) {
        //     APP_LOGE("INA238: Failed to write ADC_CONFIG");
        //     return false;
        // }

        // Using faster sampling with hardware averaging reduces noise.
        // VBUSCT=VSHCT=1 (~84 us), AVG=1 (x4); MODE = Continuous shunt + bus
        // Effective full cycle is ~672 us: (84 + 84) * 4.
        if (!adc_ina_write_reg16(0x01, 0xB249)) {
            APP_LOGE("INA238: Failed to write ADC_CONFIG");
            return false;
        }

        // SHUNT_CAL (0x02) with Current_LSB=1 mA and Rshunt=10 mOhm:
        // SHUNT_CAL = 819.2e6 * Current_LSB[A] * Rshunt[ohm]
        //           = 819.2e6 * 0.001 * 0.01 = 8192 = 0x2000.
        if (!adc_ina_write_reg16(0x02, 0x2000)) {
            APP_LOGE("INA238: Failed to write SHUNT_CAL");
            return false;
        }

        APP_LOGI("DrainTracker: INA238 detected");
        return true;
    }

    // VBUSCT=VSHCT=010 (332 us), AVG=000 (x1); MODE = Continuous shunt + bus
    // Effective full cycle is ~664 us: (332 + 332) * 1.
    if (!adc_ina_write_reg16(0x00, 0x0097)) {
        APP_LOGE("INA226: Failed to write CONFIG");
        return false;
    }

    // CALIBRATION (0x05) = 0x0200
    // Current_LSB = 1 mA & Rshunt=10 mOhm:
    // Cal = 0.00512 / (Current_LSB[A] * Rshunt[ohm])
    //     = 0.00512 / (0.001 * 0.01) = 512 = 0x0200.
    if (!adc_ina_write_reg16(0x05, 0x0200)) {
        APP_LOGE("INA226: Failed to write CALIBRATION");
        return false;
    }

    APP_LOGI("DrainTracker: INA226 detected");
    return true;
}

bool DrainTracker::adc_ina_read_reg16(uint8_t reg, uint16_t &data) {
    uint8_t buf[2];

    if (!i2c_read_block(ADC_INA_ADDR, reg, buf, 2)) { return false; }

    data = (static_cast<uint16_t>(buf[0]) << 8) | static_cast<uint16_t>(buf[1]);
    return true;
}

bool DrainTracker::adc_ina_write_reg16(uint8_t reg, uint16_t data) {
    const uint8_t buf[2] = {
        static_cast<uint8_t>(data >> 8),
        static_cast<uint8_t>(data & 0xFF)
    };
    return i2c_write_block(ADC_INA_ADDR, reg, buf, 2);
}
