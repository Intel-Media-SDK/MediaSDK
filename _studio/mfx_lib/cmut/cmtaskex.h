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
#include "cmdeviceex.h"
#include "cmkernelex.h"
#include "vector"

namespace mdfut {
  class CmTaskEx
  {
  public:
    CmTaskEx(CmDeviceEx & deviceEx)
      : deviceEx(deviceEx)
    {
      int result = deviceEx->CreateTask(pTask);
      result;
      MDFUT_FAIL_IF(result != CM_SUCCESS, result);
    }

    CmTaskEx(CmDeviceEx & deviceEx, CmKernelEx & kernelEx)
      : deviceEx(deviceEx)
    {
      int result = deviceEx->CreateTask(pTask);
      result;
      MDFUT_FAIL_IF(result != CM_SUCCESS, result);

      AddKernel(kernelEx);
    }

#if __INTEL_MDF > CM_3_0
    CmTaskEx(CmDeviceEx & deviceEx, const std::vector<CmKernelEx *> & kernelExs, DWORD syncs)
      : deviceEx(deviceEx)
    {
      int result = deviceEx->CreateTask(pTask);
      MDFUT_FAIL_IF(result != CM_SUCCESS, result);

      for (int i = 0; i < kernelExs.size(); ++i) {
        AddKernel(*kernelExs[i]);

        if (i != kernelExs.size() - 1 && (syncs & (1 << i)) != 0) {
          int result = pTask->AddSync();
          MDFUT_FAIL_IF(result != CM_SUCCESS, result);
        }
      }
    }
#endif

    ~CmTaskEx()
    {
      if (pTask != NULL) {
        deviceEx->DestroyTask(pTask);
      }
    }

    void AddKernel(CmKernelEx & kernelEx)
    {
      int result = pTask->AddKernel(kernelEx);
      result;
      MDFUT_FAIL_IF(result != CM_SUCCESS, result);

      kernelExs.push_back(&kernelEx);
    }

    CmTask * operator ->() { return pTask; }
    operator CmTask *() { return pTask; }

    const std::vector<CmKernelEx *> & KernelExs() const { return kernelExs; }

  protected:
    CmTask * pTask;
    CmDeviceEx & deviceEx;
    std::vector<CmKernelEx *> kernelExs;

      CmTaskEx(const CmTaskEx& that);
  //prohibit assignment operator
  CmTaskEx& operator=(const CmTaskEx&);
  };
};