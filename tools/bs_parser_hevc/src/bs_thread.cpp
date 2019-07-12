// Copyright (c) 2018-2019 Intel Corporation
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "bs_thread.h"

//#define BS_THREAD_TRACE

#ifdef BS_THREAD_TRACE
#include <stdio.h>
#endif

inline void __notrace(const char*, ...) {}

const char State2CS[6][8] = { "DONE", "WORKING", "WAITING", "QUEUED", "LOST", "FAILED" };

#ifdef BS_THREAD_TRACE
#define BS_THREAD_TRACE_F printf
#define BS_THREAD_TRACE_FLUSH fflush(stdout);
struct __UIA2CS
{
    static char c_str[256];
    __UIA2CS(unsigned int n, unsigned int *s)
    {
        char* p = c_str;
        p++[0] = '{';
        while (n--)
            p += sprintf_s(p, 256, " %d", s++[0]);
        p++[0] = ' ';
        p++[0] = '}';
        p++[0] = 0;
    }
};
char __UIA2CS::c_str[256];
#else
struct __UIA2CS
{
    char* c_str = nullptr;
    __UIA2CS(unsigned int n, unsigned int *s)
    {
        (void)n;
        (void)s;
    }
};
#define BS_THREAD_TRACE_F __notrace
#define BS_THREAD_TRACE_FLUSH
#endif

using namespace BsThread;

Scheduler::Scheduler()
{
    m_locked = 0;
    m_id = 0;
    m_depth = 0;
}

Scheduler::~Scheduler()
{
    Close();
}

void Scheduler::Init(unsigned int nThreads, unsigned int depth)
{
    std::unique_lock<std::recursive_mutex> lock(m_mtx);

    BS_THREAD_TRACE_F("Scheduler::Init(%d, %d); //T=%d, D=%d, L=%d\n",
        nThreads, depth, m_thread.size() + nThreads, m_depth + depth, m_locked + 1);
    BS_THREAD_TRACE_FLUSH;

    if (nThreads > m_thread.size())
    {
        unsigned int id = (unsigned int)m_thread.size();

        m_thread.resize(nThreads);

        auto t = m_thread.begin();
        std::advance(t, id);

        for (; t != m_thread.end(); t++)
        {
            t->id = id++;
            t->task = 0;
            t->terminate = false;
            t->thread = std::thread(Execute, std::ref(*t), std::ref(*this));
        }
    }

    m_depth += depth;
    m_locked++;
}

void Scheduler::Close()
{
    std::unique_lock<std::recursive_mutex> lock(m_mtx);
    BS_THREAD_TRACE_F("Scheduler::Close(); //T=%d, D=%d, L=%d\n",
        m_thread.size(), m_depth, m_locked ? m_locked - 1 : 0);
    BS_THREAD_TRACE_FLUSH;

    if (!m_locked || !--m_locked)
    {
        for (auto& t : m_thread)
        {
            std::unique_lock<std::mutex> lockTask(t.mtx);

            while (t.task && t.task->state == WORKING)
                t.cv.wait(lockTask);

            t.terminate = true;
            t.cv.notify_one();
        }

        for (auto& t : m_thread)
            t.thread.join();

        m_thread.resize(0);
        m_task.resize(0);

        m_id = 0;
        m_depth = 0;
    }
}

unsigned int Scheduler::Submit(Routine* routine, void* par, int priority, unsigned int nDep, unsigned int *dep)
{
    std::unique_lock<std::recursive_mutex> lock(m_mtx);
    BS_THREAD_TRACE_F("Scheduler::Submit(P=%d, D=%s) ", priority, __UIA2CS(nDep, dep).c_str);
    BS_THREAD_TRACE_FLUSH;

    while (m_task.size() >= m_depth)
    {
        bool flag = false;

        m_task.remove_if(
            [&] (Task& t) -> bool
            {
                if (t.state != DONE || !t.dependent.empty())
                    return false;
                flag = true;
                return true;
            }
        );

        if (!flag)
        {
            BS_THREAD_TRACE_F(" -- TaskQueueOverflow\n");
            BS_THREAD_TRACE_FLUSH;
            throw TaskQueueOverflow();
        }
        else
        {
            BS_THREAD_TRACE_F(": TaskDequeued ");
            BS_THREAD_TRACE_FLUSH;
        }
    }

    m_task.resize(m_task.size() + 1);
    Task& task = m_task.back();

    task.id = m_id++;
    task.Execute = routine;
    task.param = par;
    task.priority = priority;
    task.blocked = 0;
    task.n = 0;
    task.detach = false;

    for (unsigned int i = 0; i < nDep; i++)
    {
        auto it = std::find_if(m_task.begin(), m_task.end(),
            [&] (Task& t) -> bool { return t.id == dep[i]; });

        if (it != m_task.end())
        {
            if (it->state == FAILED || it->state == LOST)
            {
                BS_THREAD_TRACE_F(": ID=%d BL=0 -- LOST on %d\n", task.id, it->id);
                BS_THREAD_TRACE_FLUSH;

                task.blocked = 0;
                task.state = LOST;
                return task.id;
            }

            if (it->state != DONE)
            {
                it->dependent.push_back(&task);
                task.blocked++;
            }
        }
    }

    task.state = task.blocked ? WAITING : QUEUED;

    BS_THREAD_TRACE_F(": ID=%d BL=%d -- QUEUED\n", task.id, task.blocked);
    BS_THREAD_TRACE_FLUSH;

    Update(*this, 0);

    return task.id;
}

State Scheduler::Sync(unsigned int id, unsigned int waitMS, bool keepStat)
{
    std::unique_lock<std::recursive_mutex> lock(m_mtx);
    State st = LOST;
    BS_THREAD_TRACE_F("Scheduler::Sync(ID=%d, Wait=%d, Keep=%d) ", id, waitMS, keepStat);
    BS_THREAD_TRACE_FLUSH;

    auto it = std::find_if(m_task.begin(), m_task.end(),
            [&] (Task& t) -> bool { return t.id == id; });

    if (it == m_task.end() || it->detach)
    {
        BS_THREAD_TRACE_F("-- LOST(DEQUEUED)\n");
        BS_THREAD_TRACE_FLUSH;
        return LOST;
    }
    Task* pTask = &*it;
    st = pTask->state;

    if (waitMS)
    {
        BS_THREAD_TRACE_F(": wait\n");
        BS_THREAD_TRACE_FLUSH;

        lock.unlock();

        {
            std::unique_lock<std::mutex> lockTask(pTask->mtx);

            pTask->cv.wait_for(lockTask, std::chrono::milliseconds(waitMS),
                [&]() -> bool { return Ready(pTask->state); } );

            st = pTask->state;
        }

        lock.lock();

        BS_THREAD_TRACE_F("Scheduler::Sync(ID=%d, Wait=%d, Keep=%d) : return ", id, waitMS, keepStat);
        BS_THREAD_TRACE_FLUSH;
    }

    if (Ready(st) && !keepStat)
    {
        m_task.remove_if([&] (Task& t) -> bool { return t.id == id; });
    }

    BS_THREAD_TRACE_F("-- %s\n", State2CS[st]);
    BS_THREAD_TRACE_FLUSH;

    return st;
}

void Scheduler::Detach(SyncPoint id)
{
    std::unique_lock<std::recursive_mutex> lock(m_mtx);
    BS_THREAD_TRACE_F("Scheduler::Detach(ID=%d) ", id);
    BS_THREAD_TRACE_FLUSH;

    auto itT = std::find_if(m_task.begin(), m_task.end(),
            [&] (Task& t) -> bool { return t.id == id; });

    if (itT == m_task.end())
    {
        BS_THREAD_TRACE_F("-- LOST(DEQUEUED)\n");
        BS_THREAD_TRACE_FLUSH;
        return;
    }

    BS_THREAD_TRACE_F(" -- %s\n", State2CS[itT->state]);

    itT->detach = true;

    if (Ready(itT->state))
    {
        m_task.remove_if([&] (Task& t) -> bool { return t.id == id; });
    }
}

bool Scheduler::WaitForAny(unsigned int waitMS)
{
    std::unique_lock<std::recursive_mutex> lock(m_mtx);

    return (std::cv_status::timeout == m_cv.wait_for(lock, std::chrono::milliseconds(waitMS)));
}

State Scheduler::AddDependency(SyncPoint id, unsigned int nDep, SyncPoint *dep)
{
    std::unique_lock<std::recursive_mutex> lock(m_mtx);
    BS_THREAD_TRACE_F("Scheduler::AddDependency(ID=%d, D=%s) ", id, __UIA2CS(nDep, dep).c_str);
    BS_THREAD_TRACE_FLUSH;

    auto itT = std::find_if(m_task.begin(), m_task.end(),
            [&] (Task& t) -> bool { return t.id == id; });

    if (itT == m_task.end())
    {
        BS_THREAD_TRACE_F("-- LOST(DEQUEUED)\n");
        BS_THREAD_TRACE_FLUSH;
        return LOST;
    }
    Task& task = *itT;

    if (Ready(task.state) || task.state == WORKING)
    {
        BS_THREAD_TRACE_F("-- %s\n", State2CS[task.state]);
        BS_THREAD_TRACE_FLUSH;
        return task.state;
    }

    for (unsigned int i = 0; i < nDep; i++)
    {
        auto it = std::find_if(m_task.begin(), m_task.end(),
            [&] (Task& t) -> bool { return t.id == dep[i]; });

        if (it != m_task.end())
        {
            auto& base = *it;
            std::unique_lock<std::mutex> lockTask(task.mtx);

            if (base.state == FAILED || base.state == LOST)
            {
                BS_THREAD_TRACE_F(" -- LOST on %d\n", base.id);
                BS_THREAD_TRACE_FLUSH;

                task.blocked = 0;
                task.state = LOST;
                return LOST;
            }

            if (base.state != DONE)
            {
                //std::unique_lock<std::mutex> lockBase(task.mtx);
                base.dependent.push_back(&task);
                task.blocked++;
            }
        }
    }

    if (task.blocked)
    {
        std::unique_lock<std::mutex> lockTask(task.mtx);
        task.state = WAITING;
    }

    BS_THREAD_TRACE_F("-- %s\n", State2CS[task.state]);
    BS_THREAD_TRACE_FLUSH;

    return task.state;
}

bool Scheduler::Abort(SyncPoint id, unsigned int waitMS)
{
    std::unique_lock<std::recursive_mutex> lock(m_mtx);
    BS_THREAD_TRACE_F("Scheduler::Abort(ID=%d, Wait=%d) ", id, waitMS);
    BS_THREAD_TRACE_FLUSH;

    auto it = std::find_if(m_task.begin(), m_task.end(),
            [&] (Task& t) -> bool { return t.id == id; });

    if (it == m_task.end())
    {
        BS_THREAD_TRACE_F("-- LOST(DEQUEUED)\n");
        BS_THREAD_TRACE_FLUSH;
        return true;
    }

    Task* pTask = &*it;
    BS_THREAD_TRACE_F(": wait\n");
    BS_THREAD_TRACE_FLUSH;

    lock.unlock();

    {
        std::unique_lock<std::mutex> lockTask(pTask->mtx);

        pTask->cv.wait_for(lockTask, std::chrono::milliseconds(waitMS),
            [&]() -> bool { return pTask->state != WORKING; } );
    }

    lock.lock();

    if (pTask->state != WORKING)
    {
        BS_THREAD_TRACE_F("Scheduler::Abort(ID=%d, Wait=%d) : DONE\n", id, waitMS);
        BS_THREAD_TRACE_FLUSH;
        pTask->state = LOST;

        if (!pTask->dependent.empty())
            _AbortDependent(*pTask);

        if (pTask->blocked)
        {
            for (auto& t : m_task)
                t.dependent.remove(pTask);
        }

        m_task.remove_if([&] (Task& t) -> bool { return t.id == id; });

        return true;
    }

    BS_THREAD_TRACE_F("Scheduler::Abort(ID=%d, Wait=%d) : WORKING\n", id, waitMS);
    BS_THREAD_TRACE_FLUSH;

    return false;
}

void Scheduler::_AbortDependent(Task& task)
{
    BS_THREAD_TRACE_F("      Abort:");

    for (auto& dep : task.dependent)
    {
        std::unique_lock<std::mutex> lockDep(dep->mtx);
        dep->blocked = 0;
        dep->state = LOST;
        BS_THREAD_TRACE_F(" %d", dep->id);
        dep->cv.notify_one();
    }
    task.dependent.resize(0);

    BS_THREAD_TRACE_F("\n");
    BS_THREAD_TRACE_FLUSH;
}

void Scheduler::Execute(Thread& self, Scheduler& sync)
{
    std::unique_lock<std::mutex> lockThread(self.mtx);

    while (!self.terminate)
    {
        while (self.task)
        {
            auto st = self.task->Execute(self.task->param, self.task->n);

            {
                std::unique_lock<std::recursive_mutex> lock(sync.m_mtx);
                self.task->state = st;
                self.task->n++;
                Update(sync, &self);
            }

        }

        while (!self.task && !self.terminate)
            self.cv.wait(lockThread);
    }
}

void Scheduler::Update(Scheduler& self, Thread* thread)
{
    std::unique_lock<std::recursive_mutex> lock(self.m_mtx);
    auto FindThread = [&] () -> Thread*
    {
        auto it = std::find_if(self.m_thread.begin(), self.m_thread.end(),
            [] (Thread& t) -> bool { return !t.task; });

        if (it == self.m_thread.end())
            return 0;

        return &*it;
    };
    std::list<Task*> queued;
    std::list<SyncPoint> detached;

    BS_THREAD_TRACE_F("Scheduler::Update()\n");
    BS_THREAD_TRACE_FLUSH;

    if (thread && thread->task)
    {
        BS_THREAD_TRACE_F("    : TH=%d ID=%d N=%d -- %s\n",
            thread->id, thread->task->id, thread->task->n, State2CS[thread->task->state]);
        BS_THREAD_TRACE_FLUSH;

        if (thread->task->state == WORKING)
            thread->task->state = QUEUED;

        if (   thread->task->state == DONE
            || thread->task->state == FAILED)
        {
            std::unique_lock<std::mutex> lockTask(thread->task->mtx);

            if (thread->task->state == DONE)
            {
                BS_THREAD_TRACE_F("      Unlock:");

                for (auto& dep : thread->task->dependent)
                {
                    if (dep->blocked)
                    {
                        dep->blocked--;

                        if (!dep->blocked)
                            dep->state = QUEUED;

                        BS_THREAD_TRACE_F(" %d(%d)", dep->id, dep->blocked);
                    }
                }
                BS_THREAD_TRACE_F("\n");
                BS_THREAD_TRACE_FLUSH;
            }
            else if (!thread->task->dependent.empty())
                self._AbortDependent(*thread->task);

            thread->task->cv.notify_one();
        }

        if (Ready(thread->task->state))
        {
            for (auto& task : self.m_task)
            {
                if (   task.state == WAITING
                    && !task.blocked
                    && task.id != thread->task->id)
                {
                    task.state = QUEUED;
                }
                task.dependent.remove(thread->task);
            }

            if (thread->task->detach)
                detached.push_back(thread->task->id);

            self.m_cv.notify_one();
        }

        thread->task = 0;
    }

    for (auto& task : self.m_task)
    {
        if (task.state == LOST)
        {
            if (task.dependent.size())
                self._AbortDependent(task);
            if (task.detach)
                detached.push_back(task.id);
            else
            {
                std::unique_lock<std::mutex> lockTask(task.mtx);
                task.cv.notify_one();
            }
        }
        else if (task.state == QUEUED)
            queued.push_back(&task);

        BS_THREAD_TRACE_F("    : ID=%d BL=%d P=%d N=%d -- %s\n",
            task.id, task.blocked, task.priority, task.n, State2CS[task.state]);
        BS_THREAD_TRACE_FLUSH;
    }

    for (auto& id : detached)
    {
        self.m_task.remove_if([&id] (Task& t) -> bool { return t.id == id; });
    }

    queued.sort([](Task* l, Task* r) { return (l->priority > r->priority); });

    for (Task* pTask : queued)
    {
        Thread* pThread = FindThread();

        if (!pThread)
            break;

        std::unique_lock<std::mutex> lockThread(pThread->mtx, std::defer_lock);

        if (pThread != thread)
            lockThread.lock();

        pThread->task = pTask;
        pTask->state = WORKING;

        BS_THREAD_TRACE_F("    : TH=%d ID=%d P=%d N=%d -- EXECUTE\n",
            pThread->id, pTask->id, pTask->priority, pTask->n);
        BS_THREAD_TRACE_FLUSH;

        if (pThread != thread)
        {
            pThread->cv.notify_one();
            lockThread.unlock();
        }
    }
}
