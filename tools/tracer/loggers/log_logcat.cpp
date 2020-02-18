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
#if defined(ANDROID)

#include <iostream>
#include <sstream>
#include <android/log.h>
#include "log_logcat.h"

LogLogcat::LogLogcat()
{
}

LogLogcat::~LogLogcat()
{
}

void LogLogcat::WriteLog(const std::string &log)
{

    std::stringstream str_stream;
    str_stream << log;
    std::string spase = "";
    while(true) {
        spase = "";
        std::string logstr;
        std::stringstream finalstr;
        getline(str_stream, logstr);
        if(log.find("function:") == std::string::npos && log.find(">>") == std::string::npos) spase = "    ";
        if(logstr.length() > 2) {
            finalstr << ThreadInfo::GetThreadId() << " " << Timer::GetTimeStamp() << " " << spase << logstr << "\n";
        } else {
            finalstr << logstr << "\n";
        }
        __android_log_print(ANDROID_LOG_DEBUG, "mfxtracer", "%s", finalstr.str().c_str());
        if(str_stream.eof())
            break;
    }
}

#endif //#if defined(ANDROID)
