#include "prefs.hpp"
#include "proto/generated/types.pb.h"
#include "proto/generated/defaults.hpp"

class ProfilesConfig {
public:
    bool get_profiles(std::vector<uint8_t>& pb_data);
    bool get_profiles(ProfilesData& profiles);
    bool set_profiles(const std::vector<uint8_t>& pb_data);
    bool set_profiles(const ProfilesData& profiles);

    bool get_selected_profile(Profile& profile);

private:
    AsyncPreference<std::vector<uint8_t>> unselected_profiles_store{
        PrefsWriter::getInstance(),
        AsyncPreferenceKV::getInstance(),
        PREFS_NAMESPACE,
        "prf_unselected",
        std::vector<uint8_t>{std::begin(DEFAULT_PROFILES_DATA_UNSELECTED_PB), std::end(DEFAULT_PROFILES_DATA_UNSELECTED_PB)}
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
