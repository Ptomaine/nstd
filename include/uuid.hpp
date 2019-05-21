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
    uuid() : uuid_data(16, 0){}
    uuid(const std::vector<uint8_t> &data) : uuid_data(data) { if (std::size(uuid_data) != 16) { uuid_data.~vector(); throw std::runtime_error("uuid"); } }
    uuid(std::vector<uint8_t> &&data) : uuid_data(std::move(data)) { if (std::size(uuid_data) != 16) { uuid_data.~vector(); throw std::runtime_error("uuid"); } }
    uuid(uuid&&) = default;
    uuid(const uuid&) = default;
    uuid &operator=(const uuid&) = default;

    bool operator ==(const uuid &other) const
    {
        if (std::size(uuid_data) != std::size(other.uuid_data)) return false;

        return std::equal(std::begin(uuid_data), std::end(uuid_data), std::begin(other.uuid_data));
    }

    bool operator !=(const uuid &other) const
    {
        return !(operator==(other));
    }

    bool is_null() const
    {
        return std::all_of(std::begin(uuid_data), std::end(uuid_data), [](auto &&i) { return i == 0; });
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
            result.insert(std::begin(result), '{');
            result.insert(std::end(result), '}');
        }

        return result;
    }

    const std::vector<uint8_t> &data() const
    {
        return uuid_data;
    }

    static uuid generate_random()
    {
        if (!seeded)
        {
            init_random();
        }

        union { uint8_t bytes[16]; uint64_t data[2]; } s;

        s.data[0] = xorshift128plus(seed);
        s.data[1] = xorshift128plus(seed);

        s.bytes[6] = (s.bytes[6] & 0xf0) | 0x04;
        s.bytes[8] = (s.bytes[8] & 0x30) + 8;

        return std::vector<uint8_t>{ s.bytes, s.bytes + 16 };
    }

    template<typename RandomProvider = random_provider_default<uint64_t>>
    static void init_random(RandomProvider &&random_provider = RandomProvider())
    {
        do
        {
            seed[0] = random_provider();
            seed[1] = random_provider();
        } while (!(seed[0] && seed[1]));

        seeded = true;
    }

    static bool validate_uuid_string(std::string_view str, bool strict = false)
    {
    	if (std::size(str) < 32) return false;

    	if (str.front() == '{' && str.back() == '}') str = str.substr(1, std::size(str) - 2);

        std::string uuid_str;

        std::copy_if(std::begin(str), std::end(str), std::back_inserter(uuid_str), [](auto &&i) { return std::isxdigit(i); });

        if (std::size(uuid_str) != 32) return false;

        if (strict)
        {
            if (uuid_str[12] != '4') return false;

            if (std::any_of(std::begin(str), std::end(str), [](auto &&i) { return i == sep_char; }))
                for (auto pos : dash_positions) if(str[pos] != sep_char) return false;
        }

        return true;
    }

    static uuid parse(std::string_view uuid_str, bool strict = false)
    {
        if (!validate_uuid_string(uuid_str, strict)) return {};

        if (uuid_str.front() == '{' && uuid_str.back() == '}') uuid_str = uuid_str.substr(1, std::size(uuid_str) - 2);

        auto index {0};
        char bytes[3];
        uint8_t uuid_bytes[16];

        for (auto it = std::begin(uuid_str), end = std::end(uuid_str); it != end; )
        {
            if (*it == sep_char) { ++it; continue; }

            bytes[1] = *it++;
            bytes[0] = *it++;
            bytes[2] = 0;

            uuid_bytes[index++] = static_cast<uint8_t>(std::stoul(bytes, nullptr, 16));
        }

        return std::vector<uint8_t>{uuid_bytes, uuid_bytes + 16};
    }

    static uint64_t get_random_number()
    {
        return random_provider_default<uint64_t>()();
    }

private:
    std::vector<uint8_t> uuid_data;

    static uint64_t xorshift128plus(uint64_t *s)
    {
        auto s1 = s[0];
        const auto s0 = s[1];

        s[0] = s0;
        s1 ^= s1 << 23;
        s[1] = s1 ^ s0 ^ (s1 >> 18) ^ (s0 >> 5);

        return s[1] + s0;
    }

    inline static uint64_t seed[2] { 0 };
    inline static bool seeded { false };
    inline static constexpr std::array<size_t, 4> dash_positions {8, 13, 18, 23};
    inline static constexpr const char sep_char { '-' };
};

}

