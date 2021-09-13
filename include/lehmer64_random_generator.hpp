#pragma once

/*
MIT License
Copyright (c) 2020 Arlen Keshabyan (arlen.albert@gmail.com)
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

#include <cstdint>
#include "external/intx/include/intx/int128.hpp"

namespace nstd
{

class lehmer64_random_generator
{
public:
    lehmer64_random_generator(uint64_t lehmer64_seed_ = 0, uint64_t splitmix64_seed_ = 0)
    {
        if (lehmer64_seed_ != 0) lehmer64_seed(lehmer64_seed_);
        if (splitmix64_seed_ != 0) splitmix64_seed(splitmix64_seed_);
    }

    void splitmix64_seed(uint64_t seed)
    {
        splitmix64_x = seed;
    }
    
    uint64_t splitmix64()
    {
        uint64_t z = (splitmix64_x += UINT64_C(0x9E3779B97F4A7C15));
        z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
        z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);

        return z ^ (z >> 31);
    }

    uint32_t splitmix64_cast32()
    {
        return static_cast<uint32_t>(splitmix64());
    }

    uint64_t splitmix64_stateless(uint64_t index)
    {
        uint64_t z = (index + UINT64_C(0x9E3779B97F4A7C15));
        z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
        z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);

        return z ^ (z >> 31);
    }

    void lehmer64_seed(uint64_t seed)
    {
        lehmer64_state = ((static_cast<__uint128_t>(splitmix64_stateless(seed))) << 64) + splitmix64_stateless(seed + 1);
    }

    uint64_t lehmer64()
    {
        lehmer64_state *= UINT64_C(0xda942042e4dd58b5);

        return static_cast<uint64_t>(lehmer64_state >> 64);
    }

private:
    uint64_t splitmix64_x { 0 };
    intx::uint128 lehmer64_state { 0 };
};

}
