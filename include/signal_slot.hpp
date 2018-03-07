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

#include <any>
#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <deque>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>
#include <type_traits>
#include <unordered_set>

namespace nstd::signal_slot
{

using namespace std::chrono_literals;

template <typename T>
struct has_value_type
{
    T value = {};

    has_value_type() = default;
    has_value_type(has_value_type &&) = default;
    has_value_type &operator= (has_value_type &&) = default;
};

struct no_value_type
{
    no_value_type() = default;
    no_value_type(no_value_type &&) = default;
    no_value_type &operator= (no_value_type &&) = default;
};

template <typename T1>
using base_value_type = typename std::conditional<std::is_same<T1, std::void_t<>>::value, no_value_type, has_value_type<T1>>::type;

template<typename T1 = std::void_t<>, typename T2 = std::void_t<>>
class paired_ptr : public base_value_type<T1>
{
public:
	using base_type = base_value_type<T1>;
	using connected_paired_ptr_type = paired_ptr<T2, T1>;

	template<typename, typename> friend class paired_ptr;
	template<typename T12, typename T22> friend bool operator==(const paired_ptr<T12, T22> &, const paired_ptr<T12, T22> &);
	template<typename T12, typename T22> friend bool operator!=(const paired_ptr<T12, T22> &, const paired_ptr<T12, T22> &);
	template<typename T12, typename T22> friend bool operator==(const paired_ptr<T12, T22> &, std::nullptr_t);
	template<typename T12, typename T22> friend bool operator!=(const paired_ptr<T12, T22> &, std::nullptr_t);
	template<typename T12, typename T22> friend bool operator==(std::nullptr_t, const paired_ptr<T12, T22> &);
	template<typename T12, typename T22> friend bool operator!=(std::nullptr_t, const paired_ptr<T12, T22> &);

	paired_ptr()
	{
	}

	paired_ptr(connected_paired_ptr_type *ptr) : _connected_paired_ptr{ ptr }
	{
		if (_connected_paired_ptr) _connected_paired_ptr->_connected_paired_ptr = this;
	}

	paired_ptr(paired_ptr &&other) : paired_ptr{ other._connected_paired_ptr }
	{
		base_type::operator=(static_cast<base_type&&>(std::forward<paired_ptr>(other)));
		other._connected_paired_ptr = nullptr;
	}

	paired_ptr(const paired_ptr &other) = delete;
	paired_ptr & operator=(const paired_ptr &other) = delete;

	paired_ptr & operator=(paired_ptr &&other)
	{
		if (_connected_paired_ptr)
		{
			if (other._connected_paired_ptr)
				std::swap(_connected_paired_ptr->_connected_paired_ptr, other._connected_paired_ptr->_connected_paired_ptr);
			else
				_connected_paired_ptr->_connected_paired_ptr = &other;
		}
		else if (other._connected_paired_ptr)
			other._connected_paired_ptr->_connected_paired_ptr = this;

		std::swap(_connected_paired_ptr, other._connected_paired_ptr);

		base_type::operator=(static_cast<base_type&&>(std::forward<paired_ptr>(other)));

		return *this;
	}

	paired_ptr & operator=(connected_paired_ptr_type *ptr)
	{
		return *this = paired_ptr(ptr);
	}

	~paired_ptr()
	{
		disconnect();
	}

	void disconnect()
	{
		if (_connected_paired_ptr)
		{
			_connected_paired_ptr->_connected_paired_ptr = nullptr;
			_connected_paired_ptr = nullptr;
		}
	}

	const connected_paired_ptr_type * connected_ptr() const
	{
		return _connected_paired_ptr;
	}

	operator bool() const
	{
        	return (_connected_paired_ptr);
	}

	bool operator!() const
	{
        	return !(_connected_paired_ptr);
	}

protected:
	connected_paired_ptr_type *_connected_paired_ptr = nullptr;

};

template<typename T1, typename T2>
bool operator==(const paired_ptr<T1, T2> & lhs, const paired_ptr<T1, T2> & rhs)
{
    return lhs._connected_paired_ptr == rhs._connected_paired_ptr;
}
template<typename T1, typename T2>
bool operator!=(const paired_ptr<T1, T2> & lhs, const paired_ptr<T1, T2> & rhs)
{
    return !(lhs == rhs);
}
template<typename T1, typename T2>
bool operator==(const paired_ptr<T1, T2> & lhs, std::nullptr_t)
{
    return !lhs;
}
template<typename T1, typename T2>
bool operator!=(const paired_ptr<T1, T2> & lhs, std::nullptr_t)
{
    return !(lhs == nullptr);
}
template<typename T1, typename T2>
bool operator==(std::nullptr_t, const paired_ptr<T1, T2> & rhs)
{
    return rhs == nullptr;
}
template<typename T1, typename T2>
bool operator!=(std::nullptr_t, const paired_ptr<T1, T2> & rhs)
{
    return !(nullptr == rhs);
}

class slot_base
{
protected:
    template<typename... Args> friend class signal;

    paired_ptr<> _connection {};
    bool _enabled { true };

public:
    void enabled(bool is_enabled) { _enabled = is_enabled; }
    bool enabled() const { return _enabled; }
};

template<typename... Args>
class slot : public slot_base
{
protected:
    std::function<void (Args...)> _functor {};

    friend class connection;

public:
    slot() = default;
    slot(std::function<void(Args...)> &&f) : _functor{ std::forward<std::function<void(Args...)>>(f) }{}
    void operator()(const Args &... args) { _functor(args...); }
    bool is_disconnected() const { return !_connection; }
    void clear() { _connection.disconnect(); }

    bool operator == (const paired_ptr<> &s) const
    {
        return &_connection == s.connected_ptr();
    }

    bool operator == (const slot &s) const
    {
        return operator ==(s._connection);
    }
};

class signal_base
{
public:
    virtual ~signal_base() = default;
    virtual std::string_view name() const = 0;
    virtual bool enabled() const = 0;
    virtual void enabled(bool) const = 0;
    virtual size_t size() const = 0;
    virtual std::any &payload() = 0;
    virtual void enable_slot(const slot_base&, bool enabled) = 0;
    virtual void enable_slot(const paired_ptr<>&, bool enabled) = 0;
    virtual bool is_slot_enabled(const slot_base&) const = 0;
    virtual bool is_slot_enabled(const paired_ptr<>&) const = 0;
};

class connection
{
public:
    connection() = default;

    template<typename... Args>
    connection(const signal_base *signal, slot<Args...> &s) : _slot{ &s._connection }, _signal { signal }
    {
    }

    connection(connection &&other) = default;
    connection(const connection &other) = delete;
    connection &operator=(const connection &other) = delete;
    connection &operator=(connection &&other)
    {
        disconnect();

        _slot = std::move(other._slot);
        _signal = std::move(other._signal);

        return *this;
    }

    ~connection()
    {
        disconnect();
    }

    void disconnect()
    {
        if (_slot) _slot.disconnect();

        _signal = nullptr;
    }

    bool is_disconnected() const { return !_slot; }

    const signal_base &signal() const
    {
        return *_signal;
    }

    void enabled(bool enabled)
    {
        const_cast<signal_base*>(_signal)->enable_slot(_slot, enabled);
    }

    bool enabled() const
    {
        return _signal->is_slot_enabled(_slot);
    }

protected:
    paired_ptr<> _slot {};
    const signal_base *_signal { nullptr };
};

template<typename... Args>
class signal : public signal_base
{
public:
    signal() = default;
    signal(const std::string &name) : _name{ name } {}
    signal(signal &&other) = default;
    signal &operator=(signal &&other) = default;
    virtual ~signal() override = default;

    virtual void emit(const Args &... args)
    {
        if (!_enabled) return;

        std::scoped_lock lock(_emit_lock);

        if (!_enabled) return;

        {
            std::scoped_lock lock(_connect_lock);

            if (!std::empty(_pending_connections))
            {
                std::move(std::begin(_pending_connections), std::end(_pending_connections), std::back_inserter(_slots));

                _pending_connections.clear();
            }
        }

        auto end = _slots.end();

        if (!std::empty(_slots))
        {
            _slots.erase(std::remove_if(std::begin(_slots), end, [&args...](auto &callable)
            {
                if (callable.is_disconnected()) return true;

                if (callable.enabled()) callable(args...);

                return callable.is_disconnected();
            }), end);
        }
    }

    virtual connection connect(std::function<void(Args...)> &&callable)
    {
        slot<Args...> new_slot(std::forward<std::function<void(Args...)>>(callable));
        connection to_return(this, new_slot);

        std::scoped_lock lock(_connect_lock);
        auto end { std::end(_pending_connections) };

        _pending_connections.erase(std::remove_if(std::begin(_pending_connections), end, std::mem_fn(&slot<Args...>::is_disconnected)), end);
        _pending_connections.emplace_back(std::move(new_slot));

        return to_return;
    }

    template<typename T>
    connection connect(T *instance, void (T::*member_function)(Args...))
    {
        return connect([instance, member_function](Args... args) { (instance->*member_function)(args...); });
    }

    virtual void clear()
    {
        std::scoped_lock lock(_connect_lock, _emit_lock);

        _pending_connections.clear();
        _slots.clear();
    }

    virtual size_t size() const override
    {
        std::scoped_lock lock(_emit_lock);

        return _slots.size();
    }

    void name(const std::string &name)
    {
        std::scoped_lock lock(_name_lock);

        _name = name;
    }

    virtual std::string_view name() const override
    {
        std::scoped_lock lock(_name_lock);

        return _name;
    }

    virtual bool enabled() const override
    {
        return _enabled;
    }

    virtual void enabled(bool enabled) const override
    {
        _enabled = enabled;
    }

    virtual std::any &payload() override
    {
        return _payload;
    }

    virtual void enable_slot(const slot_base &slot, bool enabled) override
    {
        enable_slot(slot._connection, enabled);
    }

    virtual void enable_slot(const paired_ptr<> &slot, bool enabled) override
    {
        auto send { std::end(_slots) };
        auto sit { std::find(std::begin(_slots), send, slot) };

        if (sit != send) sit->enabled(enabled);
    }

    virtual bool is_slot_enabled(const slot_base &slot) const override
    {
        return is_slot_enabled(slot._connection);
    }

    virtual bool is_slot_enabled(const paired_ptr<> &slot) const override
    {
        auto send { std::cend(_slots) };
        auto sit { std::find(std::cbegin(_slots), send, slot) };

        if (sit != send) return sit->enabled();

        return false;
    }

protected:
    std::string _name {};
    std::vector<slot<Args...>> _slots {}, _pending_connections {};
    mutable std::mutex _connect_lock {}, _emit_lock {}, _name_lock {};
    mutable std::atomic_bool _enabled { true };
    std::any _payload;
};

template<typename... Args>
class signal_ex : public signal<signal_base*, Args...>
{
public:
    using base_class = signal<signal_base*, Args...>;

    signal_ex() = default;
    signal_ex(const std::string &name) : base_class(name) {}
    signal_ex(signal_ex &&other) = default;
    signal_ex &operator=(signal_ex &&other) = default;
    virtual ~signal_ex() override = default;

    void emit(const Args&... args)
    {
        base_class::emit(this, args...);
    }
};

template<template <typename...> typename signal_type, typename... Args>
class bridged_signal_base : public signal_type<Args...>
{
public:
    using base_class = signal_type<Args...>;

    bridged_signal_base() = default;
    bridged_signal_base(const std::string &name) : base_class(name) {}
    bridged_signal_base(const std::string &name, const std::function<bool(bridged_signal_base*)>& emit_functor) : base_class(name), _emit_functor { emit_functor } {}
    bridged_signal_base(bridged_signal_base &&other) = default;
    bridged_signal_base &operator=(bridged_signal_base &&other) = default;
    virtual ~bridged_signal_base() override = default;

    virtual void emit(const Args&... args) override
    {
        if (_bridge_enabled)
        {
            {
                if (!base_class::_enabled) return;

                std::scoped_lock lock(base_class::_emit_lock);

                if (!base_class::_enabled) return;
            }

            {
                std::scoped_lock lock(_queue_lock);

                _signal_queue.push_back(std::make_tuple(args...));
            }

            if (!_emit_functor || !_emit_functor(this)) invoke_next();
        }
        else base_class::emit(args...);
    }

    virtual void emit_sync(const Args&... args)
    {
        base_class::emit(args...);
    }

    virtual bool invoke_next()
    {
        if (std::empty(_signal_queue)) return false;

        std::scoped_lock lock(_queue_lock);

        if (std::empty(_signal_queue)) return false;

        auto args = std::move(_signal_queue.front());

        _signal_queue.pop_front();

        std::apply([this](const Args&... a){ base_class::emit(a...); }, args);

        return !std::empty(_signal_queue);
    }

    virtual void invoke_all()
    {
        if (std::empty(_signal_queue)) return;

        std::scoped_lock lock(_queue_lock);

        if (std::empty(_signal_queue)) return;

        for (auto &&args : _signal_queue) std::apply([this](const Args&... a){ base_class::emit(a...); }, args);

        _signal_queue.clear();
    }

    virtual void invoke_last_and_clear()
    {
        if (std::empty(_signal_queue)) return;

        std::scoped_lock lock(_queue_lock);

        if (std::empty(_signal_queue)) return;

        std::apply([this](const Args&... a){ base_class::emit(a...); }, _signal_queue.back());

        _signal_queue.clear();
    }

    void set_emit_functor(const std::function<bool(bridged_signal_base*)>& emit_functor)
    {
        _emit_functor = emit_functor;
    }

    const std::function<bool(bridged_signal_base*)> &get_emit_functor() const
    {
        return _emit_functor;
    }

    uint64_t get_queue_size() const
    {
        return std::size(_signal_queue);
    }

    void set_bridge_enabled(bool enabled)
    {
        _bridge_enabled = enabled;
    }

    bool get_bridge_enabled() const
    {
        return _bridge_enabled;
    }

    void clear_queue()
    {
        if (std::empty(_signal_queue)) return;

        std::scoped_lock lock(_queue_lock);

        if (std::empty(_signal_queue)) return;

        _signal_queue.clear();
    }

protected:
    std::atomic_bool _bridge_enabled { true };
    std::mutex _queue_lock {};
    std::deque<std::tuple<Args...>> _signal_queue {};
    std::function<bool(bridged_signal_base*)> _emit_functor { nullptr };
};

template<typename... Args> using bridged_signal = bridged_signal_base<signal, Args...>;
template<typename... Args> using bridged_signal_ex = bridged_signal_base<signal_ex, Args...>;

template<template <typename...> typename signal_type, typename... Args>
class throttled_signal_base : public signal_type<Args...>
{
public:
    using base_class = signal_type<Args...>;

    throttled_signal_base() = default;
    template<typename Duration>
    throttled_signal_base(const std::string &name, const Duration &throttle_ms = throttled_signal_base::_default_throttle_ms) : base_class{ name }, _throttle_ms{ std::chrono::duration_cast<std::chrono::milliseconds>(throttle_ms) } {}
    throttled_signal_base(const std::string &name) : base_class{ name } {}
    throttled_signal_base(throttled_signal_base &&other) = default;
    throttled_signal_base &operator=(throttled_signal_base &&other) = default;

    virtual ~throttled_signal_base() override
    {
        _cancelled = true;

        if (_dispatcher_thread.joinable()) _dispatcher_thread.join();

        std::scoped_lock lock(_emit_lock);

        while (!std::empty(_signal_queue))
        {
            auto args = std::move(_signal_queue.front());

            _signal_queue.pop_front();

            std::apply([this, &args](const Args&... a){ base_class::emit(a...); }, args);
        }
    }

    void emit(const Args &... args)
    {
        std::scoped_lock lock(_emit_lock);

        _signal_queue.push_back(std::make_tuple(args...));

        if (!_thread_running.exchange(true))
        {
            if (_dispatcher_thread.joinable()) _dispatcher_thread.join();

            _dispatcher_thread = std::thread([this](){ queue_dispatcher(); });
        }
    }

    template<typename Duration>
    void throttle_ms(const Duration &duration)
    {
        _throttle_ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
    }

    std::chrono::milliseconds throttle_ms() const
    {
        return _throttle_ms;
    }

protected:
    static inline std::chrono::milliseconds _default_throttle_ms { 10ms };
    std::deque<std::tuple<Args...>> _signal_queue {};
    std::mutex _emit_lock {};
    std::atomic<std::chrono::milliseconds> _throttle_ms { _default_throttle_ms };
    std::thread _dispatcher_thread {};
    std::atomic_bool _cancelled { false }, _thread_running { false };

    void queue_dispatcher()
    {
        while (!_cancelled)
        {
            {
                std::unique_lock lock(_emit_lock);

                if (_cancelled || std::empty(_signal_queue)) break;

                auto args = std::move(_signal_queue.front());

                _signal_queue.pop_front();

                std::apply([this, &args](const Args&... a){ base_class::emit(a...); }, args);

                if (std::empty(_signal_queue)) break;
            }

            std::this_thread::sleep_for(_throttle_ms.load());
        }

        _thread_running = false;
    }
};

template<typename... Args> using throttled_signal = throttled_signal_base<signal, Args...>;
template<typename... Args> using throttled_signal_ex = throttled_signal_base<signal_ex, Args...>;

struct queued_signal_default_scope {};

template<typename scope, template <typename...> typename signal_type, typename... Args>
class queued_signal_base : public signal_type<Args...>
{
public:
    using base_class = signal_type<Args...>;

    queued_signal_base() = default;
    template<typename Duration>
    queued_signal_base(const std::string &name, const Duration &throttle_ms = queued_signal_base::_default_throttle_ms) : base_class{ name } {}
    queued_signal_base(const std::string &name) : base_class{ name } {}
    queued_signal_base(queued_signal_base &&other) = default;
    queued_signal_base &operator=(queued_signal_base &&other) = default;
    queued_signal_base(const queued_signal_base &other) = delete;
    queued_signal_base &operator=(const queued_signal_base &other) = delete;

    virtual ~queued_signal_base() override
    {
        std::scoped_lock lock_(_destruct_lock);

        _cancelled = true;

        if (_dispatcher_thread.joinable()) _dispatcher_thread.join();

        std::scoped_lock lock(_emit_lock);

        auto end { std::end(_signal_queue) };

        if (!std::empty(_signal_queue))
        {
            _signal_queue.erase(std::remove_if(std::begin(_signal_queue), end, [this](auto &value)
            {
                auto &[this_, args] = value;

                if (this_ == this)
                {
                    std::apply([this, &args](const Args&... a){ base_class::emit(a...); }, args);

                    return true;
                }

                return false;
            }), end);
        }
    }

    void emit(const Args &... args)
    {
        std::scoped_lock lock(_emit_lock);

        _signal_queue.push_back({ this, std::make_tuple(args...) });

        if (!_thread_running.exchange(true))
        {
            if (_dispatcher_thread.joinable()) _dispatcher_thread.join();

            _dispatcher_thread = std::thread(&queue_dispatcher);
        }
    }

    template<typename Duration>
    static void delay_ms(const Duration &duration)
    {
        _delay_ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
    }

    static std::chrono::milliseconds delay_ms()
    {
        return _delay_ms;
    }

    static void use_delay(bool use)
    {
        _use_delay = use;
    }

    static bool use_delay()
    {
        return _use_delay;
    }

protected:
    static inline std::deque<std::tuple<queued_signal_base*, std::tuple<Args...>>> _signal_queue {};
    static inline std::mutex _emit_lock {}, _destruct_lock {};
    static inline std::atomic<std::chrono::milliseconds> _delay_ms { 0ms };
    static inline std::thread _dispatcher_thread {};
    static inline std::atomic_bool _use_delay { false }, _cancelled { false }, _thread_running { false };

    static void queue_dispatcher()
    {
        while (!_cancelled)
        {
            {
                std::unique_lock lock(_emit_lock);

                if (_cancelled || std::empty(_signal_queue)) break;

                auto value = std::move(_signal_queue.front());
                auto &[this_, args] = value;

                _signal_queue.pop_front();

                std::apply([&this_, &args](const Args&... a){ this_->base_class::emit(a...); }, args);

                if (std::empty(_signal_queue)) break;
            }

            if (_use_delay) std::this_thread::sleep_for(_delay_ms.load());
        }

        _thread_running = false;
    }
};

template<typename... Args> using queued_signal = queued_signal_base<queued_signal_default_scope, signal, Args...>;
template<typename... Args> using queued_signal_ex = queued_signal_base<queued_signal_default_scope, signal_ex, Args...>;
template<typename scope, typename... Args> using queued_signal_scoped = queued_signal_base<scope, signal, Args...>;
template<typename scope, typename... Args> using queued_signal_ex_scoped = queued_signal_base<scope, signal_ex, Args...>;

template<typename... Args>
class timer_signal : public signal<timer_signal<Args...>*, Args...>
{
public:
    using base_class = signal<timer_signal*, Args...>;

    timer_signal() = default;
    template<typename Duration>
    timer_signal(const std::string &name, const Duration &timer_ms = 1s) : base_class{ name }, _timer_ms{ std::chrono::duration_cast<std::chrono::milliseconds>(timer_ms) } {}
    timer_signal(timer_signal &&other) = default;
    timer_signal &operator=(timer_signal &&other) = default;

    virtual ~timer_signal() override
    {
        stop_timer();
    }

    void start_timer(const Args&... args)
    {
        if (!_timer_enabled.exchange(true))
        {
            _args = std::make_tuple(this, args...);
            _timer_thread = std::thread([this](){ timer_procedure(); });
        }
    }

    void stop_timer()
    {
        _timer_enabled = false;

        if (_timer_thread.joinable()) _timer_thread.join();
    }

    void disable_timer_from_slot()
    {
        _timer_enabled = false;
    }

    template<typename Duration>
    void timer_ms(const Duration &duration)
    {
        _timer_ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
    }

    std::chrono::microseconds timer_ms() const
    {
        return _timer_ms;
    }

private:
    std::atomic_bool _timer_enabled { false };
    std::thread _timer_thread {};
    std::atomic<std::chrono::milliseconds> _timer_ms { 1s };
    mutable std::mutex _emit_lock {};
    std::tuple<timer_signal*, Args...> _args {};

    void timer_procedure()
    {
        while (_timer_enabled)
        {
            {
                std::unique_lock lock(_emit_lock);

                std::apply([&](timer_signal *s, const Args&... a){ base_class::emit(s, a...); }, _args);

                if (!_timer_enabled) return;
            }

            std::this_thread::sleep_for(_timer_ms.load());
        }

        _timer_enabled = false;
    }
};

template<template <typename...> typename SignalType, typename... Args>
class signal_set_base
{
public:
    using signal_type = SignalType<Args...>;
    using signal_args_tuple_type = std::tuple<Args...>;

    signal_set_base() = default;
    virtual ~signal_set_base() = default;

    void emit(const Args &... args) const
    {
        for (auto &&s : _signals) s.second->emit(args...);
    }

    virtual const std::unique_ptr<signal_type> &get_signal(const std::string &signal_name)
    {
        auto signal { _signals.find(signal_name) };

        if (signal != _signals.end()) return signal->second;

        return _signals.emplace(signal_name, std::make_unique<signal_type>(signal_name)).first->second;
    }

    bool exists(const std::string &signal_name) const
    {
        return _signals.find(signal_name) != _signals.end();
    }

    std::unordered_set<std::string> get_signal_names() const
    {
        std::unordered_set<std::string> signal_names;

        for (auto &&s : _signals) signal_names.emplace(s.first);

        return signal_names;
    }

    const std::unique_ptr<signal_type> &operator[](const std::string &signal_name)
    {
        return get_signal(signal_name);
    }

protected:
    std::unordered_map<std::string, std::unique_ptr<signal_type>> _signals {};
};

template<template <typename...> typename BridgedSignalType, typename... Args>
class bridged_signal_set_base : public signal_set_base<BridgedSignalType, Args...>
{
public:
    using base_class = signal_set_base<BridgedSignalType, Args...>;

    bridged_signal_set_base(const std::function<bool(typename base_class::signal_type::bridged_signal_base*)>& emit_functor) : base_class {}, _emit_functor { emit_functor } {}

    virtual const std::unique_ptr<typename base_class::signal_type> &get_signal(const std::string &signal_name) override
    {
        auto &signal { base_class::get_signal(signal_name) };

        if (!signal->get_emit_functor() && _emit_functor) signal->set_emit_functor(_emit_functor);

        return signal;
    }

    void set_emit_functor(const std::function<bool(typename base_class::signal_type::bridged_signal_base*)>& emit_functor)
    {
        _emit_functor = emit_functor;
    }

    const std::function<bool(typename base_class::signal_type::bridged_signal_base*)> &get_emit_functor() const
    {
        return _emit_functor;
    }

protected:
    std::function<bool(typename base_class::signal_type::bridged_signal_base*)> _emit_functor { nullptr };
};

template<typename... Args> using signal_set = signal_set_base<signal, Args...>;
template<typename... Args> using signal_ex_set = signal_set_base<signal_ex, Args...>;
template<typename... Args> using throttled_signal_set = signal_set_base<throttled_signal, Args...>;
template<typename... Args> using throttled_signal_ex_set = signal_set_base<throttled_signal_ex, Args...>;
template<typename... Args> using queued_signal_set = signal_set_base<queued_signal, Args...>;
template<typename... Args> using queued_signal_ex_set = signal_set_base<queued_signal_ex, Args...>;
template<typename... Args> using bridged_signal_set = bridged_signal_set_base<bridged_signal, Args...>;
template<typename... Args> using bridged_signal_ex_set = bridged_signal_set_base<bridged_signal_ex, Args...>;

struct connection_bag
{
    std::deque<connection> connections;

    connection_bag &operator= (connection &&c) { connections.push_front(std::forward<connection>(c)); return *this; }
};

}

namespace std
{
    template<typename... Args>
    struct hash<std::shared_ptr<nstd::signal_slot::signal<Args...>>>
    {
        size_t operator()(const std::shared_ptr<nstd::signal_slot::signal<Args...>> &s) const
        {
            return std::hash<std::string_view>()(s->name());
        }
    };
}
