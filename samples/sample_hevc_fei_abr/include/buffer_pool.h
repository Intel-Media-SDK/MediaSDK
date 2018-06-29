/******************************************************************************\
Copyright (c) 2018, Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

This sample was distributed or derived from the Intel's Media Samples package.
The original version of this sample may be obtained from https://software.intel.com/en-us/intel-media-server-studio
or https://software.intel.com/en-us/media-client-solutions-support.
\**********************************************************************************/

#pragma once

#include <condition_variable>
#include "fei_buffer_allocator.h"

class FeiBufferAllocator;

template <class T, class D = std::default_delete<T>>
class BufferPool
{
private:
struct Return2Pool
{
    Return2Pool() :
    m_pool(nullptr) {}

    explicit
    Return2Pool(BufferPool<T, D> * pool) :
        m_pool(pool) {}

    void operator()(T* ptr)
    {
        m_pool->Add(std::unique_ptr<T, D>{ptr}); // return back to the pool
    }

    private:
        BufferPool<T, D> * m_pool;
};

public:
    using Type = std::unique_ptr<T, Return2Pool>;

    BufferPool() {}
    ~BufferPool()
    {
        std::for_each(m_pool.begin(), m_pool.end(), [this](std::unique_ptr<T, D> & item)
                                                    { m_allocator->Free(item.get()); }
                     );
    }

    // Weakness in design. Consider using a customer deleter for T
    // instead of explicit FeiBufferAllocator
    void SetDeleter(std::shared_ptr<FeiBufferAllocator> & allocator)
    {
        m_allocator = allocator;
    }

    void Add(std::unique_ptr<T, D> && buffer)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_pool.emplace_back(std::move(buffer));
        m_condition.notify_one();
    }

    Type GetBuffer()
    {
        std::unique_lock<std::mutex> lock(m_mutex);

        if (m_pool.empty())
        {
            m_condition.wait(lock, [this] { return !m_pool.empty(); });
        }

        auto tmp = m_pool.front().release();
        m_pool.pop_front();

        return Type(tmp, Return2Pool(this));
    }

    bool empty() const
    {
        return m_pool.empty();
    }

    size_t size() const
    {
        return m_pool.size();
    }

private:
    std::list<std::unique_ptr<T, D> > m_pool;
    std::mutex m_mutex;
    // Signals new free resource added to the pool.
    std::condition_variable m_condition;
    std::shared_ptr<FeiBufferAllocator> m_allocator;
};

using MVPPool = BufferPool<mfxExtFeiHevcEncMVPredictors>;
using CTUCtrlPool = BufferPool<mfxExtFeiHevcEncCtuCtrl>;
