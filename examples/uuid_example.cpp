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
#include "uuid.hpp"

using namespace std::string_literals;
using namespace nstd::uuid::literals;

int main()
{
    auto uuid { nstd::uuid::uuid::generate_random() };

    std::cout << "Generated uuid (dashes/lower case, default): " << uuid.to_string() << std::endl;
    std::cout << "Generated uuid (no dashes/lower case): " << uuid.to_string(false) << std::endl;
    std::cout << "Generated uuid (dashes/upper case): " << uuid.to_string(true, true) << std::endl;
    std::cout << "Generated uuid (no dashes/upper case): " << uuid.to_string(false, true) << std::endl;
    std::cout << "Generated uuid (dashes/upper case/braces): " << uuid.to_string(true, true, true) << std::endl;

    auto uuid_parsed { nstd::uuid::uuid::parse(uuid.to_string(true, true, true)) };

    if (uuid == uuid_parsed)
        std::cout << "Parsed uuid: " << uuid_parsed.to_string() << std::endl;
    else
        std::cout << "Error in parsing uuid " << uuid.to_string(false, true) << std::endl;

    nstd::uuid::uuid null_uuid;

    if (null_uuid.is_null() && null_uuid == nstd::uuid::uuid::parse(std::string(32, '0')) && null_uuid == nstd::uuid::uuid::null_uuid)
        std::cout << "Default constructed uuid: " << null_uuid.to_string() << std::endl;
    else
    {
        std::cout << "Error: the default constructed uuid should be the null value!" << std::endl;
    }

    auto cuuid { "b9264043-8c55-4aa6-b69e-e754ce9368d9"_uuid };

    std::cout << "Literal uuid: " << cuuid.to_string() << std::endl;

    std::cout << "exitting..." << std::endl;

    return 0;
}
