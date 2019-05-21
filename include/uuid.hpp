#pragma once

/*
MIT License
Copyright (c) 2017 Arlen Keshabyan (arlen.albert@gmail.com)
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <iterator>
#include <random>
#include <string>
#include <vector>
#include "random_provider_default.hpp"

namespace nstd::uuid
{

class uuid
{
public:
    constexpr uuid() : uuid_data { 0 }{}
    constexpr uuid(const std::array<uint8_t, 16> &data) : uuid_data { data } { }
    constexpr uuid(std::array<uint8_t, 16> &&data) : uuid_data { std::move(data) } { }
    constexpr uuid(uuid&&) = default;
    constexpr uuid(const uuid&) = default;
    constexpr uuid &operator=(const uuid&) = default;

    constexpr bool operator ==(const uuid &other) const
    {
        if (std::size(uuid_data) != std::size(other.uuid_data)) return false;

        return std::equal(std::begin(uuid_data), std::end(uuid_data), std::begin(other.uuid_data));
    }

    constexpr bool operator !=(const uuid &other) const
    {
        return !(operator==(other));
    }

    bool is_null() const
    {
        return uuid_data == null_uuid;
    }

    std::string to_string(bool use_dashes = true, bool use_uppercase = false, bool use_braces = false) const
    {
        static const char *const chars[16] { "0123456789abcdef", "0123456789ABCDEF" };
        std::string result;
        auto inserter{ std::back_inserter(result) };

        for (auto i{ 0 }; i < 32; ++i)
        {
            auto n = uuid_data[i >> 1];

            n = (i & 1) ? (n >> 4) : (n & 0xf);

            inserter++ = chars[use_uppercase][n];
        }

        if (use_dashes) for (auto pos : dash_positions) result.insert(pos, 1, sep_char);

        if (use_braces)
        {
            result.insert(std::begin(result), op_brace);
            result.insert(std::end(result), cl_brace);
        }

        return result;
    }

    constexpr const std::array<uint8_t, 16> &data() const
    {
        return uuid_data;
    }

    static constexpr uuid generate_random()
    {
        union { uint8_t bytes[16]; uint64_t data[2]; } s { 0 };

        s.data[0] = xorshift128plus(seed);
        s.data[1] = xorshift128plus(seed);

        s.bytes[6] = (s.bytes[6] & 0xf0) | 0x04;
        s.bytes[8] = (s.bytes[8] & 0x30) + 8;

        std::array<uint8_t, 16> arr { 0 };

        for (int idx { 0 }; idx < 16; ++idx) arr[idx] = s.bytes[idx];

        return arr;
    }

    template<typename RandomProvider = random_provider_default<uint64_t>>
    static constexpr void init_random(RandomProvider &&random_provider = RandomProvider())
    {
        do
        {
            seed[0] = random_provider();
            seed[1] = random_provider();
        } while (!(seed[0] && seed[1]));
    }

    static constexpr bool validate_uuid_string(std::string_view str, bool strict = false)
    {
    	if (std::size(str) < 32) return false;

    	if (str.starts_with(op_brace) && str.ends_with(cl_brace)) str = str.substr(1, std::size(str) - 2);

        auto count { 0 };

        for (auto d : str) if (is_hex_digit(d)) ++count;

        if (count != 32) return false;

        if (strict)
        {
            if (std::any_of(std::begin(str), std::end(str), [](auto &&i) { return i == sep_char; }))
                for (auto pos : dash_positions) if(str[pos] != sep_char) return false;
        }

        return true;
    }

    static constexpr uuid parse(std::string_view uuid_str, bool strict = false, bool throw_on_error = true)
    {
        if (!validate_uuid_string(uuid_str, strict)) { if (throw_on_error) { throw std::runtime_error { "Parse error. Invalid UUID." }; } else { return {}; } }

        if (uuid_str.starts_with(op_brace) && uuid_str.ends_with(cl_brace)) uuid_str = uuid_str.substr(1, std::size(uuid_str) - 2);

        auto index { 0 };
        char bytes[2] = { 0 };
        std::array<uint8_t, 16> uuid_bytes { 0 };

        for (auto it = std::begin(uuid_str), end = std::end(uuid_str); it != end; )
        {
            if (*it == sep_char) { ++it; continue; }

            bytes[1] = *it++;
            bytes[0] = *it++;

            uuid_bytes[index++] = parse_hex_byte(bytes);
        }

        return uuid_bytes;
    }

    inline static constexpr std::array<uint8_t, 16> null_uuid { 0 };

private:
    std::array<uint8_t, 16> uuid_data;

    static constexpr uint64_t xorshift128plus(uint64_t *s)
    {
        auto s1 = s[0];
        const auto s0 = s[1];

        s[0] = s0;
        s1 ^= s1 << 23;
        s[1] = s1 ^ s0 ^ (s1 >> 18) ^ (s0 >> 5);

        return s[1] + s0;
    }

    static constexpr bool is_hex_digit(char d)
    {
        return (d >= '0' && d <= '9') || (d >= 'a' && d <= 'f') || (d >= 'A' && d <= 'F');
    }

    static constexpr uint8_t parse_hex_digit(const char c)
    {
        return (c >= '0' && c <= '9') ? c - '0' : (c >= 'a' && c <= 'f') ? 10 + c - 'a' : (c >= 'A' && c <= 'F') ? 10 + c - 'A' : 0;
    }

    static constexpr uint8_t parse_hex_byte(const char ptr[2])
    {
        return (parse_hex_digit(ptr[0]) << 4) + parse_hex_digit(ptr[1]);
    }

    inline static uint64_t seed[2] { 0 };
    inline static constexpr std::array<size_t, 4> dash_positions {8, 13, 18, 23};
    inline static constexpr const char sep_char { '-' }, op_brace { '{' }, cl_brace { '}' };
};

namespace literals
{
    constexpr uuid operator "" _uuid(const char *str, size_t N)
    {
        return uuid::parse(std::string_view { str, N });
    }
}

}

