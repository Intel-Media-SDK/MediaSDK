/*//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2020 Intel Corporation
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
#include "asc_common_impl.h"

void calc_RACA_4x4_C(
    mfxU8  * pSrc,
    mfxI32   pitch,
    mfxI32 * RS,
    mfxI32 * CS
)
{
    mfxI32 i, j;
    mfxU8 *pS = pSrc;
    mfxU8 *pS2 = pSrc + pitch;
    mfxI32 Rs, Cs;

    Cs = 0;
    Rs = 0;
    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < 4; j++)
        {
            Cs += (pS[j] > pS[j + 1]) ? (pS[j] - pS[j + 1]) : (pS[j + 1] - pS[j]);
            Rs += (pS[j] > pS2[j]) ? (pS[j] - pS2[j]) : (pS2[j] - pS[j]);
        }
        pS += pitch;
        pS2 += pitch;
    }

    *CS += Cs >> 4;
    *RS += Rs >> 4;
}
