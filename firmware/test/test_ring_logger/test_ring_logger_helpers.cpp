#include <gtest/gtest.h>
#include "ring_logger/ring_logger.hpp"

TEST(RingLoggerTest, Test_is_label_in_list) {
    auto in_list = ring_logger::is_label_in_list;

    // These should match
    EXPECT_EQ(true, in_list("foo", "foo"));
    EXPECT_EQ(true, in_list("foo", " foo "));
    EXPECT_EQ(true, in_list("foo", "foo,bar"));
    EXPECT_EQ(true, in_list("foo", "foo, bar"));
    EXPECT_EQ(true, in_list("foo", "foo , bar"));
    EXPECT_EQ(true, in_list("foo", "foo,bar,,"));
    EXPECT_EQ(true, in_list("foo", "bar,foo"));
    EXPECT_EQ(true, in_list("foo", "bar, foo"));
    EXPECT_EQ(true, in_list("foo", "bar , foo"));
    EXPECT_EQ(true, in_list("foo", "bar,foo,,"));
    EXPECT_EQ(true, in_list("foo", "bar, foo,"));
    EXPECT_EQ(true, in_list("foo", "bar, foo, "));
    EXPECT_EQ(true, in_list("foo", "bar, foo , "));

    // These should not match
    EXPECT_EQ(false, in_list("foo", ""));
    EXPECT_EQ(false, in_list("foo", "foo bar"));
    EXPECT_EQ(false, in_list("foo", "foo_bar"));
    EXPECT_EQ(false, in_list("foo", "bar foo"));
    EXPECT_EQ(false, in_list("foo", "bar_foo"));
}
