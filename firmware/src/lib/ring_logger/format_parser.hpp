#pragma once

/*

// Simple placeholder without format
{}

// Hex format (lowercase/uppercase)
{:x}  // ff
{:X}  // FF

// Hex with leading 0X
{:#x} // 0Xff
{:#X} // 0XFF

// Zero-padded hex
{:04x} // 00ff

// Binary format
{:b}   // 101010
{:#b}  // 0B101010

// Decimal
{:d}   // 42

*/

#include <string>
#include <string_view>
#include <etl/format_spec.h>

namespace ring_logger {

class FormatParser {
public:
    static size_t get_placeholder_length(std::string_view str, size_t pos) {
        const size_t start = pos;
        const size_t max = str.length();

        if (pos >= max || str[pos] != '{') return 0;
        if (pos + 1 >= max) return 0;

        // Check empty {}
        if (str[pos + 1] == '}') return 2;

        // Check :
        if (str[pos + 1] != ':') return 0;
        if (pos + 2 >= max) return 0;
        pos += 2;

        // Check #
        if (pos < max && str[pos] == '#') {
            pos++;
            if (pos >= max) return 0;
        }

        // Check zero padding (must be first)
        if (pos < max && str[pos] == '0') {
            pos++;
            if (pos >= max || !std::isdigit(str[pos])) return 0;
            while (pos < max && std::isdigit(str[pos])) pos++;
            if (pos >= max) return 0;
        }
        // Check just width
        else if (pos < max && std::isdigit(str[pos])) {
            while (pos < max && std::isdigit(str[pos])) pos++;
            if (pos >= max) return 0;
        }

        // Check type and final }
        if (pos + 1 >= max) return 0;

        switch(str[pos]) {
            case 'x': case 'X': case 'b': case 'd': break;
            default: return 0;
        }
        pos++;

        if (str[pos] != '}') return 0;
        return pos + 1 - start;
    }

    static void parse_format(std::string_view str, size_t pos, etl::format_spec& spec) {
        const size_t max = str.length();
        if (pos >= max || str[pos] != '{') {
            reset_spec(spec);
            return;
        }
        if (pos + 1 >= max) {
            reset_spec(spec);
            return;
        }

        // Check empty format "{}"
        if (str[pos + 1] == '}') return;

        // Check format specifier start
        if (str[pos + 1] != ':') {
            reset_spec(spec);
            return;
        }
        if (pos + 2 >= max) {
            reset_spec(spec);
            return;
        }
        pos += 2;

        // Check #
        if (pos < max && str[pos] == '#') {
            spec.show_base(true);
            pos++;
            if (pos >= max) {
                reset_spec(spec);
                return;
            }
        }

        // Check zero padding (must be first)
        if (pos < max && str[pos] == '0') {
            spec.fill('0');
            pos++;
            if (pos >= max || !std::isdigit(str[pos])) {
                reset_spec(spec);
                return;
            }
            int width = 0;
            while (pos < max && std::isdigit(str[pos])) {
                width = width * 10 + (str[pos] - '0');
                pos++;
            }
            spec.width(width);
            if (pos >= max) {
                reset_spec(spec);
                return;
            }
        }
        // Check just width
        else if (pos < max && std::isdigit(str[pos])) {
            int width = 0;
            while (pos < max && std::isdigit(str[pos])) {
                width = width * 10 + (str[pos] - '0');
                pos++;
            }
            spec.width(width);
            if (pos >= max) {
                reset_spec(spec);
                return;
            }
        }

        // Must be type specifier
        switch(str[pos]) {
            case 'x': spec.base(16); break;
            case 'X': spec.base(16).upper_case(true); break;
            case 'b': spec.base(2); break;
            case 'd': spec.base(10); break;
            default: reset_spec(spec); return;
        }
        pos++;

        // Must end with "}"
        if (str[pos] != '}') {
            reset_spec(spec);
            return;
        }
    }

private:
    static void reset_spec(etl::format_spec& spec) {
        spec.fill(' ');
        spec.width(0);
        spec.base(10);
        spec.show_base(false);
        spec.upper_case(false);
    }
};

} // namespace ring_logger
