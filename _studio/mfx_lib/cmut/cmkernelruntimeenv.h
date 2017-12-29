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

#include "string.h"
#include "cmrtex.h"
#include "infrastructure.h"
#include "cmassert.h"
#include "map"
using namespace std;

// FIXME: how to disallow the situation y = 0, z ! = 0, the below declaration will impact CmSurface<ty, size>
//template<typename ty, unsigned int x, unsigned int z>
//struct CmSurface<ty, x, 0, z>;
template<typename ty, unsigned int x, unsigned int y = 0, unsigned int z = 0>
struct CmSurface
{
  typedef ty elementTy;
  enum { elementSize = x * (y == 0 ? 1 : y) * (z == 0 ? 1 : z) };
};

class CmAbstractArg
{
protected:
  CmAbstractArg(const CmAbstractArg &);
  CmAbstractArg & operator = (const CmAbstractArg &);
public:
  CmAbstractArg() {}
  virtual ~CmAbstractArg() = 0;
  virtual void Set(CmDeviceEx & deviceEx, CmKernelEx & kernelEx) = 0;
  virtual void Unset() = 0;
  virtual int DataSize() const = 0;
  virtual void ReadData (unsigned char * pData) const = 0;
};

inline CmAbstractArg::~CmAbstractArg() {}

template<unsigned int id, typename ty>
class CmKernelArg : public CmAbstractArg
{
public:
  CmKernelArg(const ty & v) : value (v) {}

  virtual void Set(CmDeviceEx & deviceEx, CmKernelEx & kernelEx)
  {
    kernelEx.SetKernelArg (id, value);
  }
  virtual void Unset()
  {
  }
  virtual int DataSize() const
  {
    return sizeof(ty);
  }

  void ReadData (unsigned char * pData) const
  {
    *((ty *)pData) = value;
  }

public:
  ty value;
};

template<unsigned int id, typename ty, unsigned int size>
class CmKernelArg<id, CmSurface<ty, size>> : public CmAbstractArg
{
public:
  typedef CmSurface<ty, size> surfaceTy;

  template<typename ty2> CmKernelArg(const ty2 * pData) : dataView(pData), pBufferEx(NULL) {}
  template<typename ty2> CmKernelArg(ty2 data) : dataView(data), pBufferEx(NULL) {}

  ~CmKernelArg()
  {
    delete pBufferEx;
  }

  virtual void Set(CmDeviceEx & deviceEx, CmKernelEx & kernelEx)
  {
    assert(pBufferEx == NULL);
    if (pBufferEx != NULL) {
      throw CMUT_EXCEPTION ("pBufferEx != NULL");
    }

    pBufferEx = new CmBufferEx (deviceEx, (const unsigned char *)dataView, size * sizeof (ty));
    kernelEx.SetKernelArg (id, *pBufferEx);
  }

  virtual void Unset()
  {
    delete pBufferEx;
    pBufferEx = NULL;
  }

  void ReadData (unsigned char * pData) const
  {
    assert(pBufferEx != NULL);
    if (pBufferEx == NULL) {
      throw CMUT_EXCEPTION ("pBufferEx == NULL");
    }
    pBufferEx->Read (pData);
  }

  virtual int DataSize() const
  {
    return sizeof(ty) * size;
  }

protected:
  data_view<ty, size> dataView; // Assist for data conversion
  CmBufferEx * pBufferEx;
};

template<unsigned int id, typename ty, unsigned int width, unsigned height>
class CmKernelArg<id, CmSurface<ty, width, height>> : public CmAbstractArg
{
public:
  typedef CmSurface<ty, width, height> surfaceTy;

  template<typename ty2> CmKernelArg(ty2 * pData) : dataView (pData), pSurfaceEx(NULL) {}
  template<typename ty2> CmKernelArg(ty2 data) : dataView (data), pSurfaceEx(NULL) {}

  ~CmKernelArg()
  {
    delete pSurfaceEx;
  }

  virtual void Set(CmDeviceEx & deviceEx, CmKernelEx & kernelEx)
  {
    assert(pSurfaceEx == NULL);
    if (pSurfaceEx != NULL) {
      throw CMUT_EXCEPTION ("pSurfaceEx != NULL");
    }

    pSurfaceEx = new CmSurface2DEx (deviceEx, (const unsigned char *)dataView, width * sizeof (ty), height, CM_SURFACE_FORMAT_A8);
    kernelEx.SetKernelArg (id, *pSurfaceEx);
  }

  virtual void Unset()
  {
    delete pSurfaceEx;
    pSurfaceEx = NULL;
  }

  void ReadData (unsigned char * pData) const
  {
    assert(pSurfaceEx != NULL);
    if (pSurfaceEx == NULL) {
      throw CMUT_EXCEPTION ("pSurfaceEx == NULL");
    }
    pSurfaceEx->Read (pData);
  }

  virtual int DataSize() const
  {
    return sizeof(ty) * width * height;
  }

public:
  data_view<ty, width * height> dataView;
  CmSurface2DEx * pSurfaceEx;
};

class CmKernelExecutionResult
{
protected:
  CmKernelExecutionResult & operator = (const CmKernelExecutionResult &);

public:
  CmKernelExecutionResult(string kernelName, int resultsSize)
  {
    this->kernelName  = kernelName;
    this->resultsSize = resultsSize;
    this->pResults    = new unsigned char[resultsSize];
  }

  CmKernelExecutionResult(const CmKernelExecutionResult & other)
  {
    this->kernelName  = other.kernelName;
    this->resultsSize = other.resultsSize;
    this->pResults    = new unsigned char[resultsSize];
    memcpy(this->pResults, other.pResults, this->resultsSize);
  }

  ~CmKernelExecutionResult()
  {
    delete []pResults;
  }

  unsigned char * Results() { return pResults; }

  template<typename ty, unsigned int size, typename ty2>
  CmKernelExecutionResult & AssertEqual(const ty2 * pExpects)
  {
    assert(size > 0 && size <= resultsSize);
    assert(pExpects != NULL);

    try {
      CmAssertEqual<ty, size> (pExpects).Assert(pResults);

      return *this;
    } catch(std::exception & exp) {
      cout << kernelName.c_str() << endl;
      throw exp;
    }
  }

protected:
  string kernelName;
  int resultsSize;
  unsigned char * pResults;
};

class CmKernelRuntimeEnv
{
protected:
  CmKernelRuntimeEnv(const CmKernelRuntimeEnv &);
  CmKernelRuntimeEnv & operator = (const CmKernelRuntimeEnv &);

public:
  explicit CmKernelRuntimeEnv()
  {
  }

#ifdef CMRT_EMU
  explicit CmKernelRuntimeEnv(const string & isaFileName)
  {
    this->pFunction   = NULL;
    this->isaFileName = isaFileName;
  }

  template<typename kernelFunctionTy>
  explicit CmKernelRuntimeEnv(const string & isaFileName, const char * pKernelName, kernelFunctionTy kernelFunction)
  {
    this->kernelName  = pKernelName;
    this->pFunction   = (const void *)kernelFunction;
    this->isaFileName = isaFileName;
  }

  template<typename kernelFunctionTy>
  CmKernelRuntimeEnv & KernelFunction(const char * pKernelName, kernelFunctionTy kernelFunction)
  {
    this->kernelName  = pKernelName;
    this->pFunction   = (const void *)kernelFunction;
    return *this;
  }
#else
  explicit CmKernelRuntimeEnv(const string & isaFileName)
  {
    this->isaFileName = isaFileName;
  }

  explicit CmKernelRuntimeEnv(const string & isaFileName, const char * pKernelName)
  {
    this->kernelName  = pKernelName;
    this->isaFileName = isaFileName;
  }

  CmKernelRuntimeEnv & KernelFunction(const char * pKernelName)
  {
    this->kernelName  = pKernelName;
    return *this;
  }
#endif

  ~CmKernelRuntimeEnv()
  {
    for (argsTy::iterator it = args.begin(); it != args.end(); ++it) {
      delete (*it).second;
    }
  }

  CmKernelRuntimeEnv & IsaFile(const string & isaFileName)
  {
    this->isaFileName = isaFileName;
    return *this;
  }

  template<int argIndex, typename argTy, typename valueTy>
  CmKernelRuntimeEnv & KernelArg(valueTy value)
  {
    if (args.find(argIndex) != args.end()) {
      delete args[argIndex];
    }

    args[argIndex] = new CmKernelArg<argIndex, argTy> (value);
    return *this;
  }

  CmKernelExecutionResult Execute()
  {
    CmDeviceEx deviceEx(isaFileName.c_str ());
#ifdef CMRT_EMU
    CmKernelEx kernelEx(deviceEx, kernelName.c_str (), pFunction);
#else
    CmKernelEx kernelEx(deviceEx, kernelName.c_str ());
#endif
    // TODO validate whether args has correct sequence
    kernelEx.SetThreadCount (1);

    for (argsTy::iterator it = args.begin(); it != args.end(); ++it) {
      (*it).second->Set(deviceEx, kernelEx);
    }

    CmQueueEx queueEx (deviceEx);
    queueEx.Enqueue (kernelEx);
    queueEx.WaitForLastKernel ();

    // Default destination is arg0
    CmKernelExecutionResult result(kernelName, args[0]->DataSize());

    args[0]->ReadData (result.Results());

    for (argsTy::iterator it = args.begin(); it != args.end(); ++it) {
      (*it).second->Unset();
    }

    return result;
  }

protected:
  typedef std::map<int, CmAbstractArg *> argsTy;
  argsTy args;

  string isaFileName;
  string kernelName;
#ifdef CMRT_EMU
  const void * pFunction;
#endif
};
