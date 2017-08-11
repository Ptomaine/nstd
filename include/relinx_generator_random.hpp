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

#include <limits>
#include "relinx.hpp"
#include "random_provider_default.hpp"

namespace nstd
{

template<typename RandomProvider = random_provider_default<uint64_t>>
class random_iterator_adapter
{
public:
    using self_type = random_iterator_adapter;
    using value_type = uint64_t;
    using difference_type = std::ptrdiff_t;
    using pointer = const value_type*;
    using reference = const value_type&;
    using iterator_category = std::input_iterator_tag;

    random_iterator_adapter() = default;
    random_iterator_adapter(bool begin_iterator_flag) : _result(begin_iterator_flag ? random_provider() : std::numeric_limits<uint64_t>::max()) { }
    random_iterator_adapter(const random_iterator_adapter &) = default;
    random_iterator_adapter(random_iterator_adapter &&) = default;
    random_iterator_adapter &operator=(const random_iterator_adapter &) = default;
    random_iterator_adapter &operator=(random_iterator_adapter &&) = default;

    auto operator==(const self_type &s) const -> bool
    {
        return _result == s._result;
    }

    auto operator!=(const self_type &s) const -> bool
    {
        return !(*this == s);
    }

    auto operator*() const -> const value_type&
    {
        return _result;
    }

    auto operator->() -> const value_type
    {
        return *(*this);
    }

    auto operator++() -> self_type&
    {
        do
        {
            _result = random_provider();
        }
        while (_result == std::numeric_limits<uint64_t>::max());

        return *this;
    }

    auto operator++(int) -> self_type
    {
        auto __tmp = *this;

        ++(*this);

        return __tmp;
    }

protected:
    uint64_t _result { std::numeric_limits<uint64_t>::max() };
    static inline RandomProvider random_provider;
};

template<typename RandomProvider = random_provider_default<uint64_t>>
auto from_random()
{
    return nstd::relinx::from(random_iterator_adapter<RandomProvider>(true), random_iterator_adapter<RandomProvider>());
}

}
