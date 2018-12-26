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

#include <atomic>
#include <condition_variable>
#include <chrono>
#include <cstring>
#include <functional>
#include <deque>
#include <mutex>
#include <optional>
#include <queue>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#include <unordered_map>

#include <fcntl.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#endif

#ifdef SHARP_TCP_USES_OPENSSL
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#endif

namespace nstd::net
{
using namespace std::chrono_literals;

#ifdef _WIN32
using fd_t = SOCKET;
inline constexpr const uint64_t INVALID_FD = INVALID_SOCKET;
#else
using fd_t = int;
inline constexpr const int INVALID_FD = -1;
#endif

#if defined(_MSC_VER)
#include <basetsd.h>
using ssize_t = SSIZE_T;
#endif

class sharp_tcp_error : public std::runtime_error
{
public:
    sharp_tcp_error(const std::string& what, const std::string& file = {}, size_t line = {}) :
        std::runtime_error(what),
        _file(file),
        _line(line) {}

    ~sharp_tcp_error() = default;

    sharp_tcp_error(const sharp_tcp_error&) = default;
    sharp_tcp_error& operator=(const sharp_tcp_error&) = default;

    const std::string& get_file() const
    {
        return _file;
    }

    std::size_t get_line() const
    {
        return _line;
    }

private:
    std::string _file {};
    std::size_t _line {};
};

class thread_pool
{
public:
    explicit thread_pool(std::size_t nb_threads) : _nb_threads(nb_threads)
    {
        for (std::size_t i { 0 }; i < nb_threads; ++i)
        {
            _workers.push_back(std::thread([this](){ run(); }));
        }
    }

    ~thread_pool()
    {
        stop();
    }

    thread_pool(const thread_pool&) = delete;
    thread_pool& operator=(const thread_pool&) = delete;

public:
    using task_t = std::function<void()>;

    void add_task(const task_t& task)
    {
        std::scoped_lock lock { _tasks_mtx };

        _tasks.push(task);
        _tasks_condvar.notify_all();
    }

    thread_pool& operator<<(const task_t& task)
    {
        return add_task(task), *this;
    }

    void stop()
    {
        if (!is_running()) return;

        _should_stop = true;
        _tasks_condvar.notify_all();

        for (auto& worker : _workers) worker.join();

        _workers.clear();
    }

public:
    bool is_running() const
    {
        return !_should_stop;
    }

public:
    void set_nb_threads(std::size_t nb_threads)
    {
        _nb_threads = nb_threads;

        if (_workers.size() < _nb_threads)
        {
            while (_workers.size() < _nb_threads) _workers.push_back(std::thread([this]() { run(); }));
        }
        else _tasks_condvar.notify_all();
    }

private:
    void run()
    {
        while (!should_stop())
        {
            task_t task = fetch_task();

            if (task)
            {
                try
                {
                    task();
                }
                catch (const std::exception&)
                {
                }
            }
        }
    }

    task_t fetch_task()
    {
        std::unique_lock lock { _tasks_mtx };

        _tasks_condvar.wait(lock, [this] { return should_stop() || !std::empty(_tasks); });

        if (should_stop() || std::empty(_tasks)) return nullptr;

        task_t task = std::move(_tasks.front());

        _tasks.pop();

        return task;
    }

    bool should_stop(void) const
    {
        return _should_stop || _workers.size() > _nb_threads;
    }

private:
    std::vector<std::thread> _workers {};
    std::size_t _nb_threads { 0 };
    std::atomic_bool _should_stop { false };
    std::queue<task_t> _tasks {};
    std::mutex _tasks_mtx {};
    std::condition_variable _tasks_condvar {};
};

#ifndef SOCKET_ERROR
#define SOCKET_ERROR -1
#endif

class self_pipe
{
public:
    self_pipe()
    {
#ifdef _WIN32
        _fd = ::socket(AF_INET, SOCK_DGRAM, 0);

        if (_fd == INVALID_FD) throw sharp_tcp_error { "fail socket()" };

        u_long flags = 1;

        ioctlsocket(_fd, FIONBIO, &flags);

        struct sockaddr_in inaddr;

        memset(&inaddr, 0, sizeof(inaddr));

        inaddr.sin_family      = AF_INET;
        inaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        inaddr.sin_port        = 0;

        if (::bind(_fd, (struct sockaddr*) &inaddr, sizeof(inaddr)) == SOCKET_ERROR) throw sharp_tcp_error { "fail bind()" };

        _addr_len = sizeof(_addr);

        memset(&_addr, 0, sizeof(_addr));

        if (getsockname(_fd, &_addr, &_addr_len) == SOCKET_ERROR) throw sharp_tcp_error { "fail getsockname()" };

        if (connect(_fd, &_addr, _addr_len) == SOCKET_ERROR) throw sharp_tcp_error { "fail connect()" };
#else
        if (::pipe(_fds) == -1) throw sharp_tcp_error { "pipe() failure" };
#endif
    }

    ~self_pipe()
    {
#ifdef _WIN32
        if (_fd != INVALID_FD) closesocket(_fd);
#else
        if (_fds[0] != INVALID_FD) ::close(_fds[0]);
        if (_fds[1] != INVALID_FD) ::close(_fds[1]);
#endif
    }

    self_pipe(const self_pipe&) = delete;
    self_pipe& operator=(const self_pipe&) = delete;

public:
    fd_t get_read_fd() const
    {
#ifdef _WIN32
        return _fd;
#else
        return _fds[0];
#endif
    }

    fd_t get_write_fd() const
    {
#ifdef _WIN32
        return _fd;
#else
        return _fds[1];
#endif
    }

    void notify(void)
    {
#ifdef _WIN32
        auto unused = sendto(_fd, "a", 1, 0, &_addr, _addr_len);
        static_cast<void>(unused);
#else
        auto unused = write(_fds[1], "a", 1);
        static_cast<void>(unused);
#endif
    }

    void clr_buffer(void)
    {
#ifdef _WIN32
        char buf[1024];
        auto unused = recvfrom(_fd, buf, 1024, 0, &_addr, &_addr_len);
        static_cast<void>(unused);
#else
        char buf[1024];
        auto unused = read(_fds[0], buf, 1024);
        static_cast<void>(unused);
#endif
    }

private:
#ifdef _WIN32
    fd_t _fd { INVALID_FD };
    struct sockaddr _addr {};;
    int _addr_len {};
#else
    fd_t _fds[2] { INVALID_FD, INVALID_FD };
#endif
};

template <bool UseSSL = false>
class tcp_socket
{
public:
    enum class type
    {
        CLIENT,
        SERVER,
        UNKNOWN
    };

public:
    tcp_socket() = default;

    ~tcp_socket()
    {
        std::cout << "Socket destructed..." << std::endl;
        close();
    }

#ifdef SHARP_TCP_USES_OPENSSL
    tcp_socket(fd_t fd, const std::string& host, std::uint32_t port, type t, const std::string &cert_file, const std::string &priv_key_file) : tcp_socket()
#else
    tcp_socket(fd_t fd, const std::string& host, std::uint32_t port, type t) : tcp_socket()
#endif
    {
        _fd = fd;
        _host = host;
        _port = port;
        _type = t;

#ifdef SHARP_TCP_USES_OPENSSL
        if constexpr (UseSSL)
        {
            _ssl_context = SSL_CTX_new(TLS_server_method());

            std::cout << "SSL context created..." << std::endl;

            //SSL_CTX_set_options(_ssl_context, SSL_OP_SINGLE_DH_USE);
            SSL_CTX_set_options(_ssl_context, SSL_OP_ALL|SSL_OP_NO_SSLv2|SSL_OP_NO_SSLv3);

            std::cout << "SSL options set..." << std::endl;
            std::cout << "Using cert file: " << cert_file << "; key file: " << priv_key_file << std::endl;

            if (SSL_CTX_use_certificate_file(_ssl_context, cert_file.c_str() , SSL_FILETYPE_PEM) <= 0)
                throw sharp_tcp_error { "use_certificate_file() failure" };
            std::cout << "SSL_CTX_use_certificate_file..." << std::endl;

            if (SSL_CTX_use_PrivateKey_file(_ssl_context, priv_key_file.c_str(), SSL_FILETYPE_PEM) <= 0)
                throw sharp_tcp_error { "use_PrivateKey_file() failure" };
            std::cout << "SSL_CTX_use_PrivateKey_file..." << std::endl;

            _ssl = SSL_new(_ssl_context);

            std::cout << "SSL handle created..." << std::endl;
            
            SSL_set_fd(_ssl, _fd);

            std::cout << "SSL fd set..." << std::endl;

            //SSL_do_handshake(_ssl);
            SSL_set_accept_state(_ssl);

            auto ssl_err = SSL_accept(_ssl);

            if(ssl_err <= 0)
            {
                std::cout << "SSL connection accept FAILED..." << std::endl;
                ERR_print_errors_fp(stdout);

                close();

                throw sharp_tcp_error { "SSL_accept() failure" };
            }

            std::cout << "SSL connection accepted..." << std::endl;
        }
#endif
    }

    tcp_socket(tcp_socket&& socket) :
        _fd { std::move(socket._fd) },
        _host { socket._host },
        _port { socket._port },
        _type { socket._type }
#ifdef SHARP_TCP_USES_OPENSSL
        ,
        _ssl { socket._ssl },
        _ssl_context { socket._ssl_context }
#endif
    {
        socket._fd = INVALID_FD;
        socket._type = type::UNKNOWN;
#ifdef SHARP_TCP_USES_OPENSSL
        socket._ssl = nullptr;
        socket._ssl_context = nullptr;
#endif
    }

    tcp_socket(const tcp_socket&) = delete;
    tcp_socket& operator=(const tcp_socket&) = delete;

public:
    bool operator==(const tcp_socket& rhs) const
    {
        return _fd == rhs._fd && _type == rhs._type;
    }

    bool operator!=(const tcp_socket& rhs) const
    {
        return !operator==(rhs);
    }

public:
    std::vector<uint8_t> recv(std::size_t size_to_read)
    {
#ifdef SHARP_TCP_USES_OPENSSL
        if constexpr (UseSSL)
        {
            std::vector<uint8_t> data(size_to_read, static_cast<uint8_t>(0));

            while (true)
            {
                if (_ssl == nullptr) { std::cout << "SSL is null" << std::endl; return {}; }

                ssize_t read_size { SSL_read(_ssl, std::data(data), size_to_read) };

                std::cout << "...read request..." << std::endl;

                if (read_size <= 0)
                {
                    std::cout << "...read request failed..." << read_size << std::endl;

                    switch(SSL_get_error(_ssl, size_to_read))
                    {
                    case SSL_ERROR_NONE:
                        continue;

                    case SSL_ERROR_ZERO_RETURN:
                        close();
                        return {};

                    case SSL_ERROR_WANT_READ:
                    {
                        fd_set fds;
                        struct timeval timeout {};

                        FD_ZERO(&fds);
                        FD_SET(_fd, &fds);

                        timeout.tv_sec = 5;

                        auto err { ::select(_fd + 1, &fds, nullptr, nullptr, &timeout) };
                        
                        if (err > 0) continue;

                        if (err == 0)
                        {
                            close();
                            throw sharp_tcp_error { "Error reading socket: " };
                        }
                        else
                        {
                            throw sharp_tcp_error { "Error reading socket: " };
                        }

                        break;
                    }

                    case SSL_ERROR_WANT_WRITE:
                    {
                        fd_set fds;
                        struct timeval timeout {};

                        FD_ZERO(&fds);
                        FD_SET(_fd, &fds);

                        timeout.tv_sec = 5;

                        auto err { ::select(_fd + 1, nullptr, &fds, nullptr, &timeout) };
                        
                        if (err > 0) continue;

                        if (err == 0)
                        {
                            close();
                            throw sharp_tcp_error { "Error reading socket: " };
                        }
                        else
                        {
                            throw sharp_tcp_error { "Error reading socket: " };
                        }

                        break;
                    }

                    default:
                        throw sharp_tcp_error { "Error reading socket: " };
                    }
                }
            }

            return data;
        }
        else
        {
            return recv_insecure(size_to_read);
        }
#else
        return recv_insecure(size_to_read);
#endif
    }

    std::size_t send(const std::vector<uint8_t>& data, std::size_t size_to_write)
    {
        if constexpr (UseSSL)
        {
            const uint8_t *data_ptr { std::data(data) };
            size_t total_size { 0 };

            for (const uint8_t* current_position = data_ptr, *end = data_ptr + size_to_write; current_position < end; )
            {
                ssize_t sent { SSL_write(_ssl, current_position, end - current_position) };

                total_size += sent;

                if (sent > 0)
                {
                    current_position += sent;
                }
                else
                {
                    switch (SSL_get_error(_ssl, sent))
                    {
                      case SSL_ERROR_ZERO_RETURN:
                        close();
                        throw sharp_tcp_error { "The socket disconnected" };

                      case SSL_ERROR_WANT_READ:
                      case SSL_ERROR_WANT_WRITE:
                        std::this_thread::sleep_for(200ms);
                        break;

                      default:
                        throw sharp_tcp_error { "Error sending socket: " };
                    }
                }
            }

            return total_size;
        }
        else
        {
            create_socket_if_necessary();
            check_or_set_type(type::CLIENT);

            ssize_t wr_size = ::send(_fd, reinterpret_cast<char*>(const_cast<uint8_t*>(std::data(data))), static_cast<int>(size_to_write), 0);

            if (wr_size == SOCKET_ERROR) throw sharp_tcp_error { "send() failure" };

            return wr_size;
        }
    }

    void connect(const std::string& host, std::uint32_t port, std::uint32_t timeout_msecs = 0)
    {
        _host = host;
        _port = port;

        create_socket_if_necessary();
        check_or_set_type(type::CLIENT);

#ifdef _WIN32
        struct addrinfo* result = nullptr;
        struct addrinfo hints;

        std::memset(&hints, 0, sizeof(hints));
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_family   = AF_INET;

        if (::getaddrinfo(host.c_str(), nullptr, &hints, &result) != 0) throw sharp_tcp_error { "getaddrinfo() failure" };

        struct sockaddr_in server_addr;
        std::memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_addr   = ((struct sockaddr_in*) (result->ai_addr))->sin_addr;
        server_addr.sin_port   = htons(port);
        server_addr.sin_family = AF_INET;

        ::freeaddrinfo(result);

        if (timeout_msecs > 0)
        {
            u_long mode = 1;

            if (::ioctlsocket(_fd, FIONBIO, &mode) != 0)
            {
              close();

              throw sharp_tcp_error { "connect() set non-blocking failure" };
            }
        }

        int ret = ::connect(_fd, (const struct sockaddr*) &server_addr, sizeof(server_addr));

        if (ret == -1 && ::WSAGetLastError() != WSAEWOULDBLOCK)
        {
            close();
            throw sharp_tcp_error { "connect() failure" };
        }

        if (timeout_msecs > 0)
        {
            timeval tv;
            tv.tv_sec  = (timeout_msecs / 1000);
            tv.tv_usec = ((timeout_msecs - (tv.tv_sec * 1000)) * 1000);

            FD_SET set;
            FD_ZERO(&set);
            FD_SET(_fd, &set);

            if (::select(0, NULL, &set, NULL, &tv) == 1)
            {
                u_long mode = 0;
                if (::ioctlsocket(_fd, FIONBIO, &mode) != 0)
                {
                    close();
                    throw sharp_tcp_error { "connect() set blocking failure" };
                }
            }
            else
            {
                close();
                throw sharp_tcp_error { "connect() timed out" };
            }
        }
#else
        struct sockaddr_un server_addr_un;
        struct sockaddr_in server_addr_in;

        bool is_unix_socket = _port == 0;

        if (is_unix_socket)
        {
            std::memset(&server_addr_un, 0, sizeof(server_addr_un));
            server_addr_un.sun_family = AF_UNIX;
            strncpy(server_addr_un.sun_path, host.c_str(), sizeof(server_addr_un.sun_path) - 1);
        }
        else
        {
            struct addrinfo* result = nullptr;
            struct addrinfo hints;

            std::memset(&hints, 0, sizeof(hints));
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_family   = AF_INET;

            if (getaddrinfo(host.c_str(), nullptr, &hints, &result) != 0) throw sharp_tcp_error { "getaddrinfo() failure" };

            std::memset(&server_addr_in, 0, sizeof(server_addr_in));
            server_addr_in.sin_addr   = ((struct sockaddr_in*) (result->ai_addr))->sin_addr;
            server_addr_in.sin_port   = htons(port);
            server_addr_in.sin_family = AF_INET;

            freeaddrinfo(result);
        }

        socklen_t addr_len                 = is_unix_socket ? sizeof(server_addr_un) : sizeof(server_addr_in);
        const struct sockaddr* server_addr = is_unix_socket ? (const struct sockaddr*) &server_addr_un : (const struct sockaddr*) &server_addr_in;

        if (timeout_msecs > 0)
        {
            if (fcntl(_fd, F_SETFL, fcntl(_fd, F_GETFL, 0) | O_NONBLOCK) == -1)
            {
                close();
                throw sharp_tcp_error { "connect() set non-blocking failure" };
            }
        }

        int ret = ::connect(_fd, server_addr, addr_len);
        if (ret < 0 && errno != EINPROGRESS)
        {
            close();
            throw sharp_tcp_error { "connect() failure" };
        }

        if (timeout_msecs > 0)
        {
            timeval tv;
            tv.tv_sec  = (timeout_msecs / 1000);
            tv.tv_usec = ((timeout_msecs - (tv.tv_sec * 1000)) * 1000);

            fd_set set;
            FD_ZERO(&set);
            FD_SET(_fd, &set);

            if (::select(0, NULL, &set, NULL, &tv) == 1)
            {
                if (fcntl(_fd, F_SETFL, fcntl(_fd, F_GETFL, 0) & (~O_NONBLOCK)) == -1)
                {
                    close();
                    throw sharp_tcp_error { "connect() set blocking failure" };
                }
            }
            else
            {
                close();
                throw sharp_tcp_error { "connect() timed out" };
            }
        }
#endif
    }

    void bind(const std::string& host, std::uint32_t port)
    {
        _host = host;
        _port = port;

        create_socket_if_necessary();
        check_or_set_type(type::SERVER);

#ifdef _WIN32
        struct addrinfo* result = nullptr;

        if (getaddrinfo(host.c_str(), nullptr, nullptr, &result) != 0) throw sharp_tcp_error { "getaddrinfo() failure" };

        struct sockaddr_in server_addr;

        std::memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_addr   = ((struct sockaddr_in*) (result->ai_addr))->sin_addr;
        server_addr.sin_port   = htons(port);
        server_addr.sin_family = AF_INET;

        ::freeaddrinfo(result);

        if (::bind(_fd, (const struct sockaddr*) &server_addr, sizeof(server_addr)) == SOCKET_ERROR) throw sharp_tcp_error { "bind() failure" };
#else
        struct sockaddr_un server_addr_un;
        struct sockaddr_in server_addr_in;

        bool is_unix_socket = _port == 0;

        if (is_unix_socket)
        {
            std::memset(&server_addr_un, 0, sizeof(server_addr_un));
            server_addr_un.sun_family = AF_UNIX;
            strncpy(server_addr_un.sun_path, host.c_str(), sizeof(server_addr_un.sun_path) - 1);
        }
        else
        {
            struct addrinfo* result = nullptr;

            if (getaddrinfo(host.c_str(), nullptr, nullptr, &result) != 0) throw sharp_tcp_error { "getaddrinfo() failure" };

            std::memset(&server_addr_in, 0, sizeof(server_addr_in));
            server_addr_in.sin_addr   = ((struct sockaddr_in*) (result->ai_addr))->sin_addr;
            server_addr_in.sin_port   = htons(port);
            server_addr_in.sin_family = AF_INET;

            freeaddrinfo(result);
        }

        socklen_t addr_len                 = is_unix_socket ? sizeof(server_addr_un) : sizeof(server_addr_in);
        const struct sockaddr* server_addr = is_unix_socket ? (const struct sockaddr*) &server_addr_un : (const struct sockaddr*) &server_addr_in;

        if (::bind(_fd, server_addr, addr_len) == -1) throw sharp_tcp_error { "bind() failure" };
#endif
    }

    void listen(std::size_t max_connection_queue)
    {
        create_socket_if_necessary();
        check_or_set_type(type::SERVER);

        if (::listen(_fd, static_cast<int>(max_connection_queue)) == SOCKET_ERROR) throw sharp_tcp_error { "listen() failure" };
    }

#ifdef SHARP_TCP_USES_OPENSSL
    tcp_socket accept(const std::string &cert_file, const std::string &priv_key_file)
#else
    tcp_socket accept()
#endif
    {
        create_socket_if_necessary();
        check_or_set_type(type::SERVER);

        struct sockaddr_in client_info;
        socklen_t client_info_struct_size = sizeof(client_info);

        fd_t client_fd = ::accept(_fd, (struct sockaddr*) &client_info, &client_info_struct_size);

        if (client_fd == INVALID_FD) throw sharp_tcp_error { "accept() failure" };


#ifdef SHARP_TCP_USES_OPENSSL
        return { client_fd, ::inet_ntoa(client_info.sin_addr), client_info.sin_port, type::CLIENT, cert_file, priv_key_file };
#else
        return { client_fd, ::inet_ntoa(client_info.sin_addr), client_info.sin_port, type::CLIENT };
#endif
    }

    void close()
    {
#ifdef SHARP_TCP_USES_OPENSSL
        if (UseSSL)
        {
            if (_ssl != nullptr)
            {
                SSL_shutdown(_ssl);
                SSL_free(_ssl);
                _ssl = nullptr;
            }

            if (_ssl_context != nullptr)
            {
                SSL_CTX_free(_ssl_context);
                _ssl_context = nullptr;
            }
        }
#endif

        if (_fd != INVALID_FD)
#ifdef _WIN32
        ::closesocket(_fd);
#else
        ::close(_fd);
#endif
        _fd   = INVALID_FD;
        _type = type::UNKNOWN;
    }

public:
    const std::string& get_host() const
    {
        return _host;
    }

    std::uint32_t get_port() const
    {
        return _port;
    }

    type get_type() const
    {
        return _type;
    }

    void set_type(type t)
    {
        _type = t;
    }

    fd_t get_fd() const
    {
        return _fd;
    }

    constexpr bool is_secure() const
    {
#ifdef SHARP_TCP_USES_OPENSSL
        return UseSSL;
#else
        return false;
#endif
    }

private:
    void create_socket_if_necessary()
    {
        if (_fd != INVALID_FD) return;

        _fd   = ::socket(
#ifdef _WIN32
                         AF_INET
#else
                         _port == 0 ? AF_UNIX : AF_INET
#endif
                         , SOCK_STREAM, 0);
        _type = type::UNKNOWN;

        if (_fd == INVALID_FD) throw sharp_tcp_error { "tcp_socket::create_socket_if_necessary: socket() failure" };
    }

    void check_or_set_type(type t)
    {
        if (_type != type::UNKNOWN && _type != t) throw sharp_tcp_error { "trying to perform invalid operation on socket" };

        _type = t;
    }

    std::vector<uint8_t> recv_insecure(std::size_t size_to_read)
    {
        create_socket_if_necessary();
        check_or_set_type(type::CLIENT);

        std::vector<uint8_t> data(size_to_read, static_cast<uint8_t>(0));
        uint32_t read_idx { 0 };
        ssize_t rd_size { 0 }, total_size { 0 };

        while (true)
        {
            data.resize(size_to_read * (read_idx + 1));

            rd_size = ::recv(_fd, reinterpret_cast<char*>(const_cast<uint8_t*>(std::data(data))) + (size_to_read * read_idx), static_cast<int>(size_to_read), 0);

            if (rd_size == SOCKET_ERROR) throw sharp_tcp_error { "recv() failure" };

            total_size += rd_size;

            if (rd_size == 0 || rd_size < static_cast<ssize_t>(size_to_read)) break;

            ++read_idx;
        }

        if (!total_size) throw sharp_tcp_error { "nothing to read, socket has been closed by remote host" };

        data.resize(total_size);

        return data;
    }

private:
    fd_t _fd { INVALID_FD };
    std::string _host {};
    std::uint32_t _port { 0 };
    type _type { type::UNKNOWN };
#ifdef SHARP_TCP_USES_OPENSSL
    static inline std::atomic_bool __is_openssl_initted { false };
    SSL *_ssl { nullptr };
    SSL_CTX *_ssl_context { nullptr };
#endif
};

template <bool UseSSL = false>
class io_service
{
public:
    io_service(std::size_t nb_threads = 1) : _callback_workers(nb_threads)
    {
        _poll_worker = std::thread([&](){ poll(); });
    }

    ~io_service()
    {
        _should_stop = true;

        _notifier.notify();

        if (_poll_worker.joinable()) _poll_worker.join();

        _callback_workers.stop();
    }

    io_service(const io_service&) = delete;
    io_service& operator=(const io_service&) = delete;

    void set_nb_workers(std::size_t nb_threads)
    {
        _callback_workers.set_nb_threads(nb_threads);
    }

    void set_use_timeout(std::optional<int> timeout_usecs)
    {
        _use_timeout = timeout_usecs;
    }

    using event_callback_t = std::function<void(fd_t)>;

    void track(const tcp_socket<UseSSL>& socket, const event_callback_t& rd_callback = nullptr, const event_callback_t& wr_callback = nullptr)
    {
        std::scoped_lock lock { _tracked_sockets_mtx };

        auto& track_info              { _tracked_sockets[socket.get_fd()] };
        track_info.rd_callback        = rd_callback;
        track_info.wr_callback        = wr_callback;
        track_info.marked_for_untrack = false;

        _notifier.notify();
    }

    void set_rd_callback(const tcp_socket<UseSSL>& socket, const event_callback_t& event_callback)
    {
        std::scoped_lock lock { _tracked_sockets_mtx };

        auto& track_info       { _tracked_sockets[socket.get_fd()] };
        track_info.rd_callback = event_callback;

        _notifier.notify();
    }

    void set_wr_callback(const tcp_socket<UseSSL>& socket, const event_callback_t& event_callback)
    {
        std::scoped_lock lock { _tracked_sockets_mtx };

        auto& track_info       { _tracked_sockets[socket.get_fd()] };
        track_info.wr_callback = event_callback;

        _notifier.notify();
    }

    void untrack(const tcp_socket<UseSSL>& socket)
    {
        std::scoped_lock lock { _tracked_sockets_mtx };

        auto it { _tracked_sockets.find(socket.get_fd()) };

        if (it == std::end(_tracked_sockets)) return;

        if (it->second.is_executing_rd_callback || it->second.is_executing_wr_callback)
        {
            it->second.marked_for_untrack = true;
        }
        else
        {
            _tracked_sockets.erase(it);
            _wait_for_removal_condvar.notify_all();
        }

        _notifier.notify();
    }

    void wait_for_removal(const tcp_socket<UseSSL>& socket)
    {
        std::unique_lock lock { _tracked_sockets_mtx };

        _wait_for_removal_condvar.wait(lock, [this, &socket]()
        {
            return _tracked_sockets.find(socket.get_fd()) == _tracked_sockets.end();
        });
    }

private:
    struct tracked_socket
    {
        tracked_socket() = default;

        event_callback_t rd_callback { nullptr };
        std::atomic_bool is_executing_rd_callback { false };

        event_callback_t wr_callback { nullptr };
        std::atomic_bool is_executing_wr_callback { false };
        std::atomic_bool marked_for_untrack { false };
    };

    void poll()
    {
        while (!_should_stop)
        {
            int ndfs = init_poll_fds_info();

            struct timeval* timeout_ptr { nullptr };
            struct timeval timeout;

            if (_use_timeout.has_value())
            {
                timeout.tv_usec = _use_timeout.value();
                timeout_ptr     = &timeout;
            }

            if (::select(ndfs, &_rd_set, &_wr_set, nullptr, timeout_ptr) > 0) process_events();
        }
    }

    int init_poll_fds_info()
    {
        std::scoped_lock lock { _tracked_sockets_mtx };

        _polled_fds.clear();

        FD_ZERO(&_rd_set);
        FD_ZERO(&_wr_set);

        int ndfs = (int) _notifier.get_read_fd();

        FD_SET(_notifier.get_read_fd(), &_rd_set);
        _polled_fds.push_back(_notifier.get_read_fd());

        for (const auto& socket : _tracked_sockets)
        {
            const auto& fd          { socket.first };
            const auto& socket_info { socket.second };

            bool should_rd = socket_info.rd_callback && !socket_info.is_executing_rd_callback;

            if (should_rd) FD_SET(fd, &_rd_set);

            bool should_wr = socket_info.wr_callback && !socket_info.is_executing_wr_callback;

            if (should_wr) FD_SET(fd, &_wr_set);

            if (should_rd || should_wr || socket_info.marked_for_untrack) _polled_fds.push_back(fd);

            if ((should_rd || should_wr) && (int) fd > ndfs) ndfs = (int) fd;
        }

        return ndfs + 1;
    }

    void process_events()
    {
        std::scoped_lock lock { _tracked_sockets_mtx };

        for (const auto& fd : _polled_fds)
        {
            if (fd == _notifier.get_read_fd() && FD_ISSET(fd, &_rd_set))
            {
                _notifier.clr_buffer();

                continue;
            }

            auto it { _tracked_sockets.find(fd) };

            if (it == std::end(_tracked_sockets)) continue;

            auto& socket { it->second };

            if (FD_ISSET(fd, &_rd_set) && socket.rd_callback && !socket.is_executing_rd_callback) process_rd_event(fd, socket);
            if (FD_ISSET(fd, &_wr_set) && socket.wr_callback && !socket.is_executing_wr_callback) process_wr_event(fd, socket);

            if (socket.marked_for_untrack && !socket.is_executing_rd_callback && !socket.is_executing_wr_callback)
            {
                _tracked_sockets.erase(it);
                _wait_for_removal_condvar.notify_all();
            }
        }
    }

    void process_rd_event(const fd_t& fd, tracked_socket& socket)
    {
        auto rd_callback { socket.rd_callback };

        socket.is_executing_rd_callback = true;

        _callback_workers << [=]
        {
            rd_callback(fd);

            std::scoped_lock lock { _tracked_sockets_mtx };

            auto it { _tracked_sockets.find(fd) };

            if (it == std::end(_tracked_sockets)) return;

            auto& socket                    { it->second };
            socket.is_executing_rd_callback = false;

            if (socket.marked_for_untrack && !socket.is_executing_wr_callback)
            {
                _tracked_sockets.erase(it);
                _wait_for_removal_condvar.notify_all();
            }

            _notifier.notify();
        };
    }

    void process_wr_event(const fd_t& fd, tracked_socket& socket)
    {
        auto wr_callback { socket.wr_callback };

        socket.is_executing_wr_callback = true;

        _callback_workers << [=]
        {
            wr_callback(fd);

            std::scoped_lock lock { _tracked_sockets_mtx };

            auto it = _tracked_sockets.find(fd);

            if (it == std::end(_tracked_sockets)) return;

            auto& socket                    { it->second };
            socket.is_executing_wr_callback = false;

            if (socket.marked_for_untrack && !socket.is_executing_rd_callback)
            {
                _tracked_sockets.erase(it);
                _wait_for_removal_condvar.notify_all();
            }

            _notifier.notify();
        };
    }

private:
    std::optional<int> _use_timeout{};
    std::unordered_map<fd_t, tracked_socket> _tracked_sockets {};
    std::atomic_bool _should_stop { false };
    std::thread _poll_worker {};
    thread_pool _callback_workers;
    std::mutex _tracked_sockets_mtx {};
    std::vector<fd_t> _polled_fds {};
    fd_set _rd_set {};
    fd_set _wr_set {};
    std::condition_variable _wait_for_removal_condvar {};
    self_pipe _notifier {};
};

static inline std::shared_ptr<io_service<false>> io_service_default_instance = nullptr;
static inline std::shared_ptr<io_service<true>> io_service_ssl_default_instance = nullptr;

template <bool UseSSL = false>
const std::shared_ptr<io_service<UseSSL>>& get_default_io_service(std::uint32_t nu_io_workers = 1)
{
    if constexpr (UseSSL)
    {
        if (io_service_ssl_default_instance == nullptr) io_service_ssl_default_instance = std::make_shared<io_service<UseSSL>>(nu_io_workers);
        else io_service_ssl_default_instance->set_nb_workers(nu_io_workers);

        return io_service_ssl_default_instance;
    }
    else
    {
        if (io_service_default_instance == nullptr) io_service_default_instance = std::make_shared<io_service<UseSSL>>(nu_io_workers);
        else io_service_default_instance->set_nb_workers(nu_io_workers);

        return io_service_default_instance;
    }
}

template <bool UseSSL = false>
void set_default_io_service(const std::shared_ptr<io_service<UseSSL>>& service)
{
    if constexpr (UseSSL) io_service_ssl_default_instance = service;
    else io_service_default_instance = service;
}

template <bool UseSSL = false>
class tcp_client
{
public:
    tcp_client(uint32_t nu_io_workers = 1) : _io_service { get_default_io_service<UseSSL>(nu_io_workers) } {}

    ~tcp_client()
    {
        disconnect(true);
    }

    explicit tcp_client(tcp_socket<UseSSL>&& socket) :
        _io_service { get_default_io_service<UseSSL>() },
        _socket { std::forward<tcp_socket<UseSSL>>(socket) },
        _is_connected { true }
    {
        _io_service->track(_socket);
    }

    tcp_client(const tcp_client&) = delete;
    tcp_client& operator=(const tcp_client&) = delete;

    bool operator==(const tcp_client& rhs) const
    {
        return _socket == rhs._socket;
    }

    bool operator!=(const tcp_client& rhs) const
    {
        return !operator==(rhs);
    }

    const std::string& get_host() const
    {
        return _socket.get_host();
    }

    uint32_t get_port() const
    {
        return _socket.get_port();
    }

    void connect(const std::string& host, uint32_t port, uint32_t timeout_msecs = 0)
    {
        if (is_connected()) throw sharp_tcp_error { "tcp_client is already connected" };

        try
        {
            _socket.connect(host, port, timeout_msecs);
            _io_service->track(_socket);
        }
        catch (const sharp_tcp_error& e)
        {
            _socket.close();

            throw e;
        }

        _is_connected = true;
    }

    void disconnect(bool wait_for_removal = false)
    {
        if (!is_connected()) return;

        _is_connected = false;

        clear_read_requests();
        clear_write_requests();

        _io_service->untrack(_socket);

        if (wait_for_removal) _io_service->wait_for_removal(_socket);

        _socket.close();
    }

    bool is_connected() const
    {
        return _is_connected;
    }

private:
    void call_disconnection_handler()
    {
        if (_disconnection_handler) _disconnection_handler();
    }

public:
    struct read_result
    {
        bool success { false };
        std::vector<uint8_t> buffer {};
    };

    struct write_result
    {
        bool success { false };
        std::size_t size { 0 };
    };

    using async_read_callback_t = std::function<void(read_result&)>;
    using async_write_callback_t = std::function<void(write_result&)>;

    struct read_request
    {
        std::size_t size { 0 };
        async_read_callback_t async_read_callback {};
    };

    struct write_request
    {
        std::vector<uint8_t> buffer {};
        async_write_callback_t async_write_callback {};
    };

    void async_read(const read_request& request)
    {
        std::scoped_lock lock { _read_requests_mtx };

        if (is_connected())
        {
            _io_service->set_rd_callback(_socket, [this](auto &&fd){ on_read_available(fd); });
            _read_requests.push(request);
        }
        else {}
    }

    void async_write(const write_request& request)
    {
        std::scoped_lock lock { _write_requests_mtx };

        if (is_connected())
        {
            _io_service->set_wr_callback(_socket, [this](auto fd) { on_write_available(fd); });
            _write_requests.push(request);
        }
        else {}
    }

    tcp_socket<UseSSL>& get_socket()
    {
        return _socket;
    }

    const tcp_socket<UseSSL>& get_socket() const
    {
        return _socket;
    }

    const std::shared_ptr<io_service<UseSSL>>& get_io_service() const
    {
        return _io_service;
    }

    using disconnection_handler_t = std::function<void()>;

    void set_on_disconnection_handler(const disconnection_handler_t& disconnection_handler)
    {
        _disconnection_handler = disconnection_handler;
    }

private:
    void on_read_available(fd_t)
    {
        read_result result;
        auto callback { process_read(result) };

        if (!result.success) disconnect();
        if (callback) callback(result);
        if (!result.success) call_disconnection_handler();
    }

    void on_write_available(fd_t)
    {
        write_result result;
        auto callback { process_write(result) };

        if (!result.success) disconnect();
        if (callback) callback(result);
        if (!result.success) call_disconnection_handler();
    }

    void clear_read_requests()
    {
        std::scoped_lock lock { _read_requests_mtx };

        std::queue<read_request> empty;
        std::swap(_read_requests, empty);
    }

    void clear_write_requests()
    {
        std::scoped_lock lock { _write_requests_mtx };

        std::queue<write_request> empty;
        std::swap(_write_requests, empty);
    }

    async_read_callback_t process_read(read_result& result)
    {
        std::scoped_lock lock { _read_requests_mtx };

        if (std::empty(_read_requests)) return nullptr;

        const auto& request = _read_requests.front();
        auto callback       = request.async_read_callback;

        try
        {
            result.buffer  = std::move(_socket.recv(request.size));
            result.success = true;
        }
        catch (const sharp_tcp_error&)
        {
            result.success = false;
        }

        _read_requests.pop();

        if (std::empty(_read_requests)) _io_service->set_rd_callback(_socket, nullptr);

        return callback;
    }

    async_write_callback_t process_write(write_result& result)
    {
        std::scoped_lock lock { _write_requests_mtx };

        if (std::empty(_write_requests)) return nullptr;

        const auto& request = _write_requests.front();
        auto callback       = request.async_write_callback;

        try
        {
            result.size    = _socket.send(request.buffer, request.buffer.size());
            result.success = true;
        }
        catch (const sharp_tcp_error&)
        {
            result.success = false;
        }

        _write_requests.pop();

        if (std::empty(_write_requests)) _io_service->set_wr_callback(_socket, nullptr);

        return callback;
    }

    std::shared_ptr<io_service<UseSSL>> _io_service {};
    tcp_socket<UseSSL> _socket {};
    std::atomic_bool _is_connected { false };
    std::queue<read_request> _read_requests {};
    std::queue<write_request> _write_requests {};
    std::mutex _read_requests_mtx {};
    std::mutex _write_requests_mtx {};
    disconnection_handler_t _disconnection_handler { nullptr };
};

template<bool UseSSL = false, auto ConnectionQueueSize = 1024>
class tcp_server
{
public:
#ifdef SHARP_TCP_USES_OPENSSL
    tcp_server(const std::string &cert_file, const std::string &priv_key_file) : _io_service { get_default_io_service<UseSSL>() },
        _cert_file { cert_file }, _priv_key_file { priv_key_file }
#else
    tcp_server() : _io_service { get_default_io_service<UseSSL>() }
#endif
    {
    }

    ~tcp_server()
    {
        stop();
    }

    tcp_server(const tcp_server&) = delete;
    tcp_server& operator=(const tcp_server&) = delete;

    bool operator==(const tcp_server& rhs) const
    {
        return _socket == rhs._socket;
    }

    bool operator!=(const tcp_server& rhs) const
    {
        return !operator==(rhs);
    }

    using on_new_connection_callback_t = std::function<bool(const std::shared_ptr<tcp_client<UseSSL>>&)>;

    void start(const std::string& host, std::uint32_t port, const on_new_connection_callback_t& callback = nullptr)
    {
        if (is_running()) throw sharp_tcp_error { "tcp_server is already running" };

        _socket.bind(host, port);
        _socket.listen(ConnectionQueueSize);

        _io_service->track(_socket);
        _io_service->set_rd_callback(_socket, [this](auto &&fd) { on_read_available(fd); });
        _on_new_connection_callback = callback;

        _is_running = true;
    }

    void stop(bool wait_for_removal = false, bool recursive_wait_for_removal = true)
    {
        if (!is_running()) return;

        _is_running = false;

        _io_service->untrack(_socket);

        if (wait_for_removal) _io_service->wait_for_removal(_socket);

        _socket.close();

        std::scoped_lock lock { _clients_mtx };
        for (auto& client : _clients)
        {
            client->disconnect(recursive_wait_for_removal && wait_for_removal);
        }

        _clients.clear();
    }

    bool is_running() const
    {
        return _is_running;
    }

    tcp_socket<UseSSL>& get_socket()
    {
        return _socket;
    }

    const tcp_socket<UseSSL>& get_socket() const
    {
        return _socket;
    }

    const std::shared_ptr<io_service<UseSSL>>& get_io_service() const
    {
        return _io_service;
    }

    const std::deque<std::shared_ptr<tcp_client<UseSSL>>>& get_clients() const
    {
        return _clients;
    }

private:
    void on_read_available(fd_t)
    {
        try
        {
            auto client { std::make_shared<tcp_client<UseSSL>>(_socket.accept(
#ifdef SHARP_TCP_USES_OPENSSL
                _cert_file, _priv_key_file
#endif
            )) };

            if (!_on_new_connection_callback || !_on_new_connection_callback(client))
            {
                client->set_on_disconnection_handler([this, client]() { on_client_disconnected(client); });

                _clients.emplace_back(std::move(client));
            }
            else {}
        }
        catch (const sharp_tcp_error&)
        {
            //stop();
        }
    }

    void on_client_disconnected(const std::shared_ptr<tcp_client<UseSSL>>& client)
    {
        if (!is_running()) return;

        std::scoped_lock lock { _clients_mtx };

        if (auto it = std::find(_clients.begin(), _clients.end(), client); it != _clients.end()) _clients.erase(it);
    }

    std::shared_ptr<io_service<UseSSL>> _io_service {};
    tcp_socket<UseSSL> _socket {};
    std::atomic_bool _is_running { false };
    std::deque<std::shared_ptr<tcp_client<UseSSL>>> _clients {};
    std::mutex _clients_mtx {};
    on_new_connection_callback_t _on_new_connection_callback { nullptr };
#ifdef SHARP_TCP_USES_OPENSSL
    std::string _cert_file{}, _priv_key_file{};
#endif
};

#ifdef _WIN32
class winsock_init_base
{
protected:
    struct data
    {
        long init_count_ { 0 };
        long result_ { 0 };
    };

    static void startup(data& d, unsigned char major, unsigned char minor)
    {
        WORD version = MAKEWORD(major, minor);
        WSADATA data;

        ::WSAStartup(version, &data);
    }

    static void cleanup(data& d)
    {
        ::WSACleanup();
    }

    static void throw_on_error(data& d)
    {
        if (d.result_ != 0) throw sharp_tcp_error { "winsocket2 cannot be initialized!" };
    }
};

template <int Major = 2, int Minor = 2>
class winsock_init : private winsock_init_base
{
public:
    winsock_init(bool allow_throw = true)
    {
        startup(data_, Major, Minor);

        if (allow_throw) throw_on_error(data_);
    }

    winsock_init(const winsock_init&)
    {
        startup(data_, Major, Minor);
        throw_on_error(data_);
    }

    ~winsock_init()
    {
        cleanup(data_);
    }

private:
    static data data_;
};

template <int Major, int Minor>
winsock_init_base::data winsock_init<Major, Minor>::data_;

static inline const winsock_init<>& winsock_init_instance { winsock_init<>(false) };
#endif

#ifdef SHARP_TCP_USES_OPENSSL
template <bool>
class openssl_init_handler
{
  public:
    openssl_init_handler()
    {
        SSL_library_init();
        OpenSSL_add_all_algorithms();
        SSL_load_error_strings();
        ERR_load_BIO_strings();
        ERR_load_crypto_strings();
    }
    ~openssl_init_handler()
    {
        ERR_free_strings();
        EVP_cleanup();
        CRYPTO_cleanup_all_ex_data();
        sk_SSL_COMP_free(SSL_COMP_get_compression_methods());
    }
};

static inline openssl_init_handler<true> init_openssl {};
#endif

}
