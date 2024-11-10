#include <gtest/gtest.h>
#include "rpc/json_rpc_dispatcher.hpp"

// Helpers
std::vector<uint8_t> s2v(const std::string& str) {
    return std::vector<uint8_t>(str.begin(), str.end());
}

std::string v2s(const std::vector<uint8_t>& vec) {
    return std::string(vec.begin(), vec.end());
}


// Example functions for testing
int8_t add_8bits(int8_t a, int8_t b) { return a + b; }
std::string concat(std::string a, std::string b) { return a + b; }

TEST(JsonRpcDispatcherTest, Test8BitsData) {
    JsonRpcDispatcher dispatcher;
    dispatcher.addMethod("add_8bits", add_8bits);

    std::string input = R"({"method": "add_8bits", "args": [1, 2]})";
    std::string expected = R"({"ok":true,"result":3})";
    std::string result = dispatcher.dispatch(input);

    EXPECT_EQ(expected, result);
}

TEST(JsonRpcDispatcherTest, TestStringData) {
    JsonRpcDispatcher dispatcher;
    dispatcher.addMethod("concat", concat);

    std::string input = R"({"method": "concat", "args": ["hello ", "world"]})";
    std::string expected = R"({"ok":true,"result":"hello world"})";
    std::string result = dispatcher.dispatch(input);

    EXPECT_EQ(expected, result);
}

TEST(JsonRpcDispatcherTest, TestVectorsInOut) {
    JsonRpcDispatcher dispatcher;
    dispatcher.addMethod("concat", concat);

    std::string input = R"({"method": "concat", "args": ["hello ", "world"]})";
    std::string expected = R"({"ok":true,"result":"hello world"})";
    std::vector<uint8_t> result;
    dispatcher.dispatch(s2v(input), result);

    EXPECT_EQ(expected, v2s(result));
}

TEST(JsonRpcDispatcherTest, TestUnknownMethod) {
    JsonRpcDispatcher dispatcher;

    std::string input = R"({"method": "unknown", "args": []})";
    std::string expected = R"({"ok":false,"result":"Method not found"})";
    std::string result = dispatcher.dispatch(input);

    EXPECT_EQ(expected, result);
}

TEST(JsonRpcDispatcherTest, TestNoMethodProp) {
    JsonRpcDispatcher dispatcher;

    std::string input = R"({"args": []})";
    std::string expected = R"({"ok":false,"result":"Method not found"})";
    std::string result = dispatcher.dispatch(input);

    EXPECT_EQ(expected, result);
}

TEST(JsonRpcDispatcherTest, TestMethodPropWrongType) {
    JsonRpcDispatcher dispatcher;

    std::string input = R"({"method": [], "args": []})";
    std::string expected = R"({"ok":false,"result":"Method not found"})";
    std::string result = dispatcher.dispatch(input);

    EXPECT_EQ(expected, result);
}

TEST(JsonRpcDispatcherTest, TestNoArgsProp) {
    JsonRpcDispatcher dispatcher;
    dispatcher.addMethod("add_8bits", add_8bits);

    std::string input = R"({"method": "add_8bits"})";
    std::string expected = R"({"ok":false,"result":"Number of arguments mismatch"})";
    std::string result = dispatcher.dispatch(input);

    EXPECT_EQ(expected, result);
}

TEST(JsonRpcDispatcherTest, TestArgsPropWrongType) {
    JsonRpcDispatcher dispatcher;
    dispatcher.addMethod("add_8bits", add_8bits);

    std::string input = R"({"method": "add_8bits", "args": 5})";
    std::string expected = R"({"ok":false,"result":"Number of arguments mismatch"})";
    std::string result = dispatcher.dispatch(input);

    EXPECT_EQ(expected, result);
}

TEST(JsonRpcDispatcherTest, TestArgsOverflow) {
    JsonRpcDispatcher dispatcher;
    dispatcher.addMethod("add_8bits", add_8bits);

    // JSON input with arguments 512 and 512, which are beyond the range of int8_t (-128 to 127).
    std::string input = R"({"method": "add_8bits", "args": [512, 512]})";
    // Current implementation consider that as wrong data type for simplicity.
    std::string expected = R"({"ok":false,"result":"Argument type mismatch"})";
    std::string result = dispatcher.dispatch(input);

    EXPECT_EQ(expected, result);
}

TEST(JsonRpcDispatcherTest, TestWrongArgTypeFloat) {
    JsonRpcDispatcher dispatcher;
    dispatcher.addMethod("add_8bits", add_8bits);

    std::string input = R"({"method": "add_8bits", "args": [1, 2.5]})";
    // Current implementation consider that as wrong data type for simplicity.
    std::string expected = R"({"ok":false,"result":"Argument type mismatch"})";
    std::string result = dispatcher.dispatch(input);

    EXPECT_EQ(expected, result);
}

TEST(JsonRpcDispatcherTest, TestWrongArgTypeString) {
    JsonRpcDispatcher dispatcher;
    dispatcher.addMethod("add_8bits", add_8bits);

    std::string input = R"({"method": "add_8bits", "args": [1, "string"]})";
    std::string expected = R"({"ok":false,"result":"Argument type mismatch"})";
    std::string result = dispatcher.dispatch(input);

    EXPECT_EQ(expected, result);
}

TEST(JsonRpcDispatcherTest, TestWrongArgTypeNull) {
    JsonRpcDispatcher dispatcher;
    dispatcher.addMethod("add_8bits", add_8bits);

    std::string input = R"({"method": "add_8bits", "args": [1, null]})";
    std::string expected = R"({"ok":false,"result":"Argument type mismatch"})";
    std::string result = dispatcher.dispatch(input);

    EXPECT_EQ(expected, result);
}

TEST(JsonRpcDispatcherTest, TestWrongArgTypeInt) {
    JsonRpcDispatcher dispatcher;
    dispatcher.addMethod("concat", concat);

    std::string input = R"({"method": "concat", "args": ["hello ", 1]})";
    std::string expected = R"({"ok":false,"result":"Argument type mismatch"})";
    std::string result = dispatcher.dispatch(input);

    EXPECT_EQ(expected, result);
}

TEST(JsonRpcDispatcherTest, TestStringWrongArgTypeNull) {
    JsonRpcDispatcher dispatcher;
    dispatcher.addMethod("concat", concat);

    std::string input = R"({"method": "concat", "args": ["hello ", null]})";
    std::string expected = R"({"ok":false,"result":"Argument type mismatch"})";
    std::string result = dispatcher.dispatch(input);

    EXPECT_EQ(expected, result);
}

TEST(JsonRpcDispatcherTest, TestNoArgs) {
    JsonRpcDispatcher dispatcher;
    dispatcher.addMethod("noparams", [](){ return 5; });

    std::string input = R"({"method": "noparams", "args": []})";
    std::string expected = R"({"ok":true,"result":5})";
    std::string result = dispatcher.dispatch(input);

    EXPECT_EQ(expected, result);
}

TEST(JsonRpcDispatcherTest, TestMethodThrows) {
    JsonRpcDispatcher dispatcher;
    dispatcher.addMethod("throw_exception", []() -> int { throw std::runtime_error("Test exception"); });

    std::string input = R"({"method": "throw_exception", "args": []})";
    std::string expected = R"({"ok":false,"result":"Test exception"})";
    std::string result = dispatcher.dispatch(input);

    EXPECT_EQ(expected, result);
}

TEST(JsonRpcDispatcherTest, TestOneArgument) {
    JsonRpcDispatcher dispatcher;
    dispatcher.addMethod("one_argument", [](int a) { return a * 2; });

    std::string input = R"({"method": "one_argument", "args": [2]})";
    std::string expected = R"({"ok":true,"result":4})";
    std::string result = dispatcher.dispatch(input);

    EXPECT_EQ(expected, result);
}

TEST(JsonRpcDispatcherTest, TestThreeArguments) {
    JsonRpcDispatcher dispatcher;
    dispatcher.addMethod("three_arguments", [](int a, int b, int c) { return a + b + c; });

    std::string input = R"({"method": "three_arguments", "args": [1, 2, 3]})";
    std::string expected = R"({"ok":true,"result":6})";
    std::string result = dispatcher.dispatch(input);

    EXPECT_EQ(expected, result);
}

TEST(JsonRpcDispatcherTest, TestBrokenJsonInput) {
    JsonRpcDispatcher dispatcher;
    dispatcher.addMethod("add_8bits", add_8bits);

    std::string input = R"({"method": "add_8bits", "args": [1, 2)"; // Missing closing brace
    std::string expected = R"({"ok":false,"result":"IncompleteInput"})";
    std::string result = dispatcher.dispatch(input);

    EXPECT_EQ(expected, result);
}

// Main function to run the tests
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
