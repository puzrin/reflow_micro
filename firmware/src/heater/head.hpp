#pragma once

#include <esp_adc/adc_oneshot.h>
#include <esp_adc/adc_cali.h>
#include <etl/limits.h>
#include <etl/atomic.h>
#include <pd/utils/afsm.h>
#include <vector>

#include "components/eeprom_store.hpp"
#include "lib/data_guard.hpp"
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
    static constexpr uint32_t SENSOR_FLOATING_LEVEL_MV = 700;

    using EEBuffer = etl::vector<uint8_t, 256>;

    static constexpr float UNKNOWN_TEMPERATURE = etl::numeric_limits<float>::max();
    Head();
    void setup();

    bool is_attached() const;

    bool get_head_params_pb(std::vector<uint8_t>& pb_data);
    bool set_head_params_pb(const std::vector<uint8_t>& pb_data);
    bool get_head_params_pb(EEBuffer& pb_data);
    bool set_head_params_pb(const EEBuffer& pb_data);

    auto get_head_status() const -> HeadStatus { return head_status.load(); }
    int32_t get_temperature_x10() const;

    void update_sensor_mv();

    etl::atomic<HeadStatus> head_status{HeadStatus_HeadDisconnected};
    uint32_t debounce_start{0};
    etl::atomic<HeaterType> heater_type{HeaterType_MCH};
    etl::atomic<uint32_t> last_sensor_value_mv{SENSOR_FLOATING_LEVEL_MV};

    EepromStore eeprom_store{};
    DataGuard<EEBuffer> head_params{};

private:
    void task_loop();
    void adc_init();

    adc_oneshot_unit_handle_t adc1_handle{nullptr};
    adc_cali_handle_t adc_cali_handle{nullptr};
};

extern Head head;
