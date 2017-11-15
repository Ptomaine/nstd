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
#include <iostream>
#include <signal.h>

std::condition_variable cv;

void signint_handler(int) { cv.notify_all(); }


void on_new_message(const std::shared_ptr<nstd::net::tcp_client>& client, const nstd::net::tcp_client::read_result& res)
{
    if (res.success)
    {
        std::cout << "Client recv data: '" << std::string { std::begin(res.buffer), std::end(res.buffer) } << "'" << std::endl;
        client->async_read({1024, [client](auto &&res) { on_new_message(client, res); }});
    }
    else
    {
        std::cout << "Client disconnected" << std::endl;
        client->disconnect();
    }
}

int main()
{
    nstd::net::tcp_server s;
    s.start("127.0.0.1", 3001, [](const std::shared_ptr<nstd::net::tcp_client>& client)
    {
        std::cout << "New client" << std::endl;

        client->async_read({1024, [client](auto &&res) { on_new_message(client, res); }});

        return true;
    });

    signal(SIGINT, &signint_handler);

    std::mutex mtx;
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock);

    return 0;
}
