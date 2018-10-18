// Copyright (c) 2017-2018 Intel Corporation
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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <list>

#include "mfxloader.h"

namespace MFX {

static bool parseGUID(const char* src, mfxPluginUID* uid)
{
    mfxPluginUID plugin_uid{};
    mfxU8* p = plugin_uid.Data;

    int res = sscanf(src,
        "%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx",
        p, p + 1, p + 2, p + 3, p + 4, p + 5, p + 6, p + 7, 
        p + 8, p + 9, p + 10, p + 11, p + 12, p + 13, p + 14, p + 15);

    if (res != sizeof(uid->Data)) {
        return false;
    }

    *uid = plugin_uid;
    return true;
}

void PluginInfo::Load(char* name, char* value)
{
#ifdef LINUX64
    #define FIELD_FileName "FileName64"
#else
    #define FIELD_FileName "FileName32"
#endif

    if (!strcmp(name, "Type")) {
        Type = atoi(value);
        m_parsed |= PARSED_TYPE;
    } else if (!strcmp(name, "CodecID")) {
        const int fourccLen = 4;
        if (strlen(value) == 0 || strlen(value) > fourccLen)
            return;

        CodecId = MFX_MAKEFOURCC(' ',' ',' ',' ');
        char* id = reinterpret_cast<char*>(&CodecId);
        for (size_t i = 0; i < strlen(value); ++i)
            id[i] = value[i];

        m_parsed |= PARSED_CODEC_ID;
    } else if (!strcmp(name, "GUID")) {
        if (!parseGUID(value, &PluginUID))
            return;

        m_parsed |= PARSED_UID;
    } else if (!strcmp(name, "Path") || !strcmp(name, FIELD_FileName)) {
        // strip quotes
        char* p = value + strlen(value) - 1;
        if (*value == '"' && *p == '"') {
            *p = '\0';
            ++value;
        }
        if (strlen(m_path) + strlen("/") + strlen(value) >= PATH_MAX)
            return;
        strcpy(m_path + strlen(m_path), value);
        m_parsed |= PARSED_PATH;
    } else if (0 == strcmp(name, "Default")) {
        m_default = (0 != atoi(value));
        m_parsed |= PARSED_DEFAULT;
    } else if (0 == strcmp(name, "PluginVersion")) {
        PluginVersion = atoi(value);
        m_parsed |= PARSED_VERSION;
    } else if (0 == strcmp(name, "APIVersion")) {
        APIVersion.Version = atoi(value);
        m_parsed |= PARSED_API_VERSION;
    }
}

void PluginInfo::Print()
{
  printf("[%s]\n", printUID(PluginUID).c_str());
  printf("  GUID=%s\n", printUID(PluginUID).c_str());
  printf("  PluginVersion=%d\n", PluginVersion);
  printf("  APIVersion=%d\n", APIVersion.Version);
  printf("  Path=%s\n", m_path);
  printf("  Type=%d\n", Type);
  printf("  CodecID=%s\n", printCodecId(CodecId).c_str());
  printf("  Default=%d\n", m_default);
}

// strip tailing spaces
static inline char* strip(char* s)
{
    char* p = s + strlen(s);
    while (p > s && isspace(*--p)) *p = 0;
    return s;
}

// skip initial spaces
static inline char* skip(char* s)
{
    while (*s && isspace(*s)) ++s;
    return s;
}

void parse(const char* file_name, std::list<PluginInfo>& plugins)
{
  char line[PATH_MAX];
  PluginInfo plg;

  FILE* file = fopen(file_name, "r");
  if (!file)
    return;

  while(fgets(line, PATH_MAX, file)) {
    char* p = skip(line);

    if (strchr(";#", *p)) {
      // skip comments
    } else if (*p == '[') {
      if (plg.isValid()) {
        plugins.push_back(std::move(plg));
        plg = PluginInfo{};
      }
    } else {
      char* name = p;
      char* value = p;

      while(*value && !strchr("=:;#", *value)) ++value;
      if (*value && strchr("=:", *value)) {
        *value++ = '\0';
      }

      p = value;
      while (*p && !strchr(";#", *p)) ++p;
      if (*p != '\0') {
        *p = '\0';
      }

      name = strip(name);
      value = skip(strip(value));

      if (*name != '\0' && *value != '\0') {
        plg.Load(name, value);
      }
    }
  }

  if (plg.isValid()) {
      plugins.push_back(std::move(plg));
  }

  fclose(file);

  //print(plugins); // for debug
}

} // namespace MFX
