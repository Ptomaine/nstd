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
#include <sstream>
#include <string>
#include <string_view>
#include "urdl.hpp"
#include "json.hpp"
#include "relinx.hpp"

int main()
{
    auto [headers, json_str] { nstd::download_url("http://qrng.anu.edu.au/API/jsonI.php?length=1024&type=uint8") };
    auto json { nstd::json::json::parse(json_str) };
    auto rdata { nstd::relinx::from(json[0]["data"])->select([](auto &&v){ return static_cast<uint8_t>(v);})->to_vector() };
    auto data_ptr { std::data(rdata) };

    std::cout << headers << std::endl;
    std::cout << "uint64_t: " << *reinterpret_cast<uint64_t*>(data_ptr) << std::endl; data_ptr += sizeof(uint64_t);
    std::cout << "uint64_t: " << *reinterpret_cast<uint64_t*>(data_ptr) << std::endl; data_ptr += sizeof(uint64_t);
    std::cout << "double: " << *reinterpret_cast<double*>(data_ptr) << std::endl; data_ptr += sizeof(double);
    std::cout << "float: " << *reinterpret_cast<float*>(data_ptr) << std::endl; data_ptr += sizeof(float);
    std::cout << "uint16_t: " << *reinterpret_cast<uint16_t*>(data_ptr) << std::endl; data_ptr += sizeof(uint16_t);
    std::cout << "uint32_t: " << *reinterpret_cast<uint32_t*>(data_ptr) << std::endl; data_ptr += sizeof(uint32_t);
    std::cout << "----------------------------------------------------------------------------------" << std::endl;

    auto [google_headers, html_str] { nstd::download_url("http://www.google.com") };

    std::cout << google_headers << std::endl;
    std::cout << std::string_view(std::data(html_str), 128) << " ..." << std::endl;

    std::cout << "exiting..." << std::endl;

    return 0;
}
