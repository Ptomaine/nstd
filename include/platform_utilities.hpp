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

#include "platform.hpp"
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace nstd::platform::utilities
{

auto shell_execute(const std::string_view cmd)
{
    constexpr const std::size_t buffer_size { 1024 };
    std::vector<char> buffer(buffer_size);
    std::ostringstream result;
    std::shared_ptr<FILE> pipe { ::popen(std::data(cmd), "r"), ::pclose };

    if (!pipe) throw std::runtime_error("popen() failed!");

    while (!::feof(pipe.get()))
    {
        if (::fgets(std::data(buffer), buffer_size, pipe.get()) != nullptr)
            result << std::data(buffer);
    }

    return result.str();
}


}
