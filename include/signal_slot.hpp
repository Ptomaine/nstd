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
    void disconnect()
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
    const connected_paired_ptr_type * connected_ptr() const
    {
        return _connected_paired_ptr;
    }

    /**
     * @brief Boolean conversion operator
     * @return true if connected to another paired_ptr, false otherwise
     */
    operator bool() const
    {
        return (_connected_paired_ptr);
    }

    /**
     * @brief Negation operator
     * @return true if not connected to another paired_ptr, false otherwise
     */
    bool operator!() const
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

/**
 * @brief Base class for slots
 * 
 * Slot base provides common functionality for all slot types,
 * including connection management and enabling/disabling.
 */
class slot_base
{
protected:
    template<typename... Args> friend class signal;

    paired_ptr<> _connection {};
    bool _enabled { true };

public:
    /**
     * @brief Sets whether the slot is enabled
     * @param is_enabled true to enable the slot, false to disable it
     */
    void set_enabled(bool is_enabled) { _enabled = is_enabled; }
    
    /**
     * @brief Checks if the slot is enabled
     * @return true if the slot is enabled, false otherwise
     */
    bool is_enabled() const { return _enabled; }
    
    /**
     * @brief Checks if the slot is disconnected
     * @return true if the slot is disconnected, false otherwise
     */
    bool is_disconnected() const { return !_connection; }
    
    /**
     * @brief Disconnects the slot from its signal
     */
    void disconnect() { _connection.disconnect(); }
};

/**
 * @brief A slot that can be connected to a signal
 * 
 * A slot is essentially a wrapper around a function or method that can be
 * invoked when a signal is emitted.
 * 
 * @tparam Args Types of arguments the slot accepts
 */
template<typename... Args>
class slot : public slot_base
{
protected:
    std::function<void (Args...)> _functor {};
    int64_t _priority{ 0 };

    friend class connection;

public:
    /**
     * @brief Default constructor
     */
    slot() = default;
    
    /**
     * @brief Constructor with a function and priority
     * @param f Function to be called when the slot is invoked
     * @param priority Priority of the slot (lower value means higher priority; slots with lower priority values are called first)
     */
    slot(std::function<void(Args...)>&& f, int64_t priority = 0) : _functor{ std::forward<std::function<void(Args...)>>(f) }, _priority{ priority } {}
    
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
     * @brief Gets the priority of the slot
     * @return The slot's priority
     */
    int64_t get_priority() const { return _priority; }
    
    /**
     * @brief Sets the priority of the slot
     * @param priority New priority value
     * @return The previous priority value
     */
    int64_t set_priority(int64_t priority) { std::swap(_priority, priority); return priority; }

    /**
     * @brief Compares this slot with a paired_ptr
     * @param s The paired_ptr to compare with
     * @return true if this slot is connected to the given paired_ptr
     */
    bool operator == (const paired_ptr<> &s) const
    {
        return &_connection == s.connected_ptr();
    }

    /**
     * @brief Compares this slot with another slot
     * @param s The slot to compare with
     * @return true if both slots have the same connection
     */
    bool operator == (const slot &s) const
    {
        return operator ==(s._connection);
    }

    /**
     * @brief Less-than comparison based on priority
     * @param s The slot to compare with
     * @return true if this slot has lower priority than the given slot
     */
    bool operator < (const slot& s) const
    {
        return _priority < s._priority;
    }

    /**
     * @brief Greater-than comparison based on priority
     * @param s The slot to compare with
     * @return true if this slot has higher priority than the given slot
     */
    bool operator > (const slot& s) const
    {
        return _priority > s._priority;
    }
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
     * @return The signal's name
     */
    virtual std::u8string_view name() const = 0;
    
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
    virtual size_t size() const = 0;
    
    /**
     * @brief Gets the signal's payload
     * @return Reference to the signal's payload
     */
    virtual std::any &payload() = 0;
    
    /**
     * @brief Enables or disables a slot
     * @param slot The slot to enable/disable
     * @param enabled true to enable the slot, false to disable it
     */
    virtual void enable_slot(const slot_base& slot, bool enabled) = 0;
    
    /**
     * @brief Enables or disables a slot by its connection
     * @param connection The connection to the slot
     * @param enabled true to enable the slot, false to disable it
     */
    virtual void enable_slot(const paired_ptr<>& connection, bool enabled) = 0;
    
    /**
     * @brief Checks if a slot is enabled
     * @param slot The slot to check
     * @return true if the slot is enabled, false otherwise
     */
    virtual bool is_slot_enabled(const slot_base& slot) const = 0;
    
    /**
     * @brief Checks if a slot is enabled by its connection
     * @param connection The connection to the slot
     * @return true if the slot is enabled, false otherwise
     */
    virtual bool is_slot_enabled(const paired_ptr<>& connection) const = 0;
    
    /**
     * @brief Gets the priority of a connection
     * @param connection The connection to check
     * @return The priority of the connection
     */
    virtual int64_t get_connection_priority(const paired_ptr<>& connection) const = 0;
    
    /**
     * @brief Sets the priority of a connection
     * @param connection The connection to modify
     * @param priority The new priority
     * @return true if the priority was set successfully, false otherwise
     */
    virtual bool set_connection_priority(const paired_ptr<>& connection, int64_t priority) = 0;
};

/**
 * @brief Class representing a connection between a signal and a slot
 * 
 * This class manages the lifetime of a connection between a signal and a slot.
 * When a connection object is destroyed, the connection is automatically broken.
 */
class connection
{
public:
    /**
     * @brief Default constructor creates an empty connection
     */
    connection() = default;

    /**
     * @brief Constructor that connects a signal and a slot
     * @param signal The signal to connect to
     * @param s The slot to connect
     */
    template<typename... Args>
    connection(const signal_base *signal, slot<Args...> &s) : _connection{ &s._connection }, _signal { signal }
    {
    }

    /**
     * @brief Move constructor
     * @param other The connection to move from
     */
    connection(connection &&other) noexcept = default;
    
    connection(const connection &other) = delete;
    connection &operator=(const connection &other) = delete;
    
    /**
     * @brief Move assignment operator
     * @param other The connection to move from
     * @return Reference to this connection
     */
    connection &operator=(connection &&other) noexcept
    {
        disconnect();

        _connection = std::move(other._connection);
        _signal = std::move(other._signal);

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
     */
    void disconnect()
    {
        if (_connection) _connection.disconnect();

        _signal = nullptr;
    }

    /**
     * @brief Checks if the connection is disconnected
     * @return true if the connection is disconnected, false otherwise
     */
    bool is_disconnected() const { return !_connection; }

    /**
     * @brief Gets the signal this connection is connected to
     * @return Reference to the connected signal
     */
    const signal_base &signal() const
    {
        return *_signal;
    }

    /**
     * @brief Enables or disables this connection
     * @param enabled true to enable the connection, false to disable it
     */
    void set_enabled(bool enabled)
    {
        const_cast<signal_base*>(_signal)->enable_slot(_connection, enabled);
    }

    /**
     * @brief Checks if this connection is enabled
     * @return true if the connection is enabled, false otherwise
     */
    bool is_enabled() const
    {
        return _signal->is_slot_enabled(_connection);
    }

    /**
     * @brief Gets the priority of this connection
     * @return The connection's priority
     */
    int64_t get_connection_priority() const
    {
        return _signal->get_connection_priority(_connection);
    }

    /**
     * @brief Sets the priority of this connection
     * @param priority The new priority
     * @return true if the priority was set successfully, false otherwise
     */
    bool set_connection_priority(int64_t priority)
    {
        return const_cast<signal_base*>(_signal)->set_connection_priority(_connection, priority);
    }

protected:
    paired_ptr<> _connection {};
    const signal_base *_signal { nullptr };
};

/**
 * @brief The core signal class that can be connected to slots
 * 
 * A signal can have multiple slots connected to it. When the signal is emitted,
 * all connected slots are invoked in order of their priority.
 * 
 * @tparam Args Types of arguments the signal passes to slots when emitted
 */
template<typename... Args>
class signal : public signal_base
{
public:
    using slot_type = slot<Args...>;
    using slot_container = std::multiset<slot_type, std::less<slot<Args...>>, std::allocator<slot<Args...>>>;

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
     * @brief Move constructor
     * @param other The signal to move from
     */
    signal(signal &&other) noexcept = default;
    
    /**
     * @brief Move assignment operator
     * @param other The signal to move from
     * @return Reference to this signal
     */
    signal &operator=(signal &&other) noexcept = default;
    
    /**
     * @brief Virtual destructor
     */
    virtual ~signal() override = default;

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
     * @param callable Function to connect
     * @param priority Priority of the connection
     * @return A connection object that manages the connection's lifetime
     */
    virtual connection connect(std::function<void(Args...)> &&callable, int64_t priority = 0)
    {
        std::scoped_lock<std::mutex> lock { _connect_lock };
        auto end { std::end(_pending_connections) };

        _pending_connections.erase(std::remove_if(std::begin(_pending_connections), end, std::mem_fn(&slot<Args...>::is_disconnected)), end);
        _pending_connections.emplace_back(std::forward<std::function<void(Args...)>>(callable), priority);

        return { this, _pending_connections.back() };
    }

    /**
     * @brief Operator += to connect a function to this signal
     * @param callable Function to connect
     * @return A connection object that manages the connection's lifetime
     */
    virtual connection operator += (std::function<void(Args...)> &&callable)
    {
        return connect(std::forward<std::function<void(Args...)>>(callable));
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
    connection connect(T *instance, void (T::*member_function)(Args...), int64_t priority = 0)
    {
        return connect([instance, member_function](Args... args) { (instance->*member_function)(args...); }, priority);
    }

    /**
     * @brief Disconnects all slots from this signal
     */
    virtual void clear()
    {
        std::scoped_lock<std::mutex, std::mutex> lock { _connect_lock, _emit_lock };

        _pending_connections.clear();
        _slots.clear();
    }

    /**
     * @brief Gets the number of slots connected to this signal
     * @return Number of connected slots
     */
    virtual size_t size() const override
    {
        std::scoped_lock<std::mutex> lock { _emit_lock };

        return std::size(_slots);
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
     * @return The signal's name
     */
    virtual std::u8string_view name() const override
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
     * @brief Gets this signal's payload
     * @return Reference to the signal's payload
     */
    virtual std::any &payload() override
    {
        return _payload;
    }

    /**
     * @brief Enables or disables a slot
     * @param slot The slot to enable/disable
     * @param enabled true to enable the slot, false to disable it
     */
    virtual void enable_slot(const slot_base &slot, bool enabled) override
    {
        enable_slot(slot._connection, enabled);
    }

    /**
     * @brief Enables or disables a slot by its connection
     * @param s The connection to the slot
     * @param enabled true to enable the slot, false to disable it
     */
    virtual void enable_slot(const paired_ptr<> &s, bool enabled) override
    {
        auto send { std::end(_slots) };
        auto sit { std::find(std::begin(_slots), send, s) };
        auto& slt{ const_cast<slot<Args...>&>(*sit)};

        if (sit != send) slt.set_enabled(enabled);
    }

    /**
     * @brief Checks if a slot is enabled
     * @param slot The slot to check
     * @return true if the slot is enabled, false otherwise
     */
    virtual bool is_slot_enabled(const slot_base &slot) const override
    {
        return is_slot_enabled(slot._connection);
    }

    /**
     * @brief Checks if a slot is enabled by its connection
     * @param slot The connection to the slot
     * @return true if the slot is enabled, false otherwise
     */
    virtual bool is_slot_enabled(const paired_ptr<> &slot) const override
    {
        if (auto s_ptr = find_slot(slot); s_ptr != nullptr) return s_ptr->is_enabled();

        return false;
    }

    /**
     * @brief Gets the priority of a connection
     * @param con The connection to check
     * @return The priority of the connection
     */
    virtual int64_t get_connection_priority(const paired_ptr<>& con) const override
    {
        if (auto s_ptr = find_slot(con); s_ptr != nullptr) return s_ptr->get_priority();

        return {};
    }

    /**
     * @brief Sets the priority of a connection
     * @param con The connection to modify
     * @param priority The new priority
     * @return true if the priority was set successfully, false otherwise
     */
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

    /**
     * @brief Finds a slot by its connection
     * @param con The connection to find
     * @return Pointer to the slot if found, nullptr otherwise
     */
    const slot_type* find_slot(const paired_ptr<>& con) const
    {
        auto send{ std::cend(_slots) };
        auto sit{ std::find(std::cbegin(_slots), send, con) };

        if (sit != send) return &(*sit);

        auto psend{ std::cend(_pending_connections) };
        auto psit{ std::find(std::cbegin(_pending_connections), psend, con) };

        if (psit != psend) return &(*psit);

        return nullptr;
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
 * actually delivered to the slots.
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
     * @param emit_functor Function to call when signal is emitted
     */
    bridged_signal_base(const std::u8string &name, const std::function<bool(bridged_signal_base*)>& emit_functor) : base_class { name }, _emit_functor { emit_functor } {}
    
    /**
     * @brief Constructor with emit functor
     * @param emit_functor Function to call when signal is emitted
     */
    bridged_signal_base(const std::function<bool(bridged_signal_base*)>& emit_functor) : bridged_signal_base { std::u8string {}, emit_functor } {}
    
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
    
    /**
     * @brief Move constructor
     * @param other The signal to move from
     */
    bridged_signal_base(bridged_signal_base &&other) noexcept = default;
    
    /**
     * @brief Move assignment operator
     * @param other The signal to move from
     * @return Reference to this signal
     */
    bridged_signal_base &operator=(bridged_signal_base &&other) noexcept = default;
    
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
        if (std::empty(_signal_queue)) return false;

        std::scoped_lock<std::mutex> lock { _queue_lock };

        if (std::empty(_signal_queue)) return false;

        std::apply([this](const Args&... a){ base_class::emit(a...); }, _signal_queue.front());

        _signal_queue.pop_front();

        return !std::empty(_signal_queue);
    }

    /**
     * @brief Invokes all queued signal emissions
     */
    virtual void invoke_all()
    {
        if (std::empty(_signal_queue)) return;

        std::scoped_lock<std::mutex> lock { _queue_lock };

        if (std::empty(_signal_queue)) return;

        for (auto &&args : _signal_queue) std::apply([this](const Args&... a){ base_class::emit(a...); }, args);

        _signal_queue.clear();
    }

    /**
     * @brief Invokes only the last queued signal emission and clears the queue
     */
    virtual void invoke_last_and_clear()
    {
        if (std::empty(_signal_queue)) return;

        std::scoped_lock<std::mutex> lock { _queue_lock };

        if (std::empty(_signal_queue)) return;

        std::apply([this](const Args&... a){ base_class::emit(a...); }, _signal_queue.back());

        _signal_queue.clear();
    }

    /**
     * @brief Sets the emit functor
     * @param emit_functor Function to call when signal is emitted
     */
    void set_emit_functor(const std::function<bool(bridged_signal_base*)>& emit_functor)
    {
        _emit_functor = emit_functor;
    }

    /**
     * @brief Gets the emit functor
     * @return The emit functor
     */
    const std::function<bool(bridged_signal_base*)> &get_emit_functor() const
    {
        return _emit_functor;
    }

    /**
     * @brief Gets the size of the signal queue
     * @return Number of queued signal emissions
     */
    uint64_t get_queue_size() const
    {
        std::scoped_lock<std::mutex> lock { _queue_lock };

        return std::size(_signal_queue);
    }

    /**
     * @brief Sets whether bridging is enabled
     * @param enabled true to enable bridging, false to disable it
     */
    void set_bridge_enabled(bool enabled)
    {
        _bridge_enabled = enabled;
    }

    /**
     * @brief Checks if bridging is enabled
     * @return true if bridging is enabled, false otherwise
     */
    bool get_bridge_enabled() const
    {
        return _bridge_enabled;
    }

    /**
     * @brief Clears the signal queue
     */
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
 * A throttled signal limits the rate at which signal emissions are delivered to slots.
 * 
 * @tparam signal_type The signal type to throttle
 * @tparam Args Types of arguments the signal passes to slots
 */
template<template <typename...> typename signal_type, std::copyable... Args>
requires std::derived_from<signal_type<Args...>, signal_base>
class throttled_signal_base : public signal_type<Args...>
{
public:
    using base_class = signal_type<Args...>;

    /**
     * @brief Default constructor
     */
    throttled_signal_base() = default;
    
    /**
     * @brief Constructor with a name and throttle duration
     * @param name The name of the signal
     * @param throttle_ms The throttling duration
     */
    template<typename Duration>
    throttled_signal_base(const std::u8string &name, const Duration &throttle_ms = throttled_signal_base::_default_throttle_ms) : base_class{ name }, _throttle_ms{ std::chrono::duration_cast<std::chrono::milliseconds>(throttle_ms) } {}
    
    /**
     * @brief Constructor with a name
     * @param name The name of the signal
     */
    throttled_signal_base(const std::u8string &name) : base_class{ name } {}
    
    /**
     * @brief Move constructor
     * @param other The signal to move from
     */
    throttled_signal_base(throttled_signal_base &&other) noexcept = default;
    
    /**
     * @brief Move assignment operator
     * @param other The signal to move from
     * @return Reference to this signal
     */
    throttled_signal_base &operator=(throttled_signal_base &&other) noexcept = default;

    /**
     * @brief Destructor that optionally dispatches pending signals
     */
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

    /**
     * @brief Emits the signal with the given arguments
     * 
     * Signal emissions are queued and delivered at a controlled rate.
     * 
     * @param args Arguments to pass to the slots
     */
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
    void throttle_ms(const Duration &duration)
    {
        _throttle_ms.store(std::chrono::duration_cast<std::chrono::milliseconds>(duration));
    }

    /**
     * @brief Gets the throttling duration
     * @return The throttling duration in milliseconds
     */
    std::chrono::milliseconds throttle_ms() const
    {
        return _throttle_ms.load();
    }

    /**
     * @brief Sets whether all pending signals should be dispatched on destroy
     * @param do_dispatch true to dispatch all pending signals, false otherwise
     */
    void set_dispatch_all_on_destroy(bool do_dispatch)
    {
        _dispatch_all_on_destroy = do_dispatch;
    }

    /**
     * @brief Checks if all pending signals will be dispatched on destroy
     * @return true if all pending signals will be dispatched, false otherwise
     */
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

    /**
     * @brief Thread function that dispatches queued signals
     * @param cancelled Stop token for thread cancellation
     */
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

/**
 * @brief Base class for queued signals
 * 
 * A queued signal queues emissions in a static queue, shared by all signals of
 * the same scope.
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

    /**
     * @brief Default constructor
     */
    queued_signal_base() = default;
    
    /**
     * @brief Constructor with a name and delay
     * @param name The name of the signal
     * @param delay_ms The delay between emissions
     */
    template<typename Duration>
    queued_signal_base(const std::u8string& name, const Duration& delay_ms = 0ms) : base_class{ name } { _delay_ms = delay_ms; }
    
    /**
     * @brief Constructor with a name
     * @param name The name of the signal
     */
    queued_signal_base(const std::u8string &name) : base_class{ name } {}
    
    /**
     * @brief Move constructor
     * @param other The signal to move from
     */
    queued_signal_base(queued_signal_base &&other) noexcept = default;
    
    /**
     * @brief Move assignment operator
     * @param other The signal to move from
     * @return Reference to this signal
     */
    queued_signal_base &operator=(queued_signal_base &&other) noexcept = default;
    
    queued_signal_base(const queued_signal_base &other) = delete;
    queued_signal_base &operator=(const queued_signal_base &other) = delete;

    /**
     * @brief Destructor that optionally dispatches pending signals
     */
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
        std::scoped_lock<std::mutex> lock { _emit_lock };

        _signal_queue.push_back({ this, std::make_tuple(args...) });

        if (!_thread_running.exchange(true))
        {
            if (_dispatcher_thread.joinable()) _dispatcher_thread.join();

            _dispatcher_thread = std::jthread(&queue_dispatcher);
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
     * @brief Sets the delay between emissions
     * @param duration The new delay
     */
    template<typename Duration>
    static void delay_ms(const Duration &duration)
    {
        _delay_ms.store(std::chrono::duration_cast<std::chrono::milliseconds>(duration));
    }

    /**
     * @brief Gets the delay between emissions
     * @return The delay in milliseconds
     */
    static std::chrono::milliseconds delay_ms()
    {
        return _delay_ms.load();
    }

    /**
     * @brief Sets whether to use delay between emissions
     * @param use true to use delay, false otherwise
     */
    static void use_delay(bool use)
    {
        _use_delay = use;
    }

    /**
     * @brief Checks if delay is used between emissions
     * @return true if delay is used, false otherwise
     */
    static bool use_delay()
    {
        return _use_delay;
    }

    /**
     * @brief Sets whether all pending signals should be dispatched on destroy
     * @param do_dispatch true to dispatch all pending signals, false otherwise
     */
    void set_dispatch_all_on_destroy(bool do_dispatch)
    {
        _dispatch_all_on_destroy = do_dispatch;
    }

    /**
     * @brief Checks if all pending signals will be dispatched on destroy
     * @return true if all pending signals will be dispatched, false otherwise
     */
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

    /**
     * @brief Thread function that dispatches queued signals
     * @param cancelled Stop token for thread cancellation
     */
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

                if (std::empty(_signal_queue))
                {
                    _thread_running = false;
                    break;
                }
            }

            if (_use_delay) std::this_thread::sleep_for(_delay_ms.load());
        }

        _thread_running = false;
    }
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
 * @brief A signal that emits at regular intervals
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
     * @brief Constructor with a name and timer duration
     * @param name The name of the signal
     * @param timer_ms The timer interval
     */
    template<typename Duration>
    timer_signal(const std::u8string &name, const Duration &timer_ms = 1s) : base_class{ name }, _timer_ms{ std::chrono::duration_cast<std::chrono::milliseconds>(timer_ms) } {}
    
    /**
     * @brief Move constructor
     * @param other The signal to move from
     */
    timer_signal(timer_signal &&other) noexcept = default;
    
    /**
     * @brief Move assignment operator
     * @param other The signal to move from
     * @return Reference to this signal
     */
    timer_signal &operator=(timer_signal &&other) noexcept = default;

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
            _args = std::make_tuple(this, args...);

            if (_timer_thread.joinable()) _timer_thread.join();

            _sleep.reset();

            _timer_thread = std::jthread([this](){ timer_procedure(); });
        }
    }

    /**
     * @brief Stops the timer
     */
    void stop_timer()
    {
        _sleep.cancel_wait();

        if (_timer_thread.joinable()) _timer_thread.join();
    }

    /**
     * @brief Disables the timer from a slot
     */
    void disable_timer_from_slot()
    {
        _sleep.cancel_wait();
    }

    /**
     * @brief Sets the timer interval
     * @param duration The new timer interval
     */
    template<typename Duration>
    void timer_ms(const Duration &duration)
    {
        _timer_ms.store(std::chrono::duration_cast<std::chrono::milliseconds>(duration));
    }

    /**
     * @brief Gets the timer interval
     * @return The timer interval in microseconds
     */
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

            return !_cv.wait_for(lock, duration, [this]{ return _cancelled.load(); });
        }

        /**
         * @brief Cancels any ongoing wait
         */
        void cancel_wait()
        {
            _cancelled = true;

            _cv.notify_all();
        }

        /**
         * @brief Resets the cancellation state
         */
        void reset()
        {
            _cancelled = false;
        }

    private:

        std::condition_variable _cv {};
        std::mutex _m {};
        std::atomic_bool _cancelled { false };
    } _sleep;


    /**
     * @brief Timer procedure that emits the signal at regular intervals
     */
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
    virtual signal_type &get_signal(const key_type &key)
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
    std::unordered_set<key_type> get_signal_keys() const
    {
        std::unordered_set<key_type> signal_keys;

        for (const auto &[key, _] : _signals) signal_keys.insert(key);

        return signal_keys;
    }

    /**
     * @brief Gets the number of signals
     * @return The number of signals
     */
    auto get_signal_count() const
    {
        return std::size(_signals);
    }

    /**
     * @brief Subscript operator to get or create a signal
     * @param key The key to look up
     * @return Reference to the signal
     */
    signal_type &operator[](const key_type &key)
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
    virtual typename base_class::signal_type &get_signal(const key_type &key) override
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
    const std::function<bool(typename base_class::signal_type::bridged_signal_type*)> &get_emit_functor() const
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