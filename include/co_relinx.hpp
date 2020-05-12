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

#include <algorithm>
#include <coroutine>
#include <deque>
#include <functional>
#include <iterator>
#include <list>
#include <memory>
#include <numeric>
#include <optional>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <span>
#include <iostream>

namespace nstd::co_relinx
{

using namespace std::literals;

template<typename CoroType>
class generator
{
public:
	struct promise_type;

	using handle = std::coroutine_handle<promise_type>;

	struct promise_type
	{
		CoroType current_value;

		static auto get_return_object_on_allocation_failure()
		{
			return generator{ nullptr };
		}

		auto get_return_object()
		{
			return generator{handle::from_promise(*this)};
		}

		auto initial_suspend()
		{
			return std::suspend_always{};
		}

		auto final_suspend()
		{
			return std::suspend_always{};
		}
		
		void unhandled_exception()
		{
			std::terminate();
		}

		void return_void()
		{
		}

		auto yield_value(CoroType value)
		{
			current_value = value;
			
			return std::suspend_always{};
		}
	};

	bool move_next()
	{
		return _coro ? (_coro.resume(), !_coro.done()) : false;
	}

	CoroType current_value()
	{
		return _coro.promise().current_value;
	}

	generator(generator const&) = delete;
	
	generator(generator && rhs) : _coro(rhs._coro)
	{
		rhs._coro = nullptr;
	}

	~generator()
	{
		if (_coro) _coro.destroy();
	}

private:

	generator(handle h) : _coro(h)
	{
	}

	handle _coro;
};

template<typename Container>
generator<typename Container::value_type> co_from(Container &&container)
{
	auto begin { std::begin(container) };
	auto end { std::end(container) };

	while (begin != end) { co_yield *begin; ++begin; }
}

template<typename Container>
generator<typename Container::value_type> co_from(Container &container)
{
	return co_from(std::begin(container), std::end(container));
}

template<typename Iterator>
generator<typename Iterator::value_type> co_from(Iterator begin, Iterator end)
{
	while (begin != end) { co_yield *begin; ++begin; }
}

template<typename CoroType>
generator<CoroType> co_concat(generator<CoroType> &&gen1, generator<CoroType> &&gen2)
{
	while (gen1.move_next()) co_yield gen1.current_value();
	while (gen2.move_next()) co_yield gen2.current_value();
}

template<typename CoroType, typename Functor>
generator<CoroType> co_filter(generator<CoroType> &&gen, Functor func)
{
	while (gen.move_next())
	{
		auto cv { gen.current_value() };

		if (func(cv)) co_yield cv;
	}
}

template<typename CoroType, typename Functor>
generator<CoroType> co_filter_i(generator<CoroType> &&gen, Functor func)
{
	uint64_t index { 0 };

	while (gen.move_next())
	{
		auto cv { gen.current_value() };

		if (func(cv, index++)) co_yield cv;
	}
}

template<typename CoroType>
generator<CoroType> co_reverse(generator<CoroType> &&gen)
{
	std::deque<CoroType> v;

	while (gen.move_next()) v.push_back(gen.current_value());

	auto begin { std::rbegin(v) };
	auto end { std::rend(v) };

	while (begin != end)
	{
		co_yield *begin;

		++begin;
	}
}

template<typename CoroType>
generator<CoroType> co_skip(generator<CoroType> &&gen, size_t count)
{
	bool good { true };

	if (count > 0)  { while ((good = gen.move_next()) && --count > 0) ; }

	if (good) { while (gen.move_next()) co_yield gen.current_value(); }
}

template<typename CoroType, typename Functor>
generator<CoroType> co_skip_while(generator<CoroType> &&gen, Functor func)
{
	bool good { true };

	while ((good = gen.move_next()) && func(gen.current_value())) ;

	if (good) do { co_yield gen.current_value(); } while (gen.move_next());
}

template<typename CoroType, typename Functor>
generator<CoroType> co_tee(generator<CoroType> &&gen, Functor func)
{
	while (gen.move_next())
	{
		const auto value { gen.current_value() };

		func(value);

		co_yield value;
	}
}

template<typename CoroType, typename Functor>
auto co_transform(generator<CoroType> &&gen, Functor func) -> generator<decltype(func(CoroType()))>
{
	while (gen.move_next()) co_yield func(gen.current_value());
}

template<typename CoroType, typename Functor>
auto co_transform_i(generator<CoroType> &&gen, Functor func) -> generator<decltype(func(CoroType(), 0UL))>
{
	uint64_t index { 0 };

	while (gen.move_next())
	{
		co_yield func(gen.current_value(), index++);
	}
}

template<typename CoroType, typename Functor>
generator<CoroType> co_limit(generator<CoroType> &&gen, Functor func)
{
	while (gen.move_next())
	{
		const auto value { gen.current_value() };

		if (!func(value)) break;

		co_yield value;
	}
}

template<typename CoroType, typename Functor>
generator<CoroType> co_limit_i(generator<CoroType> &&gen, Functor func)
{
	uint64_t index { 0 };

	while (gen.move_next())
	{
		const auto value { gen.current_value() };

		if (!func(value, index++)) break;

		co_yield value;
	}
}

template<typename CoroType, typename Functor>
generator<CoroType> co_distinct(generator<CoroType> &&gen, Functor func)
{
	std::unordered_set<decltype(func(CoroType()))> processed;

	while (gen.move_next())
	{
		const auto value { gen.current_value() };

		if (processed.contains(func(value))) continue;

		co_yield value;

		processed.insert(value);
	}
}

template<typename CoroType, typename Functor>
auto co_select_many(generator<CoroType> &&gen, Functor func) -> generator<decltype(func(typename CoroType::value_type()))>
{
	while (gen.move_next())
	{
		const auto value { gen.current_value() };
		
		for (const auto &v : value) co_yield func(v);
	}
}

template<typename CoroType, typename Functor>
auto co_select_many_i(generator<CoroType> &&gen, Functor func) -> generator<decltype(func(typename CoroType::value_type(), 0UL))>
{
	uint64_t index { 0 };

	while (gen.move_next())
	{
		const auto value { gen.current_value() };
		
		for (const auto &v : value) co_yield func(v, index++);
	}
}

template<typename CoroType, typename Functor>
generator<CoroType> co_except(generator<CoroType> &&gen1, generator<CoroType> &&gen2, Functor func)
{
	std::unordered_set<CoroType> processed;
	std::deque<CoroType> provided;

	while (gen2.move_next()) provided.push_back(gen2.current_value());

	while (gen1.move_next())
	{
		const auto value { gen1.current_value() };

		if (processed.contains(value)) continue;

		auto end { std::end(provided) };

		if (std::find_if(std::begin(provided), end, [&value, &func](auto &&b) { return func(value, b); } ) != end) continue;

		co_yield value;

		processed.insert(value);
	}
}

template<typename CoroType, typename Iterator, typename Functor>
generator<CoroType> co_except(generator<CoroType> &&gen, Iterator begin, Iterator end, Functor func)
{
	std::unordered_set<CoroType> processed;

	while (gen.move_next())
	{
		const auto value { gen.current_value() };

		if (processed.contains(value)) continue;

		if (std::find_if(begin, end, [&value, &func](auto &&b) { return func(value, b); } ) != end) continue;

		co_yield value;

		processed.insert(value);
	}
}

template<typename CoroType, typename Functor>
generator<CoroType> co_intersect(generator<CoroType> &&gen1, generator<CoroType> &&gen2, Functor func)
{
	std::unordered_set<CoroType> processed;
	std::deque<CoroType> provided;

	while (gen2.move_next()) provided.push_back(gen2.current_value());

	while (gen1.move_next())
	{
		const auto value { gen1.current_value() };

		if (processed.contains(value)) continue;

		auto end { std::end(provided) };

		if (std::find_if(std::begin(provided), end, [&value, &func](auto &&b) { return func(value, b); } ) == end) continue;

		co_yield value;

		processed.insert(value);
	}
}

template<typename CoroType, typename Iterator, typename Functor>
generator<CoroType> co_intersect(generator<CoroType> &&gen, Iterator begin, Iterator end, Functor func)
{
	std::unordered_set<CoroType> processed;

	while (gen.move_next())
	{
		const auto value { gen.current_value() };

		if (processed.contains(value)) continue;

		if (std::find_if(begin, end, [&value, &func](auto &&b) { return func(value, b); } ) == end) continue;

		co_yield value;

		processed.insert(value);
	}
}

template<typename CoroType>
generator<CoroType> co_default_if_empty(generator<CoroType> &&gen, CoroType default_value)
{
	if (gen.move_next())
	{
		do
		{
			co_yield gen.current_value();
		} while (gen.move_next());
	}
	else co_yield default_value;
}

template<typename CoroType>
generator<CoroType> co_cycle(generator<CoroType> &&gen, int count)
{
	if (count != 0)
	{
		std::deque<CoroType> save;

		while (gen.move_next())
		{
			if (count == 1)
			{
				co_yield gen.current_value();
			}
			else
			{
				save.push_back(gen.current_value());

				co_yield save.back();
			}
		}

		if (count != 1)
		{
			--count;

			while (true)
			{
				auto begin { std::begin(save)}, current { begin }, end{ std::end(save) };

				while (current != end)
				{
					co_yield *current;

					++current;
				}

				current = begin;

				if (count < 0) continue;

				--count;

				if (count == 0) break;
			}
		}
	}
}

template<typename CoroType>
generator<CoroType> co_take(generator<CoroType> &&gen, size_t limit)
{
	while (gen.move_next() && limit--) co_yield gen.current_value();
}

template<typename CoroType1, typename CoroType2, typename Functor>
auto co_zip(generator<CoroType1> &&gen1, generator<CoroType2> &&gen2, Functor func) -> generator<decltype(func(CoroType1(), CoroType2()))>
{
	while (gen1.move_next() && gen2.move_next()) co_yield func(gen1.current_value(), gen2.current_value());
}

template<typename CoroType>
generator<CoroType> co_range(CoroType start, size_t count)
{
	while (count--) co_yield start++;
}

template<typename CoroType>
generator<CoroType> co_repeat(CoroType value, size_t count)
{
	while (count--) co_yield value;
}

template<typename CoroType, typename JoinIterator, typename ThisKeyFunctor, typename OtherKeyFunctor, typename ResultFunctor, typename CompareFunctor>
auto co_join(generator<CoroType> &&gen, JoinIterator begin, JoinIterator end, ThisKeyFunctor &&thisKeyFunctor, OtherKeyFunctor &&otherKeyFunctor, ResultFunctor &&resultFunctor, CompareFunctor &&compareFunctor, bool leftJoin) -> generator<decltype(resultFunctor(thisKeyFunctor(CoroType()), otherKeyFunctor(typename JoinIterator::value_type())))>
{
    using thisKeyType = typename std::decay<decltype(thisKeyFunctor(CoroType()))>::type;
    using joinValueType = typename JoinIterator::value_type;

    while (gen.move_next())
    {
    	auto value { gen.current_value() };
        auto thisKey = thisKeyFunctor(value);
        auto match = std::find_if(begin, end, [&thisKey, &compareFunctor, &otherKeyFunctor](auto &&v) { return compareFunctor(thisKey, otherKeyFunctor(v)); });
        auto join_value { match == end ? joinValueType() : *match };

        if (match == end && !leftJoin) continue;

        co_yield resultFunctor(value, join_value);
    }
}

template<typename CoroType1, typename CoroType2, typename Container, typename ThisKeyFunctor, typename OtherKeyFunctor, typename ResultFunctor, typename CompareFunctor>
auto co_join(generator<CoroType1> &&gen1, generator<CoroType2> &&gen2, ThisKeyFunctor &&thisKeyFunctor, OtherKeyFunctor &&otherKeyFunctor, ResultFunctor &&resultFunctor, CompareFunctor &&compareFunctor, bool leftJoin) -> generator<decltype(resultFunctor(thisKeyFunctor(CoroType1()), otherKeyFunctor(CoroType2())))>
{
    using thisKeyType = typename std::decay<decltype(thisKeyFunctor(CoroType1()))>::type;

	std::deque<CoroType2> provided;

	while (gen2.move_next()) provided.push_back(gen2.current_value());

	auto begin { std::begin(provided) };
	auto end { std::end(provided) };

    while (gen1.move_next())
    {
    	auto value { gen1.current_value() };
        auto thisKey = thisKeyFunctor(value);
        auto match = std::find_if(begin, end, [&thisKey, &compareFunctor, &otherKeyFunctor](auto &&v) { return compareFunctor(thisKey, otherKeyFunctor(v)); });
        auto join_value { match == end ? CoroType2() : *match };

        if (match == end && !leftJoin) continue;

        co_yield resultFunctor(value, join_value);
    }
}

template<typename CoroType, typename JoinIterator, typename ThisKeyFunctor, typename OtherKeyFunctor, typename ResultFunctor, typename CompareFunctor>
auto co_group_join(generator<CoroType> &&gen, JoinIterator begin, JoinIterator end, ThisKeyFunctor &&thisKeyFunctor, OtherKeyFunctor &&otherKeyFunctor, ResultFunctor &&resultFunctor, CompareFunctor &&compareFunctor, bool leftJoin) -> generator<decltype(resultFunctor(CoroType(), std::deque<typename JoinIterator::value_type>()))>
{
    using thisKeyType = typename std::decay<decltype(thisKeyFunctor(CoroType()))>::type;
    using joinValueType = typename JoinIterator::value_type;

    while (gen.move_next())
    {
    	std::deque<joinValueType> group;
    	auto value { gen.current_value() };
        auto thisKey = thisKeyFunctor(value);

        auto current { begin };

        while (current != end)
        {
        	auto join_value { *current };

        	if (compareFunctor(thisKey, otherKeyFunctor(join_value))) group.emplace_back(join_value);

        	++current;
        }

        if (std::empty(group) && !leftJoin) continue;

        co_yield resultFunctor(value, group);
    }
}

template<typename CoroType1, typename CoroType2, typename JoinIterator, typename ThisKeyFunctor, typename OtherKeyFunctor, typename ResultFunctor, typename CompareFunctor>
auto co_group_join(generator<CoroType1> &&gen1, generator<CoroType2> &&gen2, ThisKeyFunctor &&thisKeyFunctor, OtherKeyFunctor &&otherKeyFunctor, ResultFunctor &&resultFunctor, CompareFunctor &&compareFunctor, bool leftJoin) -> generator<decltype(resultFunctor(CoroType1(), std::deque<CoroType2>()))>
{
    using thisKeyType = typename std::decay<decltype(thisKeyFunctor(CoroType1()))>::type;

	std::deque<CoroType2> provided;

	while (gen2.move_next()) provided.push_back(gen2.current_value());

	auto begin { std::begin(provided) };
	auto end { std::end(provided) };

    while (gen1.move_next())
    {
    	std::deque<CoroType2> group;
    	auto value { gen1.current_value() };
        auto thisKey = thisKeyFunctor(value);

        auto current { begin };

        while (current != end)
        {
        	auto join_value { *current };

        	if (compareFunctor(thisKey, otherKeyFunctor(join_value))) group.emplace_back(join_value);

        	++current;
        }

        if (std::empty(group) && !leftJoin) continue;

        co_yield resultFunctor(value, group);
    }
}





struct no_elements : public std::logic_error { no_elements(const std::string &what) : std::logic_error(what) {} }; ///< The exeption that is thrown when there are no elements in the targeting sequence;
struct not_found : public std::logic_error { not_found(const std::string &what) : std::logic_error(what) {} }; ///< The exeption that is thrown when there are no elements found using a filter functor on the targeting sequence
struct invalid_operation : public std::logic_error { invalid_operation(const std::string &what) : std::logic_error(what) {} }; ///< The exeption that is thrown when there is no complience to the performing action

template<typename CoroType>
class co_relinx_object
{
public:
	using value_type = CoroType;

	friend class iterator;

	class iterator
	{
	public:
	    using self_type = iterator;
	    using value_type = CoroType;
	    using difference_type = std::ptrdiff_t;
	    using pointer = const value_type*;
	    using reference = const value_type&;
	    using iterator_category = std::input_iterator_tag;

	    iterator() = default;
	    iterator(const self_type &) = default;
	    iterator(self_type &&) = default;
	    iterator &operator=(const self_type &) = default;
	    iterator &operator=(self_type &&) = default;

	    iterator(co_relinx_object *object) : _co_relinx_object { object }
	    {
	        if (!_co_relinx_object->_moved)
	        {
	        	if (!_co_relinx_object->move_next()) _co_relinx_object = nullptr;
	        }
	    }

	    auto operator==(const self_type &s) const
	    {
	    	if (!_co_relinx_object && !s._co_relinx_object) return true;
	    	if (_co_relinx_object != s._co_relinx_object) return false;

	        return _co_relinx_object->current_value() == s._co_relinx_object->current_value();
	    }

	    auto operator!=(const self_type &s) const
	    {
	        return !(*this == s);
	    }

	    auto operator*() const -> const value_type
	    {
	        return _co_relinx_object->current_value();
	    }

	    auto operator->() -> const value_type
	    {
	        return *(*this);
	    }

	    auto operator++() -> self_type&
	    {
	        if (!_co_relinx_object->move_next()) _co_relinx_object = nullptr;

	        return *this;
	    }

	    auto operator++(int) -> self_type
	    {
	        auto __tmp = *this;

	        ++(*this);

	        return __tmp;
	    }

	private:
		co_relinx_object *_co_relinx_object { nullptr };
	};

	co_relinx_object(generator<CoroType> &&gen) : _generator{ std::forward<generator<CoroType>>(gen) }
	{
	}

	auto begin()
	{
		return iterator { this };
	}

	auto end()
	{
		return iterator{};
	}

    template<typename AggregateFunctor>
    auto aggregate(AggregateFunctor &&aggregateFunctor)
    {
        auto begin { iterator { this } };
        auto end { iterator {} };

        if (begin == end) throw no_elements("aggregate"s);

        auto front = *begin;

        return std::accumulate(++begin, end, front, std::forward<AggregateFunctor>(aggregateFunctor));
    }

    template<typename SeedType, typename AggregateFunctor>
    auto aggregate(SeedType &&seed, AggregateFunctor &&aggregateFunctor)
    {
        auto begin { iterator { this } };
        auto end { iterator {} };

        if (begin == end) throw no_elements("aggregate"s);

        return std::accumulate(begin, end, std::forward<SeedType>(seed), std::forward<AggregateFunctor>(aggregateFunctor));
    }

    template<typename SeedType, typename AggregateFunctor, typename ResultSelector>
    auto aggregate(SeedType &&seed, AggregateFunctor &&aggregateFunctor, ResultSelector &&resultSelector)
    {
        auto begin { iterator { this } };
        auto end { iterator {} };

        if (begin == end) throw no_elements("aggregate"s);

        return resultSelector(aggregate(std::forward<SeedType>(seed), std::forward<AggregateFunctor>(aggregateFunctor)));
    }

    template<typename Container>
    auto concat(Container &container)
    {
    	return co_relinx_object<CoroType>(co_concat(std::move(_generator), co_from(container)));
    }

    template<typename Container>
    auto concat(Container &&container)
    {
    	return co_relinx_object<CoroType>(co_concat(std::move(_generator), co_from(std::forward<Container>(container))));
    }

    auto concat(std::initializer_list<CoroType> &&container)
    {
    	return co_relinx_object<CoroType>(co_concat(std::move(_generator), co_from(std::forward<std::initializer_list<CoroType>>(container))));
    }

    auto cycle(int count = -1)
    {
    	return co_relinx_object<CoroType>(co_cycle(std::move(_generator), count));
    }

    auto default_if_empty(CoroType default_value = CoroType())
    {
    	return co_relinx_object<CoroType>(co_default_if_empty(std::move(_generator), default_value));
    }

    template<typename Functor = std::function<CoroType(const CoroType&)>>
    auto distinct(Functor func = [](auto &&v) { return v; })
    {
    	return co_relinx_object<CoroType>(co_distinct(std::move(_generator), func));
    }

    template<typename Container>
    auto except(Container &container, std::function<bool(const CoroType&, const CoroType&)> &&compareFunctor = [](auto &&a, auto &&b) { return a == b; })
    {
    	return co_relinx_object<CoroType>(co_except(std::move(_generator), std::begin(container), std::end(container), std::forward<std::function<bool(const CoroType&, const CoroType&)>>(compareFunctor)));
    }    

    template<typename Container>
    auto except(Container &&container, std::function<bool(const CoroType&, const CoroType&)> &&compareFunctor = [](auto &&a, auto &&b) { return a == b; })
    {
    	return co_relinx_object<CoroType>(co_except(std::move(_generator), co_from(std::forward<Container>(container)), std::forward<std::function<bool(const CoroType&, const CoroType&)>>(compareFunctor)));
    }    

    auto except(std::initializer_list<CoroType> &&container, std::function<bool(const CoroType&, const CoroType&)> &&compareFunctor = [](auto &&a, auto &&b) { return a == b; })
    {
    	return co_relinx_object<CoroType>(co_except(std::move(_generator), co_from(std::forward<std::initializer_list<CoroType>>(container)), std::forward<std::function<bool(const CoroType&, const CoroType&)>>(compareFunctor)));
    }    

    template<typename Container>
    auto intersect_with(Container &container, std::function<bool(const CoroType&, const CoroType&)> &&compareFunctor = [](auto &&a, auto &&b) { return a == b; })
    {
    	return co_relinx_object<CoroType>(co_intersect(std::move(_generator), std::begin(container), std::end(container), std::forward<std::function<bool(const CoroType&, const CoroType&)>>(compareFunctor)));
    }    

    template<typename Container>
    auto intersect_with(Container &&container, std::function<bool(const CoroType&, const CoroType&)> &&compareFunctor = [](auto &&a, auto &&b) { return a == b; })
    {
    	return co_relinx_object<CoroType>(co_intersect(std::move(_generator), co_from(std::forward<Container>(container)), std::forward<std::function<bool(const CoroType&, const CoroType&)>>(compareFunctor)));
    }    

    auto intersect_with(std::initializer_list<CoroType> &&container, std::function<bool(const CoroType&, const CoroType&)> &&compareFunctor = [](auto &&a, auto &&b) { return a == b; })
    {
    	return co_relinx_object<CoroType>(co_intersect(std::move(_generator), co_from(std::forward<std::initializer_list<CoroType>>(container)), std::forward<std::function<bool(const CoroType&, const CoroType&)>>(compareFunctor)));
    }    

    template<typename Container, typename ThisKeyFunctor, typename OtherKeyFunctor, typename ResultFunctor, typename CompareFunctor>
    auto join(Container &container, ThisKeyFunctor &&thisKeyFunctor, OtherKeyFunctor &&otherKeyFunctor, ResultFunctor &&resultFunctor, CompareFunctor &&compareFunctor, bool leftJoin = false)
    {
    	return co_relinx_object<decltype(resultFunctor(thisKeyFunctor(CoroType()), otherKeyFunctor(typename Container::value_type())))>(co_join(std::move(_generator), std::begin(container), std::end(container), std::forward<ThisKeyFunctor>(thisKeyFunctor), std::forward<OtherKeyFunctor>(otherKeyFunctor), std::forward<ResultFunctor>(resultFunctor), std::forward<CompareFunctor>(compareFunctor), leftJoin));
    }

    template<typename Container, typename ThisKeyFunctor, typename OtherKeyFunctor, typename ResultFunctor, typename CompareFunctor>
    auto join(Container &&container, ThisKeyFunctor &&thisKeyFunctor, OtherKeyFunctor &&otherKeyFunctor, ResultFunctor &&resultFunctor, CompareFunctor &&compareFunctor, bool leftJoin = false)
    {
    	return co_relinx_object<decltype(resultFunctor(thisKeyFunctor(CoroType()), otherKeyFunctor(typename Container::value_type())))>(co_join(std::move(_generator), co_from(std::forward<Container>(container)), std::forward<ThisKeyFunctor>(thisKeyFunctor), std::forward<OtherKeyFunctor>(otherKeyFunctor), std::forward<ResultFunctor>(resultFunctor), std::forward<CompareFunctor>(compareFunctor), leftJoin));
    }

    template<typename CoroType1, typename ThisKeyFunctor, typename OtherKeyFunctor, typename ResultFunctor, typename CompareFunctor>
    auto join(std::initializer_list<CoroType1> &&container, ThisKeyFunctor &&thisKeyFunctor, OtherKeyFunctor &&otherKeyFunctor, ResultFunctor &&resultFunctor, CompareFunctor &&compareFunctor, bool leftJoin = false)
    {
    	return co_relinx_object<decltype(resultFunctor(thisKeyFunctor(CoroType()), otherKeyFunctor(CoroType1())))>(co_join(std::move(_generator), co_from(std::forward<std::initializer_list<CoroType1>>(container)), std::forward<ThisKeyFunctor>(thisKeyFunctor), std::forward<OtherKeyFunctor>(otherKeyFunctor), std::forward<ResultFunctor>(resultFunctor), std::forward<CompareFunctor>(compareFunctor), leftJoin));
    }

    template<typename ForeachFunctor>
    void for_each(ForeachFunctor &&foreachFunctor)
    {
        auto begin { iterator { this } };
        auto end { iterator {} };

        std::for_each(begin, end, std::forward<ForeachFunctor>(foreachFunctor));
    }

    template<typename ForeachFunctor>
    void for_each_i(ForeachFunctor &&foreachFunctor)
    {
        auto begin { iterator { this } };
        auto end { iterator {} };
        uint64_t indexer = 0;

        std::for_each(begin, end, [&foreachFunctor, &indexer](auto &&v) { foreachFunctor(v, indexer++); });
    }

    template<typename Container, typename ThisKeyFunctor, typename OtherKeyFunctor, typename ResultFunctor, typename CompareFunctor>
    auto group_join(Container &container, ThisKeyFunctor &&thisKeyFunctor, OtherKeyFunctor &&otherKeyFunctor, ResultFunctor &&resultFunctor, CompareFunctor &&compareFunctor, bool leftJoin = false)
    {
    	return co_relinx_object<decltype(resultFunctor(CoroType(), std::deque<typename Container::value_type>()))>(co_group_join(std::move(_generator), std::begin(container), std::end(container), std::forward<ThisKeyFunctor>(thisKeyFunctor), std::forward<OtherKeyFunctor>(otherKeyFunctor), std::forward<ResultFunctor>(resultFunctor), std::forward<CompareFunctor>(compareFunctor), leftJoin));
    }

    template<typename Container, typename ThisKeyFunctor, typename OtherKeyFunctor, typename ResultFunctor, typename CompareFunctor>
    auto group_join(Container &&container, ThisKeyFunctor &&thisKeyFunctor, OtherKeyFunctor &&otherKeyFunctor, ResultFunctor &&resultFunctor, CompareFunctor &&compareFunctor, bool leftJoin = false)
    {
    	return co_relinx_object<decltype(resultFunctor(CoroType(), std::deque<typename Container::value_type>()))>(co_group_join(std::move(_generator), co_from<Container>(std::forward<Container>(container)), std::forward<ThisKeyFunctor>(thisKeyFunctor), std::forward<OtherKeyFunctor>(otherKeyFunctor), std::forward<ResultFunctor>(resultFunctor), std::forward<CompareFunctor>(compareFunctor), leftJoin));
    }

    template<typename CoroType1, typename ThisKeyFunctor, typename OtherKeyFunctor, typename ResultFunctor, typename CompareFunctor>
    auto group_join(std::initializer_list<CoroType1> &&container, ThisKeyFunctor &&thisKeyFunctor, OtherKeyFunctor &&otherKeyFunctor, ResultFunctor &&resultFunctor, CompareFunctor &&compareFunctor, bool leftJoin = false)
    {
    	return co_relinx_object<decltype(resultFunctor(CoroType(), std::deque<CoroType1>()))>(co_group_join(std::move(_generator), co_from(std::forward<std::initializer_list<CoroType1>>(container)), std::forward<ThisKeyFunctor>(thisKeyFunctor), std::forward<OtherKeyFunctor>(otherKeyFunctor), std::forward<ResultFunctor>(resultFunctor), std::forward<CompareFunctor>(compareFunctor), leftJoin));
    }

    auto reverse()
    {
    	return co_relinx_object<CoroType>(co_reverse(std::move(_generator)));
    }

	template<typename Selector>
	auto select(Selector func)
	{
		return co_relinx_object<decltype(func(CoroType()))>(co_transform(std::move(_generator), func));
	}

	template<typename Selector>
	auto select_i(Selector func)
	{
		return co_relinx_object<decltype(func(CoroType(), 0UL))>(co_transform_i(std::move(_generator), func));
	}

	template<typename Selector>
	auto select_many(Selector func)
	{
		return co_relinx_object<decltype(func(typename CoroType::value_type()))>(co_select_many(std::move(_generator), func));
	}

	template<typename Selector>
	auto select_many_i(Selector func)
	{
		return co_relinx_object<decltype(func(typename CoroType::value_type(), 0UL))>(co_select_many_i(std::move(_generator), func));
	}

	template<typename Container, typename CompareFunctor>
    auto sequence_equal(Container &&container, CompareFunctor &&compareFunctor)
    {
        auto begin { iterator { this } };
        auto end { iterator {} };

        return std::equal(begin, end, std::begin(container), std::forward<CompareFunctor>(compareFunctor));
    }

	template<typename Container>
    auto sequence_equal(Container &&container)
    {
        auto begin { iterator { this } };
        auto end { iterator {} };

        return std::equal(begin, end, std::begin(container));
    }

    template<typename ConditionFunctor>
    auto single(ConditionFunctor &&conditionFunctor)
    {
        auto begin { iterator { this } };
        auto end { iterator {} };

        if (begin == end) throw no_elements("single"s);

        auto i = std::find_if(begin, end, conditionFunctor);

        if (i == end) throw not_found("single"s);

        auto result { *i };
        auto n = std::find_if(++i, end, conditionFunctor);

        if (n != end) throw invalid_operation("single"s);

        return result;
    }

    auto single()
    {
        auto begin { iterator { this } };
        auto end { iterator {} };

        if (begin == end) throw no_elements("single"s);

        auto result { *begin };

        if (++begin != end) throw invalid_operation("single"s);

        return result;
    }

    template<typename ConditionFunctor>
    auto single_or_default(ConditionFunctor &&conditionFunctor, CoroType defaultValue = CoroType())
    {
        auto begin { iterator { this } };
        auto end { iterator {} };
        auto i = std::find_if(begin, end, conditionFunctor);

        if (i == end) return defaultValue;

        auto result { *i };
        auto n = std::find_if(++i, end, conditionFunctor);

        if (n != end) throw invalid_operation("single_or_default"s);

        return result;
    }

    auto single_or_default(CoroType defaultValue = CoroType())
    {
        auto begin { iterator { this } };
        auto end { iterator {} };

        if (begin == end) return defaultValue;

        auto result { *begin };

        if (++begin != end) throw invalid_operation("single_or_default"s);

        return result;
    }

    auto skip(size_t skip)
    {
        return co_relinx_object<CoroType>(co_skip(std::move(_generator), skip));
    }    

    template<typename SkipFunctor>
    auto skip_while(SkipFunctor &&skipFunctor)
    {
        return co_relinx_object<CoroType>(co_skip_while(std::move(_generator), skipFunctor));
    }

    template<typename SkipFunctor>
    auto skip_while_i(SkipFunctor &&skipFunctor)
    {
    	uint64_t index { 0 };

  		while (move_next() && skipFunctor(current_value(), index++)) ;

        return co_relinx_object<CoroType>(std::move(_generator), true);
    }

    template<typename SelectFunctor>
    auto sum(SelectFunctor &&selectFunctor)
    {
        auto begin { iterator { this } };
        auto end { iterator {} };

        return std::accumulate(begin, end, decltype(selectFunctor(CoroType()))(), [&selectFunctor](auto &&sum, auto &&v) { return sum + selectFunctor(v); });
    }    

    auto sum()
    {
        return sum([](auto &&v){ return v; });
    }

    auto take(size_t limit)
	{
		return co_relinx_object<CoroType>(co_take(std::move(_generator), limit));
	}

	template<typename Functor>
	auto take_while(Functor func)
	{
		return co_relinx_object<CoroType>(co_limit(std::move(_generator), func));
	}

	template<typename Functor>
	auto take_while_i(Functor func)
	{
		return co_relinx_object<CoroType>(co_limit_i(std::move(_generator), func));
	}

    template<typename TeeFunctor>
    auto tee(TeeFunctor &&teeFunctor)
    {
    	return co_relinx_object<CoroType>(co_tee(std::move(_generator), teeFunctor));
    }	

	template<typename AnyContainerType>
    auto to_container()
    {
        auto begin { iterator { this } };
        auto end { iterator {} };

        return AnyContainerType(begin, end);
    }

    auto to_deque()
    {
        return to_container<std::deque<CoroType>>();
    }

    auto to_list()
    {
        return to_container<std::list<CoroType>>();
    }

    template<   typename KeySelectorFunctor,
                typename ValueSelectorFunctor = std::function<CoroType(const CoroType&)>,
                template <typename, typename> class MapType = std::unordered_map>
    auto to_map(KeySelectorFunctor &&keySelectorFunctor, ValueSelectorFunctor &&valueSelectorFunctor = [](const CoroType &v) { return v; })
    {
        using key_type = typename std::decay<decltype(keySelectorFunctor(CoroType()))>::type;
        using new_value_type = typename std::decay<decltype(valueSelectorFunctor(CoroType()))>::type;

        MapType<key_type, new_value_type> result;
        auto begin { iterator { this } };
        auto end { iterator {} };

        while (begin != end)
        {
            result.emplace(keySelectorFunctor(*begin), valueSelectorFunctor(*begin));

            ++begin;
        }

        return result;
    }

    template<   typename KeySelectorFunctor,
                typename ValueSelectorFunctor = std::function<CoroType(const CoroType&)>,
                template <typename, typename> class MultimapType = std::unordered_multimap>
    auto to_multimap(KeySelectorFunctor &&keySelectorFunctor, ValueSelectorFunctor &&valueSelectorFunctor = [](const CoroType &v) { return v; })
    {
        return to_map<  KeySelectorFunctor,
                        ValueSelectorFunctor,
                        MultimapType>(std::forward<KeySelectorFunctor>(keySelectorFunctor), std::forward<ValueSelectorFunctor>(valueSelectorFunctor));
    }

	auto to_string(const std::string &delimiter = std::string())
    {
        std::ostringstream oss;
        auto begin { iterator { this } };
        auto end { iterator {} };

        if (begin != end) { oss << *begin; while (++begin != end) oss << delimiter << *begin; }

        return oss.str();
    }

    auto to_vector()
    {
        return to_container<std::vector<CoroType>>();
    }

    template<typename Container, typename Functor = std::function<CoroType(const CoroType&)>>
    auto union_with(Container &container, Functor func = [](auto &&v) { return v; })
    {
    	return concat(container).distinct(func);
    }

    template<typename Container, typename Functor = std::function<CoroType(const CoroType&)>>
    auto union_with(Container &&container, Functor func = [](auto &&v) { return v; })
    {
    	return concat(std::forward<Container>(container)).distinct(func);
    }

    template<typename Functor = std::function<CoroType(const CoroType&)>>
    auto union_with(std::initializer_list<CoroType> &&container, Functor func = [](auto &&v) { return v; })
    {
    	return concat(std::forward<std::initializer_list<CoroType>>(container)).distinct(func);
    }

	template<typename Functor>
	auto where(Functor func)
	{
		return co_relinx_object<CoroType>(co_filter(std::move(_generator), func));
	}

	template<typename Functor>
	auto where_i(Functor func)
	{
		return co_relinx_object<CoroType>(co_filter_i(std::move(_generator), func));
	}

	template<typename Container, typename Functor>
	auto zip(Container &container, Functor func)
	{
		return co_relinx_object<decltype(func(CoroType(), typename Container::value_type()))>(co_zip(std::move(_generator), co_from(container), func));
	}

	template<typename Container, typename Functor>
	auto zip(Container &&container, Functor func)
	{
		return co_relinx_object<decltype(func(CoroType(), typename Container::value_type()))>(co_zip(std::move(_generator), co_from(std::forward<Container>(container)), func));
	}

	template<typename CoroType1, typename Functor>
	auto zip(std::initializer_list<CoroType1> &&container, Functor func)
	{
		return co_relinx_object<decltype(func(CoroType(), CoroType1()))>(co_zip(std::move(_generator), co_from(std::forward<std::initializer_list<CoroType1>>(container)), func));
	}

private:
	generator<CoroType> _generator;
	std::optional<bool> _moved {};

	bool move_next()
	{
		_moved = _generator.move_next();

		return *_moved;
	}

	CoroType current_value()
	{
		return _generator.current_value();
	}
};

template<typename Iterator>
auto from(Iterator begin, Iterator end)
{
	return co_relinx_object<typename Iterator::value_type>(co_from(begin, end));
}

template<typename Container>
auto from(Container &&container)
{
	return co_relinx_object<typename Container::value_type>(co_from(std::forward<Container>(container)));
}

template<typename CoroType>
auto from(std::initializer_list<CoroType> &&container)
{
	return co_relinx_object<CoroType>(co_from(std::forward<std::initializer_list<CoroType>>(container)));
}

template<typename Container>
auto from(Container &container)
{
	return from(std::begin(container), std::end(container));
}

template<typename T>
auto range(T start, std::size_t count)
{
    return co_relinx_object<T>(co_range(start, count));
}

template<typename T>
auto repeat(T value, std::size_t count)
{
    return co_relinx_object<T>(co_repeat(value, count));
}

}
