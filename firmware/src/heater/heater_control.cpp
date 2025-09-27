#include "components/pb2struct.hpp"
#include "head.hpp"
#include "heater_control.hpp"
#include "power.hpp"

void HeaterControl::setup() {
    static constexpr int32_t TICK_PERIOD_MS = 50;
    prev_tick_ms = get_time_ms() - TICK_PERIOD_MS;

    power.setup();
    head.setup();

    xTaskCreate(
        [](void* params) {
            auto* self = static_cast<HeaterControl*>(params);
            while (true) {
                self->tick();
                vTaskDelay(pdMS_TO_TICKS(TICK_PERIOD_MS));
            }
        }, "HeaterControl", 1024*4, this, 4, nullptr
    );
}

void HeaterControl::tick() {
    power.receive(MsgToPower_SysTick{});

    if (get_health_status() != DeviceHealthStatus_DevOK) {
        if (is_task_active) {
            application.enqueue_message(AppCmd::Stop{});
        }
        return;
    }

    // Pause control if PD profile in transition.
    if (get_power_status() != PowerStatus_PwrOK) { return; }

    HeaterControlBase::tick();
}

void HeaterControl::set_power(float power_w) {
    power.set_power_mw(static_cast<uint32_t>(power_w * 1000));
}

bool HeaterControl::get_head_params_pb(std::vector<uint8_t>& pb_data) {
    return head.get_head_params_pb(pb_data);
}

bool HeaterControl::set_head_params_pb(const std::vector<uint8_t>& pb_data) {
    return head.set_head_params_pb(pb_data);
}

bool HeaterControl::get_head_params(HeadParams& params) {
    Head::EEBuffer pb_data{};
    if (!head.get_head_params_pb(pb_data)) { return false; }

    return pb2struct(pb_data, params, HeadParams_fields);
}

bool HeaterControl::set_head_params(const HeadParams& params) {
    Head::EEBuffer pb_data{};
    if (!struct2pb(params, pb_data, HeadParams_fields, HeadParams_size)) { return false; }

    head.set_head_params_pb(pb_data);
    return true;
}

auto HeaterControl::get_health_status() -> DeviceHealthStatus {
    auto power_status = power.get_power_status();
    auto head_status = get_head_status();

    if ((power_status == PowerStatus_PwrOK || power_status == PowerStatus_PwrTransition) &&
        (head_status == HeadStatus_HeadConnected))
    {
        return DeviceHealthStatus_DevOK;
    }

    if (power_status >= PowerStatus_PwrFailure || head_status >= HeadStatus_HeadError) {
        return DeviceHealthStatus_DevFailure;
    }
    return DeviceHealthStatus_DevNotReady;
}

auto HeaterControl::get_activity_status() -> DeviceActivityStatus {
    return static_cast<DeviceActivityStatus>(application.get_state_id());
}

auto HeaterControl::get_power_status() -> PowerStatus {
    return power.get_power_status();
}

auto HeaterControl::get_head_status() -> HeadStatus {
    return head.get_head_status();
}

auto HeaterControl::get_temperature() -> float {
    return head.get_temperature_x10() * 0.1f;
}

auto HeaterControl::get_volts() -> float {
    return power.get_peak_mv() * 0.001f;
}

auto HeaterControl::get_amperes() -> float {
    return power.get_peak_ma() * 0.001f;
}

auto HeaterControl::get_duty_cycle() -> float {
    return power.get_duty_x1000() * 0.001f;
}

auto HeaterControl::get_power() -> float {
    return get_volts() * get_amperes() * power.get_duty_x1000() * 0.001f;
}

auto HeaterControl::get_resistance() -> float {
    auto r_millis = power.get_load_mohm();
    if (r_millis == etl::numeric_limits<uint32_t>::max()) {
        return etl::numeric_limits<float>::max();
    }
    return r_millis * 0.001f;
}

auto HeaterControl::get_max_power() -> float {
    power.lock();
    auto max_power = power.get_max_power_mw();
    power.unlock();
    return max_power * 0.001f;
}
