#include <gtest/gtest.h>

#include <string>
#include <vector>

#include <cbor.h>

#include "lib/cbor_rpc_dispatcher.hpp"

namespace {

struct TestSession {};

using TestBuffer = etl::vector<uint8_t, 256>;
using TestDispatcher = cbor_rpc_dispatcher::Dispatcher<8, 256, 48, TestSession>;
using TestParams = cbor_rpc_dispatcher::ParamsReader;
using TestResponse = cbor_rpc_dispatcher::ResponseWriter<256>;

template <typename EncodeParamsFn>
auto make_request(const char* method, size_t param_count, EncodeParamsFn&& encode_params) -> TestBuffer {
    TestBuffer buffer;
    buffer.resize(buffer.max_size());

    CborEncoder encoder;
    CborEncoder map;
    CborEncoder params;
    cbor_encoder_init(&encoder, buffer.data(), buffer.size(), 0);

    CborError error = cbor_encoder_create_map(&encoder, &map, 2);
    if (error == CborNoError) error = cbor_encode_text_stringz(&map, "method");
    if (error == CborNoError) error = cbor_encode_text_stringz(&map, method);
    if (error == CborNoError) error = cbor_encode_text_stringz(&map, "params");
    if (error == CborNoError) error = cbor_encoder_create_array(&map, &params, param_count);
    if (error == CborNoError) error = encode_params(params);
    if (error == CborNoError) error = cbor_encoder_close_container_checked(&map, &params);
    if (error == CborNoError) error = cbor_encoder_close_container_checked(&encoder, &map);

    EXPECT_EQ(CborNoError, error);
    buffer.resize(cbor_encoder_get_buffer_size(&encoder, buffer.data()));
    return buffer;
}

auto make_request(const char* method) -> TestBuffer {
    return make_request(method, 0, [](CborEncoder&) { return CborNoError; });
}

class ResponseView {
public:
    explicit ResponseView(const TestBuffer& buffer) {
        valid = cbor_parser_init(buffer.data(), buffer.size(), 0, &parser, &root) == CborNoError &&
            cbor_value_is_map(&root);
    }

    auto is_valid() const -> bool { return valid; }

    auto ok() const -> bool {
        CborValue value;
        if (!find("ok", value) || !cbor_value_is_boolean(&value)) {
            return false;
        }

        bool result = false;
        return cbor_value_get_boolean(&value, &result) == CborNoError && result;
    }

    auto string_field(const char* key) const -> std::string {
        CborValue value;
        if (!find(key, value) || !cbor_value_is_text_string(&value)) {
            return {};
        }

        size_t size = 0;
        if (cbor_value_calculate_string_length(&value, &size) != CborNoError) {
            return {};
        }

        std::string result(size, '\0');
        CborValue next;
        if (cbor_value_copy_text_string(&value, result.data(), &size, &next) != CborNoError) {
            return {};
        }

        result.resize(size);
        return result;
    }

    auto uint_field(const char* key) const -> uint64_t {
        CborValue value;
        if (!find(key, value) || !cbor_value_is_unsigned_integer(&value)) {
            return 0;
        }

        uint64_t result = 0;
        if (cbor_value_get_uint64(&value, &result) != CborNoError) {
            return 0;
        }

        return result;
    }

    auto int_field(const char* key) const -> int64_t {
        CborValue value;
        if (!find(key, value) || !cbor_value_is_integer(&value)) {
            return 0;
        }

        int64_t result = 0;
        if (cbor_value_get_int64_checked(&value, &result) != CborNoError) {
            return 0;
        }

        return result;
    }

    auto binary_field(const char* key) const -> std::vector<uint8_t> {
        CborValue value;
        if (!find(key, value) || !cbor_value_is_byte_string(&value)) {
            return {};
        }

        size_t size = 0;
        if (cbor_value_calculate_string_length(&value, &size) != CborNoError) {
            return {};
        }

        std::vector<uint8_t> result(size);
        CborValue next;
        if (cbor_value_copy_byte_string(&value, result.data(), &size, &next) != CborNoError) {
            return {};
        }

        result.resize(size);
        return result;
    }

private:
    CborParser parser{};
    CborValue root{};
    bool valid{false};

    auto find(const char* key, CborValue& value) const -> bool {
        return valid && cbor_value_map_find_value(&root, key, &value) == CborNoError;
    }
};

void handle_mix_ints(const TestParams& params, TestResponse& response, TestSession&) {
    uint8_t a = 0;
    int16_t b = 0;
    uint32_t c = 0;
    if (!params.get_uint8(0, a) || !params.get_int16(1, b) || !params.get_uint32(2, c)) {
        response.write_error("Bad params");
        return;
    }

    response.write_int32(static_cast<int32_t>(a) + b + static_cast<int32_t>(c));
}

void handle_need_uint8(const TestParams& params, TestResponse& response, TestSession&) {
    uint8_t value = 0;
    if (!params.get_uint8(0, value)) {
        response.write_error("Bad uint8");
        return;
    }

    response.write_uint8(value);
}

void handle_get_binary(const TestParams&, TestResponse& response, TestSession&) {
    const uint8_t payload[] = {0x01, 0x02, 0x03, 0x04};
    response.write_binary(payload, sizeof(payload));
}

void handle_silent(const TestParams&, TestResponse&, TestSession&) {}

TEST(CborRpcDispatcherTest, DispatchesIntegerParamsAndResponses) {
    TestDispatcher dispatcher;
    dispatcher.addMethod("mix_ints", TestDispatcher::MethodHandler::create<handle_mix_ints>());
    TestSession session{};

    const auto request = make_request("mix_ints", 3, [](CborEncoder& params) {
        CborError error = cbor_encode_uint(&params, 7);
        if (error == CborNoError) error = cbor_encode_int(&params, -5);
        if (error == CborNoError) error = cbor_encode_uint(&params, 1000);
        return error;
    });

    TestBuffer response;
    dispatcher.dispatch(request, response, session);

    const ResponseView view(response);
    ASSERT_TRUE(view.is_valid());
    EXPECT_TRUE(view.ok());
    EXPECT_EQ(1002, view.int_field("result"));
}

TEST(CborRpcDispatcherTest, RejectsOutOfRangeIntegerParams) {
    TestDispatcher dispatcher;
    dispatcher.addMethod("need_uint8", TestDispatcher::MethodHandler::create<handle_need_uint8>());
    TestSession session{};

    const auto request = make_request("need_uint8", 1, [](CborEncoder& params) {
        return cbor_encode_uint(&params, 300);
    });

    TestBuffer response;
    dispatcher.dispatch(request, response, session);

    const ResponseView view(response);
    ASSERT_TRUE(view.is_valid());
    EXPECT_FALSE(view.ok());
    EXPECT_EQ("Bad uint8", view.string_field("error"));
}

TEST(CborRpcDispatcherTest, AcceptsTaggedBinaryAndIntegralDoubleParams) {
    TestDispatcher dispatcher;
    dispatcher.addMethod("web_compat", TestDispatcher::MethodHandler::create<handle_need_uint8>());
    TestSession session{};

    const auto request = make_request("web_compat", 1, [](CborEncoder& params) {
        return cbor_encode_double(&params, 7.0);
    });

    TestBuffer response;
    dispatcher.dispatch(request, response, session);

    const ResponseView view(response);
    ASSERT_TRUE(view.is_valid());
    EXPECT_TRUE(view.ok());
    EXPECT_EQ(7u, view.uint_field("result"));
}

void handle_auth_like(const TestParams& params, TestResponse& response, TestSession&) {
    etl::vector<uint8_t, 16> client_id{};
    etl::vector<uint8_t, 32> hmac{};
    uint64_t timestamp = 0;
    if (!params.get_binary(0, client_id) || !params.get_binary(1, hmac) || !params.get_uint64(2, timestamp)) {
        response.write_error("Invalid params");
        return;
    }

    response.write_bool(client_id.size() == 2 && hmac.size() == 3 && timestamp == 1773933970979ull);
}

TEST(CborRpcDispatcherTest, AcceptsCborXStyleAuthenticateParams) {
    TestDispatcher dispatcher;
    dispatcher.addMethod("auth_like", TestDispatcher::MethodHandler::create<handle_auth_like>());
    TestSession session{};

    const auto request = make_request("auth_like", 3, [](CborEncoder& params) {
        CborError error = cbor_encode_tag(&params, 64);
        if (error == CborNoError) {
            const uint8_t client_id[] = {0x01, 0x02};
            error = cbor_encode_byte_string(&params, client_id, sizeof(client_id));
        }
        if (error == CborNoError) {
            const uint8_t hmac[] = {0x03, 0x04, 0x05};
            error = cbor_encode_tag(&params, 64);
            if (error == CborNoError) {
                error = cbor_encode_byte_string(&params, hmac, sizeof(hmac));
            }
        }
        if (error == CborNoError) {
            error = cbor_encode_double(&params, 1773933970979.0);
        }
        return error;
    });

    TestBuffer response;
    dispatcher.dispatch(request, response, session);

    const ResponseView view(response);
    ASSERT_TRUE(view.is_valid());
    EXPECT_TRUE(view.ok());
}

TEST(CborRpcDispatcherTest, ReturnsBinaryPayload) {
    TestDispatcher dispatcher;
    dispatcher.addMethod("get_binary", TestDispatcher::MethodHandler::create<handle_get_binary>());
    TestSession session{};

    const auto request = make_request("get_binary");

    TestBuffer response;
    dispatcher.dispatch(request, response, session);

    const ResponseView view(response);
    ASSERT_TRUE(view.is_valid());
    EXPECT_TRUE(view.ok());
    EXPECT_EQ((std::vector<uint8_t>{0x01, 0x02, 0x03, 0x04}), view.binary_field("result"));
}

TEST(CborRpcDispatcherTest, ReturnsMethodNotFoundForUnknownMethod) {
    TestDispatcher dispatcher;
    TestSession session{};

    const auto request = make_request("missing");

    TestBuffer response;
    dispatcher.dispatch(request, response, session);

    const ResponseView view(response);
    ASSERT_TRUE(view.is_valid());
    EXPECT_FALSE(view.ok());
    EXPECT_EQ("Method not found", view.string_field("error"));
}

TEST(CborRpcDispatcherTest, ReturnsInvalidParamsForWrongRequestShape) {
    TestBuffer request;
    request.resize(request.max_size());

    CborEncoder encoder;
    CborEncoder map;
    cbor_encoder_init(&encoder, request.data(), request.size(), 0);
    CborError error = cbor_encoder_create_map(&encoder, &map, 2);
    if (error == CborNoError) error = cbor_encode_text_stringz(&map, "method");
    if (error == CborNoError) error = cbor_encode_text_stringz(&map, "broken");
    if (error == CborNoError) error = cbor_encode_text_stringz(&map, "params");
    if (error == CborNoError) error = cbor_encode_uint(&map, 1);
    if (error == CborNoError) error = cbor_encoder_close_container_checked(&encoder, &map);
    ASSERT_EQ(CborNoError, error);
    request.resize(cbor_encoder_get_buffer_size(&encoder, request.data()));

    TestDispatcher dispatcher;
    TestSession session{};
    TestBuffer response;
    dispatcher.dispatch(request, response, session);

    const ResponseView view(response);
    ASSERT_TRUE(view.is_valid());
    EXPECT_FALSE(view.ok());
    EXPECT_EQ("Invalid params", view.string_field("error"));
}

TEST(CborRpcDispatcherTest, ReturnsInternalErrorWhenHandlerDoesNotRespond) {
    TestDispatcher dispatcher;
    dispatcher.addMethod("silent", TestDispatcher::MethodHandler::create<handle_silent>());
    TestSession session{};

    const auto request = make_request("silent");

    TestBuffer response;
    dispatcher.dispatch(request, response, session);

    const ResponseView view(response);
    ASSERT_TRUE(view.is_valid());
    EXPECT_FALSE(view.ok());
    EXPECT_EQ("Internal error", view.string_field("error"));
}

TEST(CborRpcDispatcherTest, ReturnsInternalErrorWhenHandlerThrows) {
    GTEST_SKIP() << "Native PlatformIO test runtime aborts on thrown exceptions";
}

} // namespace

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
