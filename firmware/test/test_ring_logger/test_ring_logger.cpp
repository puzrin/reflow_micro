#include <gtest/gtest.h>
#include "lib/ring_logger/ring_logger.hpp"

using namespace ring_logger;

constexpr auto info = RingLoggerLevelInfo;

// Define test labels
constexpr const char foo_label[] = "foo";
constexpr const char bar_label[] = "bar";
constexpr const char garbage_label[] = "garbage";
constexpr const char whitelisted_labels_list[] = "foo,bar";
constexpr const char ignored_labels_list[] = "garbage";

TEST(RingLoggerTest, BasicPushAndLpush) {
    RingBuffer<10000> ringBuffer;
    RingLogger<> logger(ringBuffer);
    char buffer[1024] = {0};

    logger.push(RingLoggerLevelInfo, "Hello, {}!", "World");
    ASSERT_TRUE(logger.pull(buffer, sizeof(buffer)));
    EXPECT_STREQ(buffer, "[INFO]: Hello, World!");

    logger.push(RingLoggerLevelDebug, "Debug message: {}", 123);
    ASSERT_TRUE(logger.pull(buffer, sizeof(buffer)));
    EXPECT_STREQ(buffer, "[DEBUG]: Debug message: 123");

    logger.push(RingLoggerLevelError, "Error message: {}", 456);
    ASSERT_TRUE(logger.pull(buffer, sizeof(buffer)));
    EXPECT_STREQ(buffer, "[ERROR]: Error message: 456");
}

TEST(RingLoggerTest, TooBigMessage) {
    RingBuffer<10000> ringBuffer;
    RingLogger<> logger(ringBuffer);
    char buffer[1024] = {0};
    std::string bigMessage(600, 'A'); // Create a big message
    logger.push(info, "{}", bigMessage.c_str());
    ASSERT_TRUE(logger.pull(buffer, sizeof(buffer)));
    EXPECT_STREQ(buffer, "[INFO]: [TOO BIG]");
}

TEST(RingLoggerTest, SupportedArgTypes) {
    RingBuffer<10000> ringBuffer;
    RingLogger<> logger(ringBuffer);
    char buffer[1024] = {0};

    int8_t int8_val = -8;
    uint8_t uint8_val = 8;
    int16_t int16_val = -16;
    uint16_t uint16_val = 16;
    int32_t int32_val = -32;
    uint32_t uint32_val = 32;
    const char* str_val = "test";
    char* mutable_str_val = const_cast<char*>("mutable");

    logger.push(info, "Test values: {}, {}, {}, {}, {}, {}, {}, {}", int8_val, uint8_val, int16_val, uint16_val, int32_val, uint32_val, str_val, mutable_str_val);
    ASSERT_TRUE(logger.pull(buffer, sizeof(buffer)));
    EXPECT_STREQ(buffer, "[INFO]: Test values: -8, 8, -16, 16, -32, 32, test, mutable");
}

TEST(RingLoggerTest, MaxArgs) {
    RingBuffer<10000> ringBuffer;
    RingLogger<> logger(ringBuffer);
    char buffer[1024] = {0};

    logger.push(info, "Test max args: {}, {}, {}, {}, {}, {}, {}, {}, {}, {}", 1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
    ASSERT_TRUE(logger.pull(buffer, sizeof(buffer)));
    EXPECT_STREQ(buffer, "[INFO]: Test max args: 1, 2, 3, 4, 5, 6, 7, 8, 9, 10");
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
