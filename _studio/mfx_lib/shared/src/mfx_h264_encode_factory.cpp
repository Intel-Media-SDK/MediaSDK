// Copyright (c) 2017-2018 Intel Corporation
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

/* ****************************************************************************** */

#include "mfx_common.h"

#if defined (MFX_ENABLE_H264_VIDEO_ENCODE_HW)

#include "mfx_h264_encode_interface.h"
#ifdef MFX_ENABLE_MFE
#include "libmfx_core_interface.h"
#endif
#if defined (MFX_VA_LINUX)
    #include "mfx_h264_encode_vaapi.h"

#endif


using namespace MfxHwH264Encode;
// platform switcher

// tmp solution

DriverEncoder* MfxHwH264Encode::CreatePlatformH264Encoder( VideoCORE* core )
{
    //MFX_CHECK_NULL_PTR1( core );
    assert( core );
    (void)core;

#if defined (MFX_VA_LINUX)

    return new VAAPIEncoder;//( core );

#endif

} // DriverEncoder* MfxHwH264Encode::CreatePlatformH264Encoder( VideoCORE* core )

#if defined(MFX_ENABLE_MFE) && !defined(AS_H264LA_PLUGIN)
MFEVAAPIEncoder* MfxHwH264Encode::CreatePlatformMFEEncoder(VideoCORE* core)
{
    assert( core );

    // needs to search, thus use special GUID
    ComPtrCore<MFEVAAPIEncoder> *pVideoEncoder = QueryCoreInterface<ComPtrCore<MFEVAAPIEncoder> >(core, MFXMFEDDIENCODER_SEARCH_GUID);
    if (!pVideoEncoder) return NULL;
    if (!pVideoEncoder->get())
        *pVideoEncoder = new MFEVAAPIEncoder;

    return pVideoEncoder->get();

} // MFEVAAPIEncoder* MfxHwH264Encode::CreatePlatformMFEEncoder( VideoCORE* core )
#endif


#endif // #if defined (MFX_ENABLE_H264_VIDEO_ENCODE_HW)
/* EOF */
