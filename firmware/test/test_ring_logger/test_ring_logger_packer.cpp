#include <gtest/gtest.h>
#include "lib/ring_logger/ring_logger_packer.hpp"

using namespace ring_logger;

constexpr size_t BUFFER_SIZE = 1024;
constexpr size_t ARGUMENTS_COUNT = 10;
using TestPacker = Packer<BUFFER_SIZE, ARGUMENTS_COUNT>;

TEST(PackerTest, PackUnpackIntegers) {
    int8_t int8_val = 42;
    int16_t int16_val = 300;
    int32_t int32_val = 100000;
    uint8_t uint8_val = 255;
    uint16_t uint16_val = 60000;
    uint32_t uint32_val = 4000000000;

    auto packedData = TestPacker::pack(int8_val, int16_val, int32_val, uint8_val, uint16_val, uint32_val);

    TestPacker::UnpackedData unpackedData;
    bool result = TestPacker::unpack(packedData, unpackedData);

    ASSERT_TRUE(result);
    ASSERT_EQ(unpackedData.size, 6u);

    EXPECT_EQ(unpackedData.data[0].type, ArgTypeTag::INT8);
    EXPECT_EQ(unpackedData.data[0].int8Value, int8_val);

    EXPECT_EQ(unpackedData.data[1].type, ArgTypeTag::INT16);
    EXPECT_EQ(unpackedData.data[1].int16Value, int16_val);

    EXPECT_EQ(unpackedData.data[2].type, ArgTypeTag::INT32);
    EXPECT_EQ(unpackedData.data[2].int32Value, int32_val);

    EXPECT_EQ(unpackedData.data[3].type, ArgTypeTag::UINT8);
    EXPECT_EQ(unpackedData.data[3].uint8Value, uint8_val);

    EXPECT_EQ(unpackedData.data[4].type, ArgTypeTag::UINT16);
    EXPECT_EQ(unpackedData.data[4].uint16Value, uint16_val);

    EXPECT_EQ(unpackedData.data[5].type, ArgTypeTag::UINT32);
    EXPECT_EQ(unpackedData.data[5].uint32Value, uint32_val);
}

TEST(PackerTest, PackUnpackString) {
    const char* foo_str = "foo";
    TestPacker::PackedData packedData = {};
    std::memset(packedData.data, '=', BUFFER_SIZE);

    // Using both pointer and literal for strings
    packedData = TestPacker::pack("", foo_str);

    TestPacker::UnpackedData unpackedData;
    bool result = TestPacker::unpack(packedData, unpackedData);

    ASSERT_TRUE(result);
    ASSERT_EQ(unpackedData.size, 2u);

    EXPECT_EQ(unpackedData.data[0].type, ArgTypeTag::STRING);
    EXPECT_EQ(std::strlen(unpackedData.data[0].stringValue), std::strlen(""));
    EXPECT_STREQ(unpackedData.data[0].stringValue, "");

    EXPECT_EQ(unpackedData.data[1].type, ArgTypeTag::STRING);
    EXPECT_EQ(std::strlen(unpackedData.data[1].stringValue), std::strlen(foo_str));
    EXPECT_STREQ(unpackedData.data[1].stringValue, foo_str);

    // Using both pointer and literal for strings
    packedData = TestPacker::pack(foo_str, "barrr");

    result = TestPacker::unpack(packedData, unpackedData);

    ASSERT_TRUE(result);
    ASSERT_EQ(unpackedData.size, 2u);

    EXPECT_EQ(unpackedData.data[0].type, ArgTypeTag::STRING);
    EXPECT_EQ(std::strlen(unpackedData.data[0].stringValue), std::strlen(foo_str));
    EXPECT_STREQ(unpackedData.data[0].stringValue, foo_str);

    EXPECT_EQ(unpackedData.data[1].type, ArgTypeTag::STRING);
    EXPECT_EQ(std::strlen(unpackedData.data[1].stringValue), std::strlen("barrr"));
    EXPECT_STREQ(unpackedData.data[1].stringValue, "barrr");
}

TEST(PackerTest, GetPackedSize) {
    int8_t int8_val = 42;
    size_t packedSize = TestPacker::getPackedSize(int8_val, "Hello, World!");

    ASSERT_EQ(packedSize, 1 + (1 + sizeof(int8_val)) + (1 + sizeof(uint16_t) + std::strlen("Hello, World!") + 1));
}

TEST(PackerTest, UnpackUnknownType) {
    TestPacker::PackedData packedData = {};
    packedData.data[0] = 1;  // One argument
    packedData.data[1] = 255; // Invalid type tag
    packedData.size = 2;

    TestPacker::UnpackedData unpackedData;
    bool result = TestPacker::unpack(packedData, unpackedData);

    ASSERT_FALSE(result);
}

TEST(PackerTest, PackMaxArguments) {
    auto packedData = TestPacker::pack(1, 2, 3, 4, 5, 6, 7, 8, 9, 10); // 10 arguments

    TestPacker::UnpackedData unpackedData;
    bool result = TestPacker::unpack(packedData, unpackedData);

    ASSERT_TRUE(result);
    ASSERT_EQ(unpackedData.size, 10u);
}

TEST(PackerTest, UnpackTooManyArguments) {
    TestPacker::PackedData packedData = {};
    packedData.data[0] = ARGUMENTS_COUNT + 1;  // Exceeds max arguments
    packedData.data[1] = static_cast<uint8_t>(ArgTypeTag::INT8);
    packedData.data[2] = 42;
    packedData.size = 3;

    TestPacker::UnpackedData unpackedData;
    bool result = TestPacker::unpack(packedData, unpackedData);

    ASSERT_FALSE(result);
}
