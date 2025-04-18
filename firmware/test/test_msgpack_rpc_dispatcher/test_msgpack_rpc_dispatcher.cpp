#include <gtest/gtest.h>
#include "lib/msgpack_rpc_dispatcher.hpp"

// Helpers
std::vector<uint8_t> s2msgp(const std::string& str) {
    JsonDocument doc;
    deserializeJson(doc, str);
    size_t size = measureMsgPack(doc);
    std::vector<uint8_t> output(size);
    serializeMsgPack(doc, output.data(), size);
    return output;
}

std::string msgp2s(const std::vector<uint8_t>& vec) {
    JsonDocument doc;
    deserializeMsgPack(doc, vec.data(), vec.size());
    std::string output;
    serializeJson(doc, output);
    return output;
}


// Example functions for testing
int8_t add_8bits(int8_t a, int8_t b) { return a + b; }
std::string concat(std::string a, std::string b) { return a + b; }

TEST(MsgpackRpcDispatcherTest, Test8BitsData) {
    MsgpackRpcDispatcher dispatcher;
    dispatcher.addMethod("add_8bits", add_8bits);

    auto input = s2msgp(R"({"method": "add_8bits", "args": [1, 2]})");
    std::string expected = R"({"ok":true,"result":3})";
    std::vector<uint8_t> result;
    dispatcher.dispatch(input, result);

    EXPECT_EQ(expected, msgp2s(result));
}

TEST(MsgpackRpcDispatcherTest, TestStringData) {
    MsgpackRpcDispatcher dispatcher;
    dispatcher.addMethod("concat", concat);

    auto input = s2msgp(R"({"method": "concat", "args": ["hello ", "world"]})");
    std::string expected = R"({"ok":true,"result":"hello world"})";
    std::vector<uint8_t> result;
    dispatcher.dispatch(input, result);

    EXPECT_EQ(expected, msgp2s(result));
}

TEST(MsgpackRpcDispatcherTest, TestUnknownMethod) {
    MsgpackRpcDispatcher dispatcher;

    auto input = s2msgp(R"({"method": "unknown", "args": []})");
    std::string expected = R"({"ok":false,"result":"Method not found"})";
    std::vector<uint8_t> result;
    dispatcher.dispatch(input, result);

    EXPECT_EQ(expected, msgp2s(result));
}

TEST(MsgpackRpcDispatcherTest, TestNoMethodProp) {
    MsgpackRpcDispatcher dispatcher;

    auto input = s2msgp(R"({"args": []})");
    std::string expected = R"({"ok":false,"result":"Method not found"})";
    std::vector<uint8_t> result;
    dispatcher.dispatch(input, result);

    EXPECT_EQ(expected, msgp2s(result));
}

TEST(MsgpackRpcDispatcherTest, TestMethodPropWrongType) {
    MsgpackRpcDispatcher dispatcher;

    auto input = s2msgp(R"({"method": [], "args": []})");
    std::string expected = R"({"ok":false,"result":"Method not found"})";
    std::vector<uint8_t> result;
    dispatcher.dispatch(input, result);

    EXPECT_EQ(expected, msgp2s(result));
}

TEST(MsgpackRpcDispatcherTest, TestNoArgsProp) {
    MsgpackRpcDispatcher dispatcher;
    dispatcher.addMethod("add_8bits", add_8bits);

    auto input = s2msgp(R"({"method": "add_8bits"})");
    std::string expected = R"({"ok":false,"result":"Number of arguments mismatch"})";
    std::vector<uint8_t> result;
    dispatcher.dispatch(input, result);

    EXPECT_EQ(expected, msgp2s(result));
}

TEST(MsgpackRpcDispatcherTest, TestArgsPropWrongType) {
    MsgpackRpcDispatcher dispatcher;
    dispatcher.addMethod("add_8bits", add_8bits);

    auto input = s2msgp(R"({"method": "add_8bits", "args": 5})");
    std::string expected = R"({"ok":false,"result":"Number of arguments mismatch"})";
    std::vector<uint8_t> result;
    dispatcher.dispatch(input, result);

    EXPECT_EQ(expected, msgp2s(result));
}

TEST(MsgpackRpcDispatcherTest, TestArgsOverflow) {
    MsgpackRpcDispatcher dispatcher;
    dispatcher.addMethod("add_8bits", add_8bits);

    // JSON input with arguments 512 and 512, which are beyond the range of int8_t (-128 to 127).
    auto input = s2msgp(R"({"method": "add_8bits", "args": [512, 512]})");
    // Current implementation consider that as wrong data type for simplicity.
    std::string expected = R"({"ok":false,"result":"Argument type mismatch"})";
    std::vector<uint8_t> result;
    dispatcher.dispatch(input, result);

    EXPECT_EQ(expected, msgp2s(result));
}

TEST(MsgpackRpcDispatcherTest, TestWrongArgTypeFloat) {
    MsgpackRpcDispatcher dispatcher;
    dispatcher.addMethod("add_8bits", add_8bits);

    auto input = s2msgp(R"({"method": "add_8bits", "args": [1, 2.5]})");
    // Current implementation consider that as wrong data type for simplicity.
    std::string expected = R"({"ok":false,"result":"Argument type mismatch"})";
    std::vector<uint8_t> result;
    dispatcher.dispatch(input, result);

    EXPECT_EQ(expected, msgp2s(result));
}

TEST(MsgpackRpcDispatcherTest, TestWrongArgTypeString) {
    MsgpackRpcDispatcher dispatcher;
    dispatcher.addMethod("add_8bits", add_8bits);

    auto input = s2msgp(R"({"method": "add_8bits", "args": [1, "string"]})");
    std::string expected = R"({"ok":false,"result":"Argument type mismatch"})";
    std::vector<uint8_t> result;
    dispatcher.dispatch(input, result);

    EXPECT_EQ(expected, msgp2s(result));
}

TEST(MsgpackRpcDispatcherTest, TestWrongArgTypeNull) {
    MsgpackRpcDispatcher dispatcher;
    dispatcher.addMethod("add_8bits", add_8bits);

    auto input = s2msgp(R"({"method": "add_8bits", "args": [1, null]})");
    std::string expected = R"({"ok":false,"result":"Argument type mismatch"})";
    std::vector<uint8_t> result;
    dispatcher.dispatch(input, result);

    EXPECT_EQ(expected, msgp2s(result));
}

TEST(MsgpackRpcDispatcherTest, TestWrongArgTypeInt) {
    MsgpackRpcDispatcher dispatcher;
    dispatcher.addMethod("concat", concat);

    auto input = s2msgp(R"({"method": "concat", "args": ["hello ", 1]})");
    std::string expected = R"({"ok":false,"result":"Argument type mismatch"})";
    std::vector<uint8_t> result;
    dispatcher.dispatch(input, result);

    EXPECT_EQ(expected, msgp2s(result));
}

TEST(MsgpackRpcDispatcherTest, TestStringWrongArgTypeNull) {
    MsgpackRpcDispatcher dispatcher;
    dispatcher.addMethod("concat", concat);

    auto input = s2msgp(R"({"method": "concat", "args": ["hello ", null]})");
    std::string expected = R"({"ok":false,"result":"Argument type mismatch"})";
    std::vector<uint8_t> result;
    dispatcher.dispatch(input, result);

    EXPECT_EQ(expected, msgp2s(result));
}

TEST(MsgpackRpcDispatcherTest, TestNoArgs) {
    MsgpackRpcDispatcher dispatcher;
    dispatcher.addMethod("noparams", [](){ return 5; });

    auto input = s2msgp(R"({"method": "noparams", "args": []})");
    std::string expected = R"({"ok":true,"result":5})";
    std::vector<uint8_t> result;
    dispatcher.dispatch(input, result);

    EXPECT_EQ(expected, msgp2s(result));
}

TEST(MsgpackRpcDispatcherTest, TestMethodThrows) {
    MsgpackRpcDispatcher dispatcher;
    dispatcher.addMethod("throw_exception", []() -> int { throw std::runtime_error("Test exception"); });

    auto input = s2msgp(R"({"method": "throw_exception", "args": []})");
    std::string expected = R"({"ok":false,"result":"Test exception"})";
    std::vector<uint8_t> result;
    dispatcher.dispatch(input, result);

    EXPECT_EQ(expected, msgp2s(result));
}

TEST(MsgpackRpcDispatcherTest, TestOneArgument) {
    MsgpackRpcDispatcher dispatcher;
    dispatcher.addMethod("one_argument", [](int a) { return a * 2; });

    auto input = s2msgp(R"({"method": "one_argument", "args": [2]})");
    std::string expected = R"({"ok":true,"result":4})";
    std::vector<uint8_t> result;
    dispatcher.dispatch(input, result);

    EXPECT_EQ(expected, msgp2s(result));
}

TEST(MsgpackRpcDispatcherTest, TestThreeArguments) {
    MsgpackRpcDispatcher dispatcher;
    dispatcher.addMethod("three_arguments", [](int a, int b, int c) { return a + b + c; });

    auto input = s2msgp(R"({"method": "three_arguments", "args": [1, 2, 3]})");
    std::string expected = R"({"ok":true,"result":6})";
    std::vector<uint8_t> result;
    dispatcher.dispatch(input, result);

    EXPECT_EQ(expected, msgp2s(result));
}

TEST(MsgpackRpcDispatcherTest, TestBrokenMsgPackInput) {
    MsgpackRpcDispatcher dispatcher;
    dispatcher.addMethod("add_8bits", add_8bits);

    // Invalid MsgPack data (array of size 2 with only 1 element)
    std::vector<uint8_t> input({0x92, 0x01});

    std::string expected = R"({"ok":false,"result":"IncompleteInput"})";
    std::vector<uint8_t> result;
    dispatcher.dispatch(input, result);

    EXPECT_EQ(expected, msgp2s(result));
}

TEST(MsgpackRpcDispatcherTest, TestBinaryReturn) {
    MsgpackRpcDispatcher dispatcher;

    std::vector<uint8_t> test_data{0x01, 0x02, 0x03, 0x04};
    dispatcher.addMethod("get_binary", [test_data]() {
        return test_data;
    });

    auto input = s2msgp(R"({"method": "get_binary", "args": []})");
    std::vector<uint8_t> result;
    dispatcher.dispatch(input, result);

    JsonDocument doc;
    deserializeMsgPack(doc, result.data(), result.size());

    EXPECT_TRUE(doc["ok"].as<bool>());

    auto binary = doc["result"].as<MsgPackBinary>();
    std::vector<uint8_t> actual_data(
        static_cast<const uint8_t*>(binary.data()),
        static_cast<const uint8_t*>(binary.data()) + binary.size()
    );

    EXPECT_EQ(actual_data, test_data);
}

TEST(MsgpackRpcDispatcherTest, TestBinaryArgument) {
    MsgpackRpcDispatcher dispatcher;

    dispatcher.addMethod("sum_binary", [](std::vector<uint8_t> data) {
        int sum = 0;
        for(uint8_t byte : data) sum += byte;
        return sum;
    });

    std::vector<uint8_t> test_data{0x01, 0x02, 0x03, 0x04};

    JsonDocument input_doc;
    input_doc["method"] = "sum_binary";
    JsonArray args = input_doc["args"].to<JsonArray>();
    args.add(MsgPackBinary(test_data.data(), test_data.size()));

    std::vector<uint8_t> input;
    size_t size = measureMsgPack(input_doc);
    input.resize(size);
    serializeMsgPack(input_doc, input.data(), size);

    // Call the method
    std::vector<uint8_t> result;
    dispatcher.dispatch(input, result);

    // Check the result
    JsonDocument doc;
    deserializeMsgPack(doc, result.data(), result.size());

    EXPECT_TRUE(doc["ok"].as<bool>());
    EXPECT_EQ(doc["result"].as<int>(), 10); // 1 + 2 + 3 + 4 = 10
}

// Main function to run the tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
