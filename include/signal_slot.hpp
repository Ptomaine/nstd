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
#include <concepts>
#include <condition_variable>
#include <deque>
#include <functional>
#include <iterator>
#include <mutex>
#include <set>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace nstd::signal_slot
{

using namespace std::chrono_literals;

template <typename T>
struct has_value_type
{
    T value = {};

    has_value_type() = default;
    has_value_type(has_value_type &&) noexcept = default;
    has_value_type &operator= (has_value_type &&) noexcept = default;
};

struct no_value_type
{
    no_value_type() = default;
    no_value_type(no_value_type &&) noexcept = default;
    no_value_type &operator= (no_value_type &&) noexcept = default;
};

template <typename T1>
using base_value_type = typename std::conditional_t<std::is_same_v<T1, std::void_t<>>, no_value_type, has_value_type<T1>>;

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

	paired_ptr(paired_ptr &&other) noexcept : paired_ptr{ other._connected_paired_ptr }
	{
		base_type::operator=(static_cast<base_type&&>(std::forward<paired_ptr>(other)));
		other._connected_paired_ptr = nullptr;
	}

	paired_ptr(const paired_ptr &other) = delete;
	paired_ptr & operator=(const paired_ptr &other) = delete;

	paired_ptr & operator=(paired_ptr &&other) noexcept
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
    void set_enabled(bool is_enabled) { _enabled = is_enabled; }
    bool is_enabled() const { return _enabled; }
};

template<typename... Args>
class slot : public slot_base
{
protected:
    std::function<void (Args...)> _functor {};
    int64_t _priority{ 0 };

    friend class connection;

public:
    slot() = default;
    slot(std::function<void(Args...)>&& f, int64_t priority = 0) : _functor{ std::forward<std::function<void(Args...)>>(f) }, _priority{ priority } {}
    void invoke(const Args &... args) const { _functor(args...); }
    void operator()(const Args &... args) const { invoke(args...); }
    bool is_disconnected() const { return !_connection; }
    void clear() { _connection.disconnect(); }
    int64_t get_priority() const { return _priority; }
    int64_t set_priority(int64_t priority) { std::swap(_priority, priority); return priority; }

    bool operator == (const paired_ptr<> &s) const
    {
        return &_connection == s.connected_ptr();
    }

    bool operator == (const slot &s) const
    {
        return operator ==(s._connection);
    }

    bool operator < (const slot& s) const
    {
        return _priority < s._priority;
    }

    bool operator > (const slot& s) const
    {
        return _priority > s._priority;
    }
};

class signal_base
{
public:
    virtual ~signal_base() = default;
    virtual std::u8string_view name() const = 0;
    virtual void set_enabled(bool) = 0;
    virtual bool is_enabled() const = 0;
    virtual size_t size() const = 0;
    virtual std::any &payload() = 0;
    virtual void enable_slot(const slot_base&, bool enabled) = 0;
    virtual void enable_slot(const paired_ptr<>&, bool enabled) = 0;
    virtual bool is_slot_enabled(const slot_base&) const = 0;
    virtual bool is_slot_enabled(const paired_ptr<>&) const = 0;
    virtual int64_t get_connection_priority(const paired_ptr<>&) const = 0;
    virtual bool set_connection_priority(const paired_ptr<>&, int64_t) = 0;
};

class connection
{
public:
    connection() = default;

    template<typename... Args>
    connection(const signal_base *signal, slot<Args...> &s) : _slot{ &s._connection }, _signal { signal }
    {
    }

    connection(connection &&other) noexcept = default;
    connection(const connection &other) = delete;
    connection &operator=(const connection &other) = delete;
    connection &operator=(connection &&other) noexcept
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

    void set_enabled(bool enabled)
    {
        const_cast<signal_base*>(_signal)->enable_slot(_slot, enabled);
    }

    bool is_enabled() const
    {
        return _signal->is_slot_enabled(_slot);
    }

    int64_t get_connection_priority() const
    {
        return _signal->get_connection_priority(_slot);
    }

    bool set_connection_priority(int64_t priority)
    {
        return const_cast<signal_base*>(_signal)->set_connection_priority(_slot, priority);
    }

protected:
    paired_ptr<> _slot {};
    const signal_base *_signal { nullptr };
};

template<typename... Args>
class signal : public signal_base
{
public:
    using slot_type = slot<Args...>;
    using slot_container = std::multiset<slot_type, std::less<slot<Args...>>, std::allocator<slot<Args...>>>;

    signal() = default;
    signal(const std::u8string &name) : _name{ name } {}
    signal(signal &&other) noexcept = default;
    signal &operator=(signal &&other) noexcept = default;
    virtual ~signal() override = default;

    void emit(const Args &... args)
    {
        if (!_enabled) return;

        std::scoped_lock<std::mutex> lock_emit { _emit_lock };

        if (!_enabled) return;

        {
            std::scoped_lock<std::mutex> lock_con { _connect_lock };

            if (!std::empty(_pending_connections))
            {
                std::move(std::begin(_pending_connections), std::end(_pending_connections), std::inserter(_slots, std::end(_slots)));

                _pending_connections.clear();
            }
        }

        if (!std::empty(_slots))
        {
            auto end = _slots.end();

            std::erase_if(_slots, [&args...](auto &callable)
            {
                if (callable.is_disconnected()) return true;

                if (callable.is_enabled()) callable.invoke(args...);

                return callable.is_disconnected();
            });
        }
    }

    void operator() (const Args &... args)
    {
        emit(args...);
    }

    virtual connection connect(std::function<void(Args...)> &&callable, int64_t priority = 0)
    {
        std::scoped_lock<std::mutex> lock { _connect_lock };
        auto end { std::end(_pending_connections) };

        _pending_connections.erase(std::remove_if(std::begin(_pending_connections), end, std::mem_fn(&slot<Args...>::is_disconnected)), end);
        _pending_connections.emplace_back(std::forward<std::function<void(Args...)>>(callable), priority);

        return { this, _pending_connections.back() };
    }

    virtual connection operator += (std::function<void(Args...)> &&callable)
    {
        return connect(std::forward<std::function<void(Args...)>>(callable));
    }

    template<typename T>
    connection connect(T *instance, void (T::*member_function)(Args...), int64_t priority = std::numeric_limits<int64_t>::max() / 2)
    {
        return connect([instance, member_function](Args... args) { (instance->*member_function)(args...); }, priority);
    }

    virtual void clear()
    {
        std::scoped_lock<std::mutex, std::mutex> lock { _connect_lock, _emit_lock };

        _pending_connections.clear();
        _slots.clear();
    }

    virtual size_t size() const override
    {
        std::scoped_lock<std::mutex> lock { _emit_lock };

        return std::size(_slots);
    }

    void name(const std::u8string &name)
    {
        std::unique_lock<std::shared_mutex> lock { _name_lock };

        _name = name;
    }

    virtual std::u8string_view name() const override
    {
        std::shared_lock<std::shared_mutex> lock { _name_lock };

        return _name;
    }

    virtual void set_enabled(bool enabled) override
    {
        _enabled = enabled;
    }

    virtual bool is_enabled() const override
    {
        return _enabled;
    }

    virtual std::any &payload() override
    {
        return _payload;
    }

    virtual void enable_slot(const slot_base &slot, bool enabled) override
    {
        enable_slot(slot._connection, enabled);
    }

    virtual void enable_slot(const paired_ptr<> &s, bool enabled) override
    {
        auto send { std::end(_slots) };
        auto sit { std::find(std::begin(_slots), send, s) };
        auto& slt{ const_cast<slot<Args...>&>(*sit)};

        if (sit != send) slt.set_enabled(enabled);
    }

    virtual bool is_slot_enabled(const slot_base &slot) const override
    {
        return is_slot_enabled(slot._connection);
    }

    virtual bool is_slot_enabled(const paired_ptr<> &slot) const override
    {
        if (auto s_ptr = find_slot(slot); s_ptr != nullptr) return s_ptr->is_enabled();

        return false;
    }

    virtual int64_t get_connection_priority(const paired_ptr<>& con) const override
    {
        if (auto s_ptr = find_slot(con); s_ptr != nullptr) return s_ptr->get_priority();

        return {};
    }

    virtual bool set_connection_priority(const paired_ptr<>& con, int64_t priority) override
    {
        if (auto s_ptr = find_slot(con); s_ptr != nullptr)
        {
            const_cast<slot_type*>(s_ptr)->set_priority(priority);

            return true;
        }

        return false;
    }

protected:
    std::u8string _name {};
    std::vector<slot<Args...>> _pending_connections {};
    slot_container _slots {};
    mutable std::mutex _connect_lock {}, _emit_lock {};
    mutable std::shared_mutex _name_lock {};
    mutable std::atomic_bool _enabled { true };
    std::any _payload;

    const slot_type* find_slot(const paired_ptr<>& slot) const
    {
        auto send{ std::cend(_slots) };
        auto sit{ std::find(std::cbegin(_slots), send, slot) };

        if (sit != send) return &(*sit);

        auto vsend{ std::cend(_pending_connections) };
        auto vsit{ std::find(std::cbegin(_pending_connections), vsend, slot) };

        if (vsit != vsend) return &(*vsit);

        return nullptr;
    }
};

template<typename... Args>
class signal_ex : public signal<signal_base*, Args...>
{
public:
    using base_class = signal<signal_base*, Args...>;

    signal_ex() = default;
    signal_ex(const std::u8string &name) : base_class(name) {}
    signal_ex(signal_ex &&other) noexcept = default;
    signal_ex &operator=(signal_ex &&other) noexcept = default;
    virtual ~signal_ex() override = default;

    void emit(const Args&... args)
    {
        base_class::emit(this, args...);
    }

    void operator() (const Args&... args)
    {
        emit(args...);
    }
};

template<template <typename...> typename signal_type, typename... Args>
requires std::derived_from<signal_type<Args...>, signal_base>
class bridged_signal_base : public signal_type<Args...>
{
public:
    using base_class = signal_type<Args...>;
    using bridged_signal_type = bridged_signal_base;

    bridged_signal_base() = default;
    bridged_signal_base(const std::u8string &name) : base_class { name } {}
    bridged_signal_base(const std::u8string &name, const std::function<bool(bridged_signal_base*)>& emit_functor) : base_class { name }, _emit_functor { emit_functor } {}
    bridged_signal_base(const std::function<bool(bridged_signal_base*)>& emit_functor) : bridged_signal_base { std::u8string {}, emit_functor } {}
    bridged_signal_base(const std::u8string &name, bool bridge_enabled) : base_class { name }, _bridge_enabled { bridge_enabled } {}
    bridged_signal_base(bool bridge_enabled) : bridged_signal_base { std::u8string {}, bridge_enabled } {}
    bridged_signal_base(bridged_signal_base &&other) noexcept = default;
    bridged_signal_base &operator=(bridged_signal_base &&other) noexcept = default;
    virtual ~bridged_signal_base() override = default;

    void emit(const Args&... args)
    {
        if (!base_class::_enabled) return;

        if (_bridge_enabled)
        {
            {
                std::scoped_lock<std::mutex> lock { _queue_lock };

                _signal_queue.push_back(std::make_tuple(args...));
            }

            if (!_emit_functor || !_emit_functor(this)) invoke_next();
        }
        else base_class::emit(args...);
    }

    void operator() (const Args&... args)
    {
        emit(args...);
    }

    void emit_sync(const Args&... args)
    {
        base_class::emit(args...);
    }

    virtual bool invoke_next()
    {
        if (std::empty(_signal_queue)) return false;

        std::scoped_lock<std::mutex> lock { _queue_lock };

        if (std::empty(_signal_queue)) return false;

        std::apply([this](const Args&... a){ base_class::emit(a...); }, _signal_queue.front());

        _signal_queue.pop_front();

        return !std::empty(_signal_queue);
    }

    virtual void invoke_all()
    {
        if (std::empty(_signal_queue)) return;

        std::scoped_lock<std::mutex> lock { _queue_lock };

        if (std::empty(_signal_queue)) return;

        for (auto &&args : _signal_queue) std::apply([this](const Args&... a){ base_class::emit(a...); }, args);

        _signal_queue.clear();
    }

    virtual void invoke_last_and_clear()
    {
        if (std::empty(_signal_queue)) return;

        std::scoped_lock<std::mutex> lock { _queue_lock };

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
        std::scoped_lock<std::mutex> lock { _queue_lock };

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

        std::scoped_lock<std::mutex> lock { _queue_lock };

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
requires std::derived_from<signal_type<Args...>, signal_base>
class throttled_signal_base : public signal_type<Args...>
{
public:
    using base_class = signal_type<Args...>;

    throttled_signal_base() = default;
    template<typename Duration>
    throttled_signal_base(const std::u8string &name, const Duration &throttle_ms = throttled_signal_base::_default_throttle_ms) : base_class{ name }, _throttle_ms{ std::chrono::duration_cast<std::chrono::milliseconds>(throttle_ms) } {}
    throttled_signal_base(const std::u8string &name) : base_class{ name } {}
    throttled_signal_base(throttled_signal_base &&other) noexcept = default;
    throttled_signal_base &operator=(throttled_signal_base &&other) noexcept = default;

    virtual ~throttled_signal_base() override
    {
        if (_dispatch_all_on_destroy)
        {
            std::scoped_lock<std::mutex> lock { _emit_lock };

            while (!std::empty(_signal_queue))
            {
                std::apply([this](const Args&... a){ base_class::emit(a...); }, _signal_queue.front());

                _signal_queue.pop_front();
            }
        }
    }

    void emit(const Args &... args)
    {
        std::scoped_lock<std::mutex> lock { _emit_lock };

        _signal_queue.push_back(std::make_tuple(args...));

        if (!_thread_running.exchange(true))
        {
            if (_dispatcher_thread.joinable()) _dispatcher_thread.join();

            _dispatcher_thread = std::jthread([this](std::stop_token cancelled){ queue_dispatcher(std::move(cancelled)); });
        }
    }

    void operator() (const Args &... args)
    {
        emit(args...);
    }

    template<typename Duration>
    void throttle_ms(const Duration &duration)
    {
        _throttle_ms.store(std::chrono::duration_cast<std::chrono::milliseconds>(duration));
    }

    std::chrono::milliseconds throttle_ms() const
    {
        return _throttle_ms.load();
    }

    void set_dispatch_all_on_destroy(bool do_dispatch)
    {
        _dispatch_all_on_destroy = do_dispatch;
    }

    bool get_dispatch_all_on_destroy() const
    {
        return _dispatch_all_on_destroy;
    }

protected:
    static inline std::chrono::milliseconds _default_throttle_ms { 10ms };
    std::deque<std::tuple<Args...>> _signal_queue {};
    std::mutex _emit_lock {};
    std::atomic<std::chrono::milliseconds> _throttle_ms { _default_throttle_ms };
    std::jthread _dispatcher_thread {};
    std::atomic_bool _thread_running { false }, _dispatch_all_on_destroy { true };

    void queue_dispatcher(std::stop_token cancelled)
    {
        while (!cancelled.stop_requested())
        {
            {
                std::scoped_lock<std::mutex> lock { _emit_lock };

                if (cancelled.stop_requested() || std::empty(_signal_queue)) break;

                std::apply([this](const Args&... a){ base_class::emit(a...); }, _signal_queue.front());

                _signal_queue.pop_front();

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
requires std::derived_from<signal_type<Args...>, signal_base>
class queued_signal_base : public signal_type<Args...>
{
public:
    using base_class = signal_type<Args...>;

    queued_signal_base() = default;
    template<typename Duration>
    queued_signal_base(const std::u8string &name, const Duration &throttle_ms = queued_signal_base::_default_throttle_ms) : base_class{ name } {}
    queued_signal_base(const std::u8string &name) : base_class{ name } {}
    queued_signal_base(queued_signal_base &&other) noexcept = default;
    queued_signal_base &operator=(queued_signal_base &&other) noexcept = default;
    queued_signal_base(const queued_signal_base &other) = delete;
    queued_signal_base &operator=(const queued_signal_base &other) = delete;

    virtual ~queued_signal_base() override
    {
        std::scoped_lock<std::mutex> lock_ { _destruct_lock };

        if (_dispatch_all_on_destroy)
        {
            std::scoped_lock<std::mutex> lock { _emit_lock };

            auto end { std::end(_signal_queue) };

            if (!std::empty(_signal_queue))
            {
                _signal_queue.erase(std::remove_if(std::begin(_signal_queue), end, [this](auto &value)
                {
                    auto &[this_, args] = value;

                    if (this_ == this)
                    {
                        std::apply([this](const Args&... a){ base_class::emit(a...); }, args);

                        return true;
                    }

                    return false;
                }), end);
            }
        }
    }

    void emit(const Args &... args)
    {
        std::scoped_lock<std::mutex> lock { _emit_lock };

        _signal_queue.push_back({ this, std::make_tuple(args...) });

        if (!_thread_running.exchange(true))
        {
            if (_dispatcher_thread.joinable()) _dispatcher_thread.join();

            _dispatcher_thread = std::jthread(&queue_dispatcher);
        }
    }

    void operator() (const Args &... args)
    {
        emit(args...);
    }

    template<typename Duration>
    static void delay_ms(const Duration &duration)
    {
        _delay_ms.store(std::chrono::duration_cast<std::chrono::milliseconds>(duration));
    }

    static std::chrono::milliseconds delay_ms()
    {
        return _delay_ms.load();
    }

    static void use_delay(bool use)
    {
        _use_delay = use;
    }

    static bool use_delay()
    {
        return _use_delay;
    }

    void set_dispatch_all_on_destroy(bool do_dispatch)
    {
        _dispatch_all_on_destroy = do_dispatch;
    }

    bool get_dispatch_all_on_destroy()
    {
        return _dispatch_all_on_destroy;
    }

protected:
    static inline std::deque<std::tuple<queued_signal_base*, std::tuple<Args...>>> _signal_queue {};
    static inline std::mutex _emit_lock {}, _destruct_lock {};
    static inline std::atomic<std::chrono::milliseconds> _delay_ms { 0ms };
    static inline std::jthread _dispatcher_thread {};
    static inline std::atomic_bool _use_delay { false }, _thread_running { false }, _dispatch_all_on_destroy { true };

    static void queue_dispatcher(std::stop_token cancelled)
    {
        while (cancelled.stop_requested() == false)
        {
            {
                std::scoped_lock<std::mutex> lock { _emit_lock };

                if (cancelled.stop_requested() || std::empty(_signal_queue)) break;

                auto &[this_, args] = _signal_queue.front();

                std::apply([&this_](const Args&... a){ this_->base_class::emit(a...); }, args);

                _signal_queue.pop_front();

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
    timer_signal(const std::u8string &name, const Duration &timer_ms = 1s) : base_class{ name }, _timer_ms{ std::chrono::duration_cast<std::chrono::milliseconds>(timer_ms) } {}
    timer_signal(timer_signal &&other) noexcept = default;
    timer_signal &operator=(timer_signal &&other) noexcept = default;

    virtual ~timer_signal() override
    {
        stop_timer();
    }

    void start_timer(const Args&... args)
    {
        if (!_timer_enabled.exchange(true))
        {
            _args = std::make_tuple(this, args...);

            if (_timer_thread.joinable()) _timer_thread.join();

            _sleep.reset();

            _timer_thread = std::jthread([this](){ timer_procedure(); });
        }
    }

    void stop_timer()
    {
        _sleep.cancel_wait();

        if (_timer_thread.joinable()) _timer_thread.join();
    }

    void disable_timer_from_slot()
    {
        _sleep.cancel_wait();
    }

    template<typename Duration>
    void timer_ms(const Duration &duration)
    {
        _timer_ms.store(std::chrono::duration_cast<std::chrono::milliseconds>(duration));
    }

    std::chrono::microseconds timer_ms() const
    {
        return _timer_ms.load();
    }

private:

    std::atomic_bool _timer_enabled { false };
    std::jthread _timer_thread {};
    std::atomic<std::chrono::milliseconds> _timer_ms { 1s };
    mutable std::mutex _emit_lock {};
    std::tuple<timer_signal*, Args...> _args {};
    class
    {
    public:

        bool wait_for(const std::chrono::milliseconds &duration)
        {
            std::unique_lock<std::mutex> lock { _m };

            return !_cv.wait_for(lock, duration, [this]{ return _cancelled.load(); });
        }

        void cancel_wait()
        {
            _cancelled = true;

            _cv.notify_all();
        }

        void reset()
        {
            _cancelled = false;
        }

    private:

        std::condition_variable _cv {};
        std::mutex _m {};
        std::atomic_bool _cancelled { false };
    } _sleep;


    void timer_procedure()
    {
        while (_sleep.wait_for(_timer_ms.load()))
        {
            {
                std::scoped_lock<std::mutex> lock { _emit_lock };

                std::apply([this](timer_signal *s, const Args&... a){ base_class::emit(s, a...); }, _args);
            }
        }

        _timer_enabled = false;
    }
};

template<typename Key, template <typename...> typename SignalType, typename... Args>
requires std::derived_from<SignalType<Args...>, signal_base>
class signal_set_base
{
public:
    using signal_type = SignalType<Args...>;
    using key_type = Key;

    signal_set_base() = default;
    virtual ~signal_set_base() = default;

    void emit(const Args &... args) const
    {
        for (auto &&s : _signals) s.second->emit(args...);
    }

    void operator() (const Args &... args) const
    {
        emit(args...);
    }

    virtual signal_type &get_signal(const key_type &key)
    {
        auto signal { _signals.find(key) };

        if (signal != _signals.end()) return *signal->second;

        if constexpr (std::is_same_v<std::decay_t<key_type>, std::u8string>) return *_signals.emplace(key, std::make_unique<signal_type>(key)).first->second;

        return *_signals.emplace(key, std::make_unique<signal_type>()).first->second;
    }

    bool exists(const key_type &key) const
    {
        return _signals.find(key) != _signals.end();
    }

    std::unordered_set<key_type> get_signal_keys() const
    {
        std::unordered_set<key_type> signal_keys;

        for (const auto &[key, _] : _signals) signal_keys.insert(key);

        return signal_keys;
    }

    auto get_signal_count() const
    {
        return std::size(_signals);
    }

    signal_type &operator[](const key_type &key)
    {
        return get_signal(key);
    }

    auto begin() const
    {
        return std::begin(_signals);
    }

    auto end() const
    {
        return std::end(_signals);
    }

protected:
    std::unordered_map<key_type, std::unique_ptr<signal_type>> _signals {};
};

template<typename Key, template <typename...> typename BridgedSignalType, typename... Args>
requires std::derived_from<BridgedSignalType<Args...>, signal_base>
class bridged_signal_set_base : public signal_set_base<Key, BridgedSignalType, Args...>
{
public:
    using base_class = signal_set_base<Key, BridgedSignalType, Args...>;
    using key_type = Key;

    bridged_signal_set_base(const std::function<bool(typename base_class::signal_type::bridged_signal_type*)>& emit_functor) : base_class {}, _emit_functor { emit_functor } {}

    virtual typename base_class::signal_type &get_signal(const key_type &key) override
    {
        auto &signal { base_class::get_signal(key) };

        if (!signal.get_emit_functor() && _emit_functor) signal.set_emit_functor(_emit_functor);

        return signal;
    }

    void set_emit_functor(const std::function<bool(typename base_class::signal_type::bridged_signal_type*)>& emit_functor)
    {
        _emit_functor = emit_functor;
    }

    const std::function<bool(typename base_class::signal_type::bridged_signal_type*)> &get_emit_functor() const
    {
        return _emit_functor;
    }

protected:
    std::function<bool(typename base_class::signal_type::bridged_signal_type*)> _emit_functor { nullptr };
};

template<typename Key, typename... Args> using signal_set = signal_set_base<Key, signal, Args...>;
template<typename Key, typename... Args> using signal_ex_set = signal_set_base<Key, signal_ex, Args...>;
template<typename Key, typename... Args> using throttled_signal_set = signal_set_base<Key, throttled_signal, Args...>;
template<typename Key, typename... Args> using throttled_signal_ex_set = signal_set_base<Key, throttled_signal_ex, Args...>;
template<typename Key, typename... Args> using queued_signal_set = signal_set_base<Key, queued_signal, Args...>;
template<typename Key, typename... Args> using queued_signal_ex_set = signal_set_base<Key, queued_signal_ex, Args...>;
template<typename Key, typename scope, typename... Args> using queued_signal_scoped_set = signal_set_base<Key, queued_signal_scoped, scope, Args...>;
template<typename Key, typename scope, typename... Args> using queued_signal_ex_scoped_set = signal_set_base<Key, queued_signal_ex_scoped, scope, Args...>;
template<typename Key, typename... Args> using bridged_signal_set = bridged_signal_set_base<Key, bridged_signal, Args...>;
template<typename Key, typename... Args> using bridged_signal_ex_set = bridged_signal_set_base<Key, bridged_signal_ex, Args...>;

struct connection_bag
{
    std::deque<connection> connections;

    connection_bag &operator= (connection &&c) { connections.push_front(std::forward<connection>(c)); return *this; }
};

}
