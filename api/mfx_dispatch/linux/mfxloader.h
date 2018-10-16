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

#ifndef __MFXLOADER_H__
#define __MFXLOADER_H__

#include <limits.h>

#include <sstream>
#include <string>

#include "mfxdefs.h"
#include "mfxplugin.h"

inline bool operator == (const mfxPluginUID &lhs, const mfxPluginUID & rhs)
{
    return !memcmp(lhs.Data, rhs.Data, sizeof(mfxPluginUID));
}

inline bool operator != (const mfxPluginUID &lhs, const mfxPluginUID & rhs)
{
    return !(lhs == rhs);
}

inline bool operator < (const mfxVersion &lhs, const mfxVersion & rhs)
{
    return (lhs.Major < rhs.Major ||
            (lhs.Major == rhs.Major && lhs.Minor < rhs.Minor));
}

inline bool operator <= (const mfxVersion &lhs, const mfxVersion & rhs)
{
    return (lhs < rhs || (lhs.Major == rhs.Major && lhs.Minor == rhs.Minor));
}

namespace MFX {

class PluginInfo : public mfxPluginParam
{
public:
  PluginInfo()
    : mfxPluginParam()
    , m_parsed()
    , m_path()
    , m_default()
  {}

  inline bool isValid() {
    return m_parsed;
  }

  inline mfxPluginUID getUID() {
    return PluginUID;
  }

  inline std::string getPath() {
    return std::string(m_path);
  }

  void Load(char* name, char* value);
  void Print();

private:
  enum
  {
    PARSED_TYPE        = 0x1,
    PARSED_CODEC_ID    = 0x2,
    PARSED_UID         = 0x4,
    PARSED_PATH        = 0x8,
    PARSED_DEFAULT     = 0x10,
    PARSED_VERSION     = 0x20,
    PARSED_API_VERSION = 0x40,
    PARSED_NAME        = 0x80,
  };

  mfxU32 m_parsed;

  char m_path[PATH_MAX];
  bool m_default;
};

void parse(const char* file_name, std::list<PluginInfo>& all_records);

static std::string printUID(const mfxPluginUID& uid)
{
  std::stringstream ss;
  ss << std::hex;
  for (auto c: uid.Data) ss << static_cast<unsigned>(c);
  return ss.str();
}

static std::string printCodecId(mfxU32 id)
{
  uint8_t* data = reinterpret_cast<uint8_t*>(&id);
  std::stringstream ss;
  for (size_t i=0; i < sizeof(id); ++i) ss << data[i];
  return ss.str();
}

static void print(std::list<PluginInfo>& plugins)
{
  for (auto& plg: plugins) {
    plg.Print();
  }
}

} // namespace MFX

#endif
