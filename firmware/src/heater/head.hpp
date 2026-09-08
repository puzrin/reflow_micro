#pragma once

#include <etl/atomic.h>
#include <etl/vector.h>
#include <pd/utils/afsm.h>

#include "components/eeprom_store.hpp"
#include "components/temperature_processor.hpp"
#include "lib/data_guard.hpp"
#include "proto/generated/types.pb.h"

class Head : public afsm::fsm<Head> {
public:
    using EEBuffer = etl::vector<uint8_t, 256>;

    static constexpr uint32_t UNKNOWN_TEMPERATURE_X10 = 10'000 * 10;

    Head();
    void setup();

    bool get_head_params_pb(etl::ivector<uint8_t>& pb_data);
    bool set_head_params_pb(const etl::ivector<uint8_t>& pb_data);
    bool get_head_params(HeadParams& params, bool skip_status_check = false);
    bool set_head_params(const HeadParams& params);

    auto get_head_status() const -> HeadStatus { return head_status.load(); }
    int32_t get_temperature_x10();

    void configure_temperature_processor();

    etl::atomic<HeadStatus> head_status{HeadStatus_HEAD_DISCONNECTED};

    EepromStore eeprom_store{};
    DataGuard<EEBuffer> head_params{};
    TemperatureProcessor temperature_processor{};
    uint8_t probe_fail_count{0};

private:
    void task_loop();

    bool is_attached() const {
        return get_head_status() == HeadStatus_HEAD_CONNECTED;
    };
};

extern Head head;
