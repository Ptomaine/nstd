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

#include "sharp_tcp.hpp"
#include "signal_slot.hpp"

namespace
{
    constexpr const uint32_t pool_size { 65536 };
}

namespace nstd::remote
{
using namespace std::literals;

class remote_signal_hub
{
public:
    void start(const std::string &host = "127.0.0.1"s, std::uint32_t port = 8)
    {
        _server.start(host, port);
    }

    void emit_remote_signal(const std::u8string &signal_name, const std::vector<uint8_t> &message)
    {
        std::vector<uint8_t> msg { std::begin(signal_name), std::end(signal_name) };

        msg.insert(std::end(msg), '\0');
        msg.insert(std::end(msg), std::begin(message), std::end(message));

        for (auto &&client : _server.get_clients())
        {
            client->async_write({ msg, nullptr });
        }
    }

private:
    nstd::net::tcp_server<pool_size> _server {};
};

/**
 * @brief Receives remote signals and delivers them to local slots
 *
 * Signals are emitted on the network thread. Slots connected through connect()
 * run on the hub's executor: by default a worker thread owned by the hub.
 */
class remote_slot_hub
{
public:
    using remote_signal_type = nstd::signal_slot::signal_ex<std::shared_ptr<std::vector<uint8_t>>>;

    remote_slot_hub() = default;

    /**
     * @brief Constructor with the executor that runs slots connected through connect()
     */
    explicit remote_slot_hub(std::shared_ptr<nstd::signal_slot::executor> delivery) : _delivery { std::move(delivery) } {}

    void connect_to_remote_signal_hub(const std::string &remote_host = "127.0.0.1"s, std::uint32_t remote_port = 8)
    {
        _client.connect(remote_host, remote_port);
        _client.async_read({pool_size, [this](auto &&result) { on_remote_signal(result); } });
    }

    /**
     * @brief Connects a slot that runs on the hub's executor
     * @param signal_name The remote signal
     * @param function Called with the signal and the received data
     */
    template<typename F>
    [[nodiscard]] nstd::signal_slot::connection connect(const std::u8string &signal_name, F &&function)
    {
        return get_remote_signal(signal_name).connect(std::forward<F>(function), _delivery);
    }

    /**
     * @brief Gets the signal for a name; slots connected to it directly run on the network thread
     */
    remote_signal_type &get_remote_signal(const std::u8string &signal_name)
    {
        std::scoped_lock lock { _signals_lock };

        return _signals[signal_name];
    }

private:
    void on_remote_signal(const nstd::net::tcp_client::read_result& result)
    {
        if (result.success)
        {
            auto begin { std::begin(result.buffer) }, end { std::end(result.buffer) }, divider_pos { std::find(begin, end, '\0') };

            if (divider_pos != end)
            {
                std::u8string signal_name { begin, divider_pos };
                remote_signal_type *signal { nullptr };

                {
                    std::scoped_lock lock { _signals_lock };

                    if (_signals.exists(signal_name)) signal = &_signals[signal_name];
                }

                if (signal) signal->emit(std::make_shared<std::vector<uint8_t>>(++divider_pos, end));
            }

            _client.async_read({pool_size, [this](auto &&res) { on_remote_signal(res); } });
        }
        else
        {
            _client.disconnect();
        }
    }

    std::mutex _signals_lock {};
    nstd::signal_slot::signal_ex_set<std::u8string, std::shared_ptr<std::vector<uint8_t>>> _signals {};
    std::shared_ptr<nstd::signal_slot::executor> _delivery { std::make_shared<nstd::signal_slot::worker_thread>() };    // after _signals: drained before they go
    nstd::net::tcp_client _client {};    // last: stopped first
};

}
