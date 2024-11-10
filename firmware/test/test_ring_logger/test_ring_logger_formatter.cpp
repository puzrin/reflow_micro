#include <gtest/gtest.h>
#include "ring_logger/ring_logger_formatter.hpp"

using namespace ring_logger;

// Test empty message
TEST(FormatterTest, EmptyMessage) {
    char output[256];
    Formatter::print(output, sizeof(output), "", nullptr, 0);
    EXPECT_STREQ(output, "");
}

// Test simple message without placeholders
TEST(FormatterTest, SimpleMessage) {
    char output[256];
    Formatter::print(output, sizeof(output), "Hello, World!", nullptr, 0);
    EXPECT_STREQ(output, "Hello, World!");
}

// Test message with one integer placeholder
TEST(FormatterTest, OneIntegerPlaceholder) {
    char output[256];
    ArgVariant args[] = { ArgVariant(42) };
    Formatter::print(output, sizeof(output), "Value: {}", args, 1);
    EXPECT_STREQ(output, "Value: 42");
}

// Test message with multiple placeholders
TEST(FormatterTest, MultiplePlaceholders) {
    char output[256];
    ArgVariant args[] = { ArgVariant(42), ArgVariant("Test"), ArgVariant(0x2A) };
    Formatter::print(output, sizeof(output), "Int: {}, Str: {}, Hex: {}", args, 3);
    EXPECT_STREQ(output, "Int: 42, Str: Test, Hex: 42");
}

// Test message with not enough arguments
TEST(FormatterTest, NotEnoughArguments) {
    char output[256];
    ArgVariant args[] = { ArgVariant(42) };
    Formatter::print(output, sizeof(output), "Value: {}, Another: {}", args, 1);
    EXPECT_STREQ(output, "Value: 42, Another: {}");
}

// Test message with overflow
TEST(FormatterTest, Overflow) {
    char output[256];
    ArgVariant args[] = { ArgVariant(42) };
    Formatter::print(output, 10, "This is a very long message that will not fit", args, 1);
    EXPECT_EQ(strlen(output), 9u);
}

// Test message with string argument
TEST(FormatterTest, StringArgument) {
    char output[256];
    ArgVariant args[] = { ArgVariant("Test") };
    Formatter::print(output, sizeof(output), "String: {}", args, 1);
    EXPECT_STREQ(output, "String: Test");
}

// Test multiple types
TEST(FormatterTest, MultipleTypes) {
    char output[256];
    ArgVariant args[] = {
        ArgVariant((int8_t)8),
        ArgVariant((int16_t)16),
        ArgVariant((int32_t)32),
        ArgVariant((uint8_t)8),
        ArgVariant((uint16_t)16),
        ArgVariant((uint32_t)32)
    };
    Formatter::print(
        output, sizeof(output), 
        "Int8: {}, Int16: {}, Int32: {}, Uint8: {}, Uint16: {}, Uint32: {}",
        args, 6
    );
    EXPECT_STREQ(output, "Int8: 8, Int16: 16, Int32: 32, Uint8: 8, Uint16: 16, Uint32: 32");
}

// Test array + number of arguments
TEST(FormatterTest, ArrayArguments) {
    char output[256];
    ArgVariant args[] = { ArgVariant(42), ArgVariant("Test"), ArgVariant(0x2A) };
    Formatter::print(output, sizeof(output), "Int: {}, Str: {}, Hex: {}", args, 3);
    EXPECT_STREQ(output, "Int: 42, Str: Test, Hex: 42");
}

// Test alias with variadic arguments
TEST(FormatterTest, AliasEmptyMessage) {
    char output[256];
    Formatter::print(output, sizeof(output), "");
    EXPECT_STREQ(output, "");
}

// Test alias with simple message
TEST(FormatterTest, AliasSimpleMessage) {
    char output[256];
    Formatter::print(output, sizeof(output), "Hello, World!");
    EXPECT_STREQ(output, "Hello, World!");
}

// Test alias with one integer placeholder
TEST(FormatterTest, AliasOneIntegerPlaceholder) {
    char output[256];
    Formatter::print(output, sizeof(output), "Value: {}", ArgVariant(42));
    EXPECT_STREQ(output, "Value: 42");
}

// Test alias with multiple placeholders
TEST(FormatterTest, AliasMultiplePlaceholders) {
    char output[256];
    Formatter::print(output, sizeof(output), "Int: {}, Str: {}, Hex: {}", ArgVariant(42), ArgVariant("Test"), ArgVariant(0x2A));
    EXPECT_STREQ(output, "Int: 42, Str: Test, Hex: 42");
}

// Test alias with not enough arguments
TEST(FormatterTest, AliasNotEnoughArguments) {
    char output[256];
    Formatter::print(output, sizeof(output), "Value: {}, Another: {}", ArgVariant(42));
    EXPECT_STREQ(output, "Value: 42, Another: {}");
}

// Test alias with string argument
TEST(FormatterTest, AliasStringArgument) {
    char output[256];
    Formatter::print(output, sizeof(output), "String: {}", ArgVariant("Test"));
    EXPECT_STREQ(output, "String: Test");
}

// Test alias with multiple types
TEST(FormatterTest, AliasMultipleTypes) {
    char output[256];
    Formatter::print(
        output, sizeof(output), 
        "Int8: {}, Int16: {}, Int32: {}, Uint8: {}, Uint16: {}, Uint32: {}",
        ArgVariant((int8_t)8),
        ArgVariant((int16_t)16),
        ArgVariant((int32_t)32),
        ArgVariant((uint8_t)8),
        ArgVariant((uint16_t)16),
        ArgVariant((uint32_t)32)
    );
    EXPECT_STREQ(output, "Int8: 8, Int16: 16, Int32: 32, Uint8: 8, Uint16: 16, Uint32: 32");
}

// Test string overflow
TEST(FormatterTest, StringOverflow) {
    char output[10];
    ArgVariant args[] = { ArgVariant("This is a very long string") };
    Formatter::print(output, sizeof(output), "{}", args, 1);
    EXPECT_EQ(strlen(output), 9u);
    EXPECT_STREQ(output, "This is a");
}

// Test number overflow
TEST(FormatterTest, NumberOverflow) {
    char output[10];
    ArgVariant args[] = { ArgVariant(1234567890) };
    Formatter::print(output, sizeof(output), "{}", args, 1);
    EXPECT_EQ(strlen(output), 9u);
    EXPECT_STREQ(output, "123456789");
}
