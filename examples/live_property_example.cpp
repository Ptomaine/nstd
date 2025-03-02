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

#include <any>
#include <iostream>
#include <iomanip>
#include <typeinfo>
#include <typeindex>
#include <vector>
#include "live_property.hpp"
#include "json.hpp"

int main()
{
    using namespace std::literals;
    namespace ss = nstd::signal_slot;

    auto to_char = [](auto &&u8str) { return reinterpret_cast<const char*>(std::u8string(u8str).c_str()); };

    ss::connection_bag cons;
    ss::bridged_signal_set<std::u8string, std::string> two_step_signal_set([&to_char](auto&& s) { std::cout << "signal: '" << to_char(s->name()) << "' pushed..." << std::endl; return true; });

    using live_int = nstd::live_property<int>;
    using live_string = nstd::live_property<std::string>;

	live_int int_prop{ u8"integer property for tests"s }, dummy_int_prop{ u8"dummy"s };

    cons = two_step_signal_set[u8"tss"].connect([](auto &&data)
    {
        std::cout << "TSS: '" << data << "'" << std::endl;
    });
    two_step_signal_set[u8"tss"].emit("this is two step signal");

    auto changing_callback = [&to_char](auto &&ctx)
    {
        std::cout << "The property '" << to_char(ctx.property.name()) << "' changing: from [" << ctx.property.value() << "] to [" << ctx.new_value << "]" << std::endl;

        using value_type = typename std::decay<decltype(ctx.property.value())>::type;

        if constexpr (std::is_arithmetic<value_type>::value)
        {
            if (ctx.cancel = ctx.new_value < 0; ctx.cancel)
            {
                std::cout << "<<<negative numbers are not allowed! The change was cancelled by a slot!>>>" << std::endl;
            }
        }
        else if constexpr (std::is_same<value_type, std::string>::value)
        {
            if (ctx.cancel = std::empty(ctx.new_value); ctx.cancel)
            {
                std::cout << "<<<empty strings are not allowed! The change was cancelled by a slot!>>>" << std::endl;
            }
        }
    };
	auto changed_callback = [&to_char](auto &&property)
    {
        std::cout << "The property '" << to_char(property.name()) << "' changed to: " << property.value() << std::endl;
    };

	cons = int_prop.signal_value_changing.connect(changing_callback);
	cons = int_prop.signal_value_changed.connect(changed_callback);

	for (auto &&c : cons.connections) std::cout << "connection name: '" << to_char(c.signal().name()) << "'" << std::endl;

	int raw_int = 50;
	dummy_int_prop = 222;
	int_prop = 1;
	int_prop = 150;

	std::cout << "...temporary disabling value_changing signal..." << std::endl;
	const_cast<ss::signal_base&>(cons.connections[1].signal()).set_enabled(false);

	int_prop = raw_int;
	int_prop *= 7;

	std::cout << "...checking for oprator== works in C++17 as expected..." << std::endl;
	std::cout << "comparing int_prop == dummy_int_prop (expecting: false): " << std::boolalpha << (int_prop == dummy_int_prop) << std::endl;

	std::cout << "...enabling value_changing signal again..." << std::endl;
	const_cast<ss::signal_base&>(cons.connections[1].signal()).set_enabled(true);

	int_prop = dummy_int_prop;

	std::cout << "now comparing int_prop == dummy_int_prop (expecting: true): " << (int_prop == dummy_int_prop) << std::endl;

	int_prop = -1;
	std::cout << "int_prop = " << int_prop << std::endl;

	std::cout << "testing ++ and --: " << std::endl;
    ++int_prop;
    int_prop++;
    --int_prop;
    int_prop--;

    two_step_signal_set[u8"tss"].invoke_all();

	cons.connections.clear(); //auto-disconnect from all slots
    std::cout << "no slots are called from now on since we destroied all connections..." << std::endl << "...setting int_prop to -1 should not be restricted now..." << std::endl;

    int_prop = -1;

    std::cout << "int_prop = " << int_prop << std::endl;

    live_string str_prop{ u8"string property for tests"s, "___"s }, dummy_string_prop{ u8"dummy"s };

	cons = str_prop.signal_value_changing.connect(changing_callback);
	cons = str_prop.signal_value_changed.connect(changed_callback);

	str_prop = "Hello World!"s;
	str_prop = ""s;

	std::cout << "str_prop = " << str_prop.value() << std::endl;

    {
        {
            ss::throttled_signal<std::string> sg(u8"THROTTLED"s, 50ms);
            cons = sg.connect([&sg, &to_char](auto &&str){ std::cout << "throttle: " << str << "; " << to_char(sg.name()) << std::endl; });

            //sg.set_dispatch_all_on_destroy(false);

            constexpr int sg_count {10};
            for (auto idx{0}; idx < sg_count; ++idx)
                sg.emit("throttled signal emitted..."s);

            std::this_thread::sleep_for(300ms);

            for (auto idx{0}; idx < sg_count; ++idx)
                sg.emit("throttled signal emitted..."s);

            std::cout << "done..." << std::endl;
            if (sg.get_dispatch_all_on_destroy()) std::cout << "emitting the rest of queued signals..." << std::endl;
        }

        ss::queued_signal<std::string> sg1(u8"THREADED 1"s), sg2(u8"THREADED 2"s);
        cons = sg1.connect([](auto &&s) { std::cout << "threaded 1: " << s << std::endl; });
        cons = sg2.connect([](auto &&s) { std::cout << "threaded 2: " << s << std::endl; });

        sg1.emit("1"s); sg2.emit("2"s);
        sg1.emit("1"s); sg2.emit("2"s);
        sg1.emit("1"s); sg2.emit("2"s);

        std::this_thread::sleep_for(0.2s);

        sg1.emit("1"s); sg2.emit("2"s);
        sg1.emit("1"s); sg2.emit("2"s);
        sg1.emit("1"s); sg2.emit("2"s);
    }

	std::this_thread::sleep_for(1s);

	ss::timer_signal<live_string*> timer(u8"My timer"s, 500ms);
	using payload_t = std::function<void(const int&)>;
	timer.payload() = payload_t { [](auto &&data){ std::cout << "payload...: " << data << std::endl; } };
	int idx { 0 };
	cons = timer.connect([&idx, &to_char](auto &&s, auto &&p)
    {
        std::cout << "timer: "s << to_char(s->name()) << std::endl;
        *p = std::to_string(++idx) + " tick..."s;

        if (idx == 2)
        {
            s->timer_ms(200ms);
            *p = "...timer duration changed to 200ms"s;
        }

        if (idx >= 5)
        {
            s->disable_timer_from_slot();
            *p = "...timer stoped... sleeping for some time..."s;
        }

        auto &payload { s->payload() };

        if (payload.has_value() && payload.type() == typeid(payload_t))
        {
            auto f { std::any_cast<payload_t&>(payload) };

            f(idx);
        }
    });
	timer.start_timer(&str_prop);

	std::this_thread::sleep_for(2s);

	ss::signal<std::string> jsig(u8"JSON signal"s);

	cons = jsig.connect([](auto &&jstr)
    {
        auto j = nstd::json::json::parse(jstr);

        //std::cout << "JSON property: " << j["/JSONObject/property"_json_pointer] <<std::endl;
        std::cout << "JSON property: " << j["JSONObject"]["property"] <<std::endl;
    });

    ss::signal_ex sex { u8"Extended signal" };
    cons = sex.connect([&to_char](auto &&sg){ std::cout << to_char(sg->name()) << " was emitted!" << std::endl; });
    sex.emit();

    nstd::json::json params, rj = {{"JSONObject"s, {{"property"s, "This is the super JSON property..."s}, {"One_more_property"s, 888}, {"Niels Lohmann does amazing json for cpp", true}}}};
    params["JSONObject"s] = {{"property"s, "This is the real JSON property..."s}};

    jsig.emit(params.dump());
    jsig.emit((R"({"JSONObject": {"property": "This is the parsed JSON property..."}})"_json).dump());
    jsig.emit(rj.dump());

    std::cout << "Pretty printed JSON:" << std::endl << std::setw(3) << rj << std::endl;

    struct CallableSet
    {
        void call_me(const std::string &str) { std::cout << str << std::endl; }
    } cs;

    //signal_set<throttled_signal<std::string>> ss;
    ss::signal_set<std::u8string, const std::string&> sss;
    cons = sss[u8"/mainwindow/button/ok"s].connect([](auto &&s){ std::cout << s << std::endl; });
    cons = sss[u8"/new/channel"s].connect([](auto &&s){ std::cout << s << std::endl; });
    cons = sss[u8"/other/channel"s].connect([&cs](auto &&s) { cs.call_me(s); });
    cons = sss[u8"/broadcast/channel"s].connect(&cs, &CallableSet::call_me); // the same way to connect as in the previous line
    cons = sss[u8"/broadcast/channel"s].connect([](auto &&) { std::cout << "/broadcast/channel..." << std::endl; });
    for (auto &&sn : sss.get_signal_keys()) std::cout << "signal name: " << to_char(sn) << std::endl;
    if (sss.exists(u8"/broadcast/channel"s)) std::cout << "/broadcast/channel is created..." << std::endl;
    sss.emit("hello..."s); //broadcasting a signal to all slots of the set

    ss::signal_ex_set<std::u8string, const std::string&> sssx;
    cons = sssx[u8"key_down"s].connect([&to_char](auto &&s, auto &&v){ std::cout << "signal name: " << to_char(s->name()) << "; value: " << v << std::endl; });
    cons = sssx[u8"key_up"s].connect([&to_char](auto &&s, auto &&v){ std::cout << "signal name: " << to_char(s->name()) << "; value: " << v << std::endl; });
    sssx.emit("smart signal..."s);

    ss::queued_signal_ex_set<std::u8string, std::string> super_signal_set;
    auto executor { [&to_char](auto &&s, auto &&v){ std::cout << "SUPER SIGNAL NAME: " << to_char(s->name()) << "; value: " << v << std::endl; } };
    cons = super_signal_set[u8"super signal 1"s].connect(executor);
    cons = super_signal_set[u8"super signal 2"s].connect(executor);
    cons = super_signal_set[u8"super signal 3"s].connect(executor);

    std::cout << "broadcasting event to all signals in the set..." << std::endl;

    super_signal_set.emit("super signal value!"s);

    std::this_thread::sleep_for(0.1s);

    {
        struct mouse_event { int x, y; };
        struct keyboard_event { int key_code; std::string modifiers; };
        struct event_data { std::type_index event_data_type_index; std::any event_data; };

        ss::queued_signal_ex_set<std::u8string, event_data> ss2;

        cons = ss2[u8"mouse_move"].connect([&to_char](auto &&s, auto &&ev)
        {
            if (ev.event_data.type() == typeid(mouse_event))
            {
                auto data { std::any_cast<mouse_event>(ev.event_data) };

                std::cout << "signal: " << to_char(s->name()) << "; event hash index: " << ev.event_data_type_index.hash_code() << "; x: " << data.x << "; y: " << data.y << std::endl;
            }
            else
            {
                std::cout << "unsupported event type. expected 'mouse_event [" << std::type_index(typeid(mouse_event)).hash_code() << "]' but got " << ev.event_data_type_index.hash_code() << std::endl;
            }
        });
        cons = ss2[u8"key_down"].connect([&to_char](auto &&s, auto &&ev)
        {
            if (ev.event_data.type() == typeid(keyboard_event))
            {
                auto data { std::any_cast<keyboard_event>(ev.event_data) };

                std::cout << "signal: " << to_char(s->name()) << "; event hash index: " << ev.event_data_type_index.hash_code() << "; key code: " << data.key_code << "; mods: " << data.modifiers << std::endl;
            }
            else
            {
                std::cout << "unsupported event type. expected 'keyboard_event ['" << std::type_index(typeid(keyboard_event)).hash_code() << "]' but got " << ev.event_data_type_index.hash_code() << std::endl;
            }
        });

        auto make_event = [](auto &&ed) { return event_data { typeid(ed), std::forward<decltype(ed)>(ed) }; };

        ss2[u8"mouse_move"].emit(make_event(mouse_event { 100, 100 }));
        ss2[u8"key_down"].emit(make_event(keyboard_event { 32, "new mods..."s }));
        ss2[u8"key_down"].emit(make_event(mouse_event { 200, 200 })); // this event is supposed to be ignored

        std::this_thread::sleep_for(0.1s);
    }

    struct my_scope {};
    ss::queued_signal_scoped_set<std::u8string, my_scope, std::string> qsss;

    cons = qsss[u8"test"].connect([](auto &&s) { std::cout << s << std::endl; });

    qsss[u8"test"].emit("Hello Queued Scoped Signal Set!");

    std::cout << "exitting..." << std::endl;

    return 0;
}
