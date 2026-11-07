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

#include <algorithm>
#include <any>
#include <atomic>
#include <chrono>
#include <concepts>
#include <condition_variable>
#include <deque>
#include <functional>
#include <iterator>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <shared_mutex>
#include <stdexcept>
#include <stop_token>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

/**
 * @namespace nstd::signal_slot
 * @brief Namespace containing a flexible signal-slot implementation
 * 
 * This namespace provides a type-safe signal-slot implementation with various
 * extensions like throttled signals, queued signals, and bridged signals.
 * Signal-slot is a design pattern that enables communication between objects
 * in a loosely coupled way.
 */
namespace nstd::signal_slot
{

using namespace std::chrono_literals;

/**
 * @brief Utility struct for types that have a value
 * @tparam T The type of the value
 */
template <typename T>
struct has_value_type
{
    T value = {};

    has_value_type() = default;
    has_value_type(has_value_type &&) noexcept = default;
    has_value_type &operator= (has_value_type &&) noexcept = default;
};

/**
 * @brief Utility struct for types that don't have a value
 */
struct no_value_type
{
    no_value_type() = default;
    no_value_type(no_value_type &&) noexcept = default;
    no_value_type &operator= (no_value_type &&) noexcept = default;
};

/**
 * @brief Meta-type that selects between value or no-value type based on T1
 * @tparam T1 Type to check
 */
template <typename T1>
using base_value_type = typename std::conditional_t<std::is_same_v<T1, std::void_t<>>, no_value_type, has_value_type<T1>>;

/**
 * @brief A smart pointer-like class that connects two objects
 * 
 * This class is used to establish a bi-directional connection between two objects.
 * When one object is disconnected or destroyed, the other is automatically notified.
 * 
 * @tparam T1 Type of value stored in this paired_ptr instance
 * @tparam T2 Type of value stored in the connected paired_ptr instance
 */
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

    /**
     * @brief Default constructor creates an unconnected paired_ptr
     */
    paired_ptr()
    {
    }

    /**
     * @brief Constructor that connects to another paired_ptr
     * @param ptr Pointer to the paired_ptr to connect with
     */
    paired_ptr(connected_paired_ptr_type *ptr) : _connected_paired_ptr{ ptr }
    {
        if (_connected_paired_ptr) _connected_paired_ptr->_connected_paired_ptr = this;
    }

    /**
     * @brief Move constructor
     * @param other The paired_ptr to move from
     */
    paired_ptr(paired_ptr &&other) noexcept : paired_ptr{ other._connected_paired_ptr }
    {
        base_type::operator=(static_cast<base_type&&>(std::forward<paired_ptr>(other)));
        other._connected_paired_ptr = nullptr;
    }

    paired_ptr(const paired_ptr &other) = delete;
    paired_ptr & operator=(const paired_ptr &other) = delete;

    /**
     * @brief Move assignment operator
     * @param other The paired_ptr to move from
     * @return Reference to this paired_ptr
     */
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

    /**
     * @brief Assignment operator from a connected_paired_ptr_type pointer
     * @param ptr Pointer to the paired_ptr to connect with
     * @return Reference to this paired_ptr
     */
    paired_ptr & operator=(connected_paired_ptr_type *ptr)
    {
        return *this = paired_ptr(ptr);
    }

    /**
     * @brief Destructor that ensures proper disconnection
     */
    ~paired_ptr()
    {
        disconnect();
    }

    /**
     * @brief Disconnects this paired_ptr from its connected pair
     */
    void disconnect() noexcept
    {
        if (_connected_paired_ptr)
        {
            _connected_paired_ptr->_connected_paired_ptr = nullptr;
            _connected_paired_ptr = nullptr;
        }
    }

    /**
     * @brief Gets the connected paired_ptr
     * @return Pointer to the connected paired_ptr
     */
    const connected_paired_ptr_type * connected_ptr() const noexcept
    {
        return _connected_paired_ptr;
    }


    /**
     * @brief Boolean conversion operator
     * @return true if connected to another paired_ptr, false otherwise
     */
    operator bool() const noexcept
    {
        return (_connected_paired_ptr);
    }

    /**
     * @brief Negation operator
     * @return true if not connected to another paired_ptr, false otherwise
     */
    bool operator!() const noexcept
    {
        return !(_connected_paired_ptr);
    }

protected:
    connected_paired_ptr_type *_connected_paired_ptr = nullptr;
};

/**
 * @brief Equality operator for paired_ptr
 * @param lhs Left-hand side paired_ptr
 * @param rhs Right-hand side paired_ptr
 * @return true if both paired_ptrs are connected to the same paired_ptr
 */
template<typename T1, typename T2>
bool operator==(const paired_ptr<T1, T2> & lhs, const paired_ptr<T1, T2> & rhs)
{
    return lhs._connected_paired_ptr == rhs._connected_paired_ptr;
}

/**
 * @brief Inequality operator for paired_ptr
 * @param lhs Left-hand side paired_ptr
 * @param rhs Right-hand side paired_ptr
 * @return true if the paired_ptrs are not connected to the same paired_ptr
 */
template<typename T1, typename T2>
bool operator!=(const paired_ptr<T1, T2> & lhs, const paired_ptr<T1, T2> & rhs)
{
    return !(lhs == rhs);
}

/**
 * @brief Equality operator for paired_ptr and nullptr
 * @param lhs Left-hand side paired_ptr
 * @param nullptr_t
 * @return true if the paired_ptr is not connected
 */
template<typename T1, typename T2>
bool operator==(const paired_ptr<T1, T2> & lhs, std::nullptr_t)
{
    return !lhs;
}

/**
 * @brief Inequality operator for paired_ptr and nullptr
 * @param lhs Left-hand side paired_ptr
 * @param nullptr_t
 * @return true if the paired_ptr is connected
 */
template<typename T1, typename T2>
bool operator!=(const paired_ptr<T1, T2> & lhs, std::nullptr_t)
{
    return !(lhs == nullptr);
}

/**
 * @brief Equality operator for nullptr and paired_ptr
 * @param nullptr_t
 * @param rhs Right-hand side paired_ptr
 * @return true if the paired_ptr is not connected
 */
template<typename T1, typename T2>
bool operator==(std::nullptr_t, const paired_ptr<T1, T2> & rhs)
{
    return rhs == nullptr;
}

/**
 * @brief Inequality operator for nullptr and paired_ptr
 * @param nullptr_t
 * @param rhs Right-hand side paired_ptr
 * @return true if the paired_ptr is connected
 */
template<typename T1, typename T2>
bool operator!=(std::nullptr_t, const paired_ptr<T1, T2> & rhs)
{
    return !(nullptr == rhs);
}

class signal_base;
class connection;

namespace detail
{

/**
 * @brief State a signal shares with its slots
 *
 * The signal owns it and slots keep only a weak reference to it, so a connection
 * can safely outlive its signal, and a signal can be destroyed from inside one of
 * its own slots.
 */
struct signal_core_base
{
    virtual ~signal_core_base() = default;

    /**
     * @brief Re-sorts the slots after a priority change; called with @ref lock held
     */
    virtual void resort_locked() = 0;

    std::mutex lock {};                ///< Guards everything in the core
    signal_base *owner { nullptr };    ///< The signal, or nullptr once it is gone
};

}

/**
 * @brief Base class for slots
 *
 * A slot is shared by its signal and its connection. Its state is atomic, so it
 * can be enabled, disabled or disconnected from any thread.
 */
class slot_base
{
public:
    virtual ~slot_base() = default;

    /**
     * @brief Sets whether the slot is enabled
     * @param is_enabled true to enable the slot, false to disable it
     */
    void set_enabled(bool is_enabled) noexcept { _enabled = is_enabled; }

    /**
     * @brief Checks if the slot is enabled
     * @return true if the slot is enabled, false otherwise
     */
    bool is_enabled() const noexcept { return _enabled; }

    /**
     * @brief Checks if the slot is disconnected
     * @return true if the slot is disconnected, false otherwise
     */
    bool is_disconnected() const noexcept { return !_connected; }

    /**
     * @brief Disconnects the slot from its signal
     *
     * The slot is never invoked by an emission that starts after this call. An
     * invocation already running on another thread is not waited for.
     */
    void disconnect() noexcept { _connected = false; }

    /**
     * @brief Gets the priority of the slot
     * @return The slot's priority (lower values are called first)
     */
    int64_t get_priority() const noexcept { return _priority; }

    /**
     * @brief Sets the priority of the slot and re-sorts its signal
     * @param priority New priority value
     * @return The previous priority value
     */
    int64_t set_priority(int64_t priority)
    {
        if (auto core { _core.lock() })
        {
            std::scoped_lock lock { core->lock };

            const auto previous { _priority.exchange(priority) };

            core->resort_locked();

            return previous;
        }

        return _priority.exchange(priority);
    }

protected:
    template<typename... Args> friend class signal;
    friend class connection;

    std::atomic_bool _connected { true };
    std::atomic_bool _enabled { true };
    std::atomic<int64_t> _priority { 0 };
    uint64_t _order { 0 };                              ///< Connection sequence number; keeps equal priorities in connection order
    std::weak_ptr<detail::signal_core_base> _core {};
};

/**
 * @brief A slot that can be connected to a signal
 *
 * A slot is essentially a wrapper around a function or method that can be
 * invoked when a signal is emitted. Arguments are passed by const reference, so
 * no copies are made per slot and move-only argument types are supported.
 *
 * @tparam Args Types of arguments the slot accepts
 */
template<typename... Args>
class slot : public slot_base
{
public:
    using functor_type = std::function<void(const Args &...)>;

    /**
     * @brief Default constructor
     */
    slot() = default;

    /**
     * @brief Constructor with a function and priority
     * @param f Function to be called when the slot is invoked
     * @param priority Priority of the slot (lower value means higher priority; slots with lower priority values are called first)
     */
    slot(functor_type &&f, int64_t priority = 0) : _functor { std::move(f) } { _priority = priority; }

    /**
     * @brief Invokes the slot with the given arguments
     * @param args Arguments to pass to the slot function
     */
    void invoke(const Args &... args) const { _functor(args...); }

    /**
     * @brief Function call operator to invoke the slot
     * @param args Arguments to pass to the slot function
     */
    void operator()(const Args &... args) const { invoke(args...); }

    /**
     * @brief Compares slot identity
     * @param s The slot to compare with
     * @return true if both are the same slot
     */
    bool operator == (const slot &s) const noexcept { return this == &s; }

    /**
     * @brief Less-than comparison based on priority
     * @param s The slot to compare with
     * @return true if this slot is called before the given slot
     */
    bool operator < (const slot &s) const noexcept { return get_priority() < s.get_priority(); }

    /**
     * @brief Greater-than comparison based on priority
     * @param s The slot to compare with
     * @return true if this slot is called after the given slot
     */
    bool operator > (const slot &s) const noexcept { return get_priority() > s.get_priority(); }

protected:
    functor_type _functor {};
};

/**
 * @brief Base interface class for all signals
 *
 * Provides a common interface for all signal types.
 */
class signal_base
{
public:
    /**
     * @brief Virtual destructor for proper inheritance
     */
    virtual ~signal_base() = default;

    /**
     * @brief Gets the name of the signal
     * @return A copy of the signal's name (a copy, so a concurrent rename cannot invalidate it)
     */
    [[nodiscard]] virtual std::u8string name() const = 0;

    /**
     * @brief Sets whether the signal is enabled
     * @param enabled true to enable the signal, false to disable it
     */
    virtual void set_enabled(bool enabled) = 0;

    /**
     * @brief Checks if the signal is enabled
     * @return true if the signal is enabled, false otherwise
     */
    virtual bool is_enabled() const = 0;

    /**
     * @brief Gets the number of slots connected to the signal
     * @return Number of connected slots
     */
    [[nodiscard]] virtual size_t size() const = 0;

    /**
     * @brief Gets the signal's payload (not synchronized)
     * @return Reference to the signal's payload
     */
    [[nodiscard]] virtual std::any &payload() = 0;

    /**
     * @brief Enables or disables a slot of this signal
     * @param slot The slot to enable/disable
     * @param enabled true to enable the slot, false to disable it
     */
    virtual void enable_slot(const slot_base &slot, bool enabled) = 0;

    /**
     * @brief Enables or disables a slot of this signal by its connection
     * @param con The connection to the slot
     * @param enabled true to enable the slot, false to disable it
     */
    virtual void enable_slot(const connection &con, bool enabled) = 0;

    /**
     * @brief Checks if a slot of this signal is enabled
     * @param slot The slot to check
     * @return true if the slot belongs to this signal and is enabled
     */
    virtual bool is_slot_enabled(const slot_base &slot) const = 0;

    /**
     * @brief Checks if a slot of this signal is enabled by its connection
     * @param con The connection to the slot
     * @return true if the slot belongs to this signal and is enabled
     */
    virtual bool is_slot_enabled(const connection &con) const = 0;

    /**
     * @brief Gets the priority of a connection
     * @param con The connection to check
     * @return The priority of the connection, or 0 if it does not belong to this signal
     */
    virtual int64_t get_connection_priority(const connection &con) const = 0;

    /**
     * @brief Sets the priority of a connection
     * @param con The connection to modify
     * @param priority The new priority
     * @return true if the priority was set successfully, false otherwise
     */
    virtual bool set_connection_priority(const connection &con, int64_t priority) = 0;
};

/**
 * @brief Class representing a connection between a signal and a slot
 *
 * This class manages the lifetime of a connection between a signal and a slot.
 * When a connection object is destroyed, the connection is automatically broken.
 * Either side may be destroyed first, on any thread.
 */
class connection
{
public:
    /**
     * @brief Default constructor creates an empty connection
     */
    connection() = default;

    /**
     * @brief Move constructor
     * @param other The connection to move from
     */
    connection(connection &&other) noexcept = default;

    connection(const connection &other) = delete;
    connection &operator=(const connection &other) = delete;

    /**
     * @brief Move assignment operator; disconnects the current connection first
     * @param other The connection to move from
     * @return Reference to this connection
     */
    connection &operator=(connection &&other) noexcept
    {
        if (this != &other)
        {
            disconnect();

            _slot = std::move(other._slot);
        }

        return *this;
    }

    /**
     * @brief Destructor that ensures the connection is properly disconnected
     */
    ~connection()
    {
        disconnect();
    }

    /**
     * @brief Disconnects the signal from the slot
     *
     * Emissions that start after this call never invoke the slot. An invocation
     * already running on another thread is not waited for.
     */
    void disconnect() noexcept
    {
        if (_slot)
        {
            _slot->disconnect();
            _slot.reset();
        }
    }

    /**
     * @brief Checks if the connection is disconnected
     * @return true if disconnected, cleared, or the signal is gone
     */
    [[nodiscard]] bool is_disconnected() const noexcept { return !_slot || _slot->is_disconnected(); }

    /**
     * @brief Gets the signal this connection was made to
     * @return Reference to the connected signal
     * @throws std::logic_error if the connection is empty or the signal no longer exists
     */
    [[nodiscard]] const signal_base &signal() const
    {
        if (_slot)
        {
            if (auto core { _slot->_core.lock() })
            {
                std::scoped_lock lock { core->lock };

                if (core->owner) return *core->owner;
            }
        }

        throw std::logic_error { "nstd::signal_slot::connection::signal(): the connection has no signal" };
    }

    /**
     * @brief Enables or disables this connection; works before the first emission too
     * @param enabled true to enable the connection, false to disable it
     */
    void set_enabled(bool enabled) noexcept
    {
        if (!is_disconnected()) _slot->set_enabled(enabled);
    }

    /**
     * @brief Checks if this connection is enabled
     * @return true if the connection is connected and enabled, false otherwise
     */
    [[nodiscard]] bool is_enabled() const noexcept
    {
        return !is_disconnected() && _slot->is_enabled();
    }

    /**
     * @brief Gets the priority of this connection
     * @return The connection's priority, or 0 if disconnected
     */
    [[nodiscard]] int64_t get_connection_priority() const noexcept
    {
        return is_disconnected() ? 0 : _slot->get_priority();
    }

    /**
     * @brief Sets the priority of this connection; the signal re-sorts its slots
     * @param priority The new priority
     * @return true if the priority was set successfully, false if disconnected
     */
    bool set_connection_priority(int64_t priority)
    {
        if (is_disconnected()) return false;

        _slot->set_priority(priority);

        return true;
    }

protected:
    template<typename... Args> friend class signal;

    explicit connection(std::shared_ptr<slot_base> s) noexcept : _slot { std::move(s) } {}

    std::shared_ptr<slot_base> _slot {};
};

/**
 * @brief The core signal class that can be connected to slots
 *
 * A signal can have multiple slots connected to it. When the signal is emitted,
 * all connected slots are invoked in order of their priority; slots with equal
 * priority are invoked in connection order.
 *
 * Thread safety: every member may be called from any thread. No internal lock is
 * held while slots run, so a slot may emit this signal again, connect, disconnect,
 * clear, query the signal or even destroy it. Each emission works on a snapshot of
 * the slots taken when it starts. If the signal is emitted from several threads at
 * once, its slots can run concurrently.
 *
 * @tparam Args Types of arguments the signal passes to slots when emitted
 */
template<typename... Args>
class signal : public signal_base
{
public:
    using slot_type = slot<Args...>;
    using functor_type = typename slot_type::functor_type;

protected:
    using slot_ptr = std::shared_ptr<slot_type>;
    using slot_list = std::vector<slot_ptr>;

    struct core_type final : detail::signal_core_base
    {
        std::shared_ptr<const slot_list> slots {};    ///< Current snapshot, sorted by priority, then connection order
        uint64_t next_order { 0 };

        void resort_locked() override { signal::rebuild_locked(*this, nullptr); }
    };

public:
    /**
     * @brief Default constructor
     */
    signal() = default;

    /**
     * @brief Constructor with a name
     * @param name The name of the signal
     */
    signal(const std::u8string &name) : _name{ name } {}

    /**
     * @brief Move constructor; connections follow the signal to its new place
     * @param other The signal to move from (left without slots)
     */
    signal(signal &&other) noexcept : _core { std::move(other._core) }
    {
        take_fields(other);
    }

    /**
     * @brief Move assignment operator; this signal's own connections are disconnected first
     * @param other The signal to move from (left without slots)
     * @return Reference to this signal
     */
    signal &operator=(signal &&other) noexcept
    {
        if (this != &other)
        {
            detach_core();

            _core = std::move(other._core);

            take_fields(other);
        }

        return *this;
    }

    signal(const signal &) = delete;
    signal &operator=(const signal &) = delete;

    /**
     * @brief Virtual destructor; disconnects all slots
     */
    virtual ~signal() override
    {
        detach_core();
    }

    /**
     * @brief Emits the signal with the given arguments
     *
     * When a signal is emitted, all connected slots are invoked with the
     * provided arguments. Slots are invoked in order of their priority.
     *
     * @param args Arguments to pass to the slots
     */
    void emit(const Args &... args)
    {
        if (!_enabled) return;

        const auto core { _core };    // keeps the slots alive even if a slot destroys this signal

        if (!core) return;

        std::shared_ptr<const slot_list> snapshot;

        {
            std::scoped_lock lock { core->lock };

            snapshot = core->slots;
        }

        if (!snapshot) return;

        bool purge { false };

        for (const auto &s : *snapshot)
        {
            if (s->is_disconnected()) { purge = true; continue; }

            if (s->is_enabled()) s->invoke(args...);
        }

        if (purge)
        {
            std::scoped_lock lock { core->lock };

            if (core->slots) rebuild_locked(*core, nullptr);
        }
    }

    /**
     * @brief Function call operator to emit the signal
     * @param args Arguments to pass to the slots
     */
    void operator() (const Args &... args)
    {
        emit(args...);
    }

    /**
     * @brief Connects a function to this signal
     *
     * A slot connected while the signal is being emitted is first called by the next emission.
     *
     * @param callable Function to connect
     * @param priority Priority of the connection
     * @return A connection object that manages the connection's lifetime
     */
    [[nodiscard]] virtual connection connect(functor_type &&callable, int64_t priority = 0)
    {
        auto s { std::make_shared<slot_type>(std::move(callable), priority) };

        if (!_core) _core = make_core(this);    // only after this signal was moved from

        std::scoped_lock lock { _core->lock };

        s->_core = _core;
        s->_order = _core->next_order++;

        rebuild_locked(*_core, s);

        return connection { std::move(s) };
    }

    /**
     * @brief Operator += to connect a function to this signal
     * @param callable Function to connect
     * @return A connection object that manages the connection's lifetime
     */
    [[nodiscard]] virtual connection operator += (functor_type &&callable)
    {
        return connect(std::move(callable));
    }

    /**
     * @brief Connects a member function to this signal
     *
     * @param instance The object instance
     * @param member_function Member function to connect
     * @param priority Priority of the connection (lower value means higher priority)
     * @return A connection object that manages the connection's lifetime
     */
    template<typename T>
    [[nodiscard]] connection connect(T *instance, void (T::*member_function)(Args...), int64_t priority = 0)
    {
        return connect([instance, member_function](const Args &... args) { (instance->*member_function)(args...); }, priority);
    }

    /**
     * @brief Disconnects all slots from this signal
     */
    virtual void clear()
    {
        if (!_core) return;

        std::scoped_lock lock { _core->lock };

        if (_core->slots) for (const auto &s : *_core->slots) s->disconnect();

        _core->slots.reset();
    }

    /**
     * @brief Gets the number of slots connected to this signal
     * @return Number of connected slots, including ones connected since the last emission
     */
    virtual size_t size() const override
    {
        if (!_core) return 0;

        std::scoped_lock lock { _core->lock };

        if (!_core->slots) return 0;

        return static_cast<size_t>(std::count_if(std::begin(*_core->slots), std::end(*_core->slots), [](const auto &s) { return !s->is_disconnected(); }));
    }

    /**
     * @brief Sets the name of this signal
     * @param name The new name
     */
    void name(const std::u8string &name)
    {
        std::unique_lock<std::shared_mutex> lock { _name_lock };

        _name = name;
    }

    /**
     * @brief Gets the name of this signal
     * @return A copy of the signal's name
     */
    virtual std::u8string name() const override
    {
        std::shared_lock<std::shared_mutex> lock { _name_lock };

        return _name;
    }

    /**
     * @brief Sets whether this signal is enabled
     * @param enabled true to enable the signal, false to disable it
     */
    virtual void set_enabled(bool enabled) override
    {
        _enabled = enabled;
    }

    /**
     * @brief Checks if this signal is enabled
     * @return true if the signal is enabled, false otherwise
     */
    virtual bool is_enabled() const override
    {
        return _enabled;
    }

    /**
     * @brief Gets this signal's payload (not synchronized)
     * @return Reference to the signal's payload
     */
    virtual std::any &payload() override
    {
        return _payload;
    }

    /**
     * @brief Enables or disables a slot of this signal
     * @param s The slot to enable/disable
     * @param enabled true to enable the slot, false to disable it
     */
    virtual void enable_slot(const slot_base &s, bool enabled) override
    {
        if (owns(s)) const_cast<slot_base &>(s).set_enabled(enabled);
    }

    /**
     * @brief Enables or disables a slot of this signal by its connection
     * @param con The connection to the slot
     * @param enabled true to enable the slot, false to disable it
     */
    virtual void enable_slot(const connection &con, bool enabled) override
    {
        if (con._slot && owns(*con._slot)) con._slot->set_enabled(enabled);
    }

    /**
     * @brief Checks if a slot of this signal is enabled
     * @param s The slot to check
     * @return true if the slot belongs to this signal and is enabled
     */
    virtual bool is_slot_enabled(const slot_base &s) const override
    {
        return owns(s) && s.is_enabled();
    }

    /**
     * @brief Checks if a slot of this signal is enabled by its connection
     * @param con The connection to the slot
     * @return true if the slot belongs to this signal and is enabled
     */
    virtual bool is_slot_enabled(const connection &con) const override
    {
        return con._slot && is_slot_enabled(*con._slot);
    }

    /**
     * @brief Gets the priority of a connection
     * @param con The connection to check
     * @return The priority of the connection, or 0 if it does not belong to this signal
     */
    virtual int64_t get_connection_priority(const connection &con) const override
    {
        return con._slot && owns(*con._slot) ? con._slot->get_priority() : 0;
    }

    /**
     * @brief Sets the priority of a connection; the slots are re-sorted
     * @param con The connection to modify
     * @param priority The new priority
     * @return true if the priority was set successfully, false otherwise
     */
    virtual bool set_connection_priority(const connection &con, int64_t priority) override
    {
        if (!con._slot || !owns(*con._slot)) return false;

        con._slot->set_priority(priority);

        return true;
    }

protected:
    std::u8string _name {};
    mutable std::shared_mutex _name_lock {};
    std::atomic_bool _enabled { true };
    std::any _payload {};
    std::shared_ptr<core_type> _core { make_core(this) };

    static std::shared_ptr<core_type> make_core(signal *owner)
    {
        auto core { std::make_shared<core_type>() };

        core->owner = owner;

        return core;
    }

    /**
     * @brief Publishes a new snapshot without disconnected slots, plus an optional new one, sorted; core lock held
     */
    static void rebuild_locked(core_type &core, slot_ptr added)
    {
        auto list { std::make_shared<slot_list>() };

        if (core.slots)
        {
            list->reserve(std::size(*core.slots) + (added ? 1 : 0));

            for (const auto &s : *core.slots) if (!s->is_disconnected()) list->push_back(s);
        }

        if (added) list->push_back(std::move(added));

        std::sort(std::begin(*list), std::end(*list), [](const slot_ptr &a, const slot_ptr &b)
        {
            const auto pa { a->_priority.load() }, pb { b->_priority.load() };

            return pa != pb ? pa < pb : a->_order < b->_order;
        });

        core.slots = std::move(list);
    }

    /**
     * @brief Checks whether a slot is connected to this signal
     */
    bool owns(const slot_base &s) const noexcept
    {
        return _core && !s.is_disconnected() && !s._core.owner_before(_core) && !_core.owner_before(s._core);
    }

    /**
     * @brief Disconnects all slots and detaches the shared state from this signal
     */
    void detach_core() noexcept
    {
        if (!_core) return;

        std::scoped_lock lock { _core->lock };

        _core->owner = nullptr;

        if (_core->slots) for (const auto &s : *_core->slots) s->disconnect();

        _core->slots.reset();
    }

    /**
     * @brief Moves everything but the core from another signal and re-points the core at this one
     */
    void take_fields(signal &other) noexcept
    {
        {
            std::scoped_lock lock { _name_lock, other._name_lock };

            _name = std::move(other._name);
        }

        _payload = std::move(other._payload);
        _enabled = other._enabled.load();

        if (_core)
        {
            std::scoped_lock lock { _core->lock };

            _core->owner = this;
        }
    }
};

/**
 * @brief Extended signal class that passes itself as the first argument to slots
 *
 * This is useful when slots need to know which signal triggered them.
 *
 * @tparam Args Types of additional arguments the signal passes to slots
 */
template<typename... Args>
class signal_ex : public signal<signal_base*, Args...>
{
public:
    using base_class = signal<signal_base*, Args...>;

    /**
     * @brief Default constructor
     */
    signal_ex() = default;

    /**
     * @brief Constructor with a name
     * @param name The name of the signal
     */
    signal_ex(const std::u8string &name) : base_class(name) {}

    /**
     * @brief Move constructor
     * @param other The signal to move from
     */
    signal_ex(signal_ex &&other) noexcept = default;

    /**
     * @brief Move assignment operator
     * @param other The signal to move from
     * @return Reference to this signal
     */
    signal_ex &operator=(signal_ex &&other) noexcept = default;

    /**
     * @brief Virtual destructor
     */
    virtual ~signal_ex() override = default;

    /**
     * @brief Emits the signal with the given arguments
     *
     * Unlike the base signal class, this automatically passes itself as the first argument.
     *
     * @param args Arguments to pass to the slots
     */
    void emit(const Args&... args)
    {
        base_class::emit(this, args...);
    }

    /**
     * @brief Function call operator to emit the signal
     * @param args Arguments to pass to the slots
     */
    void operator() (const Args&... args)
    {
        emit(args...);
    }
};

/**
 * @brief Base class for bridged signals
 *
 * A bridged signal queues emitted signals and allows controlling when they are
 * actually delivered to the slots. Slots run without the queue lock held, so the
 * emitting thread never waits for the consumer's slots, and slots may emit the
 * signal again or query the queue. Call the invoke_* functions from one consumer
 * thread to keep deliveries in order. Not movable: the emit functor receives `this`.
 *
 * @tparam signal_type The signal type to bridge
 * @tparam Args Types of arguments the signal passes to slots
 */
template<template <typename...> typename signal_type, std::copyable... Args>
requires std::derived_from<signal_type<Args...>, signal_base>
class bridged_signal_base : public signal_type<Args...>
{
public:
    using base_class = signal_type<Args...>;
    using bridged_signal_type = bridged_signal_base;
    using emit_functor_type = std::function<bool(bridged_signal_base*)>;

    /**
     * @brief Default constructor
     */
    bridged_signal_base() = default;

    /**
     * @brief Constructor with a name
     * @param name The name of the signal
     */
    bridged_signal_base(const std::u8string &name) : base_class { name } {}

    /**
     * @brief Constructor with a name and emit functor
     * @param name The name of the signal
     * @param emit_functor Called after each queued emission; return true if it arranges delivery itself
     */
    bridged_signal_base(const std::u8string &name, const emit_functor_type &emit_functor) : base_class { name }, _emit_functor { wrap(emit_functor) } {}

    /**
     * @brief Constructor with emit functor
     * @param emit_functor Called after each queued emission; return true if it arranges delivery itself
     */
    bridged_signal_base(const emit_functor_type &emit_functor) : bridged_signal_base { std::u8string {}, emit_functor } {}

    /**
     * @brief Constructor with a name and bridge enabled flag
     * @param name The name of the signal
     * @param bridge_enabled Whether the bridge is enabled
     */
    bridged_signal_base(const std::u8string &name, bool bridge_enabled) : base_class { name }, _bridge_enabled { bridge_enabled } {}

    /**
     * @brief Constructor with bridge enabled flag
     * @param bridge_enabled Whether the bridge is enabled
     */
    bridged_signal_base(bool bridge_enabled) : bridged_signal_base { std::u8string {}, bridge_enabled } {}

    bridged_signal_base(bridged_signal_base &&other) = delete;
    bridged_signal_base &operator=(bridged_signal_base &&other) = delete;

    /**
     * @brief Virtual destructor
     */
    virtual ~bridged_signal_base() override = default;

    /**
     * @brief Emits the signal with the given arguments
     *
     * If bridging is enabled, the signal is queued instead of immediately delivered.
     *
     * @param args Arguments to pass to the slots
     */
    void emit(const Args&... args)
    {
        if (!base_class::is_enabled()) return;

        if (!_bridge_enabled)
        {
            base_class::emit(args...);

            return;
        }

        std::shared_ptr<const emit_functor_type> functor;

        {
            std::scoped_lock lock { _queue_lock };

            _signal_queue.emplace_back(args...);
            functor = _emit_functor;
        }

        if (!functor || !(*functor)(this)) invoke_next();
    }

    /**
     * @brief Function call operator to emit the signal
     * @param args Arguments to pass to the slots
     */
    void operator() (const Args&... args)
    {
        emit(args...);
    }

    /**
     * @brief Emits the signal synchronously, bypassing bridging
     * @param args Arguments to pass to the slots
     */
    void emit_sync(const Args&... args)
    {
        base_class::emit(args...);
    }

    /**
     * @brief Invokes the next queued signal emission
     * @return true if there are more signals in the queue, false otherwise
     */
    virtual bool invoke_next()
    {
        std::optional<std::tuple<Args...>> next;

        {
            std::scoped_lock lock { _queue_lock };

            if (std::empty(_signal_queue)) return false;

            next.emplace(std::move(_signal_queue.front()));
            _signal_queue.pop_front();
        }

        emit_tuple(*next);

        std::scoped_lock lock { _queue_lock };

        return !std::empty(_signal_queue);
    }

    /**
     * @brief Invokes all emissions queued at the time of the call
     */
    virtual void invoke_all()
    {
        std::deque<std::tuple<Args...>> batch;

        {
            std::scoped_lock lock { _queue_lock };

            batch.swap(_signal_queue);
        }

        for (const auto &values : batch) emit_tuple(values);
    }

    /**
     * @brief Invokes only the last queued signal emission and clears the queue
     */
    virtual void invoke_last_and_clear()
    {
        std::optional<std::tuple<Args...>> last;

        {
            std::scoped_lock lock { _queue_lock };

            if (std::empty(_signal_queue)) return;

            last.emplace(std::move(_signal_queue.back()));
            _signal_queue.clear();
        }

        emit_tuple(*last);
    }

    /**
     * @brief Sets the emit functor
     * @param emit_functor Called after each queued emission; return true if it arranges delivery itself
     */
    void set_emit_functor(const emit_functor_type &emit_functor)
    {
        auto functor { wrap(emit_functor) };

        std::scoped_lock lock { _queue_lock };

        _emit_functor = std::move(functor);
    }

    /**
     * @brief Gets the emit functor
     * @return A copy of the emit functor (empty if none is set)
     */
    emit_functor_type get_emit_functor() const
    {
        std::scoped_lock lock { _queue_lock };

        return _emit_functor ? *_emit_functor : emit_functor_type {};
    }

    /**
     * @brief Gets the size of the signal queue
     * @return Number of queued signal emissions
     */
    uint64_t get_queue_size() const
    {
        std::scoped_lock lock { _queue_lock };

        return std::size(_signal_queue);
    }

    /**
     * @brief Sets whether bridging is enabled
     * @param enabled true to enable bridging, false to disable it
     */
    void set_bridge_enabled(bool enabled) noexcept
    {
        _bridge_enabled.store(enabled);
    }

    /**
     * @brief Checks if bridging is enabled
     * @return true if bridging is enabled, false otherwise
     */
    bool get_bridge_enabled() const noexcept
    {
        return _bridge_enabled;
    }

    /**
     * @brief Clears the signal queue
     */
    void clear_queue()
    {
        std::scoped_lock lock { _queue_lock };

        _signal_queue.clear();
    }

protected:
    std::atomic_bool _bridge_enabled { true };
    mutable std::mutex _queue_lock {};
    std::deque<std::tuple<Args...>> _signal_queue {};
    std::shared_ptr<const emit_functor_type> _emit_functor {};

    static std::shared_ptr<const emit_functor_type> wrap(const emit_functor_type &f)
    {
        return f ? std::make_shared<const emit_functor_type>(f) : nullptr;
    }

    void emit_tuple(const std::tuple<Args...> &values)
    {
        std::apply([this](const Args&... a){ base_class::emit(a...); }, values);
    }
};

/**
 * @brief Type alias for a bridged signal
 * @tparam Args Types of arguments the signal passes to slots
 */
template<typename... Args> using bridged_signal = bridged_signal_base<signal, Args...>;

/**
 * @brief Type alias for a bridged extended signal
 * @tparam Args Types of arguments the signal passes to slots
 */
template<typename... Args> using bridged_signal_ex = bridged_signal_base<signal_ex, Args...>;

/**
 * @brief Base class for throttled signals
 *
 * A throttled signal delivers emissions on a background thread, at most one per
 * throttle interval. By default every emission is queued; with coalescing on,
 * only the latest pending emission is kept. Slots run without internal locks held,
 * so they may emit the signal again. Do not destroy the signal from its own slot.
 *
 * @tparam signal_type The signal type to throttle
 * @tparam Args Types of arguments the signal passes to slots
 */
template<template <typename...> typename signal_type, std::copyable... Args>
requires std::derived_from<signal_type<Args...>, signal_base>
class throttled_signal_base : public signal_type<Args...>
{
protected:
    static inline std::chrono::milliseconds _default_throttle_ms { 10ms };

public:
    using base_class = signal_type<Args...>;

    /**
     * @brief Default constructor
     */
    throttled_signal_base() = default;

    /**
     * @brief Constructor with a name
     * @param name The name of the signal
     */
    throttled_signal_base(const std::u8string &name) : base_class{ name } {}

    /**
     * @brief Constructor with a name and throttle duration
     * @param name The name of the signal
     * @param throttle_ms The minimum interval between deliveries
     */
    template<typename Rep, typename Period>
    throttled_signal_base(const std::u8string &name, const std::chrono::duration<Rep, Period> &throttle_ms) : base_class{ name }, _throttle_ms{ std::chrono::duration_cast<std::chrono::milliseconds>(throttle_ms) } {}

    throttled_signal_base(throttled_signal_base &&other) = delete;
    throttled_signal_base &operator=(throttled_signal_base &&other) = delete;

    /**
     * @brief Destructor that stops the dispatcher and optionally delivers pending emissions
     */
    virtual ~throttled_signal_base() override
    {
        if (_dispatcher_thread.joinable())
        {
            _dispatcher_thread.request_stop();
            _dispatcher_thread.join();
        }

        if (_dispatch_all_on_destroy)
        {
            for (;;)
            {
                std::optional<std::tuple<Args...>> next;

                {
                    std::scoped_lock lock { _emit_lock };

                    if (std::empty(_signal_queue)) break;

                    next.emplace(std::move(_signal_queue.front()));
                    _signal_queue.pop_front();
                }

                emit_tuple(*next);
            }
        }
    }

    /**
     * @brief Emits the signal with the given arguments
     *
     * Signal emissions are queued and delivered at a controlled rate.
     *
     * @param args Arguments to pass to the slots
     */
    void emit(const Args &... args)
    {
        std::scoped_lock lock { _emit_lock };

        if (_coalesce && !std::empty(_signal_queue)) _signal_queue.back() = std::tuple<Args...> { args... };
        else _signal_queue.emplace_back(args...);

        if (!_thread_running)
        {
            _thread_running = true;

            if (_dispatcher_thread.joinable()) _dispatcher_thread.join();    // it has already released the lock and is exiting

            _dispatcher_thread = std::jthread([this](std::stop_token stop){ queue_dispatcher(std::move(stop)); });
        }
    }

    /**
     * @brief Function call operator to emit the signal
     * @param args Arguments to pass to the slots
     */
    void operator() (const Args &... args)
    {
        emit(args...);
    }

    /**
     * @brief Sets the throttling duration
     * @param duration The new throttling duration
     */
    template<typename Duration>
    void throttle_ms(const Duration &duration) noexcept
    {
        _throttle_ms.store(std::chrono::duration_cast<std::chrono::milliseconds>(duration));
    }

    /**
     * @brief Gets the throttling duration
     * @return The throttling duration in milliseconds
     */
    std::chrono::milliseconds throttle_ms() const noexcept
    {
        return _throttle_ms.load();
    }

    /**
     * @brief Sets whether only the latest pending emission is kept
     * @param coalesce true to replace a pending emission instead of queuing another one
     */
    void set_coalescing(bool coalesce) noexcept
    {
        _coalesce = coalesce;
    }

    /**
     * @brief Checks whether only the latest pending emission is kept
     * @return true if coalescing is on
     */
    bool get_coalescing() const noexcept
    {
        return _coalesce;
    }

    /**
     * @brief Sets whether all pending signals should be dispatched on destroy
     * @param do_dispatch true to dispatch all pending signals, false otherwise
     */
    void set_dispatch_all_on_destroy(bool do_dispatch) noexcept
    {
        _dispatch_all_on_destroy = do_dispatch;
    }

    /**
     * @brief Checks if all pending signals will be dispatched on destroy
     * @return true if all pending signals will be dispatched, false otherwise
     */
    bool get_dispatch_all_on_destroy() const noexcept
    {
        return _dispatch_all_on_destroy;
    }

protected:
    mutable std::mutex _emit_lock {};
    std::condition_variable_any _wake {};
    std::deque<std::tuple<Args...>> _signal_queue {};
    std::atomic<std::chrono::milliseconds> _throttle_ms { _default_throttle_ms };
    std::atomic_bool _thread_running { false }, _dispatch_all_on_destroy { true }, _coalesce { false };
    std::jthread _dispatcher_thread {};    // declared last, so it is joined before the members it uses are destroyed

    void emit_tuple(const std::tuple<Args...> &values)
    {
        std::apply([this](const Args&... a){ base_class::emit(a...); }, values);
    }

    /**
     * @brief Thread function that dispatches queued signals
     *
     * The running flag is only changed under the lock, together with the decision
     * to exit, so an emission can never land between the two.
     *
     * @param stop Stop token for thread cancellation
     */
    void queue_dispatcher(std::stop_token stop)
    {
        std::unique_lock lock { _emit_lock };

        for (;;)
        {
            if (stop.stop_requested() || std::empty(_signal_queue))
            {
                _thread_running = false;

                return;
            }

            auto next { std::move(_signal_queue.front()) };

            _signal_queue.pop_front();

            lock.unlock();
            emit_tuple(next);
            lock.lock();

            _wake.wait_for(lock, stop, _throttle_ms.load(), []{ return false; });    // interval between deliveries, also across bursts; wakes early on stop
        }
    }
};

/**
 * @brief Type alias for a throttled signal
 * @tparam Args Types of arguments the signal passes to slots
 */
template<typename... Args> using throttled_signal = throttled_signal_base<signal, Args...>;

/**
 * @brief Type alias for a throttled extended signal
 * @tparam Args Types of arguments the signal passes to slots
 */
template<typename... Args> using throttled_signal_ex = throttled_signal_base<signal_ex, Args...>;

/**
 * @brief Default scope for queued signals
 */
struct queued_signal_default_scope {};

namespace detail
{

/**
 * @brief The queue and dispatcher thread shared by all queued signals of one scope
 *
 * Emissions of every queued signal in the scope, whatever its argument types, are
 * delivered one at a time, in emission order, on one background thread.
 *
 * @tparam scope The scope tag
 */
template<typename scope>
class queued_dispatcher
{
public:
    using task_type = std::function<void()>;

    /**
     * @brief Makes sure the shared state exists (so it outlives every queued signal)
     */
    static void touch()
    {
        state();
    }

    /**
     * @brief Queues a task and starts the dispatcher thread if needed
     * @param owner The signal the task belongs to
     * @param task The delivery to run
     */
    static void post(const void *owner, task_type task)
    {
        auto &s { state() };

        std::scoped_lock lock { s.lock };

        s.queue.emplace_back(owner, std::move(task));

        if (!s.running)
        {
            s.running = true;

            if (s.thread.joinable()) s.thread.join();    // it has already released the lock and is exiting

            s.thread = std::jthread(&queued_dispatcher::run);
        }
    }

    /**
     * @brief Removes an owner's pending tasks and waits until none of its tasks is running
     * @param owner The signal being destroyed
     * @return The removed tasks, in queue order
     */
    static std::vector<task_type> withdraw(const void *owner)
    {
        auto &s { state() };
        std::vector<task_type> tasks;
        std::unique_lock lock { s.lock };

        for (auto it { std::begin(s.queue) }; it != std::end(s.queue);)
        {
            if (it->first == owner)
            {
                tasks.push_back(std::move(it->second));
                it = s.queue.erase(it);
            }
            else ++it;
        }

        if (s.thread.get_id() != std::this_thread::get_id()) s.idle.wait(lock, [&s, owner]{ return s.in_flight != owner; });

        return tasks;
    }

    static void delay(std::chrono::milliseconds duration) noexcept { state().delay = duration; }
    static std::chrono::milliseconds delay() noexcept { return state().delay; }
    static void use_delay(bool use) noexcept { state().use_delay = use; }
    static bool use_delay() noexcept { return state().use_delay; }

private:
    struct state_type
    {
        std::mutex lock {};
        std::condition_variable idle {};
        std::condition_variable_any wake {};
        std::deque<std::pair<const void*, task_type>> queue {};
        const void *in_flight { nullptr };
        bool running { false };
        std::atomic<std::chrono::milliseconds> delay { 0ms };
        std::atomic_bool use_delay { false };
        std::jthread thread {};    // declared last: stopped and joined before the rest is destroyed
    };

    static state_type &state()
    {
        static state_type s;

        return s;
    }

    static void run(std::stop_token stop)
    {
        auto &s { state() };
        std::unique_lock lock { s.lock };

        for (;;)
        {
            if (stop.stop_requested() || std::empty(s.queue))
            {
                s.running = false;

                return;
            }

            auto [owner, task] { std::move(s.queue.front()) };

            s.queue.pop_front();
            s.in_flight = owner;

            lock.unlock();
            task();
            lock.lock();

            s.in_flight = nullptr;
            s.idle.notify_all();

            if (s.use_delay && !std::empty(s.queue)) s.wake.wait_for(lock, stop, s.delay.load(), []{ return false; });
        }
    }
};

}

/**
 * @brief Base class for queued signals
 *
 * A queued signal delivers its emissions on a background thread through a queue
 * shared by all queued signals of the same scope, whatever their argument types.
 * Slots run without internal locks held, so they may emit queued signals again.
 * A destroyed signal's pending emissions are delivered or dropped, never left behind.
 *
 * @tparam scope The scope for grouping queued signals
 * @tparam signal_type The signal type to queue
 * @tparam Args Types of arguments the signal passes to slots
 */
template<typename scope, template <typename...> typename signal_type, std::copyable... Args>
requires std::derived_from<signal_type<Args...>, signal_base>
class queued_signal_base : public signal_type<Args...>
{
public:
    using base_class = signal_type<Args...>;
    using dispatcher_type = detail::queued_dispatcher<scope>;

    /**
     * @brief Default constructor
     */
    queued_signal_base() { dispatcher_type::touch(); }

    /**
     * @brief Constructor with a name
     * @param name The name of the signal
     */
    queued_signal_base(const std::u8string &name) : base_class{ name } { dispatcher_type::touch(); }

    /**
     * @brief Constructor with a name and delay
     *
     * The delay belongs to the scope's shared queue: it applies to every queued signal of the scope.
     *
     * @param name The name of the signal
     * @param delay_ms The delay between deliveries; a positive value also turns the delay on
     */
    template<typename Rep, typename Period>
    queued_signal_base(const std::u8string& name, const std::chrono::duration<Rep, Period>& delay_ms) : base_class{ name }
    {
        queued_signal_base::delay_ms(delay_ms);
        use_delay(delay_ms > std::chrono::duration<Rep, Period>::zero());
    }

    queued_signal_base(queued_signal_base &&other) = delete;
    queued_signal_base &operator=(queued_signal_base &&other) = delete;
    queued_signal_base(const queued_signal_base &other) = delete;
    queued_signal_base &operator=(const queued_signal_base &other) = delete;

    /**
     * @brief Destructor that removes this signal's pending emissions and optionally delivers them
     *
     * Waits for a delivery of this signal that is running on the dispatcher thread.
     */
    virtual ~queued_signal_base() override
    {
        auto pending { dispatcher_type::withdraw(this) };

        if (_dispatch_all_on_destroy) for (auto &task : pending) task();
    }

    /**
     * @brief Emits the signal with the given arguments
     *
     * Signal emissions are queued in a shared queue.
     * Parameters are copied and passed as a tuple to the queue.
     * Since parameters are copied, they should be copiable and should not be const, reference or rvalue.
     *
     * @param args Arguments to pass to the slots
     */
    void emit(const Args &... args)
    {
        dispatcher_type::post(this, [this, values = std::tuple<Args...> { args... }]
        {
            std::apply([this](const Args&... a){ base_class::emit(a...); }, values);
        });
    }

    /**
     * @brief Function call operator to emit the signal
     * @param args Arguments to pass to the slots
     */
    void operator() (const Args &... args)
    {
        emit(args...);
    }

    /**
     * @brief Sets the delay between deliveries of the scope's shared queue
     * @param duration The new delay
     */
    template<typename Duration>
    static void delay_ms(const Duration &duration) noexcept
    {
        dispatcher_type::delay(std::chrono::duration_cast<std::chrono::milliseconds>(duration));
    }

    /**
     * @brief Gets the delay between deliveries of the scope's shared queue
     * @return The delay in milliseconds
     */
    static std::chrono::milliseconds delay_ms() noexcept
    {
        return dispatcher_type::delay();
    }

    /**
     * @brief Sets whether the scope's shared queue waits between deliveries
     * @param use true to use delay, false otherwise
     */
    static void use_delay(bool use) noexcept
    {
        dispatcher_type::use_delay(use);
    }

    /**
     * @brief Checks if the scope's shared queue waits between deliveries
     * @return true if delay is used, false otherwise
     */
    static bool use_delay() noexcept
    {
        return dispatcher_type::use_delay();
    }

    /**
     * @brief Sets whether this signal's pending emissions are delivered when it is destroyed
     * @param do_dispatch true to dispatch them, false to drop them
     */
    void set_dispatch_all_on_destroy(bool do_dispatch) noexcept
    {
        _dispatch_all_on_destroy = do_dispatch;
    }

    /**
     * @brief Checks if this signal's pending emissions are delivered when it is destroyed
     * @return true if they will be dispatched, false otherwise
     */
    bool get_dispatch_all_on_destroy() const noexcept
    {
        return _dispatch_all_on_destroy;
    }

protected:
    std::atomic_bool _dispatch_all_on_destroy { true };
};

/**
 * @brief Type alias for a queued signal
 * @tparam Args Types of arguments the signal passes to slots
 */
template<typename... Args> using queued_signal = queued_signal_base<queued_signal_default_scope, signal, Args...>;

/**
 * @brief Type alias for a queued extended signal
 * @tparam Args Types of arguments the signal passes to slots
 */
template<typename... Args> using queued_signal_ex = queued_signal_base<queued_signal_default_scope, signal_ex, Args...>;

/**
 * @brief Type alias for a scoped queued signal
 * @tparam scope The scope for grouping queued signals
 * @tparam Args Types of arguments the signal passes to slots
 */
template<typename scope, typename... Args> using queued_signal_scoped = queued_signal_base<scope, signal, Args...>;

/**
 * @brief Type alias for a scoped queued extended signal
 * @tparam scope The scope for grouping queued signals
 * @tparam Args Types of arguments the signal passes to slots
 */
template<typename scope, typename... Args> using queued_signal_ex_scoped = queued_signal_base<scope, signal_ex, Args...>;

/**
 * @brief A signal that emits at regular intervals on a background thread
 *
 * Do not destroy the signal from its own slot; stop_timer() is safe there.
 *
 * @tparam Args Types of arguments the signal passes to slots
 */
template<std::copyable... Args>
class timer_signal : public signal<timer_signal<Args...>*, Args...>
{
public:
    using base_class = signal<timer_signal*, Args...>;

    /**
     * @brief Default constructor
     */
    timer_signal() = default;

    /**
     * @brief Constructor with a name; the interval defaults to one second
     * @param name The name of the signal
     */
    timer_signal(const std::u8string &name) : base_class{ name } {}

    /**
     * @brief Constructor with a name and timer duration
     * @param name The name of the signal
     * @param timer_ms The timer interval
     */
    template<typename Rep, typename Period>
    timer_signal(const std::u8string &name, const std::chrono::duration<Rep, Period> &timer_ms) : base_class{ name }, _timer_ms{ std::chrono::duration_cast<std::chrono::milliseconds>(timer_ms) } {}

    timer_signal(timer_signal &&other) = delete;
    timer_signal &operator=(timer_signal &&other) = delete;

    /**
     * @brief Destructor that stops the timer
     */
    virtual ~timer_signal() override
    {
        stop_timer();
    }

    /**
     * @brief Starts the timer with the given arguments
     * @param args Arguments to pass to the slots
     */
    void start_timer(const Args&... args)
    {
        if (!_timer_enabled.exchange(true))
        {
            if (_timer_thread.joinable()) _timer_thread.join();

            _args = std::make_tuple(this, args...);

            _sleep.reset();

            _timer_thread = std::jthread([this](){ timer_procedure(); });
        }
    }

    /**
     * @brief Stops the timer; from inside a slot it only stops further ticks
     */
    void stop_timer()
    {
        _sleep.cancel_wait();

        if (_timer_thread.joinable() && _timer_thread.get_id() != std::this_thread::get_id()) _timer_thread.join();
    }

    /**
     * @brief Disables the timer from a slot
     */
    void disable_timer_from_slot() noexcept
    {
        _sleep.cancel_wait();
    }

    /**
     * @brief Sets the timer interval
     * @param duration The new timer interval
     */
    template<typename Duration>
    void timer_ms(const Duration &duration) noexcept
    {
        _timer_ms.store(std::chrono::duration_cast<std::chrono::milliseconds>(duration));
    }

    /**
     * @brief Gets the timer interval
     * @return The timer interval in milliseconds
     */
    std::chrono::milliseconds timer_ms() const noexcept
    {
        return _timer_ms.load();
    }

private:

    /**
     * @brief Helper class for timer sleep with cancellation
     */
    class
    {
    public:
        /**
         * @brief Waits for the specified duration or until cancelled
         * @param duration The duration to wait
         * @return true if the wait completed normally, false if it was cancelled
         */
        bool wait_for(const std::chrono::milliseconds &duration)
        {
            std::unique_lock<std::mutex> lock { _m };

            return !_cv.wait_for(lock, duration, [this]{ return _cancelled; });
        }

        /**
         * @brief Cancels any ongoing wait; the flag is set under the mutex, so the wake-up cannot be missed
         */
        void cancel_wait()
        {
            {
                std::scoped_lock lock { _m };

                _cancelled = true;
            }

            _cv.notify_all();
        }

        /**
         * @brief Resets the cancellation state
         */
        void reset()
        {
            std::scoped_lock lock { _m };

            _cancelled = false;
        }

    private:

        std::condition_variable _cv {};
        std::mutex _m {};
        bool _cancelled { false };
    } _sleep;

    std::atomic_bool _timer_enabled { false };
    std::atomic<std::chrono::milliseconds> _timer_ms { 1s };
    std::tuple<timer_signal*, Args...> _args {};
    std::jthread _timer_thread {};    // declared last, so it is joined before the members it uses are destroyed

    /**
     * @brief Timer procedure that emits the signal at regular intervals
     */
    void timer_procedure()
    {
        while (_sleep.wait_for(_timer_ms.load()))
        {
            std::apply([this](timer_signal *s, const Args&... a){ base_class::emit(s, a...); }, _args);
        }

        _timer_enabled = false;
    }
};

/**
 * @brief A collection of signals indexed by a key
 * 
 * @tparam Key The key type for indexing signals
 * @tparam SignalType The signal type
 * @tparam Args Types of arguments the signals pass to slots
 */
template<typename Key, template <typename...> typename SignalType, typename... Args>
requires std::derived_from<SignalType<Args...>, signal_base>
class signal_set_base
{
public:
    using signal_type = SignalType<Args...>;
    using key_type = Key;

    /**
     * @brief Default constructor
     */
    signal_set_base() = default;
    
    /**
     * @brief Virtual destructor
     */
    virtual ~signal_set_base() = default;

    /**
     * @brief Emits all signals with the given arguments
     * @param args Arguments to pass to the slots
     */
    void emit(const Args &... args) const
    {
        for (auto &&s : _signals) s.second->emit(args...);
    }

    /**
     * @brief Function call operator to emit all signals
     * @param args Arguments to pass to the slots
     */
    void operator() (const Args &... args) const
    {
        emit(args...);
    }

    /**
     * @brief Gets or creates a signal for the given key
     * @param key The key to look up
     * @return Reference to the signal
     */
    [[nodiscard]] virtual signal_type &get_signal(const key_type &key)
    {
        auto signal { _signals.find(key) };

        if (signal != _signals.end()) return *signal->second;

        if constexpr (std::is_same_v<std::decay_t<key_type>, std::u8string>) return *_signals.emplace(key, std::make_unique<signal_type>(key)).first->second;

        return *_signals.emplace(key, std::make_unique<signal_type>()).first->second;
    }

    /**
     * @brief Checks if a signal exists for the given key
     * @param key The key to look up
     * @return true if a signal exists, false otherwise
     */
    bool exists(const key_type &key) const
    {
        return _signals.find(key) != _signals.end();
    }

    /**
     * @brief Gets all signal keys
     * @return Set of all signal keys
     */
    [[nodiscard]] std::unordered_set<key_type> get_signal_keys() const
    {
        std::unordered_set<key_type> signal_keys;

        for (const auto &[key, _] : _signals) signal_keys.insert(key);

        return signal_keys;
    }

    /**
     * @brief Gets the number of signals
     * @return The number of signals
     */
    [[nodiscard]] auto get_signal_count() const
    {
        return std::size(_signals);
    }

    /**
     * @brief Subscript operator to get or create a signal
     * @param key The key to look up
     * @return Reference to the signal
     */
    [[nodiscard]] signal_type &operator[](const key_type &key)
    {
        return get_signal(key);
    }

    /**
     * @brief Iterator to the beginning of the signals
     * @return Iterator to the beginning
     */
    auto begin() const
    {
        return std::begin(_signals);
    }

    /**
     * @brief Iterator to the end of the signals
     * @return Iterator to the end
     */
    auto end() const
    {
        return std::end(_signals);
    }

protected:
    std::unordered_map<key_type, std::unique_ptr<signal_type>> _signals {};
};

/**
 * @brief A collection of bridged signals indexed by a key
 * 
 * @tparam Key The key type for indexing signals
 * @tparam BridgedSignalType The bridged signal type
 * @tparam Args Types of arguments the signals pass to slots
 */
template<typename Key, template <typename...> typename BridgedSignalType, typename... Args>
requires std::derived_from<BridgedSignalType<Args...>, signal_base>
class bridged_signal_set_base : public signal_set_base<Key, BridgedSignalType, Args...>
{
public:
    using base_class = signal_set_base<Key, BridgedSignalType, Args...>;
    using key_type = Key;

    /**
     * @brief Constructor with emit functor
     * @param emit_functor Function to call when signals are emitted
     */
    bridged_signal_set_base(const std::function<bool(typename base_class::signal_type::bridged_signal_type*)>& emit_functor) : base_class {}, _emit_functor { emit_functor } {}

    /**
     * @brief Gets or creates a signal for the given key
     * @param key The key to look up
     * @return Reference to the signal
     */
    [[nodiscard]] virtual typename base_class::signal_type &get_signal(const key_type &key) override
    {
        auto &signal { base_class::get_signal(key) };

        if (!signal.get_emit_functor() && _emit_functor) signal.set_emit_functor(_emit_functor);

        return signal;
    }

    /**
     * @brief Sets the emit functor for all signals
     * @param emit_functor Function to call when signals are emitted
     */
    void set_emit_functor(const std::function<bool(typename base_class::signal_type::bridged_signal_type*)>& emit_functor)
    {
        _emit_functor = emit_functor;
    }

    /**
     * @brief Gets the emit functor
     * @return The emit functor
     */
    const std::function<bool(typename base_class::signal_type::bridged_signal_type*)> &get_emit_functor() const noexcept
    {
        return _emit_functor;
    }

protected:
    std::function<bool(typename base_class::signal_type::bridged_signal_type*)> _emit_functor { nullptr };
};

/**
 * @brief Type alias for a signal set
 * @tparam Key The key type for indexing signals
 * @tparam Args Types of arguments the signals pass to slots
 */
template<typename Key, typename... Args> using signal_set = signal_set_base<Key, signal, Args...>;

/**
 * @brief Type alias for an extended signal set
 * @tparam Key The key type for indexing signals
 * @tparam Args Types of arguments the signals pass to slots
 */
template<typename Key, typename... Args> using signal_ex_set = signal_set_base<Key, signal_ex, Args...>;

/**
 * @brief Type alias for a throttled signal set
 * @tparam Key The key type for indexing signals
 * @tparam Args Types of arguments the signals pass to slots
 */
template<typename Key, typename... Args> using throttled_signal_set = signal_set_base<Key, throttled_signal, Args...>;

/**
 * @brief Type alias for a throttled extended signal set
 * @tparam Key The key type for indexing signals
 * @tparam Args Types of arguments the signals pass to slots
 */
template<typename Key, typename... Args> using throttled_signal_ex_set = signal_set_base<Key, throttled_signal_ex, Args...>;

/**
 * @brief Type alias for a queued signal set
 * @tparam Key The key type for indexing signals
 * @tparam Args Types of arguments the signals pass to slots
 */
template<typename Key, typename... Args> using queued_signal_set = signal_set_base<Key, queued_signal, Args...>;

/**
 * @brief Type alias for a queued extended signal set
 * @tparam Key The key type for indexing signals
 * @tparam Args Types of arguments the signals pass to slots
 */
template<typename Key, typename... Args> using queued_signal_ex_set = signal_set_base<Key, queued_signal_ex, Args...>;

/**
 * @brief Type alias for a scoped queued signal set
 * @tparam Key The key type for indexing signals
 * @tparam scope The scope for grouping queued signals
 * @tparam Args Types of arguments the signals pass to slots
 */
template<typename Key, typename scope, typename... Args> using queued_signal_scoped_set = signal_set_base<Key, queued_signal_scoped, scope, Args...>;

/**
 * @brief Type alias for a scoped queued extended signal set
 * @tparam Key The key type for indexing signals
 * @tparam scope The scope for grouping queued signals
 * @tparam Args Types of arguments the signals pass to slots
 */
template<typename Key, typename scope, typename... Args> using queued_signal_ex_scoped_set = signal_set_base<Key, queued_signal_ex_scoped, scope, Args...>;

/**
 * @brief Type alias for a bridged signal set
 * @tparam Key The key type for indexing signals
 * @tparam Args Types of arguments the signals pass to slots
 */
template<typename Key, typename... Args> using bridged_signal_set = bridged_signal_set_base<Key, bridged_signal, Args...>;

/**
 * @brief Type alias for a bridged extended signal set
 * @tparam Key The key type for indexing signals
 * @tparam Args Types of arguments the signals pass to slots
 */
template<typename Key, typename... Args> using bridged_signal_ex_set = bridged_signal_set_base<Key, bridged_signal_ex, Args...>;

/**
 * @brief A container for managing multiple connections
 */
struct connection_bag
{
    std::deque<connection> connections;

    /**
     * @brief Assignment operator to add a connection to the bag
     * @param c The connection to add
     * @return Reference to this connection_bag
     */
    connection_bag &operator= (connection &&c) { connections.push_front(std::forward<connection>(c)); return *this; }
};

}