// Copyright (c) 2020 Intel Corporation
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

#include <cmath>
#include <mfx_trace2_stat.h>

#ifdef MFX_TRACE_ENABLE_STAT

mfx::Stat::Stat(const char *filename)
    : filename(filename)
{
}

void mfx::Stat::handleEvent(const mfx::Trace::Event &e)
{
}

struct Stats {
    mfxU64 totalTimeMs;  // total time in microseconds
    mfxU64 totalSquaredTimeMs;  // total squared time in microseconds
    mfxU64 number;  // number of times function was called
    const char *category;  // function type/category

    double totalTime() const
    {
        return totalTimeMs / 1000000.;
    }

    double averageTime() const
    {
        return totalTime() / number;
    }

    double standardDeviation() const
    {
        if (number == 1) return 0.;
        double avgTime = averageTime();
        double stdDev = (totalSquaredTimeMs / (1000000. * 1000000.)) / number;
        stdDev -= avgTime * avgTime;
        stdDev *= number / (number - 1.);
        stdDev = sqrt(stdDev);
        return stdDev;
    }
};

mfx::Stat::~Stat()
{
    std::map <std::string, Stats> map;
    for (const mfx::Trace::Event &e : _mfx_trace.events)
    {
        if (e.type == mfx::Trace::EventType::END)
        {
            mfxU64 timeMs = e.timestamp - _mfx_trace.events[e.parentIndex].timestamp;
            map[e.sl.function_name()].totalTimeMs += timeMs;
            map[e.sl.function_name()].totalSquaredTimeMs += timeMs * timeMs;
            map[e.sl.function_name()].number += 1;
            map[e.sl.function_name()].category = e.category;
        }
    }
    file = fopen(filename, "w");
    fprintf(file, "%-40s: %-11s, %-7s, %-10s, %-9s, %-20s\n", "Function name", "Total time", "Number", "Avg. time", "Std. dev", "Category");
    for (auto info : map)
    {
        fprintf(file, "%-40s: %-11lf, %-7llu, %-10lf, %-9lf, %-20s\n",
            info.first.c_str(), info.second.totalTime(), info.second.number,
            info.second.averageTime(), info.second.standardDeviation(), info.second.category);
    }
    fclose(file);
}

#endif  // MFX_TRACE_ENABLE_STAT
