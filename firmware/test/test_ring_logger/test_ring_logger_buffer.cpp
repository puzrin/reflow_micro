#include <gtest/gtest.h>
#include "lib/ring_logger/ring_logger_buffer.hpp"

using namespace ring_logger;

TEST(RingBufferTest, WriteAndReadSingleRecord) {
    RingBuffer<1024> buffer;
    std::vector<uint8_t> data(13, 0);
    std::vector<uint8_t> readData;

    ASSERT_TRUE(buffer.writeRecord(data));
    ASSERT_TRUE(buffer.readRecord(readData));
    ASSERT_EQ(readData, data);
}

TEST(RingBufferTest, OverflowOnRecordHeader) {
    constexpr size_t bufferSize = 32;
    ring_logger::RingBuffer<bufferSize> buffer;

    std::vector<uint8_t> data1(6, 0);
    std::vector<uint8_t> data2(21, 1);
    std::vector<uint8_t> data3(6, 2);
    std::vector<uint8_t> readData;

    ASSERT_TRUE(buffer.writeRecord(data1));
    ASSERT_TRUE(buffer.writeRecord(data2));
    ASSERT_TRUE(buffer.writeRecord(data3));

    ASSERT_TRUE(buffer.readRecord(readData));
    ASSERT_EQ(readData, data2);

    ASSERT_TRUE(buffer.readRecord(readData));
    ASSERT_EQ(readData, data3);
}

TEST(RingBufferTest, OverflowOnRecordData) {
    constexpr size_t bufferSize = 32;
    ring_logger::RingBuffer<bufferSize> buffer;

    std::vector<uint8_t> data1(6, 0);
    std::vector<uint8_t> data2(19, 1);
    std::vector<uint8_t> data3(6, 2);
    std::vector<uint8_t> readData;

    ASSERT_TRUE(buffer.writeRecord(data1));
    ASSERT_TRUE(buffer.writeRecord(data2));
    ASSERT_TRUE(buffer.writeRecord(data3));

    ASSERT_TRUE(buffer.readRecord(readData));
    ASSERT_EQ(readData, data2);

    ASSERT_TRUE(buffer.readRecord(readData));
    ASSERT_EQ(readData, data3);
}

TEST(RingBufferTest, ReadFromEmptyBuffer) {
    ring_logger::RingBuffer<1024> buffer;
    std::vector<uint8_t> readData;
    ASSERT_FALSE(buffer.readRecord(readData));
    ASSERT_TRUE(readData.empty());
}
