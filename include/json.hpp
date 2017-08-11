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

#include "external/json/src/json.hpp"
#include "ordered_map_set.hpp"

namespace nstd
{

template<class Key, class T, class Ignore, class Allocator, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>>
using ordered_map = tsl::ordered_map<Key, T, Hash, KeyEqual, Allocator>;

namespace json
{
using namespace nlohmann;
using json_ord = nlohmann::basic_json<ordered_map>;
}

}

inline nstd::json::json_ord operator "" _json_ord(const char* s, std::size_t n)
{
    return nstd::json::json_ord::parse(s, s + n);
}

inline nstd::json::json_ord::json_pointer operator "" _json_ord_pointer(const char* s, std::size_t n)
{
    return nstd::json::json_ord::json_pointer(std::string(s, n));
}
