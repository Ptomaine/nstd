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
#include <cmath>
#include <condition_variable>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <fstream>
#include <future>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>

namespace nstd::utilities
{

class cancellable_sleep
{
public:

    template<typename Duration>
    bool wait_for(const Duration &duration)
    {
        std::unique_lock<std::mutex> lock { _m };
            
        return !_cv.wait_for(lock, duration, [this]{ return _cancelled.load(); });
    }

    void cancel_wait()
    {
        _cancelled = true;
            
        _cv.notify_all();
    }

    void reset()
    {
        _cancelled = false;
    }

    ~cancellable_sleep()
    {
        cancel_wait();
    }

private:

    std::condition_variable _cv {};
    std::mutex _m {};
    std::atomic_bool _cancelled { false };
};

class at_scope_exit
{
public:

    at_scope_exit(std::function<void(void)> &&functor) : _functor { std::forward<std::function<void(void)>>(functor) } {}

    ~at_scope_exit()
    {
        if (_functor) _functor();
    }

    at_scope_exit() = delete;
    at_scope_exit(const at_scope_exit&) = delete;
    at_scope_exit(at_scope_exit&&) = default;
    at_scope_exit &operator =(const at_scope_exit&) = delete;
    at_scope_exit &operator =(at_scope_exit&&) = default;

    void reset(std::function<void(void)> &&functor = nullptr) { _functor = std::forward<std::function<void(void)>>(functor); }

private:
    std::function<void(void)> _functor;
};

// Hash function www.cs.ubc.ca/~rbridson/docs/schechter-sca08-turbulence.pdf
uint32_t turbulence_hash(uint32_t state)
{
    state ^= 2747636419u;
    state *= 2654435769u;
    state ^= state >> 16;
    state *= 2654435769u;
    state ^= state >> 16;
    state *= 2654435769u;

    return state;
}

struct case_insensitive_hash
{
    template<typename StringType>
    size_t operator()(const StringType& key) const
    {
        int ch { 0 };
        std::size_t h { 0 };
        std::hash<int> hash_;

        for(auto c : key)
        {
            if constexpr (sizeof(typename StringType::value_type) == sizeof(char)) ch = std::tolower(c);
            else ch = std::towlower(c);

            h ^= hash_(ch) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }

        return h;
    }
};

struct case_insensitive_equal
{
    template<typename StringType>
    bool operator()(const StringType& left, const StringType& right) const
    {
        return std::size(left) == std::size(right) &&
               std::equal(std::begin(left), std::end(left), std::begin(right),
                            [](auto ca, auto cb)
                            {
                                if constexpr (sizeof(typename StringType::value_type) == sizeof(char)) return std::tolower(ca) == std::tolower(cb);
                                else return std::towlower(ca) == std::towlower(cb);
                            });
    }
};

class reinterpreted_buffer
{
public:
    reinterpreted_buffer(void *data, uint64_t size) :
        _data{ reinterpret_cast<uint8_t*>(data) }, _next_data{ _data }, _size{ size }, _size_left{ _size }
    {
    }

    template<typename T>
    const T *const get_next_as()
    {
        constexpr const uint64_t type_size{ sizeof(T) };

        if (_size_left == 0 || _size_left < type_size) return nullptr;

        void *current_data { _next_data };

        _next_data += type_size;
        _size_left -= type_size;

        return reinterpret_cast<const T*const>(current_data);
    }

    template<typename T>
    const T *const get_current_as()
    {
        constexpr const uint64_t type_size{ sizeof(T) };

        if (_size_left == 0 || _size_left < type_size) return nullptr;

        return reinterpret_cast<const T*const>(_next_data);
    }

    template<typename T>
    bool is_available_as() const
    {
        return _size_left >= sizeof(T);
    }

    uint64_t get_size_left() const
    {
        return _size_left;
    }

    void *get_current_ptr() const
    {
        return _next_data;
    }

    void reset()
    {
        _next_data = _data;
        _size_left = _size;
    }

private:
    uint8_t *_data, *_next_data;
    uint64_t _size, _size_left;
};

template<typename CharT = char>
auto read_file_content(const std::filesystem::path &filepath)
{
    std::vector<CharT> contents;
    std::ifstream in(filepath, std::ios::in | std::ios::binary);

    if (in)
    {
        in.seekg(0, std::ios::end);

        contents.resize(in.tellg());

        in.seekg(0, std::ios::beg);
        in.read(&contents[0], contents.size());
        in.close();
    }

    return contents;
};

template<class Iterator, class Compare = std::less<>>
void parallel_sort(Iterator begin, Iterator end, size_t min_sortable_length, const Compare &cp = Compare())
{
    auto size { std::distance(begin, end) };

    if (size <= min_sortable_length) std::sort(begin, end, cp);
    else
    {
        Iterator mid { begin };

        std::advance(mid, size / 2);

        auto future { std::async(parallel_sort<Iterator, Compare>, begin, mid, min_sortable_length, cp) };

        parallel_sort(mid, end, min_sortable_length, cp);

        future.wait();

        std::inplace_merge(begin, mid, end, cp);
    }
}

template<typename Iterator, typename Functor>
static void parallel_for_each(Iterator begin, Iterator end, Functor func)
{
    const static unsigned nb_threads_hint { std::thread::hardware_concurrency() };
    const static unsigned nb_threads { (nb_threads_hint == 0u ? 8u : nb_threads_hint) };

    if (begin == end) return;

    auto size { std::distance(begin, end) };
    auto slice { static_cast<decltype(size)>(double(size) / nb_threads + .5) };

    if (slice < 2) { std::for_each(begin, end, func); return; }

    std::vector<std::thread> thread_pool; thread_pool.reserve(nb_threads);
    auto it1 { begin }, it2 { begin };

    while (it1 != end)
    {
        if (std::distance(it2, end) > slice) std::advance(it2, slice); else it2 = end;

        thread_pool.emplace_back(std::for_each<Iterator, Functor>, it1, it2, func), it1 = it2;
    }

    for (std::thread &t : thread_pool) if (t.joinable()) t.join();
}

template<class Iterator>
void reverse_inplace(Iterator begin, Iterator end)
{
    end = std::prev(end);

    while (std::distance(begin, end) > 0)
    {
        std::iter_swap(begin, end);

        begin = std::next(begin);
        end = std::prev(end);
    }
}

template<class Iterator>
void rotate_inplace(Iterator begin, Iterator end, int shift_amount)
{
    auto size { static_cast<int>(std::distance(begin, end)) };

    if (!size) return;

    auto index { (shift_amount % size) + ((shift_amount < 0) ? size : 0) };

    Iterator mid { begin };

    std::advance(mid, index);

    reverse_inplace(begin, mid);
    reverse_inplace(mid, end);
    reverse_inplace(begin, end);
}

template<class Container>
Container rotate(const Container &data, int shift_amount)
{
    Container result;
    auto size { static_cast<int>(std::size(data)) };

    if (!size) return result;

    auto index { (shift_amount % size) + ((shift_amount < 0) ? size : 0) };
    auto item { std::back_inserter(result) };
    auto length { size };

    while (--size >= 0)
    {
        auto current { std::begin(data) };

        std::advance(current, index++ % length);

        item = *current;
    }

    return result;
}

template<typename Iterator, typename Container>
void permute(Container &result, const Iterator &begin, Iterator from, const Iterator &end)
{
    if (from == std::prev(end))
    {
        typename Container::value_type next_record;

        std::copy(begin, end, std::back_inserter(next_record));

        std::back_inserter(result) = std::move(next_record);
    }
    else
    {
        for (auto it{ from }; it != end; it = std::next(it))
        {
            std::iter_swap(from, it);

            permute(result, begin, std::next(from), end);

            std::iter_swap(from, it);
        }
    }
}

template<typename Iterator, typename Container>
void permute(Container &result, const Iterator &begin, const Iterator &end)
{
    permute(result, begin, begin, end);
}

namespace fibonacci
{

uint64_t recursive_fibonacci(const uint64_t n)
{
    return n < 2 ? n : recursive_fibonacci(n - 1) + recursive_fibonacci(n - 2);
}

uint64_t non_recursive_fibonacci(const uint64_t n)
{
    if (n < 2) return n;
    if (n == 2) return 1;

    uint64_t prev_prev { 1 }, prev { 1 }, current { 0 };

    for (uint64_t idx { 3 }; idx <= n; ++idx)
    {
        current = prev_prev + prev;
        prev_prev = prev;
        prev = current;
    }

    return current;
}

class optimized_fibonacci
{
public:
    optimized_fibonacci() = default;
    uint64_t operator ()(const uint64_t n)
    {
        if (_already_calculated.find(n) == std::end(_already_calculated))
        {
            for (auto idx { _high_water_mark + 1 }; idx <= n; ++idx)
                _already_calculated[idx] = _already_calculated[idx - 1] + _already_calculated[idx - 2];

            _high_water_mark = n;
        }

        return _already_calculated[n];
    }

private:
    inline static std::unordered_map<uint64_t, uint64_t> _already_calculated { {0, 0}, {1, 1}, {2, 1} };
    inline static uint64_t _high_water_mark { 2 };
};

template<uint64_t N>
struct compile_time_fibonacci
{
    enum : uint64_t
    {
        value = compile_time_fibonacci<N - 1>::value + compile_time_fibonacci<N - 2>::value
    };
};

template<>
struct compile_time_fibonacci<0>
{
    enum : uint64_t
    {
        value = 0
    };
};

template<>
struct compile_time_fibonacci<1>
{
    enum : uint64_t
    {
        value = 1
    };
};

template<>
struct compile_time_fibonacci<2>
{
    enum : uint64_t
    {
        value = 1
    };
};

constexpr uint64_t binets_fibonacci(const uint64_t n)
{
    constexpr auto sqrt_5 { std::sqrt(5) };

    if (n < 2) return n;

    return static_cast<uint64_t>((std::pow(1 + sqrt_5, n) - std::pow(1 - sqrt_5, n)) / (std::pow(2, n) * sqrt_5));
}

namespace advanced
{

template <typename N>
struct multiply_2x2
{
    std::array<N, 4> operator() (const std::array<N, 4> &x, const std::array<N, 4> &y)
    {
        return { x[0] * y[0] + x[1] * y[2], x[0] * y[1] + x[1] * y[3],
                 x[2] * y[0] + x[3] * y[2], x[2] * y[1] + x[3] * y[3] };
    }
};

template <typename N>
std::array<N, 4> identity_element(const multiply_2x2<N>&) { return { N(1), N(0), N(0), N(1) }; }

template<typename T, typename N, typename O>
T power(T x, N n, O op)
{
    if (n == 0) return identity_element(op);

    while ((n & 1) == 0)
    {
        n >>= 1;
        x = op(x, x);
    }

    T result { x };

    n >>= 1;

    while (n != 0)
    {
        x = op(x, x);

        if ((n & 1) != 0) result = op(result, x);

        n >>= 1;
    }

    return result;
}

template <typename R, typename N>
R fibonacci(N n)
{
    if (n == 0) return R(0);

    return power(std::array<R, 4> {1, 1, 1, 0}, N(n - 1), multiply_2x2<R>())[0];
}

}

}

namespace net
{
using namespace std::string_view_literals;
using nstd::utilities::case_insensitive_hash;
using nstd::utilities::case_insensitive_equal;

static const std::unordered_map<std::string_view, uint8_t, case_insensitive_hash, case_insensitive_equal> entities_to_char
{
    { "&quot;"sv, 34 },
    { "&amp;"sv, 38 },
    { "&lt;"sv, 60 },
    { "&gt;"sv, 62 },
    { "&nbsp;"sv, 32 }, // Using the space char (32) instead of the non-breaking space (160).
                        // In case you need it as (160) you may set it manually in your application like this:
                        //    nstd::utilities::net::entities_to_char["&nbsp;"] = 160;
    { "&iexcl;"sv, 161 },
    { "&cent;"sv, 162 },
    { "&pound;"sv, 163 },
    { "&curren;"sv, 164 },
    { "&yen;"sv, 165 },
    { "&brvbar;"sv, 166 },
    { "&sect;"sv, 167 },
    { "&uml;"sv, 168 },
    { "&copy;"sv, 169 },
    { "&ordf;"sv, 170 },
    { "&laquo;"sv, 171 },
    { "&not;"sv, 172 },
    { "&shy;"sv, 173 },
    { "&reg;"sv, 174 },
    { "&macr;"sv, 175 },
    { "&deg;"sv, 176 },
    { "&plusmn;"sv, 177 },
    { "&sup2;"sv, 178 },
    { "&sup3;"sv, 179 },
    { "&acute;"sv, 180 },
    { "&micro;"sv, 181 },
    { "&para;"sv, 182 },
    { "&middot;"sv, 183 },
    { "&cedil;"sv, 184 },
    { "&sup1;"sv, 185 },
    { "&ordm;"sv, 186 },
    { "&raquo;"sv, 187 },
    { "&frac14;"sv, 188 },
    { "&frac12;"sv, 189 },
    { "&frac34;"sv, 190 },
    { "&iquest;"sv, 191 },
    { "&Agrave;"sv, 192 },
    { "&Aacute;"sv, 193 },
    { "&Acirc;"sv, 194 },
    { "&Atilde;"sv, 195 },
    { "&Auml;"sv, 196 },
    { "&ring;"sv, 197 },
    { "&AElig;"sv, 198 },
    { "&Ccedil;"sv, 199 },
    { "&Egrave;"sv, 200 },
    { "&Eacute;"sv, 201 },
    { "&Ecirc;"sv, 202 },
    { "&Euml;"sv, 203 },
    { "&Igrave;"sv, 204 },
    { "&Iacute;"sv, 205 },
    { "&Icirc;"sv, 206 },
    { "&Iuml;"sv, 207 },
    { "&ETH;"sv, 208 },
    { "&Ntilde;"sv, 209 },
    { "&Ograve;"sv, 210 },
    { "&Oacute;"sv, 211 },
    { "&Ocirc;"sv, 212 },
    { "&Otilde;"sv, 213 },
    { "&Ouml;"sv, 214 },
    { "&times;"sv, 215 },
    { "&Oslash;"sv, 216 },
    { "&Ugrave;"sv, 217 },
    { "&Uacute;"sv, 218 },
    { "&Ucirc;"sv, 219 },
    { "&Uuml;"sv, 220 },
    { "&Yacute;"sv, 221 },
    { "&THORN;"sv, 222 },
    { "&szlig;"sv, 223 },
    { "&agrave;"sv, 224 },
    { "&aacute;"sv, 225 },
    { "&acirc;"sv, 226 },
    { "&atilde;"sv, 227 },
    { "&auml;"sv, 228 },
    { "&aring;"sv, 229 },
    { "&aelig;"sv, 230 },
    { "&ccedil;"sv, 231 },
    { "&egrave;"sv, 232 },
    { "&eacute;"sv, 233 },
    { "&ecirc;"sv, 234 },
    { "&euml;"sv, 235 },
    { "&igrave;"sv, 236 },
    { "&iacute;"sv, 237 },
    { "&icirc;"sv, 238 },
    { "&iuml;"sv, 239 },
    { "&ieth;"sv, 240 },
    { "&ntilde;"sv, 241 },
    { "&ograve;"sv, 242 },
    { "&oacute;"sv, 243 },
    { "&ocirc;"sv, 244 },
    { "&otilde;"sv, 245 },
    { "&ouml;"sv, 246 },
    { "&divide;"sv, 247 },
    { "&oslash;"sv, 248 },
    { "&ugrave;"sv, 249 },
    { "&uacute;"sv, 250 },
    { "&ucirc;"sv, 251 },
    { "&uuml;"sv, 252 },
    { "&yacute;"sv, 253 },
    { "&thorn;"sv, 254 },
    { "&yuml;"sv, 255 }
};

static const std::unordered_map<uint8_t, std::string_view> char_to_entities
{
    { 34,  "&quot;"sv },
    { 38,  "&amp;"sv },
    { 60,  "&lt;"sv },
    { 62,  "&gt;"sv },
    { 160, "&nbsp;"sv },
    { 161, "&iexcl;"sv },
    { 162, "&cent;"sv },
    { 163, "&pound;"sv },
    { 164, "&curren;"sv },
    { 165, "&yen;"sv },
    { 166, "&brvbar;"sv },
    { 167, "&sect;"sv },
    { 168, "&uml;"sv },
    { 169, "&copy;"sv },
    { 170, "&ordf;"sv },
    { 171, "&laquo;"sv },
    { 172, "&not;"sv },
    { 173, "&shy;"sv },
    { 174, "&reg;"sv },
    { 175, "&macr;"sv },
    { 176, "&deg;"sv },
    { 177, "&plusmn;"sv },
    { 178, "&sup2;"sv },
    { 179, "&sup3;"sv },
    { 180, "&acute;"sv },
    { 181, "&micro;"sv },
    { 182, "&para;"sv },
    { 183, "&middot;"sv },
    { 184, "&cedil;"sv },
    { 185, "&sup1;"sv },
    { 186, "&ordm;"sv },
    { 187, "&raquo;"sv },
    { 188, "&frac14;"sv },
    { 189, "&frac12;"sv },
    { 190, "&frac34;"sv },
    { 191, "&iquest;"sv },
    { 192, "&Agrave;"sv },
    { 193, "&Aacute;"sv },
    { 194, "&Acirc;"sv },
    { 195, "&Atilde;"sv },
    { 196, "&Auml;"sv },
    { 197, "&ring;"sv },
    { 198, "&AElig;"sv },
    { 199, "&Ccedil;"sv },
    { 200, "&Egrave;"sv },
    { 201, "&Eacute;"sv },
    { 202, "&Ecirc;"sv },
    { 203, "&Euml;"sv },
    { 204, "&Igrave;"sv },
    { 205, "&Iacute;"sv },
    { 206, "&Icirc;"sv },
    { 207, "&Iuml;"sv },
    { 208, "&ETH;"sv },
    { 209, "&Ntilde;"sv },
    { 210, "&Ograve;"sv },
    { 211, "&Oacute;"sv },
    { 212, "&Ocirc;"sv },
    { 213, "&Otilde;"sv },
    { 214, "&Ouml;"sv },
    { 215, "&times;"sv },
    { 216, "&Oslash;"sv },
    { 217, "&Ugrave;"sv },
    { 218, "&Uacute;"sv },
    { 219, "&Ucirc;"sv },
    { 220, "&Uuml;"sv },
    { 221, "&Yacute;"sv },
    { 222, "&THORN;"sv },
    { 223, "&szlig;"sv },
    { 224, "&agrave;"sv },
    { 225, "&aacute;"sv },
    { 226, "&acirc;"sv },
    { 227, "&atilde;"sv },
    { 228, "&auml;"sv },
    { 229, "&aring;"sv },
    { 230, "&aelig;"sv },
    { 231, "&ccedil;"sv },
    { 232, "&egrave;"sv },
    { 233, "&eacute;"sv },
    { 234, "&ecirc;"sv },
    { 235, "&euml;"sv },
    { 236, "&igrave;"sv },
    { 237, "&iacute;"sv },
    { 238, "&icirc;"sv },
    { 239, "&iuml;"sv },
    { 240, "&ieth;"sv },
    { 241, "&ntilde;"sv },
    { 242, "&ograve;"sv },
    { 243, "&oacute;"sv },
    { 244, "&ocirc;"sv },
    { 245, "&otilde;"sv },
    { 246, "&ouml;"sv },
    { 247, "&divide;"sv },
    { 248, "&oslash;"sv },
    { 249, "&ugrave;"sv },
    { 250, "&uacute;"sv },
    { 251, "&ucirc;"sv },
    { 252, "&uuml;"sv },
    { 253, "&yacute;"sv },
    { 254, "&thorn;"sv },
    { 255, "&yuml;"sv }
};

std::string html_decode(std::string_view data)
{
    std::ostringstream oss;
    auto size { std::size(data) };

    for (decltype(size) pos { 0 }; pos < size; ++pos)
    {
        if (data[pos] == '&')
        {
            auto end_pos { data.find(';', pos) };

            if (end_pos == std::string_view::npos)
            {
                oss << data[pos];

                continue;
            }

            if (auto length { end_pos - pos }; length >= 3 && length <= 8)
            {
                auto entity { data.substr(pos, length + 1) };

                if (entity[1] == '#')
                {
                    int ch { std::atoi(std::string { entity.substr(2, length - 2) }.c_str()) };

                    if (ch > 0 && ch <= std::numeric_limits<uint8_t>::max()) oss << static_cast<uint8_t>(ch);
                    else
                    {
                        oss << data[pos];

                        continue;
                    }
                }
                else
                {
                    if (auto it { entities_to_char.find(entity) }; it != std::end(entities_to_char))
                        oss << (*it).second;
                    else
                    {
                        oss << data[pos];

                        continue;
                    }
                }

                pos += length;
            }
            else
            {
                oss << data[pos];

                continue;
            }
        }
        else oss << data[pos];
    }

    return oss.str();
}

std::string html_encode(std::string_view data)
{
    std::ostringstream oss;

    for (auto ch : data)
    {
        if (auto it { char_to_entities.find(static_cast<uint8_t>(ch)) }; it != std::end(char_to_entities))
             oss << (*it).second;
        else oss << ch;
    }

    return oss.str();
}

}

}
