#pragma once

/*
MIT License
Copyright (c) 2019 Arlen Keshabyan (arlen.albert@gmail.com)
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

#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>

namespace nstd
{

class thread_pool_lite
{
public:
    thread_pool_lite(long num_threads = std::max(std::thread::hardware_concurrency(), 2u) - 1u)
    {
        if (num_threads < 1) num_threads = 1;

        while (--num_threads >= 0)
        {
            _worker_threads.emplace_back([this]
            {
                while (true)
                {
                    std::function<void()> new_task;

                    {
                        std::unique_lock<std::mutex> lock(_queue_mutex);
                        
                        _condition.wait(lock, [this]{ return _cancelled || !std::empty(_tasks); });
                        
                        if(_cancelled && std::empty(_tasks)) return;

                        new_task = std::move(_tasks.front());
                        
                        _tasks.pop();
                    }

                    new_task();
                }
            });
        }
    }

    template<class Functor, class... Args>
    auto enqueue(Functor&& functor, Args&&... args)
    {
        using return_type = std::result_of_t<Functor(Args...)>;

        auto new_task { std::make_shared<std::packaged_task<return_type()>>(std::bind(std::forward<Functor>(functor), std::forward<Args>(args)...)) };
            
        std::future<return_type> result = new_task->get_future();

        {
            std::scoped_lock lock { _queue_mutex };

            if(_cancelled) throw std::runtime_error("Enqueueing on cancelled thread_pool is not allowed!");

            _tasks.emplace([new_task = std::move(new_task)](){ (*new_task)(); });
        }

        _condition.notify_one();

        return result;
    }

    auto size() const { return std::size(_worker_threads); }

    ~thread_pool_lite()
    {
        {
            std::scoped_lock lock { _queue_mutex };
            _cancelled = true;
        }

        _condition.notify_all();

        for(auto &worker: _worker_threads) worker.join();
    }

private:
    std::deque<std::thread> _worker_threads;
    std::queue<std::function<void()>> _tasks;
    
    std::mutex _queue_mutex {};
    std::condition_variable _condition {};
    bool _cancelled { false };
};

template <typename T>
class thread_safe_queue
{
public:
    ~thread_safe_queue()
    {
        invalidate();
    }

    bool try_pop(T& out)
    {
        std::scoped_lock lock { _mutex };

        if(_queue.empty() || !_valid) return false;

        out = std::move(_queue.front());

        _queue.pop();

        return true;
    }

    bool wait_pop(T& out)
    {
        std::unique_lock<std::mutex> lock{_mutex};

        _condition.wait(lock, [this]() { return !_queue.empty() || !_valid; });

        if(!_valid) return false;

        out = std::move(_queue.front());

        _queue.pop();

        return true;
    }

    void push(T value)
    {
        std::scoped_lock lock { _mutex };
        
        _queue.push(std::move(value));
        _condition.notify_one();
    }

    bool empty() const
    {
        std::scoped_lock lock { _mutex };

        return _queue.empty();
    }

    void clear()
    {
        std::scoped_lock lock { _mutex };

        while(!_queue.empty()) _queue.pop();

        _condition.notify_all();
    }

    void invalidate()
    {
        std::scoped_lock lock { _mutex };

        _valid = false;

        _condition.notify_all();
    }

    bool is_valid() const
    {
        std::scoped_lock lock{ _mutex };

        return _valid;
    }

private:
    std::atomic_bool _valid { true };
    mutable std::mutex _mutex;
    std::queue<T> _queue;
    std::condition_variable _condition;
};

class thread_pool
{
private:
    class thread_task_base
    {
    public:
        thread_task_base() = default;
        virtual ~thread_task_base() = default;
        thread_task_base(const thread_task_base& rhs) = delete;
        thread_task_base& operator=(const thread_task_base& rhs) = delete;
        thread_task_base(thread_task_base&& other) = default;
        thread_task_base& operator=(thread_task_base&& other) = default;
        virtual void execute() = 0;
    };

    template <typename Functor>
    class thread_task: public thread_task_base
    {
    public:
        thread_task(Functor&& functor) :_functor { std::move(functor) } { }
        ~thread_task() override = default;
        thread_task(const thread_task& rhs) = delete;
        thread_task& operator=(const thread_task& rhs) = delete;
        thread_task(thread_task&& other) = default;
        thread_task& operator=(thread_task&& other) = default;
        void execute() override { _functor(); }

    private:
        Functor _functor;
    };

public:
    template <typename T>
    class task_future
    {
    public:
        task_future(std::future<T>&& future) : _future{ std::move(future) } { }
        task_future(const task_future& rhs) = delete;
        task_future& operator=(const task_future& rhs) = delete;
        task_future(task_future&& other) = default;
        task_future& operator=(task_future&& other) = default;
        ~task_future() { if(_future.valid()) _future.get(); }
        auto get() { return _future.get(); }
        void wait() { _future.wait(); }

    private:
        std::future<T> _future;
    };

public:
    explicit thread_pool(long num_threads = std::max(std::thread::hardware_concurrency(), 2u) - 1u) : _completed { false }, _task_queue{}, _worker_threads{}
    {
        if (num_threads < 1) num_threads = 1;

        try
        {
            while (--num_threads >= 0) _worker_threads.emplace_back(&thread_pool::worker, this);
        }
        catch(...)
        {
            destroy();
            throw;
        }
    }

    thread_pool(const thread_pool& rhs) = delete;
    thread_pool& operator=(const thread_pool& rhs) = delete;
    ~thread_pool() { destroy(); }

    template <typename Functor, typename... Args>
    auto enqueue(Functor&& functor, Args&&... args)
    {
        auto bound_task = std::bind(std::forward<Functor>(functor), std::forward<Args>(args)...);
        using result_type = std::result_of_t<decltype(bound_task)()>;
        using packaged_task = std::packaged_task<result_type()>;
        using task_type = thread_task<packaged_task>;
        
        packaged_task task{ std::move(bound_task) };
        task_future<result_type> result { task.get_future() };

        _task_queue.push(std::make_unique<task_type>(std::move(task)));

        return result;
    }

    auto size() const { return std::size(_worker_threads); }

private:
    void worker()
    {
        while(!_completed)
        {
            std::unique_ptr<thread_task_base> task { nullptr };

            if(_task_queue.wait_pop(task)) task->execute();
        }
    }

    void destroy()
    {
        _completed = true;

        _task_queue.invalidate();

        for(auto& thread : _worker_threads)
        {
            if(thread.joinable())
            {
                thread.join();
            }
        }
    }

private:
    std::atomic_bool _completed;
    thread_safe_queue<std::unique_ptr<thread_task_base>> _task_queue;
    std::deque<std::thread> _worker_threads;
};

namespace global_thread_pool
{
    inline thread_pool& get_thread_pool()
    {
        static thread_pool static_thread_pool;

        return static_thread_pool;
    }

    template <typename Functor, typename... Args>
    inline auto enqueue(Functor&& functor, Args&&... args)
    {
        return get_thread_pool().enqueue(std::forward<Functor>(functor), std::forward<Args>(args)...);
    }
}

}
