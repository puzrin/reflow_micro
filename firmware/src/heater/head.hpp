#pragma once

#include <esp_adc/adc_continuous.h>
#include <esp_adc/adc_cali.h>
#include <etl/limits.h>
#include <etl/atomic.h>
#include <pd/utils/afsm.h>
#include <vector>

#include "components/eeprom_store.hpp"
#include "components/temperature_processor.hpp"
#include "lib/data_guard.hpp"
#include "lib/adc_interpolator.hpp"
#include "proto/generated/types.pb.h"

class Head : public afsm::fsm<Head> {
public:
    // We have 2.5v ref voltage and 560R +PT100 divider. That gives ~
    // [0.32..0.5] V range for [-50..+400] C.
    //
    // Set 2 levels to detect shorted and floating sensor:
    // - Open => Not attached
    // - Shorted => With embedded sensor
    // - In between => With PT100 sensor
    static constexpr uint32_t SENSOR_SHORTED_LEVEL_MV = 150;
    static constexpr uint32_t SENSOR_FLOATING_LEVEL_MV = 800;

    // User-configurable ADC parameters

    // Temperature update frequency
    static constexpr uint32_t TEMPERATURE_SAMPLE_FREQ_HZ = 100;
    // Samples to average (improves accuracy ~32x)
    static constexpr uint32_t ADC_OVERSAMPLING_COUNT = 200;
    // Ring buffer size for additional smoothing
    static constexpr uint32_t TEMPERATURE_RING_BUFFER_SIZE = 10;

    static constexpr size_t ADC_INTERPOLATOR_LUT_SIZE_MAX = 100;

    using EEBuffer = etl::vector<uint8_t, 256>;

    static constexpr uint32_t UNKNOWN_TEMPERATURE_X10 = 10'000 * 10;

    Head();
    void setup();

    bool get_head_params_pb(std::vector<uint8_t>& pb_data);
    bool set_head_params_pb(const std::vector<uint8_t>& pb_data);
    bool get_head_params_pb(EEBuffer& pb_data);
    bool set_head_params_pb(const EEBuffer& pb_data);
    bool get_head_params(HeadParams& params, bool skip_status_check = false);
    bool set_head_params(const HeadParams& params);

    auto get_head_status() const -> HeadStatus { return head_status.load(); }
    int32_t get_temperature_x10();

    void update_sensor_uv();
    void configure_temperature_processor();

    etl::atomic<HeadStatus> head_status{HeadStatus_HeadDisconnected};
    uint32_t debounce_start{0};
    etl::atomic<SensorType> sensor_type{SensorType_RTD};
    etl::atomic<uint32_t> last_sensor_value_uv{SENSOR_FLOATING_LEVEL_MV * 1000};

    EepromStore eeprom_store{};
    DataGuard<EEBuffer> head_params{};
    TemperatureProcessor temperature_processor{};

private:
    void task_loop();

    bool is_attached() const {
        return get_head_status() == HeadStatus_HeadConnected;
    };

    void adc_init();
    void build_adc_lut();
    static bool IRAM_ATTR adc_conv_done_callback(adc_continuous_handle_t handle,
                                                const adc_continuous_evt_data_t *edata,
                                                void *user_data);

    adc_continuous_handle_t adc_handle{nullptr};
    adc_cali_handle_t adc_cali_handle{nullptr};
    AdcInterpolator<ADC_INTERPOLATOR_LUT_SIZE_MAX> adc_interpolator;

    // Temperature ring buffer for final smoothing (stores avg_x100)
    etl::array<uint32_t, TEMPERATURE_RING_BUFFER_SIZE> temp_ring_buffer{};
    uint8_t ring_buffer_idx{0};
    etl::atomic<uint8_t> ring_buffer_count{0};
};

extern Head head;
