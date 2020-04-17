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

File Name: log.cpp

\* ****************************************************************************** */

#include "log.h"

Log* Log::_sing_log = NULL;
bool Log::useGUI = false;

Log::Log()
{
    _log_level = LOG_LEVEL_DEFAULT;

    _logmap = {
       std::pair<eLogType,ILog*>(LOG_CONSOLE, new LogConsole())
      ,std::pair<eLogType,ILog*>(LOG_FILE, new LogFile())
#if defined(_WIN32) || defined(_WIN64)
      ,std::pair<eLogType,ILog*>(LOG_ETW, new LogEtwEvents())
#else
      ,std::pair<eLogType,ILog*>(LOG_SYSLOG, new LogSyslog())
#endif
    };

    _log = _logmap[LOG_FILE];


    _logmap.insert(std::pair<eLogType,ILog*>(LOG_CONSOLE, new LogConsole()));
    _logmap.insert(std::pair<eLogType,ILog*>(LOG_FILE, new LogFile()));
#if defined(_WIN32) || defined(_WIN64)
    _logmap.insert(std::pair<eLogType,ILog*>(LOG_ETW, new LogEtwEvents()));
#else
    _logmap.insert(std::pair<eLogType,ILog*>(LOG_SYSLOG, new LogSyslog()));
#endif // _WIN32...
    _log = _logmap[LOG_CONSOLE];

}

Log::~Log()
{
    for(std::map<eLogType,ILog*>::iterator it=_logmap.begin(); it!=_logmap.end(); ++it){
        delete it->second;
    }
    _logmap.clear();
}

void Log::SetLogType(eLogType type)
{
    if(!_sing_log)
        _sing_log = new Log();

    _sing_log->_log = _sing_log->_logmap[type];
}

void Log::SetLogLevel(eLogLevel level)
{
    if(!_sing_log)
        _sing_log = new Log();

    _sing_log->_log_level = level;
}

eLogLevel Log::GetLogLevel()
{
    if(!_sing_log)
        _sing_log = new Log();

    return _sing_log->_log_level;
}

void Log::clear()
{
    if (_sing_log)
    {
        delete _sing_log;
        _sing_log = NULL;
    }
}

void Log::WriteLog(const std::string &log)
{

#if defined(_WIN32) || defined(_WIN64)
    DWORD start_flag = 0;
    DWORD size = sizeof(DWORD);
    RegGetValue(HKEY_LOCAL_MACHINE, ("Software\\Intel\\MediaSDK\\Dispatch\\tracer"), ("_start"), REG_DWORD, 0, (BYTE*)&start_flag, &size); 

    if ((start_flag) || (!useGUI))
#endif
        {
            if(!_sing_log)
                _sing_log = new Log();

            if(_sing_log->_log_level == LOG_LEVEL_DEFAULT){
                _sing_log->_log->WriteLog(log);
            }
            else if(_sing_log->_log_level == LOG_LEVEL_FULL){
                _sing_log->_log->WriteLog(log);
            }
            else if(_sing_log->_log_level == LOG_LEVEL_SHORT){
                if(log.find("function:") != std::string::npos)
                    _sing_log->_log->WriteLog(log);
            }
        }
}

