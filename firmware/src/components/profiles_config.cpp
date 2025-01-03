#include "profiles_config.hpp"
#include "components/pb2struct.hpp"
#include <memory>


auto ProfilesConfig::get_profiles(std::vector<uint8_t>& pb_data) -> bool {
    // Struct can be big, use heap instead of stack
    auto profiles_data = std::make_unique<ProfilesData>();

    get_profiles(*profiles_data);
    return struct2pb(*profiles_data, pb_data, ProfilesData_fields, ProfilesData_size);
}

auto ProfilesConfig::get_profiles(ProfilesData& profiles_config) -> bool {
    bool status = pb2struct(unselected_profiles_store.get(), profiles_config, ProfilesData_fields);
    profiles_config.selectedId = selection_store.get();

    adjustSelection(profiles_config);
    return status;
}

auto ProfilesConfig::set_profiles(const std::vector<uint8_t>& pb_data) -> bool {
    auto profiles_data = std::make_unique<ProfilesData>();

    if (!pb2struct(pb_data, *profiles_data, ProfilesData_fields)) { return false; }
    return set_profiles(*profiles_data);
}

auto ProfilesConfig::set_profiles(const ProfilesData& profiles_config) -> bool {
    auto profiles_unselected = std::make_unique<ProfilesData>(profiles_config);

    auto selection = profiles_config.selectedId;
    profiles_unselected->selectedId = -1;

    std::vector<uint8_t> buffer_pb(ProfilesData_size);
    if (!struct2pb(*profiles_unselected, buffer_pb, ProfilesData_fields, ProfilesData_size)) { return false; }

    unselected_profiles_store.set(buffer_pb);
    selection_store.set(selection);
    return true;
}

void ProfilesConfig::adjustSelection(ProfilesData& profiles_config) {
    if (profiles_config.items_count == 0) {
        profiles_config.selectedId = -1;
        return;
    }

    for (size_t i = 0; i < profiles_config.items_count; i++) {
        if (profiles_config.items[i].id == profiles_config.selectedId) { return; }
    }

    profiles_config.selectedId = profiles_config.items[0].id;
}

auto ProfilesConfig::get_selected_profile(Profile& profile) -> bool {
    auto profiles_data = std::make_unique<ProfilesData>();
    get_profiles(*profiles_data);

    for (size_t i = 0; i < profiles_data->items_count; i++) {
        if (profiles_data->items[i].id == profiles_data->selectedId) {
            profile = profiles_data->items[i];
            return true;
        }
    }
    return false;
}

auto ProfilesConfig::reset_profiles() -> void {
    unselected_profiles_store.set({std::begin(DEFAULT_PROFILES_DATA_UNSELECTED_PB), std::end(DEFAULT_PROFILES_DATA_UNSELECTED_PB)});
    selection_store.set(DEFAULT_PROFILES_SELECTION);
}