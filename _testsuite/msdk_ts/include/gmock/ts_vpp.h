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

class tsVideoVPP : virtual public tsSession
{
public:
    bool                        m_default;
    bool                        m_initialized;
    bool                        m_loaded;
    bool                        m_use_memid;
    tsExtBufType<mfxVideoParam> m_par;
    mfxFrameAllocRequest        m_request[2];
    mfxVPPStat                  m_stat;
    mfxVideoParam*              m_pPar;
    mfxVideoParam*              m_pParOut;
    mfxFrameAllocRequest*       m_pRequest;
    mfxSyncPoint*               m_pSyncPoint;
    mfxFrameSurface1*           m_pSurfIn;
    mfxFrameSurface1*           m_pSurfOut;
    mfxFrameSurface1*           m_pSurfWork;
    mfxVPPStat*                 m_pStat;
    tsSurfacePool*              m_pSurfPoolIn;
    tsSurfacePool*              m_pSurfPoolOut;
    tsSurfaceProcessor*         m_surf_in_processor;
    tsSurfaceProcessor*         m_surf_out_processor;
    mfxPluginUID*               m_uid;

    std::map<mfxSyncPoint, mfxFrameSurface1*> m_surf_out;
    tsSurfacePool               m_spool_in;
    tsSurfacePool               m_spool_out;

    tsVideoVPP(bool useDefaults = true, mfxU32 id = 0);
    virtual ~tsVideoVPP();

    mfxStatus Init();
    mfxStatus Init(mfxSession session, mfxVideoParam *par);

    mfxStatus Close(bool check=false);
    mfxStatus Close(mfxSession session);

    mfxStatus Query();
    mfxStatus Query(mfxSession session, mfxVideoParam *in, mfxVideoParam *out);

    mfxStatus QueryIOSurf();
    mfxStatus QueryIOSurf(mfxSession session, mfxVideoParam *par, mfxFrameAllocRequest *request);

    mfxStatus Reset();
    mfxStatus Reset(mfxSession session, mfxVideoParam *par);

    mfxStatus GetVideoParam();
    mfxStatus GetVideoParam(mfxSession session, mfxVideoParam *par);

    mfxStatus GetVPPStat();
    mfxStatus GetVPPStat(mfxSession session, mfxVPPStat *stat);

    mfxStatus AllocSurfaces();

    mfxStatus CreateAllocators();
    mfxStatus SetFrameAllocator();
    mfxStatus SetFrameAllocator(mfxSession session, mfxFrameAllocator *allocator);

    mfxStatus RunFrameVPPAsync();
    virtual mfxStatus RunFrameVPPAsync(mfxSession session, mfxFrameSurface1 *in, mfxFrameSurface1 *out, mfxExtVppAuxData *aux, mfxSyncPoint *syncp);

    mfxStatus RunFrameVPPAsyncEx();
    virtual mfxStatus RunFrameVPPAsyncEx(mfxSession session, mfxFrameSurface1 *in, mfxFrameSurface1 *surface_work, mfxFrameSurface1 **surface_out, mfxSyncPoint *syncp);

    mfxStatus SyncOperation();
    virtual mfxStatus SyncOperation(mfxSyncPoint syncp);
    mfxStatus SyncOperation(mfxSession session, mfxSyncPoint syncp, mfxU32 wait);

    mfxStatus ProcessFrames(mfxU32 n);
    mfxStatus ProcessFramesEx(mfxU32 n);

    mfxStatus Load();

    mfxStatus SetHandle();
    mfxStatus SetHandle(mfxSession session, mfxHandleType type, mfxHDL handle);
};

namespace tsVPPInfo
{
    struct CFormat
    {
        mfxU32 FourCC;
        mfxU16 ChromaFormat;
        mfxU16 BdY;
        mfxU16 BdC;
        mfxU16 Shift;
    };

    typedef enum eFmtId
    {
          FMT_ID_8B_420_NV12 = 0
        , FMT_ID_8B_420_YV12
        , FMT_ID_8B_422_UYVY
        , FMT_ID_8B_422_YUY2
        , FMT_ID_8B_444_AYUV
        , FMT_ID_8B_444_RGB4
        , FMT_ID_10B_420_P010
        , FMT_ID_10B_422_Y210
        , FMT_ID_10B_444_Y410
        , FMT_ID_10B_444_A2RGB10
        , FMT_ID_12B_420_P016
        , FMT_ID_12B_422_Y216
        , FMT_ID_12B_444_Y416
        , NumFormats
    } eFmtId;

    typedef mfxStatus TCCSupport[NumFormats][NumFormats];

    extern const CFormat Formats[NumFormats];
    extern const TCCSupport CCSupportTable[3];

    inline const TCCSupport& CCSupport()
    {
        return CCSupportTable[
              (g_tsHWtype >= MFX_HW_ICL)
            + (g_tsHWtype >= MFX_HW_TGL)];
    }
}
