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

#include "ts_encoder.h"
#include "ts_aux_dev.h"

enum eEncoderFunction
{
      INIT
    , RESET
    , QUERY
    , QUERYIOSURF
};

void SkipDecision(mfxVideoParam& par, eEncoderFunction function)
{
    if (g_tsConfig.lowpower == MFX_CODINGOPTION_ON)
    {
        if (   par.mfx.GopRefDist > 1
            || (par.mfx.FrameInfo.PicStruct & (MFX_PICSTRUCT_FIELD_BFF|MFX_PICSTRUCT_FIELD_TFF)))
        {
            g_tsStatus.disable();
            throw tsSKIP;
        }

        if (par.mfx.CodecId == MFX_CODEC_AVC)
        {
            if (   par.mfx.RateControlMethod != 0
                && par.mfx.RateControlMethod != MFX_RATECONTROL_CBR
                && par.mfx.RateControlMethod != MFX_RATECONTROL_VBR
                && par.mfx.RateControlMethod != MFX_RATECONTROL_AVBR
                && par.mfx.RateControlMethod != MFX_RATECONTROL_QVBR
                && par.mfx.RateControlMethod != MFX_RATECONTROL_VCM
                && par.mfx.RateControlMethod != MFX_RATECONTROL_CQP
                && g_tsStatus.m_expected <= MFX_ERR_NONE)
            {
                g_tsStatus.disable();
                throw tsSKIP;
            }
        }
    }

    if (   par.mfx.CodecId == MFX_CODEC_AVC && g_tsHWtype >= HWType::MFX_HW_ICL
        && function != QUERYIOSURF)
    {
        mfxExtCodingOption2* CO2 = GetExtBufferPtr(par);
        mfxExtCodingOption3* CO3 = GetExtBufferPtr(par);
        mfxStatus expect = g_tsStatus.m_expected;
        bool unsupported = false;

        if (   (CO2 && CO2->MaxSliceSize && par.mfx.LowPower == MFX_CODINGOPTION_ON)
            || (CO3 && CO3->FadeDetection == MFX_CODINGOPTION_ON))
        {
            expect = MFX_WRN_INCOMPATIBLE_VIDEO_PARAM;
            unsupported = true;
        }

        if (   par.mfx.RateControlMethod == MFX_RATECONTROL_LA
            || par.mfx.RateControlMethod == MFX_RATECONTROL_LA_ICQ
            || par.mfx.RateControlMethod == MFX_RATECONTROL_LA_EXT
            || par.mfx.RateControlMethod == MFX_RATECONTROL_LA_HRD)
        {
            expect = MFX_ERR_UNSUPPORTED;
            unsupported = true;
        }

        if (unsupported)
        {
            if (function != QUERY && expect == MFX_ERR_UNSUPPORTED)
                expect = MFX_ERR_INVALID_VIDEO_PARAM;
            g_tsStatus.last();
            g_tsStatus.expect(expect);
        }
    }

    if (   par.mfx.LowPower == MFX_CODINGOPTION_ON
        && par.mfx.RateControlMethod != MFX_RATECONTROL_CQP
        && function != QUERYIOSURF)
    {
        mfxExtEncoderROI* roi = GetExtBufferPtr(par);
        mfxStatus expect = g_tsStatus.m_expected;

        if (roi && roi->NumROI)
        {
            switch (function)
            {
            case INIT:
                expect = MFX_ERR_INVALID_VIDEO_PARAM;
                break;
            case RESET:
                if (expect != MFX_ERR_NOT_INITIALIZED)
                    expect = MFX_ERR_INVALID_VIDEO_PARAM;
                break;
            case QUERY:
                expect = MFX_ERR_UNSUPPORTED;
                break;
            default:
                break;
            }
        }
        g_tsStatus.expect(expect);
    }

    if (function != QUERYIOSURF && g_tsOSFamily == MFX_OS_FAMILY_WINDOWS)
    {
        mfxExtCodingOption3* CO3 = GetExtBufferPtr(par);
        if (CO3 && CO3->GPB == MFX_CODINGOPTION_OFF)
        {
            //CodingOption3.GPB == OFF is not supported on Windows
            g_tsStatus.expect(MFX_WRN_INCOMPATIBLE_VIDEO_PARAM);
        }
    }
}

void SetFrameTypeIfRequired(mfxEncodeCtrl * pCtrl, mfxVideoParam * pPar, mfxFrameSurface1 * pSurf)
{
    if (!pPar || !pPar->mfx.EncodedOrder || !pCtrl || !pSurf)
    {
        // Skip if frame draining in display order
        return;
    }

    mfxU32 order = pSurf->Data.FrameOrder;
    mfxU32 goporder = order % pPar->mfx.GopPicSize;
    if (goporder == 0)
    {
        pCtrl->FrameType = MFX_FRAMETYPE_I | MFX_FRAMETYPE_REF | MFX_FRAMETYPE_xP | MFX_FRAMETYPE_xREF;
        if (pPar->mfx.IdrInterval <= 1 || order / pPar->mfx.GopPicSize % pPar->mfx.IdrInterval == 0)
            pCtrl->FrameType |= MFX_FRAMETYPE_IDR;
    }
    else if (goporder % pPar->mfx.GopRefDist == 0) {
        pCtrl->FrameType = MFX_FRAMETYPE_P | MFX_FRAMETYPE_REF | MFX_FRAMETYPE_xP | MFX_FRAMETYPE_xREF;
    }
    else {
        pCtrl->FrameType = MFX_FRAMETYPE_B | MFX_FRAMETYPE_xB;
    }
}

tsVideoEncoder::tsVideoEncoder(mfxU32 CodecId, bool useDefaults, MsdkPluginType type)
    : m_default(useDefaults)
    , m_initialized(false)
    , m_loaded(false)
    , m_par()
    , m_bitstream()
    , m_request()
    , m_pPar(&m_par)
    , m_pParOut(&m_par)
    , m_pBitstream(&m_bitstream)
    , m_pRequest(&m_request)
    , m_pSyncPoint(&m_syncpoint)
    , m_pSurf(0)
    , m_pCtrl(&m_ctrl)
    , m_filler(0)
    , m_bs_processor(0)
    , m_frames_buffered(0)
    , m_uid(0)
    , m_single_field_processing(false)
    , m_field_processed(0)
    , m_bUseDefaultFrameType(false)
{
    m_par.mfx.CodecId = CodecId;
    if (g_tsConfig.lowpower != MFX_CODINGOPTION_UNKNOWN)
    {
        m_par.mfx.LowPower = g_tsConfig.lowpower;
    }

    if (m_par.mfx.LowPower == MFX_CODINGOPTION_ON)
    {
        m_par.mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
    }

    if(m_default)
    {
        //TODO: add codec specific
        m_par.IOPattern = MFX_IOPATTERN_IN_SYSTEM_MEMORY;
        m_par.mfx.RateControlMethod = MFX_RATECONTROL_CQP;
        m_par.mfx.QPI = m_par.mfx.QPP = m_par.mfx.QPB = 26;

        if (CodecId == MFX_CODEC_VP8 || CodecId == MFX_CODEC_VP9)
            m_par.mfx.QPB = 0;

        m_par.mfx.FrameInfo.FourCC       = MFX_FOURCC_NV12;
        m_par.mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
        if (g_tsConfig.sim) {
            m_par.mfx.FrameInfo.Width  = m_par.mfx.FrameInfo.CropW = 176;
            m_par.mfx.FrameInfo.Height = m_par.mfx.FrameInfo.CropH = 144;
        } else {
            m_par.mfx.FrameInfo.Width  = m_par.mfx.FrameInfo.CropW = 720;
            m_par.mfx.FrameInfo.Height = m_par.mfx.FrameInfo.CropH = 480;
        }
        m_par.mfx.FrameInfo.FrameRateExtN = 30;
        m_par.mfx.FrameInfo.FrameRateExtD = 1;

        if (   (CodecId == MFX_CODEC_AVC || CodecId == MFX_CODEC_MPEG2)
            && m_par.mfx.FrameInfo.PicStruct != MFX_PICSTRUCT_PROGRESSIVE)
            m_par.mfx.FrameInfo.Height = (m_par.mfx.FrameInfo.Height + 31) & ~31;
    }
    m_uid = g_tsPlugin.UID(MFX_PLUGINTYPE_VIDEO_ENCODE, CodecId, type);
    m_loaded = !m_uid;
}

tsVideoEncoder::tsVideoEncoder(mfxFeiFunction func, mfxU32 CodecId, bool useDefaults)
    : m_default(useDefaults)
    , m_initialized(false)
    , m_loaded(false)
    , m_par()
    , m_bitstream()
    , m_request()
    , m_pPar(&m_par)
    , m_pParOut(&m_par)
    , m_pBitstream(&m_bitstream)
    , m_pRequest(&m_request)
    , m_pSyncPoint(&m_syncpoint)
    , m_pSurf(0)
    , m_pCtrl(&m_ctrl)
    , m_filler(0)
    , m_bs_processor(0)
    , m_frames_buffered(0)
    , m_uid(0)
    , m_single_field_processing(false)
    , m_field_processed(0)
    , m_bUseDefaultFrameType(false)
{
    m_par.mfx.CodecId = CodecId;
    if (g_tsConfig.lowpower != MFX_CODINGOPTION_UNKNOWN)
    {
        m_par.mfx.LowPower = g_tsConfig.lowpower;
    }

    if(m_default)
    {
        m_par.IOPattern = MFX_IOPATTERN_IN_VIDEO_MEMORY;
        m_par.mfx.RateControlMethod = MFX_RATECONTROL_CQP;
        m_par.mfx.QPI = m_par.mfx.QPP = m_par.mfx.QPB = 26;
        m_par.mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;

        m_par.mfx.FrameInfo.FourCC       = MFX_FOURCC_NV12;
        m_par.mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
        m_par.mfx.FrameInfo.Width  = m_par.mfx.FrameInfo.CropW = 720;
        m_par.mfx.FrameInfo.Height = m_par.mfx.FrameInfo.CropH = 480;
        m_par.mfx.FrameInfo.FrameRateExtN = 30;
        m_par.mfx.FrameInfo.FrameRateExtD = 1;
    }


    mfxExtFeiParam& extbuffer = m_par;
    extbuffer.Func = func;

    m_loaded = true;
}

tsVideoEncoder::~tsVideoEncoder()
{
    if(m_initialized)
    {
        Close();
    }
}

mfxStatus tsVideoEncoder::Init()
{
    if(m_default)
    {
        if(!m_session)
        {
            MFXInit();TS_CHECK_MFX;
        }
        if(!m_loaded)
        {
            Load();
        }
        if(     !m_pFrameAllocator
            && (   (m_request.Type & (MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET|MFX_MEMTYPE_VIDEO_MEMORY_PROCESSOR_TARGET))
                || (m_par.IOPattern & MFX_IOPATTERN_IN_VIDEO_MEMORY)))
        {
            if(!GetAllocator())
            {
                if (m_pVAHandle)
                    SetAllocator(m_pVAHandle, true);
                else
                    UseDefaultAllocator(false);
            }
            m_pFrameAllocator = GetAllocator();
            SetFrameAllocator();TS_CHECK_MFX;
        }
        if(m_par.IOPattern & MFX_IOPATTERN_IN_OPAQUE_MEMORY)
        {
            QueryIOSurf();
            AllocOpaque(m_request, m_par);
        }
    }

    // Set single field processing flag
    mfxExtFeiParam* fei_ext = (mfxExtFeiParam*)m_par.GetExtBuffer(MFX_EXTBUFF_FEI_PARAM);
    if (fei_ext)
        m_single_field_processing = (fei_ext->SingleFieldProcessing == MFX_CODINGOPTION_ON);

    return Init(m_session, m_pPar);
}

mfxStatus tsVideoEncoder::Init(mfxSession session, mfxVideoParam *par)
{
    mfxVideoParam orig_par;

    if (par) memcpy(&orig_par, m_pPar, sizeof(mfxVideoParam));

    TRACE_FUNC2(MFXVideoENCODE_Init, session, par);
    if (par)
    {
        SkipDecision(*par, INIT);
    }
    g_tsStatus.check( MFXVideoENCODE_Init(session, par) );

    m_initialized = (g_tsStatus.get() >= 0);

    if (par)
    {
        if (g_tsConfig.lowpower != MFX_CODINGOPTION_UNKNOWN)
        {
            EXPECT_EQ(g_tsConfig.lowpower, par->mfx.LowPower)
                << "ERROR: external configuration of LowPower doesn't equal to real value\n";
        }
        EXPECT_EQ(0, memcmp(&orig_par, m_pPar, sizeof(mfxVideoParam)))
            << "ERROR: Input parameters must not be changed in Init()";
    }

    return g_tsStatus.get();
}

typedef enum tagENCODE_FUNC
{
    ENCODE_ENC = 0x0001,
    ENCODE_PAK = 0x0002,
    ENCODE_ENC_PAK = 0x0004,
    ENCODE_HybridPAK = 0x0008,
    ENCODE_WIDI = 0x8000
} ENCODE_FUNC;

// Decode Extension Functions for DXVA11 Encode
#define ENCODE_QUERY_ACCEL_CAPS_ID 0x110

mfxStatus tsVideoEncoder::GetCaps(void *pCaps, mfxU32 *pCapsSize)
{
    TRACE_FUNC2(GetCaps, pCaps, pCapsSize);
    mfxHandleType hdl_type;
    mfxHDL hdl;
    mfxU32 count = 0;
    mfxStatus sts = MFX_ERR_UNSUPPORTED;

#if defined(_WIN32) || defined(_WIN64)

    static const GUID DXVA2_Intel_Encode_HEVC_Main = { 0x28566328, 0xf041, 0x4466,{ 0x8b, 0x14, 0x8f, 0x58, 0x31, 0xe7, 0x8f, 0x8b } };
    static const GUID DXVA2_Intel_LowpowerEncode_HEVC_Main = { 0xb8b28e0c, 0xecab, 0x4217,{ 0x8c, 0x82, 0xea, 0xaa, 0x97, 0x55, 0xaa, 0xf0 } };
    static const GUID DXVA_NoEncrypt = { 0x1b81beD0, 0xa0c7, 0x11d3,{ 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5 } };

    HRESULT hr;
    GUID guid;
    if (g_tsConfig.lowpower & MFX_CODINGOPTION_ON)
        guid = DXVA2_Intel_LowpowerEncode_HEVC_Main;
    else
        guid = DXVA2_Intel_Encode_HEVC_Main;

    if (!m_is_handle_set && g_tsImpl != MFX_IMPL_SOFTWARE)
    {
        if (m_initialized)
        {
            g_tsLog << "\nERROR: Handle can't be set if encoder is already initialized!!!\n\n";
            return MFX_ERR_INVALID_HANDLE;
        }

        if (!m_pVAHandle)
        {
            m_pVAHandle = new frame_allocator(
                    (g_tsImpl & MFX_IMPL_VIA_D3D11) ? frame_allocator::HARDWARE_DX11 : frame_allocator::HARDWARE,
                    frame_allocator::ALLOC_MAX,
                    frame_allocator::ENABLE_ALL,
                    frame_allocator::ALLOC_EMPTY);
        }
        m_pVAHandle->get_hdl(hdl_type, hdl);
        SetHandle(m_session, hdl_type, hdl);
        m_is_handle_set = (g_tsStatus.get() >= 0);
    }
    if (g_tsImpl & MFX_IMPL_VIA_D3D11) {
        ID3D11Device* device;
        D3D11_VIDEO_DECODER_DESC    desc = {};
        D3D11_VIDEO_DECODER_CONFIG  config = {};
        hdl_type = mfxHandleType::MFX_HANDLE_D3D11_DEVICE;
        sts = MFXVideoCORE_GetHandle(m_session, hdl_type, (mfxHDL*)&device);
        MFX_CHECK((sts == MFX_ERR_NONE), MFX_ERR_DEVICE_FAILED);

        CComPtr<ID3D11DeviceContext>                m_context;
        CComPtr<ID3D11VideoDecoder>                 m_vdecoder;
        CComQIPtr<ID3D11VideoDevice>                m_vdevice;
        CComQIPtr<ID3D11VideoContext>               m_vcontext;

        device->GetImmediateContext(&m_context);
        MFX_CHECK(m_context, MFX_ERR_DEVICE_FAILED);

        m_vdevice = device;
        m_vcontext = m_context;
        MFX_CHECK(m_vdevice, MFX_ERR_DEVICE_FAILED);
        MFX_CHECK(m_vcontext, MFX_ERR_DEVICE_FAILED);

        // Query supported decode profiles
        {
            bool isFound = false;

            UINT profileCount = m_vdevice->GetVideoDecoderProfileCount();
            assert(profileCount > 0);

            for (UINT i = 0; i < profileCount; i++)
            {
                GUID profileGuid;
                hr = m_vdevice->GetVideoDecoderProfile(i, &profileGuid);
                MFX_CHECK(SUCCEEDED(hr), MFX_ERR_DEVICE_FAILED);

                if (guid == profileGuid)
                {
                    isFound = true;
                    break;
                }
            }
            MFX_CHECK(isFound, MFX_ERR_UNSUPPORTED);
        }
        mfxU16 width = m_pPar->mfx.FrameInfo.Width ? m_pPar->mfx.FrameInfo.Width : 720;
        mfxU16 height = m_pPar->mfx.FrameInfo.Height ? m_pPar->mfx.FrameInfo.Height : 480;
        // Query the supported encode functions
        {
            desc.SampleWidth = width;
            desc.SampleHeight = height;
            desc.OutputFormat = DXGI_FORMAT_NV12;
            desc.Guid = guid;

            hr = m_vdevice->GetVideoDecoderConfigCount(&desc, &count);
            MFX_CHECK(SUCCEEDED(hr), MFX_ERR_DEVICE_FAILED);
        }

        // CreateVideoDecoder
        {
            desc.SampleWidth = width;
            desc.SampleHeight = height;
            desc.OutputFormat = DXGI_FORMAT_NV12;
            desc.Guid = guid;

            config.ConfigDecoderSpecific = ENCODE_ENC_PAK;
            config.guidConfigBitstreamEncryption = DXVA_NoEncrypt;
            if (!!m_vdecoder)
                m_vdecoder.Release();

            hr = m_vdevice->CreateVideoDecoder(&desc, &config, &m_vdecoder);
            MFX_CHECK(SUCCEEDED(hr), MFX_ERR_DEVICE_FAILED);
        }

        // Query the encoding device capabilities
        {
            D3D11_VIDEO_DECODER_EXTENSION ext = {};
            ext.Function = ENCODE_QUERY_ACCEL_CAPS_ID;
            ext.pPrivateInputData = 0;
            ext.PrivateInputDataSize = 0;
            ext.pPrivateOutputData = pCaps;
            ext.PrivateOutputDataSize = *pCapsSize;
            ext.ResourceCount = 0;
            ext.ppResourceList = 0;

            HRESULT hr;
            hr = m_vcontext->DecoderExtension(m_vdecoder, &ext);
            MFX_CHECK(SUCCEEDED(hr), MFX_ERR_DEVICE_FAILED);
        }

    } else if (g_tsImpl != MFX_IMPL_SOFTWARE) {
        IDirect3DDeviceManager9 *device = 0;
        hdl_type = mfxHandleType::MFX_HANDLE_D3D9_DEVICE_MANAGER;
        sts = MFXVideoCORE_GetHandle(m_session, hdl_type, (mfxHDL*)&device);
        MFX_CHECK((sts == MFX_ERR_NONE), MFX_ERR_DEVICE_FAILED);

        std::auto_ptr<AuxiliaryDevice> auxDevice(new AuxiliaryDevice());
        sts = auxDevice->Initialize(device);
        MFX_CHECK((sts == MFX_ERR_NONE), MFX_ERR_DEVICE_FAILED);

        sts = auxDevice->IsAccelerationServiceExist(guid);
        MFX_CHECK((sts == MFX_ERR_NONE), MFX_ERR_DEVICE_FAILED);
        sts = auxDevice->QueryAccelCaps(&guid, pCaps, pCapsSize);
        MFX_CHECK((sts == MFX_ERR_NONE), MFX_ERR_DEVICE_FAILED);
    }
#else
    ENCODE_CAPS_HEVC* hevc_caps = (ENCODE_CAPS_HEVC*)pCaps;
    hevc_caps->BlockSize = 2;   // 32x32
    hevc_caps->SliceStructure = 2;  // arbitrary aligned
    if (g_tsHWtype >= MFX_HW_CNL)
        hevc_caps->SliceStructure = 4;  // raw aligned
    sts = MFX_ERR_NONE;
#endif

    return sts;
}

mfxStatus tsVideoEncoder::Close()
{
    Close(m_session);

    //free the surfaces in pool
    tsSurfacePool::FreeSurfaces();

    return g_tsStatus.get();
}

mfxStatus tsVideoEncoder::Close(mfxSession session)
{
    TRACE_FUNC1(MFXVideoENCODE_Close, session);
    g_tsStatus.check( MFXVideoENCODE_Close(session) );

    m_initialized = false;
    m_frames_buffered = 0;

    return g_tsStatus.get();
}

mfxStatus tsVideoEncoder::Query()
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
    return Query(m_session, m_pPar, m_pParOut);
}

mfxStatus tsVideoEncoder::Query(mfxSession session, mfxVideoParam *in, mfxVideoParam *out)
{
    TRACE_FUNC3(MFXVideoENCODE_Query, session, in, out);
    if (in)
    {
        SkipDecision(*in, QUERY);
    }
    g_tsStatus.check( MFXVideoENCODE_Query(session, in, out) );
    TS_TRACE(out);

    return g_tsStatus.get();
}

mfxStatus tsVideoEncoder::QueryIOSurf()
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
    return QueryIOSurf(m_session, m_pPar, m_pRequest);
}

mfxStatus tsVideoEncoder::QueryIOSurf(mfxSession session, mfxVideoParam *par, mfxFrameAllocRequest *request)
{
    TRACE_FUNC3(MFXVideoENCODE_QueryIOSurf, session, par, request);
    if (par)
    {
        SkipDecision(*par, QUERYIOSURF);
    }
    g_tsStatus.check( MFXVideoENCODE_QueryIOSurf(session, par, request) );
    TS_TRACE(request);

    return g_tsStatus.get();
}

mfxStatus tsVideoEncoder::Reset()
{
    return Reset(m_session, m_pPar);
}

mfxStatus tsVideoEncoder::Reset(mfxSession session, mfxVideoParam *par)
{
    TRACE_FUNC2(MFXVideoENCODE_Reset, session, par);
    if (par)
    {
        SkipDecision(*par, RESET);
    }
    g_tsStatus.check( MFXVideoENCODE_Reset(session, par) );

    //m_frames_buffered = 0;

    return g_tsStatus.get();
}

mfxStatus tsVideoEncoder::GetVideoParam()
{
    if(m_default && !m_initialized)
    {
        Init();TS_CHECK_MFX;
    }
    return GetVideoParam(m_session, m_pPar);
}

mfxStatus tsVideoEncoder::GetVideoParam(mfxSession session, mfxVideoParam *par)
{
    TRACE_FUNC2(MFXVideoENCODE_GetVideoParam, session, par);
    g_tsStatus.check( MFXVideoENCODE_GetVideoParam(session, par) );
    TS_TRACE(par);

    return g_tsStatus.get();
}

mfxStatus tsVideoEncoder::EncodeFrameAsync()
{
    mfxEncodeCtrl* prevCtrl;
    bool restoreCtrl = false;

    if(m_default)
    {
        if(!PoolSize())
        {
            if(m_pFrameAllocator && !GetAllocator())
            {
                SetAllocator(m_pFrameAllocator, true);
            }
            AllocSurfaces();TS_CHECK_MFX;
            if(!m_pFrameAllocator && (m_request.Type & (MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET|MFX_MEMTYPE_VIDEO_MEMORY_PROCESSOR_TARGET)))
            {
                m_pFrameAllocator = GetAllocator();
                SetFrameAllocator();TS_CHECK_MFX;
            }
        }
        if(!m_initialized)
        {
            Init();TS_CHECK_MFX;
        }
        if(!m_bitstream.MaxLength)
        {
            AllocBitstream();TS_CHECK_MFX;
        }
        if (m_field_processed == 0) {
            //if SingleFieldProcessing is enabled, then don't get new surface if the second field to be processed.
            //if SingleFieldProcessing is not enabled, m_field_processed == 0 always.
            m_pSurf = GetSurface(); TS_CHECK_MFX;
        }

        if(m_filler)
        {
            m_pSurf = m_filler->ProcessSurface(m_pSurf, m_pFrameAllocator);
        }

        if (m_ctrl_next.Get())
        {
            prevCtrl = m_pCtrl;
            restoreCtrl = true;
            m_pCtrl = m_ctrl_next.Get();
        }
    }

    mfxEncodeCtrl * cur_ctrl = m_pSurf ? m_pCtrl : NULL;
    if (m_bUseDefaultFrameType)
    {
        SetFrameTypeIfRequired(cur_ctrl, m_pPar, m_pSurf);
    }

    mfxStatus mfxRes = EncodeFrameAsync(m_session, cur_ctrl, m_pSurf, m_pBitstream, m_pSyncPoint);

    if (m_single_field_processing)
    {
        //m_field_processed would be use as indicator in ProcessBitstream(),
        //so increase it in advance here.
        m_field_processed = 1 - m_field_processed;
    }

    if (m_default)
    {
        if (restoreCtrl)
            m_pCtrl = prevCtrl;

        if (m_ctrl_next.Get())
        {
            if (   mfxRes == MFX_ERR_MORE_DATA
                || mfxRes == MFX_ERR_NONE)
            {
                m_ctrl_next.m_fo = m_pSurf ? m_pSurf->Data.FrameOrder : 0;
                m_ctrl_reorder_buffer.push_back(m_ctrl_next);
                m_ctrl_next.Reset();
            }

            if (mfxRes == MFX_ERR_NONE)
            {
                for (auto& p : m_ctrl_reorder_buffer)
                {
                    p.m_sp = *m_pSyncPoint;
                    p.m_lockCnt = m_request.NumFrameMin;
                }

                m_ctrl_list.splice(m_ctrl_list.end(), m_ctrl_reorder_buffer);
            }
        }
    }

    return mfxRes;
}

mfxStatus tsVideoEncoder::DrainEncodedBitstream()
{
    mfxStatus mfxRes = MFX_ERR_NONE;

    while (mfxRes == MFX_ERR_NONE)
    {
        mfxRes = EncodeFrameAsync(m_session, 0, NULL, m_pBitstream, m_pSyncPoint);
        if (mfxRes == MFX_ERR_NONE)
        {
            mfxStatus mfxResSync = SyncOperation(*m_pSyncPoint);
            if (mfxResSync != MFX_ERR_NONE)
            {
                return mfxResSync;
            }
        }
    }

    if (mfxRes == MFX_ERR_MORE_DATA)
    {
        // MORE_DATA code means everything was drained from the encoder
        return MFX_ERR_NONE;
    }
    else
    {
        return mfxRes;
    }
}

mfxStatus tsVideoEncoder::EncodeFrameAsync(mfxSession session, mfxEncodeCtrl *ctrl, mfxFrameSurface1 *surface, mfxBitstream *bs, mfxSyncPoint *syncp)
{
    TRACE_FUNC5(MFXVideoENCODE_EncodeFrameAsync, session, ctrl, surface, bs, syncp);
    mfxStatus mfxRes = MFXVideoENCODE_EncodeFrameAsync(session, ctrl, surface, bs, syncp);
    TS_TRACE(mfxRes);
    TS_TRACE(ctrl);
    TS_TRACE(surface);
    TS_TRACE(bs);
    TS_TRACE(syncp);

    m_frames_buffered += (mfxRes >= 0);

    return g_tsStatus.m_status = mfxRes;
}

mfxStatus tsVideoEncoder::AllocBitstream(mfxU32 size)
{
    if(!size)
    {
        if(m_par.mfx.CodecId == MFX_CODEC_JPEG)
        {
            size = TS_MAX((m_par.mfx.FrameInfo.Width*m_par.mfx.FrameInfo.Height), 1000000);
        }
        else
        {
            if(!m_par.mfx.BufferSizeInKB)
            {
                GetVideoParam();TS_CHECK_MFX;
            }
            size = m_par.mfx.BufferSizeInKB * TS_MAX(m_par.mfx.BRCParamMultiplier, 1) * 1000 * TS_MAX(m_par.AsyncDepth, 1);
        }
    }

    g_tsLog << "ALLOC BITSTREAM OF SIZE " << size << "\n";

    mfxMemId mid = 0;
    TRACE_FUNC4((*m_buffer_allocator.Alloc), &m_buffer_allocator, size, (MFX_MEMTYPE_SYSTEM_MEMORY|MFX_MEMTYPE_FROM_ENCODE), &mid);
    g_tsStatus.check((*m_buffer_allocator.Alloc)(&m_buffer_allocator, size, (MFX_MEMTYPE_SYSTEM_MEMORY|MFX_MEMTYPE_FROM_ENCODE), &mid));
    TRACE_FUNC3((*m_buffer_allocator.Lock), &m_buffer_allocator, mid, &m_bitstream.Data);
    g_tsStatus.check((*m_buffer_allocator.Lock)(&m_buffer_allocator, mid, &m_bitstream.Data));
    m_bitstream.MaxLength = size;

    return g_tsStatus.get();
}

mfxStatus tsVideoEncoder::AllocSurfaces()
{
    if(m_default && !m_request.NumFrameMin)
    {
        QueryIOSurf();TS_CHECK_MFX;
    }
    return tsSurfacePool::AllocSurfaces(m_request);
}

mfxStatus tsVideoEncoder::SyncOperation()
{
    return SyncOperation(m_syncpoint);
}

mfxStatus tsVideoEncoder::SyncOperation(mfxSyncPoint syncp)
{
    mfxU32 nFrames = m_frames_buffered;
    mfxStatus res = SyncOperation(m_session, syncp, MFX_INFINITE);

    if (m_default && m_bs_processor && g_tsStatus.get() == MFX_ERR_NONE)
    {
        g_tsStatus.check(m_bs_processor->ProcessBitstream(m_pBitstream ? *m_pBitstream : m_bitstream, nFrames));
        TS_CHECK_MFX;
    }

    return g_tsStatus.m_status = res;
}

mfxStatus tsVideoEncoder::SyncOperation(mfxSession session,  mfxSyncPoint syncp, mfxU32 wait)
{
    m_frames_buffered = 0;
    tsSession::SyncOperation(session, syncp, wait);

    if (!g_tsStatus.get())
    {
        for (auto& p : m_ctrl_list)
        {
            if (p.m_sp == syncp)
                p.m_unlock = true;

            if (p.m_unlock && p.m_lockCnt)
                p.m_lockCnt--;
        }

        m_ctrl_list.remove_if([](tsSharedCtrl& p) { return (p.m_unlock && !p.m_lockCnt); });
    }

    return g_tsStatus.get();
}

mfxStatus tsVideoEncoder::EncodeFrames(mfxU32 n, bool check)
{
    mfxU32 encoded = 0;
    mfxU32 submitted = 0;
    mfxU32 async = TS_MAX(1, m_par.AsyncDepth);
    mfxSyncPoint sp;

    mfxExtFeiParam* fei_ext= (mfxExtFeiParam*)m_par.GetExtBuffer(MFX_EXTBUFF_FEI_PARAM);
    if (fei_ext)
        m_single_field_processing = (fei_ext->SingleFieldProcessing == MFX_CODINGOPTION_ON);

    async = TS_MIN(n, async - 1);

    while(encoded < n)
    {
        mfxStatus sts = EncodeFrameAsync();

        if (sts == MFX_ERR_MORE_DATA)
        {
            if(!m_pSurf)
            {
                if(submitted)
                {
                    encoded += submitted;
                    SyncOperation(sp);
                }
                break;
            }

            continue;
        }

        g_tsStatus.check(); TS_CHECK_MFX;
        sp = m_syncpoint;

        //For FEI, AsyncDepth = 1, so every time one frame is submitted for encoded, it will
        //be synced immediately afterwards.
        if(++submitted >= async)
        {
            SyncOperation();TS_CHECK_MFX;

            //If SingleFieldProcessing is enabled, and the first field is processed, continue
            if (m_single_field_processing)
            {
                if (m_field_processed)
                {
                    //for the second field, no new surface submitted.
                    submitted--;
                    continue;
                }
            }

            encoded += submitted;
            submitted = 0;
            async = TS_MIN(async, (n - encoded));
        }
    }

    g_tsLog << encoded << " FRAMES ENCODED\n";

    if (check && (encoded != n))
        return MFX_ERR_UNKNOWN;

    return g_tsStatus.get();
}

mfxStatus tsVideoEncoder::InitAndSetAllocator()
{
    mfxHDL hdl;
    mfxHandleType type;

    if (!GetAllocator())
    {
        UseDefaultAllocator(
            (m_par.IOPattern & MFX_IOPATTERN_IN_SYSTEM_MEMORY)
            || (m_request.Type & MFX_MEMTYPE_SYSTEM_MEMORY)
            );
    }

    //set handle
    if (!((m_par.IOPattern & MFX_IOPATTERN_IN_SYSTEM_MEMORY)
        || (m_request.Type & MFX_MEMTYPE_SYSTEM_MEMORY))
        && (!m_pVAHandle))
    {
        m_pFrameAllocator = GetAllocator();
        SetFrameAllocator();
        m_pVAHandle = m_pFrameAllocator;
        m_pVAHandle->get_hdl(type, hdl);
        SetHandle(m_session, type, hdl);
        m_is_handle_set = (g_tsStatus.get() >= 0);
    }

    return g_tsStatus.get();
}
