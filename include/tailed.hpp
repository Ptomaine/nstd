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
#include <deque>
#include <stdexcept>

namespace nstd
{

template<class value_type, const int tail_limit = 2>
class tailed
{
private:

	std::deque<value_type> _values;

	inline value_type& push()
	{
		_values.push_front(_values.front());
		_values.pop_back();

		return _values.front();
	}

public:
	tailed(tailed &&) = default;
	tailed &operator = (tailed &&) = default;

	tailed() : _values(tail_limit)
	{
		static_assert(tail_limit > 1);
	}

	tailed(const value_type &value) : tailed()
	{
		_values.front() = value;
	}

	tailed(const tailed &copy) : _values(copy._values)
	{
		static_assert(tail_limit > 1);
	}

	const value_type &current() const
	{
		return _values.front();
	}

	const value_type &previous(const int shift = 1) const
	{
		if (shift >= tail_limit) throw std::out_of_range("The index is out of range!");

		return *std::next(std::begin(_values), shift);
	}

	tailed &operator = (const value_type &value)
	{
		push() = value;

		return *this;
	}

	const value_type &operator () () const
	{
		return _values.front();
	}

	const value_type &operator * ()
	{
		return push();
	}

	tailed &operator += (const value_type &value)
	{
		push() += value;

		return *this;
	}

	tailed &operator -= (const value_type &value)
	{
		push() -= value;

		return *this;
	}

	tailed &operator *= (const value_type &value)
	{
		push() *= value;

		return *this;
	}

	tailed &operator /= (const value_type &value)
	{
		push() /= value;

		return *this;
	}

	tailed &operator ++ ()
	{
		push()++;

		return *this;
	}

	tailed operator ++ (int)
	{
		tailed temp(*this);

		push()++;

		return temp;
	}

	tailed &operator -- ()
	{
		push()--;

		return *this;
	}

	tailed operator -- (int)
	{
		tailed temp(*this);

		push()--;

		return temp;
	}

	operator value_type&() const
	{
		return _values.front();
	}

	operator value_type() const
	{
		return _values.front();
	}

	const value_type &operator [](const int index) const
	{
		if (index >= tail_limit) throw std::out_of_range("The index is out of range!");

		return _values[index];
	}

	tailed &operator ~()
	{
		std::fill(_values.begin(), _values.end(), value_type());

		return *this;
	}
};

}
