#pragma once

#include <stddef.h>
#include <stdint.h>

#include <cstring>
#include <cmath>

#include <cbor.h>
#include <etl/delegate.h>
#include <etl/limits.h>
#include <etl/string.h>
#include <etl/string_view.h>
#include <etl/vector.h>

namespace cbor_rpc_dispatcher {

template <size_t MaxPayloadSize>
class ResponseWriter {
public:
    explicit ResponseWriter(etl::vector<uint8_t, MaxPayloadSize>& output)
        : output(output) {}

    auto did_respond() const -> bool { return responded; }

    auto write_bool(bool value) -> bool {
        return write_success([value](CborEncoder& encoder) {
            return cbor_encode_boolean(&encoder, value);
        });
    }

    auto write_uint8(uint8_t value) -> bool { return write_uint(value); }
    auto write_uint16(uint16_t value) -> bool { return write_uint(value); }
    auto write_uint32(uint32_t value) -> bool { return write_uint(value); }
    auto write_uint64(uint64_t value) -> bool { return write_uint(value); }

    auto write_uint(uint64_t value) -> bool {
        return write_success([value](CborEncoder& encoder) {
            return cbor_encode_uint(&encoder, value);
        });
    }

    auto write_int8(int8_t value) -> bool { return write_int(value); }
    auto write_int16(int16_t value) -> bool { return write_int(value); }
    auto write_int32(int32_t value) -> bool { return write_int(value); }
    auto write_int64(int64_t value) -> bool { return write_int(value); }

    auto write_int(int64_t value) -> bool {
        return write_success([value](CborEncoder& encoder) {
            return cbor_encode_int(&encoder, value);
        });
    }

    auto write_float(float value) -> bool {
        return write_success([value](CborEncoder& encoder) {
            return cbor_encode_float(&encoder, value);
        });
    }

    auto write_string(const char* value, size_t size) -> bool {
        return write_string(etl::string_view(value, size));
    }

    auto write_string(const char* value) -> bool {
        return write_string(etl::string_view(value));
    }

    auto write_string(etl::string_view value) -> bool {
        return write_success([value](CborEncoder& encoder) {
            return cbor_encode_text_string(&encoder, value.data(), value.size());
        });
    }

    auto write_binary(const uint8_t* data, size_t size) -> bool {
        return write_success([data, size](CborEncoder& encoder) {
            return cbor_encode_byte_string(&encoder, data, size);
        });
    }

    template <size_t Size>
    auto write_binary(const etl::vector<uint8_t, Size>& data) -> bool {
        return write_binary(data.data(), data.size());
    }

    auto write_error(const char* error) -> bool {
        return write_error(etl::string_view(error));
    }

    auto write_error(const char* error, size_t size) -> bool {
        return write_error(etl::string_view(error, size));
    }

    auto write_error(etl::string_view error) -> bool {
        return write_envelope(false, "error", [error](CborEncoder& encoder) {
            return cbor_encode_text_string(&encoder, error.data(), error.size());
        });
    }

private:
    etl::vector<uint8_t, MaxPayloadSize>& output;
    bool responded{false};

    template <typename EncodeValueFn>
    auto write_success(EncodeValueFn&& encode_value) -> bool {
        return write_envelope(true, "result", std::forward<EncodeValueFn>(encode_value));
    }

    template <typename EncodeValueFn>
    auto write_envelope(bool ok, const char* key, EncodeValueFn&& encode_value) -> bool {
        output.clear();
        output.resize(output.max_size());

        CborEncoder encoder;
        CborEncoder map;
        cbor_encoder_init(&encoder, output.data(), output.size(), 0);

        CborError error = cbor_encoder_create_map(&encoder, &map, 2);
        if (error == CborNoError) error = cbor_encode_text_stringz(&map, "ok");
        if (error == CborNoError) error = cbor_encode_boolean(&map, ok);
        if (error == CborNoError) error = cbor_encode_text_stringz(&map, key);
        if (error == CborNoError) error = encode_value(map);
        if (error == CborNoError) error = cbor_encoder_close_container_checked(&encoder, &map);

        if (error != CborNoError) {
            output.clear();
            responded = false;
            return false;
        }

        output.resize(cbor_encoder_get_buffer_size(&encoder, output.data()));
        responded = true;
        return true;
    }
};

class ParamsReader {
public:
    explicit ParamsReader(const CborValue& params)
        : params(params) {}

    auto is_valid() const -> bool { return cbor_value_is_array(&params); }

    auto count(size_t& count) const -> bool {
        if (!is_valid()) {
            return false;
        }

        return cbor_value_get_array_length(&params, &count) == CborNoError;
    }

    auto has_count(size_t expected_count) const -> bool {
        size_t actual_count = 0;
        return count(actual_count) && actual_count == expected_count;
    }

    auto get_bool(size_t index, bool& value) const -> bool {
        CborValue item;
        if (!get_item(index, item) || !cbor_value_is_boolean(&item)) {
            return false;
        }

        return cbor_value_get_boolean(&item, &value) == CborNoError;
    }

    auto get_float(size_t index, float& value) const -> bool {
        CborValue item;
        if (!get_item(index, item)) {
            return false;
        }

        if (cbor_value_is_float(&item)) {
            return cbor_value_get_float(&item, &value) == CborNoError;
        }

        if (cbor_value_is_double(&item)) {
            double double_value = 0;
            if (cbor_value_get_double(&item, &double_value) != CborNoError) {
                return false;
            }
            value = static_cast<float>(double_value);
            return true;
        }

        if (cbor_value_is_integer(&item)) {
            int64_t int_value = 0;
            if (cbor_value_get_int64_checked(&item, &int_value) != CborNoError) {
                return false;
            }
            value = static_cast<float>(int_value);
            return true;
        }

        return false;
    }

    auto get_int32(size_t index, int32_t& value) const -> bool {
        return get_signed_integer(index, value);
    }

    auto get_int16(size_t index, int16_t& value) const -> bool {
        return get_signed_integer(index, value);
    }

    auto get_int8(size_t index, int8_t& value) const -> bool {
        return get_signed_integer(index, value);
    }

    auto get_int64(size_t index, int64_t& value) const -> bool {
        return get_signed_integer(index, value);
    }

    auto get_uint32(size_t index, uint32_t& value) const -> bool {
        return get_unsigned_integer(index, value);
    }

    auto get_uint16(size_t index, uint16_t& value) const -> bool {
        return get_unsigned_integer(index, value);
    }

    auto get_uint8(size_t index, uint8_t& value) const -> bool {
        return get_unsigned_integer(index, value);
    }

    auto get_uint64(size_t index, uint64_t& value) const -> bool {
        return get_unsigned_integer(index, value);
    }

    template <size_t MaxSize>
    auto get_binary(size_t index, etl::vector<uint8_t, MaxSize>& output) const -> bool {
        CborValue item;
        if (!get_item(index, item) || !skip_tags(item) || !cbor_value_is_byte_string(&item)) {
            return false;
        }

        size_t size = 0;
        if (cbor_value_calculate_string_length(&item, &size) != CborNoError || size > output.max_size()) {
            return false;
        }

        output.resize(size);
        CborValue next;
        return cbor_value_copy_byte_string(&item, output.data(), &size, &next) == CborNoError;
    }

    template <size_t MaxSize>
    auto get_text(size_t index, etl::string<MaxSize>& output) const -> bool {
        CborValue item;
        if (!get_item(index, item) || !cbor_value_is_text_string(&item)) {
            return false;
        }

        size_t size = 0;
        if (cbor_value_calculate_string_length(&item, &size) != CborNoError || size > output.max_size() - 1) {
            return false;
        }

        output.resize(size);
        CborValue next;
        if (cbor_value_copy_text_string(&item, output.data(), &size, &next) != CborNoError) {
            output.clear();
            return false;
        }

        output.resize(size);
        return true;
    }

private:
    CborValue params;

    template <typename TInt>
    auto get_signed_integer(size_t index, TInt& value) const -> bool {
        CborValue item;
        if (!get_item(index, item)) {
            return false;
        }

        int64_t decoded = 0;
        if (cbor_value_is_integer(&item)) {
            if (cbor_value_get_int64_checked(&item, &decoded) != CborNoError) {
                return false;
            }
        } else {
            double double_value = 0;
            if (!get_integral_double(item, double_value)) {
                return false;
            }
            if (double_value < static_cast<double>(etl::numeric_limits<TInt>::min()) ||
                double_value > static_cast<double>(etl::numeric_limits<TInt>::max())) {
                return false;
            }
            decoded = static_cast<int64_t>(double_value);
        }

        if (decoded < static_cast<int64_t>(etl::numeric_limits<TInt>::min()) ||
            decoded > static_cast<int64_t>(etl::numeric_limits<TInt>::max())) {
            return false;
        }

        value = static_cast<TInt>(decoded);
        return true;
    }

    template <typename TUInt>
    auto get_unsigned_integer(size_t index, TUInt& value) const -> bool {
        CborValue item;
        if (!get_item(index, item)) {
            return false;
        }

        uint64_t decoded = 0;
        if (cbor_value_is_unsigned_integer(&item)) {
            if (cbor_value_get_uint64(&item, &decoded) != CborNoError) {
                return false;
            }
        } else {
            double double_value = 0;
            if (!get_integral_double(item, double_value) ||
                double_value < 0 ||
                double_value > static_cast<double>(etl::numeric_limits<TUInt>::max())) {
                return false;
            }
            decoded = static_cast<uint64_t>(double_value);
        }

        if (decoded > static_cast<uint64_t>(etl::numeric_limits<TUInt>::max())) {
            return false;
        }

        value = static_cast<TUInt>(decoded);
        return true;
    }

    static auto skip_tags(CborValue& item) -> bool {
        return cbor_value_skip_tag(&item) == CborNoError;
    }

    static auto get_integral_double(CborValue& item, double& value) -> bool {
        if (cbor_value_is_double(&item)) {
            if (cbor_value_get_double(&item, &value) != CborNoError) {
                return false;
            }
        } else if (cbor_value_is_float(&item)) {
            float float_value = 0;
            if (cbor_value_get_float(&item, &float_value) != CborNoError) {
                return false;
            }
            value = float_value;
        } else {
            return false;
        }

        return std::isfinite(value) && std::trunc(value) == value;
    }

    auto get_item(size_t index, CborValue& item) const -> bool {
        if (!is_valid()) {
            return false;
        }

        CborValue iterator;
        if (cbor_value_enter_container(&params, &iterator) != CborNoError) {
            return false;
        }

        for (size_t i = 0; i < index; ++i) {
            if (cbor_value_at_end(&iterator) ||
                cbor_value_skip_tag(&iterator) != CborNoError ||
                cbor_value_advance(&iterator) != CborNoError) {
                return false;
            }
        }

        if (cbor_value_at_end(&iterator) || cbor_value_skip_tag(&iterator) != CborNoError) {
            return false;
        }

        item = iterator;
        return true;
    }
};

template <size_t MaxMethods, size_t MaxResponseSize, size_t MaxMethodNameLength = 48>
class Dispatcher {
public:
    using Response = ResponseWriter<MaxResponseSize>;
    using MethodHandler = etl::delegate<void(const ParamsReader&, Response&)>;

    auto addMethod(const char* name, const MethodHandler& handler) -> void {
        methods.push_back({name, handler});
    }

    auto dispatch(const etl::ivector<uint8_t>& input, etl::vector<uint8_t, MaxResponseSize>& output) -> void {
        Response response(output);

        try {
            CborParser parser;
            CborValue root;
            CborError error = cbor_parser_init(input.data(), input.size(), 0, &parser, &root);
            if (error != CborNoError || !cbor_value_is_map(&root)) {
                response.write_error("Invalid request");
                return;
            }

            etl::string<MaxMethodNameLength> method;
            CborValue method_value;
            error = cbor_value_map_find_value(&root, "method", &method_value);
            if (error != CborNoError || !copy_method_name(method_value, method)) {
                response.write_error("Invalid method");
                return;
            }

            CborValue params_value;
            error = cbor_value_map_find_value(&root, "params", &params_value);
            if (error != CborNoError || !cbor_value_is_array(&params_value)) {
                response.write_error("Invalid params");
                return;
            }

            for (const auto& entry : methods) {
                if (method == entry.name) {
                    entry.handler(ParamsReader(params_value), response);
                    if (!response.did_respond()) {
                        response.write_error("Internal error");
                    }
                    return;
                }
            }

            response.write_error("Method not found");
        } catch (...) {
            response.write_error("Internal error");
        }
    }

private:
    struct MethodEntry {
        const char* name;
        MethodHandler handler;
    };

    etl::vector<MethodEntry, MaxMethods> methods{};

    static auto copy_method_name(const CborValue& value, etl::string<MaxMethodNameLength>& method) -> bool {
        if (!cbor_value_is_text_string(&value)) {
            return false;
        }

        size_t size = 0;
        if (cbor_value_calculate_string_length(&value, &size) != CborNoError || size > method.max_size() - 1) {
            return false;
        }

        method.resize(size);
        CborValue next;
        if (cbor_value_copy_text_string(&value, method.data(), &size, &next) != CborNoError) {
            method.clear();
            return false;
        }

        method.resize(size);
        return true;
    }
};

} // namespace cbor_rpc_dispatcher
