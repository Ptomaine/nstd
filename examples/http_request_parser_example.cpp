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
    using namespace nstd::net;
    using namespace nstd::relinx;
    using namespace nstd::str;

    std::vector<std::string_view> request_data
    {
        "GET /service/user?a=5&b=\"string%20param\" HTTP/1.1\r\nHost: www.test.com\r\nAccept: *\r\nAuth: private key\r\n\r\nMessage body"sv,
        "POST / HTTP/1.1\r\nHost: foo.com\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: 13\r\n\r\nsay=Hi&to=Mom"sv,
        "PUT /new.html HTTP/1.1\r\nHost: example.com\r\nContent-Type: text/html\r\nContent-Length: 15\r\n\r\n<p>New file</p>"sv,
        "DELETE /file.html HTTP/1.1\r\n\r\nOptional body"sv,
        "CONNECT www.example.com:443 HTTP/1.1"sv,
        "HEAD /index.html"sv,
        "OPTIONS /index.html HTTP/1.1"sv,
        "PATCH /file.txt HTTP/1.1\r\nHost: www.example.com\r\nContent-Type: application/example\r\nIf-Match: \"e0023aa4e\"\r\nContent-Length: 3\r\n\r\nXXX"sv,
        "TRACE /index.html"sv,
        "GET /"sv,

        " "sv,
        "AG"sv,
        "AGRRRRRRR"sv
    };

    for (const auto &data : request_data)
    {
        std::cout << std::endl << "---------------------------------------------------" << std::endl;

        if (http_request_parser p { data })
        {
            std::cout << "Resource:\t" << p.get_resource() << std::endl;
            std::cout << "Content:\t" << p.get_content() << std::endl;
            std::cout << "Method:\t\t" << p.get_method_name() << std::endl;
            std::cout << "Protocol:\t" << p.get_protocol() << std::endl;
            std::cout << "Version:\t" << p.get_version() << std::endl;

            auto resource_uri { p.get_resource_uri() };

            std::cout << "Resource URI:\t" << resource_uri.to_string() << std::endl;
            std::cout << "URI resource:\t" << resource_uri.get_path() << std::endl;
            std::cout << "Headers:\n" << from(p.get_headers())->select([](auto &&p){ auto &[key, value] = p; return compose_string("\t", key, ":\t", value); })->to_string("\n") << std::endl;
            std::cout << "Query params:\n" << from(resource_uri.get_query_parameters())->select([](auto &&p){ auto &[key, value] = p; return compose_string("\t", key, ":\t", value); })->to_string("\n") << std::endl;
        }
        else
        {
            std::cout << "Invalid request data: [" << data.substr(0, 20) << "]..." << std::endl;
        }
    }

    const char *data =
    "--boundary\r\n"
    "Content-Disposition: form-data; name=\"AttachedFile1\"; filename=\"horror-photo-1.jpg\"\r\n"
    "Content-Type: image/jpeg\r\n"
    "\r\n"
    "data1\r\n"
    "--mixed\r\n"
    "--mixed\r\n"
    "data2\r\n"
    "--boundary\r\n"
    "Content-Disposition: form-data; name=\"AttachedFile2\"; filename=\"horror-photo-2.jpg\"\r\n"
    "Content-Type: image/jpeg\r\n"
    "\r\n"
    "data11\r\n"
    "--mixed\r\n"
    "--mixed\r\n"
    "data22\r\n"
    "--boundary--\r\n";

    nstd::net::multipart_form_data mparser;

    auto mdata { mparser.parse_data(data) };

    std::cout << std::endl << "Multipart form data type: '" << mdata[0].headers["Content-Disposition"][""] << "'" << std::endl;
    std::cout << "Multipart form data size: " << std::size(mdata) << std::endl;
    std::cout << "Multipart mixed form data detected: '" << (mdata[1].mixed_content ? "true" : "false") << "'" << std::endl;
    std::cout << "Multipart form data: '" << mdata[1].content << "'" << std::endl;
    std::cout << "Multipart filename: '" << mdata[1].headers["Content-Disposition"]["filename"] << "'" << std::endl;
    std::cout << "Multipart Content-Type: '" << mdata[1].headers["Content-Type"][""] << "'" << std::endl;

    std::cout << std::endl << "exiting..." << std::endl;
    return 0;
}
