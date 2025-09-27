#pragma once

#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <pd/utils/afsm.h>

class Pwm : public afsm::fsm<Pwm>{
public:
    static constexpr uint32_t PWM_PERIOD_TICKS = 50;
    static constexpr uint32_t PWM_IDLE_PERIOD_TICKS = 500;
    static constexpr uint32_t PWM_MIN_PULSE_TICKS = 3;

    static constexpr uint32_t ADC_FILTER_SIZE = 8;
    struct ADC_ITEM { uint16_t v_raw; int16_t i_raw; };

    struct CONSUMER_INFO {
        uint32_t peak_mv = 0;
        uint32_t peak_ma = 0;
        bool is_actual = false;
    };

    Pwm();
    void setup();
    // Duty is 0..1000
    void set_duty_x1000(uint32_t millis);
    uint32_t get_duty_x1000() const;

    void enable(bool enable);
    void load_on(bool on);

    bool ina226_read_reg16(uint8_t reg, uint16_t &data);
    bool ina226_write_reg16(uint8_t reg, uint16_t data);

    auto get_consumer_info() -> CONSUMER_INFO {
        CONSUMER_INFO info;
        xSemaphoreTake(ci_update_lock, portMAX_DELAY);
        info = consumer_info;
        xSemaphoreGive(ci_update_lock);
        return info;
    }

    void set_consumer_info(uint32_t peak_mv, uint32_t peak_ma, bool is_actual) {
        xSemaphoreTake(ci_update_lock, portMAX_DELAY);
        consumer_info.peak_mv = peak_mv;
        consumer_info.peak_ma = peak_ma;
        consumer_info.is_actual = is_actual;
        xSemaphoreGive(ci_update_lock);
    }

    // PWM cycle bookkeeping
    // (ticks counted in units of PWM_TICK_MS)
    uint32_t pulse_ticks{0};
    uint32_t gap_ticks{0};
    uint32_t tick_count{0};
    uint32_t adc_count{0};
    ADC_ITEM adc_buffer[ADC_FILTER_SIZE]{};
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
    static constexpr uint8_t INA226_ADDR = 0x40; // A0, A1 to GND

    bool ina226_init();

    SemaphoreHandle_t fsm_lock{xSemaphoreCreateMutex()};
    SemaphoreHandle_t ci_update_lock{xSemaphoreCreateMutex()};
    etl::atomic<uint32_t> _duty_x1000{0};
    CONSUMER_INFO consumer_info{};
};
