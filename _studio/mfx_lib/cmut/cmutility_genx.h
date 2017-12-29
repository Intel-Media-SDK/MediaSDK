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

template<int size, typename ty>
struct bytes_of
{
  enum
  {
    value = size * sizeof (ty),
  };
};

template<typename dataTy, unsigned int rows, unsigned int cols>
_GENX_ inline void cmk_read (SurfaceIndex surfaceId, unsigned int xRead, unsigned int yRead, matrix_ref<dataTy, rows, cols> m)
{
  enum { yInterval = 32 / sizeof (dataTy), xInterval = 32 / sizeof (dataTy) };

#pragma unroll
  for (unsigned int yOffset = 0; yOffset < rows; yOffset += 8) {
#pragma unroll
    for (unsigned int xOffset = 0; xOffset < cols; xOffset += 8) {
      read (surfaceId, xRead + xOffset, yRead + yOffset, m.template select<yInterval, 1, xInterval, 1> (yOffset, xOffset));
    }
  }
}

template<typename dataTy, unsigned int size>
inline _GENX_ void cmk_read (SurfaceIndex id, unsigned int globalOffset,
                             vector_ref<unsigned int, 8> elementOffset, vector_ref<dataTy, 8> v)
{
  read (id, globalOffset, elementOffset, v);
}

template<typename dataTy, unsigned int size>
inline _GENX_ void cmk_read (SurfaceIndex id, unsigned int globalOffset,
                             vector_ref<unsigned int, 16> elementOffset, vector_ref<dataTy, 16> v)
{
  read (id, globalOffset, elementOffset, v);
}

template<typename dataTy, unsigned int size>
inline _GENX_ void cmk_read (SurfaceIndex id, unsigned int globalOffset,
                             vector_ref<unsigned int, size> elementOffset, vector_ref<dataTy, size> v)
{
  read (id, globalOffset, elementOffset.template select<16, 1> (0), v.template select<16, 1> (0));
  cmk_read<dataTy, size - 16> (id, globalOffset, elementOffset.template select<size - 16, 1> (16), v.template select<size - 16, 1> (16));
}

template<typename dataTy, unsigned int rows, unsigned int cols>
_GENX_ inline void cmk_read (SurfaceIndex id, unsigned int globalOffset,
                             matrix_ref<unsigned int, 1, cols> elementOffset, matrix_ref<dataTy, 1, cols> m)
{
  cmk_read<dataTy, cols> (id, globalOffset, elementOffset.row (0), m.row (0));
}

template<typename dataTy, unsigned int rows, unsigned int cols>
_GENX_ inline void cmk_read (SurfaceIndex id, unsigned int globalOffset,
                             matrix_ref<unsigned int, rows, cols> elementOffset, matrix_ref<dataTy, rows, cols> m)
{
  cmk_read<dataTy, cols> (id, globalOffset, elementOffset.row (0), m.row (0));
  cmk_read<dataTy, rows - 1, cols> (id, globalOffset, elementOffset.template select<rows - 1, 1, cols, 1> (1, 0), m.template select<rows - 1, 1, cols, 1> (1, 0));
}


template<typename dataTy, unsigned int rows, unsigned int cols>
inline _GENX_ void cmk_row_increment (matrix_ref<dataTy, 2, cols> m, dataTy increment)
{
  m.row (1) = m.row (0) + increment;
}

template<typename dataTy, unsigned int rows, unsigned int cols>
inline _GENX_ void cmk_row_increment (matrix_ref<dataTy, rows, cols> m, dataTy increment)
{
  m.row (1) = m.row (0) + increment;
  cmk_row_increment<dataTy, rows - 1, cols> (m.template select<rows - 1, 1, cols, 1> (1, 0), increment);
}

template<bool>
struct cmk_stop
{
};

template<int>
struct cmk_int2type
{
};
// cmk_increase_by_cols, FIXME check whether size is power of 2
template<typename dataTy, unsigned int size, unsigned int selects>
_GENX_ inline void cmk_increase_by_cols (vector_ref<dataTy, size> v, dataTy increment, cmk_stop<false>)
{
  v.template select<selects, 1> (selects) = v.template select<selects, 1> (0) + increment;
  enum { stop = selects * 2 * 2 > size };
  cmk_increase_by_cols<dataTy, size, selects * 2> (v, increment * 2, cmk_stop<stop> ());
}

template<typename dataTy, unsigned int size, unsigned int selects>
_GENX_ inline void cmk_increase_by_cols (vector_ref<dataTy, size> v, dataTy increment, cmk_stop<true>)
{
}

template<typename dataTy, unsigned int size, unsigned int selects>
_GENX_ inline void cmk_increase_by_cols (vector_ref<dataTy, size> v, dataTy increment)
{
  enum { stop = selects * 2 > size };
  cmk_increase_by_cols<dataTy, size, selects> (v, increment, cmk_stop<stop> ());
}

// cmk_increase_by_cols, FIXME check whether rows is power of 2
template<typename dataTy, unsigned int rows, unsigned int cols, unsigned int selects>
_GENX_ inline void cmk_increase_by_rows (matrix_ref<dataTy, rows, cols> m, dataTy increment, cmk_stop<false>)
{
  m.template select<selects, 1, cols, 1> (selects, 0) = m.template select<selects, 1, cols, 1> (0, 0) + increment;
  enum { stop = selects * 2 * 2 > rows };
  cmk_increase_by_rows<dataTy, rows, cols, selects * 2> (m, increment * 2, cmk_stop<stop> ());
}

template<typename dataTy, unsigned int rows, unsigned int cols, unsigned int selects>
_GENX_ inline void cmk_increase_by_rows (matrix_ref<dataTy, rows, cols> m, dataTy increment, cmk_stop<true>)
{
}

template<typename dataTy, unsigned int rows, unsigned int cols, unsigned int selects>
_GENX_ inline void cmk_increase_by_rows (matrix_ref<dataTy, rows, cols> m, dataTy increment)
{
  enum { stop = selects * 2 > ro#include "cm_rt.h"ws };
  cmk_increase_by_rows<dataTy, rows, cols, selects> (m, increment, cmk_stop<stop> ());
}


//template<typename ty, unsigned int rows, unsigned int cols>
//inline _GENX_ void cmk_read (SurfaceIndex index, unsigned int x, unsigned int y, matrix_ref<ty, rows, cols> m)
//{
//  enum { colInterval = 32 / sizeof (ty) };
//
//#pragma unroll
//  for (unsigned int col = 0; col < cols; col += colInterval) {
//    read (index, x + col * sizeof (ty), y, m.template select<rows, 1, colInterval, 1> (0, col));
//  }
//}

template<typename ty, unsigned int size>
inline _GENX_ void cmk_read (SurfaceIndex index, unsigned int offset, vector_ref<ty, size> v)
{
#pragma unroll
  for (unsigned int i = 0; i < size; i += 32) {
    read (index, offset + i * sizeof (ty), v.template select<32, 1> (i));
  }
}

template<typename ty, unsigned int size>
inline _GENX_ void cmk_read_4 (SurfaceIndex index, unsigned int offset, vector_ref<ty, size> v)
{
#pragma unroll
  for (unsigned int i = 0; i < size; i += 4) {
    read (index, offset + i * sizeof (ty), v.template select<4, 1> (i));
  }
}

template<typename ty, unsigned int size>
inline _GENX_ void cmk_write (SurfaceIndex index, unsigned int offset, vector_ref<ty, size> v)
{
#pragma unroll
  for (unsigned int i = 0; i < size; i += 32) {
    write (index, offset + i * sizeof (ty), v.template select<32, 1> (i));
  }
}

template<typename valueTy, unsigned int size>
inline _GENX_ void cmk_transpose_by_row_to_column (matrix_ref<valueTy, 8, size> tm, matrix_ref<valueTy, size, 8> m)
{
#pragma unroll
  for (unsigned int i = 0; i < size; ++i) {
    tm.column (i) = m.row (i);
  }
}

#define CM_CONST_E    2.718282f
#define CM_CONST_PI   3.141593f
#define CM_CONST_2PI  6.283185f

#define DBL_EPSILON     0.00001f /* smallest such that 1.0+DBL_EPSILON != 1.0 */

template<unsigned int rows, unsigned int cols>
inline _GENX_ matrix<float, rows, cols> cmk_atan2 (matrix_ref<short, rows, cols> y, matrix_ref<short, rows, cols> x)
{
  matrix<float, rows, cols> a0;
  matrix<float, rows, cols> a1;

  matrix<unsigned short, rows, cols> mask = y >= 0;
  a0.merge (CM_CONST_PI * 0.5, CM_CONST_PI * 1.5, mask);
  a1.merge (0, CM_CONST_PI * 2, mask);

  a1.merge (CM_CONST_PI, x < 0);

  matrix<float, rows, cols> xy = x * y;
  matrix<float, rows, cols> x2 = x * x;
  matrix<float, rows, cols> y2 = y * y;

  a0 -= (xy / (y2 + 0.28f * x2 + DBL_EPSILON));
  a1 += (xy / (x2 + 0.28f * y2 + DBL_EPSILON));

  matrix<float, rows, cols> atan2;
  atan2.merge (a1, a0, y2 <= x2);

  return atan2;
}

//template<unsigned int rows, unsigned int cols>
//_GENX_ void cmk_atan2 (matrix_ref<float, rows, cols> atan2,
//                              matrix_ref<short, rows, cols> y,
//                              matrix_ref<short, rows, cols> x)
//{
//  matrix<float, rows, cols> x2 = x * x;
//  matrix<float, rows, cols> y2 = y * y;
//
//  matrix<float, rows, cols> pi;
//  pi.merge (PI * 0.5, PI * 1.5, y >= 0);
//  matrix<float, rows, cols> a0 = pi - x * y / (y2 + 0.28f * x2 + DBL_EPSILON);
//
//  pi.merge (0, PI * 2, y >= 0);
//  pi.merge (PI, x < 0);
//  matrix<float, rows, cols> a1 = pi + x * y / (x2 + 0.28f * y2 + DBL_EPSILON);
//
//  atan2.merge (a1, a0, y2 <= x2);
//}

#define cm_floor cm_rndd
#define cm_ceil cm_rndu

//template<unsigned int rows, unsigned int cols>
//inline _GENX_ void cmk_floor (matrix_ref<short, rows, cols> floor, matrix_ref<float, rows, cols> x)
//{
//  floor = x;
//  floor -= (x < 0);
//}
