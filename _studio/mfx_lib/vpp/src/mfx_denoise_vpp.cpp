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

#include "mfx_common.h"

#if defined (MFX_ENABLE_VPP)
#if defined(__GNUC__)
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#include "mfx_vpp_utils.h"
#include "mfx_denoise_vpp.h"

/* ******************************************************************** */
/*                 implementation of VPP filter [Denoise]               */
/* ******************************************************************** */
// this range are used by Query to correct application request
#define PAR_NRF_STRENGTH_MIN                (0)
#define PAR_NRF_STRENGTH_MAX                100 // real value is 63
#define PAR_NRF_STRENGTH_DEFAULT            PAR_NRF_STRENGTH_MIN

mfxStatus MFXVideoVPPDenoise::Query( mfxExtBuffer* pHint )
{
    if( NULL == pHint )
    {
        return MFX_ERR_NONE;
    }

    mfxStatus sts = MFX_ERR_NONE;

    mfxExtVPPDenoise* pParam = (mfxExtVPPDenoise*)pHint;

    if( pParam->DenoiseFactor > PAR_NRF_STRENGTH_MAX )
    {
        pParam->DenoiseFactor = PAR_NRF_STRENGTH_MAX;

        sts = MFX_WRN_INCOMPATIBLE_VIDEO_PARAM;
    }

    return sts;

} // static mfxStatus MFXVideoVPPDenoise::Query( mfxExtBuffer* pHint )

#endif // MFX_ENABLE_VPP

/* EOF */
