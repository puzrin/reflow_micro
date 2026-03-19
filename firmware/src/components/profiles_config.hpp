#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include <etl/vector.h>

#include "prefs.hpp"
#include "proto/generated/types.pb.h"
#include "proto/generated/defaults.hpp"

class ProfilesConfig {
public:
    ~ProfilesConfig() { vSemaphoreDelete(_lock); }

    auto get_profiles(etl::ivector<uint8_t>& pb_data) -> bool;
    auto get_profiles(ProfilesData& profiles) -> bool;
    auto set_profiles(const etl::ivector<uint8_t>& pb_data) -> bool;
    auto set_profiles(const ProfilesData& profiles) -> bool;

    auto get_selected_profile(Profile& profile) -> bool;
    auto reset_profiles() -> void;

private:
    static auto default_profiles_pb() -> etl::vector<uint8_t, ProfilesData_size>;

    void lock() { xSemaphoreTake(_lock, portMAX_DELAY); }
    void unlock() { xSemaphoreGive(_lock); }

    auto get_profiles_unlocked(ProfilesData& profiles) -> bool;
    auto set_profiles_unlocked(const ProfilesData& profiles) -> bool;

    SemaphoreHandle_t _lock{xSemaphoreCreateMutex()};
    ProfilesData _scratch_profiles{};
    etl::vector<uint8_t, ProfilesData_size> _scratch_pb{};

    AsyncPreference<etl::vector<uint8_t, ProfilesData_size>> unselected_profiles_store{
        PrefsWriter::getInstance(),
        AsyncPreferenceKV::getInstance(),
        PREFS_NAMESPACE,
        "prf_unselected",
        default_profiles_pb()
    };
    AsyncPreference<int32_t> selection_store{
        PrefsWriter::getInstance(),
        AsyncPreferenceKV::getInstance(),
        PREFS_NAMESPACE,
        "prf_selection",
        DEFAULT_PROFILES_SELECTION
    };
    void adjustSelection(ProfilesData& profiles_config);
};

extern ProfilesConfig profiles_config;
