#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "components/i2c_io.hpp"
#include "components/pb2struct.hpp"
#include "head.hpp"
#include "logger.hpp"
#include "power.hpp"
#include "proto/generated/defaults.hpp"

Head head;

static constexpr uint32_t TASK_TICK_MS = 200;
static constexpr uint8_t DETACH_PROBE_FAIL_THRESHOLD = 3;

using afsm::state_id_t;

namespace HeadState {
    enum {
        Detached,
        Initializing,
        Attached
    };
}

class HeadDetached_state : public afsm::state<Head, HeadDetached_state, HeadState::Detached> {
public:
    static auto on_enter_state(Head& head) -> state_id_t {
        APP_LOGI("Head: Detached");

        head.head_status.store(HeadStatus_HEAD_DISCONNECTED);
        return No_State_Change;
    }

    static auto on_run_state(Head& head) -> state_id_t {
        if (head.eeprom_store.probe()) {
            return HeadState::Initializing;
        }
        return No_State_Change;
    }

    static void on_exit_state(Head&) {}
};

class HeadInitializing_state : public afsm::state<Head, HeadInitializing_state, HeadState::Initializing> {
public:
    static auto on_enter_state(Head& head) -> state_id_t {
        APP_LOGI("Head: Initializing");

        head.head_status.store(HeadStatus_HEAD_INITIALIZING);
        return No_State_Change;
    }

    static auto on_run_state(Head& head) -> state_id_t {
        if (!head.eeprom_store.read(head.head_params.value)) {
            APP_LOGE("Head: Failed to read EEPROM");
            return HeadState::Detached;
        }

        // If EEPROM is empty, use defaults
        if (head.head_params.value.empty()) {
            APP_LOGI("Head: No head params found, fallback to defaults");
            head.head_params.value.assign(
                std::begin(DEFAULT_HEAD_PARAMS_PB),
                std::end(DEFAULT_HEAD_PARAMS_PB)
            );
        }

        // Configure temperature processor with calibration
        head.configure_temperature_processor();

        return HeadState::Attached;
    }

    static void on_exit_state(Head&) {}
};

class HeadAttached_state : public afsm::state<Head, HeadAttached_state, HeadState::Attached> {
public:
    static auto on_enter_state(Head& head) -> state_id_t {
        APP_LOGI("Head: Attached");

        head.head_status.store(HeadStatus_HEAD_CONNECTED);
        head.probe_fail_count = 0;
        return No_State_Change;
    }

    static auto on_run_state(Head& head) -> state_id_t {
        if (head.eeprom_store.probe()) {
            head.probe_fail_count = 0;
            return No_State_Change;
        }

        if (head.probe_fail_count < DETACH_PROBE_FAIL_THRESHOLD) {
            ++head.probe_fail_count;
        }

        APP_LOGI("Head: EEPROM probe failed, attempts={}", head.probe_fail_count);

        if (head.probe_fail_count < DETACH_PROBE_FAIL_THRESHOLD) {
            return No_State_Change;
        }

        return HeadState::Detached;
    }

    static void on_exit_state(Head&) {}
};

using HEAD_STATES = afsm::state_pack<
    HeadDetached_state,
    HeadInitializing_state,
    HeadAttached_state
>;

Head::Head() {
    // Don't start the FSM here, because it requires I2C setup.
    set_states<HEAD_STATES>(afsm::Uninitialized);
}

void Head::setup() {
    i2c_init();
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
            APP_LOGE("Head: Failed to write EEPROM");
        }
    }

    // Run state machine
    run();
}

bool Head::get_head_params_pb(etl::ivector<uint8_t>& pb_data) {
    if (!is_attached()) { return false; }

    pb_data.assign(head_params.value.begin(), head_params.value.end());
    return true;
}

bool Head::set_head_params_pb(const etl::ivector<uint8_t>& pb_data) {
    if (!is_attached()) { return false; }

    EEBuffer pb_data_buf{pb_data.begin(), pb_data.end()};
    head_params.writeData(pb_data_buf);
    configure_temperature_processor();
    return true;
}

bool Head::get_head_params(HeadParams& params, bool skip_status_check) {
    if (!skip_status_check && !is_attached()) { return false; }

    return pb2struct(head_params.value, params, HeadParams_fields);
}

bool Head::set_head_params(const HeadParams& params) {
    EEBuffer pb_data{};
    if (!struct2pb(params, pb_data, HeadParams_fields)) { return false; }

    head_params.writeData(pb_data);
    configure_temperature_processor();
    return true;
}

int32_t Head::get_temperature_x10() {
    // Safety check, should never happen due to state machine
    if (!is_attached()) { return UNKNOWN_TEMPERATURE_X10; }

    auto mohms = power.get_load_mohm();
    if (mohms == Power::UNKNOWN_RESISTANCE) {
        return UNKNOWN_TEMPERATURE_X10;
    }
    return temperature_processor.get_temperature_x10(mohms);
}

void Head::configure_temperature_processor() {
    // Load calibration data
    HeadParams params = HeadParams_init_zero;
    if (get_head_params(params, true)) {
        temperature_processor.set_cal_points(
            params.sensor_p0_at,
            params.sensor_p0_value,
            params.sensor_p1_at,
            params.sensor_p1_value
        );
    }
}
