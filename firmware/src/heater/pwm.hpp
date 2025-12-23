#pragma once

#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <pd/utils/afsm.h>

class Pwm : public afsm::fsm<Pwm>{
public:
    static_assert(configTICK_RATE_HZ == 1000, "PWM timings assume 1 ms FreeRTOS tick");

    static constexpr uint32_t PWM_PERIOD_TICKS = 200;
    static constexpr uint32_t PWM_IDLE_PERIOD_TICKS = 500;
    static constexpr uint32_t PWM_MIN_PULSE_TICKS = 7;
    static constexpr uint32_t POWER_STABILIZATION_TICKS = 5;
    static_assert(PWM_MIN_PULSE_TICKS >= POWER_STABILIZATION_TICKS,
        "PWM_MIN_PULSE_TICKS must be >= POWER_STABILIZATION_TICKS for ADC readings");

    Pwm();
    void setup();
    // Duty is 0..1000
    void set_duty_x1000(uint32_t duty_0_1000);
    uint32_t get_duty_x1000() const;

    void enable(bool enable);
    void load_on(bool on);

    // PWM cycle bookkeeping
    // (ticks counted in units of PWM_TICK_MS)
    uint32_t pulse_ticks{0};
    uint32_t gap_ticks{0};
    uint32_t tick_count{0};
    // Signed error for dithering (-500..+499), 1/1000-tick units
    int32_t duty_error{0};

    void run() {
        xSemaphoreTake(fsm_lock, portMAX_DELAY);
        Base::run();
        xSemaphoreGive(fsm_lock);
    }
    void change_state(afsm::state_id_t new_state_id) {
        xSemaphoreTake(fsm_lock, portMAX_DELAY);
        Base::change_state(new_state_id);
        xSemaphoreGive(fsm_lock);
    }

    etl::atomic<bool> _enabled{false};

private:
    using Base = afsm::fsm<Pwm>;
    void task_loop();

    static constexpr gpio_num_t load_switch_pin{GPIO_NUM_3};

    SemaphoreHandle_t fsm_lock{xSemaphoreCreateMutex()};
    etl::atomic<uint32_t> _duty_x1000{0};
};
