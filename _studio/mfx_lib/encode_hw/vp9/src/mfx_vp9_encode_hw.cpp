// Copyright (c) 2018-2020 Intel Corporation
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

#include "mfxvideo++int.h"
#include "mfx_vp9_encode_hw.h"
#include "mfx_vp9_encode_hw_par.h"
#include "mfx_vp9_encode_hw_ddi.h"
#include "fast_copy.h"

namespace MfxHwVP9Encode
{
bool CheckTriStateOption(mfxU16 & opt);

void SetDefaultForLowpower(mfxU16 & lowpower, eMFXHWType platform)
{
    CheckTriStateOption(lowpower);

    if (lowpower == MFX_CODINGOPTION_UNKNOWN)
    {
#if (MFX_VERSION >= 1027)
        if (platform >= MFX_HW_ICL)
            lowpower = MFX_CODINGOPTION_ON;
        else
#else
        std::ignore = platform;
#endif
            lowpower = MFX_CODINGOPTION_OFF;
    }
}

mfxStatus MFXVideoENCODEVP9_HW::Query(VideoCORE *core, mfxVideoParam *in, mfxVideoParam *out)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_DEFAULT, "MFXVideoENCODEVP9_HW::Query");
    MFX_CHECK_NULL_PTR1(out);

    if (in == 0)
    {
        return SetSupportedParameters(*out);
    }
    else
    {
        eMFXHWType platform = core->GetHWType();

        // check attached buffers (all are supported by encoder, no duplications, etc.)
        mfxStatus sts = CheckExtBufferHeaders(in->NumExtParam, in->ExtParam);
        MFX_CHECK_STS(sts);
        sts = CheckExtBufferHeaders(out->NumExtParam, out->ExtParam);
        MFX_CHECK_STS(sts);

        VP9MfxVideoParam toValidate(*in, platform);
        SetDefaultsForProfileAndFrameInfo(toValidate);

        // get HW caps from driver
        ENCODE_CAPS_VP9 caps = {};
        sts = QueryCaps(core, caps, GetGuid(toValidate), toValidate);
        MFX_CHECK(MFX_ERR_NONE == sts, MFX_ERR_UNSUPPORTED);

        toValidate = *in;

        // validate input parameters
        sts = CheckParameters(toValidate, caps);
        toValidate.SyncInternalParamToExternal();

        // copy validated parameters to [out]
        out->mfx = toValidate.mfx;
        out->AsyncDepth = toValidate.AsyncDepth;
        out->IOPattern = toValidate.IOPattern;
        out->Protected = toValidate.Protected;

        // copy validated ext buffers
        if (in->ExtParam && out->ExtParam)
        {
            for (mfxU8 i = 0; i < in->NumExtParam; i++)
            {
                mfxExtBuffer *pInBuf = in->ExtParam[i];
                if (pInBuf)
                {
                    mfxExtBuffer *pOutBuf = GetExtendedBuffer(out->ExtParam, out->NumExtParam, pInBuf->BufferId);
                    if (pOutBuf && (pOutBuf->BufferSz == pInBuf->BufferSz))
                    {
                        mfxExtBuffer *pCorrectedBuf = GetExtendedBuffer(toValidate.ExtParam, toValidate.NumExtParam, pInBuf->BufferId);
                        MFX_CHECK_NULL_PTR1(pCorrectedBuf);
                        char* src = reinterpret_cast <char*> (pCorrectedBuf);
                        char* dst   = reinterpret_cast <char*> (pOutBuf);
                        std::copy(src, src + pCorrectedBuf->BufferSz, dst);
                    }
                    else
                    {
                        // the buffer is present in [in] and absent in [out]
                        // or buffer sizes are different in [in] and [out]
                        return MFX_ERR_UNDEFINED_BEHAVIOR;
                    }
                }
            }
        }

        return sts;
    }
}

mfxStatus MFXVideoENCODEVP9_HW::QueryIOSurf(VideoCORE *core, mfxVideoParam *par, mfxFrameAllocRequest *request)
{
    MFX_CHECK_NULL_PTR2(par,request);
    mfxStatus sts = CheckExtBufferHeaders(par->NumExtParam, par->ExtParam);
    MFX_CHECK_STS(sts);

    eMFXHWType platform = core->GetHWType();

    mfxU32 inPattern = par->IOPattern & MFX_IOPATTERN_IN_MASK;
    MFX_CHECK(inPattern == MFX_IOPATTERN_IN_VIDEO_MEMORY ||
        inPattern == MFX_IOPATTERN_IN_SYSTEM_MEMORY ||
        inPattern == MFX_IOPATTERN_IN_OPAQUE_MEMORY, MFX_ERR_INVALID_VIDEO_PARAM);

    VP9MfxVideoParam toValidate(*par, platform);

    SetDefaultsForProfileAndFrameInfo(toValidate);
    SetDefaultForLowpower(toValidate.mfx.LowPower, platform);

    // get HW caps from driver
    ENCODE_CAPS_VP9 caps = {};
    sts = QueryCaps(core, caps, GetGuid(toValidate), toValidate);
    MFX_CHECK(MFX_ERR_NONE == sts, MFX_ERR_UNSUPPORTED);

    // get validated and properly initialized set of parameters
    CheckParameters(toValidate, caps);
    SetDefaults(toValidate, caps);

    // fill mfxFrameAllocRequest
    switch (par->IOPattern)
    {
    case MFX_IOPATTERN_IN_SYSTEM_MEMORY:
        request->Type = MFX_MEMTYPE_SYS_EXT;
        break;
    case MFX_IOPATTERN_IN_VIDEO_MEMORY:
        request->Type = MFX_MEMTYPE_D3D_EXT;
        break;
    case MFX_IOPATTERN_IN_OPAQUE_MEMORY:
        request->Type = MFX_MEMTYPE_FROM_ENCODE | MFX_MEMTYPE_DXVA2_DECODER_TARGET | MFX_MEMTYPE_OPAQUE_FRAME;
        break;
    default:
        return MFX_ERR_INVALID_VIDEO_PARAM;
    }

    request->NumFrameMin = (mfxU16)CalcNumSurfRaw(toValidate);
    request->NumFrameSuggested = request->NumFrameMin;

    request->Info = par->mfx.FrameInfo;

    return MFX_ERR_NONE;
}


#if (MFX_VERSION >= 1027)
void SetReconInfo(VP9MfxVideoParam const &par, mfxFrameInfo &fi, eMFXHWType const &platform)
{
    mfxExtCodingOption3 opt3 = GetExtBufferRef(par);
    mfxU16 format = opt3.TargetChromaFormatPlus1 - 1;
    mfxU16 depth = opt3.TargetBitDepthLuma;

    fi.Width  = mfx::align2_value(fi.Width,  64);
    fi.Height = mfx::align2_value(fi.Height, 64);

    if (format == MFX_CHROMAFORMAT_YUV444 && depth == BITDEPTH_10)
    {
        fi.FourCC = MFX_FOURCC_Y410;

#ifndef LINUX
        /* Pitch = 4*W for Y410 format
           Pitch need to align on 256
           So, width aligment is 256/4 = 64 */
        fi.Width = (mfxU16)mfx::align2_value(fi.Width, 256 / 4);
        fi.Height = (mfxU16)mfx::align2_value(fi.Height * 3 / 2, 64);
#endif
    }
    else if (format == MFX_CHROMAFORMAT_YUV444 && depth == BITDEPTH_8)
    {
        fi.FourCC = MFX_FOURCC_AYUV;

#ifndef LINUX
        /* Pitch = 4*W for AYUV format
           Pitch need to align on 512
           So, width aligment is 512/4 = 128 */
        fi.Width = (mfxU16)mfx::align2_value(fi.Width, 512 / 4);
        fi.Height = (mfxU16)mfx::align2_value(fi.Height * 3 / 4, 64);
#endif
    }
    else if (format == MFX_CHROMAFORMAT_YUV420 && depth == BITDEPTH_10)
    {
#if (MFX_VERSION >= 1031)
        if (platform >= MFX_HW_TGL_LP)
        {
#ifdef LINUX
            fi.FourCC = MFX_FOURCC_P010;
#else
            fi.FourCC = MFX_FOURCC_NV12;
            fi.Width  = mfx::align2_value(fi.Width, 32) * 2;
#endif // LINUX
        }
        else
#endif
        {
            std::ignore = platform;
            fi.FourCC  = MFX_FOURCC_P010;
        }
    }
    else if (format == MFX_CHROMAFORMAT_YUV420 && depth == BITDEPTH_8)
    {
        fi.FourCC = MFX_FOURCC_NV12;
    }
    else
    {
        assert(!"undefined target format");
    }

    fi.ChromaFormat = format;
    fi.BitDepthLuma = depth;
    fi.BitDepthChroma = depth;
}
#endif

mfxStatus MFXVideoENCODEVP9_HW::Init(mfxVideoParam *par)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "MFXVideoENCODEVP9_HW::Init");

    if (m_initialized == true)
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    MFX_CHECK_NULL_PTR1(par);
    mfxStatus sts = CheckExtBufferHeaders(par->NumExtParam, par->ExtParam);
    MFX_CHECK_STS(sts);

    mfxStatus checkSts = MFX_ERR_NONE; // to save warnings ater parameters checking

    eMFXHWType platform = m_pCore->GetHWType();

    m_video = VP9MfxVideoParam(*par, platform);

    m_ddi.reset(CreatePlatformVp9Encoder(m_pCore));
    MFX_CHECK(m_ddi.get() != 0, MFX_ERR_UNSUPPORTED);

    VP9MfxVideoParam toGetGuid(m_video);

    SetDefaultsForProfileAndFrameInfo(toGetGuid);

    sts = m_ddi->CreateAuxilliaryDevice(m_pCore, GetGuid(toGetGuid), toGetGuid);
    MFX_CHECK(sts != MFX_ERR_UNSUPPORTED, MFX_ERR_INVALID_VIDEO_PARAM);
    MFX_CHECK(sts == MFX_ERR_NONE, MFX_ERR_DEVICE_FAILED);

    ENCODE_CAPS_VP9 caps = {};
    m_ddi->QueryEncodeCaps(caps);

    // get validated and properly initialized set of parameters for encoding
    checkSts = CheckParametersAndSetDefaults(m_video, caps);
    MFX_CHECK(checkSts >= 0, checkSts);

    // save WxH from Init to check resolution change in Reset
    m_initWidth = m_video.mfx.FrameInfo.Width;
    m_initHeight = m_video.mfx.FrameInfo.Height;

    sts = m_ddi->CreateAccelerationService(m_video);
    MFX_CHECK_STS(sts);

    m_rawFrames.Init(CalcNumSurfRaw(m_video));

    mfxFrameAllocRequest request = {};
    request.Info = m_video.mfx.FrameInfo;
    request.Type = MFX_MEMTYPE_D3D_INT;

    // allocate internal surfaces for input raw frames in case of SYSTEM input memory
    if (m_video.IOPattern == MFX_IOPATTERN_IN_SYSTEM_MEMORY)
    {
        request.NumFrameMin = request.NumFrameSuggested = (mfxU16)CalcNumSurfRaw(m_video);
        sts = m_rawLocalFrames.Init(m_pCore, &request, true);
        MFX_CHECK_STS(sts);
    }
    else if (m_video.IOPattern == MFX_IOPATTERN_IN_OPAQUE_MEMORY)
    {
        mfxExtOpaqueSurfaceAlloc& opaq = GetExtBufferRef(m_video);
        request.Type = opaq.In.Type;
        request.NumFrameMin = opaq.In.NumSurface;

        sts = m_opaqFrames.Init(m_pCore, &request, false);
        MFX_CHECK_STS(sts);

        if (opaq.In.Type & MFX_MEMTYPE_SYSTEM_MEMORY)
        {
            request.Type = MFX_MEMTYPE_D3D_INT;
            request.NumFrameMin = opaq.In.NumSurface;
            sts = m_rawLocalFrames.Init(m_pCore, &request, true);
            MFX_CHECK_STS(sts);
        }
    }

    // allocate and register surfaces for reconstructed frames
    request.NumFrameMin = request.NumFrameSuggested = (mfxU16)CalcNumSurfRecon(m_video);
#if (MFX_VERSION >= 1027)
    SetReconInfo(m_video, request.Info, platform);
#else
    (void)platform;
    request.Info.FourCC = MFX_FOURCC_NV12;
#endif

    sts = m_reconFrames.Init(m_pCore, &request, false);
    MFX_CHECK_STS(sts);
    sts = m_ddi->Register(m_reconFrames.GetFrameAllocReponse(), D3DDDIFMT_NV12);
    MFX_CHECK_STS(sts);

    // allocate and register surfaces for output bitstreams
    sts = m_ddi->QueryCompBufferInfo(D3DDDIFMT_INTELENCODE_BITSTREAMDATA, request, m_video.mfx.FrameInfo.Width, m_video.mfx.FrameInfo.Height);
    MFX_CHECK_STS(sts);
    request.NumFrameMin = request.NumFrameSuggested = (mfxU16)CalcNumTasks(m_video);

    if (!request.Info.Width)
    {
        request.Info.Width = m_video.mfx.FrameInfo.Width;
    }
    if (!request.Info.Height)
    {
        request.Info.Height = m_video.mfx.FrameInfo.Height;
    }

    // maximum buffer size for output bitstream is equal to two uncompressed frames of NV12 format
    //  with an assumption that in the worst case encoded frame size exceeds the uncompressed size (allowed by VP9 standard)
    mfxU32 max_buffer_size = m_video.mfx.FrameInfo.Width*m_video.mfx.FrameInfo.Height*3;

    if (m_video.mfx.FrameInfo.FourCC == MFX_FOURCC_P010
#if (MFX_VERSION >= 1027)
        || m_video.mfx.FrameInfo.FourCC == MFX_FOURCC_Y410
#endif
        ) {
        // doubling the size of the bitstream buffer if 10-bit format is used
        max_buffer_size *= 2;
    }

    // increase Width and Height in the request to allocate bigger buffer if estimated maximum size exceeds recommendation of the driver
    if (static_cast<mfxU32>(request.Info.Width * request.Info.Height) < max_buffer_size)
    {
        mfxU32 tmp_width = static_cast<mfxU32>(request.Info.Width);
        mfxU32 tmp_height = CeilDiv(max_buffer_size, static_cast<mfxU32>(request.Info.Width));

        // TODO: maximum supported width and height need to be checked on dependence on a platform and other possible factors
        const mfxU32 max_dx_dimension_size = 16384;

        if (tmp_height > max_dx_dimension_size)
        {
            tmp_height = max_dx_dimension_size;
            tmp_width = CeilDiv(max_buffer_size, max_dx_dimension_size);
            if (tmp_width > max_dx_dimension_size)
            {
                tmp_width = max_dx_dimension_size;
            }
        }

        request.Info.Width = static_cast<mfxU16>(tmp_width);
        request.Info.Height = static_cast<mfxU16>(tmp_height);
    }

    sts = m_outBitstreams.Init(m_pCore, &request, false);
    MFX_CHECK_STS(sts);
    sts = m_ddi->Register(m_outBitstreams.GetFrameAllocReponse(), D3DDDIFMT_INTELENCODE_BITSTREAMDATA);
    MFX_CHECK_STS(sts);
    m_maxBsSize = request.Info.Width * request.Info.Height;

    // allocate and register surfaces for segmentation map
    sts = m_ddi->QueryCompBufferInfo(D3DDDIFMT_INTELENCODE_MBSEGMENTMAP, request, m_video.mfx.FrameInfo.Width, m_video.mfx.FrameInfo.Height);
    MFX_CHECK_STS(sts);
    request.NumFrameMin = request.NumFrameSuggested = (mfxU16)CalcNumTasks(m_video);

    sts = m_segmentMaps.Init(m_pCore, &request, false);
    MFX_CHECK_STS(sts);
    sts = m_ddi->Register(m_segmentMaps.GetFrameAllocReponse(), D3DDDIFMT_INTELENCODE_MBSEGMENTMAP);
    MFX_CHECK_STS(sts);

    mfxU16 blockSize = MapIdToBlockSize(MFX_VP9_SEGMENT_ID_BLOCK_SIZE_64x64);
    // allocate enough space for segmentation map for lowest supported segment block size and highest supported resolution
    mfxU16 wInBlocks = (static_cast<mfxU16>(caps.MaxPicWidth) + blockSize - 1) / blockSize;
    mfxU16 hInBlocks = (static_cast<mfxU16>(caps.MaxPicHeight) + blockSize - 1) / blockSize;
    mfxU32 sizeInBlocks = wInBlocks * hInBlocks;
    m_prevSegment.SegmentId = new mfxU8[sizeInBlocks];
    memset(m_prevSegment.SegmentId, 0, sizeInBlocks);

    Zero(m_prevFrameParam);

    // prepare enough space for tasks
    m_free.resize(CalcNumTasks(m_video));
    m_numBufferedFrames = 0;

    // prepare enough space for DPB management
    m_dpb.resize(m_video.mfx.NumRefFrame);

    m_bStartIVFSequence = true;

    m_initialized = true;
    m_resetBrc = false;

    m_frameArrivalOrder = 0;
    m_taskIdForDriver = 0;
    m_frameOrderInGop = 0;
    m_frameOrderInRefStructure = 0;

    m_videoForParamChange.push_back(m_video);

    return checkSts;
}

mfxStatus MFXVideoENCODEVP9_HW::Reset(mfxVideoParam *par)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "MFXVideoENCODEVP9_HW::Reset");

    if (m_initialized == false)
    {
        return MFX_ERR_NOT_INITIALIZED;
    }

    mfxStatus sts = MFX_ERR_NONE;
    mfxStatus checkSts = MFX_ERR_NONE;

    MFX_CHECK_NULL_PTR1(par);
    sts = CheckExtBufferHeaders(par->NumExtParam, par->ExtParam);
    MFX_CHECK_STS(sts);
    MFX_CHECK(par->IOPattern == m_video.IOPattern, MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);

    eMFXHWType platform = m_pCore->GetHWType();

    VP9MfxVideoParam parBeforeReset(m_video);
    VP9MfxVideoParam parAfterReset(*par, platform);

    ENCODE_CAPS_VP9 caps = {};
    m_ddi->QueryEncodeCaps(caps);

    InheritDefaults(parAfterReset, parBeforeReset);
    parAfterReset.CalculateInternalParams();

    // get validated and properly initialized set of parameters for encoding
    checkSts = CheckParametersAndSetDefaults(parAfterReset, caps);
    MFX_CHECK(checkSts >= 0, checkSts);

    // check that no re-allocation is required
    // make sure LowPower not changed - i.e. encoder type the same
    MFX_CHECK(parBeforeReset.mfx.CodecProfile == parAfterReset.mfx.CodecProfile
        && parBeforeReset.AsyncDepth == parAfterReset.AsyncDepth
        && parBeforeReset.m_inMemType == parAfterReset.m_inMemType
        && m_initWidth >= parAfterReset.mfx.FrameInfo.Width
        && m_initHeight >= parAfterReset.mfx.FrameInfo.Height
        && m_initWidth >= parAfterReset.mfx.FrameInfo.CropW
        && m_initHeight >= parAfterReset.mfx.FrameInfo.CropH
        && (parAfterReset.m_inMemType == INPUT_VIDEO_MEMORY
            || m_rawLocalFrames.Num() >= CalcNumSurfRaw(parAfterReset))
        && m_reconFrames.Num() >= CalcNumSurfRecon(parAfterReset)
        && parBeforeReset.mfx.RateControlMethod == parAfterReset.mfx.RateControlMethod
        && parBeforeReset.mfx.LowPower == parAfterReset.mfx.LowPower,
        MFX_ERR_INCOMPATIBLE_VIDEO_PARAM);

    m_video = parAfterReset;
    m_resetBrc = isBrcResetRequired(parBeforeReset, parAfterReset);

    if (m_frameArrivalOrder)
    {
        m_bStartIVFSequence = false;
    }

    const mfxExtEncoderResetOption * pExtReset = (mfxExtEncoderResetOption*)GetExtBuffer(par->ExtParam, par->NumExtParam, MFX_EXTBUFF_ENCODER_RESET_OPTION);
    if (parBeforeReset.mfx.GopPicSize != parAfterReset.mfx.GopPicSize ||
        (pExtReset && IsOn(pExtReset->StartNewSequence)))
    {
        m_frameArrivalOrder = 0;
        m_frameOrderInGop = 0;
        m_frameOrderInRefStructure = 0;
        Zero(m_prevFrameParam);
        m_bStartIVFSequence = true;
        //m_bStartIVFSequence = true;
        // below commented code is to completely reset encoding pipeline
        /*
        // release all the reconstructed frames
        ReleaseDpbFrames(m_pCore, m_dpb);
        Zero(m_dpb);
        // release all the tasks
        for (std::list<Task>::iterator i = m_accepted.begin(); i != m_accepted.end(); i ++)
        {
            sts = FreeTask(m_pCore, *i);
            MFX_CHECK_STS(sts);
        }
        for (std::list<Task>::iterator i = m_submitted.begin(); i != m_submitted.end(); i++)
        {
            sts = FreeTask(m_pCore, *i);
            MFX_CHECK_STS(sts);
        }
        //m_accepted.resize(0);
        //m_submitted.resize(0);
        //m_free.resize(CalcNumTasks(m_video));
        m_taskIdForDriver = 0;
        */
    }
    else
    {
        // check that changes in encoding parameters don't violate VP9 specification
        const mfxExtVP9Param& extParBefore = GetExtBufferRef(parBeforeReset);
        const mfxExtVP9Param& extParAfter = GetExtBufferRef(parAfterReset);
        MFX_CHECK(m_frameArrivalOrder == 0 ||
            (extParAfter.FrameWidth <= MAX_UPSCALE_RATIO * extParBefore.FrameWidth &&
            extParAfter.FrameHeight <= MAX_UPSCALE_RATIO * extParBefore.FrameHeight &&
            static_cast<mfxF64>(extParAfter.FrameWidth) >=
            static_cast<mfxF64>(extParBefore.FrameWidth) / MAX_DOWNSCALE_RATIO &&
            static_cast<mfxF64>(extParAfter.FrameHeight) >=
            static_cast<mfxF64>(extParBefore.FrameHeight) / MAX_DOWNSCALE_RATIO),
            MFX_ERR_INVALID_VIDEO_PARAM);

        // so far dynamic scaling isn't supported together with temporal scalability
        if ((extParAfter.FrameWidth != extParBefore.FrameWidth ||
            extParAfter.FrameHeight != extParBefore.FrameHeight) &&
            parAfterReset.m_numLayers > 1)
        {
            return MFX_ERR_INVALID_VIDEO_PARAM;
        }

#if (MFX_VERSION >= 1029)
        // dynamic scaling and tiles don't work together
        if ((extParAfter.FrameWidth != extParBefore.FrameWidth ||
            extParAfter.FrameHeight != extParBefore.FrameHeight) &&
            (extParAfter.NumTileRows > 1 || extParAfter.NumTileColumns > 1))
        {
            return MFX_ERR_INVALID_VIDEO_PARAM;
        }

        // Tile switching is unsupported by driver for Gen11+
        if (platform > MFX_HW_ICL_LP &&
            (extParBefore.NumTileColumns > 1 || extParBefore.NumTileRows > 1) &&
            extParAfter.NumTileColumns == 1 && extParAfter.NumTileRows == 1)
        {
            return MFX_ERR_INVALID_VIDEO_PARAM;
        }
#endif

        // dynamic scaling and segmentation don't work together
        const mfxExtVP9Segmentation& seg = GetExtBufferRef(parAfterReset);
        if ((extParAfter.FrameWidth != extParBefore.FrameWidth ||
            extParAfter.FrameHeight != extParBefore.FrameHeight) &&
            seg.NumSegments > 1)
        {
            return MFX_ERR_INVALID_VIDEO_PARAM;
        }
    }

    sts = m_ddi->Reset(m_video);
    MFX_CHECK_STS(sts);

    m_videoForParamChange.push_back(m_video);

    return checkSts;
}

mfxStatus MFXVideoENCODEVP9_HW::RemoveObsoleteParameters()
{
    if (m_videoForParamChange.size() > 1) // m_videoForParamChange.back() contains latest encoding parameters. It shouldn't be removed.
    {
        std::list<VP9MfxVideoParam>::iterator par = m_videoForParamChange.begin();
        std::list<VP9MfxVideoParam>::iterator latest = m_videoForParamChange.end();
        latest--;
        while (par != latest)
        {
            if (m_accepted.end() == std::find_if(m_accepted.begin(), m_accepted.end(), FindTaskByMfxVideoParam(&*par)) && // [par] isn't referenced by "acceped" Tasks
                m_submitted.end() == std::find_if(m_submitted.begin(), m_submitted.end(), FindTaskByMfxVideoParam(&*par))) // [par] isn't referenced by "submitted" Tasks
            {
                // [par] isn't referenced by any Task submitted to the MFXVideoENCODEVP9_HW. It's obsolete and can be safely removed.
                par = m_videoForParamChange.erase(par);
            }
            else
            {
                par++;
            }
        }
    }

    return MFX_ERR_NONE;
}

mfxStatus MFXVideoENCODEVP9_HW::EncodeFrameSubmit(mfxEncodeCtrl *ctrl, mfxFrameSurface1 *surface, mfxBitstream *bs, mfxThreadTask *task)
{
    MFX_LTRACE_1(MFX_TRACE_LEVEL_DEFAULT, "MFXVideoENCODEVP9_HW::EncodeFrameSubmit ", "Frame %d", m_frameArrivalOrder); 

    if (m_initialized == false || m_videoForParamChange.size() == 0)
    {
        return MFX_ERR_NOT_INITIALIZED;
    }

    if (ctrl)
    {
        mfxStatus sts = CheckExtBufferHeaders(ctrl->NumExtParam, ctrl->ExtParam, true);
        MFX_CHECK_STS(sts);
    }

    mfxStatus checkSts = CheckBitstream(m_video, bs);
    MFX_CHECK_STS(checkSts);

    mfxStatus bufferingSts = MFX_ERR_NONE;

    {
        UMC::AutomaticUMCMutex guard(m_taskMutex);

        mfxStatus sts = RemoveObsoleteParameters();
        MFX_CHECK_STS(sts);

        if (m_drainState == true && surface)
        {
            return MFX_ERR_UNDEFINED_BEHAVIOR;
        }

        if (surface == 0)
        {
            // entering "drain" state. In this state new frames aren't accepted.
            m_drainState = true;

            if (m_numBufferedFrames == 0)
            {
                // all the buffered frames are drained. Exit "drain state" and notify application
                m_drainState = false;
                return MFX_ERR_MORE_DATA;
            }
            else
            {
                m_numBufferedFrames --;
            }
        }
        else
        {
            // take Task from the pool and initialize with arrived frame
            if (m_free.size() == 0)
            {
                // no tasks in the pool
                return MFX_WRN_DEVICE_BUSY;
            }
            // check that this input surface isn't used for encoding
            MFX_CHECK(m_accepted.end() == std::find_if(m_accepted.begin(), m_accepted.end(), FindTaskByRawSurface(surface)),
                MFX_ERR_UNDEFINED_BEHAVIOR);
            MFX_CHECK(m_submitted.end() == std::find_if(m_submitted.begin(), m_submitted.end(), FindTaskByRawSurface(surface)),
                MFX_ERR_UNDEFINED_BEHAVIOR);

            Task& newFrame = m_free.front();
            Zero(newFrame);
            sts = m_rawFrames.GetFrame(surface, newFrame.m_pRawFrame);
            MFX_CHECK_STS(sts);
            sts = LockSurface(newFrame.m_pRawFrame, m_pCore);
            MFX_CHECK_STS(sts);

            ENCODE_CAPS_VP9 caps = {};
            sts = m_ddi->QueryEncodeCaps(caps);
            if (sts != MFX_ERR_NONE)
                return MFX_ERR_UNDEFINED_BEHAVIOR;

            checkSts = CheckSurface(m_video, *surface, m_initWidth, m_initHeight, caps);
            MFX_CHECK_STS(checkSts);

            if (ctrl)
            {
                mfxEncodeCtrl tmpCtrl = *ctrl;
                checkSts = CheckAndFixCtrl(m_video, tmpCtrl, caps);
                MFX_CHECK(checkSts >= MFX_ERR_NONE, checkSts);
                newFrame.m_ctrl = tmpCtrl;
            }

            newFrame.m_timeStamp = surface->Data.TimeStamp;
            newFrame.m_frameOrder = m_frameArrivalOrder;
            newFrame.m_taskIdForDriver = m_taskIdForDriver;

            newFrame.m_pParam = &m_videoForParamChange.back(); // always use latest encoding parameters

            newFrame.m_resetBrc = m_resetBrc;
            m_resetBrc = false; // BRC reset is triggered only once.

            m_accepted.splice(m_accepted.end(), m_free, m_free.begin());

            if (m_free.size() > 0)
            {
                m_numBufferedFrames++; // during "drain" state application may call EncodeFrameAsync w/o respective SyncOperation
                                       // that's why we need to use separate counter to undeerstand when all frames are finally drained
                bufferingSts = MFX_ERR_MORE_DATA_SUBMIT_TASK;
            }

            m_frameArrivalOrder++;
            m_taskIdForDriver = (m_taskIdForDriver + 1) % (MAX_TASK_ID + 1);
        }

        // place mfxBitstream to the queue
        if (bufferingSts == MFX_ERR_NONE)
        {
            m_outs.push(bs);
        }
    }

    *task = (mfxThreadTask)surface;

    MFX_CHECK_STS(bufferingSts);

    return checkSts;
}

mfxStatus MFXVideoENCODEVP9_HW::ConfigTask(Task &task)
{
    VP9FrameLevelParam frameParam = { };
    const VP9MfxVideoParam& curMfxPar = *task.m_pParam;
    mfxU16 forcedFrameType = task.m_ctrl.FrameType;
    mfxU8 frameType = (mfxU8)((task.m_frameOrder % curMfxPar.mfx.GopPicSize) == 0 || (forcedFrameType & MFX_FRAMETYPE_I) ? KEY_FRAME : INTER_FRAME);

    task.m_insertIVFSeqHeader = false;
    if (frameType == KEY_FRAME)
    {
        mfxExtVP9Param& opt = GetExtBufferRef(curMfxPar);
        if (opt.WriteIVFHeaders != MFX_CODINGOPTION_OFF &&
            m_frameOrderInGop == 0 && m_bStartIVFSequence == true)
        {
            task.m_insertIVFSeqHeader = true;
        }

        m_frameOrderInGop = 0;
    }

    task.m_frameOrderInGop = m_frameOrderInGop;

    mfxExtVP9Param const &extPar = GetActualExtBufferRef(curMfxPar, task.m_ctrl);

    mfxU32 prevFrameOrderInRefStructure = m_frameOrderInRefStructure;

    if (m_frameOrderInGop == 0 ||
        (m_prevFrameParam.width != extPar.FrameWidth ||
        m_prevFrameParam.height != extPar.FrameHeight))
    {
        m_frameOrderInRefStructure = 0;
    }

    task.m_frameOrderInRefStructure = m_frameOrderInRefStructure;

    eMFXHWType platform = m_pCore->GetHWType();
    mfxStatus sts = SetFramesParams(curMfxPar, task, frameType, frameParam, platform);

    task.m_pRecFrame = 0;
    task.m_pOutBs = 0;
    task.m_pRawLocalFrame = 0;

    if (curMfxPar.m_inMemType == INPUT_SYSTEM_MEMORY)
    {
        task.m_pRawLocalFrame = m_rawLocalFrames.GetFreeFrame();
        MFX_CHECK(task.m_pRawLocalFrame != 0, MFX_WRN_DEVICE_BUSY);
    }
    task.m_pRecFrame = m_reconFrames.GetFreeFrame();
    MFX_CHECK(task.m_pRecFrame != 0, MFX_WRN_DEVICE_BUSY);
    task.m_pRecFrame->frameOrder = task.m_frameOrder;

    task.m_pOutBs = m_outBitstreams.GetFreeFrame();
    MFX_CHECK(task.m_pOutBs != 0, MFX_WRN_DEVICE_BUSY);

    if (frameParam.segmentation == APP_SEGMENTATION)
    {
        task.m_pSegmentMap = m_segmentMaps.GetFreeFrame();
        MFX_CHECK(task.m_pSegmentMap != 0, MFX_WRN_DEVICE_BUSY);
    }

    sts = DecideOnRefListAndDPBRefresh(curMfxPar, &task, m_dpb, frameParam, prevFrameOrderInRefStructure);

    task.m_frameParam = frameParam;

    task.m_pRecFrame->pSurface->Data.FrameOrder = task.m_frameOrder;

    if (task.m_frameParam.frameType == KEY_FRAME)
    {
        task.m_pRecRefFrames[REF_LAST] = task.m_pRecRefFrames[REF_GOLD] = task.m_pRecRefFrames[REF_ALT] = 0;
    }
    else
    {
        mfxU8 idxLast = task.m_frameParam.refList[REF_LAST];
        mfxU8 idxGold = task.m_frameParam.refList[REF_GOLD];
        mfxU8 idxAlt = task.m_frameParam.refList[REF_ALT];
        task.m_pRecRefFrames[REF_LAST] = m_dpb[idxLast];
        task.m_pRecRefFrames[REF_GOLD] = m_dpb[idxGold] != m_dpb[idxLast] ? m_dpb[idxGold] : 0;
        task.m_pRecRefFrames[REF_ALT] = m_dpb[idxAlt] != m_dpb[idxLast] && m_dpb[idxAlt] != m_dpb[idxGold] ? m_dpb[idxAlt] : 0;
    }

    sts = LockSurface(task.m_pRecFrame, m_pCore);
    MFX_CHECK_STS(sts);
    sts = LockSurface(task.m_pRawLocalFrame, m_pCore);
    MFX_CHECK_STS(sts);
    sts = LockSurface(task.m_pOutBs, m_pCore);
    MFX_CHECK_STS(sts);
    sts = LockSurface(task.m_pSegmentMap, m_pCore);
    MFX_CHECK_STS(sts);

    UpdateDpb(frameParam, task.m_pRecFrame, m_dpb, m_pCore);

    m_prevFrameParam = task.m_frameParam;

    return sts;
}

mfxStatus MFXVideoENCODEVP9_HW::Execute(mfxThreadTask task, mfxU32 /*uid_p*/, mfxU32 /*uid_a*/)
{
    if (m_initialized == false)
    {
        return MFX_ERR_NOT_INITIALIZED;
    }

    mfxFrameSurface1* pSurf = (mfxFrameSurface1*)task;

    // submit frame to the driver (if any)
    if (m_accepted.size()) // we have something to submit to driver
    {
        Task& newFrame = m_accepted.front(); // no mutex required here since splice() method doesn't cause race conditions for getting and modification of existing elements
        if (newFrame.m_pRawFrame->pSurface == pSurf) // frame pointed by pSurf isn't submitted yet - let's proceed with submission
        {
            MFX_LTRACE_1(MFX_TRACE_LEVEL_DEFAULT, "MFXVideoENCODEVP9_HW::SubmitFrame ", "Frame %d", newFrame.m_frameOrder);
            mfxStatus sts = MFX_ERR_NONE;
            const VP9MfxVideoParam& curMfxPar = *newFrame.m_pParam;
            newFrame.m_pPrevSegment = &m_prevSegment;

            sts = ConfigTask(newFrame);
            MFX_CHECK_STS(sts);

            mfxFrameSurface1    *pSurface = 0;
            mfxHDLPair surfaceHDL = {};

            // copy input frame from SYSTEM to VIDEO memory (if required)
            sts = CopyRawSurfaceToVideoMemory(m_pCore, curMfxPar, newFrame);
            MFX_CHECK_STS(sts);

            sts = GetInputSurface(m_pCore, curMfxPar, newFrame, pSurface);
            MFX_CHECK_STS(sts);

            // get handle to input frame in VIDEO memory (either external or local)
            sts = GetNativeHandleToRawSurface(*m_pCore, pSurface->Data.MemId, &surfaceHDL.first, m_video);
            MFX_CHECK_STS(sts);

            MFX_CHECK_STS(sts);
            MFX_CHECK(surfaceHDL.first != 0, MFX_ERR_UNDEFINED_BEHAVIOR);

            // submit the frame to the driver
            sts = m_ddi->Execute(newFrame, surfaceHDL);
            MFX_CHECK_STS(sts);

            IncreaseRef(newFrame.m_pRecFrame);

            {
                UMC::AutomaticUMCMutex guard(m_taskMutex);
                m_submitted.splice(m_submitted.end(), m_accepted, m_accepted.begin());
            }

            // save segmentation params which were applied to current frame
            mfxExtVP9Segmentation const & seg = GetActualExtBufferRef(curMfxPar, newFrame.m_ctrl);
            CopySegmentationBuffer(m_prevSegment, seg);

            m_frameOrderInGop++;
            m_frameOrderInRefStructure++;
        }
    }

    // get frame from the driver (if any)
    if (m_submitted.size() == m_video.AsyncDepth || // [AsyncDepth] asynchronous tasks are submitted to driver. It's time to make synchronization.
        (pSurf == 0 && m_submitted.size())) // or we are in "drain state" - need to synchronize all submitted to driver w/o any conditions
    {
        Task& frameToGet = m_submitted.front();

        MFX_LTRACE_1(MFX_TRACE_LEVEL_DEFAULT, "MFXVideoENCODEVP9_HW::QueryFrame ", "Frame %d", frameToGet.m_frameOrder);
        assert(m_outs.size() > 0);

        mfxStatus sts = MFX_ERR_NONE;

        sts = m_ddi->QueryStatus(frameToGet);
        if (sts == MFX_WRN_DEVICE_BUSY)
        {
            return MFX_TASK_BUSY;
        }
        MFX_CHECK_STS(sts);

        frameToGet.m_pBitsteam = m_outs.front();
        sts = UpdateBitstream(frameToGet);
        MFX_CHECK_STS(sts);

        {
            UMC::AutomaticUMCMutex guard(m_taskMutex);
            m_outs.pop();
            sts = FreeTask(m_pCore, frameToGet);
            MFX_CHECK_STS(sts);
            m_free.splice(m_free.end(), m_submitted, m_submitted.begin());
        }
    }

    return MFX_TASK_DONE;
}

mfxStatus MFXVideoENCODEVP9_HW::FreeResources(mfxThreadTask /*task*/, mfxStatus /*sts*/)
{
    return MFX_ERR_NONE;
}

mfxStatus MFXVideoENCODEVP9_HW::Close()
{
    if (m_initialized == false)
    {
        return MFX_ERR_NOT_INITIALIZED;
    }

    mfxStatus sts = MFX_ERR_NONE;

    for (std::list<Task>::iterator i = m_accepted.begin(); i != m_accepted.end(); i++)
    {
        sts = FreeTask(m_pCore, *i);
        MFX_CHECK_STS(sts);
    }
    for (std::list<Task>::iterator i = m_submitted.begin(); i != m_submitted.end(); i++)
    {
        sts = FreeTask(m_pCore, *i);
        MFX_CHECK_STS(sts);
    }

    sts = m_opaqFrames.Release();
    MFX_CHECK_STS(sts);
    sts = m_rawLocalFrames.Release();
    MFX_CHECK_STS(sts);
    sts = m_reconFrames.Release();
    MFX_CHECK_STS(sts);
    sts = m_outBitstreams.Release();
    MFX_CHECK_STS(sts);
    sts = m_segmentMaps.Release();
    MFX_CHECK_STS(sts);

    delete[] m_prevSegment.SegmentId;
    m_prevSegment.SegmentId = nullptr;

    m_initialized = false;

    return MFX_ERR_NONE;
}

mfxStatus MFXVideoENCODEVP9_HW::GetVideoParam(mfxVideoParam *par)
{
    if (m_initialized == false)
    {
        return MFX_ERR_NOT_INITIALIZED;
    }

    MFX_CHECK_NULL_PTR1(par);
    mfxStatus sts = CheckExtBufferHeaders(par->NumExtParam, par->ExtParam);
    MFX_CHECK_STS(sts);

    par->AsyncDepth = m_video.AsyncDepth;
    par->IOPattern = m_video.IOPattern;
    par->Protected = m_video.Protected;

    par->mfx = m_video.mfx;

    for (mfxU8 i = 0; i < par->NumExtParam; i++)
    {
        mfxExtBuffer *pOutBuf = par->ExtParam[i];
        if (pOutBuf == 0)
        {
            return MFX_ERR_NULL_PTR;
        }
        mfxExtBuffer *pLocalBuf = GetExtendedBuffer(m_video.ExtParam, m_video.NumExtParam, pOutBuf->BufferId);
        if (pLocalBuf && (pOutBuf->BufferSz == pLocalBuf->BufferSz))
        {
            char* src = reinterpret_cast <char*> (pLocalBuf);
            char* dst   = reinterpret_cast <char*> (pOutBuf);
            std::copy(src, src + pLocalBuf->BufferSz, dst);
        }
        else
        {
            assert(!"Encoder doesn't have requested buffer or buffer sizes are different!");
            return MFX_ERR_UNDEFINED_BEHAVIOR;
        }
    }

    return MFX_ERR_NONE;
}

inline mfxStatus UpdatePictureHeader(mfxU32 frameLen, mfxU32 frameNum, mfxU8* pPictureHeader, mfxU32 bufferSize)
{
    mfxU32 ivf_frame_header[3] = {frameLen, frameNum, 0x00000000};
    if (bufferSize < sizeof(ivf_frame_header))
        return MFX_ERR_MORE_DATA;

    std::copy(std::begin(ivf_frame_header),std::end(ivf_frame_header), reinterpret_cast<mfxU32*>(pPictureHeader));

    return MFX_ERR_NONE;
};

//#define SUPERFRAME_WA_MSDK

mfxStatus MFXVideoENCODEVP9_HW::UpdateBitstream(
    Task & task)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_DEFAULT, "MFXVideoENCODEVP9_HW::UpdateBitstream");

    mfxFrameData bitstream = {};

    FrameLocker lock(m_pCore, bitstream, task.m_pOutBs->pSurface->Data.MemId);
    if (bitstream.Y == 0)
    {
        return MFX_ERR_LOCK_MEMORY;
    }

    mfxU32   bsSizeToCopy  = task.m_bsDataLength;

#ifdef SUPERFRAME_WA_MSDK
    // "SF" below stands for Superframe
    const mfxU32 hiddenFrameSize = bsSizeToCopy - IVF_PIC_HEADER_SIZE_BYTES - (task.m_insertIVFSeqHeader ? IVF_SEQ_HEADER_SIZE_BYTES : 0);
    const mfxU32 frameSizeLen = (CeilLog2(hiddenFrameSize) + 7) / 8;
    const mfxU32 showFrameSize = 1;
    const mfxU32 SFHeaderSize = 1;
    const mfxU32 extraSizeForSF = showFrameSize + SFHeaderSize + 2 * frameSizeLen + SFHeaderSize;
#endif

    mfxU32   bsSizeAvail   = task.m_pBitsteam->MaxLength - task.m_pBitsteam->DataOffset - task.m_pBitsteam->DataLength;
    mfxU8 *  bsData        = task.m_pBitsteam->Data + task.m_pBitsteam->DataOffset + task.m_pBitsteam->DataLength;

    MFX_CHECK(bsSizeToCopy <= bsSizeAvail, MFX_ERR_NOT_ENOUGH_BUFFER);

    // Avoid segfaults on very high bitrates
    if (bsSizeToCopy > m_maxBsSize)
    {
        lock.Unlock();
        return MFX_ERR_DEVICE_FAILED;
    }

    // Copy compressed picture from d3d surface to buffer in system memory
    if (bsSizeToCopy)
    {
        mfxSize roi = {(int32_t)bsSizeToCopy,1};
        FastCopy::Copy(bsData, bsSizeToCopy, bitstream.Y, bitstream.Pitch, roi, COPY_VIDEO_TO_SYS);
    }

    mfxExtVP9Param& opt = GetExtBufferRef(m_video);

#ifdef SUPERFRAME_WA_MSDK
    mfxU8* pEnd = bsData + bsSizeToCopy;
    mfxU8* p = pEnd;

    if (task.m_frameParam.showFrame == 0)
    {
        const mfxU8 frameMarker = 0x2 << 6;
        const mfxU8 profile = 0x0 << 4;
        const mfxU8 showExistingFrame = 0x1 << 3;
        mfxU8 frameToShow = 0;
        for (mfxU8 i = 0; i < DPB_SIZE; i++)
        {
            if (task.m_frameParam.refreshRefFrames[i] == 1)
            {
                frameToShow = i;
                break;
            }
        }
        const mfxU8 showFrame = frameMarker | profile | showExistingFrame | frameToShow;

        const mfxU8 SFMarker = 0x06 << 5;
        const mfxU8 bytesPerFrameSizeM1 = (static_cast<mfxU8>(frameSizeLen) - 1) << 3;
        const mfxU8 framesInSuperFrameM1 = 1;
        const mfxU8 superFrameHeader = SFMarker | bytesPerFrameSizeM1 | framesInSuperFrameM1;

        *p = showFrame;
        p++;

        *p = superFrameHeader;
        p++;

        for (mfxU8 i = 0; i < frameSizeLen; i++)
        {
            *p = static_cast<mfxU8>(0xff & (hiddenFrameSize >> (8 * i)));
            p++;
        }

        for (mfxU8 i = 0; i < frameSizeLen; i++)
        {
            *p = static_cast<mfxU8>(0xff & (showFrameSize >> (8 * i)));
            p++;
        }
        *p = superFrameHeader;
        p++;
    }

    task.m_pBitsteam->DataLength += static_cast<mfxU32>(p - pEnd);
#endif

    task.m_pBitsteam->DataLength += bsSizeToCopy;

    if (opt.WriteIVFHeaders != MFX_CODINGOPTION_OFF)
    {
        mfxU8 * pIVFPicHeader = task.m_insertIVFSeqHeader ? bsData + IVF_SEQ_HEADER_SIZE_BYTES : bsData;

        mfxStatus sts = UpdatePictureHeader(task.m_pBitsteam->DataLength - IVF_PIC_HEADER_SIZE_BYTES - (task.m_insertIVFSeqHeader ? IVF_SEQ_HEADER_SIZE_BYTES : 0), (mfxU32)task.m_frameOrder, pIVFPicHeader, bsSizeAvail - IVF_SEQ_HEADER_SIZE_BYTES);
        MFX_CHECK_STS(sts);
    }

    // Update bitstream fields
    task.m_pBitsteam->TimeStamp = task.m_timeStamp;
    task.m_pBitsteam->FrameType = mfxU16(task.m_frameParam.frameType == KEY_FRAME ? MFX_FRAMETYPE_I : MFX_FRAMETYPE_P);
    task.m_pBitsteam->PicStruct = MFX_PICSTRUCT_PROGRESSIVE;

    return MFX_ERR_NONE;
}


} // MfxHwVP9Encode
