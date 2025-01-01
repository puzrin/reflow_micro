#pragma once

#include <string>
#include <string_view>

namespace ring_logger {

class StringTokenizer {
public:
    struct Token {
        std::string_view text;
        bool is_placeholder;

        Token(std::string_view t, bool ph) : text(t), is_placeholder(ph) {}
    };

    class iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = Token;
        using difference_type = std::ptrdiff_t;
        using pointer = Token*;
        using reference = Token&;

        iterator(std::string_view str, size_t pos = 0)
            : source(str), current_pos(pos) {
            if (current_pos < source.length()) {
                find_next_token();
            }
        }

        iterator& operator++() {
            if (current_pos < source.length()) {
                current_pos += current_token.text.length();
                find_next_token();
            }
            return *this;
        }

        iterator operator++(int) {
            iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const iterator& other) const {
            return current_pos == other.current_pos;
        }

        bool operator!=(const iterator& other) const {
            return !(*this == other);
        }

        const Token& operator*() const {
            return current_token;
        }

    private:
        std::string_view source;
        size_t current_pos;
        Token current_token{"", false};

        void find_next_token() {
            if (current_pos >= source.length()) {
                return;
            }

            if (source.substr(current_pos, 2) == "{}") {
                current_token = Token{source.substr(current_pos, 2), true};
            } else {
                size_t next_placeholder = source.find("{}", current_pos);
                if (next_placeholder == std::string::npos) {
                    current_token = Token{source.substr(current_pos), false};
                } else {
                    current_token = Token{
                        source.substr(current_pos, next_placeholder - current_pos),
                        false
                    };
                }
            }
        }
    };

    StringTokenizer(std::string_view input) : source(input) {}

    iterator begin() { return iterator(source); }
    iterator end() { return iterator(source, source.length()); }

private:
    std::string_view source;
};

} // namespace ring_logger
