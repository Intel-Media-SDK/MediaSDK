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
#include <exception>
#include <iostream>

#include "config_manager.h"
#include "strfuncs.h"

#if defined(_WIN32) || defined(_WIN64)
    #define LOG_TYPES "console, file, etw"
    #define HOME string(getenv("HOMEPATH"))
#else
    #define LOG_TYPES "console, file, syslog"
    #define HOME string(getenv("HOME"))
#endif

#define LOG_LEVELS "default, short, full"

int main(int argc, char *argv[])
{
    try {
        string config_path = HOME + string("/.mfxtracer");
        const string help =
            "\n"
            "Intel Media SDK Tracer Config Generator v. 1.0 \n"
            "\n"
            "Usage: mfx-tracer config [section.name=value ...]\n"
            "\n"
            "Options:\n"
            "  --help, -h  print help\n"
            "  --default   generate default config\n"
            "\n"
            "Major config params:\n"
            "  [core]\n"
            "    edit      enable or disable edit of config file (1 - enable, 0 - disable)\n"
            "    type      log type (you can use: " LOG_TYPES ")\n"
            "    log       log file to dump trace (if applicable)\n"
            "    level     log level (you can use: " LOG_LEVELS ")\n"
            "\n"
            "Examples:\n"
            "  mfx-tracer config --default                                # generate default config file\n"
            "  mfx-tracer config core.type file core.file ~/mfxtracer.log # set trace type and log file\n"
            "\n"
            "Config file: ~/.mfxtracer\n"
            "\n";

        map<string, map<string, string> > ini;
        map<string, map<string, string> > config;

        for (int i=1; i<argc; i++) {
            if (string(argv[i]) == string("--help") || string(argv[i]) == string("-h")) {
                cout << help;
                return 0;
            }
            if (string(argv[i]) == string("--default")) {
                map<string, string> default_params;
                default_params.insert(pair<string, string>(string("edit"), string("1")));
                default_params.insert(pair<string, string>(string("type"), string("console")));
                default_params.insert(pair<string, string>(string("level"), string("default")));
                default_params.insert(pair<string, string>(string("log"), string("trace.log")));
                config.insert(pair<string, map<string, string> >(string("core"), default_params));

                if (!ConfigManager::CreateConfig(config_path, config)) {
                    cerr << "error: failed to create default config\n";
                    return -1;
                }
                return 0;
            }
            else {
                vector<string> vsection_params = split(argv[i], '.');
                if (vsection_params.size() == 2) {
                    map<string, string> param;
                    if (ini.find(vsection_params[0]) == ini.end()) {
                        // new section
                        ini.insert(pair<string, map<string, string> >(vsection_params[0], param));
                    }

                    if (++i < argc) {
                        if (ini[vsection_params[0]].find(vsection_params[1]) == ini[vsection_params[0]].end()) {
                            ini[vsection_params[0]].insert(pair<string, string>(vsection_params[1], argv[i]));
                        } else {
                            cerr << "error: this parameter was already set: " << argv[i];
                            return -1;
                        }
                    } else {
                        cerr << "error: parameter value is not specified: " << argv[i-1] << "\n";
                        return -1;
                    }
                } else {
                    cerr << "error: invalid set of config parameter" << argv[i] << "\n";
                    return -1;
                }
            }
        }

        if (!ini.size()) {
            cerr << "error: nothing to do\n";
            return -1;
        }

        config = ConfigManager::ParceConfig(config_path);
        if (config.size() > 0) {
            if (config.find("core") != config.end()) {
                if (config.at("core").find("edit") != config.at("core").end()) {
                    if (config.at("core").at("edit") == string("0")) {
                        cerr << "fatal: writing config disabled, set core.edit=1 in the config file\n";
                        return -1;
                    }
                }
            }
        } else {
            map<string, string> params;
            params.insert(pair<string, string>(string("edit"), string("1")));
            config.insert(pair<string, map<string, string> >(string("core"), params));
        }

        map<string, map<string, string> > new_conf;
        for (map<string, map<string, string> >::iterator itini = ini.begin(); itini != ini.end(); ++itini) {
            if (config.find((*itini).first) != config.end()) {
                map<string, string> section_ini = (*itini).second;
                for (map<string, string>::iterator itsection_ini = section_ini.begin(); itsection_ini != section_ini.end(); ++itsection_ini) {
                    config.at((*itini).first)[(*itsection_ini).first] = (*itsection_ini).second;
                }
            }
            else {
                new_conf.insert((*itini));
            }
        }
        config.insert(new_conf.begin(), new_conf.end());

        if (!ConfigManager::CreateConfig(config_path, config)) {
            cerr << "error: failed to create config\n";
            return -1;
        }
        return 0;
    }
    catch(exception const &ex){
        cerr << string("exception: ") + ex.what() + "\n";
        return -2;
    }
}
