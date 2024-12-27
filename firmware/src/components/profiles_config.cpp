#include "profiles_config.hpp"
#include "components/pb2struct.hpp"


bool ProfilesConfig::get_profiles(std::vector<uint8_t>& pb_data) {
    // Struct can be big, use heap instead of stack
    auto profiles_data = new ProfilesData();

    get_profiles(*profiles_data);
    bool status = struct2pb(*profiles_data, pb_data, ProfilesData_fields, ProfilesData_size);

    delete profiles_data;
    return status;
}

bool ProfilesConfig::get_profiles(ProfilesData& profiles_config) {
    bool status = pb2struct(unselected_profiles_store.get(), profiles_config, ProfilesData_fields);
    profiles_config.selectedId = selection_store.get();

    adjustSelection(profiles_config);
    return status;
}

bool ProfilesConfig::set_profiles(const std::vector<uint8_t>& pb_data) {
    // Struct can be big, use heap instead of stack
    auto profiles_data = new ProfilesData();

    bool status = pb2struct(pb_data, *profiles_data, ProfilesData_fields);
    status = status && set_profiles(*profiles_data);

    delete profiles_data;
    return status;
}

bool ProfilesConfig::set_profiles(const ProfilesData& profiles_config) {
    auto profiles_unselected = new ProfilesData(profiles_config);

    selection_store.set(profiles_config.selectedId);
    profiles_unselected->selectedId = -1;

    std::vector<uint8_t> buffer_pb(ProfilesData_size);

    bool status = struct2pb(*profiles_unselected, buffer_pb, ProfilesData_fields, ProfilesData_size);
    unselected_profiles_store.set(buffer_pb);

    delete profiles_unselected;
    return status;
}

void ProfilesConfig::adjustSelection(ProfilesData& profiles_config) {
    if (profiles_config.items_count == 0) {
        profiles_config.selectedId = -1;
        return;
    }

    for (uint32_t i = 0; i < profiles_config.items_count; i++) {
        if (profiles_config.items[i].id == profiles_config.selectedId) return;
    }

    profiles_config.selectedId = profiles_config.items[0].id;
}