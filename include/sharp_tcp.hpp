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
#include <cstring>
#include <functional>
#include <list>
#include <mutex>
#include <optional>
#include <queue>
#include <stdexcept>
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

namespace nstd::net
{

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
        m_file(file),
        m_line(line) {}

    ~sharp_tcp_error() = default;

    sharp_tcp_error(const sharp_tcp_error&) = default;
    sharp_tcp_error& operator=(const sharp_tcp_error&) = default;

    const std::string& get_file() const
    {
        return m_file;
    }

    std::size_t get_line() const
    {
        return m_line;
    }

private:
    std::string m_file {};
    std::size_t m_line {};
};

class thread_pool
{
public:
    explicit thread_pool(std::size_t nb_threads) : m_nb_threads(nb_threads)
    {
        for (std::size_t i { 0 }; i < nb_threads; ++i)
        {
            m_workers.push_back(std::thread([this](){ run(); }));
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
        std::scoped_lock lock { m_tasks_mtx };

        m_tasks.push(task);
        m_tasks_condvar.notify_all();
    }

    thread_pool& operator<<(const task_t& task)
    {
        return add_task(task), *this;
    }

    void stop()
    {
        if (!is_running()) return;

        m_should_stop = true;
        m_tasks_condvar.notify_all();

        for (auto& worker : m_workers) worker.join();

        m_workers.clear();
    }

public:
    bool is_running() const
    {
        return !m_should_stop;
    }

public:
    void set_nb_threads(std::size_t nb_threads)
    {
        m_nb_threads = nb_threads;

        if (m_workers.size() < m_nb_threads)
        {
            while (m_workers.size() < m_nb_threads) m_workers.push_back(std::thread([this]() { run(); }));
        }
        else m_tasks_condvar.notify_all();
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
        std::unique_lock lock { m_tasks_mtx };

        m_tasks_condvar.wait(lock, [this] { return should_stop() || !std::empty(m_tasks); });

        if (should_stop() || std::empty(m_tasks)) return nullptr;

        task_t task = std::move(m_tasks.front());

        m_tasks.pop();

        return task;
    }

    bool should_stop(void) const
    {
        return m_should_stop || m_workers.size() > m_nb_threads;
    }

private:
    std::vector<std::thread> m_workers {};
    std::size_t m_nb_threads { 0 };
    std::atomic_bool m_should_stop { false };
    std::queue<task_t> m_tasks {};
    std::mutex m_tasks_mtx {};
    std::condition_variable m_tasks_condvar {};
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
        m_fd = ::socket(AF_INET, SOCK_DGRAM, 0);

        if (m_fd == INVALID_FD) throw sharp_tcp_error { "fail socket()" };

        u_long flags = 1;

        ioctlsocket(m_fd, FIONBIO, &flags);

        struct sockaddr_in inaddr;

        memset(&inaddr, 0, sizeof(inaddr));

        inaddr.sin_family      = AF_INET;
        inaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        inaddr.sin_port        = 0;

        if (::bind(m_fd, (struct sockaddr*) &inaddr, sizeof(inaddr)) == SOCKET_ERROR) throw sharp_tcp_error { "fail bind()" };

        m_addr_len = sizeof(m_addr);

        memset(&m_addr, 0, sizeof(m_addr));

        if (getsockname(m_fd, &m_addr, &m_addr_len) == SOCKET_ERROR) throw sharp_tcp_error { "fail getsockname()" };

        if (connect(m_fd, &m_addr, m_addr_len) == SOCKET_ERROR) throw sharp_tcp_error { "fail connect()" };
#else
        if (::pipe(m_fds) == -1) throw sharp_tcp_error { "pipe() failure" };
#endif
    }

    ~self_pipe()
    {
#ifdef _WIN32
        if (m_fd != INVALID_FD) closesocket(m_fd);
#else
        if (m_fds[0] != INVALID_FD) ::close(m_fds[0]);
        if (m_fds[1] != INVALID_FD) ::close(m_fds[1]);
#endif
    }

    self_pipe(const self_pipe&) = delete;
    self_pipe& operator=(const self_pipe&) = delete;

public:
    fd_t get_read_fd() const
    {
#ifdef _WIN32
        return m_fd;
#else
        return m_fds[0];
#endif
    }

    fd_t get_write_fd() const
    {
#ifdef _WIN32
        return m_fd;
#else
        return m_fds[1];
#endif
    }

    void notify(void)
    {
#ifdef _WIN32
        static_cast<void>(sendto(m_fd, "a", 1, 0, &m_addr, m_addr_len));
#else
        static_cast<void>(write(m_fds[1], "a", 1));
#endif
    }

    void clr_buffer(void)
    {
#ifdef _WIN32
        char buf[1024];
        static_cast<void>(recvfrom(m_fd, buf, 1024, 0, &m_addr, &m_addr_len));
#else
        char buf[1024];
        static_cast<void>(read(m_fds[0], buf, 1024));
#endif
    }

private:
#ifdef _WIN32
    fd_t m_fd { INVALID_FD };
    struct sockaddr m_addr {};;
    int m_addr_len {};
#else
    fd_t m_fds[2] { INVALID_FD, INVALID_FD };
#endif
};

#if _WIN32
#define cast_length(size) static_cast<int>(size)
#else
#define cast_length(size) size
#endif

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
    ~tcp_socket() = default;

    tcp_socket(fd_t fd, const std::string& host, std::uint32_t port, type t) : m_fd(fd), m_host(host), m_port(port), m_type(t) {}

    tcp_socket(tcp_socket&& socket) : m_fd(std::move(socket.m_fd)), m_host(socket.m_host), m_port(socket.m_port), m_type(socket.m_type)
    {
        socket.m_fd   = INVALID_FD;
        socket.m_type = type::UNKNOWN;
    }

    tcp_socket(const tcp_socket&) = delete;
    tcp_socket& operator=(const tcp_socket&) = delete;

public:
    bool operator==(const tcp_socket& rhs) const
    {
        return m_fd == rhs.m_fd && m_type == rhs.m_type;
    }

    bool operator!=(const tcp_socket& rhs) const
    {
        return !operator==(rhs);
    }

public:
    std::vector<char> recv(std::size_t size_to_read)
    {
        create_socket_if_necessary();
        check_or_set_type(type::CLIENT);

        std::vector<char> data(size_to_read, 0);

        ssize_t rd_size = ::recv(m_fd, const_cast<char*>(data.data()), cast_length(size_to_read), 0);

        if (rd_size == SOCKET_ERROR) throw sharp_tcp_error { "recv() failure" };

        if (rd_size == 0) throw sharp_tcp_error { "nothing to read, socket has been closed by remote host" };

        data.resize(rd_size);

        return data;
    }

    std::size_t send(const std::vector<char>& data, std::size_t size_to_write)
    {
        create_socket_if_necessary();
        check_or_set_type(type::CLIENT);

        ssize_t wr_size = ::send(m_fd, data.data(), cast_length(size_to_write), 0);

        if (wr_size == SOCKET_ERROR) throw sharp_tcp_error { "send() failure" };

        return wr_size;
    }

    void connect(const std::string& host, std::uint32_t port, std::uint32_t timeout_msecs = 0)
    {
#ifdef _WIN32
        m_host = host;
        m_port = port;

        create_socket_if_necessary();
        check_or_set_type(type::CLIENT);

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

            if (::ioctlsocket(m_fd, FIONBIO, &mode) != 0)
            {
              close();

              throw sharp_tcp_error { "connect() set non-blocking failure" };
            }
        }

        int ret = ::connect(m_fd, (const struct sockaddr*) &server_addr, sizeof(server_addr));

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
            FD_SET(m_fd, &set);

            if (::select(0, NULL, &set, NULL, &tv) == 1)
            {
                u_long mode = 0;
                if (::ioctlsocket(m_fd, FIONBIO, &mode) != 0)
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
        m_host = host;
        m_port = port;

        create_socket_if_necessary();
        check_or_set_type(type::CLIENT);

        struct sockaddr_un server_addr_un;
        struct sockaddr_in server_addr_in;

        bool is_unix_socket = m_port == 0;

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
            if (fcntl(m_fd, F_SETFL, fcntl(m_fd, F_GETFL, 0) | O_NONBLOCK) == -1)
            {
                close();
                throw sharp_tcp_error { "connect() set non-blocking failure" };
            }
        }

        int ret = ::connect(m_fd, server_addr, addr_len);
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
            FD_SET(m_fd, &set);

            if (select(0, NULL, &set, NULL, &tv) == 1)
            {
                if (fcntl(m_fd, F_SETFL, fcntl(m_fd, F_GETFL, 0) & (~O_NONBLOCK)) == -1)
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
#ifdef _WIN32
        m_host = host;
        m_port = port;

        create_socket_if_necessary();
        check_or_set_type(type::SERVER);

        struct addrinfo* result = nullptr;

        if (getaddrinfo(host.c_str(), nullptr, nullptr, &result) != 0) throw sharp_tcp_error { "getaddrinfo() failure" };

        struct sockaddr_in server_addr;

        std::memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_addr   = ((struct sockaddr_in*) (result->ai_addr))->sin_addr;
        server_addr.sin_port   = htons(port);
        server_addr.sin_family = AF_INET;

        ::freeaddrinfo(result);

        if (::bind(m_fd, (const struct sockaddr*) &server_addr, sizeof(server_addr)) == SOCKET_ERROR) throw sharp_tcp_error { "bind() failure" };
#else
        m_host = host;
        m_port = port;

        create_socket_if_necessary();
        check_or_set_type(type::SERVER);

        struct sockaddr_un server_addr_un;
        struct sockaddr_in server_addr_in;

        bool is_unix_socket = m_port == 0;

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

        if (::bind(m_fd, server_addr, addr_len) == -1) throw sharp_tcp_error { "bind() failure" };
#endif
    }

    void listen(std::size_t max_connection_queue)
    {
        create_socket_if_necessary();
        check_or_set_type(type::SERVER);

        if (::listen(m_fd, cast_length(max_connection_queue)) == SOCKET_ERROR) throw sharp_tcp_error { "listen() failure" };
    }

    tcp_socket accept()
    {
        create_socket_if_necessary();
        check_or_set_type(type::SERVER);

        struct sockaddr_in client_info;
        socklen_t client_info_struct_size = sizeof(client_info);

        fd_t client_fd = ::accept(m_fd, (struct sockaddr*) &client_info, &client_info_struct_size);

        if (client_fd == INVALID_FD) throw sharp_tcp_error { "accept() failure" };

        return { client_fd, ::inet_ntoa(client_info.sin_addr), client_info.sin_port, type::CLIENT };
    }

    void close()
    {
#ifdef _WIN32
        if (m_fd != INVALID_FD) ::closesocket(m_fd);
#else
        if (m_fd != INVALID_FD) ::close(m_fd);
#endif
        m_fd   = INVALID_FD;
        m_type = type::UNKNOWN;
    }

public:
    const std::string& get_host() const
    {
        return m_host;
    }

    std::uint32_t get_port() const
    {
        return m_port;
    }

    type get_type() const
    {
        return m_type;
    }

    void set_type(type t)
    {
        m_type = t;
    }

    fd_t get_fd() const
    {
        return m_fd;
    }

private:
    void create_socket_if_necessary()
    {
#ifdef _WIN32
        if (m_fd != INVALID_FD) return;

        m_fd   = socket(AF_INET, SOCK_STREAM, 0);
        m_type = type::UNKNOWN;

        if (m_fd == INVALID_FD) throw sharp_tcp_error { "tcp_socket::create_socket_if_necessary: socket() failure" };
#else
        if (m_fd != INVALID_FD) return;

        m_fd   = socket(m_port == 0 ? AF_UNIX : AF_INET, SOCK_STREAM, 0);
        m_type = type::UNKNOWN;

        if (m_fd == INVALID_FD) throw sharp_tcp_error { "tcp_socket::create_socket_if_necessary: socket() failure" };
#endif
    }

    void check_or_set_type(type t)
    {
        if (m_type != type::UNKNOWN && m_type != t) throw sharp_tcp_error { "trying to perform invalid operation on socket" };

        m_type = t;
    }

private:
    fd_t m_fd { INVALID_FD };
    std::string m_host {};
    std::uint32_t m_port { 0 };
    type m_type { type::UNKNOWN };
};

class io_service
{
public:
    io_service(std::size_t nb_threads = 1) : m_callback_workers(nb_threads)
    {
        m_poll_worker = std::thread([&](){ poll(); });
    }

    ~io_service()
    {
        m_should_stop = true;

        m_notifier.notify();

        if (m_poll_worker.joinable()) m_poll_worker.join();

        m_callback_workers.stop();
    }

    io_service(const io_service&) = delete;
    io_service& operator=(const io_service&) = delete;

    void set_nb_workers(std::size_t nb_threads)
    {
        m_callback_workers.set_nb_threads(nb_threads);
    }

    void set_use_timeout(std::optional<int> timeout_usecs)
    {
        _use_timeout = timeout_usecs;
    }

    using event_callback_t = std::function<void(fd_t)>;

    void track(const tcp_socket& socket, const event_callback_t& rd_callback = nullptr, const event_callback_t& wr_callback = nullptr)
    {
        std::scoped_lock lock { m_tracked_sockets_mtx };

        auto& track_info              { m_tracked_sockets[socket.get_fd()] };
        track_info.rd_callback        = rd_callback;
        track_info.wr_callback        = wr_callback;
        track_info.marked_for_untrack = false;

        m_notifier.notify();
    }

    void set_rd_callback(const tcp_socket& socket, const event_callback_t& event_callback)
    {
        std::scoped_lock lock { m_tracked_sockets_mtx };

        auto& track_info       { m_tracked_sockets[socket.get_fd()] };
        track_info.rd_callback = event_callback;

        m_notifier.notify();
    }

    void set_wr_callback(const tcp_socket& socket, const event_callback_t& event_callback)
    {
        std::scoped_lock lock { m_tracked_sockets_mtx };

        auto& track_info       { m_tracked_sockets[socket.get_fd()] };
        track_info.wr_callback = event_callback;

        m_notifier.notify();
    }

    void untrack(const tcp_socket& socket)
    {
        std::scoped_lock lock { m_tracked_sockets_mtx };

        auto it { m_tracked_sockets.find(socket.get_fd()) };

        if (it == std::end(m_tracked_sockets)) return;

        if (it->second.is_executing_rd_callback || it->second.is_executing_wr_callback)
        {
            it->second.marked_for_untrack = true;
        }
        else
        {
            m_tracked_sockets.erase(it);
            m_wait_for_removal_condvar.notify_all();
        }

        m_notifier.notify();
    }

    void wait_for_removal(const tcp_socket& socket)
    {
        std::unique_lock lock { m_tracked_sockets_mtx };

        m_wait_for_removal_condvar.wait(lock, [&]()
        {
            return m_tracked_sockets.find(socket.get_fd()) == m_tracked_sockets.end();
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
        while (!m_should_stop)
        {
            int ndfs = init_poll_fds_info();

            struct timeval* timeout_ptr { nullptr };
            struct timeval timeout;

            if (_use_timeout.has_value())
            {
                timeout.tv_usec = _use_timeout.value();
                timeout_ptr     = &timeout;
            }

            if (select(ndfs, &m_rd_set, &m_wr_set, nullptr, timeout_ptr) > 0) process_events();
        }
    }

    int init_poll_fds_info()
    {
        std::scoped_lock lock { m_tracked_sockets_mtx };

        m_polled_fds.clear();

        FD_ZERO(&m_rd_set);
        FD_ZERO(&m_wr_set);

        int ndfs = (int) m_notifier.get_read_fd();

        FD_SET(m_notifier.get_read_fd(), &m_rd_set);
        m_polled_fds.push_back(m_notifier.get_read_fd());

        for (const auto& socket : m_tracked_sockets)
        {
            const auto& fd          { socket.first };
            const auto& socket_info { socket.second };

            bool should_rd = socket_info.rd_callback && !socket_info.is_executing_rd_callback;

            if (should_rd) FD_SET(fd, &m_rd_set);

            bool should_wr = socket_info.wr_callback && !socket_info.is_executing_wr_callback;

            if (should_wr) FD_SET(fd, &m_wr_set);

            if (should_rd || should_wr || socket_info.marked_for_untrack) m_polled_fds.push_back(fd);

            if ((should_rd || should_wr) && (int) fd > ndfs) ndfs = (int) fd;
        }

        return ndfs + 1;
    }

    void process_events()
    {
        std::scoped_lock lock { m_tracked_sockets_mtx };

        for (const auto& fd : m_polled_fds)
        {
            if (fd == m_notifier.get_read_fd() && FD_ISSET(fd, &m_rd_set))
            {
                m_notifier.clr_buffer();

                continue;
            }

            auto it { m_tracked_sockets.find(fd) };

            if (it == std::end(m_tracked_sockets)) continue;

            auto& socket { it->second };

            if (FD_ISSET(fd, &m_rd_set) && socket.rd_callback && !socket.is_executing_rd_callback) process_rd_event(fd, socket);
            if (FD_ISSET(fd, &m_wr_set) && socket.wr_callback && !socket.is_executing_wr_callback) process_wr_event(fd, socket);

            if (socket.marked_for_untrack && !socket.is_executing_rd_callback && !socket.is_executing_wr_callback)
            {
                m_tracked_sockets.erase(it);
                m_wait_for_removal_condvar.notify_all();
            }
        }
    }

    void process_rd_event(const fd_t& fd, tracked_socket& socket)
    {
        auto rd_callback { socket.rd_callback };

        socket.is_executing_rd_callback = true;

        m_callback_workers << [=]
        {
            rd_callback(fd);

            std::scoped_lock lock { m_tracked_sockets_mtx };

            auto it { m_tracked_sockets.find(fd) };

            if (it == std::end(m_tracked_sockets)) return;

            auto& socket                    { it->second };
            socket.is_executing_rd_callback = false;

            if (socket.marked_for_untrack && !socket.is_executing_wr_callback)
            {
                m_tracked_sockets.erase(it);
                m_wait_for_removal_condvar.notify_all();
            }

            m_notifier.notify();
        };
    }

    void process_wr_event(const fd_t& fd, tracked_socket& socket)
    {
        auto wr_callback { socket.wr_callback };

        socket.is_executing_wr_callback = true;

        m_callback_workers << [=]
        {
            wr_callback(fd);

            std::scoped_lock lock { m_tracked_sockets_mtx };

            auto it = m_tracked_sockets.find(fd);

            if (it == std::end(m_tracked_sockets)) return;

            auto& socket                    { it->second };
            socket.is_executing_wr_callback = false;

            if (socket.marked_for_untrack && !socket.is_executing_rd_callback)
            {
                m_tracked_sockets.erase(it);
                m_wait_for_removal_condvar.notify_all();
            }

            m_notifier.notify();
        };
    }

private:
    std::optional<int> _use_timeout{};
    std::unordered_map<fd_t, tracked_socket> m_tracked_sockets {};
    std::atomic_bool m_should_stop { false };
    std::thread m_poll_worker {};
    thread_pool m_callback_workers;
    std::mutex m_tracked_sockets_mtx {};
    std::vector<fd_t> m_polled_fds {};
    fd_set m_rd_set {};
    fd_set m_wr_set {};
    std::condition_variable m_wait_for_removal_condvar {};
    self_pipe m_notifier {};
};

static inline std::shared_ptr<io_service> io_service_default_instance = nullptr;

const std::shared_ptr<io_service>& get_default_io_service(std::uint32_t num_io_workers = 1)
{
    if (io_service_default_instance == nullptr) io_service_default_instance = std::make_shared<io_service>(num_io_workers);
    else io_service_default_instance->set_nb_workers(num_io_workers);

    return io_service_default_instance;
}

void set_default_io_service(const std::shared_ptr<io_service>& service)
{
    io_service_default_instance = service;
}

class tcp_client
{
public:
    tcp_client(uint32_t num_io_workers = 1) : m_io_service { get_default_io_service(num_io_workers) } {}

    ~tcp_client()
    {
        disconnect(true);
    }

    explicit tcp_client(tcp_socket&& socket) :
        m_io_service { get_default_io_service() },
        m_socket { std::forward<tcp_socket>(socket) },
        m_is_connected { true }
    {
        m_io_service->track(m_socket);
    }

    tcp_client(const tcp_client&) = delete;
    tcp_client& operator=(const tcp_client&) = delete;

    bool operator==(const tcp_client& rhs) const
    {
        return m_socket == rhs.m_socket;
    }

    bool operator!=(const tcp_client& rhs) const
    {
        return !operator==(rhs);
    }

    const std::string& get_host() const
    {
        return m_socket.get_host();
    }

    uint32_t get_port() const
    {
        return m_socket.get_port();
    }

    void connect(const std::string& host, uint32_t port, uint32_t timeout_msecs = 0)
    {
        if (is_connected()) throw sharp_tcp_error { "tcp_client is already connected" };

        try
        {
            m_socket.connect(host, port, timeout_msecs);
            m_io_service->track(m_socket);
        }
        catch (const sharp_tcp_error& e)
        {
            m_socket.close();

            throw e;
        }

        m_is_connected = true;
    }

    void disconnect(bool wait_for_removal = false)
    {
        if (!is_connected()) return;

        m_is_connected = false;

        clear_read_requests();
        clear_write_requests();

        m_io_service->untrack(m_socket);

        if (wait_for_removal) m_io_service->wait_for_removal(m_socket);

        m_socket.close();
    }

    bool is_connected() const
    {
        return m_is_connected;
    }

private:
    void call_disconnection_handler()
    {
        if (m_disconnection_handler) m_disconnection_handler();
    }

public:
    struct read_result
    {
        bool success { false };
        std::vector<char> buffer {};
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
        std::vector<char> buffer {};
        async_write_callback_t async_write_callback {};
    };

    void async_read(const read_request& request)
    {
        std::scoped_lock lock { m_read_requests_mtx };

        if (is_connected())
        {
            m_io_service->set_rd_callback(m_socket, [this](auto &&fd){ on_read_available(fd); });
            m_read_requests.push(request);
        }
        else {}
    }

    void async_write(const write_request& request)
    {
        std::scoped_lock lock { m_write_requests_mtx };

        if (is_connected())
        {
            m_io_service->set_wr_callback(m_socket, [this](auto fd) { on_write_available(fd); });
            m_write_requests.push(request);
        }
        else {}
    }

    tcp_socket& get_socket()
    {
        return m_socket;
    }

    const tcp_socket& get_socket() const
    {
        return m_socket;
    }

    const std::shared_ptr<io_service>& get_io_service() const
    {
        return m_io_service;
    }

    using disconnection_handler_t = std::function<void()>;

    void set_on_disconnection_handler(const disconnection_handler_t& disconnection_handler)
    {
        m_disconnection_handler = disconnection_handler;
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
        std::scoped_lock lock { m_read_requests_mtx };

        std::queue<read_request> empty;
        std::swap(m_read_requests, empty);
    }

    void clear_write_requests()
    {
        std::scoped_lock lock { m_write_requests_mtx };

        std::queue<write_request> empty;
        std::swap(m_write_requests, empty);
    }

    async_read_callback_t process_read(read_result& result)
    {
        std::scoped_lock lock { m_read_requests_mtx };

        if (std::empty(m_read_requests)) return nullptr;

        const auto& request = m_read_requests.front();
        auto callback       = request.async_read_callback;

        try
        {
            result.buffer  = m_socket.recv(request.size);
            result.success = true;
        }
        catch (const sharp_tcp_error&)
        {
            result.success = false;
        }

        m_read_requests.pop();

        if (std::empty(m_read_requests)) m_io_service->set_rd_callback(m_socket, nullptr);

        return callback;
    }

    async_write_callback_t process_write(write_result& result)
    {
        std::scoped_lock lock { m_write_requests_mtx };

        if (std::empty(m_write_requests)) return nullptr;

        const auto& request = m_write_requests.front();
        auto callback       = request.async_write_callback;

        try
        {
            result.size    = m_socket.send(request.buffer, request.buffer.size());
            result.success = true;
        }
        catch (const sharp_tcp_error&)
        {
            result.success = false;
        }

        m_write_requests.pop();

        if (std::empty(m_write_requests)) m_io_service->set_wr_callback(m_socket, nullptr);

        return callback;
    }

    std::shared_ptr<io_service> m_io_service {};
    tcp_socket m_socket {};
    std::atomic_bool m_is_connected { false };
    std::queue<read_request> m_read_requests {};
    std::queue<write_request> m_write_requests {};
    std::mutex m_read_requests_mtx {};
    std::mutex m_write_requests_mtx {};
    disconnection_handler_t m_disconnection_handler { nullptr };
};

template<auto ConnectionQueueSize = 1024>
class tcp_server
{
public:
    tcp_server() : m_io_service { get_default_io_service() } {}

    ~tcp_server()
    {
        stop();
    }

    tcp_server(const tcp_server&) = delete;
    tcp_server& operator=(const tcp_server&) = delete;

    bool operator==(const tcp_server& rhs) const
    {
        return m_socket == rhs.m_socket;
    }

    bool operator!=(const tcp_server& rhs) const
    {
        return !operator==(rhs);
    }

    using on_new_connection_callback_t = std::function<bool(const std::shared_ptr<tcp_client>&)>;

    void start(const std::string& host, std::uint32_t port, const on_new_connection_callback_t& callback = nullptr)
    {
        if (is_running()) throw sharp_tcp_error { "tcp_server is already running" };

        m_socket.bind(host, port);
        m_socket.listen(ConnectionQueueSize);

        m_io_service->track(m_socket);
        m_io_service->set_rd_callback(m_socket, [this](auto &&fd) { on_read_available(fd); });
        m_on_new_connection_callback = callback;

        m_is_running = true;
    }

    void stop(bool wait_for_removal = false, bool recursive_wait_for_removal = true)
    {
        if (!is_running()) return;

        m_is_running = false;

        m_io_service->untrack(m_socket);

        if (wait_for_removal) m_io_service->wait_for_removal(m_socket);

        m_socket.close();

        std::scoped_lock lock { m_clients_mtx };
        for (auto& client : m_clients)
        {
            client->disconnect(recursive_wait_for_removal && wait_for_removal);
        }

        m_clients.clear();
    }

    bool is_running() const
    {
        return m_is_running;
    }

    tcp_socket& get_socket()
    {
        return m_socket;
    }

    const tcp_socket& get_socket() const
    {
        return m_socket;
    }

    const std::shared_ptr<io_service>& get_io_service() const
    {
        return m_io_service;
    }

    const std::list<std::shared_ptr<tcp_client>>& get_clients() const
    {
        return m_clients;
    }

private:
    void on_read_available(fd_t)
    {
        try
        {
            auto client = std::make_shared<tcp_client>(m_socket.accept());

            if (!m_on_new_connection_callback || !m_on_new_connection_callback(client))
            {
              client->set_on_disconnection_handler([this, client]() { on_client_disconnected(client); });

              m_clients.push_back(client);
            }
            else {}
        }
        catch (const sharp_tcp_error&)
        {
            stop();
        }
    }

    void on_client_disconnected(const std::shared_ptr<tcp_client>& client)
    {
        if (!is_running()) return;

        std::scoped_lock lock { m_clients_mtx };
        auto it = std::find(m_clients.begin(), m_clients.end(), client);

        if (it != m_clients.end()) m_clients.erase(it);
    }

    std::shared_ptr<io_service> m_io_service {};
    tcp_socket m_socket {};
    std::atomic_bool m_is_running { false };
    std::list<std::shared_ptr<tcp_client>> m_clients {};
    std::mutex m_clients_mtx {};
    on_new_connection_callback_t m_on_new_connection_callback { nullptr };
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

}
