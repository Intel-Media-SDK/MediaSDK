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

#include "cm/cm.h"
#include "hash_map"
#include "string"
#include "iostream"
#include "utility"

//template<typename streamTy>
//struct StreamTraits;
//
//template<typename dataTy, unsigned int size>
//struct StreamTraits<vector_ref<dataTy, size>>
//{
//  typedef dataTy dataTy_;
//  enum { size_ = size };
//};
//
//template<typename dataTy, unsigned int size>
//struct StreamTraits<vector<dataTy, size>>
//{
//  typedef dataTy dataTy_;
//  enum { size_ = size };
//};
//
//template<typename dataTy, unsigned int rows, unsigned int cols>
//struct StreamTraits<matrix_ref<dataTy, rows, cols>>
//{
//  typedef dataTy dataTy_;
//  enum { rows_ = rows, cols_ = cols };
//};
//
//template<typename dataTy, unsigned int rows, unsigned int cols>
//struct StreamTraits<matrix<dataTy, rows, cols>>
//{
//  typedef dataTy dataTy_;
//  enum { rows_ = rows, cols_ = cols };
//};

class CmKernelEx;
class CmProfiler
{
public:
  CmProfiler ();
  ~CmProfiler ();

#if defined CM_MEASURE_PERFORMANCE && defined CMRT_EMU
  static CmProfiler * & Current ();

  template<typename dataTy, unsigned int size, template<typename, unsigned int> class vectorTy>
  bool KernelRead (SurfaceIndex & id, int x, vectorTy<dataTy, size> & v)
  {
    assert (pKernelName_ != nullptr);

    KernelStatistics & kernelStatistics = CreateKernelStatistics (pKernelName_);
    kernelStatistics.OwordBlockRead (RttiVector::TypeId (v));

    return ::read (id, x, v);
  }

  template<typename dataTy, unsigned int rows, unsigned int cols, template<typename, unsigned int, unsigned int> class matrixTy>
  bool KernelRead (SurfaceIndex & id, int x, int y, matrixTy<dataTy, rows, cols> & m)
  {
    assert (pKernelName_ != nullptr);

    KernelStatistics & kernelStatistics = CreateKernelStatistics (pKernelName_);
    kernelStatistics.MediaBlockRead (RttiMatrix::TypeId (m));

    return ::read (id, x, y, m);
  }

  template<typename dataTy, unsigned int size, template<typename, unsigned int> class vectorTy>
  bool KernelRead (SurfaceIndex & id, unsigned int globalOffset,
                   vectorTy<unsigned int, size> & elementOffset, vectorTy<dataTy, size> & v)
  {
    assert (pKernelName_ != nullptr);

    KernelStatistics & kernelStatistics = CreateKernelStatistics (pKernelName_);
    kernelStatistics.DwordScatteredRead (RttiVector::TypeId (v));

    return ::read (id, globalOffset, elementOffset, v);
  }

  //struct Rtti
  //{
  //};

  struct RttiMatrix/* : public Rtti*/
  {
    template<typename dataTy, unsigned int rows, unsigned int cols, template<typename, unsigned int, unsigned int> class matrixTy>
    static RttiMatrix & TypeId (matrixTy<dataTy, rows, cols> & m)
    {
      static RttiMatrix rtti (typeid (dataTy), sizeof (dataTy), rows, cols);
      return rtti;
    }

    RttiMatrix (const type_info & dataTy, unsigned int dataSize, unsigned int rows, unsigned int cols)
      : dataTy_ (dataTy), dataSize_ (dataSize), rows_ (rows), cols_ (cols)
    {
    }

    unsigned int Bytes () const { return dataSize_ * rows_ * cols_; }

    const type_info & dataTy_;
    unsigned int dataSize_;
    unsigned int rows_;
    unsigned int cols_;
  };

  struct RttiVector/* : public Rtti*/
  {
    template<typename dataTy, unsigned int size, template<typename, unsigned int> class vectorTy>
    static RttiVector & TypeId (vectorTy<dataTy, size> & v)
    {
      static RttiVector rtti (typeid (dataTy), sizeof (dataTy), size);
      return rtti;
    }

    RttiVector (const type_info & dataTy, unsigned int dataSize, unsigned int size)
      : dataTy_ (dataTy), dataSize_ (dataSize), size_ (size)
    {
    }

    unsigned int Bytes () const { return dataSize_ * size_; }

    const type_info & dataTy_;
    unsigned int dataSize_;
    unsigned int size_;
  };

  struct KernelStatistics
  {
    typedef stdext::hash_map<RttiMatrix *, unsigned int> mediaBlockReadStatisticsTy;
    typedef stdext::hash_map<RttiVector *, unsigned int> owordBlockReadStatisticsTy;
    typedef stdext::hash_map<RttiVector *, unsigned int> dwordScatteredReadStatisticsTy;
    mediaBlockReadStatisticsTy mediaBlockReadStatistics_;
    owordBlockReadStatisticsTy owordBlockReadStatistics_;
    dwordScatteredReadStatisticsTy dwordScatteredReadStatistics_;

    template<typename RttiTy, typename hashmapTy>
    void Read (RttiTy & rtti, hashmapTy & hashmap)
    {
      hashmapTy::iterator iter = hashmap.find (&rtti);
      if (iter == hashmap.end ()) {
        std::pair<hashmapTy::iterator, bool> insert = hashmap.insert (std::make_pair (&rtti, int (0)));
        iter = insert.first;
      }
      iter->second++;
    }

    void MediaBlockRead (RttiMatrix & rtti)
    {
      Read (rtti, mediaBlockReadStatistics_);
    }

    void OwordBlockRead (RttiVector & rtti)
    {
      Read (rtti, owordBlockReadStatistics_);
    }

    void DwordScatteredRead (RttiVector & rtti)
    {
      Read (rtti, dwordScatteredReadStatistics_);
    }

    unsigned int ReadCalls ()
    {
      unsigned int calls = 0;
      for (owordBlockReadStatisticsTy::iterator iter = owordBlockReadStatistics_.begin (); iter != owordBlockReadStatistics_.end (); ++iter) {
        calls += iter->second;
      }
      for (mediaBlockReadStatisticsTy::iterator iter = mediaBlockReadStatistics_.begin (); iter != mediaBlockReadStatistics_.end (); ++iter) {
        calls += iter->second;
      }
      for (dwordScatteredReadStatisticsTy::iterator iter = dwordScatteredReadStatistics_.begin (); iter != dwordScatteredReadStatistics_.end (); ++iter) {
        calls += iter->second;
      }
      return calls;
    }

    unsigned int ReadBytes ()
    {
      unsigned int bytes = 0;
      for (owordBlockReadStatisticsTy::iterator iter = owordBlockReadStatistics_.begin (); iter != owordBlockReadStatistics_.end (); ++iter) {
        bytes += iter->first->Bytes () * iter->second;
      }
      for (mediaBlockReadStatisticsTy::iterator iter = mediaBlockReadStatistics_.begin (); iter != mediaBlockReadStatistics_.end (); ++iter) {
        bytes += iter->first->Bytes () * iter->second;
      }
      for (dwordScatteredReadStatisticsTy::iterator iter = dwordScatteredReadStatistics_.begin (); iter != dwordScatteredReadStatistics_.end (); ++iter) {
        bytes += iter->first->Bytes () * iter->second;
      }
      return bytes;
    }
  };

  KernelStatistics & CreateKernelStatistics (const char * pKernelName)
  {
    std::string kernelName (pKernelName);

    kernelProfilesTy::iterator iter = kernelProfiles_.find (kernelName);
    if (iter == kernelProfiles_.end ()) {
      std::pair<kernelProfilesTy::iterator, bool> insert = kernelProfiles_.insert (std::make_pair (kernelName, KernelStatistics ()));
      if (!insert.second) {
        throw std::exception (__FUNCTION__ " fail");
      }

      iter = insert.first;
    }

    return iter->second;
  }

  typedef stdext::hash_map<std::string, KernelStatistics> kernelProfilesTy;
  kernelProfilesTy kernelProfiles_;
#endif

  void Statistics ();

#ifdef CM_MEASURE_PERFORMANCE
  void PreReadSurface ();
  void PostReadSurface ();
  void PreWriteSurface ();
  void PostWriteSurface ();
  void PreEnqueue (CmKernelEx & kernelEx);
  void PostEnqueue ();

  const char * pKernelName_;

#else
  void PreReadSurface () {}
  void PostReadSurface () {}
  void PreWriteSurface () {}
  void PostWriteSurface () {}
  void PreEnqueue (CmKernelEx & kernelEx) {}
  void PostEnqueue () {}
#endif

  Clock clockReadSurface_;
  Clock clockWriteSurface_;
};

#if defined CMRT_EMU && defined CM_MEASURE_PERFORMANCE
#define read(...) CmProfiler::Current ()->KernelRead (__VA_ARGS__)
#endif
