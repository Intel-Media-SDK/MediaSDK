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
#include "cmutility.h"
#include "cmassert.h"
#include "iostream"
#include "fstream"
#include "vector"
using namespace std;

namespace mdfut {
  class CmDeviceEx
  {
    friend class CmKernelEx;
    friend class CmBufferEx;
    friend class CmBufferUPEx;
    friend class CmQueueEx;
    friend class CmSurface2DEx;
    friend class CmSurface2DUPEx;
    template<typename T>
    friend class CmSurface2DUPExT;
    template<typename T>
    friend class CmSurface2DExT;

  public:

    CmDeviceEx(const unsigned char * programm, int size, CmDevice  *pCmDevice,  mfxHandleType _mfxDeviceType, mfxHDL _mfxDeviceHdl, bool jit = false)
    {
      pCmDevice;
      mfxDeviceType = _mfxDeviceType;
      mfxDeviceHdl = _mfxDeviceHdl;
      MDFUT_ASSERT(programm != NULL && size > 0);

      UINT version = 0;
      int result = CM_FAILURE;//CreateCmDevice(pDevice, version);

#if defined(LINUX32) || defined (LINUX64)
      if(MFX_HANDLE_VA_DISPLAY == mfxDeviceType)
        result = CreateCmDevice(pDevice,version, (VADisplay) mfxDeviceHdl);
#endif

      if ((CM_SUCCESS != result) || !pDevice)
      {
        throw MDFUT_EXCEPTION("Cannot create CM device\n");
      }

      CmProgram * pProgram = NULL;
      if(jit)
        result = ::ReadProgramJit(pDevice, pProgram, programm, size);
      else
        result = ::ReadProgram(pDevice, pProgram, programm, size);

      if ((CM_SUCCESS != result) || !pProgram)
      {
        throw MDFUT_EXCEPTION("Cannot read CM program\n");
      }

      this->programs.push_back(pProgram);
    }

    CmDeviceEx(const char * pIsaFileName, mfxHandleType _mfxDeviceType, mfxHDL _mfxDeviceHdl)
    {
        mfxDeviceType = _mfxDeviceType;
        mfxDeviceHdl = _mfxDeviceHdl;
        UINT version = 0;

        int result = CM_FAILURE;//CreateCmDevice(pDevice, version);
#if defined(LINUX32) || defined (LINUX64)
        if(MFX_HANDLE_VA_DISPLAY == mfxDeviceType)
            result = CreateCmDevice(pDevice,version, (VADisplay) mfxDeviceHdl);
#endif
      MDFUT_FAIL_IF(result != CM_SUCCESS || pDevice == NULL, result);

      AddProgram(pIsaFileName);
    }

    CmDeviceEx(const char * pIsaFileNames[], int size, CmDevice  *pCmDevice, mfxHandleType _mfxDeviceType, mfxHDL _mfxDeviceHdl)
    {
      MDFUT_ASSERT(pIsaFileNames != NULL && size > 0);
      mfxDeviceType = _mfxDeviceType;
      mfxDeviceHdl = _mfxDeviceHdl;
      if ( ! pCmDevice )
      {
      int result = CM_FAILURE;//CreateCmDevice(pDevice, version);
      UINT version = 0;
#if defined(LINUX32) || defined (LINUX64)
        if(MFX_HANDLE_VA_DISPLAY == mfxDeviceType)
            result = CreateCmDevice(pDevice,version, (VADisplay) mfxDeviceHdl);
#endif
      MDFUT_FAIL_IF(result != CM_SUCCESS || pDevice == NULL, result);
      }
      else
      {
         pDevice = pCmDevice;
      }
      for (int i = 0; i < size; ++i) {
        AddProgram(pIsaFileNames[i]);
      }
    }

    ~CmDeviceEx()
    {
        for (unsigned int i = 0; i < programs.size(); i++) {
           pDevice->DestroyProgram(programs[i]);
        }

      DestroyCmDevice(pDevice);
    }

    void AddProgram(const char * pIsaFileName)
    {
      MDFUT_ASSERT_MESSAGE(pIsaFileName != NULL, "pIsaFileName is NULL")
      MDFUT_ASSERT_MESSAGE(pDevice != NULL, "pDevice is NULL")

  #ifdef CMRT_EMU
      int codeSize = sizeof(int);
      void * pCommonIsaCode = new BYTE[codeSize];
  #else
      ifstream is;
      is.open(pIsaFileName, ios::binary);
      if (!is.is_open()) {
        throw MDFUT_EXCEPTION("fail to open isa file\n");
      }

      is.seekg(0, ios::end);
      int codeSize = (int)is.tellg();
      is.seekg(0, ios::beg);
      if (codeSize == 0) {
        throw MDFUT_EXCEPTION("isa file has size 0\n");
      }

      BYTE * pCommonIsaCode = new BYTE[codeSize];

      is.read((char *)pCommonIsaCode, codeSize);
      is.close();
  #endif

      CmProgram * pProgram = NULL;
      INT result = pDevice->LoadProgram(pCommonIsaCode, codeSize, pProgram);
      result;
      delete[] pCommonIsaCode;

      MDFUT_FAIL_IF(result != CM_SUCCESS || pProgram == NULL, result);

      this->programs.push_back(pProgram);
    }

    CmDevice * operator -> () { return pDevice; }

    CmDevice * GetDevice() { return pDevice; }

    void GetCaps(CM_DEVICE_CAP_NAME capName, size_t & capValueSize, void * pCapValue)
    {
      int result = pDevice->GetCaps(capName, capValueSize, pCapValue);
      result;
      MDFUT_FAIL_IF(result != CM_SUCCESS, result);
    }

    CmProgram * Program(int i)
    {
      return programs[i];
    }

    void CreateThreadSpace(UINT width, UINT height, CmThreadSpace* & pThreadSpace)
    {
      int result = pDevice->CreateThreadSpace(width, height, pThreadSpace);

      result;
      MDFUT_FAIL_IF(result != CM_SUCCESS || pThreadSpace == NULL, result);
    }

  protected:
    CmSurface2D * CreateSurface2D(UINT width, UINT height, CM_SURFACE_FORMAT format)
    {
      CmSurface2D * pSurface = NULL;

      int result = pDevice->CreateSurface2D(width, height, format, pSurface);
      result;
      MDFUT_FAIL_IF(result != CM_SUCCESS || pSurface == NULL, result);

      return pSurface;
    }

    CmBuffer * CreateBuffer(UINT size)
    {
      CmBuffer * pBuffer = NULL;

      int result = pDevice->CreateBuffer (size, pBuffer);
      result;
      MDFUT_FAIL_IF(result != CM_SUCCESS || pBuffer == NULL, result);

      return pBuffer;
    }

    CmBufferUP * CreateBufferUP(const unsigned char * pData, UINT size)
    {
      CmBufferUP * pBuffer = NULL;

      INT result = pDevice->CreateBufferUP(size, (void *)pData, pBuffer);
      result;
      MDFUT_FAIL_IF(result != CM_SUCCESS || pBuffer == NULL, result);

      return pBuffer;
    }

    CmSurface2DUP * CreateSurface2DUP(const unsigned char * pData, UINT width, UINT height, CM_SURFACE_FORMAT format)
    {
      CmSurface2DUP * pSurface = NULL;

      INT result = pDevice->CreateSurface2DUP(width, height, format, (void *)pData, pSurface);
      result;
      MDFUT_FAIL_IF(result != CM_SUCCESS || pSurface == NULL, result);

      return pSurface;
    }

  #ifdef CMRT_EMU
    CmKernel * CreateKernel(const char * pKernelName, const void * fncPnt)
    {
      CmKernel * pKernel = NULL;

      for (auto program : programs) {
        INT result = pDevice->CreateKernel(program, pKernelName, fncPnt, pKernel);
        if (result == CM_SUCCESS) {
          return pKernel;
        }
      }

      throw MDFUT_EXCEPTION("fail");
    }
  #else
    CmKernel * CreateKernel(const char * pKernelName)
    {
      CmKernel * pKernel = NULL;

      for (unsigned int i = 0; i < programs.size(); i++) {
        int result = pDevice->CreateKernel(programs[i], pKernelName, pKernel);
        if (result == CM_SUCCESS) {
          return pKernel;
        }
      }

      throw MDFUT_EXCEPTION("fail");
    }
  #endif

    void DestroyKernel(CmKernel * pKernel)
    {
      pDevice->DestroyKernel(pKernel);
    }

  protected:
    CmDevice * pDevice;
    std::vector<CmProgram *> programs;
    mfxHandleType mfxDeviceType;
    mfxHDL        mfxDeviceHdl;
  };
};

#if defined MDFUT_PERF && !defined(__GNUC__)
#include "cmbenchmark.h"

namespace mdfut {
  namespace benchmark {
    MDFUT_ASPECT_CUT_START(CmDeviceEx, BenchmarkAspect)
      CmDeviceEx(const char * pIsaFileName) : CuttedCls(pIsaFileName) {}
      CmDeviceEx(const char * pIsaFileNames[], int size) : CuttedCls(pIsaFileNames, size) {}

      MDFUT_ASPECT_CUT_FUNCTION_1(CreateBuffer, UINT);
    MDFUT_ASPECT_CUT_END
  };
};

#define CmDeviceEx mdfut::benchmark::CmDeviceEx

#endif