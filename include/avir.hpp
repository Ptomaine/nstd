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

#include "external/avir/avir.h"
#ifdef AVIR_AVX
#include "external/avir/avir_float8_avx.h"
#elif defined (AVIR_SSE)
#include "external/avir/avir_float4_sse.h"
#endif
#include "external/avir/lancir.h"
#include "thread_pool.hpp"

namespace nstd
{
namespace avir = avir;

using thread_pool_base = thread_pool;
class avir_scale_thread_pool : public avir::CImageResizerThreadPool, public thread_pool_base
{
public:
    virtual int getSuggestedWorkloadCount() const override
    {
        return thread_pool_base::size();
    }

    virtual void addWorkload(CWorkload *const workload) override
    {
        _workloads.emplace_back(workload);
    }

    virtual void startAllWorkloads() override
    {
        for (auto &workload : _workloads) _tasks.emplace_back(thread_pool_base::enqueue([](auto workload){ workload->process(); }, workload));
    }

    virtual void waitAllWorkloadsToFinish() override
    {
        for (auto &task : _tasks) task.wait();
    }

    virtual void removeAllWorkloads()
    {
        _tasks.clear();
        _workloads.clear();
    }

private:
    std::deque<std::future<void>> _tasks;
    std::deque<CWorkload*> _workloads;
};

}