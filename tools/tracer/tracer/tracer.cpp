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

#include "tracer.h"

void tracer_init()
{
    std::string type = Config::GetParam("core", "type");
    if (type == std::string("console")) {
        Log::SetLogType(LOG_CONSOLE);
    } else if (type == std::string("file")) {
        Log::SetLogType(LOG_FILE);
    } else {
        // TODO: what to do with incorrect setting?
        Log::SetLogType(LOG_CONSOLE);
    }

    std::string log_level = Config::GetParam("core", "level");
    if(log_level == std::string("default")){
         Log::SetLogLevel(LOG_LEVEL_DEFAULT);
    }
    else if(log_level == std::string("short")){
        Log::SetLogLevel(LOG_LEVEL_SHORT);
    }
    else if(log_level == std::string("full")){
        Log::SetLogLevel(LOG_LEVEL_FULL);
    }
    else{
        // TODO
        Log::SetLogLevel(LOG_LEVEL_FULL);
    }
}