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

File Name: config.cpp

\* ****************************************************************************** */

#include <algorithm>
#include <ctype.h>
#include <sstream>
#include <string.h>

#include "config.h"

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
const char* get_path_reg()
    {
        char* path = new char[128];
        DWORD size = sizeof(char)*128;
        DWORD sts = RegGetValue(HKEY_LOCAL_MACHINE, (LPCTSTR)("Software\\Intel\\MediaSDK\\Dispatch\\tracer"), (LPCTSTR)("_conf"), RRF_RT_REG_SZ, (LPDWORD)0, (PVOID)path, (LPDWORD)&size);
        size_t len = strnlen_s(path, 128);

        if (sts == ERROR_SUCCESS)
        {
            for (size_t i = 0; i < len; i++)
            {
                    if (path[i] == '\\')
                        path[i] = '/';
            }

        }
        else
        {
            GetCurrentDirectory(128, path);
            len = strnlen_s(path, 128);
            for (size_t i = 0; i < len; i++)
            {
                    if (path[i] == '\\')
                        path[i] = '/';
            }
        }
        return path;
    }
#endif

Config* Config::conf = NULL;

Config::Config()
{
    const char* home =
#if defined(_WIN32) || defined(_WIN64)
        get_path_reg();
#else
        getenv("HOME");
#endif
    if (home) {
        _file_path = std::string(home) + "/.mfxtracer";

        if(!_file.is_open()){
            _file.open(_file_path.c_str(), std::ifstream::binary);
        }
    }
    
    Init();
}

Config::~Config()
{
    if(_file.is_open())
        _file.close();

    //delete ini;
}

void Config::Init()
{
    //delete tabs and spaces
    //delete comments
    //parse sections
    //parse params in sections

    if(_file.is_open()){
        //TODO parse
        std::string curent_section;
        std::map<std::string, std::string> section_params;
        for (;;) {
            std::string inistr;
            getline(_file, inistr);

            //delete tabs and spaces
            size_t firstQuote = inistr.find("\"");
            size_t secondQuote = inistr.find_last_of("\"");
            std::string firstPart="";
            std::string secondPart="";
            std::string quotePart="";
            if ((firstQuote != std::string::npos) && firstQuote < secondQuote)
            {
               firstPart = inistr.substr(0,firstQuote);
               if (secondQuote != std::string::npos)
               {
                  secondPart = inistr.substr(secondQuote+1);
                  quotePart = inistr.substr(firstQuote+1, secondQuote-firstQuote-1);
               }

               firstPart.erase(std::remove_if(firstPart.begin(), firstPart.end(), &::isspace),firstPart.end());
               secondPart.erase(std::remove_if(secondPart.begin(), secondPart.end(), &::isspace),secondPart.end());
               inistr = firstPart + quotePart + secondPart;
            }
            else
            {
                inistr.erase(std::remove_if(inistr.begin(), inistr.end(), &::isspace),inistr.end());
            }

            //delete comments
            std::basic_string <char>::size_type n1 = inistr.find("#");
            std::basic_string <char>::size_type n2 = inistr.find(";");

            if(n1 != std::string::npos && n2 != std::string::npos)
                n1>=n2 ? inistr.erase(n1, inistr.length()) : inistr.erase(n2, inistr.length());
            else if (n1 != std::string::npos)
                   inistr.erase(n1, inistr.length());
            else if(n2 != std::string::npos)
                inistr.erase(n2, inistr.length());


            std::basic_string <char>::size_type s1 = inistr.find("[");
            std::basic_string <char>::size_type s2 = inistr.find("]");
            if(s1 != std::string::npos && s2 != std::string::npos) {
                inistr.erase(s1, s1+1);
                inistr.erase(s2-1, s2);
                curent_section = inistr;
                section_params.clear();

                ini.insert(std::pair<std::string, std::map<std::string, std::string> >(inistr, section_params));

                continue;
            }

            if(!curent_section.empty()){
                std::vector<std::string> key_val = split(inistr, '=');
                if(key_val.size() == 2){
                    ini[curent_section].insert(std::pair<std::string, std::string>(key_val[0], key_val[1]));
                }
            }

            if(_file.eof())
                break;
        }

    }
    else {
        //TODO add init default config
    }
}

std::vector<std::string> &Config::split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

std::vector<std::string> Config::split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}

std::string Config::GetParam(std::string section, std::string key)
{
    if(!conf)
        conf = new Config();

    return conf->ini[section][key];
}


