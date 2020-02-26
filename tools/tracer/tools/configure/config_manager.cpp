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
#include <fstream>

#include "config_manager.h"

ConfigManager *ConfigManager::manager = 0;

ConfigManager::ConfigManager(void)
{
}

ConfigManager::~ConfigManager(void)
{
    if(manager)
        delete manager;
}

map<string, map<string, string> > ConfigManager::ParceConfig(string config_path)
{
    if(!manager)
        manager = new ConfigManager();

    map<string, map<string, string> > ini;
    ifstream  _file(config_path.c_str(), ifstream::binary);
    if(_file.is_open()) {
        //TODO parse
        std::string curent_section;
        std::map<std::string, std::string> section_params;
        while(true) {
            std::string inistr;
            getline(_file, inistr);

            //delete tabs and spaces
            std::string::size_type firstQuote = inistr.find("\"");
            std::string::size_type secondQuote = inistr.find_last_of("\"");
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
            basic_string <char>::size_type n1 = inistr.find("#");
            basic_string <char>::size_type n2 = inistr.find(";");

            if(n1 != string::npos && n2 != string::npos)
                n1>=n2 ? inistr.erase(n1, inistr.length()) : inistr.erase(n2, inistr.length());
            else if (n1 != string::npos)
                   inistr.erase(n1, inistr.length());
            else if(n2 != string::npos)
                inistr.erase(n2, inistr.length());


            basic_string <char>::size_type s1 = inistr.find("[");
            basic_string <char>::size_type s2 = inistr.find("]");
            if(s1 != string::npos && s2 != string::npos) {
                inistr.erase(s1, s1+1);
                inistr.erase(s2-1, s2);
                curent_section = inistr;
                section_params.clear();

                ini.insert(pair<string, map<string, string> >(inistr, section_params));

                continue;
            }

            if(!curent_section.empty()){
                vector<string> key_val = split(inistr, '=');
                if(key_val.size() == 2){
                    ini[curent_section].insert(std::pair<std::string, std::string>(key_val[0], key_val[1]));
                }
            }

            if(_file.eof())
                break;
        }
        _file.close();
    }

    return ini;
}

bool ConfigManager::CreateConfig(string config_path, map<string, map<string, string> > config)
{
    ofstream _file(config_path.c_str());
    if(_file.is_open()){
        for(map<string, map<string, string> >::iterator it = config.begin(); it != config.end(); ++it){
            _file << string("[") + (*it).first + string("]\n");

             map<string, string> section_params = (*it).second;
             for(map<string, string>::iterator itparams = section_params.begin(); itparams != section_params.end(); ++itparams){
                _file << "  " << (*itparams).first + string(" = ") + (*itparams).second + string("\n");
             }

             _file << "\n";
        }

        _file.close();
        return true;
    }
    return false;
}
