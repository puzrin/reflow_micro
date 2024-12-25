#include "profiles_config.hpp"
#include "pb_decode.h"
#include "pb_encode.h"

void ProfilesConfig::pb2struct(const std::vector<uint8_t>& pb_data, ProfilesData& profiles_config) {
    pb_istream_t stream = pb_istream_from_buffer(pb_data.data(), pb_data.size());
    pb_decode(&stream, ProfilesData_fields, &profiles_config);
}

void ProfilesConfig::struct2pb(const ProfilesData& profiles_config, std::vector<uint8_t>& pb_data) {
    pb_data.resize(ProfilesData_size);
    pb_ostream_t stream = pb_ostream_from_buffer(pb_data.data(), pb_data.size());
    pb_encode(&stream, ProfilesData_fields, &profiles_config);
    pb_data.resize(stream.bytes_written);
}

std::vector<uint8_t> ProfilesConfig::get_profiles_pb() {
    std::vector<uint8_t> buffer_pb(ProfilesData_size);
    auto profiles_data = new ProfilesData();

    get_profiles(*profiles_data);
    struct2pb(*profiles_data, buffer_pb);

    delete profiles_data;
    return buffer_pb;
}

void ProfilesConfig::set_profiles_pb(const std::vector<uint8_t>& pb_data) {
    auto profiles_data = new ProfilesData();

    pb2struct(pb_data, *profiles_data);
    set_profiles(*profiles_data);

    delete profiles_data;
}

void ProfilesConfig::get_profiles(ProfilesData& profiles_config) {
    pb2struct(unselected_profiles_store.get(), profiles_config);
    profiles_config.selectedId = selection_store.get();
    adjustSelection(profiles_config);
}

void ProfilesConfig::set_profiles(const ProfilesData& profiles_config) {
    auto profiles_unselected = new ProfilesData(profiles_config);

    selection_store.set(profiles_config.selectedId);
    profiles_unselected->selectedId = -1;

    std::vector<uint8_t> buffer_pb(ProfilesData_size);

    struct2pb(*profiles_unselected, buffer_pb);
    unselected_profiles_store.set(buffer_pb);

    delete profiles_unselected;
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