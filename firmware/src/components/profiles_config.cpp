#include "profiles_config.hpp"
#include "components/pb2struct.hpp"

ProfilesConfig profiles_config;

auto ProfilesConfig::default_profiles_pb() -> etl::vector<uint8_t, ProfilesData_size> {
    etl::vector<uint8_t, ProfilesData_size> buffer{};
    buffer.assign(std::begin(DEFAULT_PROFILES_DATA_UNSELECTED_PB), std::end(DEFAULT_PROFILES_DATA_UNSELECTED_PB));
    return buffer;
}

auto ProfilesConfig::get_profiles(etl::ivector<uint8_t>& pb_data) -> bool {
    lock();
    const bool status = get_profiles_unlocked(_scratch_profiles) &&
        struct2pb(_scratch_profiles, pb_data, ProfilesData_fields);
    unlock();
    return status;
}

auto ProfilesConfig::get_profiles(ProfilesData& profiles_config) -> bool {
    lock();
    const bool status = get_profiles_unlocked(profiles_config);
    unlock();
    return status;
}

auto ProfilesConfig::get_profiles_unlocked(ProfilesData& profiles_config) -> bool {
    bool status = pb2struct(unselected_profiles_store.get(), profiles_config, ProfilesData_fields);
    profiles_config.selected_id = selection_store.get();

    adjustSelection(profiles_config);
    return status;
}

auto ProfilesConfig::set_profiles(const etl::ivector<uint8_t>& pb_data) -> bool {
    lock();
    const bool status = pb2struct(pb_data, _scratch_profiles, ProfilesData_fields) &&
        set_profiles_unlocked(_scratch_profiles);
    unlock();
    return status;
}

auto ProfilesConfig::set_profiles(const ProfilesData& profiles_config) -> bool {
    lock();
    const bool status = set_profiles_unlocked(profiles_config);
    unlock();
    return status;
}

auto ProfilesConfig::set_profiles_unlocked(const ProfilesData& profiles_config) -> bool {
    _scratch_profiles = profiles_config;
    auto selection = profiles_config.selected_id;
    _scratch_profiles.selected_id = -1;

    if (!struct2pb(_scratch_profiles, _scratch_pb, ProfilesData_fields)) { return false; }

    unselected_profiles_store.set(_scratch_pb);
    selection_store.set(selection);
    return true;
}

void ProfilesConfig::adjustSelection(ProfilesData& profiles_config) {
    if (profiles_config.items_count == 0) {
        profiles_config.selected_id = -1;
        return;
    }

    for (size_t i = 0; i < profiles_config.items_count; i++) {
        if (profiles_config.items[i].id == profiles_config.selected_id) { return; }
    }

    profiles_config.selected_id = profiles_config.items[0].id;
}

auto ProfilesConfig::get_selected_profile(Profile& profile) -> bool {
    lock();
    const bool loaded = get_profiles_unlocked(_scratch_profiles);
    if (!loaded) {
        unlock();
        return false;
    }

    for (size_t i = 0; i < _scratch_profiles.items_count; i++) {
        if (_scratch_profiles.items[i].id == _scratch_profiles.selected_id) {
            profile = _scratch_profiles.items[i];
            unlock();
            return true;
        }
    }
    unlock();
    return false;
}

auto ProfilesConfig::reset_profiles() -> void {
    lock();
    unselected_profiles_store.set(default_profiles_pb());
    selection_store.set(DEFAULT_PROFILES_SELECTION);
    unlock();
}
