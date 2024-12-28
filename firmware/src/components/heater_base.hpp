#include <vector>
#include <atomic>
#include <functional>
#include "prefs.hpp"
#include "proto/generated/types.pb.h"
#include "proto/generated/defaults.hpp"
#include "lib/adrc.hpp"
#include "history.hpp"

using HeaterTaskIteratorFn = std::function<void(uint32_t, uint32_t)>;

class HeaterBase {
private:
    AsyncPreferenceMap<std::vector<uint8_t>> adrc_params;
    AsyncPreferenceMap<std::vector<uint8_t>> sensor_params;

protected:
    ADRC adrc;
    std::atomic<bool> temperature_control_flag;
    std::atomic<float> power_setpoint;
    std::atomic<float> temperature_setpoint;

public:
    HeaterBase()
        : adrc_params(&prefsWriter, prefsKV, "adrc", "", std::vector<uint8_t>{std::begin(DEFAULT_ADRC_PARAMS_PB), std::end(DEFAULT_ADRC_PARAMS_PB)})
        , sensor_params(&prefsWriter, prefsKV, "sensor", "", std::vector<uint8_t>{std::begin(DEFAULT_SENSOR_PARAMS_PB), std::end(DEFAULT_SENSOR_PARAMS_PB)})
        , temperature_control_flag(false)
        , power_setpoint(0)
        , temperature_setpoint(0)
    {};

    virtual bool is_hotplate_connected() { return true;};
    virtual uint8_t get_hotplate_id() { return 0; };

    bool get_adrc_params(std::vector<uint8_t>& pb_data);
    bool get_adrc_params(AdrcParams& params);
    bool set_adrc_params(const std::vector<uint8_t>& pb_data);
    bool set_adrc_params(const AdrcParams& params);

    bool get_sensor_params(std::vector<uint8_t>& pb_data);
    bool get_sensor_params(SensorParams& params);
    bool set_sensor_params(const std::vector<uint8_t>& pb_data);
    bool set_sensor_params(const SensorParams& params);

    void get_history(int32_t client_history_version, int32_t from, std::vector<uint8_t>& pb_data);

    virtual void start() = 0;
    virtual bool load_all_params();

    virtual float get_temperature() = 0;
    virtual float get_resistance() = 0;
    virtual float get_max_power() = 0;
    virtual float get_power() = 0;
    virtual float get_volts() = 0;
    virtual float get_amperes() = 0;
    virtual float get_duty_cycle() { return 1.0f; }

    virtual void set_power(float power) { power_setpoint = (power < 0 ? 0 : power); }
    virtual void set_temperature(float temp) { temperature_setpoint = temp; }
    virtual void temperature_control_on();
    virtual void temperature_control_off();

    virtual void tick(int32_t dt_ms);
    virtual bool set_sensor_calibration_point(uint32_t point_id, float temperature) = 0;

    // "task" machinery, by default record history.

    bool task_start(int32_t task_id, HeaterTaskIteratorFn task_iterator = nullptr);
    void task_stop();

private:
    std::atomic<bool> is_task_active = false;
    HeaterTaskIteratorFn task_iterator = nullptr;
    int32_t task_time_ms = 0;
    History history;
    int32_t history_version = 0;
    int32_t history_task_id = 0;
    int32_t history_last_recorded_ts = 0;
    static constexpr int32_t history_y_multiplier = 256;
    static constexpr float history_y_multiplier_inv = 1.0f / history_y_multiplier;
};
