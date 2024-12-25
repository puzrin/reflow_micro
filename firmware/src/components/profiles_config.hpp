#include "prefs.hpp"
#include "proto/generated/types.pb.h"
#include "proto/generated/defaults.hpp"

class ProfilesConfig {
public:
    ProfilesConfig()
        : unselected_profiles_store(&prefsWriter, prefsKV, "profiles", "unselected_data",
            std::vector<uint8_t>{std::begin(DEFAULT_PROFILES_DATA_UNSELECTED_PB), std::end(DEFAULT_PROFILES_DATA_UNSELECTED_PB)})
        , selection_store(&prefsWriter, prefsKV, "profiles", "selection",
            DEFAULT_PROFILES_SELECTION)
    {};

    std::vector<uint8_t> get_profiles_pb();
    void set_profiles_pb(const std::vector<uint8_t>& pb_data);
    void get_profiles(ProfilesData& profiles);
    void set_profiles(const ProfilesData& profiles);

private:
    AsyncPreference<std::vector<uint8_t>> unselected_profiles_store;
    AsyncPreference<int32_t> selection_store;
    void adjustSelection(ProfilesData& profiles_config);
    void pb2struct(const std::vector<uint8_t>& pb_data, ProfilesData& profiles_config);
    void struct2pb(const ProfilesData& profiles_config, std::vector<uint8_t>& pb_data);
};
