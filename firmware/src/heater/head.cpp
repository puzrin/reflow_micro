#include <esp_check.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "components/i2c_io.hpp"
#include "components/time.hpp"
#include "head.hpp"
#include "logger.hpp"
#include "lib/pt100.hpp"
#include "proto/generated/defaults.hpp"

Head head;

static constexpr uint32_t TASK_TICK_MS = 20;
static constexpr uint32_t SENSOR_DEBOUNCE_MS = 100;
static constexpr uint32_t ERROR_RESTORE_MS = 1000;

using afsm::state_id_t;

namespace HeadState {
    enum {
        Detached,
        Initializing,
        Attached,
        Error
    };
}

class HeadDetached_state : public afsm::state<Head, HeadDetached_state, HeadState::Detached> {
public:
    static auto on_enter_state(Head& head) -> state_id_t {
        APP_LOGI("Head Detached");

        head.head_status.store(HeadStatus_HeadDisconnected);
        return No_State_Change;
    }

    static auto on_run_state(Head& head) -> state_id_t {
        if (head.last_sensor_value_mv.load() <= Head::SENSOR_FLOATING_LEVEL_MV) {
            return HeadState::Initializing;
        }
        return No_State_Change;
    }

    static void on_exit_state(Head&) {}
};

class HeadInitializing_state : public afsm::state<Head, HeadInitializing_state, HeadState::Initializing> {
public:
    static auto on_enter_state(Head& head) -> state_id_t {
        APP_LOGI("Head Initializing");

        head.head_status.store(HeadStatus_HeadInitializing);
        head.debounce_start = Time::now();
        return No_State_Change;
    }

    static auto on_run_state(Head& head) -> state_id_t {
        if (head.last_sensor_value_mv.load() >= Head::SENSOR_FLOATING_LEVEL_MV) {
            return HeadState::Detached;
        }

        if (Time(head.debounce_start).expired(SENSOR_DEBOUNCE_MS))
        {
            // MCH head has real sensor (PT100).
            // If pin is shorted => use TCR-based estimates (consider heater
            // type as PCB).
            //
            // The difference is, PCB-based heater has much better heat
            // distribution. For MCH heaters using TCR-based estimates may lead
            // to errors, difficult to fix.
            head.heater_type = (head.last_sensor_value_mv.load() <= Head::SENSOR_SHORTED_LEVEL_MV)
                ? HeaterType_MCH : HeaterType_PCB;

            if (!head.eeprom_store.read(head.head_params.value)) {
                APP_LOGE("Failed to read EEPROM");
                return HeadState::Error;
            }

            // If EEPROM is empty, use defaults
            if (head.head_params.value.empty()) {
                APP_LOGI("No head params found, fallback to defaults");
                head.head_params.value.assign(
                    std::begin(DEFAULT_HEAD_PARAMS_PB),
                    std::end(DEFAULT_HEAD_PARAMS_PB)
                );
            }

            return HeadState::Attached;
        }

        return No_State_Change;
    }

    static void on_exit_state(Head&) {}
};

class HeadAttached_state : public afsm::state<Head, HeadAttached_state, HeadState::Attached> {
public:
    static auto on_enter_state(Head& head) -> state_id_t {
        APP_LOGI("Head Attached");

        head.head_status.store(HeadStatus_HeadConnected);
        return No_State_Change;
    }

    static auto on_run_state(Head& head) -> state_id_t {
        if (head.last_sensor_value_mv.load() >= Head::SENSOR_FLOATING_LEVEL_MV) {
            return HeadState::Detached;
        }

        return No_State_Change;
    }

    static void on_exit_state(Head&) {}
};

class HeadError_state : public afsm::state<Head, HeadError_state, HeadState::Error> {
public:
    static auto on_enter_state(Head& head) -> state_id_t {
        APP_LOGE("Head Error");
        head.head_status.store(HeadStatus_HeadError);
        head.debounce_start = Time::now();
        return No_State_Change;
    }

    static auto on_run_state(Head& head) -> state_id_t {
        if (head.last_sensor_value_mv.load() >= Head::SENSOR_FLOATING_LEVEL_MV) {
            return HeadState::Detached;
        }

        if (Time(head.debounce_start).expired(ERROR_RESTORE_MS)) {
            return HeadState::Detached;
        }

        return No_State_Change;
    }

    static void on_exit_state(Head&) {}
};

using HEAD_STATES = afsm::state_pack<
    HeadDetached_state,
    HeadInitializing_state,
    HeadAttached_state,
    HeadError_state
>;

Head::Head() {
    // Don't start FSM here, because int requires ADC & I2C setup
    set_states<HEAD_STATES>(afsm::Uninitialized);
}

void Head::setup() {
    i2c_init();
    // Configure ADC on IO4 (ADC1_CH4)
    adc_init();
    // Now we can start the FSM.
    change_state(HeadState::Detached);

    xTaskCreate(
        [](void* params) {
            auto* self = static_cast<Head*>(params);
            while (true) {
                self->task_loop();
                vTaskDelay(pdMS_TO_TICKS(TASK_TICK_MS));
            }
        }, "Head", 1024*4, this, 4, nullptr
    );
}

void Head::task_loop() {
    if (head_params.makeSnapshot()) {
        if (!eeprom_store.write(head_params.snapshot)) {
            APP_LOGE("Failed to write EEPROM");
        }
    }

    update_sensor_mv();

    // Run state machine
    run();
}

void Head::adc_init() {
    // Create ADC unit
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = ADC_UNIT_1,
        .clk_src = ADC_DIGI_CLK_SRC_DEFAULT,
        .ulp_mode = ADC_ULP_MODE_DISABLE
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &adc1_handle));

    // IO4 -> ADC1_CH4, 12 bit, 0 dB
    adc_oneshot_chan_cfg_t ch_cfg = {
        .atten = ADC_ATTEN_DB_0,
        .bitwidth = ADC_BITWIDTH_12
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_4, &ch_cfg));

    // Calibration (curve fitting, fallback to line fitting)
    adc_cali_curve_fitting_config_t cf = {
        .unit_id = ADC_UNIT_1,
        .chan = ADC_CHANNEL_4, // actually unused, exists only to dim warnings
        .atten = ADC_ATTEN_DB_0,
        .bitwidth = ADC_BITWIDTH_12
    };
    ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&cf, &adc_cali_handle));
}

void Head::update_sensor_mv() {
    int raw = 0;

    for (;;) {
        auto res = adc_oneshot_read(adc1_handle, ADC_CHANNEL_4, &raw);
        if (res == ESP_OK) { break; }

        APP_LOGE("Sensor ADC read failure [{}], retrying...", esp_err_to_name(res));
        vTaskDelay(1);
    }

    int mv = 0;
    ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc_cali_handle, raw, &mv));

    last_sensor_value_mv.store((uint32_t)mv);
}

bool Head::get_head_params_pb(std::vector<uint8_t>& pb_data) {
    if (get_head_status() != HeadStatus_HeadConnected) { return false; }

    pb_data.assign(head_params.value.begin(), head_params.value.end());
    return true;
}

bool Head::set_head_params_pb(const std::vector<uint8_t>& pb_data) {
    if (get_head_status() != HeadStatus_HeadConnected) { return false; }

    EEBuffer pb_data_buf{pb_data.begin(), pb_data.end()};
    head_params.writeData(pb_data_buf);
    return true;
}

bool Head::get_head_params_pb(EEBuffer& pb_data) {
    if (get_head_status() != HeadStatus_HeadConnected) { return false; }

    pb_data.assign(head_params.value.begin(), head_params.value.end());
    return true;
}

bool Head::set_head_params_pb(const EEBuffer& pb_data) {
    if (get_head_status() != HeadStatus_HeadConnected) { return false; }

    head_params.writeData(pb_data);
    return true;
}

int32_t Head::get_temperature_x10() const {
    uint32_t mv = last_sensor_value_mv.load();

    // Safety check, shuld never happen due to state machine
    if (mv < SENSOR_SHORTED_LEVEL_MV || mv > SENSOR_FLOATING_LEVEL_MV) {
        return 0;
    }

    // We have voltage divider with 560R resistor and 2.5V reference voltage.
    // So Vout = Vin * Rpt100 / (Rfixed + Rpt100)
    // => Rpt100 = Rfixed * Vout / (Vin - Vout)
    // Vin = 2.5V

    uint32_t pt100_resistance_x1000 = 560 * 1000 * mv / (2500 - mv);

    return pt100_temp_x10(pt100_resistance_x1000);
}

bool Head::is_attached() const {
    return get_head_status() == HeadStatus_HeadConnected;
}
