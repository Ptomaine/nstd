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

#include <limits.h>
#include "external/giant/giant.hpp"

namespace giant
{
    // compile-time endianness swap based on http://stackoverflow.com/a/36937049
    template<class T, std::size_t... N>
    constexpr T c_swap_impl(T i, std::index_sequence<N...>)
    {
      return (((i >> N * CHAR_BIT & std::uint8_t(-1)) << (sizeof(T) - 1 - N) * CHAR_BIT) | ...);
    }

    template<class T, class U = std::make_unsigned_t<T>>
    constexpr U c_swap(T i)
    {
      return c_swap_impl<U>(i, std::make_index_sequence<sizeof(T)>{});
    }

    template<typename T>
    constexpr T c_letobe( const T &in ) {
        return c_swap( in );
    }
    template<typename T>
    constexpr T c_betole( const T &in ) {
        return c_swap( in );
    }

    template<typename T>
    constexpr T c_letoh( const T &in ) {
        return type == xinu_type ? in : c_swap( in );
    }

    template<typename T>
    constexpr T c_htole( const T &in ) {
        return type == xinu_type ? in : c_swap( in );
    }

    template<typename T>
    constexpr T c_betoh( const T &in ) {
        return type == unix_type ? in : c_swap( in );
    }
    template<typename T>
    constexpr T c_htobe( const T &in ) {
        return type == unix_type ? in : c_swap( in );
    }
}

namespace nstd
{
namespace giant = giant;
}
