#include <etl/algorithm.h>

#include "drain_tracker.hpp"
#include "profile_selector.hpp"
#include "pwm.hpp"

extern ProfileSelector profile_selector;

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
        drain_tracker.clear_collected_data();
        pwm.load_on(true);
        return No_State_Change;
    }

    static auto on_run_state(Pwm& pwm) -> afsm::state_id_t {
        pwm.tick_count++;

        if (pwm.tick_count >= Pwm::POWER_STABILIZATION_TICKS) {
            // DrainTracker polls INA226 once per tick after stabilization.
            // Capture profile index at measurement time for eventual consistency.
            drain_tracker.collect_data(profile_selector.current_index);
        }

        if (pwm.tick_count >= pwm.pulse_ticks) { return PwmState::Gap; }
        return No_State_Change;
    }

    static void on_exit_state(Pwm& pwm) {
        if (pwm.tick_count >= pwm.pulse_ticks) {
            // End reached naturally => process ADC data averaged over the pulse tail
            // (otherwise, we are being disabled, skip this step)
            drain_tracker.process_collected_data();
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

    drain_tracker.setup();
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

void Pwm::set_duty_x1000(uint32_t duty_0_1000) {
    _duty_x1000.store(etl::clamp<uint32_t>(duty_0_1000, 0, 1000));
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
