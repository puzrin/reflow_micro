#include "components/blinker.hpp"
#include "components/fan.hpp"
#include "components/led_colors.hpp"
#include "components/pb2struct.hpp"
#include "head.hpp"
#include "heater_control.hpp"
#include "power.hpp"

void HeaterControl::setup() {
    static constexpr int32_t TICK_PERIOD_MS = 50;

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
    update_fan_speed();
    update_temperature_indicator();

    if (get_health_status() != DeviceHealthStatus_DevOK) {
        if (is_task_active.load()) {
            application.enqueue_message(AppCmd::Stop{});
        }
    }

    HeaterControlBase::tick();
}

void HeaterControl::set_power(float power_w) {
    power.set_power_mw(static_cast<uint32_t>(power_w * 1000));
}

bool HeaterControl::get_head_params_pb(std::vector<uint8_t>& pb_data) {
    return head.get_head_params_pb(pb_data);
}

bool HeaterControl::set_head_params_pb(const std::vector<uint8_t>& pb_data) {
    if (!head.set_head_params_pb(pb_data)) { return false; }
    // Refetch ADRC params. Useful for calibration experiments.
    load_all_params();
    return true;
}

bool HeaterControl::get_head_params(HeadParams& params) {
    return head.get_head_params(params);
}

bool HeaterControl::set_head_params(const HeadParams& params) {
    if (!head.set_head_params(params)) { return false; }
    // Refetch ADRC params. Useful for calibration experiments.
    load_all_params();
    return true;
}

bool HeaterControl::set_calibration_point_0(float temperature) {
    HeadParams params = HeadParams_init_zero;
    if (!get_head_params(params)) { return false; }

    if (!head.is_tcr_sensor()) {
        // RTD mode: use ADC voltage
        params.sensor_p0_value = head.last_sensor_value_uv.load();
    } else {
        // PCB mode: use heater resistance
        params.sensor_p0_value = power.get_load_mohm();
    }

    params.sensor_p0_at = temperature;
    return set_head_params(params);
}

bool HeaterControl::set_calibration_point_1(float temperature) {
    HeadParams params = HeadParams_init_zero;
    if (!get_head_params(params)) { return false; }

    if (!head.is_tcr_sensor()) {
        // RTD mode: use ADC voltage
        params.sensor_p1_value = head.last_sensor_value_uv.load();
    } else {
        // PCB mode: use heater resistance
        params.sensor_p1_value = power.get_load_mohm();
    }

    params.sensor_p1_at = temperature;
    return set_head_params(params);
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
    if (r_millis == Power::UNKNOWN_RESISTANCE) {
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

void HeaterControl::update_fan_speed() {
    constexpr int32_t C_DIFF_ON_X10 = 4 * 10;
    constexpr int32_t C_DIFF_OFF_X10 = 3 * 10;
    constexpr int32_t C_EDGE_ON_X10 = 40 * 10;

    int32_t temperature_x10 = head.get_temperature_x10();
    int32_t setpoint_x10 = lround(temperature_setpoint.load() * 10.0f);
    bool working = is_task_active.load();

    if (working) {
        // Setpoint is valid only when task is active AND temperature control enabled
        if (temperature_control_enabled.load()) {
            //
            // We should solve 2 problems:
            // - Cool down reasonable fast
            // - Avoid interference with ADRC control.
            //
            // Use simple logic with safe threshold and small hysteresis
            // - If temperature > 4C above desired => full speed
            // - If temperature < 3C above desired => off
            //
            // This is simple and should be ok. If not - can be improved later.
            //
            if (temperature_x10 > setpoint_x10 + C_DIFF_ON_X10) {
                // Enable fan ONLY when ADRC output is about zero,
                // to avoid interference.
                if (power.get_target_power_mw() < 1*1000) {
                    fan.max();
                }
            }
            if (temperature_x10 < setpoint_x10 + C_DIFF_OFF_X10) { fan.off(); }
        } else {
            // If task is working, but without temperature control - disable fan.
            // This is valid scenario for calibration-related things.
            fan.off();
        }
    } else {
        // No task => always cool down to low temperature, if head attached
        if (head.get_head_status() == HeadStatus_HeadConnected &&
            temperature_x10 != head.UNKNOWN_TEMPERATURE_X10)
        {
            if (temperature_x10 > C_EDGE_ON_X10) { fan.max(); }
            else { fan.off(); }
        }
        else {
            // Fan off when no head OR in TCR mode without power
            // (with "unknown" temperature, powered via debug connector)
            fan.off();
        }
    }
}

void HeaterControl::update_temperature_indicator() {
    constexpr int32_t T_WARM = 40;
    constexpr int32_t T_HOT = 80;
    constexpr int32_t T_VERY_HOT = 150;

    int32_t temperature_x10 = head.get_temperature_x10();
    if (temperature_x10 == head.UNKNOWN_TEMPERATURE_X10) {
        blinker.background(LCD_OK_COLOR);
        return;
    }

    int32_t t = (temperature_x10 + 5) / 10;
    Blinker::DataType color{};

    if (t <= T_WARM)
    {
        if (is_task_active.load()) { color = LCD_WARM_COLOR; }
        else { color = LCD_OK_COLOR; }
    }
    else if (t <= T_HOT)
    {
        color = blinker.interpolate(T_WARM, LCD_WARM_COLOR, T_HOT, LCD_HOT_COLOR, t);
    }
    else if (t <= T_VERY_HOT)
    {
        color = blinker.interpolate(T_HOT, LCD_HOT_COLOR, T_VERY_HOT, LCD_VERY_HOT_COLOR, t);
    }
    else { color = LCD_VERY_HOT_COLOR; }

    blinker.background(color);
}
