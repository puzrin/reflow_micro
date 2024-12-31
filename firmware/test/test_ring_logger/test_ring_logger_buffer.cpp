#include <gtest/gtest.h>
#include <algorithm>
#include "lib/ring_logger/ring_logger_buffer.hpp"

using namespace ring_logger;

TEST(RingLoggerBufferTest, WriteAndReadSingleRecord) {
    RingBuffer<1024> buffer;
    const uint8_t data[13] = {0};
    size_t dataSize = sizeof(data);

    // Write record
    ASSERT_TRUE(buffer.writeRecord(data, dataSize));

    // Read record
    uint8_t readData[1024];
    size_t readSize = 0;
    ASSERT_TRUE(buffer.readRecord(readData, readSize));

    // Validate record
    ASSERT_EQ(readSize, dataSize);
    ASSERT_TRUE(std::equal(data, data + dataSize, readData));
}

TEST(RingLoggerBufferTest, OverflowOnRecordHeader) {
    constexpr size_t bufferSize = 32;
    ring_logger::RingBuffer<bufferSize> buffer;
    const uint8_t data1[6] = {0};
    const uint8_t data2[21] = {1};
    const uint8_t data3[6] = {2};
    size_t dataSize1 = sizeof(data1);
    size_t dataSize2 = sizeof(data2);
    size_t dataSize3 = sizeof(data3);

    // Write records
    ASSERT_TRUE(buffer.writeRecord(data1, dataSize1));
    ASSERT_TRUE(buffer.writeRecord(data2, dataSize2));
    ASSERT_TRUE(buffer.writeRecord(data3, dataSize3));

    // Read records
    uint8_t readData[bufferSize];
    size_t readSize = 0;
    ASSERT_TRUE(buffer.readRecord(readData, readSize));
    ASSERT_EQ(readSize, dataSize2);
    ASSERT_TRUE(std::equal(data2, data2 + dataSize2, readData));

    ASSERT_TRUE(buffer.readRecord(readData, readSize));
    ASSERT_EQ(readSize, dataSize3);
    ASSERT_TRUE(std::equal(data3, data3 + dataSize3, readData));
}

TEST(RingLoggerBufferTest, OverflowOnRecordData) {
    constexpr size_t bufferSize = 32;
    ring_logger::RingBuffer<bufferSize> buffer;
    const uint8_t data1[6] = {0};
    const uint8_t data2[19] = {1};
    const uint8_t data3[6] = {2};
    size_t dataSize1 = sizeof(data1);
    size_t dataSize2 = sizeof(data2);
    size_t dataSize3 = sizeof(data3);

    // Write records
    ASSERT_TRUE(buffer.writeRecord(data1, dataSize1));
    ASSERT_TRUE(buffer.writeRecord(data2, dataSize2));
    ASSERT_TRUE(buffer.writeRecord(data3, dataSize3));

    // Read records
    uint8_t readData[bufferSize];
    size_t readSize = 0;
    ASSERT_TRUE(buffer.readRecord(readData, readSize));
    ASSERT_EQ(readSize, dataSize2);
    ASSERT_TRUE(std::equal(data2, data2 + dataSize2, readData));

    ASSERT_TRUE(buffer.readRecord(readData, readSize));
    ASSERT_EQ(readSize, dataSize3);
    ASSERT_TRUE(std::equal(data3, data3 + dataSize3, readData));
}

TEST(RingLoggerBufferTest, ReadFromEmptyBuffer) {
    ring_logger::RingBuffer<1024> buffer;
    uint8_t readData[1024];
    size_t readSize = 0;

    ASSERT_FALSE(buffer.readRecord(readData, readSize));
    ASSERT_EQ(readSize, static_cast<size_t>(0));
}
