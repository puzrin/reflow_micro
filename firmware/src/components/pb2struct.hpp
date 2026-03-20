#include <pb_encode.h>
#include <pb_decode.h>
#include <etl/vector.h>

template <typename T>
auto pb2struct(const etl::ivector<uint8_t>& pb_data, T& obj, const pb_msgdesc_t* fields_descriptor) -> bool {
    pb_istream_t stream = pb_istream_from_buffer(pb_data.data(), pb_data.size());
    return pb_decode(&stream, fields_descriptor, &obj);
}

template <typename T>
auto struct2pb(const T& obj, etl::ivector<uint8_t>& pb_data, const pb_msgdesc_t* fields_descriptor) -> bool {
    size_t message_length;
    if (!pb_get_encoded_size(&message_length, fields_descriptor, &obj)) { return false; }
    if (message_length > pb_data.max_size()) { return false; }
    pb_data.resize(message_length);

    pb_ostream_t stream = pb_ostream_from_buffer(pb_data.data(), pb_data.size());
    if (!pb_encode(&stream, fields_descriptor, &obj)) { return false; }
    pb_data.resize(stream.bytes_written);
    return true;
}
