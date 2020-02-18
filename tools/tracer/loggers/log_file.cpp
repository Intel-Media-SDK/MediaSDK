/* ****************************************************************************** *\

Copyright (C) 2012-2020 Intel Corporation.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
- Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
- Neither the name of Intel Corporation nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY INTEL CORPORATION "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL INTEL CORPORATION BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

File Name: log_file.cpp

\* ****************************************************************************** */

#include "log_file.h"
#include "../config/config.h"
#include "../dumps/dump.h"

std::mutex LogFile::_file_write;

LogFile::LogFile()
{
    std::string strproc_id = ToString(ThreadInfo::GetProcessId());
    std::string file_log = Config::GetParam("core", "log");
    if(!file_log.empty())
        _file_path = std::string(file_log);
    else
        _file_path = std::string("mfxtracer.log");

    if (!Log::useGUI)
    {
        strproc_id = std::string("_") + strproc_id;
        size_t pos = _file_path.rfind(".");
        if (pos == std::string::npos)
            _file_path.insert(_file_path.length(), strproc_id);
        else if((_file_path.length() - pos) > std::string(".log").length())
            _file_path.insert(_file_path.length(), strproc_id);
        else
            _file_path.insert(pos, strproc_id);
    }
}

LogFile::~LogFile()
{
    if(_file.is_open())
        _file.close();
}

void LogFile::WriteLog(const std::string &log)
{
    std::unique_lock<std::mutex> write_mutex(_file_write);
    if(!_file.is_open())
        _file.open(_file_path.c_str(), std::ios_base::app);

    std::stringstream str_stream;
    str_stream << log;
    std::string spase = "";
    for(;;) {
        spase = "";
        std::string logstr;
        getline(str_stream, logstr);
        if(log.find("function:") == std::string::npos && log.find(">>") == std::string::npos) spase = "    ";
        std::stringstream pre_out;
        if(logstr.length() > 2) pre_out << ThreadInfo::GetThreadId() << " " << Timer::GetTimeStamp() << " " << spase << logstr << std::endl;
        else pre_out << logstr << "\n";
        _file << pre_out.str();
        if(str_stream.eof())
            break;
    }
    _file.flush();

    write_mutex.unlock();
}

void LogFile::SetFilePath(std::string file_path)
{
    if(_file.is_open())
        _file.close();

    _file_path = file_path;
}
