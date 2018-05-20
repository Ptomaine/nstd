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
#include <string>
#include <utility>
#include <vector>


namespace nstd
{

class uri_exception : public std::runtime_error
{
public:
    uri_exception() = default;
    explicit uri_exception(const std::string& what_arg) : runtime_error(what_arg) {}
    explicit uri_exception(const char* what_arg) : runtime_error(what_arg) {}
};

class uri
{
public:
    using query_parameters_t = std::vector<std::pair<std::string, std::string>>;
    static inline const std::string PathReservedChars        { "?#" };
    static inline const std::string QueryReservedChars       { "?#/:;+@" };
    static inline const std::string QueryParamReservedChars  { "?#/:;+@&=" };
    static inline const std::string FragmentReservedChars;
    static inline const std::string IlligalChars             { "%<>{}|\\\"^`!*'()$,[]" };

    uri() = default;

    explicit uri(const std::string& uri)
    {
        parse(uri);
    }

    uri(const std::string& scheme, const std::string& path_etc) : _scheme(scheme)
    {
        to_lower_in_place(_scheme);

        _port = get_well_known_port();

        auto begin { std::cbegin(path_etc) }, end { std::cend(path_etc) };

        parse_path_etc(begin, end);
    }

    uri(const std::string& scheme, const std::string& authority, const std::string& path_etc) : _scheme(scheme)
    {
        to_lower_in_place(_scheme);

        auto abegin { std::cbegin(authority) }, aend { std::cend(authority) };

        parse_authority(abegin, aend);

        auto pbegin{ std::cbegin(path_etc) }, pend { std::cend(path_etc) };

        parse_path_etc(pbegin, pend);
    }

    uri(const std::string& scheme, const std::string& authority, const std::string& path, const std::string& query): _scheme(scheme), _path(path), _query(query)
    {
        to_lower_in_place(_scheme);

        auto begin { std::cbegin(authority) }, end { std::cend(authority) };

        parse_authority(begin, end);
    }

    uri(const std::string& scheme, const std::string& authority, const std::string& path, const std::string& query, const std::string& fragment) : _scheme(scheme), _path(path), _query(query), _fragment(fragment)
    {
        to_lower_in_place(_scheme);

        auto begin { std::cbegin(authority) }, end { std::cend(authority) };

        parse_authority(begin, end);
    }

    uri(const uri& uri) = default;

    uri(const uri& base_uri, const std::string& relative_uri) : _scheme(base_uri._scheme), _user_info(base_uri._user_info), _host(base_uri._host), _port(base_uri._port), _path(base_uri._path), _query(base_uri._query), _fragment(base_uri._fragment)
    {
        resolve(relative_uri);
    }

    ~uri() = default;

    uri& operator = (const uri& uri)
    {
        if (&uri != this)
        {
            _scheme   = uri._scheme;
            _user_info = uri._user_info;
            _host     = uri._host;
            _port     = uri._port;
            _path     = uri._path;
            _query    = uri._query;
            _fragment = uri._fragment;
        }
        return *this;
    }

    uri& operator = (const std::string& uri)
    {
        clear();
        parse(uri);

        return *this;
    }

    void swap(uri& uri)
    {
        std::swap(_scheme, uri._scheme);
        std::swap(_user_info, uri._user_info);
        std::swap(_host, uri._host);
        std::swap(_port, uri._port);
        std::swap(_path, uri._path);
        std::swap(_query, uri._query);
        std::swap(_fragment, uri._fragment);
    }

    void clear()
    {
        _scheme.clear();
        _user_info.clear();
        _host.clear();
        _port = 0;
        _path.clear();
        _query.clear();
        _fragment.clear();
    }

    std::string to_string() const
    {
        std::string uri;

        if (is_relative())
        {
            encode(_path, PathReservedChars, uri);
        }
        else
        {
            uri = _scheme;
            uri += ':';

            std::string auth { get_authority() };

            if (!std::empty(auth) || _scheme == "file")
            {
                uri.append("//");
                uri.append(auth);
            }

            if (!std::empty(_path))
            {
                if (!std::empty(auth) && _path[0] != '/')
                    uri += '/';

                encode(_path, PathReservedChars, uri);
            }
            else if (!std::empty(_query) || !std::empty(_fragment))
            {
                uri += '/';
            }
        }

        if (!std::empty(_query))
        {
            uri += '?';
            uri.append(_query);
        }

        if (!std::empty(_fragment))
        {
            uri += '#';
            encode(_fragment, FragmentReservedChars, uri);
        }

        return uri;
    }

    const std::string& get_scheme() const { return _scheme; }

    void set_scheme(const std::string& scheme)
    {
        _scheme = scheme;

        to_lower_in_place(_scheme);

        if (_port == 0) _port = get_well_known_port();
    }

    const std::string& get_user_info() const { return _user_info; }

    void set_user_info(const std::string& user_info)
    {
        _user_info.clear();
        decode(user_info, _user_info);
    }

    const std::string& get_host() const { return _host; }

    void set_host(const std::string& host)
    {
        _host = host;
    }

    bool is_ipv6_host() const { return _host.find(':') != std::string::npos; }

    unsigned short get_port() const
    {
       if (_port == 0) return get_well_known_port();

        return _port;
    }
    void set_port(unsigned short port)
    {
        _port = port;
    }

    std::string get_authority() const
    {
        std::string auth;

        if (!std::empty(_user_info))
        {
            auth.append(_user_info);
            auth += '@';
        }

        if (_host.find(':') != std::string::npos)
        {
            auth += '[';
            auth += _host;
            auth += ']';
        }
        else auth.append(_host);

        if (_port && !get_well_known_port())
        {
            auth += ':';
            auth += std::to_string(_port);
        }

        return auth;
    }

    void set_authority(const std::string& authority)
    {
        _user_info.clear();
        _host.clear();
        _port = 0;

        auto begin { std::cbegin(authority) }, end { std::cend(authority) };

        parse_authority(begin, end);
    }

    const std::string& get_path() const { return _path; }

    void set_path(const std::string& path)
    {
        _path.clear();

        decode(path, _path);
    }

    std::string get_query() const
    {
        std::string query;

        decode(_query, query);

        return query;
    }

    void set_query(const std::string& query)
    {
        _query.clear();

        encode(query, QueryReservedChars, _query);
    }

    void add_query_parameter(const std::string& param, const std::string& val = std::string())
    {
        if (!std::empty(_query)) _query += '&';

        encode(param, QueryParamReservedChars, _query);

        _query += '=';

        encode(val, QueryParamReservedChars, _query);
    }

    const std::string& get_raw_query() const
    {
        return _query;
    }

    void set_raw_query(const std::string& query)
    {
        _query = query;
    }

    query_parameters_t get_query_parameters() const
    {
        query_parameters_t result;
        auto it { std::cbegin(_query) }, end { std::cend(_query) };

        while (it != end)
        {
            std::string name;
            std::string value;

            while (it != end && *it != '=' && *it != '&')
            {
                if (*it == '+') name += ' ';
                else name += *it;

                ++it;
            }

            if (it != end && *it == '=')
            {
                ++it;

                while (it != end && *it != '&')
                {
                    if (*it == '+') value += ' ';
                    else value += *it;

                    ++it;
                }
            }

            std::string decoded_name;
            std::string decoded_value;

            decode(name, decoded_name);
            decode(value, decoded_value);

            result.push_back({ decoded_name, decoded_value });

            if (it != end && *it == '&') ++it;
        }

        return result;
    }

    void set_query_parameters(const query_parameters_t& params)
    {
        _query.clear();

        for (const auto&[key, value] : params) add_query_parameter(key, value);
    }

    const std::string& get_fragment() const { return _fragment; }

    void set_fragment(const std::string& fragment)
    {
        _fragment.clear();
        decode(fragment, _fragment);
    }

    void set_path_etc(const std::string& path_etc)
    {
        _path.clear();
        _query.clear();
        _fragment.clear();

        auto begin { std::cbegin(path_etc) }, end { std::cend(path_etc) };

        parse_path_etc(begin, end);
    }

    std::string get_path_etc() const
    {
        std::string path_etc;

        encode(_path, PathReservedChars, path_etc);

        if (!std::empty(_query))
        {
            path_etc += '?';
            path_etc += _query;
        }

        if (!std::empty(_fragment))
        {
            path_etc += '#';

            encode(_fragment, FragmentReservedChars, path_etc);
        }

        return path_etc;
    }

    std::string get_path_and_query() const
    {
        std::string path_and_query;

        encode(_path, PathReservedChars, path_and_query);

        if (!std::empty(_query))
        {
            path_and_query += '?';
            path_and_query += _query;
        }

        return path_and_query;
    }

    void resolve(const std::string& relative_uri)
    {
        uri parsed_uri { relative_uri };

        resolve(parsed_uri);
    }

    void resolve(const uri& relative_uri)
    {
        if (!std::empty(relative_uri._scheme))
        {
            _scheme   = relative_uri._scheme;
            _user_info = relative_uri._user_info;
            _host     = relative_uri._host;
            _port     = relative_uri._port;
            _path     = relative_uri._path;
            _query    = relative_uri._query;

            remove_dot_segments();
        }
        else
        {
            if (!std::empty(relative_uri._host))
            {
                _user_info = relative_uri._user_info;
                _host     = relative_uri._host;
                _port     = relative_uri._port;
                _path     = relative_uri._path;
                _query    = relative_uri._query;

                remove_dot_segments();
            }
            else
            {
                if (std::empty(relative_uri._path))
                {
                    if (!std::empty(relative_uri._query)) _query = relative_uri._query;
                }
                else
                {
                    if (relative_uri._path[0] == '/')
                    {
                        _path = relative_uri._path;
                        remove_dot_segments();
                    }
                    else
                    {
                        merge_path(relative_uri._path);
                    }

                    _query = relative_uri._query;
                }
            }
        }

        _fragment = relative_uri._fragment;
    }

    bool is_relative() const { return std::empty(_scheme); }

    bool empty() const { return std::empty(_scheme) && std::empty(_host) && std::empty(_path) && std::empty(_query) && std::empty(_fragment); }

    bool operator == (const uri& uri_) const { return equals(uri_); }

    bool operator == (const std::string& uri_) const { uri parsed_uri { uri_ }; return equals(parsed_uri); }

    bool operator != (const uri& uri_) const { return !equals(uri_); }

    bool operator != (const std::string& uri_) const { return !operator ==(uri_); }

    void normalize() { remove_dot_segments(!is_relative()); }

    void get_path_segments(std::vector<std::string>& segments)
    {
        get_path_segments(_path, segments);
    }

    static std::string char_to_hex(char c, bool upper_case = false)
    {
        static constexpr const char *const hexmap[] = { "0123456789abcdef", "0123456789ABCDEF" };
        std::string s;

        s += hexmap[upper_case][(c & 0xF0) >> 4];
        s += hexmap[upper_case][c & 0x0F];

        return s;
    }

    static void encode(const std::string& str, const std::string& reserved, std::string& encoded_str)
    {
        for (const auto&c : str)
        {
            if ((c >= 'a' && c <= 'z') ||
                (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') ||
                 c == '-' || c == '_'  ||
                 c == '.' || c == '~')
            {
                encoded_str += c;
            }
            else if (c <= 0x20 || c >= 0x7F || IlligalChars.find(c) != std::string::npos || reserved.find(c) != std::string::npos)
            {
                encoded_str += '%';
                encoded_str += char_to_hex(c);
            }
            else encoded_str += c;
        }
    }

    static void decode(const std::string& str, std::string& decoded_str, bool plus_as_space = false)
    {
        bool in_query { false };
        auto it { std::cbegin(str) }, end { std::cend(str) };

        while (it != end)
        {
            char c { *it++ };

            if (!in_query && c == '?') in_query = true;

            if (in_query && plus_as_space && c == '+')
            {
                c = ' ';
            }
            else if (c == '%')
            {
                if (it == end) throw uri_exception("URI encoding: no hex digit following percent sign");

                char hi { *it++ };

                if (it == end) throw uri_exception("URI encoding: two hex digits must follow percent sign");

                char lo { *it++ };

                if (hi >= '0' && hi <= '9') c = hi - '0';
                else if (hi >= 'A' && hi <= 'F') c = hi - 'A' + 10;
                else if (hi >= 'a' && hi <= 'f') c = hi - 'a' + 10;
                else throw uri_exception("URI encoding: not a hex digit");

                c *= 16;

                if (lo >= '0' && lo <= '9') c += lo - '0';
                else if (lo >= 'A' && lo <= 'F') c += lo - 'A' + 10;
                else if (lo >= 'a' && lo <= 'f') c += lo - 'a' + 10;
                else throw uri_exception("URI encoding: not a hex digit");
            }

            decoded_str += c;
        }
    }

protected:
    bool equals(const uri& uri) const
    {
        return _scheme   == uri._scheme
            && _user_info == uri._user_info
            && _host     == uri._host
            && get_port() == uri.get_port()
            && _path     == uri._path
            && _query    == uri._query
            && _fragment == uri._fragment;
    }

    bool is_well_known_port() const
    {
        return _port == get_well_known_port();
    }

    unsigned short get_well_known_port() const
    {
             if (_scheme == "ftp") return 21;
        else if (_scheme == "ssh") return 22;
        else if (_scheme == "telnet") return 23;
        else if (_scheme == "http" || _scheme == "ws") return 80;
        else if (_scheme == "nntp") return 119;
        else if (_scheme == "ldap") return 389;
        else if (_scheme == "https" || _scheme == "wss") return 443;
        else if (_scheme == "rtsp") return 554;
        else if (_scheme == "sip") return 5060;
        else if (_scheme == "sips") return 5061;
        else if (_scheme == "xmpp") return 5222;
        else return 0;
    }

    void parse(const std::string& uri)
    {
        auto it { std::cbegin(uri) }, end { std::cend(uri) };

        if (it == end) return;

        if (*it != '/' && *it != '.' && *it != '?' && *it != '#')
        {
            std::string scheme;

            while (it != end && *it != ':' && *it != '?' && *it != '#' && *it != '/') scheme += *it++;

            if (it != end && *it == ':')
            {
                ++it;

                if (it == end) throw uri_exception("URI scheme must be followed by authority or path");

                set_scheme(scheme);

                if (*it == '/')
                {
                    ++it;

                    if (it != end && *it == '/')
                    {
                        ++it;
                        parse_authority(it, end);
                    }
                    else --it;
                }

                parse_path_etc(it, end);
            }
            else
            {
                it = std::cbegin(uri);

                parse_path_etc(it, end);
            }
        }
        else parse_path_etc(it, end);
    }

    void parse_authority(std::string::const_iterator& it, const std::string::const_iterator& end)
    {
        std::string user_info;
        std::string part;

        while (it != end && *it != '/' && *it != '?' && *it != '#')
        {
            if (*it == '@')
            {
                user_info = part;
                part.clear();
            }
            else part += *it;

            ++it;
        }
        std::string::const_iterator pbeg { std::cbegin(part) }, pend { std::cend(part) };

        parse_host_and_port(pbeg, pend);

        _user_info = user_info;
    }

    void parse_host_and_port(std::string::const_iterator& it, const std::string::const_iterator& end)
    {
        if (it == end) return;

        std::string host;

        if (*it == '[')
        {
            ++it;

            while (it != end && *it != ']') host += *it++;

            if (it == end) throw uri_exception("unterminated IPv6 address");

            ++it;
        }
        else
        {
            while (it != end && *it != ':') host += *it++;
        }
        if (it != end && *it == ':')
        {
            ++it;
            std::string port;
            while (it != end) port += *it++;
            if (!std::empty(port))
            {
                int nport { 0 };

                try { nport = std::stoi(port); } catch (...) { nport = 0; }

                if (nport > 0 && nport < 65536) _port = (unsigned short) nport;
                else throw uri_exception("bad or invalid port number");
            }
            else _port = get_well_known_port();
        }
        else _port = get_well_known_port();

        _host = host;

        to_lower_in_place(_host);
    }

    void parse_path(std::string::const_iterator& it, const std::string::const_iterator& end)
    {
        std::string path;

        while (it != end && *it != '?' && *it != '#') path += *it++;

        decode(path, _path);
    }

    void parse_path_etc(std::string::const_iterator& it, const std::string::const_iterator& end)
    {
        if (it == end) return;

        if (*it != '?' && *it != '#') parse_path(it, end);

        if (it != end && *it == '?')
        {
            ++it;

            parse_query(it, end);
        }

        if (it != end && *it == '#')
        {
            ++it;

            parse_fragment(it, end);
        }
    }

    void parse_query(std::string::const_iterator& it, const std::string::const_iterator& end)
    {
        _query.clear();

        while (it != end && *it != '#') _query += *it++;
    }

    void parse_fragment(std::string::const_iterator& it, const std::string::const_iterator& end)
    {
        std::string fragment;

        while (it != end) fragment += *it++;

        decode(fragment, _fragment);
    }

    void merge_path(const std::string& path)
    {
        std::vector<std::string> segments;
        std::vector<std::string> normalized_segments;
        bool add_leading_slash { false };

        if (!std::empty(_path))
        {
            get_path_segments(segments);

            bool ends_with_slash { *(_path.rbegin()) == '/' };

            if (!ends_with_slash && !std::empty(segments)) segments.pop_back();

            add_leading_slash = _path[0] == '/';
        }

        get_path_segments(path, segments);

        add_leading_slash = add_leading_slash || (!std::empty(path) && path[0] == '/');

        bool has_trailing_slash { (!std::empty(path) && *(path.rbegin()) == '/') };
        bool add_trailing_slash { false };

        for (auto it { std::cbegin(segments) }, end { std::cend(segments) }; it != end; ++it)
        {
            if (*it == "..")
            {
                add_trailing_slash = true;

                if (!std::empty(normalized_segments)) normalized_segments.pop_back();
            }
            else if (*it != ".")
            {
                add_trailing_slash = false;

                normalized_segments.push_back(*it);
            }
            else add_trailing_slash = true;
        }

        build_path(normalized_segments, add_leading_slash, has_trailing_slash || add_trailing_slash);
    }

    void remove_dot_segments(bool remove_leading = true)
    {
        if (std::empty(_path)) return;

        bool leading_slash { _path.front() == '/' };
        bool trailing_slash { _path.back() == '/' };
        std::vector<std::string> segments;
        std::vector<std::string> normalized_segments;

        get_path_segments(segments);

        for (const auto& seg : segments)
        {
            if (seg == "..")
            {
                if (!std::empty(normalized_segments))
                {
                    if (normalized_segments.back() == "..") normalized_segments.push_back(seg);
                    else normalized_segments.pop_back();
                }
                else if (!remove_leading) normalized_segments.push_back(seg);
            }
            else if (seg != ".") normalized_segments.push_back(seg);
        }

        build_path(normalized_segments, leading_slash, trailing_slash);
    }

    static void get_path_segments(const std::string& path, std::vector<std::string>& segments)
    {
        std::string seg;

        for (const auto& p : path)
        {
            if (p == '/')
            {
                if (!std::empty(seg))
                {
                    segments.push_back(seg);
                    seg.clear();
                }
            }
            else seg += p;
        }

        if (!std::empty(seg)) segments.push_back(seg);
    }

    void build_path(const std::vector<std::string>& segments, bool leading_slash, bool trailing_slash)
    {
        _path.clear();

        bool first { true };

        for (auto it { std::cbegin(segments) }, end { std::cend(segments) }; it != end; ++it)
        {
            if (first)
            {
                first = false;

                if (leading_slash) _path += '/';
                else if (std::empty(_scheme) && (*it).find(':') != std::string::npos) _path.append("./");
            }
            else _path += '/';

            _path.append(*it);
        }

        if (trailing_slash) _path += '/';
    }

private:
    std::string    _scheme {};
    std::string    _user_info {};
    std::string    _host {};
    unsigned short _port { 0 };
    std::string    _path {};
    std::string    _query {};
    std::string    _fragment {};

    void to_lower_in_place(std::string &str)
    {
        std::transform(std::cbegin(str), std::cend(str), std::begin(str), ::tolower);
    }
};


inline void swap(uri& u1, uri& u2)
{
    u1.swap(u2);
}

}
