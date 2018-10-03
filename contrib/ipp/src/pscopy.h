// Copyright (c) 2018 Intel Corporation
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

/*M*
//
//  Purpose:    Copying a vector into second vector (Copy).
//              Filling a vector by 0               (Zero).
//
//  Contents:   mfxownsCopy_8u
//              mfxownsZero_8u
//              mfxownsSet_32s
*M*/

#if !defined (__PSCOPY_H__)
#define __PSCOPY_H__

extern Ipp8u* mfxownsCopy_8u( const Ipp8u* pSrc, Ipp8u* pDst, int len );
extern Ipp8u* mfxownsZero_8u( Ipp8u* pDst, int len );
extern Ipp32s* mfxownsSet_32s( Ipp32s val, Ipp32s* pDst, int len);

/*******************************************************************************/

#endif /* #if !defined (__PSCOPY_H__) */
