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

#if defined (MFX_ENABLE_VPP) 
#include "mfx_vpp_interface.h"

 
#if defined (MFX_VA_LINUX)
    #include "mfx_vpp_vaapi.h"

#endif

using namespace MfxHwVideoProcessing;

// platform switcher
DriverVideoProcessing* MfxHwVideoProcessing::CreateVideoProcessing(VideoCORE* /*core*/)
{
    //MFX_CHECK_NULL_PTR1( core );
    //assert( core );
    
#if defined (MFX_VA_LINUX)

    return new VAAPIVideoProcessing;

#else
    
    return NULL;

#endif
} // mfxStatus CreateVideoProcessing( VideoCORE* core )


mfxStatus VPPHWResMng::CreateDevice(VideoCORE * core){
    MFX_CHECK_NULL_PTR1(core);
    mfxStatus sts;
    Close();
    m_ddi.reset(MfxHwVideoProcessing::CreateVideoProcessing(core));

    if ( ! m_ddi.get() )
        return MFX_ERR_DEVICE_FAILED;

    mfxVideoParam par    = {};
    par.vpp.In.Width     = 352;
    par.vpp.In.Height    = 288;
    par.vpp.In.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
    par.vpp.In.FourCC    = MFX_FOURCC_NV12;
    par.vpp.Out          = par.vpp.In;

    sts = m_ddi->CreateDevice(core, &par, true);
    MFX_CHECK_STS(sts);

    sts = m_ddi->QueryCapabilities(m_caps);
    return sts;
}

#endif // ifdefined (MFX_ENABLE_VPP)
/* EOF */
