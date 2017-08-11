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

#include <chrono>
#include <iostream>
#include <vector>
#include "asio.hpp"
#include "json.hpp"
#include "relinx.hpp"

int main()
{
    using namespace std::chrono_literals;

    asio::ip::tcp::iostream s;

    s.expires_after(60s);

    s.connect("qrng.anu.edu.au", "http");
    if (!s)
    {
      std::cout << "Unable to connect: " << s.error().message() << "\n";
      return 1;
    }

    s << "GET /API/jsonI.php?length=1024&type=uint8 HTTP/1.0\r\n";
    s << "Host: qrng.anu.edu.au\r\n";
    s << "Accept: application/json\r\n";
    s << "Connection: close\r\n\r\n";

    std::string http_version;
    s >> http_version;

    unsigned int status_code;
    s >> status_code;

    std::string status_message;
    std::getline(s, status_message);

    if (!s || http_version.substr(0, 5) != "HTTP/")
    {
      std::cout << "Invalid response\n";
      return 1;
    }

    if (status_code != 200)
    {
      std::cout << "Response returned with status code " << status_code << "\n";
      return 1;
    }

    std::string header;

    while (std::getline(s, header) && header != "\r")
      std::cout << header << "\n";

    std::cout << "\n";


    std::ostringstream json_str_stream;
    json_str_stream << s.rdbuf();

    auto json_str { json_str_stream.str() };
    auto json { nstd::json::json::parse(json_str) };

    bool success { json[0]["success"] };

    if (success)
    {
        auto data { nstd::relinx::from(json[0]["data"])->select([](auto &&v){ return static_cast<int>(v); })->to_vector() };

        nstd::relinx::from(data)->for_each([](auto &&v) { std::cout << "/" << v; });

        std::cout << std::endl;
    }

	std::cout << "exitting..." << std::endl;

    return 0;
}
