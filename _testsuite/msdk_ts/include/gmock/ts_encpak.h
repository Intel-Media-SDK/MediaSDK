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

mfxStatus PrepareCtrlBuf(mfxExtFeiEncFrameCtrl* & fei_encode_ctrl, const mfxVideoParam& vpar);
mfxStatus PrepareSPSBuf(mfxExtFeiSPS* & fsps, const mfxVideoParam& vpar);
mfxStatus PreparePPSBuf(mfxExtFeiPPS* & fpps, const mfxVideoParam& vpar);
mfxStatus PrepareSliceBuf(mfxExtFeiSliceHeader* & fslice, const mfxVideoParam& vpar);

mfxStatus PreparePakMBCtrlBuf(mfxExtFeiPakMBCtrl & mb, const mfxVideoParam& vpar);
mfxStatus PrepareEncMVBuf(mfxExtFeiEncMV & mv, const mfxVideoParam& vpar);

#define   PpsDPBSize 16
void      CopyPpsDPB(const mfxExtFeiPPS::mfxExtFeiPpsDPB *src, mfxExtFeiPPS::mfxExtFeiPpsDPB *dst);

// there should be common function like this instead of one in mfx_h264_enc_common_hw.h
mfxExtBuffer* GetExtFeiBuffer(mfxExtBuffer** ebuffers, mfxU32 nbuffers, mfxU32 BufferId, mfxU32 field = 0);
mfxExtBuffer* GetExtFeiBuffer(std::vector<mfxExtBuffer*>& buff, mfxU32 BufferId, mfxU32 field = 0);

// temporary here, TODO template
inline
mfxStatus ExcludeExtBufferPtr(std::vector<mfxExtBuffer*>& buff, mfxExtBuffer* ptr)
{
    buff.erase(std::remove(buff.begin(), buff.end(), ptr), buff.end());
    return MFX_ERR_NONE;
}

#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(P) {if (P) { delete [] (P); (P) = 0; }}
#endif

enum
{
    DEFAULT_IDC = 0,
};

typedef struct {
    mfxU16 dpb_idx; // pos in pps->ReferenceFrames[0];
    mfxU16 type;    // MFX_PICTYPE_FRAME, _TOPFIELD, _BOTTOMFIELD
    mfxU16 LongTermFrameIdx; // Like PpsDPB
    mfxI16 diffFrameOrder;   // this ref minus current frame
} RefListElem;

class tsVideoENCPAK : public tsSession//, public tsSurfacePool
{
public:
    bool                        m_default;
    bool                        m_initialized;
    bool                        m_loaded;
    bool                        m_bSingleField;

    struct feiCfg {
        //tsExtBufType<mfxVideoParam> m_par;
        mfxVideoParam               m_par;
        mfxVideoParam*              m_pPar;    // todo remove
        mfxVideoParam*              m_pParOut; // after Query; todo remove
        std::vector<mfxExtBuffer*>  initbuf, inbuf, outbuf;
        mfxExtFeiSliceHeader*       fslice; // can hold 2 consecutive buffers: [1st, 2nd fields]
        mfxExtFeiPPS*               fpps;   // can hold 2 consecutive buffers: [1st, 2nd fields]
        mfxExtFeiEncFrameCtrl*      fctrl;  // only for enc --*--
        mfxExtFeiSPS*               fsps;
        mfxExtFeiParam*             fpar;
        mfxFeiFunction              Func;
        feiCfg(mfxFeiFunction func) : m_par(), m_pPar(&m_par), m_pParOut(&m_par), initbuf(3), inbuf(6), outbuf(3), fslice(0), fpps(0), fctrl(0), fsps(0), fpar(0), Func(func) {};
    } enc, pak;

    struct refInfo {
        mfxFrameSurface1* frame;
        mfxU16 LTidx;   // LT frame idx,
        bool  avail[2]; // [top, bottom] unused if both are false
    };

    tsSurfacePool               m_enc_pool;
    tsSurfacePool               m_rec_pool;
    mfxFrameAllocRequest        m_enc_request;
    mfxFrameAllocRequest        m_rec_request;

    tsExtBufType<mfxBitstream>  m_bitstream;
    mfxBitstream*               m_pBitstream;
    tsBitstreamProcessor*       m_bs_processor;

    mfxSyncPoint*               m_pSyncPoint;
    tsSurfaceProcessor*         m_filler;
    mfxPluginUID*               m_uid;

    mfxENCInput*                m_ENCInput;
    mfxENCOutput*               m_ENCOutput;
    mfxPAKInput*                m_PAKInput;
    mfxPAKOutput*               m_PAKOutput;

    mfxU32 m_BaseAllocID;
    mfxU32 m_EncPakReconAllocID;

    tsVideoENCPAK(mfxFeiFunction funcEnc, mfxFeiFunction funcPak, mfxU32 CodecId = 0, bool useDefaults = true);
    virtual ~tsVideoENCPAK();

    mfxStatus Init();
    mfxStatus Init(mfxSession session/*, mfxVideoParam *par*/);

    mfxStatus Close();
    mfxStatus Close(mfxSession session);

    mfxStatus Query();
    //mfxStatus Query(mfxSession session, mfxVideoParam *in, mfxVideoParam *out);

    mfxStatus QueryIOSurf();
    //mfxStatus QueryIOSurf(mfxSession session, mfxVideoParam *par, mfxFrameAllocRequest *request);

    mfxStatus Reset();
    //mfxStatus Reset(mfxSession session, mfxVideoParam *par);

    mfxStatus AllocBitstream(mfxU32 size = 0);
    mfxStatus GetVideoParam();
    //mfxStatus GetVideoParam(mfxSession session, mfxVideoParam *par);

    virtual mfxStatus ProcessFrameAsync();
    mfxStatus ProcessFrameAsync(mfxSession session, mfxENCInput *in, mfxENCOutput *out, mfxSyncPoint *syncp);
    mfxStatus ProcessFrameAsync(mfxSession session, mfxPAKInput *in, mfxPAKOutput *out, mfxSyncPoint *syncp);

    mfxStatus SyncOperation();
    mfxStatus SyncOperation(mfxSyncPoint syncp, tsBitstreamProcessor* bs_processor);
    mfxStatus SyncOperation(mfxSession session, mfxSyncPoint syncp, mfxU32 wait);

    mfxStatus AllocSurfaces();

    mfxStatus PrepareInitBuffers();
    mfxStatus PrepareFrameBuffers(bool secondField);

    mfxStatus CreatePpsDPB(const std::vector<refInfo>& dpb, mfxExtFeiPPS::mfxExtFeiPpsDPB *pd, bool after2nd);
    mfxStatus UpdateDPB   (std::vector<refInfo>& dpb, bool secondField);
    mfxStatus PrepareDpbBuffers(bool secondField); // fills dpb from current state
    mfxStatus FillRefLists(bool secondField);
    mfxStatus FillSliceRefs(bool secondField);

    mfxStatus EncodeFrame(bool secondField);

    mfxStatus Load() { m_loaded = (0 == tsSession::Load(m_session, m_uid, 1)); return g_tsStatus.get(); }
private:
    mfxENCInput                 ENCInput;
    mfxENCOutput                ENCOutput;
    mfxPAKInput                 PAKInput;
    mfxPAKOutput                PAKOutput;

    mfxExtFeiPPS::mfxExtFeiPpsDPB nextDpb[PpsDPBSize]; // before frame - expectation, after frame - compare to next Before

    //std::vector<mfxFrameSurface1*> refs; // dpb, updated after frame, filled at tail
    std::vector<mfxFrameSurface1*> recSet; // hw allocated recon surface pool, in->LOsufaces
    //std::vector<mfxU16> LTidxSet; // LT frame idx, co-located with recSet
    std::vector<refInfo> refInfoSet; // co-located with recSet
public:
    mfxExtFeiPakMBCtrl          m_mb[2]; // [1st, 2nd fields]
    mfxExtFeiEncMV              m_mv[2]; // [1st, 2nd fields]

    std::vector<RefListElem> RefList[2]; // [L0,L1], filled at tail


};

