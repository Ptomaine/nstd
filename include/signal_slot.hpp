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
#include <cstdint>
#include <deque>
#include <functional>
#include <iterator>
#include <map>
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
 * @brief A thread-safe signal-slot implementation
 *
 * A signal calls its connected slots when it is emitted. How a slot is called is
 * decided per connection:
 * - connected without an executor, the slot runs inside emit(), on the emitting thread;
 * - connected with an executor, emit() posts the call to it: a worker_thread, a
 *   manual_executor you drain yourself, or your own executor (a UI thread, say);
 * - throttle() wraps a slot so it runs at most once per interval with the latest arguments.
 *
 * Connections disconnect automatically when destroyed, either side may be destroyed
 * first, and a queued call whose connection is gone by the time it runs is skipped.
 */
namespace nstd::signal_slot
{

using namespace std::chrono_literals;

class signal_base;
class connection;

/**
 * @brief Runs queued slot calls
 *
 * Executors are owned through std::shared_ptr. Connections keep only a weak
 * reference, so calls posted after an executor is gone are dropped.
 */
class executor
{
public:
#if defined(__cpp_lib_move_only_function)
    using task_type = std::move_only_function<void()>;
#else
    using task_type = std::function<void()>;
#endif

    virtual ~executor() = default;

    /**
     * @brief Runs the task as soon as possible; tasks posted from one thread run in posting order
     * @param task The task to run
     */
    virtual void post(task_type task) = 0;

    /**
     * @brief Runs the task once the delay has passed
     * @param delay The minimum delay
     * @param task The task to run
     */
    virtual void post_after(std::chrono::milliseconds delay, task_type task) = 0;
};

/**
 * @brief A slot wrapped by throttle(); connect it with an executor
 * @tparam F The wrapped callable
 */
template<typename F>
struct throttled
{
    std::chrono::milliseconds interval;
    F function;
};

/**
 * @brief Makes a slot that runs at most once per interval, with the latest arguments
 *
 * The first call runs right away; calls arriving within the interval are merged
 * into one trailing call with the latest arguments. An interval of zero only merges
 * calls that pile up while one is waiting in the executor.
 *
 * @param interval The minimum time between two calls
 * @param function The slot
 * @return A throttled slot, to be connected with an executor
 */
template<typename Rep, typename Period, typename F>
[[nodiscard]] throttled<std::decay_t<F>> throttle(const std::chrono::duration<Rep, Period> &interval, F &&function)
{
    return { std::chrono::ceil<std::chrono::milliseconds>(interval), std::forward<F>(function) };
}

namespace detail
{

/**
 * @brief Argument types a queued call can copy and keep
 */
template<typename T>
concept queueable = std::is_object_v<T> && std::copy_constructible<T>;

/**
 * @brief State a signal shares with its slots
 */
struct signal_core_base
{
    virtual ~signal_core_base() = default;

    /**
     * @brief Re-sorts the slots after a priority change; called with @ref lock held
     */
    virtual void resort_locked() = 0;

    std::mutex lock {};
    const signal_base *owner { nullptr };    ///< The signal, or nullptr once it is gone
};

/**
 * @brief The part of a slot that does not depend on the argument types
 */
class slot_base
{
public:
    virtual ~slot_base() = default;

    void set_enabled(bool enabled) noexcept { _enabled = enabled; }
    bool is_enabled() const noexcept { return _enabled; }
    bool is_disconnected() const noexcept { return !_connected; }
    void disconnect() noexcept { _connected = false; }
    int64_t priority() const noexcept { return _priority; }

    void set_priority(int64_t priority)
    {
        if (auto core { _core.lock() })
        {
            std::scoped_lock lock { core->lock };

            _priority = priority;
            core->resort_locked();

            return;
        }

        _priority = priority;
    }

    std::atomic_bool _connected { true };
    std::atomic_bool _enabled { true };
    std::atomic<int64_t> _priority { 0 };
    uint64_t _order { 0 };                            ///< Connection sequence number; keeps equal priorities in connection order
    std::weak_ptr<signal_core_base> _core {};
};

/**
 * @brief A connected slot: the callable plus how it is delivered
 */
template<typename... Args>
class slot final : public slot_base
{
public:
#if defined(__cpp_lib_move_only_function)
    using functor_type = std::move_only_function<void(const Args &...)>;
#else
    using functor_type = std::function<void(const Args &...)>;
#endif

    struct throttle_state
    {
        std::mutex lock {};
        std::chrono::milliseconds interval {};
        std::chrono::steady_clock::time_point last {};
        std::optional<std::tuple<Args...>> pending {};
        bool scheduled { false };
    };

    slot(functor_type &&function, int64_t priority) : _functor { std::move(function) } { _priority = priority; }

    void invoke(const Args &... args) { _functor(args...); }

    /**
     * @brief Calls the slot, or posts the call to its executor
     */
    void deliver(const std::shared_ptr<slot> &self, const Args &... args)
    {
        if constexpr ((queueable<Args> && ...))
        {
            if (_queued)
            {
                auto target { _executor.lock() };

                if (!target) return;

                if (_throttle)
                {
                    post_throttled(self, *target, args...);
                }
                else
                {
                    target->post([weak = std::weak_ptr<slot> { self }, values = std::tuple<Args...>(args...)]
                    {
                        if (auto s { weak.lock() }; s && !s->is_disconnected() && s->is_enabled())
                            std::apply([&s](const Args &... a) { s->invoke(a...); }, values);
                    });
                }

                return;
            }
        }

        invoke(args...);
    }

    bool _queued { false };
    std::weak_ptr<executor> _executor {};
    std::unique_ptr<throttle_state> _throttle {};

private:
    functor_type _functor;

    void post_throttled(const std::shared_ptr<slot> &self, executor &target, const Args &... args)
    {
        auto &state { *_throttle };
        std::chrono::steady_clock::duration wait { 0 };

        {
            std::scoped_lock lock { state.lock };

            state.pending.emplace(args...);

            if (state.scheduled) return;

            state.scheduled = true;

            const auto due { state.last + state.interval }, now { std::chrono::steady_clock::now() };

            if (due > now) wait = due - now;
        }

        auto task { [weak = std::weak_ptr<slot> { self }] { if (auto s { weak.lock() }) s->run_throttled(); } };

        if (wait > std::chrono::steady_clock::duration::zero()) target.post_after(std::chrono::ceil<std::chrono::milliseconds>(wait), std::move(task));
        else target.post(std::move(task));
    }

    void run_throttled()
    {
        std::optional<std::tuple<Args...>> values;

        {
            std::scoped_lock lock { _throttle->lock };

            values.swap(_throttle->pending);
            _throttle->scheduled = false;
            _throttle->last = std::chrono::steady_clock::now();
        }

        if (values && !is_disconnected() && is_enabled()) std::apply([this](const Args &... a) { invoke(a...); }, *values);
    }
};

}

/**
 * @brief Base interface class for all signals
 */
class signal_base
{
public:
    virtual ~signal_base() = default;

    /**
     * @brief Gets the name the signal was created with
     * @return The signal's name
     */
    [[nodiscard]] virtual std::u8string_view name() const noexcept = 0;

    /**
     * @brief Sets whether emit() calls any slot
     * @param enabled true to enable the signal, false to disable it
     */
    virtual void set_enabled(bool enabled) noexcept = 0;

    /**
     * @brief Checks if the signal is enabled
     * @return true if the signal is enabled, false otherwise
     */
    [[nodiscard]] virtual bool is_enabled() const noexcept = 0;

    /**
     * @brief Gets the number of connected slots
     * @return Number of connected slots
     */
    [[nodiscard]] virtual std::size_t size() const = 0;
};

/**
 * @brief Owns one signal-slot connection
 *
 * The slot is disconnected when the connection is destroyed. Either side may be
 * destroyed first, on any thread.
 */
class connection
{
public:
    connection() = default;
    connection(connection &&other) noexcept = default;
    connection(const connection &other) = delete;
    connection &operator=(const connection &other) = delete;

    /**
     * @brief Move assignment operator; disconnects the current connection first
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

    ~connection()
    {
        disconnect();
    }

    /**
     * @brief Disconnects the slot
     *
     * Emissions that start after this call never call the slot, and queued calls
     * that have not started yet are skipped. A call already running on another
     * thread is not waited for.
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
     * @return The signal, or nullptr if the connection is empty or the signal is gone
     */
    [[nodiscard]] const signal_base *signal() const noexcept
    {
        if (!_slot) return nullptr;

        auto core { _slot->_core.lock() };

        if (!core) return nullptr;

        std::scoped_lock lock { core->lock };

        return core->owner;
    }

    /**
     * @brief Enables or disables the slot
     * @param enabled true to enable the slot, false to disable it
     */
    void set_enabled(bool enabled) noexcept
    {
        if (!is_disconnected()) _slot->set_enabled(enabled);
    }

    /**
     * @brief Checks if the slot is connected and enabled
     */
    [[nodiscard]] bool is_enabled() const noexcept
    {
        return !is_disconnected() && _slot->is_enabled();
    }

    /**
     * @brief Gets the slot's priority (lower values are called first)
     * @return The priority, or 0 if disconnected
     */
    [[nodiscard]] int64_t priority() const noexcept
    {
        return is_disconnected() ? 0 : _slot->priority();
    }

    /**
     * @brief Sets the slot's priority; the signal re-sorts its slots
     * @param priority The new priority
     * @return true if set, false if disconnected
     */
    bool set_priority(int64_t priority)
    {
        if (is_disconnected()) return false;

        _slot->set_priority(priority);

        return true;
    }

private:
    template<typename... Args> friend class signal;

    explicit connection(std::shared_ptr<detail::slot_base> s) noexcept : _slot { std::move(s) } {}

    std::shared_ptr<detail::slot_base> _slot {};
};

/**
 * @brief Owns a group of connections and disconnects them together
 */
class connection_bag
{
public:
    /**
     * @brief Adds a connection to the bag
     */
    connection_bag &operator+=(connection &&c)
    {
        _connections.push_back(std::move(c));

        return *this;
    }

    /**
     * @brief Adds a connection to the bag (same as +=)
     */
    connection_bag &operator=(connection &&c)
    {
        return *this += std::move(c);
    }

    /**
     * @brief Disconnects and removes all connections, newest first
     */
    void clear() noexcept
    {
        while (!std::empty(_connections)) _connections.pop_back();
    }

    /**
     * @brief Enables or disables every slot in the bag
     */
    void set_enabled(bool enabled) noexcept
    {
        for (auto &c : _connections) c.set_enabled(enabled);
    }

    [[nodiscard]] std::size_t size() const noexcept { return std::size(_connections); }
    [[nodiscard]] bool empty() const noexcept { return std::empty(_connections); }
    [[nodiscard]] auto begin() noexcept { return std::begin(_connections); }
    [[nodiscard]] auto end() noexcept { return std::end(_connections); }
    [[nodiscard]] auto begin() const noexcept { return std::cbegin(_connections); }
    [[nodiscard]] auto end() const noexcept { return std::cend(_connections); }

    ~connection_bag() { clear(); }

    connection_bag() = default;
    connection_bag(connection_bag &&) noexcept = default;
    connection_bag &operator=(connection_bag &&other) noexcept { if (this != &other) { clear(); _connections = std::move(other._connections); } return *this; }

private:
    std::vector<connection> _connections {};
};

/**
 * @brief A signal that calls its connected slots when emitted
 *
 * Slots are called in priority order (lower values first); equal priorities keep
 * connection order. Every member may be called from any thread. No internal lock
 * is held while slots run, so a slot may emit the signal again, connect,
 * disconnect, clear, or destroy the signal. Each emission works on a snapshot of
 * the slots taken when it starts. If the signal is emitted from several threads
 * at once, its direct slots can run concurrently.
 *
 * Connecting with an executor requires argument types that can be copied and
 * kept (no references): the call runs later, from copies of the arguments.
 *
 * @tparam Args Types of arguments the signal passes to slots
 */
template<typename... Args>
class signal : public signal_base
{
    using slot_type = detail::slot<Args...>;
    using slot_ptr = std::shared_ptr<slot_type>;
    using slot_list = std::vector<slot_ptr>;

    struct core_type final : detail::signal_core_base
    {
        std::shared_ptr<const slot_list> slots {};
        uint64_t next_order { 0 };

        void resort_locked() override { signal::rebuild_locked(*this, nullptr); }
    };

public:
    using functor_type = typename slot_type::functor_type;

    /**
     * @brief true if slots of this signal can be connected with an executor
     */
    static constexpr bool is_queueable { (detail::queueable<Args> && ...) };

    signal() = default;

    /**
     * @brief Constructor with a name
     * @param name The name of the signal
     */
    explicit signal(std::u8string name) : _name { std::move(name) } {}

    /**
     * @brief Move constructor; connections follow the signal to its new place
     */
    signal(signal &&other) noexcept : _core { std::move(other._core) }
    {
        take_fields(other);
    }

    /**
     * @brief Move assignment operator; this signal's own connections are disconnected first
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
     * @brief Destructor; disconnects all slots
     */
    ~signal() override
    {
        detach_core();
    }

    /**
     * @brief Calls the connected slots, or posts the calls of queued ones
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

            if (s->is_enabled()) s->deliver(s, args...);
        }

        if (purge)
        {
            std::scoped_lock lock { core->lock };

            if (core->slots) rebuild_locked(*core, nullptr);
        }
    }

    /**
     * @brief Same as emit()
     */
    void operator() (const Args &... args)
    {
        emit(args...);
    }

    /**
     * @brief Connects a callable that runs inside emit(), on the emitting thread
     * @param function Any callable taking the signal's arguments as const references
     * @param priority Lower values are called first
     * @return The connection; the slot is disconnected when it is destroyed
     */
    template<typename F>
    requires std::invocable<std::decay_t<F> &, const Args &...>
    [[nodiscard]] connection connect(F &&function, int64_t priority = 0)
    {
        return attach(std::make_shared<slot_type>(functor_type { std::forward<F>(function) }, priority));
    }

    /**
     * @brief Connects a callable whose calls are posted to an executor
     * @param function Any callable taking the signal's arguments as const references
     * @param target Where the calls run; nullptr means inside emit()
     * @param priority Lower values are posted first
     * @return The connection; queued calls are skipped once it is destroyed
     */
    template<typename F>
    requires std::invocable<std::decay_t<F> &, const Args &...> && is_queueable
    [[nodiscard]] connection connect(F &&function, std::shared_ptr<executor> target, int64_t priority = 0)
    {
        auto s { std::make_shared<slot_type>(functor_type { std::forward<F>(function) }, priority) };

        if (target)
        {
            s->_queued = true;
            s->_executor = target;
        }

        return attach(std::move(s));
    }

    /**
     * @brief Connects a throttled callable (see throttle()); its calls run on the executor
     * @param slot The result of throttle()
     * @param target Where the calls run; must not be nullptr
     * @param priority Lower values are posted first
     * @return The connection
     */
    template<typename F>
    requires std::invocable<F &, const Args &...> && is_queueable
    [[nodiscard]] connection connect(throttled<F> slot, std::shared_ptr<executor> target, int64_t priority = 0)
    {
        if (!target) throw std::invalid_argument { "nstd::signal_slot::signal::connect(): a throttled slot needs an executor" };

        auto s { std::make_shared<slot_type>(functor_type { std::move(slot.function) }, priority) };

        s->_queued = true;
        s->_executor = target;
        s->_throttle = std::make_unique<typename slot_type::throttle_state>();
        s->_throttle->interval = slot.interval;

        return attach(std::move(s));
    }

    /**
     * @brief Connects a member function that runs inside emit()
     * @param instance The object; keep the connection no longer than the object lives
     * @param member Any member function callable with the signal's arguments
     * @param priority Lower values are called first
     * @return The connection
     */
    template<typename T, typename M>
    requires std::is_member_function_pointer_v<M> && std::invocable<M, T *, const Args &...>
    [[nodiscard]] connection connect(T *instance, M member, int64_t priority = 0)
    {
        return connect([instance, member](const Args &... a) { std::invoke(member, instance, a...); }, priority);
    }

    /**
     * @brief Connects a member function whose calls are posted to an executor
     * @param instance The object; destroy the connection where the executor runs, before the object
     * @param member Any member function callable with the signal's arguments
     * @param target Where the calls run; nullptr means inside emit()
     * @param priority Lower values are posted first
     * @return The connection
     */
    template<typename T, typename M>
    requires std::is_member_function_pointer_v<M> && std::invocable<M, T *, const Args &...> && is_queueable
    [[nodiscard]] connection connect(T *instance, M member, std::shared_ptr<executor> target, int64_t priority = 0)
    {
        return connect([instance, member](const Args &... a) { std::invoke(member, instance, a...); }, std::move(target), priority);
    }

    /**
     * @brief Same as connect(function)
     */
    template<typename F>
    requires std::invocable<std::decay_t<F> &, const Args &...>
    [[nodiscard]] connection operator += (F &&function)
    {
        return connect(std::forward<F>(function));
    }

    /**
     * @brief Disconnects all slots
     */
    void clear()
    {
        if (!_core) return;

        std::scoped_lock lock { _core->lock };

        if (_core->slots) for (const auto &s : *_core->slots) s->disconnect();

        _core->slots.reset();
    }

    /**
     * @brief Gets the number of connected slots
     */
    [[nodiscard]] std::size_t size() const override
    {
        if (!_core) return 0;

        std::scoped_lock lock { _core->lock };

        if (!_core->slots) return 0;

        return static_cast<std::size_t>(std::count_if(std::begin(*_core->slots), std::end(*_core->slots), [](const auto &s) { return !s->is_disconnected(); }));
    }

    /**
     * @brief Gets the name the signal was created with
     */
    [[nodiscard]] std::u8string_view name() const noexcept override
    {
        return _name;
    }

    /**
     * @brief Sets whether emit() calls any slot
     */
    void set_enabled(bool enabled) noexcept override
    {
        _enabled = enabled;
    }

    /**
     * @brief Checks if the signal is enabled
     */
    [[nodiscard]] bool is_enabled() const noexcept override
    {
        return _enabled;
    }

private:
    std::u8string _name {};
    std::atomic_bool _enabled { true };
    std::shared_ptr<core_type> _core { make_core(this) };

    static std::shared_ptr<core_type> make_core(const signal *owner)
    {
        auto core { std::make_shared<core_type>() };

        core->owner = owner;

        return core;
    }

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

    connection attach(slot_ptr s)
    {
        if (!_core) _core = make_core(this);    // only after this signal was moved from

        std::scoped_lock lock { _core->lock };

        s->_core = _core;
        s->_order = _core->next_order++;

        rebuild_locked(*_core, s);

        return connection { std::move(s) };
    }

    void detach_core() noexcept
    {
        if (!_core) return;

        std::scoped_lock lock { _core->lock };

        _core->owner = nullptr;

        if (_core->slots) for (const auto &s : *_core->slots) s->disconnect();

        _core->slots.reset();
    }

    void take_fields(signal &other) noexcept
    {
        _name = std::move(other._name);
        _enabled = other._enabled.load();

        if (_core)
        {
            std::scoped_lock lock { _core->lock };

            _core->owner = this;
        }
    }
};

/**
 * @brief A signal that passes itself as the first argument to slots
 *
 * Useful in signal sets, where a slot needs to know which signal fired. With a
 * queued connection, the signal must outlive the call.
 *
 * @tparam Args Types of the other arguments
 */
template<typename... Args>
class signal_ex : public signal<signal_base*, Args...>
{
public:
    using base_class = signal<signal_base*, Args...>;

    signal_ex() = default;
    explicit signal_ex(std::u8string name) : base_class(std::move(name)) {}
    signal_ex(signal_ex &&other) noexcept = default;
    signal_ex &operator=(signal_ex &&other) noexcept = default;
    ~signal_ex() override = default;

    /**
     * @brief Calls the slots with this signal as the first argument
     */
    void emit(const Args&... args)
    {
        base_class::emit(this, args...);
    }

    /**
     * @brief Same as emit()
     */
    void operator() (const Args&... args)
    {
        emit(args...);
    }
};

/**
 * @brief An executor with its own background thread
 *
 * Tasks run one at a time, in posting order. The destructor runs the tasks that are
 * already due, drops delayed ones that are not, and waits for the thread, unless it
 * is called from a task on this thread, in which case the thread finishes on its own.
 * A task that throws terminates the program.
 */
class worker_thread final : public executor
{
public:
    worker_thread() : _state { std::make_shared<state>() }, _thread { [s = _state](std::stop_token stop) { run(*s, stop); } } {}

    worker_thread(const worker_thread &) = delete;
    worker_thread &operator=(const worker_thread &) = delete;

    ~worker_thread() override
    {
        _thread.request_stop();

        if (_thread.joinable())
        {
            if (_thread.get_id() == std::this_thread::get_id()) _thread.detach();
            else _thread.join();
        }
    }

    void post(task_type task) override
    {
        {
            std::scoped_lock lock { _state->lock };

            _state->ready.push_back(std::move(task));
            ++_state->generation;
        }

        _state->wake.notify_one();
    }

    void post_after(std::chrono::milliseconds delay, task_type task) override
    {
        {
            std::scoped_lock lock { _state->lock };

            _state->timed.emplace(std::chrono::steady_clock::now() + delay, std::move(task));
            ++_state->generation;
        }

        _state->wake.notify_one();
    }

    /**
     * @brief Gets the number of tasks waiting to run
     */
    [[nodiscard]] std::size_t pending() const
    {
        std::scoped_lock lock { _state->lock };

        return std::size(_state->ready) + std::size(_state->timed);
    }

    /**
     * @brief Drops the tasks waiting to run
     */
    void clear()
    {
        std::deque<task_type> ready;
        std::multimap<std::chrono::steady_clock::time_point, task_type> timed;

        {
            std::scoped_lock lock { _state->lock };

            ready.swap(_state->ready);
            timed.swap(_state->timed);
        }
    }

    /**
     * @brief Checks whether the caller runs on this worker's thread
     */
    [[nodiscard]] bool is_current() const noexcept
    {
        return _state->thread_id == std::this_thread::get_id();
    }

private:
    struct state
    {
        mutable std::mutex lock {};
        std::condition_variable_any wake {};
        std::deque<task_type> ready {};
        std::multimap<std::chrono::steady_clock::time_point, task_type> timed {};
        uint64_t generation { 0 };
        std::atomic<std::thread::id> thread_id {};
    };

    std::shared_ptr<state> _state;
    std::jthread _thread;

    static void run(state &s, std::stop_token stop)
    {
        s.thread_id = std::this_thread::get_id();

        std::unique_lock lock { s.lock };

        for (;;)
        {
            const auto now { std::chrono::steady_clock::now() };

            while (!std::empty(s.timed) && std::begin(s.timed)->first <= now)
            {
                s.ready.push_back(std::move(std::begin(s.timed)->second));
                s.timed.erase(std::begin(s.timed));
            }

            if (!std::empty(s.ready))
            {
                {
                    auto task { std::move(s.ready.front()) };

                    s.ready.pop_front();
                    lock.unlock();
                    task();
                }

                lock.lock();

                continue;
            }

            if (stop.stop_requested()) break;

            const auto seen { s.generation };

            if (std::empty(s.timed)) s.wake.wait(lock, stop, [&s, seen] { return s.generation != seen; });
            else s.wake.wait_until(lock, stop, std::begin(s.timed)->first, [&s, seen] { return s.generation != seen; });
        }

        std::multimap<std::chrono::steady_clock::time_point, task_type> dropped;

        dropped.swap(s.timed);
        lock.unlock();
    }
};

/**
 * @brief An executor that queues tasks until you run them
 *
 * Use it to deliver slot calls inside your own loop (a game loop, a UI idle
 * handler): call run_all() or run_one() there. An optional notifier is called
 * after each post, so the loop can be woken up.
 */
class manual_executor final : public executor
{
public:
    void post(task_type task) override
    {
        std::function<void()> notifier;

        {
            std::scoped_lock lock { _lock };

            _ready.push_back(std::move(task));
            notifier = _notifier;
        }

        if (notifier) notifier();
    }

    void post_after(std::chrono::milliseconds delay, task_type task) override
    {
        std::function<void()> notifier;

        {
            std::scoped_lock lock { _lock };

            _timed.emplace(std::chrono::steady_clock::now() + delay, std::move(task));
            notifier = _notifier;
        }

        if (notifier) notifier();
    }

    /**
     * @brief Runs the oldest task that is due
     * @return true if a task ran
     */
    bool run_one()
    {
        task_type task;

        {
            std::scoped_lock lock { _lock };

            promote_due_locked();

            if (std::empty(_ready)) return false;

            task = std::move(_ready.front());
            _ready.pop_front();
        }

        task();

        return true;
    }

    /**
     * @brief Runs the tasks that are due now; tasks they post wait for the next call
     * @return The number of tasks that ran
     */
    std::size_t run_all()
    {
        std::deque<task_type> batch;

        {
            std::scoped_lock lock { _lock };

            promote_due_locked();
            batch.swap(_ready);
        }

        for (auto &task : batch) task();

        return std::size(batch);
    }

    /**
     * @brief Gets when the next task is due (now if one is ready)
     * @return The due time, or nothing if no task is waiting
     */
    [[nodiscard]] std::optional<std::chrono::steady_clock::time_point> next_due() const
    {
        std::scoped_lock lock { _lock };

        if (!std::empty(_ready)) return std::chrono::steady_clock::now();
        if (!std::empty(_timed)) return std::begin(_timed)->first;

        return std::nullopt;
    }

    /**
     * @brief Gets the number of tasks waiting, due or not
     */
    [[nodiscard]] std::size_t pending() const
    {
        std::scoped_lock lock { _lock };

        return std::size(_ready) + std::size(_timed);
    }

    /**
     * @brief Drops the tasks waiting to run
     */
    void clear()
    {
        std::deque<task_type> ready;
        std::multimap<std::chrono::steady_clock::time_point, task_type> timed;

        {
            std::scoped_lock lock { _lock };

            ready.swap(_ready);
            timed.swap(_timed);
        }
    }

    /**
     * @brief Sets a function called after each post (outside the executor's lock)
     */
    void set_notifier(std::function<void()> notifier)
    {
        std::scoped_lock lock { _lock };

        _notifier = std::move(notifier);
    }

private:
    mutable std::mutex _lock {};
    std::deque<task_type> _ready {};
    std::multimap<std::chrono::steady_clock::time_point, task_type> _timed {};
    std::function<void()> _notifier {};

    void promote_due_locked()
    {
        const auto now { std::chrono::steady_clock::now() };

        while (!std::empty(_timed) && std::begin(_timed)->first <= now)
        {
            _ready.push_back(std::move(std::begin(_timed)->second));
            _timed.erase(std::begin(_timed));
        }
    }
};

/**
 * @brief Emits tick() at a fixed interval on its own thread
 *
 * Connect slots with an executor to run them elsewhere. start() and stop() may be
 * called from one owner thread, and from inside a tick slot; the timer may also be
 * destroyed from inside a tick slot.
 */
class timer
{
public:
    timer() : timer { 1s } {}

    /**
     * @brief Constructor with the interval
     * @param interval Time between ticks
     */
    template<typename Rep, typename Period>
    explicit timer(const std::chrono::duration<Rep, Period> &interval) : _state { std::make_shared<state>() }
    {
        _state->interval = std::chrono::ceil<std::chrono::milliseconds>(interval);
    }

    timer(const timer &) = delete;
    timer &operator=(const timer &) = delete;

    ~timer()
    {
        stop();

        if (_thread.joinable())
        {
            if (_thread.get_id() == std::this_thread::get_id()) _thread.detach();
            else _thread.join();
        }
    }

    /**
     * @brief The signal emitted on every tick, on the timer's thread
     */
    [[nodiscard]] signal<> &tick() noexcept
    {
        return _state->tick;
    }

    /**
     * @brief Starts ticking; the first tick comes one interval from now
     */
    void start()
    {
        std::unique_lock lock { _state->lock };

        if (_state->running) return;

        _state->running = true;

        if (_state->thread_id == std::this_thread::get_id()) return;    // restarted from a tick slot: the current thread keeps going

        const auto generation { ++_state->generation };

        lock.unlock();

        if (_thread.joinable()) _thread.join();

        _thread = std::thread { [s = _state, generation] { run(s, generation); } };
    }

    /**
     * @brief Stops ticking; waits for a running tick unless called from a tick slot
     */
    void stop()
    {
        bool on_timer_thread { false };

        {
            std::scoped_lock lock { _state->lock };

            _state->running = false;
            on_timer_thread = _state->thread_id == std::this_thread::get_id();
        }

        _state->wake.notify_all();

        if (!on_timer_thread && _thread.joinable()) _thread.join();
    }

    /**
     * @brief Checks whether the timer is ticking
     */
    [[nodiscard]] bool is_running() const
    {
        std::scoped_lock lock { _state->lock };

        return _state->running;
    }

    /**
     * @brief Sets the interval; it applies from the next tick on
     */
    template<typename Rep, typename Period>
    void interval(const std::chrono::duration<Rep, Period> &interval)
    {
        std::scoped_lock lock { _state->lock };

        _state->interval = std::chrono::ceil<std::chrono::milliseconds>(interval);
    }

    /**
     * @brief Gets the interval
     */
    [[nodiscard]] std::chrono::milliseconds interval() const
    {
        std::scoped_lock lock { _state->lock };

        return _state->interval;
    }

private:
    struct state
    {
        mutable std::mutex lock {};
        std::condition_variable wake {};
        signal<> tick {};
        std::chrono::milliseconds interval { 1s };
        bool running { false };
        uint64_t generation { 0 };
        std::thread::id thread_id {};
    };

    std::shared_ptr<state> _state;
    std::thread _thread {};

    static void run(std::shared_ptr<state> s, uint64_t generation)
    {
        std::unique_lock lock { s->lock };

        s->thread_id = std::this_thread::get_id();

        auto next { std::chrono::steady_clock::now() + s->interval };
        const auto stopped { [&s, generation] { return !s->running || s->generation != generation; } };

        for (;;)
        {
            if (s->wake.wait_until(lock, next, stopped)) break;

            lock.unlock();
            s->tick.emit();
            lock.lock();

            if (stopped()) break;

            next += s->interval;

            if (const auto now { std::chrono::steady_clock::now() }; next <= now) next = now + s->interval;    // skip ticks a slow slot missed
        }

        if (s->generation == generation) s->thread_id = {};
    }
};

/**
 * @brief A collection of signals indexed by a key, created on first use
 *
 * Not synchronized: create the signals before using the set from several threads.
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

    signal_set_base() = default;
    virtual ~signal_set_base() = default;

    /**
     * @brief Emits all signals with the given arguments
     */
    void emit(const Args &... args) const
    {
        for (auto &&s : _signals) s.second->emit(args...);
    }

    /**
     * @brief Same as emit()
     */
    void operator() (const Args &... args) const
    {
        emit(args...);
    }

    /**
     * @brief Gets or creates the signal for a key; a u8string key also names the signal
     */
    [[nodiscard]] virtual signal_type &get_signal(const key_type &key)
    {
        auto signal { _signals.find(key) };

        if (signal != _signals.end()) return *signal->second;

        if constexpr (std::is_same_v<std::decay_t<key_type>, std::u8string>) return *_signals.emplace(key, std::make_unique<signal_type>(key)).first->second;
        else return *_signals.emplace(key, std::make_unique<signal_type>()).first->second;
    }

    /**
     * @brief Checks if a signal exists for the key
     */
    [[nodiscard]] bool exists(const key_type &key) const
    {
        return _signals.find(key) != _signals.end();
    }

    /**
     * @brief Gets all signal keys
     */
    [[nodiscard]] std::unordered_set<key_type> get_signal_keys() const
    {
        std::unordered_set<key_type> signal_keys;

        for (const auto &[key, _] : _signals) signal_keys.insert(key);

        return signal_keys;
    }

    /**
     * @brief Gets the number of signals
     */
    [[nodiscard]] auto get_signal_count() const
    {
        return std::size(_signals);
    }

    /**
     * @brief Same as get_signal()
     */
    [[nodiscard]] signal_type &operator[](const key_type &key)
    {
        return get_signal(key);
    }

    [[nodiscard]] auto begin() const { return std::begin(_signals); }
    [[nodiscard]] auto end() const { return std::end(_signals); }

protected:
    std::unordered_map<key_type, std::unique_ptr<signal_type>> _signals {};
};

/**
 * @brief A set of signals indexed by a key
 */
template<typename Key, typename... Args> using signal_set = signal_set_base<Key, signal, Args...>;

/**
 * @brief A set of signals that pass themselves as the first argument
 */
template<typename Key, typename... Args> using signal_ex_set = signal_set_base<Key, signal_ex, Args...>;

}
