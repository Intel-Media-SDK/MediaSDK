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
#include "cmtaskex.h"
#include "cmkernelex.h"
#include "cmassert.h"
#include "vector"
#include "string"

namespace mdfut {
  class CmQueueEx
  {
  public:
    CmQueueEx(CmDeviceEx & deviceEx)
      : deviceEx(deviceEx)
    {
      INT result = deviceEx->CreateQueue(pQueue);
      result;
      MDFUT_FAIL_IF(result != CM_SUCCESS, result);
    }

    ~CmQueueEx()
    {
      for (unsigned int i = 0; i < events.size(); i++) {
        pQueue->DestroyEvent(events[i]);
      }
    }

    void Enqueue(CmTask * pTask, CmThreadSpace * pThreadSpace = NULL)
    {
      CmEvent * pEvent = NULL;

      INT result = pQueue->Enqueue(pTask, pEvent, pThreadSpace);
      result;
      MDFUT_FAIL_IF(result != CM_SUCCESS, result);

      events.push_back(pEvent);
    }

    void Enqueue(CmTaskEx & taskEx, CmThreadSpace * pThreadSpace = NULL)
    {
      Enqueue((CmTask *)taskEx, pThreadSpace);
    }

    void Enqueue(CmKernelEx & kernelEx, CmThreadSpace * pThreadSpace = NULL)
    {
      CmTaskEx taskEx(deviceEx, kernelEx);
      Enqueue(taskEx, pThreadSpace);
    }

#if __INTEL_MDF > CM_3_0
    void Enqueue(std::vector<CmKernelEx *> kernelExs, DWORD syncs, CmThreadSpace * pThreadSpace = NULL)
    {
      CmTaskEx taskEx(deviceEx, kernelExs, syncs);
      Enqueue(taskEx, pThreadSpace);
    }
#endif

    void WaitForFinished(UINT64 * pExecutionTime = NULL)
    {
      INT result = LastEvent()->WaitForTaskFinished(CM_MAX_TIMEOUT_MS);
      result;
      MDFUT_FAIL_IF(result != CM_SUCCESS, result);

      UINT64 time, totalTime = 0;
      for (unsigned int i = 0; i < events.size(); i++) {

        // if requested, accumulate kernel times before destroying events
        if (pExecutionTime) {
          events[i]->GetExecutionTime(time);
          totalTime += (time & 0xffffffff);
        }
        pQueue->DestroyEvent(events[i]);
      }
      events.clear();

      if (pExecutionTime) *pExecutionTime = totalTime;
    }

    CmEvent * LastEvent()
    {
      return *events.rbegin();
    }

    CmQueue * operator ->() { return pQueue; }

  protected:
    CmQueue * pQueue;
    CmDeviceEx & deviceEx;
    std::vector<CmEvent *> events;
          CmQueueEx(const CmQueueEx& that);
  //prohibit assignment operator
  CmQueueEx& operator=(const CmQueueEx&);
  };
};
