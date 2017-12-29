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
#include "iostream"

namespace mdfut {
  template<typename T>
  class CmSurface2DExT
  {
  public:
    CmSurface2DExT(CmDeviceEx & deviceEx, unsigned int width, unsigned int height, CM_SURFACE_FORMAT format)
      : format(format), deviceEx(deviceEx), height(height), malloced(false)
    {
  #ifdef _DEBUG
      std::cout << "CmSurface2DExT (" << width << ", " << height << ")" << std::endl;
  #endif
      Construct(width, height, format);
    }

    void Construct(unsigned int width, unsigned int height, CM_SURFACE_FORMAT format)
    {
      assert (format == CM_SURFACE_FORMAT_A8R8G8B8 || format == CM_SURFACE_FORMAT_A8);
      if (format != CM_SURFACE_FORMAT_A8R8G8B8 && format != CM_SURFACE_FORMAT_A8) {
        throw MDFUT_EXCEPTION("fail");
      }

      pCmSurface2D = deviceEx.CreateSurface2D(width, height, format);

      this->format = format;
      this->height = height;
      switch (format) {
        case CM_SURFACE_FORMAT_A8R8G8B8:
          this->width = (width * 4) / sizeof(T);
          break;
        case CM_SURFACE_FORMAT_A8:
          this->width = width / sizeof(T);
          break;
        default:
          throw MDFUT_EXCEPTION("fail");
          break;
      }

      pData = new T[width * height];
      malloced = true;
    }

    void Destruct()
    {
      if (pCmSurface2D != NULL) {
        deviceEx->DestroySurface(pCmSurface2D);
        pCmSurface2D = NULL;
      }

      if (malloced) {
        delete []pData;
        pData = NULL;
        malloced = false;
      }
    }
    //CmSurface2DExT (CmDeviceEx & deviceEx, const unsigned char * pData, unsigned int width, unsigned int height, CM_SURFACE_FORMAT format);
    ~CmSurface2DExT()
    {
      Destruct ();
    }

    operator CmSurface2D & () { return *pCmSurface2D; }
    CmSurface2D * operator -> () { return pCmSurface2D; }

    void Read(void * pSysMem, CmEvent* pEvent = NULL)
    {
      assert (pSysMem != NULL);
      if (pSysMem == NULL) {
        throw MDFUT_EXCEPTION("fail");
      }

      int result = pCmSurface2D->ReadSurface((unsigned char *)pSysMem, pEvent);
      MDFUT_FAIL_IF(result != CM_SUCCESS, result);
    }

    //void Read (void * pSysMem, const unsigned int stride, CmEvent* pEvent = NULL);

    void Write(void * pSysMem)
    {
      assert (pSysMem != NULL);
      if (pSysMem == NULL) {
        throw MDFUT_EXCEPTION("fail");
      }

      int result = pCmSurface2D->WriteSurface((const unsigned char *)pSysMem, NULL);
      MDFUT_FAIL_IF(result != CM_SUCCESS, result);
    }

    void Resize(unsigned int width, unsigned int height)
    {
      Destruct();

      Construct(width, height, format);
    }

    T & Data(int id)
    {
      return pData[id];
    }
    T & Data(int row, int col)
    {
      return pData[row * width + col];
    }

    T * DataPtr() const { return pData; }
    T * DataPtr(int row) const { return pData + row * width; }

    void CommitWrite()
    {
      this->Write(pData);
    }

    void CommitRead()
    {
      this->Read(pData);
    }

    const unsigned int Height () const { return height; }
    const unsigned int Width () const { return width; }
    const unsigned int Pitch () const { return width * sizeof(T); }

  protected:
    CmSurface2D * pCmSurface2D;
    CmDeviceEx & deviceEx;

    T * pData;
    bool malloced;

    unsigned int width;// in elements
    unsigned int height;
    CM_SURFACE_FORMAT format;
  };
};