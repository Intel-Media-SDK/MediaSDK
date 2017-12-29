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
#include "cmassert.h"

namespace mdfut {
  class CmBufferEx;
  class CmBufferUPEx;
  class CmSurface2DEx;
  class CmSurface2DUPEx;

  class CmKernelEx
  {
  public:

  #ifdef CMRT_EMU
    template<typename kernelFunctionTy>
    CmKernelEx(CmDeviceEx & deviceEx, const char * pKernelName, kernelFunctionTy kernelFunction)
      : deviceEx(deviceEx), kernelName(pKernelName)
    {
  #ifdef _DEBUG
      cout << "CmKernelEx(\"" << pKernelName << "\")" << endl;
  #endif
      pKernel = deviceEx.CreateKernel(pKernelName, (const void *)kernelFunction);
    }
  #else
    CmKernelEx(CmDeviceEx & deviceEx, const char * pKernelName)
      : deviceEx(deviceEx), kernelName(pKernelName)
    {
  #ifdef _DEBUG
      cout << "CmKernelEx(\"" << pKernelName << "\")" << endl;
  #endif
      pKernel = deviceEx.CreateKernel(pKernelName);
    }
  #endif

    ~CmKernelEx()
    {
      deviceEx.DestroyKernel(pKernel);
    }

  public:
    CmKernel * operator ->() { return pKernel; }
    operator CmKernel *() { return pKernel; }

    void SetThreadCount (unsigned int threadCount)
    {
      int result = pKernel->SetThreadCount(threadCount);
      result;
      MDFUT_FAIL_IF(result != CM_SUCCESS, result);
    }

#if __INTEL_MDF > CM_3_0
    void SetThreadSpace(CmThreadSpace * pThreadSpace = NULL)
    {
      int result = pKernel->AssociateThreadSpace(pThreadSpace);
      MDFUT_FAIL_IF(result != CM_SUCCESS, result);
    }
#endif

    template<typename ArgTy>
    void SetKernelArg(UINT index, ArgTy arg)
    {
      int result = pKernel->SetKernelArg(index, sizeof(ArgTy), &arg);
      result;
      MDFUT_FAIL_IF(result != CM_SUCCESS, result);
    }

    template<typename ArgTy>
    void SetThreadArg(UINT threadId, UINT index, ArgTy arg)
    {
      int result = pKernel->SetThreadArg(threadId, index, sizeof(ArgTy), &arg);
      MDFUT_FAIL_IF(result != CM_SUCCESS, result);
    }

  #define SPECIALIZE_SET_ARG(SurfaceType) \
    void SetKernelArg(UINT index, SurfaceType##Ex & surface)  \
    { \
      SetKernelArg(index, (SurfaceType &)surface); \
    } \
    \
    void SetKernelArg(UINT index, SurfaceType & surface)  \
    { \
      SetKernelArgBySurface(index, surface); \
    } \
    \
    void SetThreadArg(UINT threadId, UINT index, SurfaceType##Ex & surface)  \
    { \
      SetThreadArg(threadId, index, (SurfaceType &)surface); \
    } \
    \
    void SetThreadArg(UINT threadId, UINT index, SurfaceType & surface)  \
    { \
      SetThreadArgBySurface(threadId, index, surface); \
    }

    SPECIALIZE_SET_ARG(CmBuffer)
    SPECIALIZE_SET_ARG(CmBufferUP)
    SPECIALIZE_SET_ARG(CmSurface2D)
    SPECIALIZE_SET_ARG(CmSurface2DUP)

    void SetKernelArg(UINT index, CmSurface3D & surface3D)
    {
      SetKernelArgBySurface (index, surface3D);
    }

    const std::string & KernelName() const { return kernelName; }

  protected:
    template<typename SurfaceTy>
    void SetKernelArgBySurface(UINT index, SurfaceTy & surface)
    {
      SurfaceIndex * pSurfaceIndex = NULL;
      int result = surface.GetIndex(pSurfaceIndex);
      MDFUT_FAIL_IF(result != CM_SUCCESS, result);

      result = pKernel->SetKernelArg(index, sizeof(SurfaceIndex), pSurfaceIndex);
      MDFUT_FAIL_IF(result != CM_SUCCESS, result);
    }

    template<typename SurfaceTy>
    void SetThreadArgBySurface(UINT threadId, UINT index, SurfaceTy & surface)
    {
      SurfaceIndex * pSurfaceIndex = NULL;
      int result = surface.GetIndex(pSurfaceIndex);
      MDFUT_FAIL_IF(result != CM_SUCCESS, result);

      result = pKernel->SetThreadArg(threadId, index, sizeof(SurfaceIndex), pSurfaceIndex);
      MDFUT_FAIL_IF(result != CM_SUCCESS, result);
    }

  protected:
    CmKernel * pKernel;
    CmDeviceEx & deviceEx;
    std::string kernelName;
  CmKernelEx(const CmKernelEx& that);
  //prohibit assignment operator
  CmKernelEx& operator=(const CmKernelEx&);
  };
};
