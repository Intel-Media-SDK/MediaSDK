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

#if !defined(__MFX_THREAD_TASK_H)
#define __MFX_THREAD_TASK_H

#include <mfxvideo.h>
#include <mfxvideo++int.h>
#include "mfxenc.h"
#include "mfxpak.h"

// Declare task's parameters structure
typedef
struct MFX_THREAD_TASK_PARAMETERS
{
    union
    {
        // ENCODE parameters
        struct
        {
            mfxEncodeCtrl *ctrl;                                // [IN] pointer to encode control
            mfxFrameSurface1 *surface;                          // [IN] pointer to the source surface
            mfxBitstream *bs;                                   // [OUT] pointer to the destination bitstream
            mfxEncodeInternalParams internal_params;            // output of EncodeFrameCheck(), input of EncodeFrame()

        } encode;

        // DECODE parameters
        struct
        {
            mfxBitstream *bs;                                   // [IN] pointer to the source bitstream
            mfxFrameSurface1 *surface_work;                     // [IN] pointer to the surface for decoding
            mfxFrameSurface1 *surface_out;                      // [OUT] pointer to the current being decoded surface

        } decode;

        // VPP parameters
        struct
        {
            mfxFrameSurface1 *in;                               // [IN] pointer to the source surface
            mfxFrameSurface1 *out;                              // [OUT] pointer to the destination surface
            mfxExtVppAuxData *aux;                              // [IN] auxilary encoding data

        } vpp;

        // ENC, PAK parameters
        struct
        {
            mfxENCInput  *in; 
            mfxENCOutput *out;
        } enc;
        struct
        {
            mfxPAKInput  *in;
            mfxPAKOutput *out;
        } pak_ext;
    };

} MFX_THREAD_TASK_PARAMETERS;

#endif // __MFX_THREAD_TASK_H
