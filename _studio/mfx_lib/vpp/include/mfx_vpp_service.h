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

/* ****************************************************************************** */

#include "mfx_common.h"

#if defined (MFX_ENABLE_VPP)

#ifndef __MFX_VPP_SERVICE_H
#define __MFX_VPP_SERVICE_H

#include "mfxvideo++int.h"
#include "mfx_vpp_defs.h"

class DynamicFramesPool {

public:
    DynamicFramesPool( void );
    virtual ~DynamicFramesPool();

    mfxStatus Init( VideoCORE *core, mfxFrameAllocRequest* pRequest, bool needAlloc);
    mfxStatus Close( void );
    mfxStatus Reset( void );
    mfxStatus Zero( void );
    mfxStatus Lock( void );
    mfxStatus Unlock( void );
    mfxStatus GetFreeSurface(mfxFrameSurface1** ppSurface);

    mfxStatus SetCrop( mfxFrameInfo* pInfo );

    mfxStatus PreProcessSync( void );
    mfxStatus PostProcessSync( void );
    mfxStatus GetFilledSurface(mfxFrameSurface1*  in, mfxFrameSurface1** ppSurface);

protected:
    mfxU16            m_numFrames;
    mfxFrameSurface1* m_pSurface;
    mfxMemId*         m_mids;
    VideoCORE*        m_core;
    mfxFrameData**    m_ptrDataTab;
    mfxU16*           m_savedLocked;
    bool*             m_bLockedOutput;
    bool              m_allocated;

private:
    // Declare private copy constructor to avoid accidental assignment
    // and klocwork complaining.
    DynamicFramesPool(const DynamicFramesPool &);
    DynamicFramesPool & operator = (const DynamicFramesPool &);
};

#endif // __MFX_VPP_SERVICE_H

#endif // MFX_ENABLE_VPP
/*EOF*/
