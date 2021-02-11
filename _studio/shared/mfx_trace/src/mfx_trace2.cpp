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

#include <cstring>
#include <sstream>
#include <thread>
#include <mfx_trace2.h>
#include <mfx_trace2_textlog.h>
#include <mfx_trace2_chrome.h>
#include <mfx_trace2_stat.h>
#include <mfx_trace2_vtune_ftrace.h>

mfx::Trace _mfx_trace;


mfx::Trace::Trace()
{
#ifdef __linux__
    FILE *config = fopen((std::string(getenv("HOME")) + "/.mfx_trace").c_str(), "r");
    if (!config)
        config = fopen("/etc/mfx_trace", "r");
    if (!config)
        return;
    char buffer[128] = {};
    std::map <std::string, std::string> options;
    options["TextLog"] = "/tmp/mfx.log";
    while (fgets(buffer, sizeof(buffer), config))
    {
        char key_buffer[32] = {}, val_buffer[32] = {};
        sscanf(buffer, "%[^=]=%s\n", key_buffer, val_buffer);
        options[key_buffer] = val_buffer;
    }
    fclose(config);
#endif  // __linux__

    events.reserve(64000);
#if defined(USE_MFX_TRACE2) && defined(MFX_TRACE_ENABLE_TEXTLOG)
    if (options["Output"] == "0x30") // maybe incorrect code
    {
        backends.push_back(std::unique_ptr<TraceBackend>(new TextLog(options["TextLog"].c_str())));
        printf("[TRACE] TextLog backend %s is enabled!\n", options["TextLog"].c_str());
    }
#endif
#if defined(USE_MFX_TRACE2) && defined(MFX_TRACE_ENABLE_STAT)
    backends.push_back(std::unique_ptr<TraceBackend>(new Stat("/tmp/stat2.log")));
    printf("[TRACE] Stat backend is enabled!\n");
#endif
#if defined(USE_MFX_TRACE2) && defined(MFX_TRACE_ENABLE_CHROME)
    backends.push_back(std::unique_ptr<TraceBackend>(new Chrome));
    printf("[TRACE] Chrome backend is enabled!\n");
#endif
#if defined(USE_MFX_TRACE2) && defined(MFX_TRACE_ENABLE_FTRACE)
    backends.push_back(std::unique_ptr<TraceBackend>(new VTune));
    printf("[TRACE] VTune backend is enabled!\n");
#endif
}

void mfx::Trace::pushEvent(const mfx::Trace::Event &e)
{
    std::lock_guard<std::mutex> guard(traceMutex);
    events.push_back(e);
    for (const auto &backend : backends)
    {
        backend->handleEvent(e);
    }
}

mfx::Trace::~Trace()
{
}

mfx::Trace::Node::Node()
    : type(mfx::Trace::NodeType::NONE)
{
}

mfx::Trace::Node::Node(const std::string &str)
    : type(mfx::Trace::NodeType::STRING)
    , str(str)
{
}

mfx::Trace::Node::Node(const std::map <std::string, Node> &map)
    : type(mfx::Trace::NodeType::MAPPING)
    , map(map)
{
}

mfx::Trace::Node::Node(const std::vector <Node> &vec)
    : type(mfx::Trace::NodeType::VECTOR)
    , vec(vec)
{
}

mfx::Trace::Node::Node(const mfx::Trace::Node &n)
{
    type = n.type;
    switch (type)
    {
    case NodeType::NONE:
        break;
    case NodeType::STRING:
        new (&str) std::string;
        str = n.str;
        break;
    case NodeType::MAPPING:
        new (&map) std::map <std::string, Node>;
        map = n.map;
        break;
    case NodeType::VECTOR:
        new (&vec) std::vector <Node>;
        vec = n.vec;
        break;
    }
}

mfx::Trace::Node &mfx::Trace::Node::operator=(const mfx::Trace::Node &n)
{
    type = n.type;
    switch (type)
    {
    case NodeType::NONE:
        break;
    case NodeType::STRING:
        new (&str) std::string;
        str = n.str;
        break;
    case NodeType::MAPPING:
        new (&map) std::map <std::string, Node>;
        map = n.map;
        break;
    case NodeType::VECTOR:
        new (&vec) std::vector <Node>;
        vec = n.vec;
        break;
    }
    return *this;
}

mfx::Trace::Node::~Node()
{
    switch (type)
    {
    case NodeType::NONE:
        break;
    case NodeType::STRING:
        str.~basic_string();
        break;
    case NodeType::MAPPING:
        map.~map();
        break;
    case NodeType::VECTOR:
        vec.~vector();
        break;
    }
}

mfx::source_location::source_location(mfxU32 _line, const char* _file_name, const char* _function_name)
    : _line(_line)
    , _file_name(_file_name)
    , _function_name(_function_name)
{
}

mfx::Trace::Scope::Scope(source_location sl, const char* name, mfxU8 level)
    : level(level)
{
    if (_mfx_trace.backends.size() == 0) return;
    if (level < _mfx_trace.loggingLevel) return;
    e.sl = sl;
    e.name = name;
    e.timestamp = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()).time_since_epoch().count();
    e.threadId = static_cast<std::ostringstream const &>(std::ostringstream() << std::this_thread::get_id()).str();
    e.parentIndex = 0;
    e.id = _mfx_trace.idCounter++;
    e.category = nullptr;
    e.type = mfx::Trace::EventType::BEGIN;
    parentIndex = _mfx_trace.events.size();
    _mfx_trace.pushEvent(e);
    e.parentIndex = parentIndex;
}

mfx::Trace::Scope::Scope(source_location sl, const char* name, const char *category, mfxU8 level)
    : level(level)
{
    if (_mfx_trace.backends.size() == 0) return;
    if (level < _mfx_trace.loggingLevel) return;
    e.sl = sl;
    e.name = name;
    e.timestamp = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()).time_since_epoch().count();
    e.threadId = static_cast<std::ostringstream const &>(std::ostringstream() << std::this_thread::get_id()).str();
    e.parentIndex = 0;
    e.id = _mfx_trace.idCounter++;
    e.category = category;
    e.type = mfx::Trace::EventType::BEGIN;
    parentIndex = _mfx_trace.events.size();
    _mfx_trace.pushEvent(e);
    e.parentIndex = parentIndex;
}

mfx::Trace::Scope::~Scope()
{
    if (_mfx_trace.backends.size() == 0) return;
    if (level < _mfx_trace.loggingLevel) return;
    e.type = mfx::Trace::EventType::END;
    e.description = "";
    e.timestamp = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()).time_since_epoch().count();
    _mfx_trace.pushEvent(e);
}

std::string mfx::Trace::hex(mfxU64 value)
{
    return static_cast<std::ostringstream const &>(std::ostringstream() << "0x" << std::hex << value).str();
}

std::string mfx::Trace::hex(mfxU32 value)
{
    return static_cast<std::ostringstream const &>(std::ostringstream() << "0x" << std::hex << value).str();
}

std::string mfx::Trace::hex(mfxU16 value)
{
    return static_cast<std::ostringstream const &>(std::ostringstream() << "0x" << std::hex << value).str();
}

std::string mfx::Trace::hex(mfxU8 value)
{
    return static_cast<std::ostringstream const &>(std::ostringstream() << "0x" << std::hex << (mfxU16)value).str();
}

void mfx::Trace::Scope::event_pair(const char* key, const std::string &value)
{
    e.map.emplace(key, Node(value));
}

void mfx::Trace::Scope::event_pair(const char* key, mfxU32 value)
{
    e.map.emplace(key, Node(std::to_string(value)));
}

void mfx::Trace::Scope::event_pair(const char* key, const mfxVideoParam &value)
{
    // Not all fields are listed here
    std::map <std::string, Node> mfxVideoParamMap;
    mfxVideoParamMap.emplace("AllocId", Node(std::to_string(value.AllocId)));
    mfxVideoParamMap.emplace("NumExtParam", Node(std::to_string(value.NumExtParam)));
    std::map <std::string, Node> mfxInfoMFXMap;
    {
        mfxInfoMFXMap.emplace("LowPower", Node(std::to_string(value.mfx.LowPower)));
        std::map <std::string, Node> frameInfoMap;
        {
            frameInfoMap.emplace("Width", Node(std::to_string(value.mfx.FrameInfo.Width)));
            frameInfoMap.emplace("Height", Node(std::to_string(value.mfx.FrameInfo.Height)));
            std::map <std::string, Node> frameIdMap;
            {
                frameIdMap.emplace("TemporalId", Node(std::to_string(value.mfx.FrameInfo.FrameId.TemporalId)));
                frameIdMap.emplace("PriorityId", Node(std::to_string(value.mfx.FrameInfo.FrameId.PriorityId)));
            }
            frameInfoMap.emplace("FrameId", Node(frameIdMap));
        }
        mfxInfoMFXMap.emplace("FrameInfo", Node(frameInfoMap));
    }
    mfxVideoParamMap.emplace("mfxInfoMFX", Node(mfxInfoMFXMap));
    e.map.emplace(key, Node(mfxVideoParamMap));
}

void mfx::Trace::Scope::event_pair(const char* key, const mfxExtBuffer &value)
{
    std::map <std::string, Node> extBufferMap;
    extBufferMap.emplace("BufferId", std::to_string(value.BufferId));
    extBufferMap.emplace("BufferSz", std::to_string(value.BufferSz));
    e.map.emplace(key, Node(extBufferMap));
}

void mfx::Trace::Scope::event_pair(const char* key, const mfxExtCodingOption2 &value)
{
    std::map <std::string, Node> codingOptionMap;
    codingOptionMap.emplace("IntRefType", Node(std::to_string(value.IntRefType)));
    codingOptionMap.emplace("IntRefCycleSize", Node(std::to_string(value.IntRefCycleSize)));
    codingOptionMap.emplace("IntRefQPDelta", Node(std::to_string(value.IntRefQPDelta)));
    codingOptionMap.emplace("MaxFrameSize", Node(std::to_string(value.MaxFrameSize)));
    codingOptionMap.emplace("MaxSliceSize", Node(std::to_string(value.MaxSliceSize)));
    codingOptionMap.emplace("BitrateLimit", Node(std::to_string(value.BitrateLimit)));
    codingOptionMap.emplace("MBBRC", Node(std::to_string(value.MBBRC)));
    codingOptionMap.emplace("ExtBRC", Node(std::to_string(value.ExtBRC)));
    codingOptionMap.emplace("LookAheadDepth", Node(std::to_string(value.LookAheadDepth)));
    codingOptionMap.emplace("Trellis", Node(std::to_string(value.Trellis)));
    codingOptionMap.emplace("RepeatPPS", Node(std::to_string(value.RepeatPPS)));
    codingOptionMap.emplace("BRefType", Node(std::to_string(value.BRefType)));
    codingOptionMap.emplace("AdaptiveI", Node(std::to_string(value.AdaptiveI)));
    codingOptionMap.emplace("AdaptiveB", Node(std::to_string(value.AdaptiveB)));
    codingOptionMap.emplace("LookAheadDS", Node(std::to_string(value.LookAheadDS)));
    codingOptionMap.emplace("NumMbPerSlice", Node(std::to_string(value.NumMbPerSlice)));
    codingOptionMap.emplace("SkipFrame", Node(std::to_string(value.SkipFrame)));
    codingOptionMap.emplace("MinQPI", Node(std::to_string(value.MinQPI)));
    codingOptionMap.emplace("MaxQPI", Node(std::to_string(value.MaxQPI)));
    codingOptionMap.emplace("MinQPP", Node(std::to_string(value.MinQPP)));
    codingOptionMap.emplace("MaxQPP", Node(std::to_string(value.MaxQPP)));
    codingOptionMap.emplace("MinQPB", Node(std::to_string(value.MinQPB)));
    codingOptionMap.emplace("MaxQPB", Node(std::to_string(value.MaxQPB)));
    codingOptionMap.emplace("FixedFrameRate", Node(std::to_string(value.FixedFrameRate)));
    codingOptionMap.emplace("DisableDeblockingIdc", Node(std::to_string(value.DisableDeblockingIdc)));
    codingOptionMap.emplace("DisableVUI", Node(std::to_string(value.DisableVUI)));
    codingOptionMap.emplace("BufferingPeriodSEI", Node(std::to_string(value.BufferingPeriodSEI)));
    codingOptionMap.emplace("EnableMAD", Node(std::to_string(value.EnableMAD)));
    codingOptionMap.emplace("UseRawRef", Node(std::to_string(value.UseRawRef)));
    e.map.emplace(key, Node(codingOptionMap));
}

void mfx::Trace::Scope::event_pair(const char* key, const MFX_GUID &value)
{
    std::vector <Node> guidVec;
    guidVec.emplace_back(Node(hex(value.Data1)));
    guidVec.emplace_back(Node(hex(value.Data2)));
    guidVec.emplace_back(Node(hex(value.Data3)));
    std::vector <Node> guidData4Vec;
    for (unsigned i = 0; i < sizeof(value.Data4) / sizeof(value.Data4[0]); ++i)
        guidData4Vec.emplace_back(Node(hex(static_cast<mfxU8>(value.Data4[i]))));
    guidVec.emplace_back(Node(guidData4Vec));
    e.map.emplace(key, Node(guidVec));
}

void mfx::Trace::Scope::event_pair(const char* key, void *value)
{
    std::string val = static_cast<std::ostringstream const &>(std::ostringstream() << value).str();
    e.map.emplace(key, Node(val));
}
