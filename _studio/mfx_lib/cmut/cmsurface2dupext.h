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

namespace mdfut {
  template<typename T>
  class CmSurface2DUPExT
  {
  public:
    CmSurface2DUPExT(CmDeviceEx & deviceEx, unsigned int width, unsigned int height, CM_SURFACE_FORMAT format)
      : malloced(false), pitchWidth(0), deviceEx(deviceEx), format(format), initHeight(height), height(height), pData(NULL)
    {
  #ifdef _DEBUG
      std::cout << "CmSurface2DUPEx (" << width << ", " << height << ")" << std::endl;
  #endif

      assert (format == CM_SURFACE_FORMAT_A8R8G8B8 || format == CM_SURFACE_FORMAT_A8);
      if (format != CM_SURFACE_FORMAT_A8R8G8B8 && format != CM_SURFACE_FORMAT_A8) {
        throw MDFUT_EXCEPTION("fail");
      }

      int result = deviceEx->GetSurface2DInfo (width, height, format, pitchWidth, physicalSize);
      MDFUT_FAIL_IF(result != CM_SUCCESS, result);

      pData = (T *)CM_ALIGNED_MALLOC (physicalSize, 0x1000);
      if (pData == NULL) {
        throw MDFUT_EXCEPTION2("fail", std::bad_alloc);
      }
      malloced = true;

      pCmSurface2D = deviceEx.CreateSurface2DUP ((unsigned char *)pData, width, height, format);

      pitchWidth /= sizeof(T);
      switch (format) {
        case CM_SURFACE_FORMAT_A8R8G8B8:
          initWidth = this->width = (width * 4) / sizeof(T);
          break;
        case CM_SURFACE_FORMAT_A8:
          initWidth = this->width = width / sizeof(T);
          break;
        default:
          throw MDFUT_EXCEPTION("fail");
          break;
      }
    }

    ~CmSurface2DUPExT()
    {
      if (pCmSurface2D != NULL) {
        deviceEx->DestroySurface2DUP(pCmSurface2D);
      }

      if (malloced) {
        CM_ALIGNED_FREE(pData);
      }
    }

    void Resize(unsigned int width, unsigned int height)
    {
      if (height > initHeight) {
        throw MDFUT_EXCEPTION("fail");
      }

      assert (format == CM_SURFACE_FORMAT_A8R8G8B8 || format == CM_SURFACE_FORMAT_A8);

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

      if (this->width > initWidth) {
        throw MDFUT_EXCEPTION("fail");
      }
    }

    void Write(void * pSysMem)
    {
      assert (pSysMem != NULL);
      if (pSysMem == NULL) {
        throw MDFUT_EXCEPTION("fail");
      }

      for (int i = 0; i < Height (); ++i) {
        memcpy_s (DataPtr (i), Width () * sizeof(T), (T *)pSysMem + i * Width (), Width () * sizeof(T));
      }
    }

    operator CmSurface2DUP & () { return *pCmSurface2D; }
    CmSurface2DUP * operator -> () { return pCmSurface2D; }

    const unsigned int Pitch() const { return pitchWidth * sizeof(T); }
    const unsigned int Height() const { return height; }
    const unsigned int Width() const { return width; }

    T & Data(int id)
    {
      return pData[(id / width * pitchWidth) + (id % width)];
    }
    T & Data(int row, int col)
    {
      return pData[row * pitchWidth + col];
    }
    T * DataPtr() const { return pData; }
    T * DataPtr(int row) const { return pData + row * pitchWidth; }

  protected:
    CmSurface2DUP * pCmSurface2D;
    CmDeviceEx & deviceEx;

    T * pData;

    unsigned int initWidth;// in elements
    unsigned int initHeight;

    unsigned int pitchWidth;// in elements
    unsigned int width;// in elements
    unsigned int physicalSize;
    unsigned int height;
    const CM_SURFACE_FORMAT format;

    bool malloced;
  };
};