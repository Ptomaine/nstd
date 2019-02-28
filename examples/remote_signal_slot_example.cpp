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

    auto to_string = [](auto &&container) { return std::string { std::begin(container), std::end(container) }; };

    ::signal(SIGINT, &signint_handler);

    nstd::remote::remote_signal_hub remote_signals {};
    remote_signals.start();

    nstd::signal_slot::connection_bag cons;
    nstd::remote::remote_slot_hub remote_slots {};
    remote_slots.connect_to_remote_signal_hub();

    cons = remote_slots.get_remote_signal("test_signal"s)->connect([&to_string](auto s, auto &&data)
    {
        auto json_obj { nstd::json::json::from_cbor(*data) };

        std::cout << "remote signal: "s << s->name() << ", data: "s << json_obj.dump() << std::endl;
    });

    std::this_thread::sleep_for(30ms);

    nstd::json::json json_obj;

    json_obj["binary_data"] = nstd::base64::base64_encode(nstd::from_random()->take(20)->to_vector());
    json_obj["client"] = "nstd::example_client"s;
    json_obj["id"] = nstd::uuid::uuid::generate_random().to_string();
    json_obj["message"] = "...testing remote signal..."s;
    json_obj["sequence"] = nstd::random_provider_default<uint32_t>()();
    json_obj["success"] = true;

    auto msg { nstd::json::json::to_cbor(json_obj) };
    remote_signals.emit_remote_signal("test_signal"s, msg);

    std::mutex mtx;
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock);

    return 0;
}
