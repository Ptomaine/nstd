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

#include <cstdio>
#include <memory>
#include <sstream>
#include <string_view>
#include <vector>

#ifdef _WIN32
#if defined(_MSC_VER) && !defined(NOMINMAX)
#define NOMINMAX 1
#endif
#include <windows.h>
#endif // _WIN32

namespace nstd::platform::utilities
{

auto set_console_utf8()
{
#ifdef _WIN32
    if (::GetConsoleCP() != CP_UTF8) ::SetConsoleCP(CP_UTF8);
    if (::GetConsoleOutputCP() != CP_UTF8) ::SetConsoleOutputCP(CP_UTF8);
#endif // _WIN32
}

auto get_console_encoding()
{
#ifdef _WIN32
    return std::make_tuple(::GetConsoleCP(), ::GetConsoleOutputCP());
#endif
}

class scoped_console_utf8
{
#ifdef _WIN32
private:
    std::tuple<UINT, UINT> _code_pages {};
#endif

public:
    scoped_console_utf8()
#ifdef _WIN32
    : _code_pages { get_console_encoding() }
#endif
    {
#ifdef _WIN32
        set_console_utf8();
#endif
    }

    ~scoped_console_utf8()
    {
#ifdef _WIN32
    	const auto&[console_cp, console_output_cp] = _code_pages;

        ::SetConsoleCP(console_cp);
        ::SetConsoleOutputCP(console_output_cp);
#endif
    }
};

auto shell_execute(const std::string_view cmd)
{
    constexpr const std::size_t buffer_size { 1024 };
    std::vector<char> buffer(buffer_size);
    std::ostringstream result;
    std::shared_ptr<FILE> pipe {
#if defined(_MSC_VER)
    ::_popen(std::data(cmd), "r"), ::_pclose
#else
    ::popen(std::data(cmd), "r"), ::pclose
#endif
    };

    if (!pipe) throw std::runtime_error("popen() failed!");

    auto pipe_file { pipe.get() };

    while (!::feof(pipe_file))
    {
        if (::fgets(std::data(buffer), buffer_size, pipe_file) != nullptr)
            result << std::data(buffer);
    }

    return result.str();
}

}
