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

template<typename Iterator>
generator<typename Iterator::value_type> co_from(Iterator begin, Iterator end)
{
	while (begin != end) { co_yield *begin; ++begin; }
}

template<typename Container>
generator<typename Container::value_type> co_from(const Container &container)
{
	return co_from(std::begin(container), std::end(container));
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

template<typename CoroType, typename ExceptIterator, typename Functor>
generator<CoroType> co_except(generator<CoroType> &&gen, ExceptIterator begin, ExceptIterator end, Functor func)
{
	std::unordered_set<CoroType> processed;

	while (gen.move_next())
	{
		const auto value { gen.current_value() };

		if (processed.contains(value)) continue;

		if (std::find_if(begin, end, [&value, &func](auto &&b) { return func(value, b); } ) != end) continue;

		processed.insert(value);

		co_yield value;
	}
}

template<typename CoroType, typename IntersectIterator, typename Functor>
generator<CoroType> co_intersect(generator<CoroType> &&gen, IntersectIterator begin, IntersectIterator end, Functor func)
{
	std::unordered_set<CoroType> processed;

	while (gen.move_next())
	{
		const auto value { gen.current_value() };

		if (processed.contains(value)) continue;

		if (std::find_if(begin, end, [&value, &func](auto &&b) { return func(value, b); } ) == end) continue;

		processed.insert(value);

		co_yield value;
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




struct no_elements : public std::logic_error { no_elements(const std::string &what) : std::logic_error(what) {} }; ///< The exeption that is thrown when there are no elements in the targeting sequence;
struct not_found : public std::logic_error { not_found(const std::string &what) : std::logic_error(what) {} }; ///< The exeption that is thrown when there are no elements found using a filter functor on the targeting sequence
struct invalid_operation : public std::logic_error { invalid_operation(const std::string &what) : std::logic_error(what) {} }; ///< The exeption that is thrown when there is no complience to the performing action

template<typename CoroType>
class co_relinx_object
{
public:
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
	        if (!_co_relinx_object->_moved) _co_relinx_object->move_next();
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

	bool move_next()
	{
		_moved = _generator.move_next();

		return *_moved;
	}

	CoroType current_value()
	{
		return _generator.current_value();
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

    template<typename ConditionFunctor>
    auto all(ConditionFunctor &&conditionFunctor)
    {
        auto begin { iterator { this } };
        auto end { iterator {} };

        return std::all_of(begin, end, std::forward<ConditionFunctor>(conditionFunctor));
    }

    template<typename ConditionFunctor>
    auto any(ConditionFunctor &&conditionFunctor)
    {
        return std::any_of(begin, end, std::forward<ConditionFunctor>(conditionFunctor));
    }

    auto any()
    {
        auto begin { iterator { this } };
        auto end { iterator {} };

        return !(begin == end);
    }

    template<typename Container>
    auto concat(const Container &container)
    {
    	return co_relinx_object<CoroType>(co_concat(std::move(_generator), co_from(container)));
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

	template<typename Functor>
	auto where(Functor func)
	{
		return co_relinx_object<CoroType>(co_filter(std::move(_generator), func));
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
    auto union_with(const Container &container, Functor func = [](auto &&v) { return v; })
    {
    	return concat(container).distinct(func);
    }

	template<typename Container, typename Functor>
	auto zip(const Container &container, Functor func)
	{
		return co_relinx_object<decltype(func(CoroType(), typename Container::value_type()))>(co_zip(std::move(_generator), co_from(container), func));
	}

private:
	generator<CoroType> _generator;
	std::optional<bool> _moved {};
};

template<typename Iterator>
auto from(Iterator begin, Iterator end)
{
	return co_relinx_object<typename Iterator::value_type>(co_from(begin, end));
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
