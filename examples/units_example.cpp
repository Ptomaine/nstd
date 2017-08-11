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
#include "units.hpp"

int main()
{
    using namespace nstd::units;
    using namespace nstd::units::values;

    km kms { 50 };
    mile miles { kms };
    yard yards { kms };
    cm cantimeters { kms };
    m meters { kms };

    std::cout << kms << " = " << miles << " = " << yards << " = " << cantimeters << " = " << meters << std::endl;

    Celsius c { 25.5 };
    Fahrenheit f { c };

    std::cout << c << " = " << f << std::endl;

    auto lyrs { 300'000 };
    km flight { constants::lightyear * lyrs };

    std::cout << lyrs << " light years = " << flight << std::endl;

    std::cout << std::endl << "exiting..." << std::endl;

    return 0;
}
