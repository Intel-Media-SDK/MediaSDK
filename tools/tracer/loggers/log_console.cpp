/* ****************************************************************************** *\

Copyright (C) 2020 Intel Corporation.  All rights reserved.

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

File Name: log_console.cpp

\* ****************************************************************************** */

#include <iostream>
#include <sstream>
#include "log_console.h"

LogConsole::LogConsole()
{
}

LogConsole::~LogConsole()
{
}

void LogConsole::WriteLog(const std::string &log)
{

    
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
            std::cout << pre_out.str();
            if(str_stream.eof())
                break;
        }
    
}
