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

#include <math.h>

#include "mfx_vpp_utils.h"
#include "mfx_procamp_vpp.h"

/* Procamp Filter Parameter Settings
NAME          MIN         MAX         Step        Default
Brightness     -100.0      100.0       0.1         0.0
Contrast          0.0       10.0       0.01        1.0
Hue            -180.0      180.0       0.1         0.0
Saturation         0.0      10.0       0.01        1.0
*/
#define VPP_PROCAMP_BRIGHTNESS_MAX      100.0
#define VPP_PROCAMP_BRIGHTNESS_MIN     -100.0
#define VPP_PROCAMP_BRIGHTNESS_DEFAULT    0.0

#define VPP_PROCAMP_CONTRAST_MAX     10.0
#define VPP_PROCAMP_CONTRAST_MIN      0.0
#define VPP_PROCAMP_CONTRAST_DEFAULT  1.0

#define VPP_PROCAMP_HUE_MAX          180.0
#define VPP_PROCAMP_HUE_MIN         -180.0
#define VPP_PROCAMP_HUE_DEFAULT        0.0

#define VPP_PROCAMP_SATURATION_MAX     10.0
#define VPP_PROCAMP_SATURATION_MIN      0.0
#define VPP_PROCAMP_SATURATION_DEFAULT  1.0

/* ******************************************************************** */
/*           implementation of VPP filter [ProcessingAmplified]         */
/* ******************************************************************** */

mfxStatus MFXVideoVPPProcAmp::Query( mfxExtBuffer* pHint )
{
    if( NULL == pHint )
    {
        return MFX_ERR_NONE;
    }

    mfxStatus sts = MFX_ERR_NONE;

    mfxExtVPPProcAmp* pParam = (mfxExtVPPProcAmp*)pHint;

    /* Brightness */
    if( pParam->Brightness < VPP_PROCAMP_BRIGHTNESS_MIN ||
        pParam->Brightness > VPP_PROCAMP_BRIGHTNESS_MAX )
    {
        pParam->Brightness = mfx::clamp(pParam->Brightness, VPP_PROCAMP_BRIGHTNESS_MIN, VPP_PROCAMP_BRIGHTNESS_MAX);

        sts = MFX_WRN_INCOMPATIBLE_VIDEO_PARAM;
    }
    /* Contrast */
    if( pParam->Contrast < VPP_PROCAMP_CONTRAST_MIN || pParam->Contrast > VPP_PROCAMP_CONTRAST_MAX )
    {
        pParam->Contrast = mfx::clamp(pParam->Contrast, VPP_PROCAMP_CONTRAST_MIN, VPP_PROCAMP_CONTRAST_MAX);

        sts = MFX_WRN_INCOMPATIBLE_VIDEO_PARAM;
    }
    /* Hue */
    if( pParam->Hue < VPP_PROCAMP_HUE_MIN || pParam->Hue > VPP_PROCAMP_HUE_MAX )
    {
        pParam->Hue = mfx::clamp(pParam->Hue, VPP_PROCAMP_HUE_MIN, VPP_PROCAMP_HUE_MAX);

        sts = MFX_WRN_INCOMPATIBLE_VIDEO_PARAM;
    }
    /* Saturation */
    if( pParam->Saturation < VPP_PROCAMP_SATURATION_MIN ||
        pParam->Saturation > VPP_PROCAMP_SATURATION_MAX)
    {
        pParam->Saturation = mfx::clamp(pParam->Saturation, VPP_PROCAMP_SATURATION_MIN, VPP_PROCAMP_SATURATION_MAX);

        sts = MFX_WRN_INCOMPATIBLE_VIDEO_PARAM;
    }

    return sts;

} // static mfxStatus MFXVideoVPPProcAmp::Query( mfxExtBuffer* pHint )

#endif // MFX_ENABLE_VPP
/* EOF */
