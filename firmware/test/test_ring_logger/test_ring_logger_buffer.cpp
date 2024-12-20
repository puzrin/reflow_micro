#include <gtest/gtest.h>
#include "lib/ring_logger/ring_logger_buffer.hpp"

TEST(RingLoggerBufferTest, WriteAndReadSingleRecord) {
    ring_logger::RingBuffer<1024> buffer;
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
    ASSERT_EQ(std::memcmp(data, readData, dataSize), 0);
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
    ASSERT_EQ(std::memcmp(data2, readData, dataSize2), 0);

    ASSERT_TRUE(buffer.readRecord(readData, readSize));
    ASSERT_EQ(readSize, dataSize3);
    ASSERT_EQ(std::memcmp(data3, readData, dataSize3), 0);
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
    ASSERT_EQ(std::memcmp(data2, readData, dataSize2), 0);

    ASSERT_TRUE(buffer.readRecord(readData, readSize));
    ASSERT_EQ(readSize, dataSize3);
    ASSERT_EQ(std::memcmp(data3, readData, dataSize3), 0);
}

TEST(RingLoggerBufferTest, ReadFromEmptyBuffer) {
    ring_logger::RingBuffer<1024> buffer;
    uint8_t readData[1024];
    size_t readSize = 0;

    ASSERT_FALSE(buffer.readRecord(readData, readSize));
    ASSERT_EQ(readSize, static_cast<size_t>(0));
}
