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

#define URDL_HEADER_ONLY
#define URDL_DISABLE_SSL
#define ASIO_STANDALONE
#define ASIO_HAS_STD_CHRONO

#include "external/asio/asio/include/asio.hpp"
#include "external/urdl/http.hpp"
#include "external/urdl/istream.hpp"
#include "external/urdl/istreambuf.hpp"
#include "external/urdl/option_set.hpp"
#include "external/urdl/read_stream.hpp"
#include "external/urdl/url.hpp"

#include <string>
#include <vector>

namespace nstd
{
    namespace asio = asio;
    namespace urdl = urdl;

   template<typename ContainerType = std::string>
    std::pair<std::string, ContainerType> download_url(const urdl::url &url)
    {
        nstd::asio::io_context io_service;

        nstd::urdl::read_stream stream(io_service);

        stream.open(url);

        std::vector<uint8_t> result;

        while (true)
        {
            char data[1024];
            asio::error_code ec;
            std::size_t length { stream.read_some(nstd::asio::buffer(data), ec) };

            if (ec == nstd::asio::error::eof) break;

            if (ec) throw std::system_error(ec);

            std::copy(reinterpret_cast<uint8_t*>(data), reinterpret_cast<uint8_t*>(data) + length, std::back_inserter(result));
        }

        return std::make_pair(stream.headers(), ContainerType(std::begin(result), std::end(result)));
    }
}
