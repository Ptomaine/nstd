/*
MIT License
Copyright (c) 2018 Arlen Keshabyan (arlen.albert@gmail.com)
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
#include <vector>

#include "relinx.hpp"
#include "strings.hpp"
#include "http_request_parser.hpp"

int main()
{
    using namespace std::string_literals;
    using namespace nstd::http;
    using namespace nstd::relinx;
    using namespace nstd::str;

    std::vector<std::string> request_data
    {
        "GET /service/user?a=5&b=\"string%20param\" HTTP/1.1\r\nHost: www.test.com\r\nAccept: *\r\nAuth: private key\r\n\r\nMessage body"s,
        "POST / HTTP/1.1\r\nHost: foo.com\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: 13\r\n\r\nsay=Hi&to=Mom"s,
        "PUT /new.html HTTP/1.1\r\nHost: example.com\r\nContent-Type: text/html\r\nContent-Length: 15\r\n\r\n<p>New file</p>"s,
        "DELETE /file.html HTTP/1.1\r\n\r\nOptional body"s,
        "CONNECT www.example.com:443 HTTP/1.1"s,
        "HEAD /index.html"s,
        "OPTIONS /index.html HTTP/1.1"s,
        "PATCH /file.txt HTTP/1.1\r\nHost: www.example.com\r\nContent-Type: application/example\r\nIf-Match: \"e0023aa4e\"\r\nContent-Length: 3\r\n\r\nXXX"s,
        "TRACE /index.html"s,
        " ",
        "AG",
        "AGRRRRRRR"
    };

    for (auto &data : request_data)
    {
        std::cout << std::endl << "---------------------------------------------------" << std::endl;

        if (http_request_parser p { reinterpret_cast<uint8_t*>(std::data(data)), std::size(data) })
        {
            std::cout << "Resource:\t" << p.get_resource() << std::endl;
            std::cout << "Content:\t" << p.get_content() << std::endl;
            std::cout << "Method:\t\t" << p.get_method_name() << std::endl;
            std::cout << "Protocol:\t" << p.get_protocol() << std::endl;
            std::cout << "Version:\t" << p.get_version() << std::endl;

            auto resource_uri { p.get_resource_uri() };

            std::cout << "Resource URI:\t" << resource_uri.to_string() << std::endl;
            std::cout << "Headers:\n" << from(p.get_headers())->select([](auto &&p){ auto &[key, value] = p; return compose_string("\t", key, ":\t", value); })->to_string("\n") << std::endl;
            std::cout << "Query params:\n" << from(resource_uri.get_query_parameters())->select([](auto &&p){ auto &[key, value] = p; return compose_string("\t", key, ":\t", value); })->to_string("\n") << std::endl;
        }
        else
        {
            std::cout << "Invalid request data: [" << data.substr(0, 20) << "]..." << std::endl;
        }
    }

    std::cout << std::endl << "exiting..." << std::endl;
    return 0;
}
