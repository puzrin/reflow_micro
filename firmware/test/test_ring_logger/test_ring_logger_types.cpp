#include <gtest/gtest.h>
#include "lib/ring_logger/ring_logger_types.hpp"

using namespace ring_logger;

// I8
TEST(RingLoggerTypes, I8EncodeDecodeTest) {
    BinVector buffer;
    const int8_t test_val = 42;

    EncoderI8<int8_t>::write(test_val, buffer);

    std::string result;
    DecoderI8 decoder(buffer, 0);
    decoder.format(result);

    EXPECT_EQ(result, "42");
}

TEST(RingLoggerTypes, CharEncodeDecodeTest) {
    BinVector buffer;
    const char test_val = 42;

    EncoderI8<char>::write(test_val, buffer);

    std::string result;
    DecoderI8 decoder(buffer, 0);
    decoder.format(result);

    EXPECT_EQ(result, "42");
}

TEST(RingLoggerTypes, BoolEncodeDecodeTest) {
    BinVector buffer;
    const bool test_val = true;

    EncoderI8<bool>::write(test_val, buffer);

    std::string result;
    DecoderI8 decoder(buffer, 0);
    decoder.format(result);

    EXPECT_EQ(result, "1");
}

// U8
TEST(RingLoggerTypes, U8EncodeDecodeTest) {
    BinVector buffer;
    const uint8_t test_val = 200;

    EncoderU8<uint8_t>::write(test_val, buffer);

    std::string result;
    DecoderU8 decoder(buffer, 0);
    decoder.format(result);

    EXPECT_EQ(result, "200");
}

TEST(RingLoggerTypes, UCharEncodeDecodeTest) {
    BinVector buffer;
    const unsigned char test_val = 200;

    EncoderU8<unsigned char>::write(test_val, buffer);

    std::string result;
    DecoderU8 decoder(buffer, 0);
    decoder.format(result);

    EXPECT_EQ(result, "200");
}

// I16
TEST(RingLoggerTypes, I16EncodeDecodeTest) {
    BinVector buffer;
    const int16_t test_val = 12345;

    EncoderI16<int16_t>::write(test_val, buffer);

    std::string result;
    DecoderI16 decoder(buffer, 0);
    decoder.format(result);

    EXPECT_EQ(result, "12345");
}

TEST(RingLoggerTypes, ShortEncodeDecodeTest) {
    BinVector buffer;
    const short test_val = 12345;

    EncoderI16<short>::write(test_val, buffer);

    std::string result;
    DecoderI16 decoder(buffer, 0);
    decoder.format(result);

    EXPECT_EQ(result, "12345");
}

// U16
TEST(RingLoggerTypes, U16EncodeDecodeTest) {
    BinVector buffer;
    const uint16_t test_val = 65000;

    EncoderU16<uint16_t>::write(test_val, buffer);

    std::string result;
    DecoderU16 decoder(buffer, 0);
    decoder.format(result);

    EXPECT_EQ(result, "65000");
}

TEST(RingLoggerTypes, UShortEncodeDecodeTest) {
    BinVector buffer;
    const unsigned short test_val = 65000;

    EncoderU16<unsigned short>::write(test_val, buffer);

    std::string result;
    DecoderU16 decoder(buffer, 0);
    decoder.format(result);

    EXPECT_EQ(result, "65000");
}

// I32
TEST(RingLoggerTypes, I32EncodeDecodeTest) {
    BinVector buffer;
    const int32_t test_val = 123456789;

    EncoderI32<int32_t>::write(test_val, buffer);

    std::string result;
    DecoderI32 decoder(buffer, 0);
    decoder.format(result);

    EXPECT_EQ(result, "123456789");
}

TEST(RingLoggerTypes, IntEncodeDecodeTest) {
    BinVector buffer;
    const int test_val = 123456789;

    EncoderI32<int>::write(test_val, buffer);

    std::string result;
    DecoderI32 decoder(buffer, 0);
    decoder.format(result);

    EXPECT_EQ(result, "123456789");
}

// U32
TEST(RingLoggerTypes, U32EncodeDecodeTest) {
    BinVector buffer;
    const uint32_t test_val = 4000000000U;

    EncoderU32<uint32_t>::write(test_val, buffer);

    std::string result;
    DecoderU32 decoder(buffer, 0);
    decoder.format(result);

    EXPECT_EQ(result, "4000000000");
}

TEST(RingLoggerTypes, UIntEncodeDecodeTest) {
    BinVector buffer;
    const unsigned int test_val = 4000000000U;

    EncoderU32<unsigned int>::write(test_val, buffer);

    std::string result;
    DecoderU32 decoder(buffer, 0);
    decoder.format(result);

    EXPECT_EQ(result, "4000000000");
}

// I64
TEST(RingLoggerTypes, I64EncodeDecodeTest) {
    BinVector buffer;
    const int64_t test_val = 1234567890123456789LL;

    EncoderI64<int64_t>::write(test_val, buffer);

    std::string result;
    DecoderI64 decoder(buffer, 0);
    decoder.format(result);

    EXPECT_EQ(result, "1234567890123456789");
}

// U64
TEST(RingLoggerTypes, U64EncodeDecodeTest) {
    BinVector buffer;
    const uint64_t test_val = 18000000000000000000ULL;

    EncoderU64<uint64_t>::write(test_val, buffer);

    std::string result;
    DecoderU64 decoder(buffer, 0);
    decoder.format(result);

    EXPECT_EQ(result, "18000000000000000000");
}

// Float
TEST(RingLoggerTypes, FloatEncodeDecodeTest) {
    BinVector buffer;
    const float test_val = 123.456f;

    EncoderFlt<float>::write(test_val, buffer);

    std::string result;
    DecoderFlt decoder(buffer, 0);
    decoder.format(result);

    float decoded_val = std::stof(result);
    EXPECT_NEAR(decoded_val, test_val, 0.0001f);
}

// Double
TEST(RingLoggerTypes, DoubleEncodeDecodeTest) {
    BinVector buffer;
    const double test_val = 123.456789;

    EncoderDbl<double>::write(test_val, buffer);

    std::string result;
    DecoderDbl decoder(buffer, 0);
    decoder.format(result);

    double decoded_val = std::stod(result);
    EXPECT_NEAR(decoded_val, test_val, 0.000001);
}

// String
TEST(RingLoggerTypes, StdStringEncodeDecodeTest) {
    BinVector buffer;
    const std::string test_str = "Hello, World!";

    EncoderStdString<std::string>::write(test_str, buffer);

    std::string result;
    DecoderStr decoder(buffer, 0);
    decoder.format(result);

    EXPECT_EQ(result, test_str);
}

// C-strings
TEST(RingLoggerTypes, CStringEncodeDecodeTest) {
    BinVector buffer;
    const char* test_str = "Hello, World!";

    EncoderCString<const char*>::write(test_str, buffer);

    std::string result;
    DecoderStr decoder(buffer, 0);
    decoder.format(result);

    EXPECT_EQ(result, test_str);
}

TEST(RingLoggerTypes, CharPtrEncodeDecodeTest) {
    BinVector buffer;
    char test_str[] = "Hello, World!";

    EncoderCString<char*>::write(test_str, buffer);

    std::string result;
    DecoderStr decoder(buffer, 0);
    decoder.format(result);

    EXPECT_EQ(result, test_str);
}

/* This is not actual, because we force literal types decay in push.
TEST(RingLoggerTypes, StringLiteralTest) {
    BinVector buffer;

    EncoderCString<decltype("Hello, World!")>::write("Hello, World!", buffer);

    std::string result;
    DecoderStr decoder(buffer, 0);
    decoder.format(result);

    EXPECT_EQ(result, "Hello, World!");
}
*/