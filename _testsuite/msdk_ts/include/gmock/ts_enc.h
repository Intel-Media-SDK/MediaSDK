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

#pragma once

#include "ts_session.h"
#include "ts_ext_buffers.h"
#include "ts_surface.h"
#include "ts_encoder.h"

class tsVideoENC : public tsSession, public tsSurfacePool
{
public:
    bool                        m_default;
    bool                        m_initialized;
    bool                        m_loaded;
    tsExtBufType<mfxVideoParam> m_par;
    tsExtBufType<mfxEncodeCtrl> m_ctrl;
    mfxFrameAllocRequest        m_request;
    mfxVideoParam*              m_pPar;
    mfxVideoParam*              m_pParOut;
    mfxFrameAllocRequest*       m_pRequest;
    mfxSyncPoint*               m_pSyncPoint;
    tsSurfaceProcessor*         m_filler;
    mfxU32                      m_frames_buffered;
    mfxPluginUID*               m_uid;
    mfxENCInput*                m_ENCInput;
    mfxENCOutput*               m_ENCOutput;
    tsVideoEncoder*             m_pEncode;

    tsVideoENC(mfxU32 CodecId = 0, bool useDefaults = true, tsVideoEncoder* enc = 0);
    tsVideoENC(mfxFeiFunction func, mfxU32 CodecId = 0, bool useDefaults = true);
    ~tsVideoENC();

    mfxStatus Init();
    mfxStatus Init(mfxSession session, mfxVideoParam *par);

    mfxStatus Close();
    mfxStatus Close(mfxSession session);

    mfxStatus Query();
    mfxStatus Query(mfxSession session, mfxVideoParam *in, mfxVideoParam *out);

    mfxStatus QueryIOSurf();
    mfxStatus QueryIOSurf(mfxSession session, mfxVideoParam *par, mfxFrameAllocRequest *request);

    mfxStatus Reset();
    mfxStatus Reset(mfxSession session, mfxVideoParam *par);

    mfxStatus ProcessFrameAsync();
    mfxStatus ProcessFrameAsync(mfxSession session, mfxENCInput *in, mfxENCOutput *out, mfxSyncPoint *syncp);

    mfxStatus SyncOperation();
    mfxStatus SyncOperation(mfxSyncPoint syncp);
    mfxStatus SyncOperation(mfxSession session, mfxSyncPoint syncp, mfxU32 wait);

    mfxStatus AllocSurfaces();

    mfxStatus Encode1N(mfxU32 n);

    mfxStatus Load() { m_loaded = (0 == tsSession::Load(m_session, m_uid, 1)); return g_tsStatus.get(); }
};