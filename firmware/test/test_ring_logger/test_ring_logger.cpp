#include <gtest/gtest.h>
#include "lib/ring_logger/ring_logger.hpp"

using namespace ring_logger;

constexpr auto info = RingLoggerLevelInfo;

TEST(RingLoggerTest, BasicPush) {
    RingBuffer<10000> ringBuffer;
    RingLogger<> logger(ringBuffer);
    std::string output;

    logger.push(RingLoggerLevelInfo, "Hello, {}!", "World");
    ASSERT_TRUE(logger.pull(output));
    EXPECT_EQ(output, "[INFO]: Hello, World!");

    output.clear();
    logger.push(RingLoggerLevelDebug, "Debug message: {}", 123);
    ASSERT_TRUE(logger.pull(output));
    EXPECT_EQ(output, "[DEBUG]: Debug message: 123");

    output.clear();
    logger.push(RingLoggerLevelError, "Error message: {}", 456);
    ASSERT_TRUE(logger.pull(output));
    EXPECT_EQ(output, "[ERROR]: Error message: 456");
}


TEST(RingLoggerTest, SupportedArgTypes) {
    RingBuffer<10000> ringBuffer;
    RingLogger<> logger(ringBuffer);
    std::string output;

    int8_t int8_val = -8;
    uint8_t uint8_val = 8;
    int16_t int16_val = -16;
    uint16_t uint16_val = 16;
    int32_t int32_val = -32;
    uint32_t uint32_val = 32;
    const char* str_val = "test";
    char* mutable_str_val = const_cast<char*>("mutable");

    logger.push(info, "Test values: {}, {}, {}, {}, {}, {}, {}, {}",
        int8_val, uint8_val, int16_val, uint16_val,
        int32_val, uint32_val, str_val, mutable_str_val);
    ASSERT_TRUE(logger.pull(output));
    EXPECT_EQ(output, "[INFO]: Test values: -8, 8, -16, 16, -32, 32, test, mutable");
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
