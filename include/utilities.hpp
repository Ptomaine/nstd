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
#include <cctype>
#include <filesystem>
#include <functional>
#include <fstream>
#include <future>
#include <string>
#include <string_view>
#include <unordered_map>

namespace nstd::utilities
{

class at_scope_exit
{
public:

    at_scope_exit(const std::function<void(void)> &functor) : _functor { functor } {}

    ~at_scope_exit()
    {
        _functor();
    }

    at_scope_exit() = delete;
    at_scope_exit(const at_scope_exit&) = delete;
    at_scope_exit(at_scope_exit&&) = delete;
    at_scope_exit &operator =(const at_scope_exit&) = delete;
    at_scope_exit &operator =(at_scope_exit&&) = delete;

private:
    const std::function<void(void)> &_functor;
};

struct case_insensitive_hash
{
    template<typename StringType>
    size_t operator()(const StringType& key) const
    {
        std::size_t h { 0 };
        std::hash<int> hash;

        if constexpr (std::is_same_v<std::remove_reference_t<std::remove_cv_t<typename StringType::value_type>>, char>)
        {
            for(auto c : key) h ^= hash(std::tolower(c)) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }
        else if constexpr (std::is_same_v<std::remove_reference_t<std::remove_cv_t<typename StringType::value_type>>, wchar_t>)
        {
            for(auto c : key) h ^= hash(std::towlower(c)) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }
        else static_assert("Unsupported char type");

        return h;
    }
};

struct case_insensitive_equal
{
    template<typename StringType>
    bool operator()(const StringType& left, const StringType& right) const
    {
        if constexpr (std::is_same_v<std::remove_reference_t<std::remove_cv_t<typename StringType::value_type>>, char>)
        {
            return std::size(left) == std::size(right) &&
                   std::equal(std::begin(left), std::end(left), std::begin(right),
                                [](auto ca, auto cb)
                                {
                                    return std::tolower(ca) == std::tolower(cb);
                                });
        }
        else if constexpr (std::is_same_v<std::remove_reference_t<std::remove_cv_t<typename StringType::value_type>>, wchar_t>)
        {
            return std::size(left) == std::size(right) &&
                   std::equal(std::begin(left), std::end(left), std::begin(right),
                                [](auto ca, auto cb)
                                {
                                    return std::towlower(ca) == std::towlower(cb);
                                });
        }
        else static_assert("Unsupported char type");
    }
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

}

}
