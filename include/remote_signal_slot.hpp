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

    void emit_remote_signal(const std::string &signal_name, const std::vector<uint8_t> &message)
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
    nstd::net::tcp_server<65536> _server {};
};

class remote_slot_hub
{
public:
    void connect_to_remote_signal_hub(const std::string &remote_host = "127.0.0.1"s, std::uint32_t remote_port = 8)
    {
        _client.connect(remote_host, remote_port);
        _client.async_read({65536, [this](auto &&result) { on_remote_signal(result); } });
    }

    auto &get_remote_signal(const std::string &signal_name)
    {
        return _signal_queue[signal_name];
    }

private:
    void on_remote_signal(const nstd::net::tcp_client<>::read_result& result)
    {
        if (result.success)
        {
            auto divider_pos { std::find(std::begin(result.buffer), std::end(result.buffer), '\0') };

            if (divider_pos != std::end(result.buffer))
            {
                std::string signal_name { std::begin(result.buffer), divider_pos };

                if (_signal_queue.exists(signal_name))
                {
                    std::vector<uint8_t> data { ++divider_pos, std::end(result.buffer) };

                     _signal_queue[signal_name]->emit(std::ref(data));
                }
            }

            _client.async_read({65536, [this](auto &&res) { on_remote_signal(res); } });
        }
        else
        {
            _client.disconnect();
        }
    }

    nstd::net::tcp_client<> _client {};
    nstd::signal_slot::queued_signal_ex_set<std::vector<uint8_t>> _signal_queue {};
};

}
