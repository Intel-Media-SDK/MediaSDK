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

#include "cmrt_cross_platform.h"
#include "vector"
#include "cmkernelex.h"
#include "cmdeviceex.h"
#include "cmassert.h"

namespace mdfut {
  class CmThreadSpaceEx
  {
  public:
    CmThreadSpaceEx(CmDeviceEx & deviceEx, unsigned int width, unsigned int height)
      : deviceEx(deviceEx), pThreadSpace(NULL), width(width), height(height)
    {
      int result = deviceEx->CreateThreadSpace(width, height, pThreadSpace);
      result;
      MDFUT_FAIL_IF(result != CM_SUCCESS || pThreadSpace == NULL, result);
    }

    ~CmThreadSpaceEx()
    {
      deviceEx->DestroyThreadSpace(pThreadSpace);
    }

    operator CmThreadSpace *() { return pThreadSpace; }
    CmThreadSpace * operator ->() { return pThreadSpace; }
    unsigned int ThreadCount() const { return width * height; }

  protected:
    CmDeviceEx & deviceEx;
    CmThreadSpace * pThreadSpace;
    unsigned int width;
    unsigned int height;
          CmThreadSpaceEx(const CmThreadSpaceEx& that);
  //prohibit assignment operator
  CmThreadSpaceEx& operator=(const CmThreadSpaceEx&);
  };
};