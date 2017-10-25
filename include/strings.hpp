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

#include <codecvt>
#include <iomanip>
#include <limits>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>

namespace nstd::str
{

namespace
{

template <typename StringViewType>
inline auto trim_left_impl(StringViewType sv, const StringViewType& chars_to_remove)
{
    sv.remove_prefix(std::min(sv.find_first_not_of(chars_to_remove), std::size(sv)));

    return sv;
}

template <typename StringViewType>
inline auto trim_right_impl(StringViewType sv, const StringViewType& chars_to_remove)
{
    sv.remove_suffix(sv.size() - (sv.find_last_not_of(chars_to_remove) + 1));

    return sv;
}

template <typename StringViewType>
inline auto trim_impl(StringViewType sv, const StringViewType& chars_to_remove)
{
    return trim_right_impl(trim_left_impl(sv, chars_to_remove), chars_to_remove);
}

}

inline std::string from_utf16_to_utf8(const std::u16string &s)
{
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> conv;

    return conv.to_bytes(s);
}

inline std::string from_utf32_to_utf8(const std::u32string &s)
{
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;

    return conv.to_bytes(s);
}

inline std::u16string from_utf8_to_utf16(const std::string &s)
{
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> conv;

    return conv.from_bytes(s);
}

inline std::u16string from_utf32_to_utf16(const std::u32string &s)
{
    std::wstring_convert<std::codecvt_utf16<char32_t>, char32_t> conv;
    std::string bytes = conv.to_bytes(s);

    return std::u16string(reinterpret_cast<const char16_t*>(bytes.c_str()), bytes.length() / sizeof(char16_t));
}

inline std::u32string from_utf8_to_utf32(const std::string &s)
{
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;

    return conv.from_bytes(s);
}

inline std::u32string from_utf16_to_utf32(const std::u16string &s)
{
    const char16_t *pData = std::data(s);
    std::wstring_convert<std::codecvt_utf16<char32_t>, char32_t> conv;

    return conv.from_bytes(reinterpret_cast<const char*>(pData), reinterpret_cast<const char*>(pData + s.length()));
}

inline std::wstring from_utf16_to_wchar(const std::u16string &s)
{
    const char16_t *pData = std::data(s);

#if defined(_LITTLE_ENDIAN) \
    || ( defined(BYTE_ORDER) && defined(LITTLE_ENDIAN) && BYTE_ORDER == LITTLE_ENDIAN ) \
    || ( defined(_BYTE_ORDER) && defined(_LITTLE_ENDIAN) && _BYTE_ORDER == _LITTLE_ENDIAN ) \
    || ( defined(__BYTE_ORDER) && defined(__LITTLE_ENDIAN) && __BYTE_ORDER == __LITTLE_ENDIAN ) \
    || defined(__i386__) || defined(__alpha__) \
    || defined(__ia64) || defined(__ia64__) \
    || defined(_M_IX86) || defined(_M_IA64) \
    || defined(_M_ALPHA) || defined(__amd64) \
    || defined(__amd64__) || defined(_M_AMD64) \
    || defined(__x86_64) || defined(__x86_64__) \
    || defined(_M_X64)
    std::wstring_convert<std::codecvt_utf16<wchar_t, 0x10ffff, std::little_endian>, wchar_t> conv;
    return conv.from_bytes(reinterpret_cast<const char*>(pData), reinterpret_cast<const char*>(pData + s.length()));
#else
    std::wstring_convert<std::codecvt_utf16<wchar_t>, wchar_t> conv;
    return conv.from_bytes(reinterpret_cast<const char*>(pData), reinterpret_cast<const char*>(pData + s.length()));
#endif
}

inline std::wstring from_utf8_to_wchar(const std::string &s)
{
    const char *pData = std::data(s);
    std::wstring_convert<std::codecvt_utf8<wchar_t, 0x10ffff, std::little_endian>, wchar_t> conv;

    return conv.from_bytes(reinterpret_cast<const char*>(pData), reinterpret_cast<const char*>(pData + s.length()));
}

constexpr const char     *const whitespace_chars { " \t\n\v\f\r" };
constexpr const wchar_t  *const wwhitespace_chars { L" \t\n\v\f\r" };
constexpr const char16_t *const u16whitespace_chars { u" \t\n\v\f\r" };
constexpr const char32_t *const u32whitespace_chars { U" \t\n\v\f\r" };
constexpr const char *const boolalpha[] = { "false", "true" };
constexpr const wchar_t *const wboolalpha[] = { L"false", L"true" };
constexpr const char16_t *const u16boolalpha[] = { u"false", u"true" };
constexpr const char32_t *const u32boolalpha[] = { U"false", U"true" };

inline std::string_view trim_left(std::string_view sv, const std::string_view &chars_to_remove = whitespace_chars) { return trim_left_impl(sv, chars_to_remove); }
inline std::wstring_view trim_left(std::wstring_view sv, const std::wstring_view &chars_to_remove = wwhitespace_chars) { return trim_left_impl(sv, chars_to_remove); }
inline std::u16string_view trim_left(std::u16string_view sv, const std::u16string_view &chars_to_remove = u16whitespace_chars) { return trim_left_impl(sv, chars_to_remove); }
inline std::u32string_view trim_left(std::u32string_view sv, const std::u32string_view &chars_to_remove = u32whitespace_chars) { return trim_left_impl(sv, chars_to_remove); }

inline std::string_view trim_right(std::string_view sv, const std::string_view &chars_to_remove = whitespace_chars) { return trim_right_impl(sv, chars_to_remove); }
inline std::wstring_view trim_right(std::wstring_view sv, const std::wstring_view &chars_to_remove = wwhitespace_chars) { return trim_right_impl(sv, chars_to_remove); }
inline std::u16string_view trim_right(std::u16string_view sv, const std::u16string_view &chars_to_remove = u16whitespace_chars) { return trim_right_impl(sv, chars_to_remove); }
inline std::u32string_view trim_right(std::u32string_view sv, const std::u32string_view &chars_to_remove = u32whitespace_chars) { return trim_right_impl(sv, chars_to_remove); }

inline std::string_view trim(std::string_view sv, const std::string_view &chars_to_remove = whitespace_chars) { return trim_impl(sv, chars_to_remove); }
inline std::wstring_view trim(std::wstring_view sv, const std::wstring_view &chars_to_remove = wwhitespace_chars) { return trim_impl(sv, chars_to_remove); }
inline std::u16string_view trim(std::u16string_view sv, const std::u16string_view &chars_to_remove = u16whitespace_chars) { return trim_impl(sv, chars_to_remove); }
inline std::u32string_view trim(std::u32string_view sv, const std::u32string_view &chars_to_remove = u32whitespace_chars) { return trim_impl(sv, chars_to_remove); }

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

inline static const std::regex is_empty_or_ws_regex { R"(^\s*$)", std::regex_constants::ECMAScript | std::regex_constants::optimize };
inline static const std::wregex is_empty_or_ws_wregex { LR"(^\s*$)", std::regex_constants::ECMAScript | std::regex_constants::optimize };

inline bool is_empty_or_ws(const std::string &str)
{
    return std::regex_match(str, is_empty_or_ws_regex);
}

inline bool is_empty_or_ws(const std::wstring &str)
{
    return std::regex_match(str, is_empty_or_ws_wregex);
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
         if (value ==  std::numeric_limits<NumericType>::infinity()) return  "INF";
    else if (value == -std::numeric_limits<NumericType>::infinity()) return "-INF";
    else if (value != value) return "NaN";

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
