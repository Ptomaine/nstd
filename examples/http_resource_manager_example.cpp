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

#include <algorithm>
#include <exception>
#include <iostream>
#include <chrono>
#include <ctime>
#include <map>
#include <memory>
#include <csignal>
#include <string>
#include <string_view>
#include <sstream>
#include "http_resource_manager.hpp"
#include "utilities.hpp"

using namespace std::string_literals;

std::condition_variable cv;

void signint_handler(int) { cv.notify_all(); }

using namespace nstd::net;

int main(int argc, char *argv[])
{
    using M = http_request_parser::http_method_id;
    using S = http_resource_manager::response::http_status_codes;

    http_resource_manager mgr;

    mgr.add_route(M::GET, R"(^\/services\/([^/]+)\/([^/]+))", [](auto &&req) // /services/v8/user
	{
		http_resource_manager::response resp { S::OK };
		std::string service_name { req->match[2] };

		resp.content << std::string("<html><body><p>Service: ") + service_name + "</p></body></html>";
		resp.add_content_type_header("html", "utf-8").add_header("Connection", "Closed").send_response(req->client);
	});

    mgr.add_route(M::GET, R"(^\/$)", [](auto &&req) // /
	{
        auto full_path { req->manager->get_root_path() };

        full_path += "/index.html"s;

        if (fs::exists(full_path) && fs::is_regular_file(full_path))
        {
            http_resource_manager::response resp { S::TemporaryRedirect };

            resp.content << "<html><body><p>Redirect link: <a href=\"/index.html\">click here</a></p></body></html>";
            resp.add_content_type_header("html", "utf-8").add_header("Location", "/index.html").send_response(req->client);
        }
        else
        {
            http_resource_manager::response resp { S::OK };

            resp.content << "<html><body><p>Index</p></body></html>";
            resp.add_content_type_header("html", "utf-8").add_header("Connection", "Closed").send_response(req->client);
        }
	});

    mgr.add_route(M::GET, R"(^\/time$)", [](auto &&req) // /time
	{
		http_resource_manager::response resp { S::OK };
		auto cur_time { std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) };

		resp.content << std::string("<html><body><p>") + std::ctime(&cur_time) + "</p></body></html>";
		resp.add_content_type_header("html", "utf-8").add_header("Connection", "Closed").send_response(req->client);
	});

    mgr.add_route(M::GET, R"(^\/throw$)", [](auto &&req) // /throw
	{
		throw std::runtime_error("Test exception");
	});

    mgr.add_status_handler(http_resource_manager::response::http_status_codes::NotFound, [](auto &&req)
	{
        auto full_path { req->manager->get_root_path() / ("."s + req->resource) };
        auto media_type { fs::path { full_path }.extension().string() };

        if (std::size(media_type) > 0 && media_type[0] == '.') media_type = media_type.substr(1);

        const auto &[type_exists, it] = media_types::find(media_type);

        if (fs::exists(full_path) && fs::is_regular_file(full_path) && type_exists)
        {
            http_resource_manager::response resp { S::OK };
            auto data { nstd::utilities::read_file_content(full_path) };

            resp.content.write(std::data(data), std::size(data));
            resp.add_content_type_header(media_type).add_header("Connection", "Closed").send_response(req->client);
        }
        else
        {
            http_resource_manager::response resp { S::NotFound };

            resp.content << "<html><head><title>Not Found</title></head><body><h1>404 Not Found</h1></body></html>";
            resp.add_content_type_header("html", "utf-8").add_header("Connection", "Closed").send_response(req->client);
        }
	});

    std::cout << "Press Ctrl+C to exit..." << std::endl << std::endl;

    mgr.start("127.0.0.1", 3001);

    std::signal(SIGINT, &signint_handler);

    std::mutex mtx;
    std::unique_lock<std::mutex> lock(mtx);

    cv.wait(lock);

    return 0;
}
