#pragma once

/*
MIT License
Copyright (c) 2018 - 2019 Arlen Keshabyan (arlen.albert@gmail.com)
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
#include <limits>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <sstream>
#include <regex>

#include "http_request_parser.hpp"
#include "media_types.hpp"
#include "sharp_tcp.hpp"
#include "signal_slot.hpp"
#include "utilities.hpp"

namespace nstd::net
{
using namespace std::string_literals;
using namespace std::string_view_literals;
using namespace nstd::signal_slot;
namespace fs = std::filesystem;

class http_resource_manager
{
public:

    struct request
    {
        std::vector<uint8_t> data;
        http_request_parser parser;
        std::string resource;
        std::string resource_pattern;
        std::smatch match;
        std::shared_ptr<tcp_client> client;
        http_resource_manager *manager;
        bool completed { false };
    };

    struct response
    {
        enum http_status_codes : int
        {
            Continue = 100,
            SwitchingProtocols = 101,
            OK = 200,
            Created = 201,
            Accepted = 202,
            NonAuthoritativeInformation = 203,
            NoContent = 204,
            ResetContent = 205,
            PartialContent = 206,
            MultipleChoices = 300,
            Ambiguous = 300,
            MovedPermanently = 301,
            Moved = 301,
            Found = 302,
            SeeOther = 303,
            RedirectMethod = 303,
            NotModified = 304,
            UseProxy = 305,
            Unused = 306,
            TemporaryRedirect = 307,
            PermanentRedirect = 308,
            BadRequest = 400,
            Unauthorized = 401,
            PaymentRequired = 402,
            Forbidden = 403,
            NotFound = 404,
            MethodNotAllowed = 405,
            NotAcceptable = 406,
            ProxyAuthenticationRequired = 407,
            RequestTimeout = 408,
            Conflict = 409,
            Gone = 410,
            LengthRequired = 411,
            PreconditionFailed = 412,
            RequestEntityTooLarge = 413,
            RequestUriTooLong = 414,
            UnsupportedMediaType = 415,
            RequestedRangeNotSatisfiable = 416,
            ExpectationFailed = 417,
            UpgradeRequired = 426,
            InternalServerError = 500,
            NotImplemented = 501,
            BadGateway = 502,
            ServiceUnavailable = 503,
            GatewayTimeout = 504,
            HttpVersionNotSupported = 505
        };

        inline static std::unordered_map<int, std::string_view> http_status_code_map
        {
            { Continue, "Continue"sv },
            { SwitchingProtocols, "Switching Protocols"sv },
            { OK, "OK"sv },
            { Created, "Created"sv },
            { Accepted, "Accepted"sv },
            { NonAuthoritativeInformation, "Non Authoritative Information"sv },
            { NoContent, "No Content"sv },
            { ResetContent, "Reset Content"sv },
            { PartialContent, "Partial Content"sv },
            { MultipleChoices, "Multiple Choices"sv },
            { Ambiguous, "Ambiguous"sv },
            { MovedPermanently, "Moved Permanently"sv },
            { Moved, "Moved"sv },
            { Found, "Found"sv },
            { SeeOther, "SeeOther"sv },
            { RedirectMethod, "Redirect Method"sv },
            { NotModified, "Not Modified"sv },
            { UseProxy, "Use Proxy"sv },
            { Unused, "Unused"sv },
            { TemporaryRedirect, "Temporary Redirect"sv },
            { PermanentRedirect, "Permanent Redirect"sv },
            { BadRequest, "Bad Request"sv },
            { Unauthorized, "Unauthorized"sv },
            { PaymentRequired, "Payment Required"sv },
            { Forbidden, "Forbidden"sv },
            { NotFound, "Not Found"sv },
            { MethodNotAllowed, "Method Not Allowed"sv },
            { NotAcceptable, "Not Acceptable"sv },
            { ProxyAuthenticationRequired, "Proxy Authentication Required"sv },
            { RequestTimeout, "Request Timeout"sv },
            { Conflict, "Conflict"sv },
            { Gone, "Gone"sv },
            { LengthRequired, "Length Required"sv },
            { PreconditionFailed, "Precondition Failed"sv },
            { RequestEntityTooLarge, "Request Entity Too Large"sv },
            { RequestUriTooLong, "Request Uri TooLong"sv },
            { UnsupportedMediaType, "Unsupported Media Type"sv },
            { RequestedRangeNotSatisfiable, "Requested Range Not Satisfiable"sv },
            { ExpectationFailed, "Expectation Failed"sv },
            { UpgradeRequired, "Upgrade Required"sv },
            { InternalServerError, "Internal Server Error"sv },
            { NotImplemented, "Not Implemented"sv },
            { BadGateway, "Bad Gateway"sv },
            { ServiceUnavailable, "Service Unavailable"sv },
            { GatewayTimeout, "Gateway Timeout"sv },
            { HttpVersionNotSupported, "Http Version Not Supported"sv }
        };

        const std::string NL { "\r\n"s };
        const std::string NLNL { "\r\n\r\n"s };

        response(int response_code)
        {
            if (auto it { http_status_code_map.find(response_code) }; it != std::end(http_status_code_map))
            {
                stream << http_vesion << " " << (*it).first << " " << (*it).second << NL;
            }
        }

        response &add_header(const std::string &header_name, const std::string &header_value)
        {
            headers << header_name << ": " << header_value << NL;

            return *this;
        }

        response &add_raw_header(const std::string &header)
        {
            headers << header << NL;

            return *this;
        }

        response &add_content_type_header(const std::string &media_name, const std::string &encoding = {})
        {
            if (const auto &[exists, it] = media_types::find(media_name); exists)
            {
                headers << "Content-Type: " << (*it).second;

                if (!std::empty(encoding)) headers << "; charset=" << encoding;

                headers << NL;
            }

            return *this;
        }

        typename tcp_client::write_request &prepare_response_data()
        {
            if (std::empty(response_data.buffer))
            {
                std::string out_headers { headers.str() };
                std::string out_content { content.str() };

                if (!std::empty(out_headers)) stream << out_headers;

                auto has_content { !std::empty(out_content) };

                if (has_content) stream << "Content-Length: " << std::size(out_content) << NL;

                stream << NL;

                if (has_content) stream.write(std::data(out_content), std::size(out_content));

                std::string result { stream.str() };

                response_data.buffer.resize(std::size(result));

                std::copy(std::begin(result), std::end(result), std::begin(response_data.buffer));
            }

            return response_data;
        }

        void send_response(std::shared_ptr<tcp_client> client, std::function<void(typename tcp_client::write_result&)> callback = nullptr)
        {
            auto &resp { prepare_response_data() };

            resp.async_write_callback = std::move(callback);

            client->async_write(resp);
        }

        std::ostringstream stream;
        std::ostringstream headers;
        std::ostringstream content;
        typename tcp_client::write_request response_data;
        std::string http_vesion { "HTTP/1.1" };
    };

    using request_ptr = std::shared_ptr<request>;

    http_resource_manager() = default;

    ~http_resource_manager()
    {
        stop();
    }

    void start(const std::string &host, int port)
    {
        _server.start(host, port, [this](const std::shared_ptr<tcp_client>& client)
        {
            client->async_read({65536, [this, client](auto &&res) { on_new_request(client, res); }});

            return true;
        });
    }

    void stop()
    {
        if (is_server_running()) _server.stop();
    }

    bool is_server_running() const
    {
        return _server.is_running();
    }

    void add_route(http_request_parser::http_method_id method, const std::string &pattern, std::function<void(request_ptr)> callback)
    {
        std::scoped_lock lock { _add_route_mutex };

        _regexes.emplace(pattern, std::regex(pattern, std::regex_constants::ECMAScript | std::regex_constants::icase | std::regex_constants::optimize));

        _cons = _signals[method][std::u8string { std::begin(pattern), std::end(pattern) }].connect(std::move(callback));
    }

    void add_status_handler(typename response::http_status_codes status_code, std::function<void(request_ptr)> callback)
    {
        std::scoped_lock lock { _add_route_mutex };

        _cons = _status_signals[status_code].connect(std::move(callback));
    }

    const fs::path &get_root_path() const
    {
        return _root_folder;
    }

    void set_root_path(const fs::path &new_root)
    {
        _root_folder = new_root;
    }

protected:

    tcp_server<> _server;
    std::unordered_map<http_request_parser::http_method_id, signal_set<std::u8string, request_ptr>> _signals;
    std::unordered_map<int, signal<request_ptr>> _status_signals;
    std::unordered_map<std::string, std::regex> _regexes;
    nstd::signal_slot::signal<request_ptr> _not_found_signal;
    std::mutex _add_route_mutex;
    connection_bag _cons;
    fs::path _root_folder { fs::current_path() / "www"s };

    void on_new_request(const std::shared_ptr<tcp_client>& client, typename tcp_client::read_result& res)
    {
        if (res.success)
        {
            http_request_parser p { res.buffer };

            std::string resource { p.get_resource_uri().get_path() };
            int completed { false };

            std::scoped_lock lock { _add_route_mutex };

            for (const auto &sig : _signals[p.get_method()])
            {
                std::string pattern { std::string { std::begin(sig.first), std::end(sig.first) } };
                std::smatch sm;
                const std::regex &reg { _regexes.find(pattern)->second };

                if (std::regex_match(resource, sm, reg))
                {
                    auto req { std::make_shared<request>() };

                    req->data = std::move(res.buffer);
                    req->parser = std::move(p);
                    req->resource = std::move(resource);
                    req->resource_pattern = std::move(pattern);
                    req->match = std::move(sm);
                    req->client = client;
                    req->manager = this;

                    try
                    {
                        sig.second->emit(req);

                        if (completed = req->completed; !completed)
                        {
                            res.buffer = std::move(req->data);
                            p = std::move(req->parser);
                            resource = std::move(req->resource);

                            continue;
                        }
                    }
                    catch(const std::exception &e)
                    {
                        completed = true;
                        response resp { response::InternalServerError };

                        resp.content << "<html><head><title>Internal Server Error</title></head><body><h1>500 Internal Server Error</h1>" <<
                                "<p>" << nstd::utilities::net::html_encode(e.what()) << "</p>" <<
                                "</body></html>";
                        resp.add_content_type_header("html", "utf-8").add_header("Connection", "Closed").send_response(client);
                    }

                    break;
                }
            }

            if (!completed)
            {
                auto req { std::make_shared<request>() };

                req->data = std::move(res.buffer);
                req->parser = std::move(p);
                req->resource = std::move(resource);
                req->client = client;
                req->manager = this;

                _status_signals[response::http_status_codes::NotFound].emit(req);
            }

            client->async_read({65536, [this, client](auto &&res) { on_new_request(client, res); }});
        }
        else
        {
            client->disconnect();
        }
    }
};

}
