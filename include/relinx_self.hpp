#pragma once

/*
MIT License

Copyright (c) 2025 Arlen Keshabyan (arlen.albert@gmail.com)

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
#include <any>
#include <concepts>
#include <functional>
#include <iterator>
#include <list>
#include <memory>
#include <numeric>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <type_traits>
#include <variant>

#include <iostream>

namespace nstd::relinx_self
{

using namespace std::literals;

template<typename T> using default_container = std::vector<T>;
template<typename T, typename V> using default_map = std::unordered_map<T, V>;
template<typename T, typename V> using default_multimap = std::unordered_multimap<T, V>;
template<typename T> using default_set = std::unordered_set<T>;

template<typename T>
concept PointerFor = std::is_pointer_v<T>;

template<typename P>
concept PolymorphicPointerFor = PointerFor<P> && std::is_polymorphic_v<std::remove_pointer_t<P>>;

template <typename T, template <typename...> class Z>
struct is_specialization_of : std::false_type { };

template <typename... Args, template <typename...> class Z>
struct is_specialization_of<Z<Args...>, Z> : std::true_type { };

template <typename A, template <typename...> typename B>
concept SpecializationOf = is_specialization_of<A, B>::value;

using default_iterator_adapter_tag = std::forward_iterator_tag;

template<typename ParentIterator1, typename ParentIterator2>
class concat_iterator_adapter
{
public:
    using self_type = concat_iterator_adapter<ParentIterator1, ParentIterator2>;
    using value_type = typename std::decay<decltype(*ParentIterator2())>::type;
    using difference_type = std::ptrdiff_t;
    using pointer = const value_type*;
    using reference = const value_type&;
    using iterator_category = default_iterator_adapter_tag;

    concat_iterator_adapter() = default;
    concat_iterator_adapter(const self_type &) = default;
    concat_iterator_adapter(self_type &&) noexcept = default;
    concat_iterator_adapter &operator=(const self_type &) = default;
    concat_iterator_adapter &operator=(self_type &&) noexcept = default;

    concat_iterator_adapter(ParentIterator1 begin1, ParentIterator1 end1, ParentIterator2 begin2, ParentIterator2 end2)
    : _begin1(begin1), _end1(end1), _begin2(begin2), _end2(end2)
    {
    }

    auto operator==(const self_type &s) const
    {
        return ((_begin1 == _end1 && s._begin1 == s._end1 && _begin2 == _end2 && s._begin2 == s._end2) ||
                (_begin1 == s._begin1 && _end1 == s._end1 && _begin2 == s._begin2 && _end2 == s._end2));
    }

    auto operator!=(const self_type &s) const
    {
        return !(*this == s);
    }

    auto operator*() const -> const value_type&
    {
        if (_begin1 == _end1) return *_begin2;

        return *_begin1;
    }

    auto operator->() const -> const value_type&
    {
        return *(*this);
    }

    auto operator++() -> self_type&
    {
        if (_begin1 == _end1)
        {
            if (_begin2 != _end2) ++_begin2;
        }
        else ++_begin1;

        return *this;
    }

    auto operator++(int) -> self_type
    {
        auto __tmp = *this;

        ++(*this);

        return __tmp;
    }

protected:
    ParentIterator1 _begin1;
    ParentIterator1 _end1;
    ParentIterator2 _begin2;
    ParentIterator2 _end2;
};

template<typename ParentIterator, typename FilterFunctor>
class filter_iterator_adapter
{
public:
    using self_type = filter_iterator_adapter<ParentIterator, FilterFunctor>;
    using value_type = typename std::decay<decltype(*ParentIterator())>::type;
    using difference_type = std::ptrdiff_t;
    using pointer = const value_type*;
    using reference = const value_type&;
    using iterator_category = default_iterator_adapter_tag;

    filter_iterator_adapter() = default;
    filter_iterator_adapter(const self_type &) = default;
    filter_iterator_adapter(self_type &&) noexcept = default;
    filter_iterator_adapter &operator=(const self_type &) = default;
    filter_iterator_adapter &operator=(self_type &&) noexcept = default;

    filter_iterator_adapter(ParentIterator begin, ParentIterator end, FilterFunctor &&filterFunctor)
    : _begin(begin), _end(end), _filterFunctor(std::forward<FilterFunctor>(filterFunctor))
    {
        _begin = std::find_if(_begin, _end, _filterFunctor);
    }

    auto operator==(const self_type &s) const
    {
        return ((_begin == _end && s._begin == s._end) ||
                (_begin == s._begin && _end == s._end));
    }

    auto operator!=(const self_type &s) const
    {
        return !(*this == s);
    }

    auto operator*() const -> const value_type&
    {
        return *_begin;
    }

    auto operator->() -> const value_type
    {
        return *(*this);
    }

    auto operator++() -> self_type&
    {
        _begin = std::find_if(++_begin, _end, _filterFunctor);

        return *this;
    }

    auto operator++(int) -> self_type
    {
        auto __tmp = *this;

        ++(*this);

        return __tmp;
    }

protected:
    ParentIterator _begin;
    ParentIterator _end;
    FilterFunctor _filterFunctor;
};

template<typename ParentIterator, typename TeeFunctor>
class tee_iterator_adapter
{
public:
    using self_type = tee_iterator_adapter<ParentIterator, TeeFunctor>;
    using value_type = typename std::decay<decltype(*ParentIterator())>::type;
    using difference_type = std::ptrdiff_t;
    using pointer = const value_type*;
    using reference = const value_type&;
    using iterator_category = default_iterator_adapter_tag;

    tee_iterator_adapter() = default;
    tee_iterator_adapter(const self_type &) = default;
    tee_iterator_adapter(self_type &&) noexcept = default;
    tee_iterator_adapter &operator=(const self_type &) = default;
    tee_iterator_adapter &operator=(self_type &&) noexcept = default;

    tee_iterator_adapter(ParentIterator begin, ParentIterator end, TeeFunctor teeFunctor)
    : _begin(begin), _end(end), _teeFunctor(std::move(teeFunctor))
    {
    }

    auto operator==(const self_type &s) const
    {
        return ((_begin == _end && s._begin == s._end) ||
                (_begin == s._begin && _end == s._end));
    }

    auto operator!=(const self_type &s) const
    {
        return !(*this == s);
    }

    auto operator*() const -> const value_type&
    {
        auto &value { *_begin };

        _teeFunctor(value);

        return value;
    }

    auto operator->() const -> const value_type&
    {
        return *(*this);
    }

    auto operator++() -> self_type&
    {
        ++_begin;

        return *this;
    }

    auto operator++(int) -> self_type
    {
        auto __tmp = *this;

        ++(*this);

        return __tmp;
    }

protected:
    ParentIterator _begin;
    ParentIterator _end;
    TeeFunctor _teeFunctor;
};

template<typename ParentIterator, typename TransformFunctor>
class transform_iterator_adapter
{
protected:
    TransformFunctor _transformFunctor;

public:
    using self_type = transform_iterator_adapter<ParentIterator, TransformFunctor>;
    using parent_value_type = typename std::decay<decltype(*std::declval<ParentIterator>())>::type;
    using value_type = decltype(std::declval<TransformFunctor>()(std::declval<parent_value_type>()));
    using difference_type = std::ptrdiff_t;
    using pointer = const value_type*;
    using reference = const value_type&;
    using iterator_category = default_iterator_adapter_tag;

    transform_iterator_adapter() = default;
    transform_iterator_adapter(const self_type &) = default;
    transform_iterator_adapter(self_type &&) noexcept = default;
    transform_iterator_adapter &operator=(const self_type &) = default;
    transform_iterator_adapter &operator=(self_type &&) noexcept = default;

    transform_iterator_adapter(ParentIterator begin, ParentIterator end, TransformFunctor &&transformFunctor)
    : _transformFunctor(std::forward<TransformFunctor>(transformFunctor)), _begin(begin), _end(end)
    {
    }

    auto operator==(const self_type &s) const
    {
        return ((_begin == _end && s._begin == s._end) ||
                (_begin == s._begin && _end == s._end));
    }

    auto operator!=(const self_type &s) const
    {
        return !(*this == s);
    }

    auto operator*() const -> const value_type&
    {
        return (_transformedValue = _transformFunctor(*_begin));
    }

    auto operator->() -> const value_type
    {
        return *(*this);
    }

    auto operator++() -> self_type&
    {
        ++_begin;

        return *this;
    }

    auto operator++(int) -> self_type
    {
        auto __tmp = *this;

        ++(*this);

        return __tmp;
    }

protected:
    ParentIterator _begin;
    ParentIterator _end;
    mutable value_type _transformedValue {};
};

template<typename ParentIterator, typename LimitFunctor>
class limit_iterator_adapter
{
public:
    using self_type = limit_iterator_adapter<ParentIterator, LimitFunctor>;
    using value_type = typename std::decay<decltype(*ParentIterator())>::type;
    using difference_type = std::ptrdiff_t;
    using pointer = const value_type*;
    using reference = const value_type&;
    using iterator_category = default_iterator_adapter_tag;

    limit_iterator_adapter() = default;
    limit_iterator_adapter(const self_type &) = default;
    limit_iterator_adapter(self_type &&) noexcept = default;
    limit_iterator_adapter &operator=(const self_type &) = default;
    limit_iterator_adapter &operator=(self_type &&) noexcept = default;

    limit_iterator_adapter(ParentIterator begin, ParentIterator end, LimitFunctor limitFunctor)
    : _begin(begin), _end(end), _limitFunctor(std::move(limitFunctor))
    {
        if (_limitFunctor && _begin != _end && !_limitFunctor(*_begin)) 
            _begin = _end;
    }

    auto operator==(const self_type &s) const
    {
        return ((_begin == _end && s._begin == s._end) ||
                (_begin == s._begin && _end == s._end));
    }

    auto operator!=(const self_type &s) const
    {
        return !(*this == s);
    }

    auto operator*() const -> const value_type&
    {
        return *_begin;
    }

    auto operator->() const -> const value_type&
    {
        return *(*this);
    }

    auto operator++() -> self_type&
    {
        ++_begin;
        if (_limitFunctor && _begin != _end && !_limitFunctor(*_begin)) 
            _begin = _end;

        return *this;
    }

    auto operator++(int) -> self_type
    {
        auto __tmp = *this;

        ++(*this);

        return __tmp;
    }

protected:
    ParentIterator _begin;
    ParentIterator _end;
    LimitFunctor _limitFunctor;
};

template<typename ParentIterator, typename KeyFunctor>
class distinct_iterator_adapter
{
protected:
    KeyFunctor _keyFunctor;

public:
    using self_type = distinct_iterator_adapter<ParentIterator, KeyFunctor>;
    using value_type = typename std::decay<decltype(*ParentIterator())>::type;
    using key_value_type = typename std::decay<decltype(std::declval<KeyFunctor>()(std::declval<value_type>()))>::type;
    using difference_type = std::ptrdiff_t;
    using pointer = const value_type*;
    using reference = const value_type&;
    using iterator_category = default_iterator_adapter_tag;

    distinct_iterator_adapter() = default;
    distinct_iterator_adapter(const self_type &) = default;
    distinct_iterator_adapter(self_type &&) noexcept = default;
    distinct_iterator_adapter &operator=(const self_type &) = default;
    distinct_iterator_adapter &operator=(self_type &&) noexcept = default;

    distinct_iterator_adapter(ParentIterator begin, ParentIterator end, KeyFunctor keyFunctor)
    : _keyFunctor(std::move(keyFunctor)), _begin(begin), _end(end)
    {
        if (_keyFunctor && _begin != _end) _key_set.insert(_keyFunctor(*_begin));
    }

    auto operator==(const self_type &s) const
    {
        return ((_begin == _end && s._begin == s._end) ||
                (_begin == s._begin && _end == s._end));
    }

    auto operator!=(const self_type &s) const
    {
        return !(*this == s);
    }

    auto operator*() const -> const value_type&
    {
        return *_begin;
    }

    auto operator->() const -> const value_type&
    {
        return *(*this);
    }

    auto operator++() -> self_type&
    {
        if (!_keyFunctor) {
            ++_begin;
            return *this;
        }
        
        key_value_type key;
        ++_begin;
        
        while (_begin != _end && _key_set.find((key = _keyFunctor(*_begin))) != _key_set.end()) 
            ++_begin;

        if (_begin != _end) 
            _key_set.insert(key);

        return *this;
    }

    auto operator++(int) -> self_type
    {
        auto __tmp = *this;

        ++(*this);

        return __tmp;
    }

protected:
    ParentIterator _begin;
    ParentIterator _end;
    default_set<key_value_type> _key_set {};
};

template<typename ParentIterator, typename ContainerSelectorFunctor>
class select_many_iterator_adapter
{
protected:
    ContainerSelectorFunctor _containerSelectorFunctor;

public:
    using self_type = select_many_iterator_adapter<ParentIterator, ContainerSelectorFunctor>;
    using parent_value_type = typename std::decay<decltype(*ParentIterator())>::type;
    using container_type = decltype(std::declval<ContainerSelectorFunctor>()(std::declval<parent_value_type>()));
    using container_iterator_type = decltype(std::begin(std::declval<container_type>()));
    using value_type = typename std::decay<decltype(*std::declval<container_iterator_type>())>::type;
    using difference_type = std::ptrdiff_t;
    using pointer = const value_type*;
    using reference = const value_type&;
    using iterator_category = default_iterator_adapter_tag;

    select_many_iterator_adapter() = default;
    select_many_iterator_adapter(const self_type &) = default;
    select_many_iterator_adapter(self_type &&) noexcept = default;
    select_many_iterator_adapter &operator=(const self_type &) = default;
    select_many_iterator_adapter &operator=(self_type &&) noexcept = default;

    select_many_iterator_adapter(ParentIterator begin, ParentIterator end, ContainerSelectorFunctor &&containerSelectorFunctor)
    : _containerSelectorFunctor(std::forward<ContainerSelectorFunctor>(containerSelectorFunctor)), _begin(begin), _end(end)
    {
        while (_container_begin == _container_end)
        {
            if (_begin == _end) break;

            _resultContainer = _containerSelectorFunctor(*_begin);
            _container_begin = std::begin(_resultContainer);
            _container_end = std::end(_resultContainer);

            if (_container_begin == _container_end) ++_begin;
        }
    }

    auto operator==(const self_type &s) const
    {
        return ((_begin == _end && s._begin == s._end) ||
                (_begin == s._begin && _end == s._end));
    }

    auto operator!=(const self_type &s) const
    {
        return !(*this == s);
    }

    auto operator*() const -> const value_type&
    {
        return *_container_begin;
    }

    auto operator->() -> const value_type
    {
        return *(*this);
    }

    auto operator++() -> self_type&
    {
        if (_container_begin != _container_end) ++_container_begin;

        while (_container_begin == _container_end)
        {
            ++_begin;

            if (_begin == _end) break;

            _resultContainer = _containerSelectorFunctor(*_begin);
            _container_begin = std::begin(_resultContainer);
            _container_end = std::end(_resultContainer);
        }

        return *this;
    }

    auto operator++(int) -> self_type
    {
        auto __tmp = *this;

        ++(*this);

        return __tmp;
    }

protected:
    ParentIterator _begin;
    ParentIterator _end;
    value_type _resultValue {};
    container_type _resultContainer {};
    container_iterator_type _container_begin {};
    container_iterator_type _container_end {};
};

template<typename ParentIterator, typename ExceptIterator, typename CompareFunctor>
class except_iterator_adapter
{
public:
    using self_type = except_iterator_adapter<ParentIterator, ExceptIterator, CompareFunctor>;
    using value_type = typename std::decay<decltype(*ParentIterator())>::type;
    using difference_type = std::ptrdiff_t;
    using pointer = const value_type*;
    using reference = const value_type&;
    using iterator_category = default_iterator_adapter_tag;

    except_iterator_adapter() = default;
    except_iterator_adapter(const self_type &) = default;
    except_iterator_adapter(self_type &&) noexcept = default;
    except_iterator_adapter &operator=(const self_type &) = default;
    except_iterator_adapter &operator=(self_type &&) noexcept = default;

    except_iterator_adapter(ParentIterator begin, ParentIterator end, ExceptIterator exceptBegin, ExceptIterator exceptEnd, CompareFunctor &&compareFunctor)
    : _begin(begin), _end(end), _exceptBegin(exceptBegin), _exceptEnd(exceptEnd), _compareFunctor(std::forward<CompareFunctor>(compareFunctor))
    {
        if (_begin != _end) _begin = std::find_if(_begin, _end, [this](auto &&a) { return std::find_if(_exceptBegin, _exceptEnd, [this, &a](auto &&b) { return _compareFunctor(a, b); } ) == _exceptEnd && _processedValues.find(a) == _processedValues.end(); });
        if (_begin != _end) _processedValues.insert(*_begin);
    }

    auto operator==(const self_type &s) const
    {
        return ((_begin == _end && s._begin == s._end) ||
                (_begin == s._begin && _end == s._end));
    }

    auto operator!=(const self_type &s) const
    {
        return !(*this == s);
    }

    auto operator*() const -> const value_type&
    {
        return *_begin;
    }

    auto operator->() -> const value_type
    {
        return *(*this);
    }

    auto operator++() -> self_type&
    {
        if (_begin != _end) _begin = std::find_if(++_begin, _end, [this](auto &&a) { return std::find_if(_exceptBegin, _exceptEnd, [this, &a](auto &&b) { return _compareFunctor(a, b); } ) == _exceptEnd && _processedValues.find(a) == _processedValues.end(); });
        if (_begin != _end) _processedValues.insert(*_begin);

        return *this;
    }

    auto operator++(int) -> self_type
    {
        auto __tmp = *this;

        ++(*this);

        return __tmp;
    }

protected:
    ParentIterator _begin;
    ParentIterator _end;
    ExceptIterator _exceptBegin;
    ExceptIterator _exceptEnd;
    CompareFunctor _compareFunctor;
    default_set<value_type> _processedValues {};
};

template<typename ParentIterator, typename IntersectIterator, typename CompareFunctor>
class intersect_iterator_adapter
{
public:
    using self_type = intersect_iterator_adapter<ParentIterator, IntersectIterator, CompareFunctor>;
    using value_type = typename std::decay<decltype(*ParentIterator())>::type;
    using difference_type = std::ptrdiff_t;
    using pointer = const value_type*;
    using reference = const value_type&;
    using iterator_category = default_iterator_adapter_tag;

    intersect_iterator_adapter() = default;
    intersect_iterator_adapter(const self_type &) = default;
    intersect_iterator_adapter(self_type &&) noexcept = default;
    intersect_iterator_adapter &operator=(const self_type &) = default;
    intersect_iterator_adapter &operator=(self_type &&) noexcept = default;

    intersect_iterator_adapter(ParentIterator begin, ParentIterator end, IntersectIterator intersectBegin, IntersectIterator intersectEnd, CompareFunctor &&compareFunctor)
    : _begin(begin), _end(end), _intersectBegin(intersectBegin), _intersectEnd(intersectEnd), _compareFunctor(std::forward<CompareFunctor>(compareFunctor))
    {
        if (_begin != _end) _begin = std::find_if(_begin, _end, [this](auto &&a) { return std::find_if(_intersectBegin, _intersectEnd, [this, &a](auto &&b) { return _compareFunctor(a, b); } ) != _intersectEnd && _processedValues.find(a) == _processedValues.end(); });
        if (_begin != _end) _processedValues.insert(*_begin);
    }

    auto operator==(const self_type &s) const
    {
        return ((_begin == _end && s._begin == s._end) ||
                (_begin == s._begin && _end == s._end));
    }

    auto operator!=(const self_type &s) const
    {
        return !(*this == s);
    }

    auto operator*() const -> const value_type&
    {
        return *_begin;
    }

    auto operator->() -> const value_type
    {
        return *(*this);
    }

    auto operator++() -> self_type&
    {
        if (_begin != _end) _begin = std::find_if(++_begin, _end, [this](auto &&a) { return std::find_if(_intersectBegin, _intersectEnd, [this, &a](auto &&b) { return _compareFunctor(a, b); } ) != _intersectEnd && _processedValues.find(a) == _processedValues.end(); });
        if (_begin != _end) _processedValues.insert(*_begin);

        return *this;
    }

    auto operator++(int) -> self_type
    {
        auto __tmp = *this;

        ++(*this);

        return __tmp;
    }

protected:
    ParentIterator _begin;
    ParentIterator _end;
    IntersectIterator _intersectBegin;
    IntersectIterator _intersectEnd;
    CompareFunctor _compareFunctor;
    default_set<value_type> _processedValues {};
};

template<typename ParentIterator>
class cycle_iterator_adapter
{
public:
    using self_type = cycle_iterator_adapter<ParentIterator>;
    using value_type = typename std::decay<decltype(*ParentIterator())>::type;
    using difference_type = std::ptrdiff_t;
    using pointer = const value_type*;
    using reference = const value_type&;
    using iterator_category = default_iterator_adapter_tag;

    cycle_iterator_adapter() = default;
    cycle_iterator_adapter(const self_type &) = default;
    cycle_iterator_adapter(self_type &&) noexcept = default;
    cycle_iterator_adapter &operator=(const self_type &) = default;
    cycle_iterator_adapter &operator=(self_type &&) noexcept = default;

    cycle_iterator_adapter(ParentIterator begin, ParentIterator end, std::ptrdiff_t times)
    : _begin(begin), _end(end), _current(begin), _numberOfTimes(times)
    {
        if (_numberOfTimes == 0 && _begin != _end) _current = _begin = _end;
    }

    auto operator==(const self_type &s) const
    {
        return ((_current == _end && s._current == s._end) ||
                (_current == s._current && _end == s._end));
    }

    auto operator!=(const self_type &s) const
    {
        return !(*this == s);
    }

    auto operator*() const -> const value_type&
    {
        return *_current;
    }

    auto operator->() const -> const value_type&
    {
        return *(*this);
    }

    auto operator++() -> self_type&
    {
        ++_current;

        if (_current == _end && (_numberOfTimes < 0 || ++_currentIteration < _numberOfTimes)) _current = _begin;

        return *this;
    }

    auto operator++(int) -> self_type
    {
        auto __tmp = *this;

        ++(*this);

        return __tmp;
    }

protected:
    ParentIterator _begin;
    ParentIterator _end;
    ParentIterator _current;

    std::ptrdiff_t _numberOfTimes;
    std::ptrdiff_t _currentIteration { 0 };
};

template<typename ParentIterator, typename JoinIterator, typename ThisKeyFunctor, typename OtherKeyFunctor, typename ResultFunctor, typename CompareFunctor>
class join_iterator_adapter
{
protected:
    ThisKeyFunctor _thisKeyFunctor;
    OtherKeyFunctor _otherKeyFunctor;
    ResultFunctor _resultFunctor;

public:
    using self_type = join_iterator_adapter<ParentIterator, JoinIterator, ThisKeyFunctor, OtherKeyFunctor, ResultFunctor, CompareFunctor>;
    using value_type = typename std::decay<decltype(std::declval<ResultFunctor>()(std::declval<typename std::decay<decltype(*ParentIterator())>::type>(), std::declval<typename std::decay<decltype(*JoinIterator())>::type>()))>::type;
    using parent_value_type = typename std::decay<decltype(*ParentIterator())>::type;
    using join_value_type = typename std::decay<decltype(*JoinIterator())>::type;
    using difference_type = std::ptrdiff_t;
    using pointer = const value_type*;
    using reference = const value_type&;
    using iterator_category = default_iterator_adapter_tag;

    join_iterator_adapter() = default;
    join_iterator_adapter(const self_type &) = default;
    join_iterator_adapter(self_type &&) noexcept = default;
    join_iterator_adapter &operator=(const self_type &) = default;
    join_iterator_adapter &operator=(self_type &&) noexcept = default;

    join_iterator_adapter(ParentIterator begin,
                          ParentIterator end,
                          JoinIterator joinBegin,
                          JoinIterator joinEnd,
                          ThisKeyFunctor &&thisKeyFunctor,
                          OtherKeyFunctor &&otherKeyFunctor,
                          ResultFunctor &&resultFunctor,
                          CompareFunctor &&compareFunctor,
                          bool leftJoin)
    : _thisKeyFunctor(std::forward<ThisKeyFunctor>(thisKeyFunctor)),
        _otherKeyFunctor(std::forward<OtherKeyFunctor>(otherKeyFunctor)),
        _resultFunctor(std::forward<ResultFunctor>(resultFunctor)),
        _begin(begin),
        _end(end),
        _joinBegin(joinBegin),
        _joinEnd(joinEnd),
        _joinMatch(joinBegin),
        _joinMatchBegin(joinBegin),
        _compareFunctor(std::forward<CompareFunctor>(compareFunctor)),
        _leftJoin(leftJoin)
    {
        using thisKeyType = typename std::decay<decltype(_thisKeyFunctor(parent_value_type()))>::type;

        thisKeyType thisKey;

        while (_begin != _end)
        {
            thisKey = _thisKeyFunctor(*_begin);

            _joinMatch = std::find_if(_joinBegin, _joinEnd, [this, &thisKey](auto &&v) { return _compareFunctor(thisKey, _otherKeyFunctor(v)); });

            if (_joinMatch != _joinEnd || _leftJoin) break;

            ++_begin;
        }
    }

    auto operator==(const self_type &s) const
    {
        return ((_begin == _end && s._begin == s._end) ||
                (_begin == s._begin && _end == s._end));
    }

    auto operator!=(const self_type &s) const
    {
        return !(*this == s);
    }

    auto operator*() const -> const value_type&
    {
        if (_joinMatchBegin == _joinBegin && _joinMatch == _joinEnd && _leftJoin) return (_resultValue = _resultFunctor(*_begin, join_value_type()));

        return (_resultValue = _resultFunctor(*_begin, *_joinMatch));
    }

    auto operator->() -> const value_type
    {
        return *(*this);
    }

    auto operator++() -> self_type&
    {
        using thisKeyType = typename std::decay<decltype(_thisKeyFunctor(parent_value_type()))>::type;

        thisKeyType thisKey;

        if (_joinMatchBegin == _joinBegin && _joinMatch == _joinEnd && _leftJoin) ++_begin;

        while (_begin != _end)
        {
            thisKey = _thisKeyFunctor(*_begin);
            _joinMatchBegin = _joinMatch == _joinEnd ? _joinBegin : ++_joinMatch;

            _joinMatch = std::find_if(_joinMatchBegin, _joinEnd, [this, &thisKey](auto &&v) { return _compareFunctor(thisKey, _otherKeyFunctor(v)); });

            if (_joinMatch != _joinEnd || (_joinMatchBegin == _joinBegin && _leftJoin)) break;

            ++_begin;
        }

        return *this;
    }

    auto operator++(int) -> self_type
    {
        auto __tmp = *this;

        ++(*this);

        return __tmp;
    }

protected:
    ParentIterator _begin;
    ParentIterator _end;
    JoinIterator _joinBegin;
    JoinIterator _joinEnd;
    JoinIterator _joinMatch;
    JoinIterator _joinMatchBegin;
    CompareFunctor _compareFunctor;
    bool _leftJoin;
    mutable value_type _resultValue {};
};

template<typename ParentIterator, typename JoinIterator, typename ThisKeyFunctor, typename OtherKeyFunctor, typename ResultFunctor, typename CompareFunctor>
class group_join_iterator_adapter
{
protected:
    ThisKeyFunctor _thisKeyFunctor;
    OtherKeyFunctor _otherKeyFunctor;
    ResultFunctor _resultFunctor;

public:
    using self_type = group_join_iterator_adapter<ParentIterator, JoinIterator, ThisKeyFunctor, OtherKeyFunctor, ResultFunctor, CompareFunctor>;
    using value_type = typename std::decay<decltype(std::declval<ResultFunctor>()(std::declval<typename std::decay<decltype(*ParentIterator())>::type>(), default_container<typename std::decay<decltype(*JoinIterator())>::type>()))>::type;
    using parent_value_type = typename std::decay<decltype(*ParentIterator())>::type;
    using join_value_type = typename std::decay<decltype(*JoinIterator())>::type;
    using difference_type = std::ptrdiff_t;
    using pointer = const value_type*;
    using reference = const value_type&;
    using iterator_category = default_iterator_adapter_tag;

    group_join_iterator_adapter() = default;
    group_join_iterator_adapter(const self_type &) = default;
    group_join_iterator_adapter(self_type &&) noexcept = default;
    group_join_iterator_adapter &operator=(const self_type &) = default;
    group_join_iterator_adapter &operator=(self_type &&) noexcept = default;

    group_join_iterator_adapter(ParentIterator begin,
                          ParentIterator end,
                          JoinIterator joinBegin,
                          JoinIterator joinEnd,
                          ThisKeyFunctor &&thisKeyFunctor,
                          OtherKeyFunctor &&otherKeyFunctor,
                          ResultFunctor &&resultFunctor,
                          CompareFunctor &&compareFunctor,
                          bool leftJoin)
    : _thisKeyFunctor(std::forward<ThisKeyFunctor>(thisKeyFunctor)),
        _otherKeyFunctor(std::forward<OtherKeyFunctor>(otherKeyFunctor)),
        _resultFunctor(std::forward<ResultFunctor>(resultFunctor)),
        _begin(begin),
        _end(end),
        _joinBegin(joinBegin),
        _joinEnd(joinEnd),
        _compareFunctor(std::forward<CompareFunctor>(compareFunctor)),
        _leftJoin(leftJoin)
    {
        using thisKeyType = typename std::decay<decltype(_thisKeyFunctor(parent_value_type()))>::type;

        thisKeyType thisKey;

        while (_begin != _end)
        {
            thisKey = _thisKeyFunctor(*_begin);

            auto joinIt = _joinBegin;

            while (joinIt != _joinEnd) { if (_compareFunctor(thisKey, _otherKeyFunctor(*joinIt))) _groupedValues.emplace_back(*joinIt); ++joinIt; }

            if (!_groupedValues.empty() || _leftJoin) break;

            ++_begin;
        }
    }

    auto operator==(const self_type &s) const
    {
        return ((_begin == _end && s._begin == s._end) ||
                (_begin == s._begin && _end == s._end));
    }

    auto operator!=(const self_type &s) const
    {
        return !(*this == s);
    }

    auto operator*() const -> const value_type&
    {
        return (_resultValue = _resultFunctor(*_begin, _groupedValues));
    }

    auto operator->() -> const value_type
    {
        return *(*this);
    }

    auto operator++() -> self_type&
    {
        using thisKeyType = typename std::decay<decltype(_thisKeyFunctor(parent_value_type()))>::type;

        _groupedValues.clear();

        thisKeyType thisKey;

        ++_begin;

        while (_begin != _end)
        {
            thisKey = _thisKeyFunctor(*_begin);

            auto joinIt = _joinBegin;

            while (joinIt != _joinEnd) { if (_compareFunctor(thisKey, _otherKeyFunctor(*joinIt))) _groupedValues.emplace_back(*joinIt); ++joinIt; }

            if (!_groupedValues.empty() || _leftJoin) break;

            ++_begin;
        }

        return *this;
    }

    auto operator++(int) -> self_type
    {
        auto __tmp = *this;

        ++(*this);

        return __tmp;
    }

protected:
    ParentIterator _begin;
    ParentIterator _end;
    JoinIterator _joinBegin;
    JoinIterator _joinEnd;
    CompareFunctor _compareFunctor;
    bool _leftJoin;
    mutable value_type _resultValue {};
    default_container<join_value_type> _groupedValues {};
};

template<typename ParentIterator1, typename ParentIterator2, typename ResultFunctor>
class zip_iterator_adapter
{
protected:
    ResultFunctor _resultFunctor;

public:
    using self_type = zip_iterator_adapter<ParentIterator1, ParentIterator2, ResultFunctor>;
    using value_type = typename std::decay<decltype(std::declval<ResultFunctor>()(std::declval<typename std::decay<decltype(*ParentIterator1())>::type>(), std::declval<typename std::decay<decltype(*ParentIterator2())>::type>()))>::type;
    using difference_type = std::ptrdiff_t;
    using pointer = const value_type*;
    using reference = const value_type&;
    using iterator_category = default_iterator_adapter_tag;

    zip_iterator_adapter() = default;
    zip_iterator_adapter(const self_type &) = default;
    zip_iterator_adapter(self_type &&) noexcept = default;
    zip_iterator_adapter &operator=(const self_type &) = default;
    zip_iterator_adapter &operator=(self_type &&) noexcept = default;

    zip_iterator_adapter(ParentIterator1 begin1, ParentIterator1 end1, ParentIterator2 begin2, ParentIterator2 end2, ResultFunctor &&resultFunctor)
    : _resultFunctor(std::forward<ResultFunctor>(resultFunctor)), _begin1(begin1), _end1(end1), _begin2(begin2), _end2(end2)
    {
        if (_begin1 == _end1 || _begin2 == _end2)
        {
            _begin1 = _end1;
            _begin2 = _end2;
        }
    }

    auto operator==(const self_type &s) const
    {
        return ((_begin1 == _end1 && s._begin1 == s._end1 && _begin2 == _end2 && s._begin2 == s._end2) ||
                (_begin1 == s._begin1 && _end1 == s._end1 && _begin2 == s._begin2 && _end2 == s._end2));
    }

    auto operator!=(const self_type &s) const
    {
        return !(*this == s);
    }

    auto operator*() const -> const value_type&
    {
        return (_resultValue = _resultFunctor(*_begin1, *_begin2));
    }

    auto operator->() const -> const value_type&
    {
        return *(*this);
    }

    auto operator++() -> self_type&
    {
        ++_begin1;
        ++_begin2;

        if (_begin1 == _end1 || _begin2 == _end2)
        {
            _begin1 = _end1;
            _begin2 = _end2;
        }

        return *this;
    }

    auto operator++(int) -> self_type
    {
        auto __tmp = *this;

        ++(*this);

        return __tmp;
    }

protected:
    ParentIterator1 _begin1;
    ParentIterator1 _end1;
    ParentIterator2 _begin2;
    ParentIterator2 _end2;
    mutable value_type _resultValue {};
};

struct no_elements : public std::logic_error { no_elements(const std::string &what) : std::logic_error(what) {} }; ///< The exeption that is thrown when there are no elements in the targeting sequence;
struct not_found : public std::logic_error { not_found(const std::string &what) : std::logic_error(what) {} }; ///< The exeption that is thrown when there are no elements found using a filter functor on the targeting sequence
struct invalid_operation : public std::logic_error { invalid_operation(const std::string &what) : std::logic_error(what) {} }; ///< The exeption that is thrown when there is no complience to the performing action

template<typename ParentRelinxType, typename Iterator, typename ContainerType = default_container<typename std::decay<decltype(*Iterator())>::type>>
class relinx_object_ordered;

template<typename ParentRelinxType, typename Iterator, typename ContainerType = default_container<typename std::decay<decltype(*Iterator())>::type>>
class relinx_object
{
public:
    using self_type = relinx_object<ParentRelinxType, Iterator, ContainerType>;
    using value_type = typename std::decay<decltype(*Iterator())>::type;
    using iterator_type = typename std::decay<Iterator>::type;

    relinx_object() = delete;
    relinx_object(const relinx_object &) = delete;
    auto operator =(const relinx_object &) -> relinx_object & = delete;
    relinx_object(relinx_object &&) noexcept = default;

    // Single constructor for iterators - takes by value and moves
    relinx_object(std::shared_ptr<ParentRelinxType> parent_relinx_object_ptr, iterator_type begin, iterator_type end) noexcept 
        : _begin(std::move(begin)), _end(std::move(end)), _parent_relinx_object_ptr(std::move(parent_relinx_object_ptr)) { }
    
    // Container constructor
    relinx_object(std::shared_ptr<ParentRelinxType> parent_relinx_object_ptr, ContainerType container) noexcept 
        : _container(std::move(container)), _begin(std::begin(_container)), _end(std::end(_container)), _parent_relinx_object_ptr(std::move(parent_relinx_object_ptr)) { }

    void set_self_ptr(std::shared_ptr<self_type> self_ptr) noexcept {
        _self_ptr = std::move(self_ptr);
    }

    ~relinx_object() noexcept = default;

    auto begin() const noexcept
    {
        return _begin;
    }

    auto end() const noexcept
    {
        return _end;
    }

    /**
        \brief Applies an accumulator functor over a sequence.

        The aggregate method makes it simple to perform a calculation over a sequence of values.
        This method works by calling AggregateFunctor one time for each element in source except the first one.
        Each time AggregateFunctor is called, aggregate passes both the element from the sequence and an aggregated value (as the first argument to AggregateFunctor).
        The first element of source is used as the initial aggregate value.
        The result of AggregateFunctor replaces the previous aggregated value. aggregate returns the final result of AggregateFunctor.
        This overload of the aggregate method isn't suitable for all cases because it uses the first element of source as the initial aggregate value.
        You should choose another overload if the return value should include only the elements of source that meet a certain condition.
        For example, this overload isn't reliable if you want to calculate the sum of the even numbers in source.
        The result will be incorrect if the first element is odd instead of even.
        To simplify common aggregation operations, the standard query operators also include a general purpose count method and four numeric aggregation methods, namely min, max, sum, and average.

        \param aggregateFunctor A functor. The first parameter of the functor is the current aggregated value and the second one is the current value of the sequence. The return value of the functor is the next or last aggregated value.

        \return An aggregated value.

        Example of usage: \code auto result = from({1, 2, 3, 4, 5, 6, 7, 8, 9, 10})->aggregate([](auto &&aggregated_value, auto &&current_value) { return aggregated_value + current_value; }); \endcode
    */
    template<typename AggregateFunctor>
    auto aggregate(AggregateFunctor &&aggregateFunctor) const
    {
        if (_begin == _end) throw no_elements("aggregate"s);

        auto begin = _begin;
        auto front = *_begin;

        return std::accumulate(++begin, _end, front, std::forward<AggregateFunctor>(aggregateFunctor));
    }

    /**
        \brief Applies an accumulator functor over a sequence. The specified seed value is used as the initial accumulator value.

        The aggregate method makes it simple to perform a calculation over a sequence of values.This method works by calling func one time for each element in source.
        Each time func is called, aggregate passes both the element from the sequence and an aggregated value (as the first argument to func).
        The value of the seed parameter is used as the initial aggregate value. The result of func replaces the previous aggregated value. aggregate returns the final result of func.
        To simplify common aggregation operations, the standard query operators also include a general purpose count method and four numeric aggregation methods, namely min, max, sum, and average.

        \param seed Is an initial value for the aggreagte functor.
        \param aggregateFunctor A functor. The first parameter of the functor is the current aggregated value and the second one is the current value of the sequence. The return value of the functor is the next or last aggregated value.

        \return An aggregated value.

        Example of usage: \code auto result = from({2, 3, 4, 5, 6, 7, 8, 9, 10})->aggregate(1, [](auto &&aggregated_value, auto &&current_value) { return aggregated_value + current_value; }); \endcode
    */
    template<typename SeedType, typename AggregateFunctor>
    auto aggregate(SeedType &&seed, AggregateFunctor &&aggregateFunctor) const
    {
        if (_begin == _end) throw no_elements("aggregate"s);

        return std::accumulate(_begin, _end, std::forward<SeedType>(seed), std::forward<AggregateFunctor>(aggregateFunctor));
    }

    /**
        \brief Applies an accumulator functor over a sequence. The specified seed value is used as the initial accumulator value, and the specified functor is used to select the result value.

        The aggregate method makes it simple to perform a calculation over a sequence of values. This method works by calling func one time for each element in source.
        Each time func is called, aggregate passes both the element from the sequence and an aggregated value (as the first argument to func). The value of the seed parameter is used as the initial aggregate value.
        The result of func replaces the previous aggregated value. The final result of func is passed to ResultSelector to obtain the final result of aggregate.
        To simplify common aggregation operations, the standard query operators also include a general purpose count method and four numeric aggregation methods, namely min, max, sum, and average.

        \param seed Is an initional value for the aggreagte functor.
        \param aggregateFunctor A functor. The first parameter of the functor is the current aggregated value and the second one is the current value of the sequence. The return value of the functor is the next or last aggregated value.
        \param resultSelector A functor that does the final transformation of the aggregated value. The single parameter of the functor is the final aggregated value. The return value of the functor is the transformed value of any other type or the same one.

        \return A transformed aggregated value.

        Example: \code auto result = from({9, 8, 7, 6, 5, 4, 3, 2, 1})->aggregate(10, [](auto &&a, auto &&b) { return a + b; }, [](auto &&r) { return "("s + std::to_string(r * 2.5 + .5) + ")"; }); \endcode
    */
    template<typename SeedType, typename AggregateFunctor, typename ResultSelector>
    auto aggregate(SeedType &&seed, AggregateFunctor &&aggregateFunctor, ResultSelector &&resultSelector) const
    {
        if (_begin == _end) throw no_elements("aggregate"s);

        return resultSelector(std::forward<decltype(aggregate(seed, aggregateFunctor))>(aggregate(std::forward<SeedType>(seed), std::forward<AggregateFunctor>(aggregateFunctor))));
    }

    /**
        \brief Determines whether all elements of a sequence satisfy a condition.

        Determines whether all elements of a sequence satisfy a condition.

        \param conditionFunctor A functor to test each element for a condition.

        \return true if every element of the source sequence passes the test in the specified predicate, or if the sequence is empty; otherwise, false.
    */
    template<typename ConditionFunctor>
    auto all(ConditionFunctor &&conditionFunctor) const noexcept
    {
        return std::all_of(_begin, _end, std::forward<ConditionFunctor>(conditionFunctor));
    }

    /**
        \brief Determines whether any element of a sequence exists or satisfies a condition.

        Determines whether any element of a sequence exists or satisfies a condition.

        \param conditionFunctor A functor to test each element for a condition.

        \return true if any elements in the source sequence pass the test in the specified predicate; otherwise, false.
    */
    template<typename ConditionFunctor>
    auto any(ConditionFunctor &&conditionFunctor) const noexcept
    {
        return std::any_of(_begin, _end, std::forward<ConditionFunctor>(conditionFunctor));
    }

    /**
        \brief Determines whether a sequence contains any elements.

        Determines whether a sequence contains any elements.

        \return true if the source sequence contains any elements; otherwise, false.
    */
    auto any() const noexcept
    {
        return !(_begin == _end);
    }

    /**
        \brief Computes the average of a sequence.

        Computes the average of a sequence of values that are obtained by invoking a transform functor on each element of the input sequence.

        \param avgFunctor A functor. The single parameter is the current sequence value. The return value of the functor is a transformed value.

        \return A computed average value.

        Example: \code auto result = from({1, 2, 3, 4, 5, 6, 7, 8, 9, 10})->average([](auto &&r){ return r * 1.5; }); \endcode the result is 8.25
    */
    template<typename AvgFunctor>
    auto average(AvgFunctor &&avgFunctor) const noexcept
    {
        if (_begin == _end) throw no_elements("average"s);

        return sum(std::forward<AvgFunctor>(avgFunctor)) / std::distance(_begin, _end);
    }

    /**
        \brief Computes the average of a sequence.

        Computes the average of a sequence.

        \return Returns an average value.

    */
    auto average() const noexcept
    {
        return average([](auto &&v){ return v; });
    }

    /**
        \brief Casts the elements of a relinc_object to the specified type.

        Casts the elements of the relinx_object to the specified type using the static_cast operator.
        If you need dynamic_cast or reinterprete_cast you can use 'select' member functor to transform elements using that casting operators.


        \note This method produces a lazy evaluation relinx_object.

        \return returns a new relinx_object with elements casted to the specified type.

        Example: \code
        struct base {};
        struct derived : public base { virtual ~derived(){} };

        auto data = {new derived(), new derived(), new derived()};

        auto result1 = from({1, 2, 3, 4, 5, 6, 7, 8, 9, 10})->cast<float>();
        auto result2 = from({derived(), derived(), derived()})->cast<base>();
        auto result3 = from(data)->cast<base*>();

        for(auto &&i : data){ delete i; };

        //type_of emulation
        {
            struct derived2 : public base { virtual ~derived2(){} };

            std::list<base*> data = {new derived(), new derived2(), new derived(), new derived(), new derived2()};

            auto result = from(data)->select([](auto &&i) { return dynamic_cast<derived2*>(i); })->where([](auto &&v) { return v != nullptr; });
            // result now contains two derived2 objects

            for(auto &&i : data){ delete i; };
        }
        \endcode
    */
    template<typename CastType>
    auto cast() noexcept
    {
        using adapter_type = transform_iterator_adapter<iterator_type, std::function<CastType(const value_type&)>>;
        using next_relinx_object = relinx_object<self_type, adapter_type, ContainerType>;

        auto result = std::make_shared<next_relinx_object>(_self_ptr, 
                                                         adapter_type(_begin, _end, [](auto &&v){ return static_cast<CastType>(v); }), 
                                                         adapter_type(_end, _end, nullptr));
        result->set_self_ptr(result);
        return result;
    }

    /**
        \brief Concatenates two sequences.

        Concatenates two sequences with the same element type or types that can be converted to the current relinc_object element type implicitly.

        \param begin Is a begin iterator of a container to add to the current relinc_object.
        \param end Is an end iterator of a container to add to the current relinc_object.

        \note This method produces a lazy evaluation relinx_object.

        \return returns concatenated relinc_object.
    */
    template<typename ConcatIterator>
    auto concat(const ConcatIterator &begin, const ConcatIterator &end) noexcept
    {
        using adapter_type = concat_iterator_adapter<iterator_type, typename std::decay<ConcatIterator>::type>;
        using next_relinx_type = relinx_object<self_type, adapter_type, ContainerType>;

        auto result = std::make_shared<next_relinx_type>(_self_ptr, 
                                                      adapter_type(_begin, _end, begin, end), 
                                                      adapter_type(_end, _end, end, end));
        result->set_self_ptr(result);
        return result;
    }

    template<typename Container>
    auto concat(Container &&c) noexcept
    {
        return concat(std::begin(c), std::end(c));
    }

    template<typename T>
    auto concat(std::initializer_list<T> &&container) noexcept
    {
        return concat<std::initializer_list<T>>(std::forward<std::initializer_list<T>>(container));
    }

    template<typename T>
    auto concat(std::shared_ptr<T> &&relinx_object) noexcept
    {
        return concat(*relinx_object);
    }

    /**
        \brief Determines whether a sequence contains a specified element.

        Determines whether a sequence contains a specified element by using the equality operator as a comparer.

        \param value A value to seek for.

        \return returns true if an element was found.

        Example: \code auto result = from({1, 2, 3, 4, 5, 6, 7, 8, 9, 10})->contains(5); \endcode
    */
    auto contains(value_type &&value) const noexcept
    {
        return std::find(_begin, _end, value) != _end;
    }

    /**
        \brief Determines whether a sequence contains a specified element.

        Determines whether a sequence contains a specified element by using the supplied functor as a comparer.

        \param compareFunctor A functor used as a comparer.

        \return returns true if an element found that satisfies a condition.

        Example: \code auto result = from({1, 2, 3, 4, 5, 6, 7, 8, 9, 10})->contains([](auto &&i) { return i % 2 == 0; }); \endcode checks whether a sequence contains an even number
    */
    template<typename CompareFunctor>
    auto contains(CompareFunctor &&compareFunctor) const noexcept
    {
        return std::find_if(_begin, _end, std::forward<CompareFunctor>(compareFunctor)) != _end;
    }

    /**
        \brief Returns the number of elements in a sequence.

        Returns the number of elements in a sequence.

        \return The number of elements in the input sequence.

        Example: \code auto result = from({1, 2, 3, 4, 5, 6, 7, 8, 9, 10})->count(); \endcode return 10 as the result.
    */
    auto count() const noexcept
    {
        return std::distance(_begin, _end);
    }

    /**
        \brief Returns the number of elements in a sequence of a specified value.

        Returns the number of elements in a sequence of a specified value.

        \param value A value to count.

        \return Returns the number of elements in a sequence that equal to a specified value.

        Example: \code auto result = from({1, 2, 3, 1, 2, 3, 1, 2, 3})->count(1); \endcode counts number of 1 and returns 3 as the result.
    */
    auto count(value_type &&value) const noexcept
    {
        return std::count(_begin, _end, std::forward<value_type>(value));
    }

    /**
        \brief Returns a number that represents how many elements in the specified sequence satisfy a condition.

        Returns a number that represents how many elements in the specified sequence satisfy a condition.

        \param conditionFunctor A functor that checks condition.

        \return A number that represents how many elements in the sequence satisfy the condition in the filtering functor.

        Example: \code auto result = from({1, 2, 3, 4, 5, 6, 7, 8, 9, 10})->count([](auto &&i) { return i % 2 == 0; }); \endcode counts even numbers and returns 5 as the result.
    */
    template<typename ConditionFunctor>
    auto count(ConditionFunctor &&conditionFunctor) const noexcept
    {
        return std::count_if(_begin, _end, std::forward<ConditionFunctor>(conditionFunctor));
    }

    /**
        \brief Returns distinct elements from a sequence by using the default hash provider to distinguish values.

        Returns distinct elements from a sequence by using the default hash provider to distinguish values.
        You can write your own custom hash provider for custom types.

        \param keyFunctor This is a key provider functor. This way items distinguished by a key. By defualt, the key functor returns each contained item as a key.

        \note This method produces a lazy evaluation relinx_object.

        \return The relinx_object that contains distinct values.
    */
    template<typename KeyFunctor = std::function<value_type(const value_type &)>>
    auto distinct(KeyFunctor &&keyFunctor = [](auto &&v) { return v; }) noexcept
    {
        using ret_type = typename std::decay<decltype(keyFunctor(value_type()))>::type;
        using adapter_type = distinct_iterator_adapter<iterator_type, std::function<ret_type(const value_type &)>>;
        using next_relinx_type = relinx_object<self_type, adapter_type, ContainerType>;

        auto result = std::make_shared<next_relinx_type>(_self_ptr, 
                                                       adapter_type(_begin, _end, std::forward<KeyFunctor>(keyFunctor)), 
                                                       adapter_type(_end, _end, nullptr));
        result->set_self_ptr(result);
        return result;
    }

    /**
        \brief Filters a sequence of values based on a predicate.

        Filters a sequence of values based on a predicate.

        \param filterFunctor A function to test each element for a condition.

        \note This method produces a lazy evaluation relinx_object.

        \return A relinx_object that contains elements from the input sequence that satisfy the condition.
    */
    template<typename FilterFunctor>
    auto where(FilterFunctor &&filterFunctor) noexcept
    {
        using adapter_type = filter_iterator_adapter<iterator_type, std::decay_t<FilterFunctor>>;
        using next_relinx_type = relinx_object<self_type, adapter_type, ContainerType>;

        auto result = std::make_shared<next_relinx_type>(_self_ptr, 
                                                       adapter_type(_begin, _end, std::forward<FilterFunctor>(filterFunctor)), 
                                                       adapter_type(_end, _end, std::decay_t<FilterFunctor>()));
        result->set_self_ptr(result);
        return result;
    }

    /** 
        \brief Projects each element of a sequence into a new form.

        This method is implemented by using deferred execution.
        The immediate return value is an object that is a result of a transformation.
        A transformation applied on the current element on each iteration only.
        The query represented by this method is not executed until the object is enumerated.
        This projection method requires the transform functor, selector, to produce one value for each value in the source sequence.
        If selector returns a value that is itself a container, it is up to the consumer to traverse the subsequences manually.
        In such a situation, it might be better for your query to return a single coalesced sequence of values.
        To achieve this, use the \ref select_many method instead of \ref select.
        Although \ref select_many works similarly to \ref select, it differs in that the transform functor returns a collection
        that is then expanded by \ref select_many before it is returned.

        \param transformFunctor A transform functor to apply to each element. It takes one input parameter: a current element.

        \note This method produces a lazy evaluation relinx_object.

        \return An relinx_object whose elements are the result of invoking the transform functor on each element of source.
    */
    template<typename TransformFunctor>
    auto select(TransformFunctor &&transformFunctor) noexcept
    {
        using adapter_type = transform_iterator_adapter<iterator_type, std::decay_t<TransformFunctor>>;
        using next_relinx_type = relinx_object<self_type, adapter_type, ContainerType>;

        auto result = std::make_shared<next_relinx_type>(_self_ptr, 
                                                       adapter_type(_begin, _end, std::forward<TransformFunctor>(transformFunctor)), 
                                                       adapter_type(_end, _end, std::decay_t<TransformFunctor>()));
        result->set_self_ptr(result);
        return result;
    }

    /**
        \brief Returns a specified number of contiguous elements from the start of a sequence.

        Returns a specified number of contiguous elements from the start of a sequence.

        \param limit The number of elements to return.

        \note This method produces a lazy evaluation relinx_object.

        \return A relinx_object that contains the specified number of elements from the start of the input sequence.
    */
    auto take(std::ptrdiff_t limit) noexcept
    {
        using adapter_type = limit_iterator_adapter<iterator_type, std::function<bool(const value_type&)>>;
        using next_relinx_type = relinx_object<self_type, adapter_type, ContainerType>;

        auto result = std::make_shared<next_relinx_type>(_self_ptr, 
                                                       adapter_type(_begin, _end, [_indexer = limit](auto &&) mutable { return _indexer--; }), 
                                                       adapter_type(_end, _end, nullptr));
        result->set_self_ptr(result);
        return result;
    }

    /**
        \brief Returns elements from a sequence as long as a specified condition is true.

        Returns elements from a sequence as long as a specified condition is true, and then skips the remaining elements.

        \param limitFunctor A functor to test each element for a condition.

        \note This method produces a lazy evaluation relinx_object.

        \return An relinx_object that contains the elements from the input sequence that occur before the element at which the test no longer passes.
    */
    template<typename LimitFunctor>
    auto take_while(LimitFunctor &&limitFunctor) noexcept
    {
        using adapter_type = limit_iterator_adapter<iterator_type, std::function<bool(const value_type&)>>;
        using next_relinx_type = relinx_object<self_type, adapter_type, ContainerType>;

        auto result = std::make_shared<next_relinx_type>(_self_ptr, 
                                                       adapter_type(_begin, _end, std::forward<LimitFunctor>(limitFunctor)), 
                                                       adapter_type(_end, _end, nullptr));
        result->set_self_ptr(result);
        return result;
    }

    /** 
        \brief Transparently traverse a sequence and calls an immutable action.

        Transparently traverse a sequence and calls an immutable action. This method doesn't change the original sequence.

        \param teeFunctor A function to call on each element.

        \note This method produces a lazy evaluation relinx_object.

        \return A relinx_object that contains exectly the same elements from the input sequence.
    */
    template<typename TeeFunctor>
    auto tee(TeeFunctor &&teeFunctor) noexcept
    {
        using adapter_type = tee_iterator_adapter<iterator_type, std::function<void(const value_type&)>>;
        using next_relinx_type = relinx_object<self_type, adapter_type, ContainerType>;

        auto no_op = [](const value_type&) { };
        
        auto result = std::make_shared<next_relinx_type>(_self_ptr, 
                                                       adapter_type(_begin, _end, std::forward<TeeFunctor>(teeFunctor)), 
                                                       adapter_type(_end, _end, no_op));
        result->set_self_ptr(result);
        return result;
    }

    /**
        \brief Converts a current relinx_object to the specified container type.

        Converts a current relinx_object to the specified container type. A container must support construction from two forward iterators.

        \return A container that contains all elements from a current relinx_object.
    */
    template<typename AnyContainerType>
    auto to_container() const noexcept
    {
        return AnyContainerType(_begin, _end);
    }

    /**
        \brief Converts a current relinz_object to the std::list container.

        Converts a current relinz_object to the std::list container.

        \return A std::list conatainer that contains all elements from a current relinx_object.
    */
    auto to_list() const noexcept
    {
        return to_container<std::list<value_type>>();
    }

    /**
        \brief Creates a std::string object that contains stringified version of all elements a current relinx_object contains.

        Creates a std::string object that contains stringified version of all elements a current relinx_object contains.

        \param delimiter A string delimiter to insert between elements.

        \return A std::string object that contains stringified version of all elements a current relinx_object contains.
    */
    auto to_string(const std::string &delimiter = std::string()) const noexcept
    {
        std::ostringstream oss;
        auto begin = _begin;
        auto end = _end;

        if (begin != end) { oss << *begin; while (++begin != end) oss << delimiter << *begin; }

        return oss.str();
    }

    /**
        \brief Converts a current relinz_object to the std::vector container.

        Converts a current relinz_object to the std::vector container.

        \return A std::vector conatainer that contains all elements from a current relinx_object.
    */
    auto to_vector() const noexcept
    {
        return to_container<std::vector<value_type>>();
    }
    
    /**
     * \brief Creates a map from a sequence according to specified key selector and element selector functions.
     * 
     * Creates a map from a sequence according to specified key selector and element selector functions.
     * 
     * \param keySelectorFunctor A function to extract a key from each element.
     * \param valueSelectorFunctor A transform function to produce a result element value from each element.
     * 
     * \return A map that contains values selected from the input sequence.
     */
    template<typename KeySelectorFunctor,
             typename ValueSelectorFunctor = std::function<value_type(const value_type&)>,
             template <typename, typename> class MapType = default_map>
    auto to_map(KeySelectorFunctor &&keySelectorFunctor, ValueSelectorFunctor &&valueSelectorFunctor = [](const value_type &v) { return v; }) const noexcept
    {
        using key_type = typename std::decay<decltype(keySelectorFunctor(value_type()))>::type;
        using new_value_type = typename std::decay<decltype(valueSelectorFunctor(value_type()))>::type;

        MapType<key_type, new_value_type> result;
        auto begin = _begin;
        auto end = _end;

        while (begin != end)
        {
            result.emplace(keySelectorFunctor(*begin), valueSelectorFunctor(*begin));
            ++begin;
        }

        return result;
    }
    
    /**
     * \brief Creates a multimap from a sequence according to specified key selector and element selector functions.
     * 
     * Creates a multimap from a sequence according to specified key selector and element selector functions.
     * 
     * \param keySelectorFunctor A function to extract a key from each element.
     * \param valueSelectorFunctor A transform function to produce a result element value from each element.
     * 
     * \return A multimap that contains values selected from the input sequence.
     */
    template<typename KeySelectorFunctor,
             typename ValueSelectorFunctor = std::function<value_type(const value_type&)>,
             template <typename, typename> class MultimapType = default_multimap>
    auto to_multimap(KeySelectorFunctor &&keySelectorFunctor, ValueSelectorFunctor &&valueSelectorFunctor = [](const value_type &v) { return v; }) const noexcept
    {
        return to_map<KeySelectorFunctor,
                     ValueSelectorFunctor,
                     MultimapType>(std::forward<KeySelectorFunctor>(keySelectorFunctor), std::forward<ValueSelectorFunctor>(valueSelectorFunctor));
    }
    
    /**
     * \brief Returns the elements of the specified sequence or the specified value in a singleton collection if the sequence is empty.
     * 
     * Returns the elements of the specified sequence or the specified value in a singleton collection if the sequence is empty.
     * 
     * \param default_value The value to return if the sequence is empty.
     * 
     * \return A sequence that contains default_value if source is empty; otherwise, source.
     */
    auto default_if_empty(value_type default_value = value_type()) noexcept
    {
        if (_begin == _end)
        {
            // Create a container with the default value and return from it
            default_container<value_type> values;
            values.push_back(default_value);
            
            // Return a new relinx_object with this container
            using next_relinx_type = relinx_object<void, typename default_container<value_type>::iterator>;
            
            auto result = std::make_shared<next_relinx_type>(std::shared_ptr<void>(nullptr), std::move(values));
            result->set_self_ptr(result);
            return result;
        }
        
        // If not empty, return the current object (no type inconsistency)
        return _self_ptr;
    }
    
    /**
     * \brief Filters the elements of a sequence based on a specified polymorphic type.
     * 
     * Filters the elements of a sequence based on a specified polymorphic type.
     * 
     * \return A sequence that contains elements from the input sequence of the specified type.
     */
    template<PolymorphicPointerFor CastType>
    auto of_type() noexcept
    {
        return where([](auto &&v) { return dynamic_cast<CastType>(v) != nullptr; })
               ->select([](auto &&v) { return static_cast<CastType>(v); });
    }
    
    /**
     * \brief Filters the elements of a sequence based on a specified type (std::any specialization).
     * 
     * Filters the elements of a sequence based on a specified type for std::any elements.
     * 
     * \return A sequence that contains elements from the input sequence of the specified type.
     */
    template<typename CastType> 
    requires std::is_same_v<typename iterator_type::value_type, std::any>
    auto of_type() noexcept
    {
        return where([](auto &&v) { return std::any_cast<CastType>(&v) != nullptr; })
               ->select([](auto &&v) { return std::any_cast<CastType>(v); });
    }
    
    /**
     * \brief Filters the elements of a sequence based on a specified type (std::variant specialization).
     * 
     * Filters the elements of a sequence based on a specified type for std::variant elements.
     * 
     * \return A sequence that contains elements from the input sequence of the specified type.
     */
    template<typename CastType> 
    requires SpecializationOf<typename iterator_type::value_type, std::variant>
    auto of_type() noexcept
    {
        return where([](auto &&v) { return std::holds_alternative<CastType>(v); })
               ->select([](auto &&v) { return std::get<CastType>(v); });
    }
    
    /**
     * \brief Projects each element of a sequence into a new form with index information.
     * 
     * Projects each element of a sequence into a new form with index information.
     * 
     * \param transformFunctor A transform function to apply to each source element and its index.
     * 
     * \note This method produces a lazy evaluation relinx_object.
     * 
     * \return A sequence whose elements are the result of invoking the transform function on each element of source with its index.
     */
    template<typename TransformFunctor>
    auto select_i(TransformFunctor &&transformFunctor) noexcept
    {
        return select([indexer = 0, transformFunctor = std::forward<TransformFunctor>(transformFunctor)](auto &&v) mutable { 
            return transformFunctor(v, indexer++); 
        });
    }
    
    /**
     * \brief Bypasses elements in a sequence as long as a specified condition is true and then returns the remaining elements, with index information.
     * 
     * Bypasses elements in a sequence as long as a specified condition is true and then returns the remaining elements, with index information.
     * 
     * \param skipFunctor A function to test each source element for a condition; the second parameter represents the element's index.
     * 
     * \note This method modifies the current relinx_object by advancing its iterator.
     * 
     * \return A sequence that contains the elements from the input sequence starting at the first element
     *         in the linear series that does not pass the test specified by predicate.
     */
    template<typename SkipFunctor>
    auto skip_while_i(SkipFunctor &&skipFunctor) noexcept
    {
        uint64_t indexer = 0;
        
        while (_begin != _end && skipFunctor(*_begin, indexer++))
            ++_begin;
        
        return _self_ptr;
    }
    
    /**
     * \brief Computes the sum of a sequence of values.
     * 
     * Computes the sum of a sequence of values.
     * 
     * \return The sum of the values in the sequence.
     * 
     * \throws no_elements if the sequence is empty.
     */
    auto sum() const noexcept
    {
        if (_begin == _end)
            throw no_elements("sum"s);
        
        auto begin = _begin;
        auto sum_value = *begin;
        ++begin;
        
        for (; begin != _end; ++begin)
            sum_value = sum_value + *begin;
        
        return sum_value;
    }
    
    /**
     * \brief Computes the sum of the sequence of values that are obtained by invoking a transform function on each element of the input sequence.
     * 
     * Computes the sum of the sequence of values that are obtained by invoking a transform function on each element of the input sequence.
     * 
     * \param selectFunctor A transform function to apply to each element.
     * 
     * \return The sum of the projected values.
     * 
     * \throws no_elements if the sequence is empty.
     */
    template<typename SelectFunctor>
    auto sum(SelectFunctor &&selectFunctor) const noexcept
    {
        if (_begin == _end)
            throw no_elements("sum"s);
        
        auto begin = _begin;
        auto sum_value = selectFunctor(*begin);
        ++begin;
        
        for (; begin != _end; ++begin)
            sum_value = sum_value + selectFunctor(*begin);
        
        return sum_value;
    }
    
    /**
     * \brief Returns elements from a sequence as long as a specified condition is true, with index information.
     * 
     * Returns elements from a sequence as long as a specified condition is true, and then skips the remaining elements, with index information.
     * 
     * \param limitFunctor A function to test each element for a condition; the second parameter represents the element's index.
     * 
     * \note This method produces a lazy evaluation relinx_object.
     * 
     * \return A sequence that contains the elements from the input sequence that occur before the element at which the test no longer passes.
     */
    template<typename LimitFunctor>
    auto take_while_i(LimitFunctor &&limitFunctor) noexcept
    {
        using adapter_type = limit_iterator_adapter<iterator_type, std::function<bool(const value_type&)>>;
        using next_relinx_type = relinx_object<self_type, adapter_type, ContainerType>;
        
        uint64_t indexer = 0;
        
        auto result = std::make_shared<next_relinx_type>(_self_ptr, 
                                                     adapter_type(_begin, _end, [_indexer = 0, limitFunctor = std::forward<LimitFunctor>(limitFunctor)](auto &&v) mutable { return limitFunctor(v, _indexer++); }), 
                                                     adapter_type(_end, _end, nullptr));
        result->set_self_ptr(result);
        return result;
    }
    
    /**
     * \brief Determines whether two sequences are equal by comparing their elements by using a specified compare functor.
     * 
     * Determines whether two sequences are equal by comparing their elements by using a specified compare functor.
     * 
     * \param container A sequence to compare to.
     * \param compareFunctor A compare functor to use to compare elements.
     * 
     * \return true if the two source sequences are of equal length and their corresponding elements are equal according to the compare functor; otherwise, false.
     */
    template<typename Container, typename CompareFunctor>
    auto sequence_equal(Container &&container, CompareFunctor &&compareFunctor) const noexcept
    {
        return std::equal(_begin, _end, std::begin(container), std::forward<CompareFunctor>(compareFunctor));
    }
    
    /**
     * \brief Determines whether two sequences are equal by comparing the elements by using the default equality comparer for their type.
     * 
     * Determines whether two sequences are equal by comparing the elements by using the default equality comparer for their type.
     * 
     * \param container A sequence to compare to.
     * 
     * \return true if the two source sequences are of equal length and their corresponding elements are equal according to the default equality comparer for their type; otherwise, false.
     */
    template<typename Container>
    auto sequence_equal(Container &&container) const noexcept
    {
        return std::equal(_begin, _end, std::begin(container));
    }
    
    /**
     * \brief Determines whether no elements of a sequence satisfy a condition.
     * 
     * Determines whether no elements of a sequence satisfy a condition.
     * 
     * \param conditionFunctor A function to test each element for a condition.
     * 
     * \return true if no elements in the source sequence pass the test in the specified predicate; otherwise, false.
     */
    template<typename ConditionFunctor>
    auto none(ConditionFunctor &&conditionFunctor) const noexcept
    {
        return std::none_of(_begin, _end, std::forward<ConditionFunctor>(conditionFunctor));
    }

    /**
        \brief Returns a cycled sequence.

        The cycle member functor can return sequence in two different ways. When the 'times' parameter value is positive then it tells
        how many times to cycle the whole sequence. In this case, the resulted sequence size will be the initial input sequence size multiplied by 'times' value.
        When the 'times' parameter value is negative then the output sequence has the infinite size.
        This way, you should limit the output sequence size by any limiting member function followed by the 'cycle' member functor.
        But in this case, a limiting member functor (like \ref take member function) indicates the number of elements to put into the resulted sequence.

        \attention Do not use any of non-lazy or finalizing member functions (in infinite cycle mode) prior to any limiting member function since it causes infinite loop!
        Member functions that do not rely on a finite sequence are relatively safe to use. Be careful, cycle is infinite by default!

        \param times A number of times it has to cycle the whole input sequence. A negative value indicates to cycle forever. Be careful, cycle is infinite by default!

        \note This method produces a lazy evaluation relinx_object.

        \return A relinx_object that is cycled.
    */
    auto cycle(std::ptrdiff_t times = -1) noexcept
    {
        using adapter_type = cycle_iterator_adapter<decltype(_begin)>;
        using next_relinx_type = relinx_object<self_type, adapter_type, ContainerType>;

        auto result = std::make_shared<next_relinx_type>(_self_ptr, 
                                                adapter_type(_begin, _end, times), 
                                                adapter_type(_end, _end, times));
        result->set_self_ptr(result);
        return result;
    }

    /** \brief Produces the set difference of two sequences. The set difference is the members of the first sequence that don't appear in the second sequence.

        \param container A container whose elements that also occur in the current relinx_object's sequence will cause those elements to be removed from the resulted relinx_object.
        \param compareFunctor A functor to compare values. By default, elements are compared using type's operator ==.

        \return A relinx_object that contains the set difference of the elements of two sequences.
    */
    template<typename Container>
    auto except(Container &&container, std::function<bool(const value_type&, const value_type&)> &&compareFunctor = [](auto &&a, auto &&b) { return a == b; }) noexcept
    {
        using adapter_type = except_iterator_adapter<iterator_type, decltype(std::begin(container)), std::function<bool(const value_type&, const value_type&)>>;
        using next_relinx_type = relinx_object<self_type, adapter_type, ContainerType>;

        auto end_it = std::end(container);

        auto result = std::make_shared<next_relinx_type>(_self_ptr, 
                                                      adapter_type(_begin, _end, std::begin(container), end_it, std::forward<std::function<bool(const value_type&, const value_type&)>>(compareFunctor)), 
                                                      adapter_type(_end, _end, end_it, end_it, nullptr));
        result->set_self_ptr(result);
        return result;
    }

    template<typename T>
    auto except(std::initializer_list<T> &&container, std::function<bool(const value_type&, const value_type&)> &&compareFunctor = [](auto &&a, auto &&b) { return a == b; }) noexcept
    {
        return except<std::initializer_list<T>>(std::forward<std::initializer_list<T>>(container), std::forward<std::function<bool(const value_type&, const value_type&)>>(compareFunctor));
    }

    /** \brief Iterates over the current relinx_object elements and calls a user-defined functor for each element.

        Iterates over the current relinx_object elements and calls a user-defined functor for each element.

        \param foreachFunctor A functor that is called for each element.
    */
    template<typename ForeachFunctor>
    auto for_each(ForeachFunctor &&foreachFunctor) const noexcept -> void
    {
        for (auto begin = _begin, end = _end; begin != end; ++begin) foreachFunctor(*begin);
    }

    /** \brief Iterates over the current relinx_object elements and calls a user-defined functor for each element.

        Iterates over the current relinx_object elements and calls a user-defined functor for each element.
        The foreachFunctor must return boolean. Returning false from it will break the loop.

        \param foreachFunctor A functor that is called for each element.
    */
    template<typename ForeachFunctor>
    auto for_each_breakable(ForeachFunctor &&foreachFunctor) const noexcept -> void
    {
        for (auto begin = _begin, end = _end; begin != end; ++begin) if (!foreachFunctor(*begin)) break;
    }

    /** \brief Iterates over the current relinx_object elements and calls a user-defined functor for each element.

        Iterates over the current relinx_object elements and calls a user-defined functor for each element along with the element index.

        \param foreachFunctor A functor that is called for each element.
    */
    template<typename ForeachFunctor>
    auto for_each_i(ForeachFunctor &&foreachFunctor) const noexcept -> void
    {
        uint64_t _indexer { 0 };

        for (auto begin = _begin, end = _end; begin != end; ++begin) foreachFunctor(*begin, _indexer++);
    }

    /** \brief Iterates over the current relinx_object elements and calls a user-defined functor for each element.

        Iterates over the current relinx_object elements and calls a user-defined functor for each element along with the element index.
        The foreachFunctor must return boolean. Returning false from it will break the loop.

        \param foreachFunctor A functor that is called for each element.
    */
    template<typename ForeachFunctor>
    auto for_each_i_breakable(ForeachFunctor &&foreachFunctor) const noexcept -> void
    {
        uint64_t _indexer { 0 };

        for (auto begin = _begin, end = _end; begin != end; ++begin) if (!foreachFunctor(*begin, _indexer++)) break;
    }

    /** \brief Correlates the elements of two sequences based on key equality, and groups the results.

        Correlates the elements of two sequences based on key equality, and groups the results.

        \param container A data container that supports iteration.
        \param thisKeyFunctor A key selector functor for the current relinx_object.
        \param otherKeyFunctor A key selector functor for the container (you supply it as the first parameter to the group_join).
        \param resultFunctor A functor that returns a required result. It takes two parameters: the current element and a joined result.
        \param compareFunctor A compare functor that returns whether two keys are equal or not.
        \param leftJoin If true, then it keeps elements that are not matched in join operation.

        \note This method produces a lazy evaluation relinx_object.

        \return A relinx_object that contains joined and grouped elements.
    */
    template<typename Container, typename ThisKeyFunctor, typename OtherKeyFunctor, typename ResultFunctor, typename CompareFunctor>
    auto group_join(Container &&container, ThisKeyFunctor &&thisKeyFunctor, OtherKeyFunctor &&otherKeyFunctor, ResultFunctor &&resultFunctor, CompareFunctor &&compareFunctor, bool leftJoin = false) noexcept
    {
        using joinIteratorType = typename std::decay<decltype(std::begin(container))>::type;
        using joinType = typename std::decay<decltype(*joinIteratorType())>::type;
        using thisKeyType = typename std::decay<decltype(thisKeyFunctor(value_type()))>::type;
        using otherKeyType = typename std::decay<decltype(otherKeyFunctor(joinType()))>::type;
        using resultType = typename std::decay<decltype(resultFunctor(value_type(), default_container<joinType>()))>::type;
        using adapter_type = group_join_iterator_adapter<iterator_type,
                                                  joinIteratorType,
                                                  std::function<thisKeyType(const value_type&)>,
                                                  std::function<otherKeyType(const joinType&)>,
                                                  std::function<resultType(const value_type&, const default_container<joinType>&)>,
                                                  std::function<bool(const thisKeyType&, const otherKeyType&)>>;
        using next_relinx_type = relinx_object<self_type, adapter_type, ContainerType>;

        auto end_it = std::end(container);

        auto result = std::make_shared<next_relinx_type>(_self_ptr,
                                        adapter_type
                                            (
                                                _begin,
                                                _end,
                                                std::begin(container),
                                                end_it,
                                                std::forward<ThisKeyFunctor>(thisKeyFunctor),
                                                std::forward<OtherKeyFunctor>(otherKeyFunctor),
                                                std::forward<ResultFunctor>(resultFunctor),
                                                std::forward<CompareFunctor>(compareFunctor),
                                                leftJoin
                                             ),
                                        adapter_type
                                             (
                                                _end,
                                                _end,
                                                end_it,
                                                end_it,
                                                nullptr,
                                                nullptr,
                                                nullptr,
                                                nullptr,
                                                leftJoin
                                             ));
        result->set_self_ptr(result);
        return result;
    }

    template<typename T, typename ThisKeyFunctor, typename OtherKeyFunctor, typename ResultFunctor, typename CompareFunctor>
    auto group_join(std::initializer_list<T> &&container, ThisKeyFunctor &&thisKeyFunctor, OtherKeyFunctor &&otherKeyFunctor, ResultFunctor &&resultFunctor, CompareFunctor &&compareFunctor, bool leftJoin = false) noexcept
    {
        return group_join<std::initializer_list<T>,
                            ThisKeyFunctor,
                            OtherKeyFunctor,
                            ResultFunctor,
                            CompareFunctor>(
                                    std::forward<std::initializer_list<T>>(container),
                                    std::forward<ThisKeyFunctor>(thisKeyFunctor),
                                    std::forward<OtherKeyFunctor>(otherKeyFunctor),
                                    std::forward<ResultFunctor>(resultFunctor),
                                    std::forward<CompareFunctor>(compareFunctor),
                                    leftJoin);
    }

    /** \brief Correlates the elements of two sequences based on key equality (using operator == as a comparator), and groups the results.

        Correlates the elements of two sequences based on key equality, and groups the results.

        \param container A data container that supports iteration.
        \param thisKeyFunctor A key selector functor for the current relinx_object.
        \param otherKeyFunctor A key selector functor for the container (you supply it as the first parameter to the group_join).
        \param resultFunctor A functor that returns a required result. It takes two parameters: the current element and a joined result.
        \param leftJoin If true, then it keeps elements that are not matched in join operation.

        \note This method produces a lazy evaluation relinx_object.

        \return A relinx_object that contains joined and grouped elements.
    */
    template<typename Container, typename ThisKeyFunctor, typename OtherKeyFunctor, typename ResultFunctor>
    auto group_join(Container &&container, ThisKeyFunctor &&thisKeyFunctor, OtherKeyFunctor &&otherKeyFunctor, ResultFunctor &&resultFunctor, bool leftJoin = false) noexcept
    {
        return group_join(std::forward<Container>(container),
                    std::forward<ThisKeyFunctor>(thisKeyFunctor),
                    std::forward<OtherKeyFunctor>(otherKeyFunctor),
                    std::forward<ResultFunctor>(resultFunctor),
                    [](auto &&a, auto &&b) { return a == b; },
                    leftJoin);
    }

    template<typename T, typename ThisKeyFunctor, typename OtherKeyFunctor, typename ResultFunctor>
    auto group_join(std::initializer_list<T> &&container, ThisKeyFunctor &&thisKeyFunctor, OtherKeyFunctor &&otherKeyFunctor, ResultFunctor &&resultFunctor, bool leftJoin = false) noexcept
    {
        return group_join<std::initializer_list<T>,
                            ThisKeyFunctor,
                            OtherKeyFunctor,
                            ResultFunctor>(
                                    std::forward<std::initializer_list<T>>(container),
                                    std::forward<ThisKeyFunctor>(thisKeyFunctor),
                                    std::forward<OtherKeyFunctor>(otherKeyFunctor),
                                    std::forward<ResultFunctor>(resultFunctor),
                                    leftJoin);
    }

    /** \brief Produces the set intersection of two sequences.

        Produces the set intersection of two sequences.

        \param container A data container that supports iteration.
        \param compareFunctor A functor that compares elements. This parameter is defaulted to functor that uses operator == to compare elements.

        \note This method produces a lazy evaluation relinx_object.

        \return A relinx_object that holds a set of intersected elements.
    */
    template<typename Container>
    auto intersect_with(Container &&container, std::function<bool(const value_type&, const value_type&)> &&compareFunctor = [](auto &&a, auto &&b) { return a == b; }) noexcept
    {
        using adapter_type = intersect_iterator_adapter<iterator_type, decltype(std::begin(container)), std::function<bool(const value_type&, const value_type&)>>;
        using next_relinx_type = relinx_object<self_type, adapter_type, ContainerType>;

        auto end_it = std::end(container);

        auto result = std::make_shared<next_relinx_type>(_self_ptr, 
                                                     adapter_type(_begin, _end, std::begin(container), end_it, std::forward<std::function<bool(const value_type&, const value_type&)>>(compareFunctor)), 
                                                     adapter_type(_end, _end, end_it, end_it, nullptr));
        result->set_self_ptr(result);
        return result;
    }

    template<typename T>
    auto intersect_with(std::initializer_list<T> &&container, std::function<bool(const value_type&, const value_type&)> &&compareFunctor = [](auto &&a, auto &&b) { return a == b; }) noexcept
    {
        return intersect_with<std::initializer_list<T>>(std::forward<std::initializer_list<T>>(container), std::forward<std::function<bool(const value_type&, const value_type&)>>(compareFunctor));
    }

    /** \brief Correlates the elements of two sequences based on matching keys.

        Correlates the elements of two sequences based on matching keys.

        \param container A data container that supports iteration.
        \param thisKeyFunctor A key selector functor for the current relinx_object.
        \param otherKeyFunctor A key selector functor for the container (you supply it as the first parameter to the join).
        \param resultFunctor A functor that returns a required result. It takes two matched elements as a parameters.
        \param compareFunctor A compare functor that returns whether two keys are equal or not.
        \param leftJoin If true, then it keeps elements that are not matched in join operation.

        \note This method produces a lazy evaluation relinx_object.

        \return A relinx_object that contains joined elements.
    */
    template<typename Container, typename ThisKeyFunctor, typename OtherKeyFunctor, typename ResultFunctor, typename CompareFunctor>
    auto join(Container &&container, ThisKeyFunctor &&thisKeyFunctor, OtherKeyFunctor &&otherKeyFunctor, ResultFunctor &&resultFunctor, CompareFunctor &&compareFunctor, bool leftJoin = false) noexcept
    {
        using joinIteratorType = typename std::decay<decltype(std::begin(container))>::type;
        using joinType = typename std::decay<decltype(*joinIteratorType())>::type;
        using thisKeyType = typename std::decay<decltype(thisKeyFunctor(value_type()))>::type;
        using otherKeyType = typename std::decay<decltype(otherKeyFunctor(joinType()))>::type;
        using resultType = typename std::decay<decltype(resultFunctor(value_type(), joinType()))>::type;

        using adapter_type = join_iterator_adapter<iterator_type,
                                            joinIteratorType,
                                            std::function<thisKeyType(const value_type&)>,
                                            std::function<otherKeyType(const joinType&)>,
                                            std::function<resultType(const value_type&, const joinType&)>,
                                            std::function<bool(const thisKeyType&, const otherKeyType&)>>;
        using next_relinx_type = relinx_object<self_type, adapter_type, ContainerType>;

        auto end_it = std::end(container);

        auto result = std::make_shared<next_relinx_type>(_self_ptr,
                                     adapter_type
                                            (
                                                _begin,
                                                _end,
                                                std::begin(container),
                                                end_it,
                                                std::forward<ThisKeyFunctor>(thisKeyFunctor),
                                                std::forward<OtherKeyFunctor>(otherKeyFunctor),
                                                std::forward<ResultFunctor>(resultFunctor),
                                                std::forward<CompareFunctor>(compareFunctor),
                                                leftJoin
                                             ),
                                     adapter_type
                                             (
                                                _end,
                                                _end,
                                                end_it,
                                                end_it,
                                                nullptr,
                                                nullptr,
                                                nullptr,
                                                nullptr,
                                                leftJoin
                                             ));
        result->set_self_ptr(result);
        return result;
    }

    template<typename T, typename ThisKeyFunctor, typename OtherKeyFunctor, typename ResultFunctor, typename CompareFunctor>
    auto join(std::initializer_list<T> &&container, ThisKeyFunctor &&thisKeyFunctor, OtherKeyFunctor &&otherKeyFunctor, ResultFunctor &&resultFunctor, CompareFunctor &&compareFunctor, bool leftJoin = false) noexcept
    {
        return join<std::initializer_list<T>,
                    ThisKeyFunctor,
                    OtherKeyFunctor,
                    ResultFunctor,
                    CompareFunctor>
                        (std::forward<std::initializer_list<T>>(container),
                        std::forward<ThisKeyFunctor>(thisKeyFunctor),
                        std::forward<OtherKeyFunctor>(otherKeyFunctor),
                        std::forward<ResultFunctor>(resultFunctor),
                        std::forward<CompareFunctor>(compareFunctor),
                        leftJoin);
    }

    /** \brief This is the overloaded version of join that uses operator == to compare elements

        This is the overloaded version of join that uses operator == to compare elements

        \note See join.
    */
    template<typename Container, typename ThisKeyFunctor, typename OtherKeyFunctor, typename ResultFunctor>
    auto join(Container &&container, ThisKeyFunctor &&thisKeyFunctor, OtherKeyFunctor &&otherKeyFunctor, ResultFunctor &&resultFunctor, bool leftJoin = false) noexcept
    {
        return join(std::forward<Container>(container),
                    std::forward<ThisKeyFunctor>(thisKeyFunctor),
                    std::forward<OtherKeyFunctor>(otherKeyFunctor),
                    std::forward<ResultFunctor>(resultFunctor),
                    [](auto &&a, auto &&b) { return a == b; },
                    leftJoin);
    }

    template<typename T, typename ThisKeyFunctor, typename OtherKeyFunctor, typename ResultFunctor>
    auto join(std::initializer_list<T> &&container, ThisKeyFunctor &&thisKeyFunctor, OtherKeyFunctor &&otherKeyFunctor, ResultFunctor &&resultFunctor, bool leftJoin = false) noexcept
    {
        return join<std::initializer_list<T>,
                    ThisKeyFunctor,
                    OtherKeyFunctor,
                    ResultFunctor>
                        (std::forward<std::initializer_list<T>>(container),
                        std::forward<ThisKeyFunctor>(thisKeyFunctor),
                        std::forward<OtherKeyFunctor>(otherKeyFunctor),
                        std::forward<ResultFunctor>(resultFunctor),
                        leftJoin);
    }

    /** \brief Projects each element of a sequence to a container and flattens the resulting sequences into one sequence.

        The select_many method enumerates the input sequence, uses a transform function to map each element to a container,
        and then enumerates and yields the elements of each such container object.
        That is, for each element of source, selector is invoked and a sequence of values is returned.
        select_many then flattens this two-dimensional \ref relinx_object of containers into a one-dimensional \ref relinx_object and returns it.

        \param containerSelectorFunctor A transform functor to apply to each element.

        \note This method produces a lazy evaluation relinx_object.

        \return A \ref relinx_object whose elements are the result of invoking the one-to-many transform function on each element of the input sequence.
    */
    template<typename ContainerSelectorFunctor>
    auto select_many(ContainerSelectorFunctor &&containerSelectorFunctor) noexcept
    {
        using cont_type = decltype(containerSelectorFunctor(value_type()));
        using adapter_type = select_many_iterator_adapter<iterator_type, std::function<cont_type(const value_type&)>>;
        using next_relinx_type = relinx_object<self_type, adapter_type, ContainerType>;

        auto result = std::make_shared<next_relinx_type>(_self_ptr, 
                                                     adapter_type(_begin, _end, std::forward<ContainerSelectorFunctor>(containerSelectorFunctor)), 
                                                     adapter_type(_end, _end, nullptr));
        result->set_self_ptr(result);
        return result;
    }

    /** \brief Projects each element of a sequence to a container and flattens the resulting sequences into one sequence.

        This metod is similar to \ref select_many method except for the first parameter of the container selector functor that is the current index of an element.

        \param containerSelectorFunctor A transform functor to apply to each element. It takes two input parameters: the current index of an element and the element itself.

        \note This method produces a lazy evaluation relinx_object.

        \return An relinx_object whose elements are the result of invoking the one-to-many transform function on each element of the input sequence.
    */
    template<typename ContainerSelectorFunctor>
    auto select_many_i(ContainerSelectorFunctor &&containerSelectorFunctor) noexcept
    {
        return select_many([_indexer = 0, &containerSelectorFunctor](auto &&v) mutable { return containerSelectorFunctor(std::forward<decltype(v)>(v), _indexer++); });
    }

    /** \brief Produces the set union of two sequences.

        Produces the set union of two sequences.

        \param container A container whose distinct elements form elements of the current relinx_object set for the union.
        \param keyFunctor A key provider functor. This way items distinguished by a key. By defualt, the key functor returns each contained item as a key.

        \note This method produces a lazy evaluation relinx_object.

        \return A relinx_object that contains the elements from both input sequences, excluding duplicates.
    */
    template<typename Container, typename KeyFunctor = std::function<value_type(const value_type &)>>
    auto union_with(Container &&container, KeyFunctor &&keyFunctor = [](auto &&v) { return v; }) noexcept
    {
        return concat(std::forward<Container>(container))->distinct(std::forward<KeyFunctor>(keyFunctor));
    }

    template<typename T>
    auto union_with(std::initializer_list<T> &&container) noexcept
    {
        return union_with<std::initializer_list<T>>(std::forward<std::initializer_list<T>>(container));
    }

    /** \brief The method merges each element of the first sequence with an element that has the same index in the second sequence.

         The method merges each element of the first sequence with an element that has the same index in the second sequence. If the sequences do not have the same number of elements, the method merges sequences until it reaches the end of one of them. For example, if one sequence has three elements and the other one has four, the result sequence will have only three elements.

        \param container A container with sequence to merge to.
        \param resultFunctor A functor that gets two elements as input and specifies how to merge two elements from two sequences. Usually, the return type of a functor is the std::pair of two elements.

        \note This method produces a lazy evaluation relinx_object.

     \return A new merged sequence of elements of type what the resultFunctor return type is.
    */
    template<typename Container, typename ResultFunctor>
    auto zip(Container &&container, ResultFunctor &&resultFunctor) noexcept
    {
        using container_iterator_type = decltype(std::begin(container));
        using container_type = typename std::decay<decltype(*std::begin(container))>::type;
        using ret_type = decltype(resultFunctor(value_type(), container_type()));
        using adapter_type = zip_iterator_adapter<iterator_type, container_iterator_type, std::function<ret_type(const value_type&, const container_type&)>>;
        using next_relinx_type = relinx_object<self_type, adapter_type, ContainerType>;

        auto end_it = std::end(container);

        auto result = std::make_shared<next_relinx_type>(_self_ptr, 
                                                     adapter_type(_begin, _end, std::begin(container), end_it, std::forward<ResultFunctor>(resultFunctor)), 
                                                     adapter_type(_end, _end, end_it, end_it, nullptr));
        result->set_self_ptr(result);
        return result;
    }

    template<typename T, typename ResultFunctor>
    auto zip(std::initializer_list<T> &&container, ResultFunctor &&resultFunctor) noexcept
    {
        return zip<std::initializer_list<T>, ResultFunctor>(std::forward<std::initializer_list<T>>(container), std::forward<ResultFunctor>(resultFunctor));
    }

    /**
     * \brief Returns the first element of a sequence.
     * 
     * Returns the first element of a sequence. Throws exception if sequence contains no elements.
     * 
     * \return The first element in the sequence.
     * 
     * \throws no_elements if the sequence is empty.
     */
    auto first() const
    {
        if (_begin == _end) throw no_elements("first"s);
        
        return *_begin;
    }
    
    /**
     * \brief Returns the first element in a sequence that satisfies a specified condition.
     * 
     * Returns the first element in a sequence that satisfies a specified condition.
     * Throws exception if no such element is found.
     * 
     * \param conditionFunctor A function to test each element for a condition.
     * 
     * \return The first element in the sequence that passes the test.
     * 
     * \throws not_found if no element satisfies the condition.
     */
    template<typename ConditionFunctor>
    auto first(ConditionFunctor &&conditionFunctor) const
    {
        auto it = std::find_if(_begin, _end, std::forward<ConditionFunctor>(conditionFunctor));
        
        if (it == _end) throw not_found("first"s);
        
        return *it;
    }
    
    /**
     * \brief Returns the first element of a sequence, or a default value if the sequence contains no elements.
     * 
     * Returns the first element of a sequence, or a default value if the sequence contains no elements.
     * 
     * \return The first element in the sequence, or default value if sequence is empty.
     */
    auto first_or_default() const
    {
        if (_begin == _end) return value_type{};
        
        return *_begin;
    }
    
    /**
     * \brief Returns the first element in a sequence that satisfies a condition or a default value if no such element is found.
     * 
     * Returns the first element in a sequence that satisfies a condition or a default value if no such element is found.
     * 
     * \param conditionFunctor A function to test each element for a condition.
     * 
     * \return The first element in the sequence that passes the test, or default value if no element is found.
     */
    template<typename ConditionFunctor>
    auto first_or_default(ConditionFunctor &&conditionFunctor) const
    {
        auto it = std::find_if(_begin, _end, std::forward<ConditionFunctor>(conditionFunctor));
        
        if (it == _end) return value_type{};
        
        return *it;
    }
    
    /**
     * \brief Returns the last element of a sequence.
     * 
     * Returns the last element of a sequence. Throws exception if sequence contains no elements.
     * 
     * \return The last element in the sequence.
     * 
     * \throws no_elements if the sequence is empty.
     */
    auto last() const
    {
        if (_begin == _end) throw no_elements("last"s);
        
        auto lastIt = _last([](auto&&) { return true; });
        
        return *lastIt;
    }
    
    /**
     * \brief Returns the last element in a sequence that satisfies a specified condition.
     * 
     * Returns the last element in a sequence that satisfies a specified condition.
     * Throws exception if no such element is found.
     * 
     * \param conditionFunctor A function to test each element for a condition.
     * 
     * \return The last element in the sequence that passes the test.
     * 
     * \throws not_found if no element satisfies the condition.
     */
    template<typename ConditionFunctor>
    auto last(ConditionFunctor &&conditionFunctor) const
    {
        auto lastIt = _last(std::forward<ConditionFunctor>(conditionFunctor));
        
        if (lastIt == _end) throw not_found("last"s);
        
        return *lastIt;
    }
    
    /**
     * \brief Returns the last element of a sequence, or a default value if the sequence contains no elements.
     * 
     * Returns the last element of a sequence, or a default value if the sequence contains no elements.
     * 
     * \return The last element in the sequence, or default value if sequence is empty.
     */
    auto last_or_default() const
    {
        if (_begin == _end) return value_type{};
        
        auto lastIt = _last([](auto&&) { return true; });
        
        return *lastIt;
    }
    
    /**
     * \brief Returns the last element in a sequence that satisfies a condition or a default value if no such element is found.
     * 
     * Returns the last element in a sequence that satisfies a condition or a default value if no such element is found.
     * 
     * \param conditionFunctor A function to test each element for a condition.
     * 
     * \return The last element in the sequence that passes the test, or default value if no element is found.
     */
    template<typename ConditionFunctor>
    auto last_or_default(ConditionFunctor &&conditionFunctor) const
    {
        auto lastIt = _last(std::forward<ConditionFunctor>(conditionFunctor));
        
        if (lastIt == _end) return value_type{};
        
        return *lastIt;
    }
    
    /**
     * \brief Returns the element at a specified index in a sequence.
     * 
     * Returns the element at a specified index in a sequence. Throws exception if index is out of range.
     * 
     * \param index The zero-based index of the element to retrieve.
     * 
     * \return The element at the specified position in the sequence.
     * 
     * \throws no_elements if the sequence is empty or index is out of range.
     */
    auto element_at(std::size_t index) const
    {
        if (_begin == _end) throw no_elements("element_at"s);
        
        auto it = _begin;
        for (std::size_t i = 0; i < index && it != _end; ++i, ++it) {}
        
        if (it == _end) throw no_elements("element_at"s);
        
        return *it;
    }
    
    /**
     * \brief Returns the element at a specified index in a sequence or a default value if the index is out of range.
     * 
     * Returns the element at a specified index in a sequence or a default value if the index is out of range.
     * 
     * \param index The zero-based index of the element to retrieve.
     * 
     * \return The element at the specified position in the sequence, or default value if index is out of range.
     */
    auto element_at_or_default(std::size_t index) const
    {
        if (_begin == _end) return value_type{};
        
        auto it = _begin;
        for (std::size_t i = 0; i < index && it != _end; ++i, ++it) {}
        
        if (it == _end) return value_type{};
        
        return *it;
    }
    
    /**
     * \brief Returns the only element of a sequence, and throws an exception if there is not exactly one element in the sequence.
     * 
     * Returns the only element of a sequence, and throws an exception if there is not exactly one element in the sequence.
     * 
     * \return The single element of the input sequence.
     * 
     * \throws no_elements if the sequence is empty.
     * \throws invalid_operation if there is more than one element in the sequence.
     */
    auto single() const
    {
        if (_begin == _end) throw no_elements("single"s);
        
        auto it = _begin;
        auto value = *it;
        
        if (++it != _end) throw invalid_operation("single"s);
        
        return value;
    }
    
    /**
     * \brief Returns the only element of a sequence that satisfies a specified condition.
     * 
     * Returns the only element of a sequence that satisfies a specified condition.
     * Throws exception if no element or more than one element satisfies the condition.
     * 
     * \param conditionFunctor A function to test each element for a condition.
     * 
     * \return The single element of the input sequence that satisfies a condition.
     * 
     * \throws not_found if no element satisfies the condition.
     * \throws invalid_operation if more than one element satisfies the condition.
     */
    template<typename ConditionFunctor>
    auto single(ConditionFunctor &&conditionFunctor) const
    {
        auto begin = _begin;
        auto end = _end;
        
        while (begin != end && !conditionFunctor(*begin)) ++begin;
        
        if (begin == end) throw not_found("single"s);
        
        auto value = *begin;
        ++begin;
        
        while (begin != end)
        {
            if (conditionFunctor(*begin)) throw invalid_operation("single"s);
            ++begin;
        }
        
        return value;
    }
    
    /**
     * \brief Returns the only element of a sequence, or a default value if the sequence is empty.
     * 
     * Returns the only element of a sequence, or a default value if the sequence is empty.
     * This method throws an exception if there is more than one element in the sequence.
     * 
     * \return The single element of the input sequence, or default value if the sequence is empty.
     * 
     * \throws invalid_operation if there is more than one element in the sequence.
     */
    auto single_or_default() const
    {
        if (_begin == _end) return value_type{};
        
        auto it = _begin;
        auto value = *it;
        
        if (++it != _end) throw invalid_operation("single_or_default"s);
        
        return value;
    }
    
    /**
     * \brief Returns the only element of a sequence that satisfies a condition or a default value if no such element exists.
     * 
     * Returns the only element of a sequence that satisfies a condition or a default value if no such element exists.
     * This method throws an exception if more than one element satisfies the condition.
     * 
     * \param conditionFunctor A function to test each element for a condition.
     * 
     * \return The single element of the input sequence that satisfies a condition, or default value if no such element is found.
     * 
     * \throws invalid_operation if more than one element satisfies the condition.
     */
    template<typename ConditionFunctor>
    auto single_or_default(ConditionFunctor &&conditionFunctor) const
    {
        auto begin = _begin;
        auto end = _end;
        
        while (begin != end && !conditionFunctor(*begin)) ++begin;
        
        if (begin == end) return value_type{};
        
        auto value = *begin;
        ++begin;
        
        while (begin != end)
        {
            if (conditionFunctor(*begin)) throw invalid_operation("single_or_default"s);
            ++begin;
        }
        
        return value;
    }
    
    /**
     * \brief Bypasses a specified number of elements in a sequence and then returns the remaining elements.
     * 
     * Bypasses a specified number of elements in a sequence and then returns the remaining elements.
     * 
     * \param count The number of elements to skip before returning the remaining elements.
     * 
     * \note This method modifies the current relinx_object by advancing its iterator.
     * 
     * \return A relinx_object that contains the elements that occur after the specified index in the input sequence.
     */
    auto skip(std::size_t count) noexcept
    {
        while (_begin != _end && count-- > 0) ++_begin;
        
        return _self_ptr;
    }
    
    /**
     * \brief Bypasses elements in a sequence as long as a specified condition is true and then returns the remaining elements.
     * 
     * Bypasses elements in a sequence as long as a specified condition is true and then returns the remaining elements.
     * 
     * \param predicate A function to test each element for a condition.
     * 
     * \note This method modifies the current relinx_object by advancing its iterator.
     * 
     * \return A relinx_object that contains the elements from the input sequence starting at the first element
     *         in the linear series that does not pass the test specified by predicate.
     */
    template<typename Predicate>
    auto skip_while(Predicate &&predicate) noexcept
    {
        while (_begin != _end && predicate(*_begin)) ++_begin;
        
        return _self_ptr;
    }
    
    /**
     * \brief Returns the minimum value in a sequence.
     * 
     * Returns the minimum value in a sequence.
     * 
     * \return The minimum value in the sequence.
     * 
     * \throws no_elements if the sequence is empty.
     */
    auto min() const
    {
        if (_begin == _end) throw no_elements("min"s);
        
        return *std::min_element(_begin, _end);
    }
    
    /**
     * \brief Invokes a transform function on each element of a sequence and returns the minimum resulting value.
     * 
     * Invokes a transform function on each element of a sequence and returns the minimum resulting value.
     * 
     * \param selector A transform function to apply to each element.
     * 
     * \return The minimum value in the sequence of transformed values.
     * 
     * \throws no_elements if the sequence is empty.
     */
    template<typename Selector>
    auto min(Selector &&selector) const
    {
        if (_begin == _end) throw no_elements("min"s);
        
        using result_type = decltype(selector(*_begin));
        
        result_type min_value = selector(*_begin);
        auto it = _begin;
        ++it;
        
        for (; it != _end; ++it)
        {
            auto current = selector(*it);
            if (current < min_value)
                min_value = current;
        }
        
        return min_value;
    }
    
    /**
     * \brief Returns the maximum value in a sequence.
     * 
     * Returns the maximum value in a sequence.
     * 
     * \return The maximum value in the sequence.
     * 
     * \throws no_elements if the sequence is empty.
     */
    auto max() const
    {
        if (_begin == _end) throw no_elements("max"s);
        
        return *std::max_element(_begin, _end);
    }
    
    /**
     * \brief Invokes a transform function on each element of a sequence and returns the maximum resulting value.
     * 
     * Invokes a transform function on each element of a sequence and returns the maximum resulting value.
     * 
     * \param selector A transform function to apply to each element.
     * 
     * \return The maximum value in the sequence of transformed values.
     * 
     * \throws no_elements if the sequence is empty.
     */
    template<typename Selector>
    auto max(Selector &&selector) const
    {
        if (_begin == _end) throw no_elements("max"s);
        
        using result_type = decltype(selector(*_begin));
        
        result_type max_value = selector(*_begin);
        auto it = _begin;
        ++it;
        
        for (; it != _end; ++it)
        {
            auto current = selector(*it);
            if (current > max_value)
                max_value = current;
        }
        
        return max_value;
    }
    
    /**
     * \brief Groups the elements of a sequence according to a specified key selector function.
     * 
     * Groups the elements of a sequence according to a specified key selector function.
     * 
     * \param keySelector A function to extract the key for each element.
     * 
     * \note This method produces a lazy evaluation relinx_object.
     * 
     * \return A relinx_object where each element is a group with a key and a collection of elements.
     */
    template<typename KeySelector>
    auto group_by(KeySelector &&keySelector) noexcept
    {
        using key_type = decltype(keySelector(*_begin));
        
        // Store iterators instead of values for better performance
        default_map<key_type, default_container<iterator_type>> group_map;
        
        auto begin = _begin;
        auto end = _end;
        
        while (begin != end)
        {
            group_map[keySelector(*begin)].emplace_back(begin);
            ++begin;
        }
        
        using next_relinx_type = relinx_object<self_type, decltype(std::begin(group_map)), decltype(group_map)>;
        
        auto result = std::make_shared<next_relinx_type>(_self_ptr, std::move(group_map));
        result->set_self_ptr(result);
        return result;
    }
    
    /**
     * \brief Inverts the order of the elements in a sequence.
     * 
     * Inverts the order of the elements in a sequence.
     * 
     * \note This method produces a lazy evaluation relinx_object.
     * 
     * \return A sequence whose elements correspond to those of the input sequence in reverse order.
     */
    auto reverse() noexcept
    {
        if constexpr (std::is_same<typename iterator_type::iterator_category, std::bidirectional_iterator_tag>::value || 
                      std::is_same<typename iterator_type::iterator_category, std::random_access_iterator_tag>::value)
        {
            using next_relinx_type = relinx_object<self_type, decltype(std::make_reverse_iterator(_begin)), ContainerType>;

            auto result = std::make_shared<next_relinx_type>(_self_ptr, std::make_reverse_iterator(_end), std::make_reverse_iterator(_begin));
            result->set_self_ptr(result);
            return result;
        }
        else
        {
            default_container<value_type> reversed;
            reversed.reserve(std::distance(_begin, _end));
            
            std::copy(_begin, _end, std::back_inserter(reversed));
            std::reverse(reversed.begin(), reversed.end());
            
            using next_relinx_type = relinx_object<self_type, typename default_container<value_type>::iterator, default_container<value_type>>;
            
            auto result = std::make_shared<next_relinx_type>(_self_ptr, std::move(reversed));
            result->set_self_ptr(result);
            return result;
        }
    }
    
    /**
     * \brief Sorts the elements of a sequence in ascending order according to a key.
     *
     * Sorts the elements of a sequence in ascending order according to a key.
     *
     * \param selectFunctor A functor to extract a key from an element.
     * \param sortFunctor A comparison functor that defines the sort order.
     *
     * \return A relinx_object_ordered whose elements are sorted according to a key.
     */
    template<typename SelectFunctor, typename SortFunctor>
    auto order_by(SelectFunctor &&selectFunctor, SortFunctor &&sortFunctor) noexcept
    {
        default_container<value_type> sorted(_begin, _end);
        
        std::sort(std::begin(sorted), std::end(sorted), [&selectFunctor, &sortFunctor](auto &&a, auto &&b) { 
            return sortFunctor(selectFunctor(a), selectFunctor(b)); 
        });
        
        using next_relinx_type = relinx_object_ordered<self_type, typename default_container<value_type>::iterator, SelectFunctor>;
        
        auto result = std::make_shared<next_relinx_type>(_self_ptr, std::move(sorted), std::forward<SelectFunctor>(selectFunctor));
        result->set_self_ptr(result);
        return result;
    }
    
    /**
     * \brief Sorts the elements of a sequence in ascending order according to a key.
     *
     * Sorts the elements of a sequence in ascending order according to a key.
     *
     * \param selectFunctor A functor to extract a key from an element.
     *
     * \return A relinx_object_ordered whose elements are sorted according to a key.
     */
    template<typename SelectFunctor = std::function<value_type(const value_type&)>>
    auto order_by(SelectFunctor &&selectFunctor = [](auto &&v) { return v; }) noexcept
    {
        return order_by(std::forward<SelectFunctor>(selectFunctor), 
                      std::less<typename std::decay<decltype(selectFunctor(value_type()))>::type>());
    }
    
    /**
     * \brief Sorts the elements of a sequence in descending order according to a key.
     *
     * Sorts the elements of a sequence in descending order according to a key.
     *
     * \param selectFunctor A functor to extract a key from an element.
     *
     * \return A relinx_object_ordered whose elements are sorted in descending order according to a key.
     */
    template<typename SelectFunctor = std::function<value_type(const value_type&)>>
    auto order_by_descending(SelectFunctor &&selectFunctor = [](auto &&v) { return v; }) noexcept
    {
        return order_by(std::forward<SelectFunctor>(selectFunctor), 
                      std::greater<typename std::decay<decltype(selectFunctor(value_type()))>::type>());
    }

protected:
    ContainerType _container {};
    ContainerType _def_val_container {};
    mutable value_type _default_value {};

    Iterator _begin;
    Iterator _end;
    std::shared_ptr<ParentRelinxType> _parent_relinx_object_ptr;
    std::shared_ptr<self_type> _self_ptr;

    template<typename ConditionFunctor>
    auto _last(ConditionFunctor &&conditionFunctor) const noexcept
    {
        auto begin = _begin;
        auto end = _end;
        auto lastValueIt = end;

        while (begin != end)
        {
            if (conditionFunctor(*begin)) lastValueIt = begin;

            ++begin;
        }

        return lastValueIt;
    }
};

template<typename ParentRelinxType, typename Iterator, typename PreviousSelectFunctor>
class relinx_object_ordered : public relinx_object<ParentRelinxType, Iterator, default_container<typename std::decay<decltype(*Iterator())>::type>>
{
public:
    using self_type = relinx_object_ordered<ParentRelinxType, Iterator, PreviousSelectFunctor>;
    using value_type = typename std::decay<decltype(*Iterator())>::type;
    using container_type = default_container<value_type>;
    using base = relinx_object<ParentRelinxType, Iterator, container_type>;

    relinx_object_ordered() = delete;
    relinx_object_ordered(const self_type &) = delete;
    auto operator =(const self_type &) -> relinx_object_ordered & = delete;
    relinx_object_ordered(self_type &&) noexcept = default;

    relinx_object_ordered(std::shared_ptr<ParentRelinxType> parent_relinx_object_ptr, default_container<value_type> ordered, PreviousSelectFunctor previousSelectFunctor) noexcept :
        base(std::move(parent_relinx_object_ptr), std::begin(ordered), std::end(ordered)),
        _ordered_values(std::move(ordered)),
        _previousSelectFunctor(std::move(previousSelectFunctor))
        {
        }

    void set_self_ptr(std::shared_ptr<self_type> self_ptr) noexcept {
        this->_self_ptr = std::move(self_ptr);
    }

    ~relinx_object_ordered() noexcept = default;

    auto begin() const noexcept
    {
        return std::begin(_ordered_values);
    }

    auto end() const noexcept
    {
        return std::end(_ordered_values);
    }

    template<typename SelectFunctor, typename SortFunctor>
    auto order_by(SelectFunctor &&selectFunctor, SortFunctor &&sortFunctor)
    {
        std::sort(std::begin(_ordered_values), std::end(_ordered_values), [&selectFunctor, &sortFunctor](auto &&a, auto &&b) { return sortFunctor(selectFunctor(a), selectFunctor(b)); });

        using next_relinx_type = relinx_object_ordered<self_type, Iterator, SelectFunctor>;

        auto result = std::make_shared<next_relinx_type>(
            std::static_pointer_cast<self_type>(this->_self_ptr), 
            std::move(_ordered_values), 
            std::forward<SelectFunctor>(selectFunctor));
        
        result->set_self_ptr(result);
        return result;
    }

    template<typename SelectFunctor = std::function<value_type(const value_type&)>>
    auto order_by(SelectFunctor &&selectFunctor = [](auto &&v) { return v; })
    {
        return order_by(std::forward<SelectFunctor>(selectFunctor), std::less<typename std::decay<decltype(selectFunctor(value_type()))>::type>());
    }

    template<typename SelectFunctor = std::function<value_type(const value_type&)>>
    auto order_by_descending(SelectFunctor &&selectFunctor = [](auto &&v) { return v; })
    {
        return order_by(std::forward<SelectFunctor>(selectFunctor), std::greater<typename std::decay<decltype(selectFunctor(value_type()))>::type>());
    }

    template<typename SelectFunctor, typename SortFunctor>
    auto then_by(SelectFunctor &&selectFunctor, SortFunctor &&sortFunctor)
    {
        auto begin = std::begin(_ordered_values);
        auto end = std::end(_ordered_values);
        decltype(begin) prev_begin;

        while (begin != end)
        {
            const auto &&partition_value = _previousSelectFunctor(*begin);

            prev_begin = begin;

            while (begin != end && _previousSelectFunctor(*begin) == partition_value) ++begin;

            std::sort(prev_begin, begin, [&sortFunctor, &selectFunctor](auto &&a, auto &&b) { return sortFunctor(selectFunctor(a), selectFunctor(b)); });
        }

        using next_relinx_type = relinx_object_ordered<self_type, Iterator, SelectFunctor>;

        auto result = std::make_shared<next_relinx_type>(
            std::static_pointer_cast<self_type>(this->_self_ptr), 
            std::move(_ordered_values), 
            std::forward<SelectFunctor>(selectFunctor));
        
        result->set_self_ptr(result);
        return result;
    }

    template<typename SelectFunctor = std::function<value_type(const value_type&)>>
    auto then_by(SelectFunctor &&selectFunctor = [](auto &&v) { return v; })
    {
        return then_by(std::forward<SelectFunctor>(selectFunctor), std::less<typename std::decay<decltype(selectFunctor(value_type()))>::type>());
    }

    template<typename SelectFunctor = std::function<value_type(const value_type&)>>
    auto then_by_descending(SelectFunctor &&selectFunctor = [](auto &&v) { return v; })
    {
        return then_by(std::forward<SelectFunctor>(selectFunctor), std::greater<typename std::decay<decltype(selectFunctor(value_type()))>::type>());
    }

protected:
    default_container<value_type> _ordered_values;
    PreviousSelectFunctor _previousSelectFunctor;
};

template<typename Container>
auto from(const Container &c)
{
    using next_relinx_type = relinx_object<void, typename std::decay<decltype(std::begin(c))>::type>;

    auto result = std::make_shared<next_relinx_type>(std::shared_ptr<void>(nullptr), std::begin(c), std::end(c));
    result->set_self_ptr(result);
    return result;
}

template<typename Container>
auto from(Container &c)
{
    const Container &const_alias { c };

    return from<Container>(const_alias);
}

template<typename Container>
auto from(Container &&c)
{
    using next_relinx_type = relinx_object<void, typename std::decay<decltype(std::begin(c))>::type, typename std::decay<decltype(c)>::type>;

    auto result = std::make_shared<next_relinx_type>(std::shared_ptr<void>(nullptr), std::forward<typename std::decay<decltype(c)>::type>(c));
    result->set_self_ptr(result);
    return result;
}

template <typename T>
auto from(const std::initializer_list<T> &i)
{
    return from<std::initializer_list<T>>(i);
}

template <typename T>
auto from(std::initializer_list<T> &i)
{
    return from<std::initializer_list<T>>(i);
}

template <typename T>
auto from(std::initializer_list<T> &&i)
{
    default_container<T> result(std::size(i));

    std::move(std::begin(i), std::end(i), std::begin(result));

    return from<default_container<T>>(std::move(result));
}

template <typename Iterator>
auto from(Iterator &&begin, Iterator &&end)
{
    using next_relinx_type = relinx_object<void, typename std::decay<decltype(begin)>::type>;

    auto result = std::make_shared<next_relinx_type>(std::shared_ptr<void>(nullptr), std::forward<Iterator>(begin), std::forward<Iterator>(end));
    result->set_self_ptr(result);
    return result;
}

template <typename T>
auto from(T *a, std::size_t s)
{
    using next_relinx_type = relinx_object<void, T*>;

    auto result = std::make_shared<next_relinx_type>(std::shared_ptr<void>(nullptr), a, a + s);
    result->set_self_ptr(result);
    return result;
}

/**
    \brief Generates a sequence of elements within a specified range.

    Generates a sequence of elements within a specified range.
    The element's type must support post increment operator (++)

    \param start The value of the first value in the sequence.
    \param count The number of times to repeat the value in the generated sequence.

    \return A relinx_object that contains a generated range.
*/
template<typename T>
auto range(T start, std::size_t count)
{
    default_container<T> c(count);

    std::generate(std::begin(c), std::end(c), [&start](){ return start++; });

    using next_relinx_type = relinx_object<void, decltype(std::begin(c))>;

    auto result = std::make_shared<next_relinx_type>(std::shared_ptr<void>(nullptr), std::move(c));
    result->set_self_ptr(result);
    return result;
}

/**
    \brief Generates a sequence that contains one repeated value.

    Generates a sequence that contains one repeated value.

    \param e The value to be repeated.
    \param count The number of times to repeat the value in the generated sequence.

    \return A relinx_object that contains a repeated value.
*/
template<typename T>
auto repeat(T e, std::size_t count)
{
    default_container<T> c(count);

    std::fill(std::begin(c), std::end(c), e);

    using next_relinx_type = relinx_object<void, decltype(std::begin(c))>;

    auto result = std::make_shared<next_relinx_type>(std::shared_ptr<void>(nullptr), std::move(c));
    result->set_self_ptr(result);
    return result;
}

} // namespace nstd::relinx_self