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

#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>
#include <list>
#include <algorithm>
#include <exception>

namespace BsThread
{

typedef enum
{
      DONE = 0
    , WORKING
    , WAITING
    , QUEUED
    , LOST
    , FAILED
} State;

typedef State Routine(void*, unsigned int entryCnt);
typedef unsigned int SyncPoint;

struct Task
{
    Routine* Execute;
    void* param;
    unsigned int id;
    int priority;
    unsigned int n;
    std::list<Task*> dependent;
    unsigned int blocked;
    bool detach;
    State state;
    std::mutex mtx;
    std::condition_variable cv;
};

struct Thread
{
    std::mutex mtx;
    std::condition_variable cv;
    std::thread thread;
    Task* task;
    bool terminate;
    unsigned int id;
};

inline bool Ready(State s)
{
    return (s == DONE || s == FAILED || s == LOST);
}

// sync required
class TaskQueueOverflow : public std::exception
{
public:
    TaskQueueOverflow() {/*printf("TaskQueueOverflow\n"); fflush(stdout);*/}
    ~TaskQueueOverflow() {}
};

class Scheduler
{
private:
    std::list<Thread>       m_thread;
    std::list<Task>         m_task;
    std::recursive_mutex    m_mtx;
    std::condition_variable_any m_cv;
    unsigned int            m_id;
    size_t                  m_depth;
    unsigned int            m_locked;

    static void Execute (Thread& self, Scheduler& sync);
    static void Update  (Scheduler& self, Thread* thread);

    void _AbortDependent(Task& task);

public:
    Scheduler();
    ~Scheduler();

    void Init(unsigned int nThreads, unsigned int depth = 128);
    void Close();

    SyncPoint   Submit  (Routine* routine, void* par, int priority, unsigned int nDep, SyncPoint *dep);
    State       Sync    (SyncPoint sp, unsigned int waitMS, bool keepStat = false);
    bool        Abort   (SyncPoint sp, unsigned int waitMS);

    State   AddDependency   (SyncPoint task, unsigned int nDep, SyncPoint *dep);
    void    Detach          (SyncPoint task); // no sync for this task
    bool    WaitForAny      (unsigned int waitMS);
};

};