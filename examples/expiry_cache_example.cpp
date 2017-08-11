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

#include <iostream>
#include <string>
#include "expiry_cache.hpp"

using namespace std::string_literals;

struct Item
{
	Item() = default;
	~Item() { std::cout << "Item deleted..." << std::endl; }
};

int main()
{
	using namespace std::chrono_literals;

	nstd::expiry_cache<std::string, Item*> a(800ms);
	a.set_vacuum_idle_period(200ms);
	auto c = a.signal_data_expired.connect([](auto &k, auto &v)
    {
        std::cout << "Key: '" << k << "' expired: " << v << std::endl;
        delete v;
    });

	a.start_auto_vacuum();

	const auto key { "My item"s };
	a.put(key, new Item);

	if (Item* b = nullptr; a.get(key, b))
    {
        std::cout << "Item '" << key << "' is found! :) address: " << b << std::endl;
    }
    else
    {
        std::cout << "Item is not found :(" << std::endl;
    }

	std::cout << "Sleeping for 150 ms..." << std::endl;
	std::this_thread::sleep_for(150ms);

	std::cout << "Container size: " << a.size() << std::endl;

	std::cout << "Sleeping for 1 sec..." << std::endl;
	std::this_thread::sleep_for(1s);

	std::cout << "Container size: " << a.size() << std::endl;

    return 0;
}
