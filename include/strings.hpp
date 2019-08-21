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

#include <cwctype>
#include <cctype>
#include <iomanip>
#include <limits>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>

#include "utf8.hpp"

namespace nstd::str
{

namespace
{

template <typename StringViewType>
inline auto trim_left_impl(StringViewType sv, const StringViewType chars_to_remove)
{
    sv.remove_prefix(std::min(sv.find_first_not_of(chars_to_remove), std::size(sv)));

    return sv;
}

template <typename StringViewType>
inline auto trim_right_impl(StringViewType sv, const StringViewType chars_to_remove)
{
    sv.remove_suffix(sv.size() - (sv.find_last_not_of(chars_to_remove) + 1));

    return sv;
}

template <typename StringViewType>
inline auto trim_impl(StringViewType sv, const StringViewType chars_to_remove)
{
    return trim_right_impl(trim_left_impl(sv, chars_to_remove), chars_to_remove);
}

}

inline std::u8string from_utf16_to_utf8(const std::u16string_view s)
{
    std::u8string result;

    nstd::utf8::utf16to8(std::begin(s), std::end(s), std::back_inserter(result));

    return result;
}

inline std::u8string from_utf32_to_utf8(const std::u32string_view s)
{
    std::u8string result;

    nstd::utf8::utf32to8(std::begin(s), std::end(s), std::back_inserter(result));

    return result;
}

inline std::u16string from_utf8_to_utf16(const std::u8string_view s)
{
    std::u16string result;

    nstd::utf8::utf8to16(std::begin(s), std::end(s), std::back_inserter(result));

    return result;
}

inline std::u16string from_utf32_to_utf16(const std::u32string_view s)
{
    std::u8string in;
    std::u16string result;

    nstd::utf8::utf32to8(std::begin(s), std::end(s), std::back_inserter(in));
    nstd::utf8::utf8to16(std::begin(in), std::end(in), std::back_inserter(result));

    return result;
}

inline std::u32string from_utf8_to_utf32(const std::u8string_view s)
{
    std::u32string result;

    nstd::utf8::utf8to32(std::begin(s), std::end(s), std::back_inserter(result));

    return result;
}

inline std::u32string from_utf16_to_utf32(const std::u16string_view s)
{
    std::u8string in;
    std::u32string result;

    nstd::utf8::utf16to8(std::begin(s), std::end(s), std::back_inserter(in));
    nstd::utf8::utf8to32(std::begin(in), std::end(in), std::back_inserter(result));

    return result;
}

constexpr const char     *const whitespace_chars { " \t\n\v\f\r" };
constexpr const wchar_t  *const wwhitespace_chars { L" \t\n\v\f\r" };
constexpr const char8_t *const u8whitespace_chars { u8" \t\n\v\f\r" };
constexpr const char16_t *const u16whitespace_chars { u" \t\n\v\f\r" };
constexpr const char32_t *const u32whitespace_chars { U" \t\n\v\f\r" };
constexpr const char *const boolalpha[] = { "false", "true" };
constexpr const wchar_t *const wboolalpha[] = { L"false", L"true" };
constexpr const char8_t *const u8boolalpha[] = { u8"false", u8"true" };
constexpr const char16_t *const u16boolalpha[] = { u"false", u"true" };
constexpr const char32_t *const u32boolalpha[] = { U"false", U"true" };

inline std::string_view trim_left(std::string_view sv, const std::string_view chars_to_remove = whitespace_chars) { return trim_left_impl(sv, chars_to_remove); }
inline std::wstring_view trim_left(std::wstring_view sv, const std::wstring_view chars_to_remove = wwhitespace_chars) { return trim_left_impl(sv, chars_to_remove); }
inline std::u8string_view trim_left(std::u8string_view sv, const std::u8string_view chars_to_remove = u8whitespace_chars) { return trim_left_impl(sv, chars_to_remove); }
inline std::u16string_view trim_left(std::u16string_view sv, const std::u16string_view chars_to_remove = u16whitespace_chars) { return trim_left_impl(sv, chars_to_remove); }
inline std::u32string_view trim_left(std::u32string_view sv, const std::u32string_view chars_to_remove = u32whitespace_chars) { return trim_left_impl(sv, chars_to_remove); }

inline std::string_view trim_right(std::string_view sv, const std::string_view chars_to_remove = whitespace_chars) { return trim_right_impl(sv, chars_to_remove); }
inline std::wstring_view trim_right(std::wstring_view sv, const std::wstring_view chars_to_remove = wwhitespace_chars) { return trim_right_impl(sv, chars_to_remove); }
inline std::u8string_view trim_right(std::u8string_view sv, const std::u8string_view chars_to_remove = u8whitespace_chars) { return trim_right_impl(sv, chars_to_remove); }
inline std::u16string_view trim_right(std::u16string_view sv, const std::u16string_view chars_to_remove = u16whitespace_chars) { return trim_right_impl(sv, chars_to_remove); }
inline std::u32string_view trim_right(std::u32string_view sv, const std::u32string_view chars_to_remove = u32whitespace_chars) { return trim_right_impl(sv, chars_to_remove); }

inline std::string_view trim(std::string_view sv, const std::string_view chars_to_remove = whitespace_chars) { return trim_impl(sv, chars_to_remove); }
inline std::wstring_view trim(std::wstring_view sv, const std::wstring_view chars_to_remove = wwhitespace_chars) { return trim_impl(sv, chars_to_remove); }
inline std::u8string_view trim(std::u8string_view sv, const std::u8string_view chars_to_remove = u8whitespace_chars) { return trim_impl(sv, chars_to_remove); }
inline std::u16string_view trim(std::u16string_view sv, const std::u16string_view chars_to_remove = u16whitespace_chars) { return trim_impl(sv, chars_to_remove); }
inline std::u32string_view trim(std::u32string_view sv, const std::u32string_view chars_to_remove = u32whitespace_chars) { return trim_impl(sv, chars_to_remove); }

template <typename T>
using any_string = std::basic_string<T, std::char_traits<T>, std::allocator<T>>;

template <typename T>
inline any_string<T>& replace_all_inplace(any_string<T>& str, const any_string<T>& from, const any_string<T>& to)
{
    if (std::empty(from)) return str;

    size_t start_pos { 0 };

    while((start_pos = str.find(from, start_pos)) != std::string::npos)
    {
        str.replace(start_pos, from.length(), to);

        start_pos += std::size(to);
    }

    return str;
}

template <typename T>
inline any_string<T> replace_all(const any_string<T>& cstr, const any_string<T>& from, const any_string<T>& to)
{
    any_string<T> str { cstr };

    return replace_all_inplace(str, from, to);
}

template <typename T>
inline any_string<T> replace_regex(const any_string<T>& cstr, const any_string<T>& from, const any_string<T>& to, std::regex_constants::syntax_option_type opts = std::regex_constants::ECMAScript)
{
    return std::regex_replace(cstr, std::basic_regex<T>{ from, opts }, to);
}

template<typename T>
inline bool is_empty_or_ws(const any_string<T> &str)
{
    static_assert(std::is_same_v<T, char> || std::is_same_v<T, wchar_t>, "Unsupported string type provided to is_empty_or_ws. Supported types are std::string and std::wstring");

    return std::all_of(std::begin(str), std::end(str), [](auto &&c) { if constexpr (std::is_same_v<T, char>) return std::isspace(static_cast<unsigned char>(c)); else return std::iswspace(c); });
}

template <typename C, typename T>
inline any_string<T> join(const C& container, const any_string<T>& delimiter)
{
    std::basic_stringstream<T> oss;
    auto begin = std::begin(container);
    auto end = std::end(container);

    if (begin != end) { oss << *begin; while (++begin != end) oss << delimiter << *begin; }

    return oss.str();
}

template <typename T>
inline std::vector<any_string<T>> split_regex(const any_string<T>& input, const any_string<T>& pattern, std::regex_constants::syntax_option_type opts = std::regex_constants::ECMAScript)
{
    std::basic_regex<T> re { pattern, opts };

    return { std::regex_token_iterator<typename any_string<T>::const_iterator> { std::begin(input), std::end(input), re, -1 },
             std::regex_token_iterator<typename any_string<T>::const_iterator> {} };
}

template<typename ... Args>
inline std::string compose_string(const Args& ... args)
{
    std::ostringstream oss;

    ((oss << args), ... );

    return oss.str();
}

template<typename ... Args>
inline std::wstring compose_wstring(const Args& ... args)
{
    std::wostringstream oss;

    ((oss << args), ... );

    return oss.str();
}

template<typename NumericType>
inline std::string numeric_to_string(NumericType value, int precision = -1)
{
         if (value ==  std::numeric_limits<NumericType>::infinity()) return  "INF";
    else if (value == -std::numeric_limits<NumericType>::infinity()) return "-INF";
    else if (value != value) return "NaN";

    std::ostringstream oss;

    if (precision >= 0 && precision <= std::numeric_limits<NumericType>::max_digits10) oss << std::setprecision(precision);

    return (oss << value), oss.str();
}

template<typename NumericType>
inline std::wstring numeric_to_wstring(NumericType value, int precision = -1)
{
         if (value ==  std::numeric_limits<NumericType>::infinity()) return  L"INF";
    else if (value == -std::numeric_limits<NumericType>::infinity()) return L"-INF";
    else if (value != value) return L"NaN";

    std::wostringstream oss;

    if (precision >= 0 && precision <= std::numeric_limits<NumericType>::max_digits10) oss << std::setprecision(precision);

    return (oss << value), oss.str();
}

template<typename NumericType>
inline NumericType string_to_numeric(const std::string &value)
{
         if (value == "INF") return std::numeric_limits<NumericType>::infinity();
    else if (value == "-INF") return -std::numeric_limits<NumericType>::infinity();
    else if (value == "NaN") return std::numeric_limits<NumericType>::quiet_NaN();

    std::istringstream iss { value };

    NumericType result;

    return (iss >> result), result;
}

template<typename NumericType>
inline NumericType wstring_to_numeric(const std::wstring &value)
{
         if (value == L"INF") return std::numeric_limits<NumericType>::infinity();
    else if (value == L"-INF") return -std::numeric_limits<NumericType>::infinity();
    else if (value == L"NaN") return std::numeric_limits<NumericType>::quiet_NaN();

    std::wistringstream iss { value };

    NumericType result;

    return (iss >> result), result;
}

}
