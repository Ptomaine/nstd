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

	tailed() : _values(tail_limit)
	{
	}

	tailed(const value_type &value) : tailed()
	{
		_values.front() = value;
	}

	tailed(const tailed<value_type, tail_limit> &copy) : _values(copy._values)
	{
	}

	value_type &current()
	{
		return _values.front();
	}

	value_type &previous()
	{
		static_assert(tail_limit > 1);

		auto it { std::begin(_values) };

		return std::advance(it, 1), *it;
	}

	tailed &operator = (const value_type &value)
	{
		push() = value;

		return *this;
	}

	value_type &operator () ()
	{
		return _values.front();
	}

	value_type &operator * ()
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

	operator value_type&()
	{
		return _values.front();
	}

	operator value_type() const
	{
		return _values.front();
	}

	value_type &operator [](const int index)
	{
		return _values[index];
	}

	value_type operator [](const int index) const
	{
		return _values[index];
	}

	tailed &operator ~()
	{
		std::fill(_values.begin(), _values.end(), value_type());

		return *this;
	}
};

}
