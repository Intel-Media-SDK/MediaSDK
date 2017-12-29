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

#include "cminfrastructure.h"

#define INT2STRING2(i) #i
#define INT2STRING(i)  INT2STRING2(i)

#define MDFUT_EXCEPTION2(message, exception) exception();
#define MDFUT_EXCEPTION(message) exception();

#define MDFUT_FAIL_IF(condition, result)

#define MDFUT_ASSERT_EQUAL(expect, actual, message)  \
  if (expect != actual) {                           \
    throw MDFUT_EXCEPTION(message);                 \
  }
#define MDFUT_ASSERT(condition)                        \
  if (!(condition)) {                                 \
    throw MDFUT_EXCEPTION("Expect : " #condition);    \
  }
#define MDFUT_ASSERT_MESSAGE(condition, message)       \
  if (!(condition)) {                                 \
    throw MDFUT_EXCEPTION(message);                   \
  }


template<typename ty, unsigned int size>
class CmAssertEqual
{
public:
  typedef ty elementTy;
  enum { elementSize = size };
public:
  template<typename ty2>
  CmAssertEqual(const ty2 * pExpects) : dataView(pExpects)
  {
  }

  void Assert(const void * pData) const
  {
    const ty * pActuals = (const ty *)pData;

    for (unsigned int i = 0; i < size; ++i) {
      if (!IsEqual(dataView[i], pActuals[i])) {
        write<size>((const ty *)dataView, pActuals, i);

        throw MDFUT_EXCEPTION("Difference is detected");
      }
    }
  }

  template<typename ty2>
  bool IsEqual(ty2 x, ty2 y) const
  {
    return x == y;
  }

  bool IsEqual(float x, float y) const
  {
    return fabs(x - y) <= 1e-5;
  }

  bool IsEqual(double x, double y) const
  {
    return fabs(x - y) <= 1e-5;
  }

protected:
  data_view<ty, size> dataView;
};
