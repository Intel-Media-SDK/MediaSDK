// Copyright (c) 2017 Intel Corporation
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

#pragma once

#if defined MDFUT_PERF && !defined(__GNUC__)
#include "windows.h"
#include "winbase.h"

#include "hash_map"
#include "string"
#include "iostream"
#include "iomanip"

#include "assert.h"
#include "cminfrastructure.h"

namespace mdfut {
  namespace benchmark {
    class Benchmark
    {
    public:
      Benchmark(std::string name) : name(name) {}
      Benchmark() : name("Benchmark") {}
      ~Benchmark()
      {
        Statistics();
        for (auto pair : clocks) {
          delete pair.second;
        }
        clocks.clear();
      }

      LONGLONG Duration(std::string name) const
      {
        auto iter = clocks.find(name);
        assert(iter != clocks.end());

        return iter->second->Duration();
      }

      void Begin(std::string name)
      {
        auto iter = clocks.find(name);
        if (iter == clocks.end()) {
          auto pair = clocks.insert(std::make_pair(name, new Clock()));
          iter = pair.first;
          //if (!pair.second) {
          //  throw std::exception ();
          //}
        }

        iter->second->Begin();
      }

      void End(std::string name)
      {
        auto iter = clocks.find(name);
        assert(iter != clocks.end());

        if (iter != clocks.end()) {
          iter->second->End();
        }
      }

      void Statistics()
      {
        for (auto pair : clocks) {
          cout << name << " " << pair.first << " # "
               << pair.second->Count() << " calls, "
               << pair.second->Duration() << " microseconds, "
               << pair.second->Duration() / pair.second->Count() << " microseconds/call"
               << endl;
        }
      }

    protected:
      stdext::hash_map<std::string, mdfut::Clock *> clocks;
      std::string name;
    };

    class BenchmarkAspect
    {
    public:
      static Benchmark & benchmark()
      {
        static Benchmark benchmark;
        return benchmark;
      }

      BenchmarkAspect(const char * pFunctionName) : functionName(pFunctionName)
      {
        benchmark().Begin(functionName.c_str());
      }

      ~BenchmarkAspect()
      {
        benchmark().End(functionName.c_str());
      }

    protected:
      std::string functionName;
    };
  };
};

template<typename R, typename C, typename ArgType>
R DEDUCE_RETURN_TYPE_1(R (C::*)(ArgType));

template<typename C, typename ArgType>
void DEDUCE_RETURN_TYPE_1(void (C::*)(ArgType));

#define MDFUT_ASPECT_CUT_FUNCTION_1(FunctionName, ArgType)  \
  auto FunctionName(ArgType v) -> decltype(DEDUCE_RETURN_TYPE_1(&CuttedCls::##FunctionName))  \
  { \
    AspectCls aspect(__FUNCTION__); \
    return CuttedCls::##FunctionName(v); \
  }

#define MDFUT_ASPECT_CUT_START(cls, aspect) \
  class cls : public mdfut::##cls \
  { \
  public: \
    typedef mdfut::##cls CuttedCls; \
    typedef aspect AspectCls;

#define MDFUT_ASPECT_CUT_END  \
  };

#endif