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

#if defined(MFX_TRACE_ENABLE_CHROME) && (defined(LINUX32))
#include <cstdio>
#include <syscall.h>
#include <unistd.h>
#include <algorithm>
#include <chrono>
#include <map>
#include <mutex>
#include <sstream>
#include <thread>

#include <mfx_trace.h>

static const mfxTraceLevel trace_level = MFX_TRACE_LEVEL_API;
static FILE *chrome_trace_file = nullptr;
static mfxU64 id_counter = 0ll;
static ChromeTrace chrome_trace;
static std::mutex chrome_trace_event_mutex;


ChromeTrace::ChromeTrace()
{
    events.reserve(64000);
}

ChromeTrace::~ChromeTrace()
{
    static const std::map <std::string, std::string> type_mapping = {
        // sync
        {"MFX_SyncOperation", "sync"},
        // dec
        {"MFX_DecodeFrameAsync", "dec"},
        // enc
        {"MFX_EncodeFrameAsync", "enc"},
    };
    for (auto it = events.begin(); it != events.end(); ++it)
    {
        auto resolved_type = type_mapping.find(it->type);
        it->type = resolved_type != type_mapping.end() ? resolved_type->second : it->type;
    }
    // Add flow events (arrows)
    std::map <std::string, Event> flows;
    for (auto it = events.begin(); it != events.end(); ++it)
    {
        if (it->ph != 'E' || it->type == "sync")
            continue;
        auto syncpIterator = std::find_if(
            it->info.begin(), it->info.end(), [](const std::pair <std::string, std::string> &p)
            { return p.first == "syncp"; });
        std::string syncp = syncpIterator == it->info.end() ? "0" : syncpIterator->second;
        if (syncp != "0")
        {
            flows.insert({syncp, *it});
        }
    }
    for (auto it = events.begin(); it != events.end(); ++it)
    {
        if (it->ph != 'E' || it->type != "sync")
            continue;
        auto syncpIterator = std::find_if(
            it->info.begin(), it->info.end(), [](const std::pair <std::string, std::string> &p)
            { return p.first == "syncp"; });
        std::string syncp = syncpIterator == it->info.end() ? "0" : syncpIterator->second;
        auto flowBegin = flows.find(syncp);
        if (flowBegin != flows.end())
        {
            Event e;
            e.name = "link";
            e.cat = "link: " + flowBegin->second.type + "->sync";
            e.type = flowBegin->second.type;
            e.timestamp = flowBegin->second.timestamp;
            e.id = ++id_counter;
            e.ph = 's';
            e.threadId = it->threadId;
            chrome_trace.events.push_back(e);
            e.name = "link";
            e.cat = "link: " + flowBegin->second.type + "->sync";
            e.type = "sync";
            e.timestamp = it->start;
            e.id = id_counter;
            e.ph = 'f';
            chrome_trace.events.push_back(e);
            flows.erase(flowBegin);
        }
    }
    std::lock_guard <std::mutex> lock(chrome_trace_event_mutex);
    if (!chrome_trace_file)
    {
        char buffer[64] = {};
        snprintf(buffer, sizeof(buffer), "chrome_trace_%lu.json", syscall(SYS_getpid));
        chrome_trace_file = fopen(buffer, "w");
        fprintf(chrome_trace_file, "[\n");
    }
    for (ChromeTrace::Event &e : events)
    {
        e.write(chrome_trace_file);
    }
    fclose(chrome_trace_file);
}

void ChromeTrace::Event::write(FILE *file) {
    fprintf(file, "{");
    fprintf(file, "\"name\": \"%s\", \"ph\": \"%c\", \"ts\": %llu, \"pid\": %s, \"tid\": \"%s\", \"id\": %llu, ",
        name.c_str(), ph, timestamp, threadId.c_str(), type.c_str(), id);
    if (ph == 's' || ph == 'f')  // Flow events
    {
        fprintf(file, "\"cat\": \"%s\", \"bp\": \"e\", ", cat.c_str());
    }
    fprintf(file, "\"args\": {");
    for (auto it = info.begin(); it != info.end(); ++it)
    {
        fprintf(file, "\"%s\": \"%s\"", it->first.c_str(), it->second.c_str());
        if (it + 1 != info.end())
        {
            fprintf(file, ", ");
        }
    }
    fprintf(file, "}},\n");
}

ChromeTrace::ScopedEventCreator::ScopedEventCreator(mfxTraceLevel level, const char *name)
    : level(level)
    , name(name)
    , id(id_counter++)
{
    if (level > trace_level) return;
    std::lock_guard <std::mutex> lock(chrome_trace_event_mutex);
    std::string threadId = static_cast<std::ostringstream const &>(std::ostringstream() << std::this_thread::get_id()).str();
    mfxU64 timestamp = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()).time_since_epoch().count();\
    start = timestamp;
    Event e;
    e.name = name;
    e.type = name;
    e.start = e.timestamp = timestamp;
    e.id = id;
    e.ph = 'B';
    e.threadId = threadId;
    chrome_trace.events.push_back(e);
}

void ChromeTrace::ScopedEventCreator::add_info(const std::string &key, const std::string &value)
{
    if (level > trace_level) return;
    info.emplace_back(key, value);
}

void ChromeTrace::ScopedEventCreator::add_info(const std::string &key, void *value)
{
    if (level > trace_level) return;
    info.emplace_back(key, static_cast<std::ostringstream const &>(std::ostringstream() << value).str());
}

ChromeTrace::ScopedEventCreator::~ScopedEventCreator()
{
    if (level > trace_level) return;
    std::lock_guard <std::mutex> lock(chrome_trace_event_mutex);
    mfxU64 timestamp = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()).time_since_epoch().count();
    std::string threadId = static_cast<std::ostringstream const &>(std::ostringstream() << std::this_thread::get_id()).str();
    Event e;
    e.name = name;
    e.type = name;
    e.start = start;
    e.timestamp = timestamp;
    e.id = id;
    e.ph = 'E';
    e.threadId = threadId;
    e.info = std::move(info);
    chrome_trace.events.push_back(e);
}

#endif  // #if defined(MFX_TRACE_ENABLE_CHROME) && (defined(LINUX32))
