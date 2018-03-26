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

#include "ts_encpak.h"

//#include "sample_utils.h"
// for
mfxExtBuffer* GetExtBuffer(mfxExtBuffer** ebuffers, mfxU32 nbuffers, mfxU32 BufferId);

// there should be common function like this instead of one in mfx_h264_enc_common_hw.h
mfxExtBuffer* GetExtFeiBuffer(mfxExtBuffer** ebuffers, mfxU32 nbuffers, mfxU32 BufferId, mfxU32 field)
{
    for (mfxU16 i = 0; i < nbuffers; i++)
         if (ebuffers[i] != 0 && ebuffers[i]->BufferId == BufferId && !field --) // skip "field" times
            return (ebuffers[i]);
    return 0;
}

mfxExtBuffer* GetExtFeiBuffer(std::vector<mfxExtBuffer*>& buff, mfxU32 BufferId, mfxU32 field)
{
    for (std::vector<mfxExtBuffer*>::iterator it = buff.begin(); it != buff.end(); it++) {
        if (*it && (*it)->BufferId == BufferId && !field --)
            return *it;
    }
    return 0;
}

void CopyPpsDPB  (const mfxExtFeiPPS::mfxExtFeiPpsDPB *src, mfxExtFeiPPS::mfxExtFeiPpsDPB *dst)
{
    for (mfxU32 d=0; d < PpsDPBSize; d++)
        dst[d] = src[d];
}

mfxU32 FindIdxInPpsDPB  (const mfxExtFeiPPS::mfxExtFeiPpsDPB *src, mfxU16 idx)
{
    for (mfxU32 d=0; d < PpsDPBSize; d++)
        if (src[d].Index == idx)
            return d;
    return PpsDPBSize;
}

// returns mismatched idx or PpsDPBSize if all match
mfxU32 ComparePpsDPB  (const mfxExtFeiPPS::mfxExtFeiPpsDPB *src, const mfxExtFeiPPS::mfxExtFeiPpsDPB *dst)
{
    mfxU32 d;
    for (d = 0; d < PpsDPBSize && src[d].Index != 0xffff; d++)
        if (dst[d].Index != src[d].Index ||
            dst[d].LongTermFrameIdx != src[d].LongTermFrameIdx ||
            (src[d].LongTermFrameIdx == 0xffff && dst[d].FrameNumWrap != src[d].FrameNumWrap) ||
            dst[d].PicType != src[d].PicType)
            return d;
    if (d < PpsDPBSize && dst[d].Index != 0xffff)
        return d;
    return PpsDPBSize;
}

mfxStatus PrepareCtrlBuf(mfxExtFeiEncFrameCtrl* & fei_encode_ctrl, const mfxVideoParam& vpar)
{
    mfxU32 nfields = (vpar.mfx.FrameInfo.PicStruct != MFX_PICSTRUCT_PROGRESSIVE) ? 2 : 1;
    if (!fei_encode_ctrl)
        fei_encode_ctrl = (mfxExtFeiEncFrameCtrl*) new mfxU8 [sizeof(mfxExtFeiEncFrameCtrl) * nfields];
    memset(fei_encode_ctrl, 0, sizeof(*fei_encode_ctrl) * nfields);

    for (mfxU32 field = 0; field < nfields; field++)
    {
        fei_encode_ctrl[field].Header.BufferId = MFX_EXTBUFF_FEI_ENC_CTRL;
        fei_encode_ctrl[field].Header.BufferSz = sizeof(mfxExtFeiEncFrameCtrl);

        fei_encode_ctrl[field].SearchPath             = 2;
        fei_encode_ctrl[field].LenSP                  = 57;
        fei_encode_ctrl[field].SubMBPartMask          = 0;
        fei_encode_ctrl[field].MultiPredL0            = 0;
        fei_encode_ctrl[field].MultiPredL1            = 0;
        fei_encode_ctrl[field].SubPelMode             = 0x03;
        fei_encode_ctrl[field].InterSAD               = 0;
        fei_encode_ctrl[field].IntraSAD               = 0;
        fei_encode_ctrl[field].IntraPartMask          = 0;
        fei_encode_ctrl[field].DistortionType         = 0;
        fei_encode_ctrl[field].RepartitionCheckEnable = 0;
        fei_encode_ctrl[field].AdaptiveSearch         = 0;
        fei_encode_ctrl[field].MVPredictor            = 0;
        fei_encode_ctrl[field].NumMVPredictors[0]     = 0;
        fei_encode_ctrl[field].PerMBQp                = 26;
        fei_encode_ctrl[field].PerMBInput             = 0;
        fei_encode_ctrl[field].MBSizeCtrl             = 0;
        fei_encode_ctrl[field].ColocatedMbDistortion  = 0;
        fei_encode_ctrl[field].RefHeight              = 32;
        fei_encode_ctrl[field].RefWidth               = 32;
        fei_encode_ctrl[field].SearchWindow           = 5;
    }

    return MFX_ERR_NONE;
}

mfxStatus PrepareSPSBuf(mfxExtFeiSPS* & fsps, const mfxVideoParam& vpar)
{
    if (!fsps)
        fsps = (mfxExtFeiSPS*) new mfxU8 [sizeof(mfxExtFeiSPS)];

    memset(fsps, 0, sizeof(*fsps));
    fsps->Header.BufferId = MFX_EXTBUFF_FEI_SPS;
    fsps->Header.BufferSz = sizeof(mfxExtFeiSPS);

    fsps->SPSId = DEFAULT_IDC;
    fsps->PicOrderCntType = 0;
    fsps->Log2MaxPicOrderCntLsb = 4;

    return MFX_ERR_NONE;
}

mfxStatus PreparePPSBuf(mfxExtFeiPPS* & fpps, const mfxVideoParam& vpar)
{
    mfxU32 nfields = (vpar.mfx.FrameInfo.PicStruct != MFX_PICSTRUCT_PROGRESSIVE) ? 2 : 1;
    if (!fpps)
        fpps = (mfxExtFeiPPS*) new mfxU8 [sizeof(mfxExtFeiPPS) * nfields];

    memset(fpps, 0, sizeof(*fpps));
    fpps->Header.BufferId = MFX_EXTBUFF_FEI_PPS;
    fpps->Header.BufferSz = sizeof(mfxExtFeiPPS);

    fpps->SPSId = DEFAULT_IDC;
    fpps->PPSId = DEFAULT_IDC;

    fpps->FrameType = MFX_FRAMETYPE_I;
    fpps->PictureType = MFX_PICTYPE_FRAME;

    fpps->PicInitQP = 26;
    fpps->NumRefIdxL0Active = 0;
    fpps->NumRefIdxL1Active = 0;
    for(int i=0; i<16; i++) {
        fpps->DpbBefore[i].Index = 0xffff;
        fpps->DpbBefore[i].PicType = 0;
        fpps->DpbBefore[i].FrameNumWrap = 0;
        fpps->DpbBefore[i].LongTermFrameIdx = 0;
    }
    CopyPpsDPB(fpps->DpbBefore, fpps->DpbAfter);

    //fpps->ChromaQPIndexOffset;
    //fpps->SecondChromaQPIndexOffset;

    fpps->Transform8x8ModeFlag = 1;

    if (nfields == 2) // the same for 2nd field
        fpps[1] = fpps[0];

    return MFX_ERR_NONE;
}

mfxStatus PrepareParamBuf(mfxExtFeiParam* & fpar, const mfxVideoParam& vpar, mfxFeiFunction func)
{
    if (!fpar)
        fpar = (mfxExtFeiParam*) new mfxU8 [sizeof(mfxExtFeiParam)];

    memset(fpar, 0, sizeof(*fpar));
    fpar->Header.BufferId = MFX_EXTBUFF_FEI_PARAM;
    fpar->Header.BufferSz = sizeof(mfxExtFeiParam);
    fpar->Func = func;
    fpar->SingleFieldProcessing = /*(*/(vpar.mfx.FrameInfo.PicStruct & MFX_PICSTRUCT_PROGRESSIVE) /* || !m_bSingleField)*/ ? MFX_CODINGOPTION_OFF : MFX_CODINGOPTION_ON;

    return MFX_ERR_NONE;
}


mfxStatus PrepareSliceBuf(mfxExtFeiSliceHeader* & fslice, const mfxVideoParam& vpar)
{
    mfxU32 nfields = (vpar.mfx.FrameInfo.PicStruct != MFX_PICSTRUCT_PROGRESSIVE) ? 2 : 1;
    if (!fslice)
        fslice = (mfxExtFeiSliceHeader*) new mfxU8 [sizeof(mfxExtFeiSliceHeader) * nfields];
    else
        SAFE_DELETE_ARRAY (fslice->Slice)

    memset(fslice, 0, sizeof(*fslice) * nfields);
    for (mfxU32 field = 0; field < nfields; field++)
    {
        fslice[field].Header.BufferId = MFX_EXTBUFF_FEI_SLICE;
        fslice[field].Header.BufferSz = sizeof(mfxExtFeiSliceHeader);

        mfxI32 wmb = (vpar.mfx.FrameInfo.Width+15)>>4;
        mfxI32 hmb = (vpar.mfx.FrameInfo.Height+15)>>4;
        if (vpar.mfx.FrameInfo.PicStruct != MFX_PICSTRUCT_PROGRESSIVE)
            hmb = (hmb+1) >> 1;
        mfxI32 nummb = wmb*hmb;
        mfxU16 numSlice = vpar.mfx.NumSlice;
        if (numSlice == 0 || numSlice > nummb) numSlice = 1;

        fslice[field].Slice = new mfxExtFeiSliceHeader::mfxSlice[numSlice];
        memset(fslice[field].Slice, 0, sizeof(*fslice[field].Slice) * numSlice);
        fslice[field].NumSlice = numSlice;

        mfxU16 firstmb = 0;
        for (mfxI32 s = 0; s < numSlice; s++) {
            mfxExtFeiSliceHeader::mfxSlice &slice = fslice[field].Slice[s];
            slice.SliceType = 2; //FEI_SLICETYPE_I;
            slice.PPSId = DEFAULT_IDC;
            slice.IdrPicId = DEFAULT_IDC;
            slice.CabacInitIdc = 0;
            slice.NumRefIdxL0Active = 0;
            slice.NumRefIdxL1Active = 0;
            slice.SliceQPDelta = 0;
            slice.DisableDeblockingFilterIdc = 0;
            slice.SliceAlphaC0OffsetDiv2 = 0;
            slice.SliceBetaOffsetDiv2 = 0;
            // RefL0/L1 ignored

            slice.MBAddress = firstmb;
            mfxI32 lastmb = nummb*(s+1)/numSlice;
            slice.NumMBs = lastmb - firstmb;
            firstmb = lastmb;
            if (firstmb >= nummb) {
                fslice[field].NumSlice = numSlice = s+1;
                break;
            }
        }
        fslice[field].Slice[numSlice-1].NumMBs = nummb - fslice[field].Slice[numSlice-1].MBAddress; // eon of last slice is last MB
    }

    return MFX_ERR_NONE;
}

mfxStatus PrepareEncMVBuf(mfxExtFeiEncMV & mv, const mfxVideoParam& vpar)
{
    SAFE_DELETE_ARRAY(mv.MB);
    memset(&mv, 0, sizeof(mv));
    mv.Header.BufferId = MFX_EXTBUFF_FEI_ENC_MV;
    mv.Header.BufferSz = sizeof(mfxExtFeiEncMV);

    mfxI32 wmb = (vpar.mfx.FrameInfo.Width+15)>>4;
    mfxI32 hmb = (vpar.mfx.FrameInfo.Height+15)>>4;
    if (vpar.mfx.FrameInfo.PicStruct != MFX_PICSTRUCT_PROGRESSIVE)
        hmb = (hmb+1) >> 1;
    mfxI32 nummb = wmb*hmb;

    mv.MB = new mfxExtFeiEncMV::mfxExtFeiEncMVMB[nummb];
    memset(mv.MB, 0, sizeof(*mv.MB) * nummb);
    mv.NumMBAlloc = nummb;

    return MFX_ERR_NONE;
}

mfxStatus PreparePakMBCtrlBuf(mfxExtFeiPakMBCtrl & mb, const mfxVideoParam& vpar)
{
    SAFE_DELETE_ARRAY(mb.MB);
    memset(&mb, 0, sizeof(mb));
    mb.Header.BufferId = MFX_EXTBUFF_FEI_PAK_CTRL;
    mb.Header.BufferSz = sizeof(mfxExtFeiPakMBCtrl);

    mfxI32 wmb = (vpar.mfx.FrameInfo.Width+15)>>4;
    mfxI32 hmb = (vpar.mfx.FrameInfo.Height+15)>>4;
    if (vpar.mfx.FrameInfo.PicStruct != MFX_PICSTRUCT_PROGRESSIVE)
        hmb = (hmb+1) >> 1;
    mfxI32 nummb = wmb*hmb;

    mb.MB = new mfxFeiPakMBCtrl[nummb];
    memset(mb.MB, 0, sizeof(*mb.MB) * nummb);
    mb.NumMBAlloc = nummb;

    return MFX_ERR_NONE;
}

// The function deletes buffers and zeroes pointers in array. Todo get rid of it
mfxStatus tsVideoENCPAK::Close()
{
    m_enc_pool.FreeSurfaces();
    m_rec_pool.FreeSurfaces();

    SAFE_DELETE_ARRAY(m_bitstream.Data);
    mfxU32 nfields = (enc.m_pPar->mfx.FrameInfo.PicStruct != MFX_PICSTRUCT_PROGRESSIVE) ? 2 : 1;
    for (mfxU32 field = 0; field < nfields; field++) {
        SAFE_DELETE_ARRAY(m_mb[field].MB);
        SAFE_DELETE_ARRAY(m_mv[field].MB);
    }

    SAFE_DELETE_ARRAY(enc.fctrl);
    SAFE_DELETE_ARRAY(enc.fpps);
    SAFE_DELETE_ARRAY(enc.fsps);
    SAFE_DELETE_ARRAY(enc.fpar);
    if (enc.fslice) {
        SAFE_DELETE_ARRAY(enc.fslice->Slice);
        SAFE_DELETE_ARRAY(enc.fslice);
    }

    SAFE_DELETE_ARRAY(pak.fctrl);
    SAFE_DELETE_ARRAY(pak.fpps);
    SAFE_DELETE_ARRAY(pak.fsps);
    SAFE_DELETE_ARRAY(pak.fpar);
    if (pak.fslice) {
        SAFE_DELETE_ARRAY(pak.fslice->Slice);
        SAFE_DELETE_ARRAY(pak.fslice);
    }

    m_initialized = false;

    return Close(m_session);
}

tsVideoENCPAK::tsVideoENCPAK(mfxFeiFunction funcEnc, mfxFeiFunction funcPak, mfxU32 CodecId, bool useDefaults)
    : m_default(useDefaults)
    , m_initialized(false)
    , m_loaded(false)
    , m_bSingleField(true)
    , enc(funcEnc)
    , pak(funcPak)
    , m_enc_pool()
    , m_rec_pool()
    , m_enc_request()
    , m_rec_request()
    , m_bitstream()
    , m_pBitstream(&m_bitstream)
    , m_bs_processor(0)
    , m_pSyncPoint(&m_syncpoint)
    , m_filler(0)
    , m_uid(0)
    , m_ENCInput(&ENCInput)
    , m_ENCOutput(&ENCOutput)
    , m_PAKInput(&PAKInput)
    , m_PAKOutput(&PAKOutput)
{
    memset(&ENCInput, 0, sizeof(ENCInput));
    memset(&ENCOutput, 0, sizeof(ENCOutput));
    memset(&PAKInput, 0, sizeof(PAKInput));
    memset(&PAKOutput, 0, sizeof(PAKOutput));
    memset(&m_mb, 0, sizeof(m_mb));
    memset(&m_mv, 0, sizeof(m_mv));
    for (int i=0; i<PpsDPBSize; i++) nextDpb[i].Index = 0xffff;

    if(m_default)
    {
        enc.m_par.mfx.FrameInfo.Width  = enc.m_par.mfx.FrameInfo.CropW = 720;
        enc.m_par.mfx.FrameInfo.Height = enc.m_par.mfx.FrameInfo.CropH = 576;
        enc.m_par.mfx.FrameInfo.FourCC        = MFX_FOURCC_NV12;
        enc.m_par.mfx.FrameInfo.ChromaFormat  = MFX_CHROMAFORMAT_YUV420;
        enc.m_par.mfx.FrameInfo.PicStruct     = MFX_PICSTRUCT_PROGRESSIVE;
        enc.m_par.mfx.FrameInfo.FrameRateExtN = 30;
        enc.m_par.mfx.FrameInfo.FrameRateExtD = 1;
        enc.m_par.mfx.RateControlMethod = MFX_RATECONTROL_CQP;
        enc.m_par.mfx.QPI = enc.m_par.mfx.QPP = enc.m_par.mfx.QPB = 26;
        enc.m_par.IOPattern        = MFX_IOPATTERN_IN_VIDEO_MEMORY;
        enc.m_par.mfx.EncodedOrder = 1;
        enc.m_par.AsyncDepth       = 1;

        pak.m_par.mfx = enc.m_par.mfx;
        pak.m_par.IOPattern = enc.m_par.IOPattern;
        pak.m_par.mfx.EncodedOrder = 1;
        pak.m_par.AsyncDepth       = 1;
    }

    pak.m_par.mfx.CodecId = enc.m_par.mfx.CodecId = CodecId;

    m_loaded = true;
}

tsVideoENCPAK::~tsVideoENCPAK()
{
    if(m_initialized)
    {
        Close();
    }
}

mfxStatus tsVideoENCPAK::Init()
{
    if(m_default)
    {
        if (!m_session)
        {
            MFXInit();TS_CHECK_MFX;
        }
        if (!m_loaded)
        {
            Load();
        }

        m_BaseAllocID = (mfxU64)&m_session & 0xffffffff;
        m_EncPakReconAllocID = m_BaseAllocID + 0; // TODO remove this var

        enc.m_par.AllocId = m_EncPakReconAllocID;
        pak.m_par.AllocId = m_EncPakReconAllocID;

        if (!m_pFrameAllocator
            && (   (m_enc_request.Type & (MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET|MFX_MEMTYPE_VIDEO_MEMORY_PROCESSOR_TARGET))
                || (enc.m_par.IOPattern & MFX_IOPATTERN_IN_VIDEO_MEMORY)))
        {
            m_pFrameAllocator = m_pVAHandle;
            SetFrameAllocator(); TS_CHECK_MFX;
            m_enc_pool.SetAllocator(m_pFrameAllocator, true);
            m_rec_pool.SetAllocator(m_pFrameAllocator, true);
        }
    }

    AllocSurfaces();

    enc.m_par.ExtParam     = enc.initbuf.data();
    enc.m_par.NumExtParam  = (mfxU16)enc.initbuf.size();
    pak.m_par.ExtParam     = pak.initbuf.data();
    pak.m_par.NumExtParam  = (mfxU16)pak.initbuf.size();

    mfxStatus sts = Init(m_session);
    if (sts < MFX_ERR_NONE)
        return sts;

    mfxU32 nfields = (enc.m_pPar->mfx.FrameInfo.PicStruct != MFX_PICSTRUCT_PROGRESSIVE) ? 2 : 1;
    for (mfxU32 field = 0; field < nfields; field++) {
        PreparePakMBCtrlBuf(m_mb[field], *enc.m_pPar);
        PrepareEncMVBuf(m_mv[field], *enc.m_pPar);
    }

    m_initialized = true;
    GetVideoParam(); // to get modifications if any

    recSet.resize(m_rec_pool.PoolSize());
    refInfoSet.resize(m_rec_pool.PoolSize());

    for (mfxU32 i = 0; i < m_rec_pool.PoolSize(); i++) {
        recSet[i] = m_rec_pool.GetSurface(i);
        refInfoSet[i].frame = m_rec_pool.GetSurface(i);
        refInfoSet[i].LTidx = 0xffff;
        refInfoSet[i].avail[1] = refInfoSet[i].avail[0] = false;
    }

    return sts;
}

mfxStatus tsVideoENCPAK::Init(mfxSession session/*, mfxVideoParam *par*/)
{
    mfxVideoParam orig_par, *par;

    // ENC
    par = enc.m_pPar;
    if (par)
        memcpy(&orig_par, par, sizeof(mfxVideoParam));

    TRACE_FUNC2(MFXVideoENC_Init, session, par);
    g_tsStatus.check( MFXVideoENC_Init(session, par) );

    mfxStatus enc_status = g_tsStatus.get();

    if (par)
        EXPECT_EQ(0, memcmp(&orig_par, par, sizeof(mfxVideoParam)))
            << "ERROR: Input parameters must not be changed in Enc Init()";

    if (enc_status < 0) return enc_status;

    // PAK
    par = pak.m_pPar;
    if (par)
        memcpy(&orig_par, par, sizeof(mfxVideoParam));

    TRACE_FUNC2(MFXVideoPAK_Init, session, par);
    g_tsStatus.check( MFXVideoPAK_Init(session, par) );

    mfxStatus pak_status = g_tsStatus.get();

    if (par)
        EXPECT_EQ(0, memcmp(&orig_par, par, sizeof(mfxVideoParam)))
            << "ERROR: Input parameters must not be changed in Pak Init()";

    if (pak_status < 0) return pak_status;

    return enc_status == 0 ? pak_status : enc_status; // first warning
}

mfxStatus tsVideoENCPAK::Close(mfxSession session)
{
    TRACE_FUNC1(MFXVideoENC_Close, session);
    g_tsStatus.check( MFXVideoENC_Close(session) );

    TRACE_FUNC1(MFXVideoPAK_Close, session);
    g_tsStatus.check( MFXVideoPAK_Close(session) );

    return g_tsStatus.get();
}

mfxStatus tsVideoENCPAK::Query()
{
    if(m_default)
    {
        if(!m_session)
        {
            MFXInit();
        }
        if(!m_loaded)
        {
            Load();
        }
    }

    //return Query(m_session, m_pPar, m_pParOut);
    TRACE_FUNC3(MFXVideoENC_Query, m_session, enc.m_pPar, enc.m_pParOut);
    g_tsStatus.check( MFXVideoENC_Query(m_session, enc.m_pPar, enc.m_pParOut) );

    // check if PAK is needed
    TRACE_FUNC3(MFXVideoPAK_Query, m_session, pak.m_pPar, pak.m_pParOut);
    g_tsStatus.check( MFXVideoPAK_Query(m_session, pak.m_pPar, pak.m_pParOut) );

    TS_TRACE(enc.m_pParOut);
    TS_TRACE(pak.m_pParOut);

    return g_tsStatus.get();
}


mfxStatus tsVideoENCPAK::QueryIOSurf()
{
    if(m_default)
    {
        if(!m_session)
        {
            MFXInit();
        }
        if(!m_loaded)
        {
            Load();
        }
    }

    m_enc_request.AllocId           = m_BaseAllocID;
    m_enc_request.Info              = enc.m_pPar->mfx.FrameInfo;
    m_enc_request.NumFrameMin       = enc.m_pPar->mfx.GopRefDist + enc.m_pPar->AsyncDepth; // what if !progressive?
    m_enc_request.NumFrameSuggested = m_enc_request.NumFrameMin;
    m_enc_request.Type              = MFX_MEMTYPE_EXTERNAL_FRAME | MFX_MEMTYPE_VIDEO_MEMORY_PROCESSOR_TARGET;
    m_enc_request.Type              |= (MFX_MEMTYPE_FROM_ENC | MFX_MEMTYPE_FROM_PAK);
    TS_TRACE(m_enc_request);

    m_rec_request.AllocId           = m_EncPakReconAllocID;
    m_rec_request.Info              = pak.m_pPar->mfx.FrameInfo;
    m_rec_request.NumFrameMin       = pak.m_pPar->mfx.GopRefDist * 2 + (pak.m_pPar->AsyncDepth-1) + 1 + pak.m_pPar->mfx.NumRefFrame + 1;
    m_rec_request.NumFrameSuggested = m_rec_request.NumFrameMin;
    m_rec_request.Type              = MFX_MEMTYPE_INTERNAL_FRAME | MFX_MEMTYPE_VIDEO_MEMORY_PROCESSOR_TARGET;
    m_rec_request.Type              |= (MFX_MEMTYPE_FROM_ENC | MFX_MEMTYPE_FROM_PAK);
    TS_TRACE(m_rec_request);

    return g_tsStatus.get();
}

mfxStatus tsVideoENCPAK::Reset()
{
    TRACE_FUNC2(MFXVideoENC_Reset, m_session, enc.m_pPar);
    g_tsStatus.check( MFXVideoENC_Reset(m_session, enc.m_pPar) );

    TRACE_FUNC2(MFXVideoPAK_Reset, m_session, pak.m_pPar);
    g_tsStatus.check( MFXVideoPAK_Reset(m_session, pak.m_pPar) );

    return g_tsStatus.get();
}

mfxStatus tsVideoENCPAK::AllocBitstream(mfxU32 size)
{
    if(!size)
    {
        if(enc.m_par.mfx.CodecId == MFX_CODEC_JPEG)
        {
            size = TS_MAX((enc.m_par.mfx.FrameInfo.Width*enc.m_par.mfx.FrameInfo.Height), 1000000);
        }
        else
        {
            if(!enc.m_par.mfx.BufferSizeInKB)
            {
                GetVideoParam();TS_CHECK_MFX;
            }
            size = enc.m_par.mfx.BufferSizeInKB * TS_MAX(enc.m_par.mfx.BRCParamMultiplier, 1) * 1000 * TS_MAX(enc.m_par.AsyncDepth, 1);
        }
    }

    g_tsLog << "ALLOC BITSTREAM OF SIZE " << size << "\n";

    m_bitstream.Data = new mfxU8[size];
    m_bitstream.MaxLength = size;

    return g_tsStatus.get();
}

mfxStatus tsVideoENCPAK::GetVideoParam()
{
    if(m_default && !m_initialized)
    {
        Init();TS_CHECK_MFX;
    }
    TRACE_FUNC2(MFXVideoENC_GetVideoParam, m_session, enc.m_pPar);
    g_tsStatus.check( MFXVideoENC_GetVideoParam(m_session, enc.m_pPar) );
    TS_TRACE(enc.m_pPar);

    TRACE_FUNC2(MFXVideoPAK_GetVideoParam, m_session, pak.m_pPar);
    g_tsStatus.check( MFXVideoPAK_GetVideoParam(m_session, pak.m_pPar) );
    TS_TRACE(pak.m_pPar);

    return g_tsStatus.get();
}

mfxStatus tsVideoENCPAK::PrepareInitBuffers()
{
    enc.initbuf.clear();
    pak.initbuf.clear();

    PrepareParamBuf(enc.fpar, enc.m_par, enc.Func);
    enc.initbuf.push_back(&enc.fpar->Header);
    PrepareParamBuf(pak.fpar, pak.m_par, pak.Func);
    pak.initbuf.push_back(&pak.fpar->Header);

    PrepareSPSBuf(enc.fsps, enc.m_par);
    enc.initbuf.push_back(&enc.fsps->Header);
    PrepareSPSBuf(pak.fsps, pak.m_par);
    pak.initbuf.push_back(&pak.fsps->Header);

    // single pps for interlace too
    PreparePPSBuf(enc.fpps, enc.m_par);
    enc.initbuf.push_back(&enc.fpps->Header);
    PreparePPSBuf(pak.fpps, pak.m_par);
    pak.initbuf.push_back(&pak.fpps->Header);

    return MFX_ERR_NONE;
}

mfxStatus tsVideoENCPAK::CreatePpsDPB  (const std::vector<refInfo>& dpb, mfxExtFeiPPS::mfxExtFeiPpsDPB *pd, bool after2nd)
{
    for (mfxU32 r=0, d=0; d < PpsDPBSize; r++) {
        if (r >= dpb.size()) {
            pd[d].Index = 0xffff;
            d++;
            continue;
        }
        const mfxFrameSurface1* surf = dpb[r].frame;
        assert(surf == recSet[r]);
        mfxU16 refType = 0;
        if (dpb[r].avail[0])
            refType |= MFX_PICTYPE_TOPFIELD;
        if (dpb[r].avail[1])
            refType |= MFX_PICTYPE_BOTTOMFIELD;
//        if ( refType && surf->Info.PicStruct == MFX_PICSTRUCT_PROGRESSIVE)
//            refType |= MFX_PICTYPE_FRAME; // this way fails!
        if (!refType)
            continue;
        pd[d].Index = r;
        pd[d].PicType = refType;
        pd[d].FrameNumWrap = surf->Data.FrameOrder; // simplified
        pd[d].LongTermFrameIdx = dpb[r].LTidx;
        d++;
    }

    return MFX_ERR_NONE;
}

// update DPB - simplest way for a while
mfxStatus tsVideoENCPAK::UpdateDPB (std::vector<refInfo>& dpb, bool secondField)
{
    const mfxU32 curField = secondField; // to avoid confusing
    mfxU16 type = enc.fpps[curField].FrameType;
    if (type & MFX_FRAMETYPE_REF) {

        mfxU16 count = 0, minOrder = 0xffff, minOrderIdx = 0;
        bool currIsInDPB = false;
        if (type & MFX_FRAMETYPE_IDR) {
            for (mfxU32 r=0; r<dpb.size(); r++) {
                dpb[r].avail[1] = dpb[r].avail[0] = 0;
                dpb[r].LTidx = 0xffff;
            }
        } else {
            for (mfxU32 r=0; r<dpb.size(); r++) {
                if (!dpb[r].avail[1] && !dpb[r].avail[0])
                    continue; // unused
                if (dpb[r].frame == m_PAKOutput->OutSurface) {
                    currIsInDPB = true;
                    break; // no need to continue - dpb won't change
                }
                count ++;
                if (dpb[r].LTidx == 0xffff && dpb[r].frame->Data.FrameOrder < minOrder) { // not count LT
                    minOrder = dpb[r].frame->Data.FrameOrder;
                    minOrderIdx = r;
                }
            }
        }

        mfxU16 idx = static_cast<mfxU16>(std::distance(recSet.begin(), std::find(recSet.begin(), recSet.end(), m_PAKOutput->OutSurface)));

        if (m_PAKOutput->OutSurface->Info.PicStruct & MFX_PICSTRUCT_PROGRESSIVE) {
            dpb[idx].avail[0] = dpb[idx].avail[1] = true;
        } else {
            mfxU32 fieldParity = !!(m_PAKOutput->OutSurface->Info.PicStruct & MFX_PICSTRUCT_FIELD_BFF);
            dpb[idx].avail[secondField ^ fieldParity] = true;
        }
        if (!secondField)
            dpb[idx].LTidx = 0xffff;

        if (!currIsInDPB && count == enc.m_pPar->mfx.NumRefFrame) {
            dpb[minOrderIdx].avail[1] = dpb[minOrderIdx].avail[0] = false;
            //dpb[minOrderIdx].LTidx = 0xffff;
        }
    }

    return MFX_ERR_NONE;
}

mfxStatus tsVideoENCPAK::PrepareDpbBuffers (bool secondField)
{
    const mfxU32 curField = secondField; // to avoid confusing

    CreatePpsDPB(refInfoSet, enc.fpps[curField].DpbBefore, false && secondField); // && for hint

    // test error if differ
    if (ComparePpsDPB(enc.fpps[curField].DpbBefore, nextDpb) < PpsDPBSize )
        return MFX_ERR_UNDEFINED_BEHAVIOR;

    std::vector<refInfo> _refs = refInfoSet; // make local copy to see the future
    UpdateDPB(_refs, secondField);

    CreatePpsDPB(_refs, enc.fpps[curField].DpbAfter, true && secondField); // && for hint
    CopyPpsDPB(enc.fpps[curField].DpbAfter, nextDpb); // will check if test modified DpbAfter

    CopyPpsDPB(enc.fpps[curField].DpbBefore, pak.fpps[curField].DpbBefore);
    CopyPpsDPB(enc.fpps[curField].DpbAfter, pak.fpps[curField].DpbAfter);

    return MFX_ERR_NONE;
}

// is called for each field, but every call fills for both fields
mfxStatus tsVideoENCPAK::PrepareFrameBuffers (bool secondField)
{
    const mfxU32 curField = secondField; // to avoid confusing
    if (!secondField) {
        if (!m_enc_pool.PoolSize()) // shouldn't go into
        {
            if (m_pFrameAllocator && !m_enc_pool.GetAllocator())
            {
                m_enc_pool.SetAllocator(m_pFrameAllocator, true);
            }
            if (!m_pFrameAllocator && (m_enc_request.Type & (MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET|MFX_MEMTYPE_VIDEO_MEMORY_PROCESSOR_TARGET)))
            {
                m_pFrameAllocator = m_enc_pool.GetAllocator();
                SetFrameAllocator();TS_CHECK_MFX;
            }
            if (m_pFrameAllocator && !m_rec_pool.GetAllocator())
            {
                m_rec_pool.SetAllocator(m_pFrameAllocator, true);
            }
            AllocSurfaces();TS_CHECK_MFX;
        }

        if(!m_initialized)
        {
            Init();TS_CHECK_MFX;
        }

        if(!m_bitstream.MaxLength)
        {
            AllocBitstream();TS_CHECK_MFX;
        }

        // before request from reconstruct pool, update locked flags according to current refs
        for (mfxU32 r=0; r<refInfoSet.size(); r++) {
            if (!refInfoSet[r].avail[1] && !refInfoSet[r].avail[0]) {
                refInfoSet[r].frame->Data.Locked = 0;
            } else {
                refInfoSet[r].frame->Data.Locked = 1;
            }
        }

        m_PAKInput->InSurface   = m_ENCInput->InSurface   = m_enc_pool.GetSurface(); TS_CHECK_MFX;
        m_ENCOutput->OutSurface = m_PAKOutput->OutSurface = m_rec_pool.GetSurface(); TS_CHECK_MFX;
        if (m_filler)
            m_PAKInput->InSurface = m_ENCInput->InSurface = m_filler->ProcessSurface(m_PAKInput->InSurface, m_pFrameAllocator);

        // to compute dpb_after, FrameOrder is used to emulate FrameNumWrap
        m_PAKOutput->OutSurface->Data.FrameOrder = m_PAKInput->InSurface->Data.FrameOrder;

        m_PAKOutput->Bs = m_pBitstream; // once for both fields?
    }

    m_PAKInput->L0Surface  = m_ENCInput->L0Surface  = recSet.data();
    m_PAKInput->NumFrameL0 = m_ENCInput->NumFrameL0 = recSet.size();

    // refill all buffers for each field
    enc.inbuf.clear();
    enc.outbuf.clear();
    pak.inbuf.clear();
    pak.outbuf.clear();

    mfxU32 nfields = (enc.m_pPar->mfx.FrameInfo.PicStruct != MFX_PICSTRUCT_PROGRESSIVE) ? 2 : 1;
    for (mfxU32 field = (m_bSingleField && secondField) ? 1 : 0; field < ((!m_bSingleField || secondField) ? nfields : 1); field++)
    {
        PrepareSliceBuf(enc.fslice, enc.m_par);
        PrepareCtrlBuf(enc.fctrl, enc.m_par);
        PreparePPSBuf(enc.fpps, enc.m_par);
        PrepareSliceBuf(pak.fslice, pak.m_par);
        PrepareCtrlBuf(pak.fctrl, pak.m_par);
        PreparePPSBuf(pak.fpps, pak.m_par);

        enc.inbuf.push_back(&enc.fpps[field].Header);
        enc.inbuf.push_back(&enc.fslice[field].Header);
        enc.inbuf.push_back(&enc.fctrl[field].Header);
        enc.outbuf.push_back(&m_mb[field].Header);
        enc.outbuf.push_back(&m_mv[field].Header);

        pak.inbuf.push_back(&pak.fpps[field].Header);
        pak.inbuf.push_back(&pak.fslice[field].Header);
        pak.inbuf.push_back(&m_mb[field].Header);
        pak.inbuf.push_back(&m_mv[field].Header);
    }


    mfxU16 frtype = 0, slicetype = 0;
    mfxI32 order = m_ENCInput->InSurface->Data.FrameOrder;
    mfxI32 goporder = order % enc.m_pPar->mfx.GopPicSize;
    if (goporder == 0)
    {
        if (!secondField)
        {
            frtype = MFX_FRAMETYPE_I | MFX_FRAMETYPE_REF;
            slicetype = 2; //FEI_SLICETYPE_I;
            if (enc.m_pPar->mfx.IdrInterval <= 1 || order / enc.m_pPar->mfx.GopPicSize % enc.m_pPar->mfx.IdrInterval == 0)
                frtype |= MFX_FRAMETYPE_IDR;
        }
        else
        {
            frtype = MFX_FRAMETYPE_P | MFX_FRAMETYPE_REF;
            slicetype = 0; //FEI_SLICETYPE_P;
        }
    }
    else if (goporder % enc.m_pPar->mfx.GopRefDist == 0) {
        frtype = MFX_FRAMETYPE_P | MFX_FRAMETYPE_REF;
        slicetype = 0; //FEI_SLICETYPE_P;
    } else {
        frtype = MFX_FRAMETYPE_B;
        slicetype = 1; //FEI_SLICETYPE_B;
    }

    mfxFrameInfo &info = m_ENCInput->InSurface->Info;
    mfxU16 pictype = info.PicStruct == MFX_PICSTRUCT_PROGRESSIVE ? MFX_PICTYPE_FRAME :
        ((info.PicStruct == MFX_PICSTRUCT_FIELD_TFF) ?
        (!secondField ? MFX_PICTYPE_TOPFIELD : MFX_PICTYPE_BOTTOMFIELD) :
        ( secondField ? MFX_PICTYPE_TOPFIELD : MFX_PICTYPE_BOTTOMFIELD));

    pak.fpps[curField].FrameType   = enc.fpps[curField].FrameType   = frtype;
    pak.fpps[curField].PictureType = enc.fpps[curField].PictureType = pictype;

    for (mfxI32 s=0; s<enc.fslice[curField].NumSlice; s++) {
        enc.fslice[curField].Slice[s].SliceType = slicetype;
        pak.fslice[curField].Slice[s].SliceType = slicetype;
    }

    mfxStatus sts = PrepareDpbBuffers(secondField);
    if (sts != MFX_ERR_NONE)
        return sts;
    FillRefLists(secondField);
    FillSliceRefs(secondField);

    // HACK to pass checking in lib: after 1st == before 2nd, 2nd frameType
    if (enc.m_pPar->mfx.FrameInfo.PicStruct != MFX_PICSTRUCT_PROGRESSIVE) {
        if (!secondField) {
            CopyPpsDPB(enc.fpps[0].DpbBefore, enc.fpps[1].DpbBefore);
            CopyPpsDPB(pak.fpps[0].DpbBefore, pak.fpps[1].DpbBefore);

            enc.fpps[1].FrameType = enc.fpps[0].FrameType;
            if (enc.fpps[1].FrameType & MFX_FRAMETYPE_I)
                enc.fpps[1].FrameType = (enc.fpps[1].FrameType &~ (MFX_FRAMETYPE_I | MFX_FRAMETYPE_IDR)) | MFX_FRAMETYPE_P;
            pak.fpps[1].FrameType = enc.fpps[1].FrameType;
        } else {
            CopyPpsDPB(enc.fpps[1].DpbBefore, enc.fpps[0].DpbBefore);
            CopyPpsDPB(pak.fpps[1].DpbBefore, pak.fpps[0].DpbBefore);

            enc.fpps[0].FrameType = enc.fpps[1].FrameType; // don't care, just to pass check
            pak.fpps[0].FrameType = enc.fpps[0].FrameType;
        }
    }

    return MFX_ERR_NONE;
}

void InsertRefListSorted (std::vector<RefListElem> RefList[2], RefListElem rle) // ref-current for ST
{
    // Note: acc to spec BWD follows FWD for L0 and vice versa for L1 and then LT. Don't care here.
    if (rle.LongTermFrameIdx == 0xffff) { // ST
        mfxI32 isL1 = rle.diffFrameOrder > 0; // ignore case when 2nd field is B-ref for first
        mfxI16 diffFrameOrder = std::abs(rle.diffFrameOrder);
        std::vector<RefListElem>::iterator it = RefList[isL1].begin();
        for ( ; it != RefList[isL1].end(); it++) {
            if ((*it).LongTermFrameIdx == 0xffff && std::abs((*it).diffFrameOrder) < diffFrameOrder )
                break;
        }
        RefList[isL1].insert(it, rle); // insert before smaller ST
    } else { // LT
        for (int isL1 = 0; isL1 <= 1; isL1 ++) {
            std::vector<RefListElem>::iterator it = RefList[isL1].begin();
            for ( ; it != RefList[isL1].end(); it++) {
                if ((*it).LongTermFrameIdx == 0xffff || (*it).LongTermFrameIdx < rle.LongTermFrameIdx )
                    break;
            }
            RefList[isL1].insert(it, rle); // insert before first ST or smaller LT
        }
    }

}

// map dpb to reconstruct pool, fill reflists from refs (dpb)
mfxStatus tsVideoENCPAK::FillRefLists(bool secondField)
{
    mfxFrameSurface1* input = m_PAKInput->InSurface;
    std::vector<RefListElem> LT;
    RefList[0].clear();
    RefList[1].clear();

    mfxU32 fieldParity = secondField ^ !!(input->Info.PicStruct & MFX_PICSTRUCT_FIELD_BFF); // 0 for top
    mfxU32 fieldMode = (input->Info.PicStruct != MFX_PICSTRUCT_PROGRESSIVE);

    for (mfxU16 idx=0; idx<refInfoSet.size(); idx++) {
        mfxFrameSurface1* surf = refInfoSet[idx].frame;

        // insert to reflists, same parity first
        mfxI16 diffPicNum = mfxI16 (!fieldMode ? surf->Data.FrameOrder - input->Data.FrameOrder :
                (surf->Data.FrameOrder*2 + 1) - (input->Data.FrameOrder*2 + 1));
        mfxU16 refType = surf->Info.PicStruct == MFX_PICSTRUCT_PROGRESSIVE ? MFX_PICTYPE_FRAME :
                (!fieldParity ? MFX_PICTYPE_TOPFIELD : MFX_PICTYPE_BOTTOMFIELD);
        mfxU16 longTermPicNum = refInfoSet[idx].LTidx;
        if (longTermPicNum != 0xffff && fieldMode)
            longTermPicNum = longTermPicNum*2 + 1;
        RefListElem rle = {idx, refType, longTermPicNum, diffPicNum};

        if (refInfoSet[idx].avail[fieldParity]) // if fist field present in the ref
            InsertRefListSorted(RefList, rle);  // insert frame or first field

        // insert opposite field for interlace if it is encoded
        if (fieldMode && refInfoSet[idx].avail[!fieldParity]) {
            rle.type ^= MFX_PICTYPE_TOPFIELD ^ MFX_PICTYPE_BOTTOMFIELD; // change to opposite
            rle.diffFrameOrder -= 1;
            if (rle.LongTermFrameIdx != 0xffff)
                rle.LongTermFrameIdx -= 1;
            InsertRefListSorted( RefList, rle);
        }
    }

    return MFX_ERR_NONE;
}

mfxStatus tsVideoENCPAK::FillSliceRefs(bool secondField)
{
    const mfxU32 curField = secondField; // to avoid confusing

    for (mfxI32 s=0; s<enc.fslice[curField].NumSlice; s++) {
        // latest first
        for (mfxU32 r=0; r<RefList[0].size(); r++) {
            pak.fslice[curField].Slice[s].RefL0[r].Index       = enc.fslice[curField].Slice[s].RefL0[r].Index       = RefList[0][RefList[0].size()-1 - r].dpb_idx;
            pak.fslice[curField].Slice[s].RefL0[r].PictureType = enc.fslice[curField].Slice[s].RefL0[r].PictureType = RefList[0][RefList[0].size()-1 - r].type;
        }
        for (mfxU32 r=0; r<RefList[1].size(); r++) {
            pak.fslice[curField].Slice[s].RefL1[r].Index       = enc.fslice[curField].Slice[s].RefL1[r].Index       = RefList[1][RefList[1].size()-1 - r].dpb_idx;
            pak.fslice[curField].Slice[s].RefL1[r].PictureType = enc.fslice[curField].Slice[s].RefL1[r].PictureType = RefList[1][RefList[1].size()-1 - r].type;
        }

        // TODO: Return *.fpps[curField].NumRefIdxL?Active back after relaxing of restrictions on encpak buffers parameters
        enc.fslice[curField].Slice[s].PPSId             = enc.fpps[curField].PPSId;
        enc.fslice[curField].Slice[s].NumRefIdxL0Active = RefList[0].size();
        enc.fslice[curField].Slice[s].NumRefIdxL1Active = RefList[1].size();
        pak.fslice[curField].Slice[s].PPSId             = pak.fpps[curField].PPSId;
        pak.fslice[curField].Slice[s].NumRefIdxL0Active = RefList[0].size();
        pak.fslice[curField].Slice[s].NumRefIdxL1Active = RefList[1].size();
    }
    return MFX_ERR_NONE;
}

mfxStatus tsVideoENCPAK::ProcessFrameAsync()
{

    m_ENCInput->ExtParam     = enc.inbuf.data();
    m_ENCInput->NumExtParam  = (mfxU16)enc.inbuf.size();
    m_ENCOutput->ExtParam    = enc.outbuf.data();
    m_ENCOutput->NumExtParam = (mfxU16)enc.outbuf.size();

    mfxStatus mfxRes = ProcessFrameAsync(m_session, m_ENCInput, m_ENCOutput, m_pSyncPoint);
    if (mfxRes != MFX_ERR_NONE)
        return mfxRes;

    mfxRes = SyncOperation(*m_pSyncPoint, 0);
    if (mfxRes != MFX_ERR_NONE)
        return mfxRes;

    m_PAKInput->ExtParam     = pak.inbuf.data();
    m_PAKInput->NumExtParam  = (mfxU16)pak.inbuf.size();
    m_PAKOutput->ExtParam    = pak.outbuf.data();
    m_PAKOutput->NumExtParam = (mfxU16)pak.outbuf.size();

    mfxRes = ProcessFrameAsync(m_session, m_PAKInput, m_PAKOutput, m_pSyncPoint);
    if (mfxRes != MFX_ERR_NONE)
        return mfxRes;

    mfxRes = SyncOperation(*m_pSyncPoint, m_bs_processor);
    if (mfxRes != MFX_ERR_NONE)
        return mfxRes;

    return MFX_ERR_NONE;
}

mfxStatus tsVideoENCPAK::ProcessFrameAsync(mfxSession session, mfxENCInput *in, mfxENCOutput *out, mfxSyncPoint *syncp)
{
    TRACE_FUNC4(MFXVideoENC_ProcessFrameAsync, session, in, out, syncp);
    mfxStatus mfxRes = MFXVideoENC_ProcessFrameAsync(session, in, out, syncp);
    TS_TRACE(mfxRes);
    TS_TRACE(in);
    TS_TRACE(out);
    TS_TRACE(syncp);

    return g_tsStatus.m_status = mfxRes;
}

mfxStatus tsVideoENCPAK::ProcessFrameAsync(mfxSession session, mfxPAKInput *in, mfxPAKOutput *out, mfxSyncPoint *syncp)
{
    TRACE_FUNC4(MFXVideoPAK_ProcessFrameAsync, session, in, out, syncp);
    mfxStatus mfxRes = MFXVideoPAK_ProcessFrameAsync(session, in, out, syncp);
    TS_TRACE(mfxRes);
    TS_TRACE(in);
    TS_TRACE(out);
    TS_TRACE(syncp);

    return g_tsStatus.m_status = mfxRes;
}


mfxStatus tsVideoENCPAK::SyncOperation()
{
    return SyncOperation(m_syncpoint, 0);
}

mfxStatus tsVideoENCPAK::SyncOperation(mfxSyncPoint syncp, tsBitstreamProcessor* bs_processor)
{
    mfxU32 nFrames = 1;
    mfxStatus res = SyncOperation(m_session, syncp, MFX_INFINITE);

    // for PAK only
    if (m_default && bs_processor && g_tsStatus.get() == MFX_ERR_NONE)
    {
        g_tsStatus.check(res = bs_processor->ProcessBitstream(m_pBitstream ? *m_pBitstream : m_bitstream, nFrames));
        TS_CHECK_MFX;
    }

    return res;
}

mfxStatus tsVideoENCPAK::SyncOperation(mfxSession session,  mfxSyncPoint syncp, mfxU32 wait)
{
    return tsSession::SyncOperation(session, syncp, wait);
}

mfxStatus tsVideoENCPAK::AllocSurfaces()
{
    if(m_default && !m_enc_request.NumFrameMin)
    {
        QueryIOSurf(); TS_CHECK_MFX;
    }

    m_enc_pool.AllocSurfaces(m_enc_request); TS_CHECK_MFX;
    return m_rec_pool.AllocSurfaces(m_rec_request);
}


mfxStatus tsVideoENCPAK::EncodeFrame(bool secondField)
{
    const mfxExtFeiPPS* pps = &enc.fpps[secondField];

    if (ComparePpsDPB(pps->DpbAfter, nextDpb) < PpsDPBSize) {
        // used mmco - modify internal refs
        mfxI32 dropped = 0;
        for (mfxU32 r=0; r<refInfoSet.size(); r++) {
            mfxU32 idxExpect = FindIdxInPpsDPB(nextDpb, r);
            mfxU32 idxAfter  = FindIdxInPpsDPB(pps->DpbAfter, r);
            const mfxExtFeiPPS::mfxExtFeiPpsDPB* expect = (idxExpect==PpsDPBSize) ? 0 : &nextDpb[idxExpect];
            const mfxExtFeiPPS::mfxExtFeiPpsDPB* after  = (idxAfter ==PpsDPBSize) ? 0 : &pps->DpbAfter[idxAfter];
            bool isCurrent = refInfoSet[r].frame == m_PAKOutput->OutSurface;
            if (expect) {
                if (!after) { // removed by mmco
                    refInfoSet[r].avail[1] = refInfoSet[r].avail[0] = 0;
                    refInfoSet[r].LTidx = 0xffff;
                    dropped++;
                } else { // still here, can be current, probably modified - update below
                    ;
                }
            } else if (after) { // appeared, not dropped by pipe, another must be dropped. Also update below.
                dropped--;
            }
            if (after) { //update
                refInfoSet[r].LTidx = after->LongTermFrameIdx; // ok for current too, shouldn't affect - TODO check it
                refInfoSet[r].avail[0] = !!(after->PicType & (MFX_PICTYPE_FRAME | MFX_PICTYPE_TOPFIELD));
                refInfoSet[r].avail[1] = !!(after->PicType & (MFX_PICTYPE_FRAME | MFX_PICTYPE_BOTTOMFIELD));
                if (isCurrent) { // clear current frame of field
                    if (secondField) { // clear for current field
                        mfxU32 curFieldIsBottom = !!(refInfoSet[r].frame->Info.PicStruct & MFX_PICSTRUCT_FIELD_TFF); // second field
                        refInfoSet[r].avail[curFieldIsBottom] = 0;
                    } else { // clear both
                        refInfoSet[r].avail[1] = refInfoSet[r].avail[0] = 0;
                    }
                }
            }
        }
        if (dropped < 0)
            return MFX_ERR_ABORTED; // or what?

        // Now rebuild reflists for (and) slice headers
        FillRefLists(secondField);
        FillSliceRefs(secondField);
    }

    mfxStatus sts = ProcessFrameAsync();
    if (sts != MFX_ERR_NONE)
        return sts;

    UpdateDPB(refInfoSet, secondField);

    CreatePpsDPB(refInfoSet, nextDpb, true && secondField); // to check before next frame // && for hint

    return sts;
}
