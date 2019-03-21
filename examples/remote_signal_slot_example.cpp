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

#include <iomanip>
#include <iostream>
#include <signal.h>
#include "base64.hpp"
#include "json.hpp"
#include "relinx_generator_random.hpp"
#include "remote_signal_slot.hpp"
#include "uuid.hpp"

std::condition_variable cv;

void signint_handler(int) { cv.notify_all(); }

int main()
{
    using namespace std::literals;

    std::cout << "Press Ctrl+C to exit..." << std::endl << std::endl;

    ::signal(SIGINT, &signint_handler);

    const std::string host { "127.0.0.1"s }, signal_name { "remote_test_signal"s };
    constexpr const uint32_t port { 6789u };

    nstd::remote::remote_signal_hub remote_signals {}; // signal provider/server
    remote_signals.start(host, port);

    nstd::signal_slot::connection_bag cons;
    nstd::remote::remote_slot_hub local_slots {}; // remote signal subscriber/client
    local_slots.connect_to_remote_signal_hub(host, port);

    cons = local_slots.get_remote_signal(signal_name)->connect([](auto s, auto &&data) // defining signal processor on client side
    {
        if (data)
        {
            auto json_obj { nstd::json::json::from_cbor(*data) };

            std::cout << "remote signal: "s << s->name() << std::endl << "data: "s << std::endl << std::setw(3) << json_obj << std::endl;
        }
    });

    std::this_thread::sleep_for(30ms);

    auto random_base64 { [](){ return nstd::base64::base64_encode(nstd::from_random()->take(5)->to_vector()); } };

    nstd::signal_slot::timer_signal timer { "emitter"s, 2s };

    cons = timer.connect([&remote_signals, &random_base64, &signal_name](auto)
    {
        nstd::json::json json_obj
        {
            { "binary_data", random_base64() },
            { "client", "nstd::example_client"s },
            { "id", nstd::uuid::uuid::generate_random().to_string() },
            { "message", "...testing remote signal..."s },
            { "sequence", nstd::random_provider_default<uint32_t>()() },
            { "success", true }
        };

        auto msg { nstd::json::json::to_cbor(json_obj) };
        
        remote_signals.emit_remote_signal(signal_name, msg); // emitting remote signal
    });

    timer.start_timer();

    std::mutex mtx;
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock);

    return 0;
}
