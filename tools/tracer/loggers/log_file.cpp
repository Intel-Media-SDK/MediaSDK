// Copyright (c) 2018 Intel Corporation
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
