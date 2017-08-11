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
#include "relinx_generator_uuid.hpp"
#include "relinx_generator_random.hpp"
#include "random_provider_quantum.hpp"

int main()
{
    nstd::from_uuid()->take(5)->for_each([](auto &&u)
    {
        std::cout << u.to_string() << std::endl;
    });

    nstd::from_random()->take(10)->for_each([](auto &&u)
    {
        std::cout << u << std::endl;
    });

    std::cout << std::endl << "From the quantum provider:" << std::endl;
    nstd::from_random<nstd::random_provider_quantum<>>()->take(200)->for_each([](auto &&u)
    {
        std::cout << u << std::endl;
    });

	std::cout << "exitting..." << std::endl;

    return 0;
}
