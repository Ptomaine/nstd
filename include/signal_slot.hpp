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
#include <chrono>
#include <functional>
#include <mutex>
#include <queue>
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

	template<typename, typename> friend struct paired_ptr;
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

template<typename... Args>
class slot
{
protected:
    std::function<void (Args...)> _functor {};
    paired_ptr<> _connection {};

    friend class connection;

public:
    slot() = default;
    slot(std::function<void(Args...)> &&f) : _functor{ std::forward<std::function<void(Args...)>>(f) }{}
    void operator()(const Args &... args) { _functor(args...); }
    bool is_disconnected() const { return !_connection; }
    void clear() { _connection.disconnect(); }
};

class signal_base
{
public:
    virtual ~signal_base() = default;
    virtual std::string_view name() const = 0;
    virtual bool enabled() const = 0;
    virtual void enabled(bool enabled) const = 0;
    virtual size_t size() const = 0;
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
                std::move(_pending_connections.begin(), _pending_connections.end(), std::back_inserter(_slots));

                _pending_connections.clear();
            }
        }

        auto end = _slots.end();

        if (!std::empty(_slots))
        {
            _slots.erase(std::remove_if(_slots.begin(), end, [&args...](auto &callable)
            {
                if (callable.is_disconnected()) return true;

                callable(args...);

                return callable.is_disconnected();
            }), end);
        }
    }

    virtual connection connect(std::function<void(Args...)> &&callable)
    {
        slot<Args...> new_slot(std::forward<std::function<void(Args...)>>(callable));
        connection to_return(this, new_slot);

        std::scoped_lock lock(_connect_lock);

        _pending_connections.erase(std::remove_if(_pending_connections.begin(), _pending_connections.end(), std::mem_fn(&slot<Args...>::is_disconnected)), _pending_connections.end());
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

    virtual bool enabled() const
    {
        return _enabled;
    }

    virtual void enabled(bool enabled) const
    {
        _enabled = enabled;
    }

protected:
    std::string _name {};
    std::vector<slot<Args...>> _slots {}, _pending_connections {};
    mutable std::mutex _connect_lock {}, _emit_lock {}, _name_lock {};
    mutable std::atomic_bool _enabled { true };
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

template<typename... Args>
class throttled_signal : public signal<Args...>
{
public:
    using base_class = signal<Args...>;

    throttled_signal() = default;
    template<typename Duration>
    throttled_signal(const std::string &name, const Duration &throttle_ms = throttled_signal::_default_throttle_ms) : base_class{ name }, _throttle_ms{ std::chrono::duration_cast<std::chrono::milliseconds>(throttle_ms) } {}
    throttled_signal(const std::string &name) : base_class{ name } {}
    throttled_signal(throttled_signal &&other) = default;
    throttled_signal &operator=(throttled_signal &&other) = default;

    virtual ~throttled_signal() override
    {
        _cancelled = true;

        if (_dispatcher_thread.joinable()) _dispatcher_thread.join();

        std::scoped_lock lock(_emit_lock);

        while (!std::empty(_signal_queue))
        {
            auto args = std::move(_signal_queue.front());
            _signal_queue.pop();

            std::apply([&](const Args&... a){ base_class::emit(a...); }, args);
        }
    }

    void emit(const Args &... args)
    {
        std::scoped_lock lock(_emit_lock);

        _signal_queue.push(std::make_tuple(args...));

        if (!_thread_running.exchange(true))
        {
            _dispatcher_thread = std::thread([this](){ queue_dispatcher(); });
        }
    }

    template<typename Duration>
    void throttle_ms(const Duration &duration)
    {
        _throttle_ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
    }

    std::chrono::microseconds throttle_ms() const
    {
        return _throttle_ms;
    }

protected:
    static inline std::chrono::milliseconds _default_throttle_ms { 10ms };
    std::queue<std::tuple<Args...>> _signal_queue {};
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

                _signal_queue.pop();

                std::apply([&](const Args&... a){ base_class::emit(a...); }, args);

                if (std::empty(_signal_queue)) break;
            }

            std::this_thread::sleep_for(_throttle_ms.load());
        }

        _thread_running = false;
    }
};

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

template<typename SignalType>
class signal_set
{
public:
    signal_set() = default;
    ~signal_set() = default;

    template <typename... Args>
    void emit(const Args &... args) const
    {
        for (auto &&s : _signals) s.second->emit(args...);
    }

    const std::unique_ptr<SignalType> &get_signal(const std::string &signal_name)
    {
        auto signal { _signals.find(signal_name) };

        if (signal != _signals.end()) return signal->second;

        return _signals.emplace(signal_name, std::make_unique<SignalType>(signal_name)).first->second;
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

    const std::unique_ptr<SignalType> &operator[](const std::string &signal_name)
    {
        return get_signal(signal_name);
    }

protected:
    std::unordered_map<std::string, std::unique_ptr<SignalType>> _signals {};
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
