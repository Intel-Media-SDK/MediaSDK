/*//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2020 Intel Corporation
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
*/
#include "fast_copy_c_impl.h"

void copyVideoToSys_C(const mfxU8* src, mfxU8* dst, int width)
{
    std::copy(src, src + width, dst);
}

void copyVideoToSysShift_C(const mfxU16* src, mfxU16* dst, int width, int shift)
{
    for (int i = 0; i < width; i++)
        *dst++ = (*src++) >> shift;
}

void copySysToVideoShift_C(const mfxU16* src, mfxU16* dst, int width, int shift)
{
    for (int i = 0; i < width; i++)
        *dst++ = (*src++) << shift;
}
