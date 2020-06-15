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
#ifndef LOGGER_H_
#define LOGGER_H_

#include <map>
#include "log_console.h"
#include "log_etw_events.h"
#include "log_file.h"
#include "log_syslog.h"

enum eLogType{
    LOG_FILE,
    LOG_CONSOLE,
#if defined(_WIN32) || defined(_WIN64)
    LOG_ETW,
#else
    LOG_SYSLOG,
#endif
};

enum eLogLevel{
    LOG_LEVEL_DEFAULT,
    LOG_LEVEL_SHORT,
    LOG_LEVEL_FULL,
};

class Log
{
public:
    static void WriteLog(const std::string &log);
    static void SetLogType(eLogType type);
    static void SetFilePath(std::string file_path);
    static void SetLogLevel(eLogLevel level);
    static eLogLevel GetLogLevel();
    static void clear();
    static bool useGUI;
private:
    Log();
    ~Log();
    static Log *_sing_log;
    eLogLevel _log_level;
    ILog *_log;
    std::map<eLogType, ILog*> _logmap;
};

#endif //LOGGER_H_
