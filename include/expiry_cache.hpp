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
#include <memory>
#include <thread>
#include <tuple>
#include <unordered_map>
#include "signal_slot.hpp"

namespace nstd
{
using namespace std::chrono_literals;

template<typename key_type, typename value_type>
class expiry_cache
{
public:
    using self_type = expiry_cache<key_type, value_type>;
    using time_point_type = std::chrono::time_point<std::chrono::high_resolution_clock>;
    using data_type = std::tuple<time_point_type, std::chrono::milliseconds, value_type>;

    expiry_cache() = default;
    ~expiry_cache() { stop_auto_vacuum(); clear(); }

    template<typename duration_type>
    expiry_cache(const duration_type &duration)
    {
        set_expiry(duration);
    }

    void put(const key_type &key, const value_type &value, const time_point_type &time_point)
    {
        put(key, value, std::chrono::duration_cast<std::chrono::milliseconds>(time_point - std::chrono::high_resolution_clock::now()));
    }

    template<typename duration_type = std::chrono::milliseconds>
    void put(const key_type &key, const value_type &value, const duration_type &duration = 0ms)
    {
        std::scoped_lock lock {_mutex};

        auto it{ _data.find(key) };

        if (it != std::end(_data)) _erase(it);

        auto expiry_duration_ms { std::chrono::duration_cast<std::chrono::milliseconds>(duration) };

        _data.emplace(key, std::make_tuple(std::chrono::high_resolution_clock::now(), (expiry_duration_ms == 0ms) ? _expiry_duration_ms.load(std::memory_order_relaxed) : expiry_duration_ms, value));
    }

    bool exists(const key_type &key)
    {
        std::scoped_lock lock {_mutex};

        return _data.find(key) != std::end(_data);
    }

    void touch(const key_type &key, bool force_touch = true)
    {
        std::scoped_lock lock {_mutex};

        auto it{ _data.find(key) };

        if (it != std::end(_data) && (_access_prolongs.load(std::memory_order_relaxed) || force_touch)) std::get<0>(it->second) = std::chrono::high_resolution_clock::now();
    }

    bool get(const key_type &key, value_type &value)
    {
        std::scoped_lock lock {_mutex};

        auto it{ _data.find(key) };

        if (it != std::end(_data))
        {
            auto &val = it->second;
            auto now{ std::chrono::high_resolution_clock::now() };

            if (!_auto_vacuum && std::chrono::duration_cast<std::chrono::milliseconds>(now - std::get<0>(val)) > std::get<1>(val))
            {
                _erase(it);

                return false;
            }

            value = std::get<2>(val);

            if (_access_prolongs.load(std::memory_order_relaxed)) std::get<0>(val) = now;

            return true;
        }

        return false;
    }

    void set_access_prolongs(bool prolongs = true)
    {
        _access_prolongs.store(prolongs, std::memory_order_relaxed);
    }

    bool is_access_prolongs() const
    {
        return _access_prolongs;
    }

    template<typename duration_type>
    void set_expiry(const duration_type &duration)
    {
        _expiry_duration_ms.store(std::chrono::duration_cast<std::chrono::milliseconds>(duration), std::memory_order_relaxed);
    }

    void set_expiry(const key_type &key, const time_point_type &time_point)
    {
        std::scoped_lock lock {_mutex};

        auto it{ _data.find(key) };

        if (it != std::end(_data)) std::get<1>(it->second) = std::chrono::duration_cast<std::chrono::milliseconds>(time_point - std::chrono::high_resolution_clock::now());
    }

    template<typename duration_type>
    void set_expiry(const key_type &key, const duration_type &duration)
    {
        std::scoped_lock lock {_mutex};

        auto it{ _data.find(key) };

        if (it != std::end(_data)) std::get<1>(it->second) = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
    }

    const std::chrono::milliseconds get_expiry() const
    {
        return _expiry_duration_ms.load(std::memory_order_relaxed);
    }

    const std::chrono::milliseconds get_expiry(const key_type &key) const
    {
        std::scoped_lock lock {_mutex};

        auto it{ _data.find(key) };

        if (it != std::end(_data)) return std::get<1>(it->second);

        return _expiry_duration_ms.load(std::memory_order_relaxed);
    }

    const time_point_type get_expiry_time_point(const key_type &key) const
    {
        std::scoped_lock lock {_mutex};

        auto it{ _data.find(key) };

        if (it != std::end(_data)) return std::get<0>(it->second) + std::get<1>(it->second);

        return std::chrono::high_resolution_clock::now();
    }

    void clear()
    {
        std::scoped_lock lock {_mutex};

        _vacuum();

        for (auto &data : _data) signal_data_expired.emit(data.first, std::get<2>(data.second));

        _data.clear();
    }

    size_t size() const
    {
        std::scoped_lock lock {_mutex};

        return _data.size();
    }

    void vacuum()
    {
        std::scoped_lock lock {_mutex};
        
        _vacuum();
    }

    template<typename duration_type>
    void set_vacuum_idle_period(const duration_type &duration)
    {
        _vacuum_idle_period_ms.store(std::chrono::duration_cast<std::chrono::milliseconds>(duration), std::memory_order_relaxed);
    }

    std::chrono::milliseconds get_vacuum_idle_period() const
    {
        return _vacuum_idle_period_ms;
    }

    void start_auto_vacuum()
    {
        if (_auto_vacuum.load(std::memory_order_relaxed)) return;

        std::scoped_lock lock {_mutex};

        if (_auto_vacuum.load(std::memory_order_relaxed)) return;

        _auto_vacuum_thread = std::thread([this]() { _auto_vacuum_procedure(); });
        _auto_vacuum.store(true, std::memory_order_relaxed);
    }

    void stop_auto_vacuum()
    {
        if (!_auto_vacuum.load(std::memory_order_relaxed)) return;

        std::scoped_lock lock {_mutex};

        if (!_auto_vacuum.load(std::memory_order_relaxed)) return;

        _cancel_auto_vacuum.store(true, std::memory_order_relaxed);

        if (_auto_vacuum_thread.joinable()) _auto_vacuum_thread.join();

        _auto_vacuum.store(false, std::memory_order_relaxed);
        _cancel_auto_vacuum.store(false, std::memory_order_relaxed);
    }

    nstd::signal_slot::signal<const key_type &, value_type &> signal_data_expired {};

private:
    std::unordered_map<key_type, data_type> _data {};

    void _auto_vacuum_procedure()
    {
        auto time{ std::chrono::high_resolution_clock::now() };

        while (!_cancel_auto_vacuum.load(std::memory_order_relaxed))
        {
            auto now{ std::chrono::high_resolution_clock::now() };

            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - time) > _vacuum_idle_period_ms.load(std::memory_order_relaxed))
            {
                time = now;

                vacuum();
            }

            if (_cancel_auto_vacuum.load(std::memory_order_relaxed)) return;

            std::this_thread::sleep_for(100ms);
        }
    }

    void _erase(decltype(std::begin(_data)) &it)
    {
        auto &val = it->second;

        signal_data_expired.emit(it->first, std::get<2>(val));

        _data.erase(it);
    }

    void _vacuum()
    {
        auto now{ std::chrono::high_resolution_clock::now() };
        std::vector<decltype(std::begin(_data))> expired;

        for (auto it{ std::begin(_data) }, end{std::end(_data)}; it != end; ++it)
        {
            const auto &val = it->second;

            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - std::get<0>(val)) > std::get<1>(val))
                expired.push_back(it);
        }

        for (auto &it : expired) _erase(it);
    }

    std::atomic<std::chrono::milliseconds> _expiry_duration_ms { 10min };
    std::atomic<std::chrono::milliseconds> _vacuum_idle_period_ms { 1min };
    std::atomic_bool _access_prolongs { false };
    std::atomic_bool _auto_vacuum { false };
    std::atomic_bool _cancel_auto_vacuum { false };
    std::thread _auto_vacuum_thread {};
    mutable std::mutex _mutex {};
};
}
