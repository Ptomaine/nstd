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

#include <string>
#include <string_view>
#include <unordered_map>

#include "platform.hpp"
#include "uri.hpp"

namespace nstd::net
{

class http_request_parser
{
public:
    enum class http_method_id : uint8_t
    {
        CONNECT,
        DELETE,
        GET,
        HEAD,
        OPTIONS,
        PATCH,
        POST,
        PUT,
        TRACE,

        UNKNOWN
    };

    http_request_parser(const std::vector<uint8_t> &request_data) :
        _request_data { std::data(request_data) },
        _request_data_size { std::size(request_data) }
    {
        parse();
    }

    http_request_parser(const std::string_view request_data) :
        _request_data { reinterpret_cast<const uint8_t*>(std::data(request_data)) },
        _request_data_size { std::size(request_data) }
    {
        parse();
    }

    http_request_parser() = default;
    http_request_parser(http_request_parser &&) = default;
    http_request_parser &operator =(http_request_parser &&) = default;
    http_request_parser(const http_request_parser &) = default;
    http_request_parser &operator =(const http_request_parser &) = default;

    void reset(const std::vector<uint8_t> &request_data)
    {
        clear();

        _request_data = std::data(request_data);
        _request_data_size = std::size(request_data);

        parse();
    }

    void reset(const std::string_view request_data)
    {
        clear();

        _request_data = reinterpret_cast<const uint8_t*>(std::data(request_data));
        _request_data_size = std::size(request_data);

        parse();
    }

    bool is_ok() const
    {
        return  _request_data &&
                _request_data_size >= 5 &&
                _http_method_traits.method != http_method_id::UNKNOWN;
    }

    operator bool () const
    {
        return is_ok();
    }

    auto get_resource() const
    {
        return _resource;
    }

    auto get_resource_uri() const
    {
        nstd::uri rurl { std::string { _resource } };

        if (rurl.is_relative())
        {
            auto host { _headers.find("Host") };

            if (host != _headers.end())
            {
                rurl.set_scheme(std::string { _protocol });
                rurl.set_host(std::string { (*host).second });
            }
        }

        if (_http_method_traits.method == http_method_id::POST)
        {
            if (auto content_type { _headers.find("Content-Type") };
                content_type == _headers.end() ||
                content_type->second == "application/x-www-form-urlencoded")
                    rurl.set_raw_query(std::string { _content });
        }

        return rurl;
    }

    auto get_protocol() const
    {
        return _protocol;
    }

    auto get_version() const
    {
        return _version;
    }

    auto get_headers() const
    {
        return _headers;
    }

    auto get_content() const
    {
        return _content;
    }

    auto get_method() const
    {
        return _http_method_traits.method;
    }

    auto get_method_name() const
    {
        return _method_mames[static_cast<uint32_t>(_http_method_traits.method)];
    }

    bool is_known_method() const
    {
        return _http_method_traits.method != http_method_id::UNKNOWN;
    }

    bool is_connect() const
    {
        return _http_method_traits.method == http_method_id::CONNECT;
    }

    bool is_delete() const
    {
        return _http_method_traits.method == http_method_id::DELETE;
    }

    bool is_get() const
    {
        return _http_method_traits.method == http_method_id::GET;
    }

    bool is_head() const
    {
        return _http_method_traits.method == http_method_id::HEAD;
    }

    bool is_options() const
    {
        return _http_method_traits.method == http_method_id::OPTIONS;
    }

    bool is_patch() const
    {
        return _http_method_traits.method == http_method_id::PATCH;
    }

    bool is_post() const
    {
        return _http_method_traits.method == http_method_id::POST;
    }

    bool is_put() const
    {
        return _http_method_traits.method == http_method_id::PUT;
    }

    bool is_trace() const
    {
        return _http_method_traits.method == http_method_id::TRACE;
    }

    void clear()
    {
        _request_data = nullptr;
        _request_data_size = 0;

        _http_method_traits = { http_method_id::UNKNOWN, 0xffffffff };
        _resource = nullptr;
        _version_part = nullptr;
        _protocol = nullptr;
        _version = nullptr;
        _content = nullptr;
        _headers.clear();
    }

    struct http_constants
    {
#ifdef ARCH_LITTLE_ENDIAN
        static constexpr const uint32_t CONN_method_header_chars { 0x4e4e4f43 };
        static constexpr const uint32_t ECT_method_header_chars  { 0x20544345 };

        static constexpr const uint32_t DELE_method_header_chars { 0x454c4544 };
        static constexpr const uint32_t ETE_method_header_chars  { 0x20455445 };

        static constexpr const uint32_t GET_method_header_chars  { 0x20544547 };

        static constexpr const uint32_t HEAD_method_header_chars { 0x44414548 };

        static constexpr const uint32_t OPTI_method_header_chars { 0x4954504f };
        static constexpr const uint32_t ONS_method_header_chars  { 0x20534e4f };

        static constexpr const uint32_t PATC_method_header_chars { 0x43544150 };
        static constexpr const uint32_t TCH_method_header_chars  { 0x20484354 };

        static constexpr const uint32_t POST_method_header_chars { 0x54534f50 };

        static constexpr const uint32_t PUT_method_header_chars  { 0x20545550 };

        static constexpr const uint32_t TRAC_method_header_chars { 0x43415254 };
        static constexpr const uint32_t ACE_method_header_chars  { 0x20454341 };

        static constexpr const uint16_t CRLF                     { 0x0a0d };
        static constexpr const uint32_t CRLFCRLF                 { 0x0a0d0a0d };
        static constexpr const uint32_t CRLFDDASH                { 0x2d2d0a0d };

#else
        static constexpr const uint32_t CONN_method_header_chars { 0x434f4e4e };
        static constexpr const uint32_t ECT_method_header_chars  { 0x45435420 };

        static constexpr const uint32_t DELE_method_header_chars { 0x44454c45 };
        static constexpr const uint32_t ETE_method_header_chars  { 0x45544520 };

        static constexpr const uint32_t GET_method_header_chars  { 0x47455420 };

        static constexpr const uint32_t HEAD_method_header_chars { 0x48454144 };

        static constexpr const uint32_t OPTI_method_header_chars { 0x4f505449 };
        static constexpr const uint32_t ONS_method_header_chars  { 0x4f4e5320 };

        static constexpr const uint32_t PATC_method_header_chars { 0x50415443 };
        static constexpr const uint32_t TCH_method_header_chars  { 0x54434820 };

        static constexpr const uint32_t POST_method_header_chars { 0x504f5354 };

        static constexpr const uint32_t PUT_method_header_chars  { 0x50555420 };

        static constexpr const uint32_t TRAC_method_header_chars { 0x54524143 };
        static constexpr const uint32_t ACE_method_header_chars  { 0x41434520 };

        static constexpr const uint16_t CRLF                     { 0x0d0a };
        static constexpr const uint32_t CRLFCRLF                 { 0x0d0a0d0a };
        static constexpr const uint32_t CRLFDDASH                { 0x0d0a2d2d };
#endif
        static constexpr const uint32_t CONNECT_method_skip_size { 8 };
        static constexpr const uint32_t DELETE_method_skip_size  { 7 };
        static constexpr const uint32_t GET_method_skip_size     { 4 };
        static constexpr const uint32_t HEAD_method_skip_size    { 5 };
        static constexpr const uint32_t OPTIONS_method_skip_size { 8 };
        static constexpr const uint32_t PATCH_method_skip_size   { 6 };
        static constexpr const uint32_t POST_method_skip_size    { 5 };
        static constexpr const uint32_t PUT_method_skip_size     { 4 };
        static constexpr const uint32_t TRACE_method_skip_size   { 6 };
        static constexpr const uint8_t CR                        { 0x0a };
        static constexpr const uint8_t LF                        { 0x0d };
        static constexpr const uint16_t DDASH                    { 0x2d2d };
    };

protected:
    static constexpr const char *const _method_mames[] { "CONNECT", "DELETE", "GET", "HEAD", "OPTIONS", "PATCH", "POST", "PUT", "TRACE", "UNKNOWN" };

    struct http_method_traits
    {
        http_method_id method;
        uint32_t method_skip_size;
    };

    void parse()
    {
        if (!_request_data || !*_request_data || _request_data_size < 5) return;

        _http_method_traits = { http_method_id::UNKNOWN, 0xffffffff };

        switch (*reinterpret_cast<const uint32_t*>(_request_data))
        {
        case http_constants::CONN_method_header_chars:
            if (*reinterpret_cast<const uint32_t*>(_request_data + sizeof(uint32_t)) == http_constants::ECT_method_header_chars) _http_method_traits = { http_method_id::CONNECT, http_constants::CONNECT_method_skip_size };
            break;
        case http_constants::DELE_method_header_chars:
            if (*reinterpret_cast<const uint32_t*>(_request_data + sizeof(uint32_t) - 1) == http_constants::ETE_method_header_chars) _http_method_traits = { http_method_id::DELETE, http_constants::DELETE_method_skip_size };
            break;
        case http_constants::GET_method_header_chars: _http_method_traits = { http_method_id::GET, http_constants::GET_method_skip_size };
            break;
        case http_constants::HEAD_method_header_chars:
            if (*(_request_data + http_constants::HEAD_method_skip_size - 1) == ' ') _http_method_traits = { http_method_id::HEAD, http_constants::HEAD_method_skip_size };
            break;
        case http_constants::OPTI_method_header_chars:
            if (*reinterpret_cast<const uint32_t*>(_request_data + sizeof(uint32_t)) == http_constants::ONS_method_header_chars) _http_method_traits = { http_method_id::OPTIONS, http_constants::OPTIONS_method_skip_size };
            break;
        case http_constants::PATC_method_header_chars:
            if (*reinterpret_cast<const uint32_t*>(_request_data + 2) == http_constants::TCH_method_header_chars) _http_method_traits = { http_method_id::PATCH, http_constants::PATCH_method_skip_size };
            break;
        case http_constants::POST_method_header_chars:
            if (*(_request_data + http_constants::POST_method_skip_size - 1) == ' ') _http_method_traits = { http_method_id::POST, http_constants::POST_method_skip_size };
            break;
        case http_constants::PUT_method_header_chars: _http_method_traits = { http_method_id::PUT, http_constants::PUT_method_skip_size };
            break;
        case http_constants::TRAC_method_header_chars:
            if (*reinterpret_cast<const uint32_t*>(_request_data + 2) == http_constants::ACE_method_header_chars) _http_method_traits = { http_method_id::TRACE, http_constants::TRACE_method_skip_size };
            break;
        default:
            break;
        }

        auto request_data_end { _request_data + _request_data_size };

        if (!_request_data || _http_method_traits.method == http_method_id::UNKNOWN) return;

        const uint8_t *resource_begin = _request_data + _http_method_traits.method_skip_size;

        while (resource_begin < request_data_end && *resource_begin == ' ') ++resource_begin;

        if (resource_begin >= request_data_end) return;

        const uint8_t *version_begin = resource_begin;

        while (version_begin < request_data_end && *version_begin != ' ') ++version_begin;

        _resource = { reinterpret_cast<const char*>(resource_begin), static_cast<size_t>(version_begin - resource_begin) };

        if (version_begin >= request_data_end) return;

        while (version_begin < request_data_end && *version_begin == ' ') ++version_begin;

        if (version_begin >= request_data_end) return;

        const uint8_t *version_end = version_begin;

        while (version_end < request_data_end && static_cast<uint8_t>(*version_end) != http_constants::LF) ++version_end;

        if (version_begin >= request_data_end) return;

        _version_part = { reinterpret_cast<const char*>(version_begin), static_cast<size_t>(version_end - version_begin) };

        if (auto sp {_version_part.find('/')}; !std::empty(_version_part) && sp != std::string_view::npos)
        {
            _protocol = _version_part.substr(0, sp);
            _version = _version_part.substr(sp + 1);
        }

        ++version_end;

        if (static_cast<uint8_t>(*version_end) == http_constants::CR) ++version_end;

        if (version_end >= request_data_end) return;

        const uint8_t *param_begin{ version_end }, *param_end{ version_end }, *value_begin{ nullptr }, *value_end{ nullptr };

        while (param_begin < request_data_end && *reinterpret_cast<const uint16_t*>(param_begin) != http_constants::CRLF)
        {
            while (param_end < request_data_end && *param_end != ':') ++param_end;

            if (param_end >= request_data_end) break;

            value_begin = param_end + 1;

            while (value_begin < request_data_end && *value_begin == ' ') ++value_begin;

            value_end = value_begin;

            while (value_end < request_data_end && static_cast<uint8_t>(*value_end) != http_constants::LF) ++value_end;

            _headers.insert({ { reinterpret_cast<const char*>(param_begin), static_cast<size_t>(param_end - param_begin) }, { reinterpret_cast<const char*>(value_begin), static_cast<size_t>(value_end - value_begin) } });

            if (value_end >= request_data_end) break;

            param_begin = value_end + 1;

            if (static_cast<uint8_t>(*param_begin) == http_constants::CR) ++param_begin;

            param_end = param_begin;
        }

        auto content_ptr { (*reinterpret_cast<const uint16_t*>(param_begin) == http_constants::CRLF) ? param_begin + 2 : nullptr };

        if (content_ptr) _content = { reinterpret_cast<const char*>(content_ptr), static_cast<size_t>(_request_data + _request_data_size - content_ptr) };
    }

    const uint8_t *_request_data { nullptr };
    size_t _request_data_size { 0 };
    http_method_traits _http_method_traits { http_method_id::UNKNOWN, 0xffffffff };
    std::string_view _resource {}, _version_part {}, _protocol {}, _version {}, _content {};
    std::unordered_map<std::string_view, std::string_view> _headers;
};

class multipart_form_data
{
public:
    struct multipart_item
    {
        std::unordered_map<std::string_view, std::unordered_map<std::string_view, std::string_view>> headers;
        std::string_view content;
    };

    std::vector<multipart_item> parse_data(std::string_view data, std::string_view boundary = {})
    {
        using http_constants = http_request_parser::http_constants;

        _data = data;

        if (std::empty(boundary))
        {
            auto data_ptr{ std::data(_data) };
            auto cursor{ data_ptr };
            auto data_end{ data_ptr + std::size(_data) };

            while (cursor < data_end && *reinterpret_cast<const uint16_t*>(cursor) != http_constants::DDASH) ++cursor;

            if (cursor < data_end)
            {
                cursor += sizeof(http_constants::DDASH);

                auto btag_start{ cursor };

                while (cursor < data_end && *reinterpret_cast<const uint16_t*>(cursor) != http_constants::CRLF && *reinterpret_cast<const uint16_t*>(cursor) != http_constants::DDASH) ++cursor;

                boundary = { btag_start, static_cast<size_t>(cursor - btag_start) };
            }
        }

        _boundary = boundary;

        return parse();
    }

    static std::unordered_map<std::string_view, std::string_view> parse_header_value(std::string_view header_value)
    {
        std::unordered_map<std::string_view, std::string_view> result;
        auto cursor{ std::data(header_value) };
        auto data_end{ cursor + std::size(header_value) };

        while (cursor < data_end)
        {
            std::string_view name, value;

            while (cursor < data_end && *cursor == ' ') ++cursor;

            auto cursor_prev{ cursor };
            bool name_consumed{ false };

            while (cursor < data_end && *cursor != ';')
            {
                if (*cursor == '=' && !name_consumed)
                {
                    name = { cursor_prev, static_cast<size_t>(cursor - cursor_prev) };

                    ++cursor;

                    if (*cursor == '\"') ++cursor;

                    cursor_prev = cursor;
                    name_consumed = true;
                }

                ++cursor;
            }

            bool remove_quotes{ *(cursor - 1) == '\"' };

            if (remove_quotes) --cursor;

            value = { cursor_prev, static_cast<size_t>(cursor - cursor_prev) };

            if (remove_quotes) ++cursor;

            result.emplace(std::move(name), std::move(value));

            ++cursor;
        }

        return result;
    }

protected:
    std::string_view _data;
    std::string_view _boundary;
    std::vector<multipart_item> _multipart_items;

    std::vector<multipart_item> parse()
    {
        using http_constants = http_request_parser::http_constants;

        std::vector<multipart_item> multipart_items;

        auto data_ptr{ std::data(_data) };
        auto data_end{ data_ptr + std::size(_data) };
        auto btag{ std::data(_boundary) };
        auto btag_length{ std::size(_boundary) };
        auto cursor{ data_ptr };

        if (std::empty(_data) || std::empty(_boundary)) return {};

        while (cursor < data_end)
        {
            multipart_item item;

            while (cursor < data_end && *reinterpret_cast<const uint16_t*>(cursor) != http_constants::DDASH) ++cursor;

            if (cursor >= data_end) break;

            cursor += sizeof(http_constants::DDASH);

            auto btag_check{ btag };
            auto btag_end{ cursor + btag_length };

            while (cursor < btag_end && *cursor++ == *btag_check++);

            if (cursor < btag_end) break;
            if (*reinterpret_cast<const uint16_t*>(cursor) != http_constants::CRLF) break;

            cursor += sizeof(http_constants::CRLF);

            while (*reinterpret_cast<const uint16_t*>(cursor) != http_constants::CRLF)
            {
                auto param_start{ cursor };

                while (cursor < data_end && *cursor != ':') ++cursor;

                std::string_view param{ param_start, static_cast<size_t>(cursor - param_start) };

                if (cursor >= data_end) break;

                cursor += 1;

                while (cursor < data_end && *cursor == ' ') ++cursor;

                if (cursor >= data_end) break;

                auto value_start{ cursor };

                while (cursor < data_end && *reinterpret_cast<const uint16_t*>(cursor) != http_constants::CRLF) ++cursor;

                item.headers.emplace(std::move(param), parse_header_value({ value_start, static_cast<size_t>(cursor - value_start) }));

                cursor += sizeof(http_constants::CRLF);
            }

            if (*reinterpret_cast<const uint16_t*>(cursor) == http_constants::CRLF && *reinterpret_cast<const uint16_t*>(cursor - sizeof(http_constants::CRLF)) == http_constants::CRLF)
            {
                cursor += sizeof(http_constants::CRLF);

                auto body_start{ cursor };

                while (cursor < data_end && *reinterpret_cast<const uint32_t*>(cursor) != http_constants::CRLFDDASH) ++cursor;

                btag_check = btag;
                btag_end = cursor + sizeof(http_constants::CRLFDDASH) + btag_length;

                auto tag_start{ cursor + sizeof(http_constants::CRLFDDASH) };

                while (tag_start < data_end && tag_start < btag_end && *tag_start++ == *btag_check++);

                if (tag_start >= btag_end)
                {
                    item.content = { body_start, static_cast<size_t>(cursor - body_start) };
                }

                cursor += sizeof(http_constants::CRLF);
            }

            multipart_items.emplace_back(std::move(item));
        }

        return multipart_items;
    }
};

}
