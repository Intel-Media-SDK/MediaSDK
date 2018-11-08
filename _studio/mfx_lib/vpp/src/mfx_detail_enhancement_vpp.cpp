// Copyright (c) 2017-2019 Intel Corporation
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

#include "mfx_common.h"

#if defined (MFX_ENABLE_VPP)

#include "mfx_vpp_utils.h"
#include "mfx_detail_enhancement_vpp.h"

#ifndef ALIGN16
#define ALIGN16(SZ) (((SZ + 15) >> 4) << 4) // round up to a multiple of 16
#endif

#define VPP_DETAIL_GAIN_MIN      (0)
#define VPP_DETAIL_GAIN_MAX      (63)
#define VPP_DETAIL_GAIN_MAX_REAL (63)
// replaced range [0...63] to native [0, 100%] wo notification of dev
// so a lot of modification required to provide smooth control (recalculate test streams etc)
// solution is realGain = CLIP(userGain, 0, 63)
#define VPP_DETAIL_GAIN_MAX_USER_LEVEL (100)
#define VPP_DETAIL_GAIN_DEFAULT VPP_DETAIL_GAIN_MIN

/* ******************************************************************** */
/*           implementation of VPP filter [DetailEnhancement]           */
/* ******************************************************************** */

mfxStatus MFXVideoVPPDetailEnhancement::Query( mfxExtBuffer* pHint )
{
    if( NULL == pHint )
    {
        return MFX_ERR_NONE;
    }

    mfxStatus sts = MFX_ERR_NONE;

    mfxExtVPPDetail* pParam = (mfxExtVPPDetail*)pHint;

    if( pParam->DetailFactor > VPP_DETAIL_GAIN_MAX_USER_LEVEL )
    {
        pParam->DetailFactor = VPP_DETAIL_GAIN_MAX_USER_LEVEL;

        sts = MFX_WRN_INCOMPATIBLE_VIDEO_PARAM;
    }

    return sts;

} // mfxStatus MFXVideoVPPDetailEnhancement::Query( mfxExtBuffer* pHint )

#endif // MFX_ENABLE_VPP
/* EOF */
