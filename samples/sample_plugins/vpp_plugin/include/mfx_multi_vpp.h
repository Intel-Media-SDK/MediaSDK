/******************************************************************************\
Copyright (c) 2005-2017, Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

This sample was distributed or derived from the Intel's Media Samples package.
The original version of this sample may be obtained from https://software.intel.com/en-us/intel-media-server-studio
or https://software.intel.com/en-us/media-client-solutions-support.
\**********************************************************************************/

#ifndef __MFX_MULTI_VPP_H
#define __MFX_MULTI_VPP_H

// An interface for a pipeline consisting of multiple (maximum 3) VPP-like components. Base implementation - for single VPP.
// The application should use this interface to be able to seamlessly switch from MFXVideoVPP to MFXVideoVPPPlugin in the pipeline.
class MFXVideoMultiVPP
{
public:

    MFXVideoMultiVPP(mfxSession session) { m_session = session; }
    virtual ~MFXVideoMultiVPP(void) { Close(); }

    // topology methods
    virtual mfxStatus QueryIOSurf(mfxVideoParam *par, mfxFrameAllocRequest request[2], mfxVideoParam *par1 = NULL, mfxVideoParam *par2 = NULL)
    { par1; par2; return MFXVideoVPP_QueryIOSurf(m_session, par, request); }

    virtual mfxStatus Init(mfxVideoParam *par, mfxVideoParam *par1 = NULL, mfxVideoParam *par2 = NULL)
    { par1; par2; return MFXVideoVPP_Init(m_session, par); }

    virtual mfxStatus Reset(mfxVideoParam *par, mfxVideoParam *par1 = NULL, mfxVideoParam *par2 = NULL)
    { par1; par2; return MFXVideoVPP_Reset(m_session, par); }

    virtual mfxStatus RunFrameVPPAsync(mfxFrameSurface1 *in, mfxFrameSurface1 *out, mfxExtVppAuxData *aux, mfxSyncPoint *syncp)
    { return MFXVideoVPP_RunFrameVPPAsync(m_session, in, out, aux, syncp); }

    virtual mfxStatus SyncOperation(mfxSyncPoint syncp, mfxU32 wait)
    {  return MFXVideoCORE_SyncOperation(m_session, syncp, wait);}

    virtual mfxStatus Close(void) { return MFXVideoVPP_Close(m_session); }

    // per-component methods
    virtual mfxStatus Query(mfxVideoParam *in, mfxVideoParam *out, mfxU8 component_idx = 0)
    { component_idx; return MFXVideoVPP_Query(m_session, in, out); }

    virtual mfxStatus GetVideoParam(mfxVideoParam *par, mfxU8 component_idx = 0)
    { component_idx; return MFXVideoVPP_GetVideoParam(m_session, par); }

    virtual mfxStatus GetVPPStat(mfxVPPStat *stat, mfxU8 component_idx = 0)
    { component_idx; return MFXVideoVPP_GetVPPStat(m_session, stat); }

protected:

    mfxSession m_session;                                       // (mfxSession) handle to the owning session
};

#endif//__MFX_MULTI_VPP_H