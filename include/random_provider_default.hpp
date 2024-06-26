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

#include <chrono>
#include <type_traits>
#include <mutex>
#include <random>

namespace nstd
{

template<typename T = uint64_t>
requires std::is_integral_v<T> || std::is_floating_point_v<T>
class random_provider_default
{
public:
    auto operator ()() -> decltype(auto)
    {
        std::scoped_lock lock{ _mutex };

        return rand_dist(rand_gen);
    }

private:
    inline static std::mt19937_64 rand_gen{ static_cast<T>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count()) };
    inline static std::uniform_int_distribution<T> rand_dist{ 1 };
    inline static std::mutex _mutex;
};

template<typename T>
requires std::is_integral_v<T> || std::is_floating_point_v<T>
auto random_number_between = [](T low, T high)
{
    auto random_func = [distribution  = std::uniform_int_distribution<T>(low, high), 
                        random_engine = std::mt19937{ std::random_device{}() }]() mutable
    {
        return distribution(random_engine);
    };

    return random_func;
};

}
