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

#include "iostream"
#include "cmassert.h"

class CmExampleTest
{
public:
  virtual ~CmExampleTest () = 0;

public:
  bool Test ()
  {
    bool result = true;

    try
    {
      Setup ();

      RunAndTimeFor (*this, &CmExampleTest::RunExpect, "Expect ");

      RunAndTimeFor (*this, &CmExampleTest::RunActual, "Actual ");

      Validate ();
    } catch (std::exception & exp) {
      std::cout << "Error => " << exp.what () << std::endl;
      result = false;
    }

    try
    {
      Teardown ();
    } catch (...) {
      result = false;
    }

    return result;
  }

protected:
  virtual void Setup () = 0;
  virtual void Teardown () = 0;
  virtual void RunActual () = 0;
  virtual void RunExpect () = 0;
  virtual void Validate () = 0;

protected:
  template<typename objectTy, typename funcTy>
  static void RunAndTimeFor (objectTy & object, funcTy func, const char * description)
  {
    //Clock clock;
    //clock.Begin ();

    (object.*func) ();

    //clock.End ();

    //std::cout << description << " @ " << clock.Duration () << " ms" << std::endl;
  }
};

inline CmExampleTest::~CmExampleTest() {}

inline void AssertEqual (float x, float y, float threshold = 1e-3)
{
  float diff = x - y;
  if (diff > threshold || diff < -threshold) {
    throw MDFUT_EXCEPTION("AssertEqual Fail");
  }
}
