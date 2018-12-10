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

#ifndef __MFX_VPP_MVC_H
#define __MFX_VPP_MVC_H

#include <memory>
#include <map>

#include "mfxvideo++int.h"
#include "mfx_task.h"
#include "mfx_vpp_defs.h"
#include "mfx_vpp_base.h"
#include "mfx_vpp_sw.h"

namespace MfxVideoProcessing
{
    class ImplementationMvc : public VideoVPP 
    {
    public:

        static mfxStatus Query(VideoCORE* core, mfxVideoParam *in, mfxVideoParam *out)
        {
            return VideoVPPBase::Query( core, in, out);
        }

        static mfxStatus QueryIOSurf(VideoCORE* core, mfxVideoParam *par, mfxFrameAllocRequest *request);

        ImplementationMvc(VideoCORE *core);
        virtual ~ImplementationMvc();

        // VideoVPP
        virtual mfxStatus RunFrameVPP(mfxFrameSurface1 *in, mfxFrameSurface1 *out, mfxExtVppAuxData *aux);

        // VideoBase methods
        virtual mfxStatus Reset(mfxVideoParam *par);
        virtual mfxStatus Close(void);
        virtual mfxStatus Init(mfxVideoParam *par);

        virtual mfxStatus GetVideoParam(
            mfxVideoParam *par);

        virtual mfxStatus GetVPPStat(
            mfxVPPStat *stat);

        virtual mfxStatus VppFrameCheck(
            mfxFrameSurface1 *in, 
            mfxFrameSurface1 *out, 
            mfxExtVppAuxData *aux,
            MFX_ENTRY_POINT pEntryPoint[], 
            mfxU32 &numEntryPoints);

        virtual mfxStatus VppFrameCheck(
            mfxFrameSurface1 *, 
            mfxFrameSurface1 *)
        {
            return MFX_ERR_UNSUPPORTED;
        }

        virtual mfxTaskThreadingPolicy GetThreadingPolicy(void);

        // multi threading of SW_VPP functions
        mfxStatus RunVPPTask(
            mfxFrameSurface1 *in, 
            mfxFrameSurface1 *out, 
            FilterVPP::InternalParam *pParam );

        mfxStatus ResetTaskCounters();


    private:

        bool       m_bInit;
        bool       m_bMultiViewMode;
        VideoCORE* m_core;

        typedef std::map<mfxU16, VideoVPPBase*> mfxMultiViewVPP;
        typedef mfxMultiViewVPP::iterator mfxMultiViewVPP_Iterator;

        mfxMultiViewVPP_Iterator m_iteratorVPP;
        mfxMultiViewVPP m_VPP;
    };

}; // namespace MfxVideoProcessing

#endif // __MFX_VPP_MVC_H
#endif // MFX_ENABLE_VPP
/* EOF */
