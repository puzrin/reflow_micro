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
public:
    virtual auto is_hotplate_connected() -> bool { return true;};
    virtual auto get_hotplate_id() -> uint8_t { return 0; };

    auto get_head_params(std::vector<uint8_t>& pb_data) -> bool;
    auto get_head_params(HeadParams& params) -> bool;
    auto set_head_params(const std::vector<uint8_t>& pb_data) -> bool;
    auto set_head_params(const HeadParams& params) -> bool;

    void get_history(int32_t client_history_version, float from, std::vector<uint8_t>& pb_data);

    virtual void start() = 0;
    virtual auto load_all_params() -> bool;

    virtual auto get_temperature() -> float = 0;
    virtual auto get_resistance() -> float = 0;
    virtual auto get_max_power() -> float = 0;
    virtual auto get_power() -> float = 0;
    virtual auto get_volts() -> float = 0;
    virtual auto get_amperes() -> float = 0;
    virtual auto get_duty_cycle() -> float { return 1.0F; }

    virtual void set_power(float power) { power_setpoint = (power < 0 ? 0 : power); }
    virtual void set_temperature(float temp) { temperature_setpoint = temp; }
    virtual void temperature_control_on();
    virtual void temperature_control_off();

    virtual void tick(int32_t dt_ms);

    // "task" machinery, by default record history.

    auto task_start(int32_t task_id, HeaterTaskIteratorFn task_iterator = nullptr) -> bool;
    void task_stop();

protected:
    ADRC adrc{};
    std::atomic<bool> temperature_control_flag{false};
    std::atomic<float> power_setpoint{0};
    std::atomic<float> temperature_setpoint{0};

private:
    AsyncPreference<std::vector<uint8_t>> head_params{
        PrefsWriter::getInstance(),
        AsyncPreferenceKV::getInstance(),
        PREFS_NAMESPACE,
        "head",
        std::vector<uint8_t>{std::begin(DEFAULT_HEAD_PARAMS_PB), std::end(DEFAULT_HEAD_PARAMS_PB)}
    };
    std::atomic<bool> is_task_active{false};
    HeaterTaskIteratorFn task_iterator{nullptr};
    int32_t task_time_ms{0};
    History history{};
    int32_t history_version{0};
    int32_t history_task_id{0};
    int32_t history_last_recorded_ts{0};
    static constexpr int32_t history_y_multiplier = 100;
    static constexpr float history_y_multiplier_inv = 1.0F / history_y_multiplier;
};
