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
#include <cstdlib>
#include <exception>
#include <iostream>
#include <limits>
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
using namespace std::string_view_literals;
using namespace nstd::signal_slot;
namespace fs = std::filesystem;

static const std::unordered_map<std::string_view, uint8_t> entities_to_char
{
    { "&quot;"sv, 34 },
    { "&amp;"sv, 38 },
    { "&lt;"sv, 60 },
    { "&gt;"sv, 62 },
    { "&nbsp;"sv, ' ' },
    { "&iexcl;"sv, 161 },
    { "&cent;"sv, 162 },
    { "&pound;"sv, 163 },
    { "&curren;"sv, 164 },
    { "&yen;"sv, 165 },
    { "&brvbar;"sv, 166 },
    { "&sect;"sv, 167 },
    { "&uml;"sv, 168 },
    { "&copy;"sv, 169 },
    { "&ordf;"sv, 170 },
    { "&laquo;"sv, 171 },
    { "&not;"sv, 172 },
    { "&shy;"sv, 173 },
    { "&reg;"sv, 174 },
    { "&macr;"sv, 175 },
    { "&deg;"sv, 176 },
    { "&plusmn;"sv, 177 },
    { "&sup2;"sv, 178 },
    { "&sup3;"sv, 179 },
    { "&acute;"sv, 180 },
    { "&micro;"sv, 181 },
    { "&para;"sv, 182 },
    { "&middot;"sv, 183 },
    { "&cedil;"sv, 184 },
    { "&sup1;"sv, 185 },
    { "&ordm;"sv, 186 },
    { "&raquo;"sv, 187 },
    { "&frac14;"sv, 188 },
    { "&frac12;"sv, 189 },
    { "&frac34;"sv, 190 },
    { "&iquest;"sv, 191 },
    { "&Agrave;"sv, 192 },
    { "&Aacute;"sv, 193 },
    { "&Acirc;"sv, 194 },
    { "&Atilde;"sv, 195 },
    { "&Auml;"sv, 196 },
    { "&ring;"sv, 197 },
    { "&AElig;"sv, 198 },
    { "&Ccedil;"sv, 199 },
    { "&Egrave;"sv, 200 },
    { "&Eacute;"sv, 201 },
    { "&Ecirc;"sv, 202 },
    { "&Euml;"sv, 203 },
    { "&Igrave;"sv, 204 },
    { "&Iacute;"sv, 205 },
    { "&Icirc;"sv, 206 },
    { "&Iuml;"sv, 207 },
    { "&ETH;"sv, 208 },
    { "&Ntilde;"sv, 209 },
    { "&Ograve;"sv, 210 },
    { "&Oacute;"sv, 211 },
    { "&Ocirc;"sv, 212 },
    { "&Otilde;"sv, 213 },
    { "&Ouml;"sv, 214 },
    { "&times;"sv, 215 },
    { "&Oslash;"sv, 216 },
    { "&Ugrave;"sv, 217 },
    { "&Uacute;"sv, 218 },
    { "&Ucirc;"sv, 219 },
    { "&Uuml;"sv, 220 },
    { "&Yacute;"sv, 221 },
    { "&THORN;"sv, 222 },
    { "&szlig;"sv, 223 },
    { "&agrave;"sv, 224 },
    { "&aacute;"sv, 225 },
    { "&acirc;"sv, 226 },
    { "&atilde;"sv, 227 },
    { "&auml;"sv, 228 },
    { "&aring;"sv, 229 },
    { "&aelig;"sv, 230 },
    { "&ccedil;"sv, 231 },
    { "&egrave;"sv, 232 },
    { "&eacute;"sv, 233 },
    { "&ecirc;"sv, 234 },
    { "&euml;"sv, 235 },
    { "&igrave;"sv, 236 },
    { "&iacute;"sv, 237 },
    { "&icirc;"sv, 238 },
    { "&iuml;"sv, 239 },
    { "&ieth;"sv, 240 },
    { "&ntilde;"sv, 241 },
    { "&ograve;"sv, 242 },
    { "&oacute;"sv, 243 },
    { "&ocirc;"sv, 244 },
    { "&otilde;"sv, 245 },
    { "&ouml;"sv, 246 },
    { "&divide;"sv, 247 },
    { "&oslash;"sv, 248 },
    { "&ugrave;"sv, 249 },
    { "&uacute;"sv, 250 },
    { "&ucirc;"sv, 251 },
    { "&uuml;"sv, 252 },
    { "&yacute;"sv, 253 },
    { "&thorn;"sv, 254 },
    { "&yuml;"sv, 255 }
};

static const std::unordered_map<uint8_t, std::string_view> char_to_entities
{
    { 34,  "&quot;"sv },
    { 38,  "&amp;"sv },
    { 60,  "&lt;"sv },
    { 62,  "&gt;"sv },
    { 161, "&iexcl;"sv },
    { 162, "&cent;"sv },
    { 163, "&pound;"sv },
    { 164, "&curren;"sv },
    { 165, "&yen;"sv },
    { 166, "&brvbar;"sv },
    { 167, "&sect;"sv },
    { 168, "&uml;"sv },
    { 169, "&copy;"sv },
    { 170, "&ordf;"sv },
    { 171, "&laquo;"sv },
    { 172, "&not;"sv },
    { 173, "&shy;"sv },
    { 174, "&reg;"sv },
    { 175, "&macr;"sv },
    { 176, "&deg;"sv },
    { 177, "&plusmn;"sv },
    { 178, "&sup2;"sv },
    { 179, "&sup3;"sv },
    { 180, "&acute;"sv },
    { 181, "&micro;"sv },
    { 182, "&para;"sv },
    { 183, "&middot;"sv },
    { 184, "&cedil;"sv },
    { 185, "&sup1;"sv },
    { 186, "&ordm;"sv },
    { 187, "&raquo;"sv },
    { 188, "&frac14;"sv },
    { 189, "&frac12;"sv },
    { 190, "&frac34;"sv },
    { 191, "&iquest;"sv },
    { 192, "&Agrave;"sv },
    { 193, "&Aacute;"sv },
    { 194, "&Acirc;"sv },
    { 195, "&Atilde;"sv },
    { 196, "&Auml;"sv },
    { 197, "&ring;"sv },
    { 198, "&AElig;"sv },
    { 199, "&Ccedil;"sv },
    { 200, "&Egrave;"sv },
    { 201, "&Eacute;"sv },
    { 202, "&Ecirc;"sv },
    { 203, "&Euml;"sv },
    { 204, "&Igrave;"sv },
    { 205, "&Iacute;"sv },
    { 206, "&Icirc;"sv },
    { 207, "&Iuml;"sv },
    { 208, "&ETH;"sv },
    { 209, "&Ntilde;"sv },
    { 210, "&Ograve;"sv },
    { 211, "&Oacute;"sv },
    { 212, "&Ocirc;"sv },
    { 213, "&Otilde;"sv },
    { 214, "&Ouml;"sv },
    { 215, "&times;"sv },
    { 216, "&Oslash;"sv },
    { 217, "&Ugrave;"sv },
    { 218, "&Uacute;"sv },
    { 219, "&Ucirc;"sv },
    { 220, "&Uuml;"sv },
    { 221, "&Yacute;"sv },
    { 222, "&THORN;"sv },
    { 223, "&szlig;"sv },
    { 224, "&agrave;"sv },
    { 225, "&aacute;"sv },
    { 226, "&acirc;"sv },
    { 227, "&atilde;"sv },
    { 228, "&auml;"sv },
    { 229, "&aring;"sv },
    { 230, "&aelig;"sv },
    { 231, "&ccedil;"sv },
    { 232, "&egrave;"sv },
    { 233, "&eacute;"sv },
    { 234, "&ecirc;"sv },
    { 235, "&euml;"sv },
    { 236, "&igrave;"sv },
    { 237, "&iacute;"sv },
    { 238, "&icirc;"sv },
    { 239, "&iuml;"sv },
    { 240, "&ieth;"sv },
    { 241, "&ntilde;"sv },
    { 242, "&ograve;"sv },
    { 243, "&oacute;"sv },
    { 244, "&ocirc;"sv },
    { 245, "&otilde;"sv },
    { 246, "&ouml;"sv },
    { 247, "&divide;"sv },
    { 248, "&oslash;"sv },
    { 249, "&ugrave;"sv },
    { 250, "&uacute;"sv },
    { 251, "&ucirc;"sv },
    { 252, "&uuml;"sv },
    { 253, "&yacute;"sv },
    { 254, "&thorn;"sv },
    { 255, "&yuml;"sv }
};

std::string html_decode(std::string_view data)
{
    std::ostringstream oss;
    auto size { std::size(data) };

    for (decltype(size) pos { 0 }; pos < size; ++pos)
    {
        if (data[pos] == '&')
        {
            auto end_pos { data.find(';', pos) };

            if (end_pos == std::string_view::npos)
            {
                oss << data[pos];

                continue;
            }

            if (auto length { end_pos - pos + 1 }; length > 3 && length <= 8)
            {
                auto entity { data.substr(pos, length) };

                if (entity[1] == '#')
                {
                    int ch { std::atoi(std::string { entity.substr(2, length - 3) }.c_str()) }; 

                    if (ch > 0 && ch <= std::numeric_limits<uint8_t>::max()) oss << static_cast<uint8_t>(ch);
                    else
                    {
                        oss << data[pos];

                        continue;
                    }
                }
                else
                {
                    if (auto it { entities_to_char.find(entity) }; it != std::end(entities_to_char))
                        oss << (*it).second;
                    else
                    {
                        oss << data[pos];

                        continue;
                    }
                }

                pos += length - 1;
            }
            else
            {
                oss << data[pos];

                continue;
            }
        }
        else oss << data[pos];
    }

    return oss.str();
}

std::string html_encode(std::string_view data)
{
    std::ostringstream oss;

    for (auto ch : data)
    {
        if (auto it { char_to_entities.find(static_cast<uint8_t>(ch)) }; it != std::end(char_to_entities))
             oss << (*it).second;
        else oss << ch;
    }

    return oss.str();
}



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

    void start(const std::string &host, int port)
    {
        _server.start(host, port, [this](const std::shared_ptr<tcp_client>& client)
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

    tcp_server<> _server;
    std::unordered_map<http_request_parser::http_method_id, signal_set<request_ptr>> _signals;
    std::unordered_map<int, signal<request_ptr>> _status_signals;
    std::unordered_map<std::string, std::regex> _regexes;
    nstd::signal_slot::signal<request_ptr> _not_found_signal;
    std::mutex _add_route_mutex;
    connection_bag _cons;
    fs::path _root_folder { fs::current_path() / "www"s };

    void on_new_request(const std::shared_ptr<tcp_client>& client, const typename tcp_client::read_result& res)
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
                                "<p>" << html_encode(e.what()) << "</p>" <<
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
