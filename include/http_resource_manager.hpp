#pragma once

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
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <sstream>
#include <regex>

#include "signal_slot.hpp"
#include "http_request_parser.hpp"
#include "sharp_tcp.hpp"
#include "media_types.hpp"

namespace nstd::net
{
using namespace std::string_literals;
using namespace nstd::signal_slot;
namespace fs = std::filesystem;

template <bool UseSSL = false>
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
        std::shared_ptr<tcp_client<UseSSL>> client;
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

        inline static std::unordered_map<int, std::string> http_status_code_map
        {
            { Continue, "Continue"s },
            { SwitchingProtocols, "Switching Protocols"s },
            { OK, "OK"s },
            { Created, "Created"s },
            { Accepted, "Accepted"s },
            { NonAuthoritativeInformation, "Non Authoritative Information"s },
            { NoContent, "No Content"s },
            { ResetContent, "Reset Content"s },
            { PartialContent, "Partial Content"s },
            { MultipleChoices, "Multiple Choices"s },
            { Ambiguous, "Ambiguous"s },
            { MovedPermanently, "Moved Permanently"s },
            { Moved, "Moved"s },
            { Found, "Found"s },
            { SeeOther, "SeeOther"s },
            { RedirectMethod, "Redirect Method"s },
            { NotModified, "Not Modified"s },
            { UseProxy, "Use Proxy"s },
            { Unused, "Unused"s },
            { TemporaryRedirect, "Temporary Redirect"s },
            { PermanentRedirect, "Permanent Redirect"s },
            { BadRequest, "Bad Request"s },
            { Unauthorized, "Unauthorized"s },
            { PaymentRequired, "Payment Required"s },
            { Forbidden, "Forbidden"s },
            { NotFound, "Not Found"s },
            { MethodNotAllowed, "Method Not Allowed"s },
            { NotAcceptable, "Not Acceptable"s },
            { ProxyAuthenticationRequired, "Proxy Authentication Required"s },
            { RequestTimeout, "Request Timeout"s },
            { Conflict, "Conflict"s },
            { Gone, "Gone"s },
            { LengthRequired, "Length Required"s },
            { PreconditionFailed, "Precondition Failed"s },
            { RequestEntityTooLarge, "Request Entity Too Large"s },
            { RequestUriTooLong, "Request Uri TooLong"s },
            { UnsupportedMediaType, "Unsupported Media Type"s },
            { RequestedRangeNotSatisfiable, "Requested Range Not Satisfiable"s },
            { ExpectationFailed, "Expectation Failed"s },
            { UpgradeRequired, "Upgrade Required"s },
            { InternalServerError, "Internal Server Error"s },
            { NotImplemented, "Not Implemented"s },
            { BadGateway, "Bad Gateway"s },
            { ServiceUnavailable, "Service Unavailable"s },
            { GatewayTimeout, "Gateway Timeout"s },
            { HttpVersionNotSupported, "Http Version Not Supported"s }
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

        typename tcp_client<UseSSL>::write_request &prepare_response_data()
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

        void send_response(std::shared_ptr<tcp_client<UseSSL>> client, std::function<void(typename tcp_client<UseSSL>::write_result&)> callback = nullptr)
        {
            auto &resp { prepare_response_data() };

            resp.async_write_callback = std::move(callback);

            client->async_write(resp);
        }

        std::ostringstream stream;
        std::ostringstream headers;
        std::ostringstream content;
        typename tcp_client<UseSSL>::write_request response_data;
        std::string http_vesion { "HTTP/1.1" };
    };

#ifdef SHARP_TCP_USES_OPENSSL
    http_resource_manager(const std::string &cert_file, const std::string &priv_key_file) : _server { cert_file, priv_key_file }
    {
    }
#endif

    using request_ptr = std::shared_ptr<request>;

    void start(const std::string &host, int port)
    {
        _server.start(host, port, [this](const std::shared_ptr<tcp_client<UseSSL>>& client)
        {
            client->async_read({65536, [this, client](auto &&res) { on_new_request(client, res); }});

            return true;
        });
    }

    void add_route(http_request_parser::http_method_id method, const std::string &pattern, std::function<void(request_ptr)> callback)
    {
        std::scoped_lock { _add_route_mutex };

        _regexes.emplace(pattern, std::regex(pattern, std::regex_constants::ECMAScript | std::regex_constants::icase | std::regex_constants::optimize));

        _cons = _signals[method][pattern]->connect(std::move(callback));
    }

    void add_status_handler(typename response::http_status_codes status_code, std::function<void(request_ptr)> callback)
    {
        std::scoped_lock { _add_route_mutex };

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

    tcp_server<UseSSL> _server;
    std::unordered_map<http_request_parser::http_method_id, signal_set<request_ptr>> _signals;
    std::unordered_map<int, signal<request_ptr>> _status_signals;
    std::unordered_map<std::string, std::regex> _regexes;
    nstd::signal_slot::signal<request_ptr> _not_found_signal;
    std::mutex _add_route_mutex;
    connection_bag _cons;
    fs::path _root_folder { fs::current_path() / "www"s };

    void on_new_request(const std::shared_ptr<tcp_client<UseSSL>>& client, const typename tcp_client<UseSSL>::read_result& res)
    {
        if (res.success)
        {
            http_request_parser p { res.buffer };

            std::string resource { p.get_resource_uri().get_path() };
            int completed { false };

            std::scoped_lock { _add_route_mutex };

            for (const auto &sig : _signals[p.get_method()])
            {
                std::string pattern { sig.first };
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

                        if (completed = req->completed; !completed) continue;
                    }
                    catch(const std::exception &e)
                    {
                        completed = true;
                        response resp { response::InternalServerError };

                        resp.content << "<html><head><title>Internal Server Error</title></head><body><h1>500 Internal Server Error</h1>" <<
                                "<p>" << e.what() << "</p>" <<
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
