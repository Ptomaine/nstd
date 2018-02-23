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
#include "base64.hpp"
#include "crc32.hpp"
#include "tailed.hpp"

int main()
{
    using namespace std::string_literals;

    auto b64 { nstd::base64::base64_encode("Hello World!"s) };

    std::cout << b64 << std::endl;
    std::cout << nstd::base64::base64_decode<std::string>(b64) << std::endl;

    std::vector<uint8_t> data { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0 };
    auto b64_bin { nstd::base64::base64_encode(data) };

    std::cout << b64_bin << std::endl;

    auto data_restored { nstd::base64::base64_decode(b64_bin) };

    std::cout << "Binary data: ";
    for (auto &&i : data_restored) std::cout << static_cast<int>(i);
    std::cout << std::endl << std::endl;

    std::cout << "  crc32 :" << nstd::crc32::crc32("1234567890", 10) << std::endl;
    std::cout << "c_crc32 :" << nstd::crc32::c_crc32<10>("1234567890") << std::endl;

    nstd::tailed<int, 3> tailed_int { 5 };

    tailed_int = 10;
    ++tailed_int;

    std::cout << tailed_int[0] << ">>" << tailed_int[1] << ">>" << tailed_int[2] << std::endl;

	std::cout << "exitting..." << std::endl;

    return 0;
}
