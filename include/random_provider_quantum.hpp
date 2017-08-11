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
#include <iterator>
#include <mutex>
#include "urdl.hpp"
#include "json.hpp"

namespace nstd
{

template<typename T = uint64_t>
class random_provider_quantum
{
public:
    random_provider_quantum()
    {
        std::scoped_lock lock{ _mutex };

        preload_quantum_data();
    }

    auto operator ()() -> decltype(auto)
    {
        std::scoped_lock lock{ _mutex };

        auto data_begin { std::data(_quantum_data) };

        if (_cursor + sizeof(T) >= _cache_size)
        {
            preload_quantum_data();
        }

        T result { *reinterpret_cast<T*>(data_begin + _cursor) };

        _cursor += sizeof(T);

        return result;
    }

private:
    static constexpr const char *_url { "http://qrng.anu.edu.au/API/jsonI.php?type=uint8&length=" };
    static constexpr const std::size_t _cache_size { 1024 };
    inline static std::array<uint8_t, _cache_size> _quantum_data;
    inline static std::size_t _cursor { 0 };
    inline static std::mutex _mutex;

    void preload_quantum_data()
    {
        auto const length_to_download { _cache_size - (_cache_size - (!_cursor ? _cache_size : _cursor)) };
        auto const length_to_move { _cache_size - length_to_download };
        auto url { std::string(_url) + std::to_string(length_to_download) };
        auto json_str { nstd::download_url(url).second };
        auto json { nstd::json::json::parse(json_str) };

        if (!json[0]["success"])
        {
            using namespace std::string_literals;

            throw std::runtime_error("Request for quantum random numbers failed! URL is '"s + url + "'");
        }

        auto &json_random_data { json[0]["data"] };
        auto data_begin { std::data(_quantum_data) };

        if (length_to_move) std::move(data_begin + _cursor, data_begin + _cache_size, data_begin);

        _cursor = 0;

        std::transform(std::begin(json_random_data), std::end(json_random_data), data_begin + length_to_move, [](auto &&v) { return static_cast<uint8_t>(v); });
    }
};

}
