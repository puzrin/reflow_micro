#include <gtest/gtest.h>
#include "lib/ring_logger/ring_logger_tokenizer.hpp"

using namespace ring_logger;

class StringTokenizerTest : public ::testing::Test {
protected:
    void verify_tokens(const std::string& input,
                      const std::vector<std::pair<std::string, bool>>& expected) {
        StringTokenizer tokenizer(input);
        auto it = tokenizer.begin();

        for (const auto& [exp_text, exp_is_placeholder] : expected) {
            ASSERT_NE(it, tokenizer.end());
            EXPECT_EQ(std::string((*it).text), exp_text);
            EXPECT_EQ((*it).is_placeholder, exp_is_placeholder);
            ++it;
        }

        EXPECT_EQ(it, tokenizer.end());
    }
};

TEST_F(StringTokenizerTest, EmptyString) {
    verify_tokens("", {});
}

TEST_F(StringTokenizerTest, NoPlaceholders) {
    verify_tokens("simple text", {
        {"simple text", false}
    });
}

TEST_F(StringTokenizerTest, SinglePlaceholder) {
    verify_tokens("before {} after", {
        {"before ", false},
        {"{}", true},
        {" after", false}
    });
}

TEST_F(StringTokenizerTest, MultiplePlaceholders) {
    verify_tokens("foo {} bar {} bas", {
        {"foo ", false},
        {"{}", true},
        {" bar ", false},
        {"{}", true},
        {" bas", false}
    });
}

TEST_F(StringTokenizerTest, ConsecutivePlaceholders) {
    verify_tokens("text{}{}", {
        {"text", false},
        {"{}", true},
        {"{}", true}
    });
}

TEST_F(StringTokenizerTest, PlaceholdersAtBoundaries) {
    verify_tokens("{} text {}", {
        {"{}", true},
        {" text ", false},
        {"{}", true}
    });
}

TEST_F(StringTokenizerTest, IteratorOperations) {
    StringTokenizer tokenizer("a {} b");
    auto it = tokenizer.begin();

    // Test operator*
    EXPECT_EQ((*it).text, "a ");
    EXPECT_FALSE((*it).is_placeholder);

    // Test operator++ (prefix)
    ++it;
    EXPECT_EQ((*it).text, "{}");
    EXPECT_TRUE((*it).is_placeholder);

    // Test operator++ (postfix)
    auto old_it = it++;
    EXPECT_EQ((*old_it).text, "{}");
    EXPECT_EQ((*it).text, " b");
}

TEST_F(StringTokenizerTest, RangeBasedFor) {
    StringTokenizer tokenizer("x {} y");
    std::vector<std::pair<std::string, bool>> actual;

    for (const auto& token : tokenizer) {
        actual.emplace_back(std::string(token.text), token.is_placeholder);
    }

    std::vector<std::pair<std::string, bool>> expected = {
        {"x ", false},
        {"{}", true},
        {" y", false}
    };

    EXPECT_EQ(actual, expected);
}

// Test incorrect placehoder
TEST_F(StringTokenizerTest, Incomplete_Placeholders) {
    verify_tokens("text { text", {
        {"text { text", false}
    });

    verify_tokens("text } text", {
        {"text } text", false}
    });
}

// Test space characters
TEST_F(StringTokenizerTest, WhitespaceHandling) {
    verify_tokens("  {}  ", {
        {"  ", false},
        {"{}", true},
        {"  ", false}
    });
}
