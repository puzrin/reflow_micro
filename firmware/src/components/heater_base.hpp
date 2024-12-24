#include <vector>
#include <atomic>
#include "prefs.hpp"
#include "proto/generated/types.pb.h"
#include "proto/generated/defaults.hpp"
#include "lib/adrc.hpp"

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

    std::vector<uint8_t> get_adrc_params_pb();
    bool set_adrc_params_pb(const std::vector<uint8_t>& pb_data);
    AdrcParams get_adrc_params();
    bool set_adrc_params(const AdrcParams& params);

    std::vector<uint8_t> get_sensor_params_pb();
    bool set_sensor_params_pb(const std::vector<uint8_t>& pb_data);
    SensorParams get_sensor_params();
    bool set_sensor_params(const SensorParams& params);

    virtual void load_all_params();

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

    virtual void iterate(float dt);
    virtual bool set_sensor_calibration_point(uint32_t point_id, float temperature) = 0;
};
