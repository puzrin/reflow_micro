#include <esp_check.h>
#include <esp_adc/adc_cali_scheme.h>
#include <soc/soc_caps.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "components/i2c_io.hpp"
#include "components/pb2struct.hpp"
#include "components/time.hpp"
#include "head.hpp"
#include "logger.hpp"
#include "lib/pt100.hpp"
#include "power.hpp"
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
        APP_LOGI("Head: Detached");

        head.head_status.store(HeadStatus_HeadDisconnected);
        return No_State_Change;
    }

    static auto on_run_state(Head& head) -> state_id_t {
        if (head.last_sensor_value_uv.load() <= Head::SENSOR_FLOATING_LEVEL_MV * 1000) {
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

        head.head_status.store(HeadStatus_HeadInitializing);
        head.debounce_start = Time::now();
        return No_State_Change;
    }

    static auto on_run_state(Head& head) -> state_id_t {
        if (head.last_sensor_value_uv.load() >= Head::SENSOR_FLOATING_LEVEL_MV * 1000) {
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
            head.heater_type = (head.last_sensor_value_uv.load() <= Head::SENSOR_SHORTED_LEVEL_MV * 1000)
                ? HeaterType_PCB : HeaterType_MCH;

            if (!head.eeprom_store.read(head.head_params.value)) {
                APP_LOGE("Head: Failed to read EEPROM");
                return HeadState::Error;
            }

            // If EEPROM is empty, use defaults
            if (head.head_params.value.empty()) {
                APP_LOGI("Head: No head params found, fallback to defaults");
                head.head_params.value.assign(
                    std::begin(DEFAULT_HEAD_PARAMS_PB),
                    std::end(DEFAULT_HEAD_PARAMS_PB)
                );
            }

            // Configure temperature processor with sensor type and calibration
            head.configure_temperature_processor();

            return HeadState::Attached;
        }

        return No_State_Change;
    }

    static void on_exit_state(Head&) {}
};

class HeadAttached_state : public afsm::state<Head, HeadAttached_state, HeadState::Attached> {
public:
    static auto on_enter_state(Head& head) -> state_id_t {
        APP_LOGI("Head: Attached");

        head.head_status.store(HeadStatus_HeadConnected);
        return No_State_Change;
    }

    static auto on_run_state(Head& head) -> state_id_t {
        if (head.last_sensor_value_uv.load() >= Head::SENSOR_FLOATING_LEVEL_MV * 1000) {
            return HeadState::Detached;
        }

        return No_State_Change;
    }

    static void on_exit_state(Head&) {}
};

class HeadError_state : public afsm::state<Head, HeadError_state, HeadState::Error> {
public:
    static auto on_enter_state(Head& head) -> state_id_t {
        APP_LOGE("Head: Error");
        head.head_status.store(HeadStatus_HeadError);
        head.debounce_start = Time::now();
        return No_State_Change;
    }

    static auto on_run_state(Head& head) -> state_id_t {
        if (head.last_sensor_value_uv.load() >= Head::SENSOR_FLOATING_LEVEL_MV * 1000) {
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
            APP_LOGE("Head: Failed to write EEPROM");
        }
    }

    update_sensor_uv();

    // Run state machine
    run();
}

void Head::adc_init() {
    // Calculate derived parameters
    constexpr uint32_t adc_sample_freq_hz = TEMPERATURE_SAMPLE_FREQ_HZ * ADC_OVERSAMPLING_COUNT;
    constexpr uint32_t conv_frame_size = ADC_OVERSAMPLING_COUNT * SOC_ADC_DIGI_RESULT_BYTES;
    constexpr uint32_t dma_buffer_size = conv_frame_size * 2;

    // Compile-time checks for hardware limits
    static_assert(adc_sample_freq_hz <= SOC_ADC_SAMPLE_FREQ_THRES_HIGH,
                  "ADC sample frequency exceeds hardware limit");
    static_assert(adc_sample_freq_hz >= SOC_ADC_SAMPLE_FREQ_THRES_LOW,
                  "ADC sample frequency below hardware limit");
    static_assert(conv_frame_size % SOC_ADC_DIGI_RESULT_BYTES == 0,
                  "Frame size must be multiple of SOC_ADC_DIGI_RESULT_BYTES");
    static_assert(ADC_OVERSAMPLING_COUNT >= 10,
                  "Too few samples for meaningful averaging");

    // Create ADC handle
    adc_continuous_handle_cfg_t adc_config{};
    adc_config.max_store_buf_size = dma_buffer_size;
    adc_config.conv_frame_size = conv_frame_size;
    adc_config.flags.flush_pool = 0;
    ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &adc_handle));

    // Configure ADC
    adc_digi_pattern_config_t adc_pattern[1]{};
    adc_pattern[0].atten = ADC_ATTEN_DB_0;
    adc_pattern[0].channel = ADC_CHANNEL_4;
    adc_pattern[0].unit = ADC_UNIT_1;
    adc_pattern[0].bit_width = ADC_BITWIDTH_12;

    adc_continuous_config_t dig_cfg{};
    dig_cfg.pattern_num = 1;
    dig_cfg.adc_pattern = adc_pattern;
    dig_cfg.sample_freq_hz = adc_sample_freq_hz;
    dig_cfg.conv_mode = ADC_CONV_SINGLE_UNIT_1;
    dig_cfg.format = ADC_DIGI_OUTPUT_FORMAT_TYPE2;
    ESP_ERROR_CHECK(adc_continuous_config(adc_handle, &dig_cfg));

    // Setup calibration
    adc_cali_curve_fitting_config_t cf{};
    cf.unit_id = ADC_UNIT_1;
    cf.chan = ADC_CHANNEL_4;
    cf.atten = ADC_ATTEN_DB_0;
    cf.bitwidth = ADC_BITWIDTH_12;
    ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&cf, &adc_cali_handle));

    // Build ADC calibration lookup table with sub-mV interpolation
    build_adc_lut();

    // Register callback
    adc_continuous_evt_cbs_t cbs{};
    cbs.on_conv_done = adc_conv_done_callback;
    cbs.on_pool_ovf = nullptr;
    ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(adc_handle, &cbs, this));

    // Start continuous conversion
    ESP_ERROR_CHECK(adc_continuous_start(adc_handle));
}

void Head::build_adc_lut() {
    int prev_mV = 0;
    adc_cali_raw_to_voltage(adc_cali_handle, 0, &prev_mV);

    uint32_t last_transition_idx = 0;
    size_t grid_point = 0;

    for (uint32_t raw = 1; raw <= 4095; raw++) {
        int mV = 0;
        adc_cali_raw_to_voltage(adc_cali_handle, raw, &mV);

        if (mV != prev_mV) {
            last_transition_idx = raw;
            prev_mV = mV;
        }

        uint32_t next_grid_idx = (grid_point * 4095) / (ADC_INTERPOLATOR_LUT_SIZE_MAX - 1);

        if (raw >= next_grid_idx && grid_point < ADC_INTERPOLATOR_LUT_SIZE_MAX) {
            adc_interpolator.points.push_back({
                .raw = static_cast<uint16_t>(last_transition_idx),
                .mV = static_cast<uint16_t>(prev_mV)
            });
            grid_point++;
        }
    }
}

bool IRAM_ATTR Head::adc_conv_done_callback(adc_continuous_handle_t handle,
                                           const adc_continuous_evt_data_t *edata,
                                           void *user_data) {
    Head* self = static_cast<Head*>(user_data);

    uint32_t sum = 0;
    uint32_t count = 0;

    // Average all samples in this frame
    for (uint32_t i = 0; i < edata->size; i += SOC_ADC_DIGI_RESULT_BYTES) {
        adc_digi_output_data_t *p = (adc_digi_output_data_t*)&edata->conv_frame_buffer[i];

        if (p->type2.channel == ADC_CHANNEL_4) {
            sum += p->type2.data;
            count++;
        }
    }

    if (count > 0) {
        // Store avg_x100 to preserve fractional precision
        uint32_t avg_x100 = (sum * 100) / count;

        self->temp_ring_buffer[self->ring_buffer_idx] = avg_x100;
        self->ring_buffer_idx = (self->ring_buffer_idx + 1) % TEMPERATURE_RING_BUFFER_SIZE;
        auto prev_count = self->ring_buffer_count.load();
        if (prev_count < TEMPERATURE_RING_BUFFER_SIZE) {
            self->ring_buffer_count.store(static_cast<uint8_t>(prev_count + 1));
        }
    }

    return false; // Don't yield from ISR
}

void Head::update_sensor_uv() {
    uint32_t valid_count = ring_buffer_count.load();
    if (valid_count == 0) {
        return;
    }

    // Sum all avg_x100 values from ring buffer
    uint32_t total_sum = 0;
    for (uint32_t i = 0; i < valid_count; i++) {
        total_sum += temp_ring_buffer[i];
    }

    // Convert to microvolts using interpolator (scale = valid_count * 100)
    uint32_t uV = adc_interpolator.to_uv(total_sum, valid_count * 100);

    last_sensor_value_uv.store(uV);
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
    configure_temperature_processor();
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
    configure_temperature_processor();
    return true;
}

bool Head::get_head_params(HeadParams& params, bool skip_status_check) {
    if (!skip_status_check &&
        get_head_status() != HeadStatus_HeadConnected)
    {
        return false;
    }

    return pb2struct(head_params.value, params, HeadParams_fields);
}

bool Head::set_head_params(const HeadParams& params) {
    EEBuffer pb_data{};
    if (!struct2pb(params, pb_data, HeadParams_fields, HeadParams_size)) { return false; }

    set_head_params_pb(pb_data);
    return true;
}

int32_t Head::get_temperature_x10() {
    // Safety check, should never happen due to state machine
    if (get_head_status() != HeadStatus_HeadConnected) {
        return UNKNOWN_TEMPERATURE_X10;
    }

    if (heater_type.load() == HeaterType_MCH) {
        // Use RTD sensor for MCH heads
        auto uV = last_sensor_value_uv.load();
        return temperature_processor.get_temperature_x10(uV);
    }
    else {
        auto mohms = power.get_load_mohm();
        if (mohms == Power::UNKNOWN_RESISTANCE) {
            return UNKNOWN_TEMPERATURE_X10;
        }
        return temperature_processor.get_temperature_x10(mohms);
    }
}

bool Head::is_attached() const {
    return get_head_status() == HeadStatus_HeadConnected;
}

void Head::configure_temperature_processor() {
    // Set sensor type based on heater type
    if (heater_type.load() == HeaterType_MCH) {
        // MCH => use RTD sensor
        temperature_processor.set_sensor_type(TemperatureProcessor::SensorType::RTD);
    } else {
        // PCB => use TCR-based estimates
        temperature_processor.set_sensor_type(TemperatureProcessor::SensorType::TCR);
    }

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
