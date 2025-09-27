#include <etl/algorithm.h>

#include "components/i2c_io.hpp"
#include "logger.hpp"
#include "pwm.hpp"

namespace PwmState {
    enum {
        Disabled,
        Pulse,
        Gap
    };
}

class PwmDisabled_state : public afsm::state<Pwm, PwmDisabled_state, PwmState::Disabled> {
public:
    static auto on_enter_state(Pwm& pwm) -> afsm::state_id_t {
        pwm.load_on(false);
        pwm.duty_error = 0;

        auto ci = pwm.get_consumer_info();
        pwm.set_consumer_info(ci.peak_mv, ci.peak_ma, false);

        pwm._enabled.store(false);
        return No_State_Change;
    }

    static auto on_run_state(Pwm& pwm) -> afsm::state_id_t {
        if (pwm._enabled.load()) { return PwmState::Pulse; }
        return No_State_Change;
    }
    static void on_exit_state(Pwm&) {}
};

class PwmPulse_state : public afsm::state<Pwm, PwmPulse_state, PwmState::Pulse> {
public:
    static auto on_enter_state(Pwm& pwm) -> afsm::state_id_t {
        auto duty_x1000 = pwm.get_duty_x1000();

        if (duty_x1000 == 0) {
            pwm.pulse_ticks = Pwm::PWM_MIN_PULSE_TICKS;
            pwm.gap_ticks = Pwm::PWM_IDLE_PERIOD_TICKS;
            pwm.duty_error = 0; // reset carry when idle pulsing
        } else {
            const uint32_t desired = duty_x1000 * Pwm::PWM_PERIOD_TICKS;
            // Round-to-nearest with error feedback (signed error in 1/1000-tick)
            int32_t tmp = static_cast<int32_t>(desired) + pwm.duty_error + 500;
            uint32_t pulse_ticks = static_cast<uint32_t>(tmp / 1000);

            // Clamp pulse to valid range (min pulse .. full period)
            pwm.pulse_ticks = etl::clamp(pulse_ticks, Pwm::PWM_MIN_PULSE_TICKS, Pwm::PWM_PERIOD_TICKS);
            pwm.gap_ticks = Pwm::PWM_PERIOD_TICKS - pwm.pulse_ticks;

            // Update signed error (deviation) after clamping to reflect actual pulse length
            // Restrict error range to avoid overflow due to pulse clamping
            pwm.duty_error = etl::clamp(
                static_cast<int32_t>(desired) + pwm.duty_error - static_cast<int32_t>(pwm.pulse_ticks) * 1000,
                static_cast<int32_t>(-500),
                static_cast<int32_t>(499)
            );
        }

        pwm.tick_count = 0;
        pwm.adc_count = 0;
        pwm.load_on(true);
        return No_State_Change;
    }

    static auto on_run_state(Pwm& pwm) -> afsm::state_id_t {
        pwm.tick_count++;

        if (pwm.tick_count >= Pwm::PWM_MIN_PULSE_TICKS) {
            uint16_t adc_v_raw;
            uint16_t adc_i_raw;
            if (pwm.ina226_read_reg16(0x02, adc_v_raw) &&
                pwm.ina226_read_reg16(0x04, adc_i_raw)) {
                pwm.adc_buffer[pwm.adc_count % Pwm::ADC_FILTER_SIZE] = {
                    .v_raw = adc_v_raw,
                    .i_raw = static_cast<int16_t>(adc_i_raw)
                };
                // Valid reading
                pwm.adc_count++;
            }
        }

        if (pwm.tick_count >= pwm.pulse_ticks) { return PwmState::Gap; }
        return No_State_Change;
    }

    static void on_exit_state(Pwm& pwm) {
        if (pwm.tick_count >= pwm.pulse_ticks && pwm.adc_count > 0) {
            // End reached naturally => process ADC data
            // (otherwise, we are being disabled, skip this step)
            auto count = etl::min(pwm.adc_count, Pwm::ADC_FILTER_SIZE);
            uint32_t v_sum{0};
            int32_t i_sum{0};

            for (uint32_t i = 0; i < count; i++) {
                v_sum += pwm.adc_buffer[i].v_raw;
                i_sum += pwm.adc_buffer[i].i_raw;
            }

            if (i_sum < 0) { i_sum = 0; }
            // Vbus = V_raw * 1.25 LSB (per datasheet)
            uint32_t peak_mv = ((v_sum / count) * 5 + 2) / 4; // x1.25, rounded
            // Shunt current already in mA
            uint32_t peak_ma = i_sum / count;
            pwm.set_consumer_info(peak_mv, peak_ma, true);
        }
    }
};

class PwmGap_state : public afsm::state<Pwm, PwmGap_state, PwmState::Gap> {
public:
    static auto on_enter_state(Pwm& pwm) -> afsm::state_id_t {
        // If no gap, go back to pulse immediately
        if (pwm.gap_ticks == 0) { return PwmState::Pulse; }

        pwm.tick_count = 0;
        pwm.load_on(false);
        return No_State_Change;
    }

    static auto on_run_state(Pwm& pwm) -> afsm::state_id_t {
        if (++pwm.tick_count >= pwm.gap_ticks) { return PwmState::Pulse; }
        return No_State_Change;
    }

    static void on_exit_state(Pwm&) {}
};

using PWM_STATES = afsm::state_pack<
    PwmDisabled_state,
    PwmPulse_state,
    PwmGap_state
>;

// (no static state – all timing stored in Pwm instance)

Pwm::Pwm() {
    set_states<PWM_STATES>(afsm::Uninitialized);
}

void Pwm::task_loop() {
    run();
}

void Pwm::setup() {
    // Configure switch pin.
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << load_switch_pin),
        .mode = GPIO_MODE_OUTPUT,
        // MOSFET driver already has proper pullup
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    i2c_init();
    ina226_init();
    change_state(PwmState::Disabled);

    xTaskCreate(
        // Set high priority
        // TODO: Ensure it's ok with NimBLE stack.
        [](void* params) {
            auto* self = static_cast<Pwm*>(params);
            while (true) {
                self->task_loop();
                vTaskDelay(1);
            }
        }, "Pwm", 1024*4, this, 15, nullptr
    );
}

void Pwm::set_duty_x1000(uint32_t millis) {
    _duty_x1000.store(etl::clamp<uint32_t>(millis, 0, 1000));
}

uint32_t Pwm::get_duty_x1000() const {
    return _duty_x1000.load();
}

void Pwm::enable(bool enable) {
    if (enable) {
        // Only set flag, to work in sync with ticks
        _enabled.store(true);
    } else {
        // Disable immediately
        change_state(PwmState::Disabled);
    }
}

void Pwm::load_on(bool on) {
    gpio_set_level(load_switch_pin, on ? 1 : 0);
}

bool Pwm::ina226_init() {
    // CONFIG (0x00) = 0x0007
    // AVG=000 (×1), VBUSCT=000 (140 µs), VSHCT=000 (140 µs), MODE=111 (Shunt+Bus, Continuous).
    if (!ina226_write_reg16(0x00, 0x0007)) {
        APP_LOGE("INA226: Failed to write CONFIG");
        return false;
    }

    // CALIBRATION (0x05) = 0x0200
    // Current_LSB = 1 mA & Rshunt=10 мΩ:
    // Cal = 0.00512 / (Current_LSB[A] * Rshunt[Ω]) = 0.00512 / (0.001 * 0.01) = 512 = 0x0200.
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

bool Pwm::ina226_read_reg16(uint8_t reg, uint16_t &data) {
    uint8_t buf[2];

    if (!i2c_read_block(INA226_ADDR, reg, buf, 2)) { return false; }

    data = (static_cast<uint16_t>(buf[0]) << 8) | static_cast<uint16_t>(buf[1]);
    return true;
}

bool Pwm::ina226_write_reg16(uint8_t reg, uint16_t data) {
    const uint8_t buf[2] = {
        static_cast<uint8_t>(data >> 8),
        static_cast<uint8_t>(data & 0xFF)
    };
    return i2c_write_block(INA226_ADDR, reg, buf, 2);
}
