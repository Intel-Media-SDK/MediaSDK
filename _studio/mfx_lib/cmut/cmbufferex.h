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
#include "assert.h"

namespace mdfut {
  class CmBufferEx
  {
  public:
    CmBufferEx(CmDeviceEx & cmDeviceEx, int size)
      : cmDeviceEx(cmDeviceEx)
    {
  #ifdef _DEBUG
      cout << "CmBufferEx (" << size << ")" << endl;
  #endif
      pCmBuffer = cmDeviceEx.CreateBuffer(size);
    }

    CmBufferEx(CmDeviceEx & cmDeviceEx, const unsigned char * pData, int size)
      : cmDeviceEx(cmDeviceEx)
    {
  #ifdef _DEBUG
      cout << "CmBufferEx (" << size << ")" << endl;
  #endif
      assert(pData != NULL);

      pCmBuffer = cmDeviceEx.CreateBuffer(size);
      Write(pData);
    }

    ~CmBufferEx()
    {
      if (pCmBuffer != NULL) {
        cmDeviceEx->DestroySurface(pCmBuffer);
      }
    }

    operator CmBuffer & () { return *pCmBuffer; }
    CmBuffer * operator -> () { return pCmBuffer; }

    void Read(void * pSysMem, CmEvent * pEvent = NULL)
    {
      assert(pSysMem != NULL);

      int result = pCmBuffer->ReadSurface((unsigned char *)pSysMem, pEvent);
      result;
      MDFUT_FAIL_IF(result != CM_SUCCESS, result);
    }

    void Write(const unsigned char * pSysMem)
    {
      assert(pSysMem != NULL);
      int result = pCmBuffer->WriteSurface(pSysMem, NULL);
      result;
      MDFUT_FAIL_IF(result != CM_SUCCESS, result);
    }

  protected:
    CmBuffer * pCmBuffer;
    CmDeviceEx & cmDeviceEx;

private:
  //prohobit copy constructor
  CmBufferEx(const CmBufferEx& that);
  //prohibit assignment operator
  CmBufferEx& operator=(const CmBufferEx&);
  };

  class CmBufferUPEx
  {
  public:
    CmBufferUPEx(CmDeviceEx & cmDeviceEx, const unsigned char * pData, int size)
      : cmDeviceEx(cmDeviceEx)
    {
      assert (pData != NULL);

      pCmBuffer = cmDeviceEx.CreateBufferUP(pData, size);
    }

    ~CmBufferUPEx()
    {
      if (pCmBuffer != NULL) {
        cmDeviceEx->DestroyBufferUP(pCmBuffer);
      }
    }

    operator CmBufferUP & () { return *pCmBuffer; }
    CmBufferUP * operator -> () { return pCmBuffer; }

  protected:
    CmBufferUP * pCmBuffer;
    CmDeviceEx & cmDeviceEx;
private:
  //prohobit copy constructor
  CmBufferUPEx(const CmBufferUPEx& that);
  //prohibit assignment operator
  CmBufferUPEx& operator=(const CmBufferUPEx&);
  };

  class CmSurface2DEx
  {
  public:
    CmSurface2DEx(CmDeviceEx & cmDeviceEx, UINT width, UINT height, CM_SURFACE_FORMAT format)
        : cmDeviceEx(cmDeviceEx), isCreated(true)
    {
  #ifdef _DEBUG
      cout << "CmSurface2DEx (" << width << ", " << height << ")" << endl;
  #endif
      pCmSurface2D = cmDeviceEx.CreateSurface2D(width, height, format);
    }

    CmSurface2DEx(CmDeviceEx & cmDeviceEx, UINT width, UINT height, CM_SURFACE_FORMAT format, CmSurface2D* psurf)
        : cmDeviceEx(cmDeviceEx), isCreated(false)
    {
        format;
  #ifdef _DEBUG
        cout << "CmSurface2DEx (" << width << ", " << height << ")" << endl;
  #endif
        assert(psurf != NULL);
        pCmSurface2D = psurf;
    }

    CmSurface2DEx(CmDeviceEx & cmDeviceEx, const unsigned char * pData, UINT width, UINT height, CM_SURFACE_FORMAT format)
        : cmDeviceEx(cmDeviceEx), isCreated(true)
    {
  #ifdef _DEBUG
      cout << "CmSurface2DEx (" << width << ", " << height << ")" << endl;
  #endif
      assert(pData != NULL);

      pCmSurface2D = cmDeviceEx.CreateSurface2D(width, height, format);
      Write (pData);
    }

    ~CmSurface2DEx()
    {
      if (pCmSurface2D != NULL && isCreated) {
        cmDeviceEx->DestroySurface(pCmSurface2D);
      }
    }

    operator CmSurface2D & () { return *pCmSurface2D; }
    CmSurface2D * operator -> () { return pCmSurface2D; }

    void Read(void * pSysMem, CmEvent* pEvent = NULL)
    {
      assert (pSysMem != NULL);
      int result = pCmSurface2D->ReadSurface((unsigned char *)pSysMem, pEvent);
      result;
      MDFUT_FAIL_IF(result != CM_SUCCESS, result);
    }

    void Read(void * pSysMem, const UINT stride, CmEvent* pEvent = NULL)
    {
      assert (pSysMem != NULL);
      int result = pCmSurface2D->ReadSurfaceStride((unsigned char *)pSysMem, pEvent, stride);
      result;
      MDFUT_FAIL_IF(result != CM_SUCCESS, result);
    }

    void Write(const unsigned char * pSysMem)
    {
      assert (pSysMem != NULL);
      int result = pCmSurface2D->WriteSurface(pSysMem, NULL);
      result;
      MDFUT_FAIL_IF(result != CM_SUCCESS, result);
    }

  protected:
    CmSurface2D * pCmSurface2D;
    CmDeviceEx & cmDeviceEx;
    bool isCreated;
private:
  //prohobit copy constructor
  CmSurface2DEx(const CmSurface2DEx& that);
  //prohibit assignment operator
  CmSurface2DEx& operator=(const CmSurface2DEx&);
  };

  class CmSurface2DUPEx
  {
  public:
    CmSurface2DUPEx(CmDeviceEx & deviceEx, unsigned int width, unsigned int height, CM_SURFACE_FORMAT format)
      : deviceEx(deviceEx), pData(NULL), width(width), height(height),  pitch(0), format(format) , malloced(false)
    {
  #ifdef _DEBUG
      std::cout << "CmSurface2DUPEx (" << width << ", " << height << ")" << std::endl;
  #endif
      int result = deviceEx->GetSurface2DInfo(width, height, format, pitch, physicalSize);
      result;
      MDFUT_FAIL_IF(result != CM_SUCCESS, result);

      pData = CM_ALIGNED_MALLOC(physicalSize, 0x1000);
      if (pData == NULL) {
        // Can't throw a bad_alloc with a string in the constructor
        // Throw a derived variant instead
        throw MDFUT_EXCEPTION2("fail", cm_bad_alloc);
      }
      malloced = true;

      pCmSurface2D = deviceEx.CreateSurface2DUP((unsigned char *)pData, width, height, format);
    }

    ~CmSurface2DUPEx()
    {
      if (pCmSurface2D != NULL) {
        deviceEx->DestroySurface2DUP(pCmSurface2D);
      }

      if (malloced) {
        CM_ALIGNED_FREE(pData);
      }
    }

    operator CmSurface2DUP & () { return *pCmSurface2D; }
    CmSurface2DUP * operator -> () { return pCmSurface2D; }

    const unsigned int Pitch() const { return pitch; }
    const unsigned int Height() const { return height; }
    const unsigned int Width() const { return width; }

    void * DataPtr() const { return pData; }
    void * DataPtr(int row) const { return (unsigned char *)pData + row * pitch; }

   // void Read(void * pSysMem, CmEvent* pEvent = NULL)
   // {
   //   assert (pSysMem != NULL);
      //for (int row = 0; row < actualHeight(); row++)
      //    memcpy((char*)pSysMem + actualWidth() *row, DataPtr(row), actualWidth());
   // }deviceEx(deviceEx)

   // void Write(const unsigned char * pSysMem)
   // {
   //   assert (pSysMem != NULL);
      //for (int row = 0; row < actualHeight(); row++)
      //    memcpy(DataPtr(row), (char*)pSysMem + actualWidth()*row, actualWidth());
   // }

  protected:
    CmSurface2DUP * pCmSurface2D;
    CmDeviceEx & deviceEx;

    void * pData;

    unsigned int width;// in elements
    unsigned int height;
    unsigned int pitch;// in bytes
    unsigned int physicalSize;
    const CM_SURFACE_FORMAT format;

    bool malloced;

private:
  //prohobit copy constructor
  CmSurface2DUPEx(const CmSurface2DUPEx& that);
  //prohibit assignment operator
  CmSurface2DUPEx& operator=(const CmSurface2DUPEx&);
    //int actualWidth() const {
       // switch (format) {
       // case CM_SURFACE_FORMAT_A8R8G8B8:
       // case CM_SURFACE_FORMAT_X8R8G8B8:
       // case CM_SURFACE_FORMAT_R32F:
          //  return 4 * width;
       // case CM_SURFACE_FORMAT_V8U8:
       // case CM_SURFACE_FORMAT_R16_SINT:
       // case CM_SURFACE_FORMAT_UYVY:
       // case CM_SURFACE_FORMAT_YUY2:
          //  return 2 * width;
       // case CM_SURFACE_FORMAT_A8:
       // case CM_SURFACE_FORMAT_P8:
       // case CM_SURFACE_FORMAT_R8_UINT:
          //  return 1 * width;
       // case CM_SURFACE_FORMAT_NV12:
          //  return 1 * width;
       // }
       // throw MDFUT_EXCEPTION("Unkown Format Code");
    //}
    //int actualHeight() const {
       // switch (format) {
       // case CM_SURFACE_FORMAT_A8R8G8B8:
       // case CM_SURFACE_FORMAT_X8R8G8B8:
       // case CM_SURFACE_FORMAT_R32F:
          //  return 1 * height;
       // case CM_SURFACE_FORMAT_V8U8:
       // case CM_SURFACE_FORMAT_R16_SINT:
       // case CM_SURFACE_FORMAT_UYVY:
       // case CM_SURFACE_FORMAT_YUY2:
          //  return 1 * height;
       // case CM_SURFACE_FORMAT_A8:
       // case CM_SURFACE_FORMAT_P8:
       // case CM_SURFACE_FORMAT_R8_UINT:
          //  return 1 * height;
       // case CM_SURFACE_FORMAT_NV12:
          //  return 1.5 * height;
       // }
       // throw MDFUT_EXCEPTION("Unkown Format Code");
    //}

  };
};