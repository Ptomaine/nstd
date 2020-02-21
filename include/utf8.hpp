#pragma once

/*
MIT License
Copyright (c) 2019 Arlen Keshabyan (arlen.albert@gmail.com)
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


#include <stdexcept>
#include <iterator>

namespace nstd::utf8
{
namespace internal
{
constexpr const uint16_t lead_surrogate_min  = 0xd800u;
constexpr const uint16_t lead_surrogate_max  = 0xdbffu;
constexpr const uint16_t trail_surrogate_min = 0xdc00u;
constexpr const uint16_t trail_surrogate_max = 0xdfffu;
constexpr const uint16_t lead_offset         = lead_surrogate_min - (0x10000 >> 10);
constexpr const uint32_t surrogate_offset    = 0x10000u - (lead_surrogate_min << 10) - trail_surrogate_min;
constexpr const uint32_t code_point_max      = 0x0010ffffu;

template<typename octet_type>
inline char8_t mask8(octet_type oc)
{
    return static_cast<char8_t>(0xff & oc);
}

template<typename u16_type>
inline uint16_t mask16(u16_type oc)
{
    return static_cast<uint16_t>(0xffff & oc);
}

template<typename octet_type>
inline bool is_trail(octet_type oc)
{
    return ((utf8::internal::mask8(oc) >> 6) == 0x2);
}

template <typename u16>
inline bool is_lead_surrogate(u16 cp)
{
    return (cp >= lead_surrogate_min && cp <= lead_surrogate_max);
}

template <typename u16>
inline bool is_trail_surrogate(u16 cp)
{
    return (cp >= trail_surrogate_min && cp <= trail_surrogate_max);
}

template <typename u16>
inline bool is_surrogate(u16 cp)
{
    return (cp >= lead_surrogate_min && cp <= trail_surrogate_max);
}

template <typename u32>
inline bool is_code_point_valid(u32 cp)
{
    return (cp <= code_point_max && !utf8::internal::is_surrogate(cp));
}

template <typename octet_iterator>
inline auto sequence_length(octet_iterator lead_it) -> typename std::iterator_traits<octet_iterator>::difference_type
{
    char8_t lead = utf8::internal::mask8(*lead_it);

         if ( lead < 0x80) return 1;
    else if ((lead >> 5) == 0x6) return 2;
    else if ((lead >> 4) == 0xe) return 3;
    else if ((lead >> 3) == 0x1e) return 4;

    return 0;
}

template <typename octet_difference_type>
inline bool is_overlong_sequence(uint32_t cp, octet_difference_type length)
{
    if (cp < 0x80)
    {
        if (length != 1) return true;
    }
    else if (cp < 0x800)
    {
        if (length != 2) return true;
    }
    else if (cp < 0x10000)
    {
        if (length != 3) return true;
    }

    return false;
}

enum class utf_error
{
    utf8_ok,
    not_enough_room,
    invalid_lead,
    incomplete_sequence,
    overlong_sequence,
    invalid_code_point
};

template <typename octet_iterator>
utf_error increase_safely(octet_iterator& it, octet_iterator end)
{
    if (++it == end) return utf_error::not_enough_room;

    if (!utf8::internal::is_trail(*it)) return utf_error::incomplete_sequence;
    
    return utf_error::utf8_ok;
}

template <typename octet_iterator>
utf_error get_sequence_1(octet_iterator& it, octet_iterator end, uint32_t& code_point)
{
    if (it == end) return utf_error::not_enough_room;

    code_point = utf8::internal::mask8(*it);

    return utf_error::utf8_ok;
}

template <typename octet_iterator>
utf_error get_sequence_2(octet_iterator& it, octet_iterator end, uint32_t& code_point)
{
    if (it == end) return utf_error::not_enough_room;
    
    code_point = utf8::internal::mask8(*it);

    if (utf_error ret { increase_safely(it, end) }; ret != utf_error::utf8_ok) return ret;

    code_point = ((code_point << 6) & 0x7ff) + ((*it) & 0x3f);

    return utf_error::utf8_ok;
}

template <typename octet_iterator>
utf_error get_sequence_3(octet_iterator& it, octet_iterator end, uint32_t& code_point)
{
    if (it == end) return utf_error::not_enough_room;
        
    code_point = utf8::internal::mask8(*it);

    if (utf_error ret { increase_safely(it, end) }; ret != utf_error::utf8_ok) return ret;

    code_point = ((code_point << 12) & 0xffff) + ((utf8::internal::mask8(*it) << 6) & 0xfff);

    if (utf_error ret { increase_safely(it, end) }; ret != utf_error::utf8_ok) return ret;

    code_point += (*it) & 0x3f;

    return utf_error::utf8_ok;
}

template <typename octet_iterator>
utf_error get_sequence_4(octet_iterator& it, octet_iterator end, uint32_t& code_point)
{
    if (it == end) return utf_error::not_enough_room;

    code_point = utf8::internal::mask8(*it);

    if (utf_error ret { increase_safely(it, end) }; ret != utf_error::utf8_ok) return ret;

    code_point = ((code_point << 18) & 0x1fffff) + ((utf8::internal::mask8(*it) << 12) & 0x3ffff);

    if (utf_error ret { increase_safely(it, end) }; ret != utf_error::utf8_ok) return ret;

    code_point += (utf8::internal::mask8(*it) << 6) & 0xfff;

    if (utf_error ret { increase_safely(it, end) }; ret != utf_error::utf8_ok) return ret;

    code_point += (*it) & 0x3f;

    return utf_error::utf8_ok;
}

template <typename octet_iterator>
utf_error validate_next(octet_iterator& it, octet_iterator end, uint32_t& code_point)
{
    octet_iterator original_it = it;

    uint32_t cp = 0;

    using octet_difference_type = typename std::iterator_traits<octet_iterator>::difference_type;

    const octet_difference_type length = utf8::internal::sequence_length(it);

    utf_error err = utf_error::utf8_ok;

    switch (length)
    {
        case 0: return utf_error::invalid_lead;
        case 1:
            err = utf8::internal::get_sequence_1(it, end, cp);
            break;
        case 2:
            err = utf8::internal::get_sequence_2(it, end, cp);
            break;
        case 3:
            err = utf8::internal::get_sequence_3(it, end, cp);
            break;
        case 4:
            err = utf8::internal::get_sequence_4(it, end, cp);
            break;
    }

    if (err == utf_error::utf8_ok)
    {
        if (utf8::internal::is_code_point_valid(cp))
        {
            if (!utf8::internal::is_overlong_sequence(cp, length))
            {
                code_point = cp;
                ++it;

                return utf_error::utf8_ok;
            }
            else err = utf_error::overlong_sequence;
        }
        else err = utf_error::invalid_code_point;
    }

    it = original_it;

    return err;
}

template <typename octet_iterator>
inline utf_error validate_next(octet_iterator& it, octet_iterator end)
{
    uint32_t ignored;
    return utf8::internal::validate_next(it, end, ignored);
}

}

template <typename octet_iterator>
octet_iterator find_invalid(octet_iterator start, octet_iterator end)
{
    octet_iterator result = start;

    while (result != end)
    {
        utf8::internal::utf_error err_code = utf8::internal::validate_next(result, end);

        if (err_code != internal::utf_error::utf8_ok) return result;
    }

    return result;
}

template <typename octet_iterator>
inline bool is_valid(octet_iterator start, octet_iterator end)
{
    return (utf8::find_invalid(start, end) == end);
}

constexpr static inline const char8_t bom[] = { 0xef, 0xbb, 0xbf };

template <typename octet_iterator>
inline bool starts_with_bom(octet_iterator it, octet_iterator end)
{
    return (
        ((it != end) && (utf8::internal::mask8(*it++)) == bom[0]) &&
        ((it != end) && (utf8::internal::mask8(*it++)) == bom[1]) &&
        ((it != end) && (utf8::internal::mask8(*it))   == bom[2])
       );
}

class exception : public ::std::exception { };

class invalid_code_point : public exception
{
    uint32_t cp;
public:
    invalid_code_point(uint32_t cp) : cp(cp) {}
    virtual const char* what() const throw() { return "Invalid code point"; }
    uint32_t code_point() const {return cp;}
};

class invalid_utf8 : public exception
{
    char8_t u8;
public:
    invalid_utf8 (char8_t u) : u8(u) {}
    virtual const char* what() const throw() { return "Invalid UTF-8"; }
    char8_t utf8_octet() const {return u8;}
};

class invalid_utf16 : public exception
{
    uint16_t u16;
public:
    invalid_utf16 (uint16_t u) : u16(u) {}
    virtual const char* what() const throw() { return "Invalid UTF-16"; }
    uint16_t utf16_word() const {return u16;}
};

class not_enough_room : public exception
{
public:
    virtual const char* what() const throw() { return "Not enough space"; }
};

template <typename octet_iterator>
octet_iterator append(uint32_t cp, octet_iterator result)
{
    if (!utf8::internal::is_code_point_valid(cp)) throw invalid_code_point(cp);

    if (cp < 0x80) *(result++) = static_cast<char8_t>(cp);
    else if (cp < 0x800)
    {
        *(result++) = static_cast<char8_t>((cp >> 6)            | 0xc0);
        *(result++) = static_cast<char8_t>((cp & 0x3f)          | 0x80);
    }
    else if (cp < 0x10000)
    {
        *(result++) = static_cast<char8_t>((cp >> 12)           | 0xe0);
        *(result++) = static_cast<char8_t>(((cp >> 6) & 0x3f)   | 0x80);
        *(result++) = static_cast<char8_t>((cp & 0x3f)          | 0x80);
    }
    else
    {
        *(result++) = static_cast<char8_t>((cp >> 18)           | 0xf0);
        *(result++) = static_cast<char8_t>(((cp >> 12) & 0x3f)  | 0x80);
        *(result++) = static_cast<char8_t>(((cp >> 6) & 0x3f)   | 0x80);
        *(result++) = static_cast<char8_t>((cp & 0x3f)          | 0x80);
    }
    return result;
}

template <typename octet_iterator, typename output_iterator>
output_iterator replace_invalid(octet_iterator start, octet_iterator end, output_iterator out, uint32_t replacement)
{
    while (start != end)
    {
        octet_iterator sequence_start { start };
        internal::utf_error err_code { utf8::internal::validate_next(start, end) };

        switch (err_code)
        {
            case internal::utf_error::utf8_ok:
                for (octet_iterator it = sequence_start; it != start; ++it) *out++ = *it;
                break;
            case internal::utf_error::not_enough_room:
                throw not_enough_room();
            case internal::utf_error::invalid_lead:
                out = utf8::append (replacement, out);
                ++start;
                break;
            case internal::utf_error::incomplete_sequence:
            case internal::utf_error::overlong_sequence:
            case internal::utf_error::invalid_code_point:
                out = utf8::append (replacement, out);
                ++start;
                while (start != end && utf8::internal::is_trail(*start)) ++start;
                break;
        }
    }

    return out;
}

template <typename octet_iterator, typename output_iterator>
inline output_iterator replace_invalid(octet_iterator start, octet_iterator end, output_iterator out)
{
    static const uint32_t replacement_marker = utf8::internal::mask16(0xfffd);
    
    return utf8::replace_invalid(start, end, out, replacement_marker);
}

template <typename octet_iterator>
uint32_t next(octet_iterator& it, octet_iterator end)
{
    uint32_t cp { 0 };
    internal::utf_error err_code { utf8::internal::validate_next(it, end, cp) };
    
    switch (err_code)
    {
        case internal::utf_error::utf8_ok : break;
        case internal::utf_error::not_enough_room : throw not_enough_room();
        case internal::utf_error::invalid_lead :
        case internal::utf_error::incomplete_sequence :
        case internal::utf_error::overlong_sequence : throw invalid_utf8(*it);
        case internal::utf_error::invalid_code_point : throw invalid_code_point(cp);
    }

    return cp;
}

template <typename octet_iterator>
uint32_t peek_next(octet_iterator it, octet_iterator end)
{
    return utf8::next(it, end);
}

template <typename octet_iterator>
uint32_t prior(octet_iterator& it, octet_iterator start)
{
    if (it == start)
        throw not_enough_room();

    octet_iterator end = it;

    while (utf8::internal::is_trail(*(--it)))
        if (it == start)
            throw invalid_utf8(*it);

    return utf8::peek_next(it, end);
}

template <typename octet_iterator, typename distance_type>
void advance(octet_iterator& it, distance_type n, octet_iterator end)
{
    for (distance_type i = 0; i < n; ++i) utf8::next(it, end);
}

template <typename octet_iterator>
auto distance(octet_iterator first, octet_iterator last) -> typename std::iterator_traits<octet_iterator>::difference_type
{
    typename std::iterator_traits<octet_iterator>::difference_type dist { 0 };

    for (; first < last; ++dist) utf8::next(first, last);

    return dist;
}

template <typename u16bit_iterator, typename octet_iterator>
octet_iterator utf16to8(u16bit_iterator start, u16bit_iterator end, octet_iterator result)
{
    while (start != end)
    {
        uint32_t cp { utf8::internal::mask16(*start++) };

        if (utf8::internal::is_lead_surrogate(cp))
        {
            if (start != end)
            {
                uint32_t trail_surrogate { utf8::internal::mask16(*start++) };

                if (utf8::internal::is_trail_surrogate(trail_surrogate)) cp = (cp << 10) + trail_surrogate + internal::surrogate_offset;
                else throw invalid_utf16(static_cast<uint16_t>(trail_surrogate));
            } else throw invalid_utf16(static_cast<uint16_t>(cp));

        }
        else if (utf8::internal::is_trail_surrogate(cp)) throw invalid_utf16(static_cast<uint16_t>(cp));

        result = utf8::append(cp, result);
    }

    return result;
}

template <typename u16bit_iterator, typename octet_iterator>
u16bit_iterator utf8to16(octet_iterator start, octet_iterator end, u16bit_iterator result)
{
    while (start != end)
    {
        uint32_t cp { utf8::next(start, end) };

        if (cp > 0xffff)
        {
            *result++ = static_cast<uint16_t>((cp >> 10)   + internal::lead_offset);
            *result++ = static_cast<uint16_t>((cp & 0x3ff) + internal::trail_surrogate_min);
        } else *result++ = static_cast<uint16_t>(cp);
    }

    return result;
}

template <typename octet_iterator, typename u32bit_iterator>
octet_iterator utf32to8(u32bit_iterator start, u32bit_iterator end, octet_iterator result)
{
    while (start != end) result = utf8::append(*(start++), result);

    return result;
}

template <typename octet_iterator, typename u32bit_iterator>
u32bit_iterator utf8to32(octet_iterator start, octet_iterator end, u32bit_iterator result)
{
    while (start != end) (*result++) = utf8::next(start, end);

    return result;
}

template <typename octet_iterator>
class iterator : public std::iterator <std::bidirectional_iterator_tag, uint32_t>
{
private:
    octet_iterator it;
    octet_iterator range_start;
    octet_iterator range_end;

public:
    iterator () { }
    explicit iterator(const octet_iterator& octet_it, const octet_iterator& range_start, const octet_iterator& range_end) :
            it(octet_it), range_start(range_start), range_end(range_end)
    {
        if (it < range_start || it > range_end) throw std::out_of_range("Invalid utf-8 iterator position");
    }

    octet_iterator base() const { return it; }

    uint32_t operator *() const
    {
        octet_iterator temp = it;
        return utf8::next(temp, range_end);
    }

    bool operator ==(const iterator& rhs) const
    {
        if (range_start != rhs.range_start || range_end != rhs.range_end) throw std::logic_error("Comparing utf-8 iterators defined with different ranges");

        return (it == rhs.it);
    }

    bool operator !=(const iterator& rhs) const
    {
        return !(operator == (rhs));
    }

    iterator& operator ++()
    {
        utf8::next(it, range_end);
        return *this;
    }

    iterator operator ++(int)
    {
        iterator temp = *this;

        utf8::next(it, range_end);

        return temp;
    }

    iterator& operator --()
    {
        utf8::prior(it, range_start);

        return *this;
    }

    iterator operator --(int)
    {
        iterator temp = *this;

        utf8::prior(it, range_start);

        return temp;
    }
};

namespace unchecked
{

template <typename octet_iterator>
octet_iterator append(uint32_t cp, octet_iterator result)
{
    if (cp < 0x80) *(result++) = static_cast<char8_t>(cp);  
    else if (cp < 0x800)
    {
        *(result++) = static_cast<char8_t>((cp >> 6)          | 0xc0);
        *(result++) = static_cast<char8_t>((cp & 0x3f)        | 0x80);
    }
    else if (cp < 0x10000)
    {
        *(result++) = static_cast<char8_t>((cp >> 12)         | 0xe0);
        *(result++) = static_cast<char8_t>(((cp >> 6) & 0x3f) | 0x80);
        *(result++) = static_cast<char8_t>((cp & 0x3f)        | 0x80);
    }
    else
    {                                // four octets
        *(result++) = static_cast<char8_t>((cp >> 18)         | 0xf0);
        *(result++) = static_cast<char8_t>(((cp >> 12) & 0x3f)| 0x80);
        *(result++) = static_cast<char8_t>(((cp >> 6) & 0x3f) | 0x80);
        *(result++) = static_cast<char8_t>((cp & 0x3f)        | 0x80);
    }

    return result;
}

template <typename octet_iterator>
uint32_t next(octet_iterator& it)
{
    uint32_t cp { utf8::internal::mask8(*it) };
    typename std::iterator_traits<octet_iterator>::difference_type length { utf8::internal::sequence_length(it) };

    switch (length)
    {
        case 1: break;
        case 2:
            it++;
            cp = ((cp << 6) & 0x7ff) + ((*it) & 0x3f);
            break;
        case 3:
            ++it; 
            cp = ((cp << 12) & 0xffff) + ((utf8::internal::mask8(*it) << 6) & 0xfff);
            ++it;
            cp += (*it) & 0x3f;
            break;
        case 4:
            ++it;
            cp = ((cp << 18) & 0x1fffff) + ((utf8::internal::mask8(*it) << 12) & 0x3ffff);                
            ++it;
            cp += (utf8::internal::mask8(*it) << 6) & 0xfff;
            ++it;
            cp += (*it) & 0x3f; 
            break;
    }

    ++it;

    return cp;        
}

template <typename octet_iterator>
uint32_t peek_next(octet_iterator it)
{
    return utf8::unchecked::next(it);
}

template <typename octet_iterator>
uint32_t prior(octet_iterator& it)
{
    while (utf8::internal::is_trail(*(--it))) ;

    octet_iterator temp = it;

    return utf8::unchecked::next(temp);
}

template <typename octet_iterator, typename distance_type>
void advance(octet_iterator& it, distance_type n)
{
    for (distance_type i = 0; i < n; ++i) utf8::unchecked::next(it);
}

template <typename octet_iterator>
auto distance(octet_iterator first, octet_iterator last) -> typename std::iterator_traits<octet_iterator>::difference_type
{
    typename std::iterator_traits<octet_iterator>::difference_type dist { 0 };

    for (; first < last; ++dist) utf8::unchecked::next(first);

    return dist;
}

template <typename u16bit_iterator, typename octet_iterator>
octet_iterator utf16to8(u16bit_iterator start, u16bit_iterator end, octet_iterator result)
{       
    while (start != end)
    {
        uint32_t cp = utf8::internal::mask16(*start++);

        if (utf8::internal::is_lead_surrogate(cp))
        {
            uint32_t trail_surrogate = utf8::internal::mask16(*start++);
            cp = (cp << 10) + trail_surrogate + internal::surrogate_offset;
        }

        result = utf8::unchecked::append(cp, result);
    }

    return result;         
}

template <typename u16bit_iterator, typename octet_iterator>
u16bit_iterator utf8to16(octet_iterator start, octet_iterator end, u16bit_iterator result)
{
    while (start < end)
    {
        uint32_t cp = utf8::unchecked::next(start);

        if (cp > 0xffff)
        {
            *result++ = static_cast<uint16_t>((cp >> 10)   + internal::lead_offset);
            *result++ = static_cast<uint16_t>((cp & 0x3ff) + internal::trail_surrogate_min);
        }
        else *result++ = static_cast<uint16_t>(cp);
    }

    return result;
}

template <typename octet_iterator, typename u32bit_iterator>
octet_iterator utf32to8(u32bit_iterator start, u32bit_iterator end, octet_iterator result)
{
    while (start != end) result = utf8::unchecked::append(*(start++), result);

    return result;
}

template <typename octet_iterator, typename u32bit_iterator>
u32bit_iterator utf8to32(octet_iterator start, octet_iterator end, u32bit_iterator result)
{
    while (start < end) (*result++) = utf8::unchecked::next(start);

    return result;
}

template <typename octet_iterator>
class iterator : public std::iterator <std::bidirectional_iterator_tag, uint32_t>
{
private:
    octet_iterator it;

public:
    iterator() { }
    explicit iterator (const octet_iterator& octet_it): it(octet_it) { }

    octet_iterator base() const { return it; }

    uint32_t operator *() const
    {
        octet_iterator temp = it;
        return utf8::unchecked::next(temp);
    }

    bool operator ==(const iterator& rhs) const 
    { 
        return (it == rhs.it);
    }

    bool operator !=(const iterator& rhs) const
    {
        return !(operator == (rhs));
    }

    iterator& operator ++() 
    {
        ::std::advance(it, utf8::internal::sequence_length(it));
        return *this;
    }

    iterator operator ++(int)
    {
        iterator temp = *this;
        ::std::advance(it, utf8::internal::sequence_length(it));
        return temp;
    }  

    iterator& operator --()
    {
        utf8::unchecked::prior(it);
        return *this;
    }

    iterator operator --(int)
    {
        iterator temp = *this;
        utf8::unchecked::prior(it);
        return temp;
    }
};

}

}
