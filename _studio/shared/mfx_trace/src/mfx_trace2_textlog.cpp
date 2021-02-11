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

#include <mfx_trace2_textlog.h>

#ifdef MFX_TRACE_ENABLE_TEXTLOG

mfx::TextLog::TextLog(const char *filename)
{
    file = fopen(filename, "w");
}

static void print_node(const mfx::Trace::Node &node, FILE *f, unsigned depth = 1)
{
    if (node.type == mfx::Trace::NodeType::STRING)
    {
        fprintf(f, "\"%s\"", node.str.c_str());
    }
    else if (node.type == mfx::Trace::NodeType::MAPPING)
    {
        mfxU64 index = 0;
        for (const auto &pair : node.map)
        {
            fprintf(f, "\n");
            for (unsigned i = 0; i < depth + 1; ++i) fprintf(f, "\t");
            fprintf(f, "\"%s\": ", pair.first.c_str());
            print_node(pair.second, f, depth + 1);
            ++index;
        }
    }
    else if (node.type == mfx::Trace::NodeType::VECTOR)
    {
        for (size_t i = 0; i < node.vec.size(); ++i)
        {
            print_node(node.vec[i], f);
            if (i + 1 != node.vec.size())
            {
                fprintf(f, ", ");
            }
        }
    }
}

void mfx::TextLog::handleEvent(const mfx::Trace::Event &e)
{
    const char *eventType = "";
    switch (e.type)
    {
    case mfx::Trace::EventType::BEGIN:
        eventType = "ENTER";
        break;
    case mfx::Trace::EventType::END:
        eventType = "EXIT";
        break;
    case mfx::Trace::EventType::INFO:
        eventType = "VARIABLE";
        break;
    default:
        break;
    }
    fprintf(file, "%s:%d - %s %llu %s\n", e.sl.file_name(), e.sl.line(), e.sl.function_name(), e.timestamp, eventType);
    if (e.type == mfx::Trace::EventType::INFO)
    {
        auto entry = e.map.find(e.description);
        if (entry != e.map.end())
        {
            fprintf(file, "\t\"%s\":", entry->first.c_str());
            print_node(entry->second, file);
            fprintf(file, "\n");
        }
    }
}

mfx::TextLog::~TextLog()
{
    fclose(file);
}

#endif  // MFX_TRACE_ENABLE_TEXTLOG
