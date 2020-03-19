// Copyright (c) 2017-2019 Intel Corporation
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

#include "mfx_common.h"

#if defined (MFX_ENABLE_MJPEG_VIDEO_ENCODE)

#include "mfx_mjpeg_encode_hw.h"
#include "mfx_enc_common.h"
#include "mfx_task.h"
#include "umc_defs.h"
#include "mfx_ext_buffers.h"

using namespace MfxHwMJpegEncode;

template<typename T>
bool setPlaneROI(T value, T* pDst, int dstStep, mfxSize roiSize)
{
    if (!pDst || roiSize.width < 0 || roiSize.height < 0)
        return false;

    for (int h = 0; h < roiSize.height; h++) {
        std::fill(pDst, pDst + roiSize.width, value);
        pDst = (T *)((unsigned char*)pDst + dstStep);
    }

    return true;
}

template<typename T>
bool swapChannels(const T* pSrc, int srcStep,
    T* pDst, int dstStep, mfxSize roiSize, const int dstOrder[3])
{
    const T *src0, *src1, *src2;
    T *dst = pDst;
    int width = roiSize.width * 4, height = roiSize.height;
    int h;
    int s;

    if (!pSrc || !pDst || srcStep <= 0 || dstStep <= 0 || roiSize.width <= 0 || roiSize.height <= 0)
        return false;

    if ((dstOrder[0] < 0) || (dstOrder[0] > 2)) return false;
    if ((dstOrder[1] < 0) || (dstOrder[1] > 2)) return false;
    if ((dstOrder[2] < 0) || (dstOrder[2] > 2)) return false;

    src0 = (T*)(pSrc + dstOrder[0]);
    src1 = (T*)(pSrc + dstOrder[1]);
    src2 = (T*)(pSrc + dstOrder[2]);

    if ((srcStep == dstStep) && (srcStep == width)) {
        width *= height;
        height = 1;
    }

    for (h = 0; h < height; h++) {
        for (s = 0; s < width; s += 4) {
            T x0 = src0[s];
            T x1 = src1[s];
            T x2 = src2[s];
            dst[s] = x0;
            dst[s + 1] = x1;
            dst[s + 2] = x2;
        }
        src0 += srcStep, src1 += srcStep, src2 += srcStep;
        dst += dstStep;
    }

    return true;
}



MfxFrameAllocResponse::MfxFrameAllocResponse()
    :mfxFrameAllocResponse() , m_core(0)
{
}

MfxFrameAllocResponse::~MfxFrameAllocResponse()
{
    Free();
}

void MfxFrameAllocResponse::Free()
{
    if (m_core)
    {
        if (MFX_HW_D3D11 == m_core->GetVAType() && m_responseQueue.size())
        {
            for (size_t i = 0; i < m_responseQueue.size(); i++)
                m_core->FreeFrames(&m_responseQueue[i]);
        }
        else
        {
            if (mids)
            {
                m_core->FreeFrames(this);
            }
        }
        m_core = NULL;
    }

}

mfxStatus MfxFrameAllocResponse::Alloc(
    VideoCORE *            core,
    mfxFrameAllocRequest & req,
    bool isCopyRequired = true)
{
    mfxStatus sts = MFX_ERR_NONE;

    if (m_core || core == NULL)
    {
        sts = MFX_ERR_MEMORY_ALLOC;
    }
    MFX_CHECK_STS(sts);

    m_core = core;

    if (MFX_HW_D3D11 == core->GetVAType())
    {
        mfxFrameAllocRequest tmp = req;
        tmp.NumFrameMin = tmp.NumFrameSuggested = 1;

        m_responseQueue.resize(req.NumFrameMin);
        m_mids.resize(req.NumFrameMin);

        for (int i = 0; i < req.NumFrameMin; i++)
        {
            sts = core->AllocFrames(&tmp, &m_responseQueue[i], isCopyRequired);
            MFX_CHECK_STS(sts);
            m_mids[i] = m_responseQueue[i].mids[0];
        }

        mids = &m_mids[0];
        NumFrameActual = req.NumFrameMin;
    }
    else
    {
        sts = core->AllocFrames(&req, this, isCopyRequired);
        MFX_CHECK_STS(sts);
    }

    if (NumFrameActual < req.NumFrameMin)
        return MFX_ERR_MEMORY_ALLOC;

    return MFX_ERR_NONE;
}

MFXVideoENCODEMJPEG_HW::MFXVideoENCODEMJPEG_HW(VideoCORE *core, mfxStatus *sts)
    : m_checkedJpegQT()
    , m_checkedJpegHT()
{
    m_pCore        = core;
    m_bInitialized = false;
    m_deviceFailed = false;
    m_isOpaqIn     = false;
    m_counter      = 1;

    memset(&m_vFirstParam, 0, sizeof(mfxVideoParam));
    memset(&m_vParam, 0, sizeof(mfxVideoParam));
    memset(&m_raw, 0, sizeof(m_raw));

    *sts = (core ? MFX_ERR_NONE : MFX_ERR_NULL_PTR);
}

mfxStatus MFXVideoENCODEMJPEG_HW::Query(VideoCORE * core, mfxVideoParam *in, mfxVideoParam *out)
{
    mfxU32 isCorrected = 0;
    mfxU32 isInvalid = 0;
    MFX_CHECK_NULL_PTR2(core, out);

    if (!in)
    {
        memset(&out->mfx, 0, sizeof(out->mfx));

        out->mfx.FrameInfo.FourCC        = MFX_FOURCC_NV12;
        out->mfx.FrameInfo.Width         = 1;
        out->mfx.FrameInfo.Height        = 1;
        out->mfx.FrameInfo.CropX         = 0;
        out->mfx.FrameInfo.CropY         = 0;
        out->mfx.FrameInfo.CropW         = 1;
        out->mfx.FrameInfo.CropH         = 1;
        out->mfx.FrameInfo.FrameRateExtN = 1;
        out->mfx.FrameInfo.FrameRateExtD = 1;
        out->mfx.FrameInfo.AspectRatioW  = 1;
        out->mfx.FrameInfo.AspectRatioH  = 1;
        out->mfx.FrameInfo.PicStruct     = 1;
        out->mfx.FrameInfo.ChromaFormat  = MFX_CHROMAFORMAT_YUV420;
        out->mfx.CodecId                 = MFX_CODEC_JPEG;
        out->mfx.CodecLevel              = 0;
        out->mfx.CodecProfile            = MFX_PROFILE_JPEG_BASELINE;
        out->mfx.NumThread               = 1;
        out->mfx.Interleaved             = MFX_SCANTYPE_INTERLEAVED;
        out->mfx.Quality                 = 1;
        out->mfx.RestartInterval         = 0;
        out->AsyncDepth                  = 1;
        out->IOPattern                   = 1;
        out->Protected                   = 0;

        //Extended coding options
        mfxStatus sts = CheckExtBufferId(*out);
        if (sts != MFX_ERR_NONE)
            return MFX_WRN_PARTIAL_ACCELERATION;

        JpegEncCaps hwCaps = {};
        sts = QueryHwCaps(core, hwCaps);
        if (sts != MFX_ERR_NONE)
            return MFX_WRN_PARTIAL_ACCELERATION;
    }
    else
    {
        // Check HW caps
        JpegEncCaps hwCaps = {};
        mfxStatus sts = QueryHwCaps(core, hwCaps);
        if (sts != MFX_ERR_NONE)
            return MFX_WRN_PARTIAL_ACCELERATION;

        sts = CheckJpegParam(core, *in, hwCaps);
        if (sts == MFX_WRN_PARTIAL_ACCELERATION)
            return MFX_WRN_PARTIAL_ACCELERATION;
        else if (sts == MFX_ERR_INCOMPATIBLE_VIDEO_PARAM)
            isInvalid++;

        // Extended coding options
        sts = CheckExtBufferId(*in);
        if (sts != MFX_ERR_NONE)
            isInvalid++;
        sts = CheckExtBufferId(*out);
        if (sts != MFX_ERR_NONE)
            isInvalid++;

        // Check external buffers
        mfxExtJPEGQuantTables* qt_in  = (mfxExtJPEGQuantTables*)GetExtBuffer( in->ExtParam, in->NumExtParam, MFX_EXTBUFF_JPEG_QT );
        mfxExtJPEGQuantTables* qt_out = (mfxExtJPEGQuantTables*)GetExtBuffer( out->ExtParam, out->NumExtParam, MFX_EXTBUFF_JPEG_QT );
        mfxExtJPEGHuffmanTables* ht_in  = (mfxExtJPEGHuffmanTables*)GetExtBuffer( in->ExtParam, in->NumExtParam, MFX_EXTBUFF_JPEG_HUFFMAN );
        mfxExtJPEGHuffmanTables* ht_out = (mfxExtJPEGHuffmanTables*)GetExtBuffer( out->ExtParam, out->NumExtParam, MFX_EXTBUFF_JPEG_HUFFMAN );

        if ((qt_in == 0) != (qt_out == 0) ||
            (ht_in == 0) != (ht_out == 0))
            return MFX_ERR_UNDEFINED_BEHAVIOR;

        if (qt_in && qt_out)
        {
            if(qt_in->NumTable <= 4)
            {
                qt_out->NumTable = qt_in->NumTable;
                for(mfxU16 i=0; i<qt_out->NumTable; i++)
                    for(mfxU16 j=0; j<64; j++)
                        qt_out->Qm[i][j] = qt_in->Qm[i][j];
            }
            else
            {
                qt_out->NumTable = 0;
                for(mfxU16 i=0; i<4; i++)
                    for(mfxU16 j=0; j<64; j++)
                        qt_out->Qm[i][j] = 0;
                isInvalid++;
            }
        }

        if (ht_in && ht_out)
        {
            if(ht_in->NumDCTable <= 4)
            {
                ht_out->NumDCTable = ht_in->NumDCTable;
                for(mfxU16 i=0; i<ht_out->NumDCTable; i++)
                {
                    for(mfxU16 j=0; j<16; j++)
                        ht_out->DCTables[i].Bits[j] = ht_in->DCTables[i].Bits[j];
                    for(mfxU16 j=0; j<12; j++)
                        ht_out->DCTables[i].Values[j] = ht_in->DCTables[i].Values[j];
                }
            }
            else
            {
                ht_out->NumDCTable = 0;
                for(mfxU16 i=0; i<4; i++)
                {
                    for(mfxU16 j=0; j<16; j++)
                        ht_out->DCTables[i].Bits[j] = 0;
                    for(mfxU16 j=0; j<12; j++)
                        ht_out->DCTables[i].Values[j] = 0;
                }
                isInvalid++;
            }

            if(ht_in->NumACTable <= 4)
            {
                ht_out->NumACTable = ht_in->NumACTable;
                for(mfxU16 i=0; i<ht_out->NumACTable; i++)
                {
                    for(mfxU16 j=0; j<16; j++)
                        ht_out->ACTables[i].Bits[j] = ht_in->ACTables[i].Bits[j];
                    for(mfxU16 j=0; j<162; j++)
                        ht_out->ACTables[i].Values[j] = ht_in->ACTables[i].Values[j];
                }
            }
            else
            {
                ht_out->NumACTable = 0;
                for(mfxU16 i=0; i<4; i++)
                {
                    for(mfxU16 j=0; j<16; j++)
                        ht_out->ACTables[i].Bits[j] = 0;
                    for(mfxU16 j=0; j<162; j++)
                        ht_out->ACTables[i].Values[j] = 0;
                }
                isInvalid++;
            }
        }

        // Check options for correctness
        if (in->mfx.CodecId != MFX_CODEC_JPEG)
            isInvalid++;
        out->mfx.CodecId = MFX_CODEC_JPEG;

        // profile and level can be corrected
        switch(in->mfx.CodecProfile)
        {
            case MFX_PROFILE_JPEG_BASELINE:
            //case MFX_PROFILE_JPEG_EXTENDED:
            //case MFX_PROFILE_JPEG_PROGRESSIVE:
            //case MFX_PROFILE_JPEG_LOSSLESS:
            case MFX_PROFILE_UNKNOWN:
                out->mfx.CodecProfile = MFX_PROFILE_JPEG_BASELINE;
                break;
            default:
                isInvalid++;
                out->mfx.CodecProfile = MFX_PROFILE_UNKNOWN;
                break;
        }

        mfxU32 fourCC = in->mfx.FrameInfo.FourCC;
        mfxU16 chromaFormat = in->mfx.FrameInfo.ChromaFormat;

        if ((fourCC == 0 && chromaFormat == 0) ||
            (fourCC == MFX_FOURCC_NV12 && (chromaFormat == MFX_CHROMAFORMAT_YUV420 || chromaFormat == MFX_CHROMAFORMAT_YUV400)) ||
            (fourCC == MFX_FOURCC_YUY2 && chromaFormat == MFX_CHROMAFORMAT_YUV422H) ||
            (fourCC == MFX_FOURCC_RGB4 && chromaFormat == MFX_CHROMAFORMAT_YUV444))
        {
            out->mfx.FrameInfo.FourCC = in->mfx.FrameInfo.FourCC;
            out->mfx.FrameInfo.ChromaFormat = in->mfx.FrameInfo.ChromaFormat;
        }
        else
        {
            out->mfx.FrameInfo.FourCC = 0;
            out->mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
            return MFX_WRN_PARTIAL_ACCELERATION;
        }

        if (in->Protected != 0)
            isInvalid++;

        out->Protected = 0;
        out->AsyncDepth = in->AsyncDepth;

        //Check for valid framerate
        if((!in->mfx.FrameInfo.FrameRateExtN && in->mfx.FrameInfo.FrameRateExtD) ||
            (in->mfx.FrameInfo.FrameRateExtN && !in->mfx.FrameInfo.FrameRateExtD) ||
            (in->mfx.FrameInfo.FrameRateExtD && ((mfxF64)in->mfx.FrameInfo.FrameRateExtN / in->mfx.FrameInfo.FrameRateExtD) > 172))
        {
            isInvalid++;
            out->mfx.FrameInfo.FrameRateExtN = out->mfx.FrameInfo.FrameRateExtD = 0;
        }
        else
        {
            out->mfx.FrameInfo.FrameRateExtN = in->mfx.FrameInfo.FrameRateExtN;
            out->mfx.FrameInfo.FrameRateExtD = in->mfx.FrameInfo.FrameRateExtD;
        }

        switch(in->IOPattern)
        {
            case 0:
            case MFX_IOPATTERN_IN_SYSTEM_MEMORY:
            case MFX_IOPATTERN_IN_OPAQUE_MEMORY:
            case MFX_IOPATTERN_IN_VIDEO_MEMORY:
                out->IOPattern = in->IOPattern;
                break;
            default:
                isCorrected++;
                if(in->IOPattern & MFX_IOPATTERN_IN_SYSTEM_MEMORY)
                    out->IOPattern = MFX_IOPATTERN_IN_SYSTEM_MEMORY;
                else if(in->IOPattern & MFX_IOPATTERN_IN_VIDEO_MEMORY)
                    out->IOPattern = MFX_IOPATTERN_IN_VIDEO_MEMORY;
                else
                    out->IOPattern = 0;
        }

        out->mfx.NumThread = in->mfx.NumThread;
        if(out->mfx.NumThread < 1)
            out->mfx.NumThread = 1;

        // Check crops
        if (in->mfx.FrameInfo.CropH > in->mfx.FrameInfo.Height && in->mfx.FrameInfo.Height) {
            out->mfx.FrameInfo.CropH = 0;
            isInvalid ++;
        } else
            out->mfx.FrameInfo.CropH = in->mfx.FrameInfo.CropH;
        if (in->mfx.FrameInfo.CropW > in->mfx.FrameInfo.Width && in->mfx.FrameInfo.Width) {
            out->mfx.FrameInfo.CropW = 0;
            isInvalid ++;
        } else
            out->mfx.FrameInfo.CropW = in->mfx.FrameInfo.CropW;
        if (in->mfx.FrameInfo.CropX + in->mfx.FrameInfo.CropW > in->mfx.FrameInfo.Width) {
            out->mfx.FrameInfo.CropX = 0;
            isInvalid ++;
        } else
            out->mfx.FrameInfo.CropX = in->mfx.FrameInfo.CropX;
        if (in->mfx.FrameInfo.CropY + in->mfx.FrameInfo.CropH > in->mfx.FrameInfo.Height) {
            out->mfx.FrameInfo.CropY = 0;
            isInvalid ++;
        } else
            out->mfx.FrameInfo.CropY = in->mfx.FrameInfo.CropY;

        out->mfx.FrameInfo.AspectRatioW = in->mfx.FrameInfo.AspectRatioW;
        out->mfx.FrameInfo.AspectRatioH = in->mfx.FrameInfo.AspectRatioH;

        if (in->mfx.Quality > 100)
        {
            out->mfx.Quality = 100;
            isCorrected++;
        }
        else
        {
            out->mfx.Quality = in->mfx.Quality;
        }

        out->mfx.FrameInfo.Height = in->mfx.FrameInfo.Height;
        out->mfx.FrameInfo.Width = in->mfx.FrameInfo.Width;
        out->mfx.Interleaved = in->mfx.Interleaved;
        out->mfx.RestartInterval = in->mfx.RestartInterval;

        switch (in->mfx.FrameInfo.PicStruct)
        {
            case MFX_PICSTRUCT_UNKNOWN:
            case MFX_PICSTRUCT_PROGRESSIVE:
                out->mfx.FrameInfo.PicStruct = in->mfx.FrameInfo.PicStruct;
                break;
            case MFX_PICSTRUCT_FIELD_TFF:
            case MFX_PICSTRUCT_FIELD_BFF:
            //case MFX_PICSTRUCT_FIELD_REPEATED:
            //case MFX_PICSTRUCT_FRAME_DOUBLING:
            //case MFX_PICSTRUCT_FRAME_TRIPLING:
                return MFX_WRN_PARTIAL_ACCELERATION;
            default:
                isInvalid++;
                out->mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_UNKNOWN;
                break;
        }
    }

    if(isInvalid)
        return MFX_ERR_UNSUPPORTED;

    if(isCorrected)
        return MFX_WRN_INCOMPATIBLE_VIDEO_PARAM;

    return MFX_ERR_NONE;
}

// function to calculate required number of external surfaces and create request
mfxStatus MFXVideoENCODEMJPEG_HW::QueryIOSurf(VideoCORE * core, mfxVideoParam *par, mfxFrameAllocRequest *request)
{
    mfxStatus sts = MFX_ERR_NONE;

    JpegEncCaps hwCaps = { };
    sts = QueryHwCaps(core, hwCaps);
    if (sts != MFX_ERR_NONE)
        return MFX_WRN_PARTIAL_ACCELERATION;

    sts = CheckJpegParam(core, *par, hwCaps);
    if (sts == MFX_WRN_PARTIAL_ACCELERATION)
        return MFX_WRN_PARTIAL_ACCELERATION;

    // check for valid IOPattern
    mfxU16 inPattern = par->IOPattern & (MFX_IOPATTERN_IN_VIDEO_MEMORY | MFX_IOPATTERN_IN_SYSTEM_MEMORY | MFX_IOPATTERN_IN_OPAQUE_MEMORY);
    if ((par->IOPattern == 0) ||
        ((inPattern != MFX_IOPATTERN_IN_VIDEO_MEMORY) && (inPattern != MFX_IOPATTERN_IN_SYSTEM_MEMORY) && (inPattern != MFX_IOPATTERN_IN_OPAQUE_MEMORY)))
    {
        return MFX_ERR_INVALID_VIDEO_PARAM;
    }

    request->Info              = par->mfx.FrameInfo;

    if (inPattern == MFX_IOPATTERN_IN_SYSTEM_MEMORY)
    {
        request->NumFrameMin = 1;
        request->Type =
            MFX_MEMTYPE_EXTERNAL_FRAME |
            MFX_MEMTYPE_FROM_ENCODE |
            MFX_MEMTYPE_SYSTEM_MEMORY;
    }
    else // MFX_IOPATTERN_IN_VIDEO_MEMORY || MFX_IOPATTERN_IN_OPAQUE_MEMORY
    {
        request->NumFrameMin = 1;
        request->Type = MFX_MEMTYPE_FROM_ENCODE | MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET;
        request->Type |= (inPattern == MFX_IOPATTERN_IN_OPAQUE_MEMORY)
            ? MFX_MEMTYPE_OPAQUE_FRAME
            : MFX_MEMTYPE_EXTERNAL_FRAME;
    }
    request->NumFrameSuggested = std::max(request->NumFrameMin, par->AsyncDepth);

    return MFX_ERR_NONE;
}

mfxStatus MFXVideoENCODEMJPEG_HW::Init(mfxVideoParam *par)
{
    mfxStatus sts, QueryStatus;

    if (m_bInitialized || !m_pCore)
        return MFX_ERR_UNDEFINED_BEHAVIOR;

    if(par == NULL)
        return MFX_ERR_NULL_PTR;

    MFX_CHECK( CheckExtBufferId(*par) == MFX_ERR_NONE, MFX_ERR_INVALID_VIDEO_PARAM );

    mfxExtJPEGQuantTables*    jpegQT       = (mfxExtJPEGQuantTables*)   GetExtBuffer( par->ExtParam, par->NumExtParam, MFX_EXTBUFF_JPEG_QT );
    mfxExtJPEGHuffmanTables*  jpegHT       = (mfxExtJPEGHuffmanTables*) GetExtBuffer( par->ExtParam, par->NumExtParam, MFX_EXTBUFF_JPEG_HUFFMAN );
    mfxExtOpaqueSurfaceAlloc* opaqAllocReq = (mfxExtOpaqueSurfaceAlloc*)GetExtBuffer( par->ExtParam, par->NumExtParam, MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION );

    mfxVideoParam checked;
    mfxU16 ext_counter = 0;
    checked = *par;

    if (jpegQT)
    {
        m_checkedJpegQT = *jpegQT;
        m_pCheckedExt[ext_counter++] = &m_checkedJpegQT.Header;
    }
    else
    {
        memset(&m_checkedJpegQT, 0, sizeof(m_checkedJpegQT));
        m_checkedJpegQT.Header.BufferId = MFX_EXTBUFF_JPEG_QT;
        m_checkedJpegQT.Header.BufferSz = sizeof(m_checkedJpegQT);
    }
    if (jpegHT)
    {
        m_checkedJpegHT = *jpegHT;
        m_pCheckedExt[ext_counter++] = &m_checkedJpegHT.Header;
    }
    else
    {
        memset(&m_checkedJpegHT, 0, sizeof(m_checkedJpegHT));
        m_checkedJpegHT.Header.BufferId = MFX_EXTBUFF_JPEG_HUFFMAN;
        m_checkedJpegHT.Header.BufferSz = sizeof(m_checkedJpegHT);
    }
    if (opaqAllocReq)
    {
        m_checkedOpaqAllocReq = *opaqAllocReq;
        m_pCheckedExt[ext_counter++] = &m_checkedOpaqAllocReq.Header;
    }
    checked.ExtParam = m_pCheckedExt;
    checked.NumExtParam = ext_counter;

    sts = Query(m_pCore, par, &checked);

    if ((sts != MFX_ERR_NONE) &&
        (sts != MFX_WRN_INCOMPATIBLE_VIDEO_PARAM))
    {
        if (sts == MFX_ERR_UNSUPPORTED)
            return MFX_ERR_INVALID_VIDEO_PARAM;
        else
            return sts;
    }

    QueryStatus = sts;

    par = &checked; // from now work with fixed copy of input!

    if (opaqAllocReq)
        opaqAllocReq = &m_checkedOpaqAllocReq;

    if (!m_pCore->IsExternalFrameAllocator() && (par->IOPattern & (MFX_IOPATTERN_OUT_VIDEO_MEMORY | MFX_IOPATTERN_IN_VIDEO_MEMORY)))
        return MFX_ERR_INVALID_VIDEO_PARAM;

    m_ddi.reset( CreatePlatformMJpegEncoder( m_pCore ) );
    if (m_ddi.get() == 0)
        return MFX_WRN_PARTIAL_ACCELERATION;

    MFX_INTERNAL_CPY(&m_vFirstParam, par, sizeof(mfxVideoParam));
    MFX_INTERNAL_CPY(&m_vParam, &m_vFirstParam, sizeof(mfxVideoParam));

    sts = m_ddi->CreateAuxilliaryDevice(
        m_pCore,
        m_vParam.mfx.FrameInfo.Width,
        m_vParam.mfx.FrameInfo.Height);
    if (sts != MFX_ERR_NONE)
        return MFX_WRN_PARTIAL_ACCELERATION;

    sts = m_ddi->CreateAccelerationService(m_vParam);
    if (sts != MFX_ERR_NONE)
        return MFX_WRN_PARTIAL_ACCELERATION;

    mfxFrameAllocRequest request = { };
    request.Info = m_vParam.mfx.FrameInfo;

    // for JPEG encoding, one raw buffer is enough. but regarding to
    // motion JPEG video, we'd better use multiple buffers to support async mode.
    mfxU16 surface_num = JPEG_VIDEO_SURFACE_NUM + m_vParam.AsyncDepth;

    // WA for RGB swapping issue
    if (m_vParam.mfx.FrameInfo.FourCC == MFX_FOURCC_RGB4)
    {
        //currently MDF do not support BGR, doesn't matter what format we set for internal frame, result will be the same
        request.Info.FourCC = (m_pCore->GetVAType() == MFX_HW_VAAPI)?MFX_FOURCC_BGR4:MFX_FOURCC_RGB4;
        request.Type = MFX_MEMTYPE_VIDEO_INT;
#if defined(LINUX)
        request.Type |= MFX_MEMTYPE_VIDEO_MEMORY_ENCODER_TARGET; // required for libva especially for RGB32
#endif
        request.NumFrameMin = surface_num;
        request.NumFrameSuggested = request.NumFrameMin;

        sts = m_pCore->AllocFrames(&request, &m_raw, true);
        MFX_CHECK(
            sts == MFX_ERR_NONE &&
            m_raw.NumFrameActual >= request.NumFrameMin,
            MFX_ERR_MEMORY_ALLOC);
        if (MFX_IOPATTERN_IN_OPAQUE_MEMORY == m_vParam.IOPattern)
        {
            if( NULL == opaqAllocReq )
                return MFX_ERR_INVALID_VIDEO_PARAM;
            else
            {
                m_isOpaqIn = true;
            }
        }
    }
    else if (m_vParam.IOPattern == MFX_IOPATTERN_IN_SYSTEM_MEMORY)
    {
    // Allocate raw surfaces.
    // This is required only in case of system memory at input

        request.Type = MFX_MEMTYPE_VIDEO_INT;
#if defined(LINUX)
        request.Type |= MFX_MEMTYPE_VIDEO_MEMORY_ENCODER_TARGET; // required for libva especially for RGB32
#endif
        request.NumFrameMin = surface_num;
        request.NumFrameSuggested = request.NumFrameMin;

        sts = m_pCore->AllocFrames(&request, &m_raw, true);
        MFX_CHECK(
            sts == MFX_ERR_NONE &&
            m_raw.NumFrameActual >= request.NumFrameMin,
            MFX_ERR_MEMORY_ALLOC);
    }
    else if (MFX_IOPATTERN_IN_OPAQUE_MEMORY == m_vParam.IOPattern)
    {
        if( NULL == opaqAllocReq )
            return MFX_ERR_INVALID_VIDEO_PARAM;
        else
        {
            m_isOpaqIn = true;
        }
    }

    // Allocate bitstream surfaces.
    request.Type = MFX_MEMTYPE_VIDEO_INT;
#if defined(LINUX)
    request.Type |= MFX_MEMTYPE_VIDEO_MEMORY_ENCODER_TARGET; // required for libva especially for RGB32
#endif
    request.NumFrameMin = surface_num;
    request.NumFrameSuggested = request.NumFrameMin;

    // driver may suggest too small buffer for bitstream
    sts = m_ddi->QueryBitstreamBufferInfo(request);
    MFX_CHECK_STS(sts);

    mfxU16 doubleBytesPerPx = 0;
    switch(m_vParam.mfx.FrameInfo.FourCC)
    {
        case MFX_FOURCC_NV12:
        case MFX_FOURCC_YV12:
            doubleBytesPerPx = 3;
            break;
        case MFX_FOURCC_YUY2:
            doubleBytesPerPx = 4;
            break;
        case MFX_FOURCC_RGB4:
        default:
            doubleBytesPerPx = 8;
            break;
    }
    request.Info.Width  = std::max        (request.Info.Width,  m_vParam.mfx.FrameInfo.Width);
    request.Info.Height = std::max<mfxU16>(request.Info.Height, m_vParam.mfx.FrameInfo.Height * doubleBytesPerPx / 2);

    sts = m_bitstream.Alloc(m_pCore, request);
    MFX_CHECK(
        sts == MFX_ERR_NONE &&
        m_bitstream.NumFrameActual >= request.NumFrameMin,
        MFX_ERR_MEMORY_ALLOC);

    sts = m_ddi->RegisterBitstreamBuffer(m_bitstream);
    MFX_CHECK_STS(sts);

    sts = m_TaskManager.Init(surface_num);
    MFX_CHECK_STS(sts);

    m_bInitialized = true;
    return (QueryStatus == MFX_ERR_NONE) ? sts : QueryStatus;
}

mfxStatus MFXVideoENCODEMJPEG_HW::Reset(mfxVideoParam *par)
{
    mfxStatus sts;

    if(!m_bInitialized)
        return MFX_ERR_NOT_INITIALIZED;

    if(par == NULL)
        return MFX_ERR_NULL_PTR;

    sts = CheckExtBufferId(*par);
    MFX_CHECK_STS(sts);

    mfxExtJPEGQuantTables*    jpegQT       = (mfxExtJPEGQuantTables*)   GetExtBuffer( par->ExtParam, par->NumExtParam, MFX_EXTBUFF_JPEG_QT );
    mfxExtJPEGHuffmanTables*  jpegHT       = (mfxExtJPEGHuffmanTables*) GetExtBuffer( par->ExtParam, par->NumExtParam, MFX_EXTBUFF_JPEG_HUFFMAN );
    mfxExtOpaqueSurfaceAlloc* opaqAllocReq = (mfxExtOpaqueSurfaceAlloc*)GetExtBuffer( par->ExtParam, par->NumExtParam, MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION );

    mfxVideoParam checked;
    mfxU16 ext_counter = 0;
    checked = *par;

    if (jpegQT)
    {
        m_checkedJpegQT = *jpegQT;
        m_pCheckedExt[ext_counter++] = &m_checkedJpegQT.Header;
    }
    else
    {
        memset(&m_checkedJpegQT, 0, sizeof(m_checkedJpegQT));
        m_checkedJpegQT.Header.BufferId = MFX_EXTBUFF_JPEG_QT;
        m_checkedJpegQT.Header.BufferSz = sizeof(m_checkedJpegQT);
    }
    if (jpegHT)
    {
        m_checkedJpegHT = *jpegHT;
        m_pCheckedExt[ext_counter++] = &m_checkedJpegHT.Header;
    }
    else
    {
        memset(&m_checkedJpegHT, 0, sizeof(m_checkedJpegHT));
        m_checkedJpegHT.Header.BufferId = MFX_EXTBUFF_JPEG_HUFFMAN;
        m_checkedJpegHT.Header.BufferSz = sizeof(m_checkedJpegHT);
    }
    if (opaqAllocReq)
    {
        m_checkedOpaqAllocReq = *opaqAllocReq;
        m_pCheckedExt[ext_counter++] = &m_checkedOpaqAllocReq.Header;
    }
    checked.ExtParam = m_pCheckedExt;
    checked.NumExtParam = ext_counter;

    sts = Query(m_pCore, par, &checked);

    if (sts != MFX_ERR_NONE && sts != MFX_WRN_INCOMPATIBLE_VIDEO_PARAM && sts != MFX_WRN_PARTIAL_ACCELERATION)
    {
        if (sts == MFX_ERR_UNSUPPORTED)
            return MFX_ERR_INVALID_VIDEO_PARAM;
        else
            return sts;
    }

    par = &checked; // from now work with fixed copy of input!

    // check for valid IOPattern
    mfxU16 IOPatternIn = par->IOPattern & (MFX_IOPATTERN_IN_VIDEO_MEMORY | MFX_IOPATTERN_IN_SYSTEM_MEMORY | MFX_IOPATTERN_IN_OPAQUE_MEMORY);
    if (!par->IOPattern ||
        ((IOPatternIn != MFX_IOPATTERN_IN_VIDEO_MEMORY) && (IOPatternIn != MFX_IOPATTERN_IN_SYSTEM_MEMORY) && (IOPatternIn != MFX_IOPATTERN_IN_OPAQUE_MEMORY)))
    {
        return MFX_ERR_INVALID_VIDEO_PARAM;
    }

    if (!m_pCore->IsExternalFrameAllocator() && (par->IOPattern & (MFX_IOPATTERN_OUT_VIDEO_MEMORY | MFX_IOPATTERN_IN_VIDEO_MEMORY)))
        return MFX_ERR_INVALID_VIDEO_PARAM;

    // checks for opaque memory
    if (!(m_vFirstParam.IOPattern & MFX_IOPATTERN_IN_OPAQUE_MEMORY) && (par->IOPattern & MFX_IOPATTERN_IN_OPAQUE_MEMORY))
        return MFX_ERR_INCOMPATIBLE_VIDEO_PARAM;

    if(par->mfx.FrameInfo.PicStruct != MFX_PICSTRUCT_UNKNOWN &&
       par->mfx.FrameInfo.PicStruct != MFX_PICSTRUCT_PROGRESSIVE &&
       par->mfx.FrameInfo.PicStruct != MFX_PICSTRUCT_FIELD_TFF &&
       par->mfx.FrameInfo.PicStruct != MFX_PICSTRUCT_FIELD_BFF)
    {
        return MFX_ERR_INVALID_VIDEO_PARAM;
    }

    // check that new params don't require allocation of additional memory
    if (par->mfx.FrameInfo.Width > m_vFirstParam.mfx.FrameInfo.Width ||
        par->mfx.FrameInfo.Height > m_vFirstParam.mfx.FrameInfo.Height ||
        m_vFirstParam.mfx.FrameInfo.FourCC != par->mfx.FrameInfo.FourCC ||
        m_vFirstParam.mfx.FrameInfo.ChromaFormat != par->mfx.FrameInfo.ChromaFormat)
        return MFX_ERR_INCOMPATIBLE_VIDEO_PARAM;

    sts = m_TaskManager.Reset();
    MFX_CHECK_STS(sts);

    m_vParam.mfx = par->mfx;

    m_vParam.IOPattern = par->IOPattern;
    m_vParam.Protected = 0;

    if(par->AsyncDepth != m_vFirstParam.AsyncDepth)
        return MFX_ERR_INVALID_VIDEO_PARAM;

    m_counter = 1;

    return sts;
}


mfxStatus MFXVideoENCODEMJPEG_HW::Close(void)
{
    mfxStatus sts = m_TaskManager.Close();
    MFX_CHECK_STS(sts);

    m_bitstream.Free();

    if (m_raw.NumFrameActual != 0)
    {
        m_pCore->FreeFrames(&m_raw);
        memset(&m_raw, 0, sizeof(m_raw));
    }

    return MFX_ERR_NONE;
}

mfxStatus MFXVideoENCODEMJPEG_HW::GetVideoParam(mfxVideoParam *par)
{
    if (!m_bInitialized)
        return MFX_ERR_NOT_INITIALIZED;

    MFX_CHECK_NULL_PTR1(par);

    par->mfx = m_vParam.mfx;
    par->Protected = m_vParam.Protected;
    par->IOPattern = m_vParam.IOPattern;
    par->AsyncDepth = m_vParam.AsyncDepth;

    return MFX_ERR_NONE;
}

mfxStatus MFXVideoENCODEMJPEG_HW::GetFrameParam(mfxFrameParam *par)
{
    MFX_CHECK_NULL_PTR1(par);
    return MFX_ERR_UNSUPPORTED;
}

mfxStatus MFXVideoENCODEMJPEG_HW::GetEncodeStat(mfxEncodeStat *stat)
{
    if (!m_bInitialized)
        return MFX_ERR_NOT_INITIALIZED;

    MFX_CHECK_NULL_PTR1(stat)
    memset(stat, 0, sizeof(mfxEncodeStat));
    /*stat->NumCachedFrame = 0;
    stat->NumBit = m_totalBits;
    stat->NumFrame = m_encodedFrames;*/

    return MFX_ERR_UNSUPPORTED;
}

// main function to run encoding process, syncronous
mfxStatus MFXVideoENCODEMJPEG_HW::EncodeFrameCheck(
                               mfxEncodeCtrl *ctrl,
                               mfxFrameSurface1 *inSurface,
                               mfxBitstream *bs,
                               mfxFrameSurface1 ** /*reordered_surface*/,
                               mfxEncodeInternalParams * /*pInternalParams*/,
                               MFX_ENTRY_POINT pEntryPoints[],
                               mfxU32 &numEntryPoints)
{
    mfxExtJPEGQuantTables*   jpegQT = NULL;
    mfxExtJPEGHuffmanTables* jpegHT = NULL;
    JpegEncCaps              hwCaps = {};

    mfxFrameSurface1 *surface = GetOriginalSurface(inSurface);
    // input surface is opaque surface
    if (surface != inSurface)
    {
        if (surface == 0)
            return MFX_ERR_UNDEFINED_BEHAVIOR;

        surface->Info = inSurface->Info;
        surface->Data.Corrupted = inSurface->Data.Corrupted;
        surface->Data.DataFlag = inSurface->Data.DataFlag;
        surface->Data.TimeStamp = inSurface->Data.TimeStamp;
        surface->Data.FrameOrder = inSurface->Data.FrameOrder;
    }

    if (inSurface && !surface)
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    mfxStatus checkSts = CheckEncodeFrameParam(
        surface,
        bs,
        m_pCore->IsExternalFrameAllocator());
    MFX_CHECK(checkSts >= MFX_ERR_NONE, checkSts);

    mfxStatus status = checkSts;

    if (ctrl && ctrl->ExtParam && ctrl->NumExtParam > 0)
    {
        jpegQT = (mfxExtJPEGQuantTables*)   GetExtBuffer( ctrl->ExtParam, ctrl->NumExtParam, MFX_EXTBUFF_JPEG_QT );
        jpegHT = (mfxExtJPEGHuffmanTables*) GetExtBuffer( ctrl->ExtParam, ctrl->NumExtParam, MFX_EXTBUFF_JPEG_HUFFMAN );
    }

    // Check new tables if exists
    if (jpegQT || jpegHT)
    {
        mfxVideoParam vPar = m_vParam;
        vPar.ExtParam = ctrl->ExtParam;
        vPar.NumExtParam = ctrl->NumExtParam;

        mfxStatus mfxRes = m_ddi->QueryEncodeCaps(hwCaps);
        MFX_CHECK_STS(mfxRes);

        mfxRes = CheckJpegParam(m_pCore, vPar, hwCaps);
        if (mfxRes != MFX_ERR_NONE)
            return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    mfxExtJPEGQuantTables* jpegQTInitial = (mfxExtJPEGQuantTables*) GetExtBuffer( m_vParam.ExtParam, m_vParam.NumExtParam, MFX_EXTBUFF_JPEG_QT );
    if (!(jpegQTInitial || jpegQT || m_vParam.mfx.Quality))
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    DdiTask * task = 0;
    checkSts = m_TaskManager.AssignTask(task);
    MFX_CHECK_STS(checkSts);

    if (!task->m_pDdiData || task->m_cleanDdiData || jpegQT || jpegHT)
    {
        task->m_cleanDdiData = jpegQT || jpegHT;

        if (task->m_pDdiData)
        {
            task->m_pDdiData->Close();
            delete task->m_pDdiData;
            task->m_pDdiData = NULL;
        }

        mfxStatus mfxRes = m_ddi->QueryEncodeCaps(hwCaps);
        MFX_CHECK_STS(mfxRes);

        task->m_pDdiData = new MfxHwMJpegEncode::ExecuteBuffers;
        checkSts = task->m_pDdiData->Init(&m_vParam, ctrl, &hwCaps);
        MFX_CHECK_STS(checkSts);
    }

    bs->TimeStamp = surface->Data.TimeStamp;
    bs->DecodeTimeStamp = surface->Data.TimeStamp;
    bs->FrameType = MFX_FRAMETYPE_I;

    m_pCore->IncreaseReference(&surface->Data);
    task->surface = surface;
    task->bs      = bs;
    task->m_statusReportNumber = m_counter++;

    // definition tasks for MSDK scheduler
    pEntryPoints[0].pState               = this;
    pEntryPoints[0].pParam               = task;
    // callback to run after complete task / depricated
    pEntryPoints[0].pCompleteProc        = 0;
    // callback to run after complete sub-task (for SW implementation makes sense) / (NON-OBLIGATORY)
    pEntryPoints[0].pGetSubTaskProc      = 0;
    pEntryPoints[0].pCompleteSubTaskProc = 0;

    pEntryPoints[0].requiredNumThreads   = 1;
    pEntryPoints[1] = pEntryPoints[0];
    pEntryPoints[0].pRoutineName = (char *)"Encode Submit";
    pEntryPoints[1].pRoutineName = (char *)"Encode Query";

    pEntryPoints[0].pRoutine = TaskRoutineSubmitFrame;
    pEntryPoints[1].pRoutine = TaskRoutineQueryFrame;

    numEntryPoints = 2;

    return status;
}


mfxStatus MFXVideoENCODEMJPEG_HW::CheckEncodeFrameParam(
    mfxFrameSurface1    * surface,
    mfxBitstream        * bs,
    bool                  isExternalFrameAllocator)
{
    mfxStatus sts = MFX_ERR_NONE;

    if (!m_bInitialized) return MFX_ERR_NOT_INITIALIZED;

    MFX_CHECK_NULL_PTR1(bs);
    MFX_CHECK_NULL_PTR1(bs->Data);

    // Check for enough bitstream buffer size
    if ( (0 == bs->MaxLength) || (bs->MaxLength <= (bs->DataOffset + bs->DataLength)) )
        return MFX_ERR_NOT_ENOUGH_BUFFER;

    if ( NULL != surface )
    {
        if (surface->Info.ChromaFormat != m_vParam.mfx.FrameInfo.ChromaFormat)
            return MFX_ERR_INVALID_VIDEO_PARAM;

        if (m_vParam.IOPattern == MFX_IOPATTERN_IN_SYSTEM_MEMORY)
        {
            mfxU32 pitch = surface->Data.PitchLow + ((mfxU32)surface->Data.PitchHigh << 16);
            MFX_CHECK((surface->Data.Y == 0) == (surface->Data.UV == 0), MFX_ERR_UNDEFINED_BEHAVIOR);
            MFX_CHECK((surface->Data.Y == 0) || (pitch != 0), MFX_ERR_UNDEFINED_BEHAVIOR);
            MFX_CHECK(surface->Data.Y != 0 || isExternalFrameAllocator, MFX_ERR_UNDEFINED_BEHAVIOR);
        }

        MFX_CHECK(surface->Info.Width >= m_vParam.mfx.FrameInfo.Width, MFX_ERR_INVALID_VIDEO_PARAM);
        MFX_CHECK(surface->Info.Height >= m_vParam.mfx.FrameInfo.Height, MFX_ERR_INVALID_VIDEO_PARAM);
    }
    else
    {
        return MFX_ERR_MORE_DATA;
    }

    return sts;
}

// Routine to submit task to HW.  asyncronous part of encdoing
mfxStatus MFXVideoENCODEMJPEG_HW::TaskRoutineSubmitFrame(
    void * state,
    void * param,
    mfxU32 /*threadNumber*/,
    mfxU32 /*callNumber*/)
{
    MFXVideoENCODEMJPEG_HW & enc = *(MFXVideoENCODEMJPEG_HW*)state;
    DdiTask &task = *(DdiTask*)param;

    mfxStatus sts = enc.CheckDevice();
    MFX_CHECK_STS(sts);

    mfxHDLPair surfacePair = { };
    mfxHDL     surfaceHDL = 0;

    mfxHDL *pSurfaceHdl;

    if (MFX_HW_D3D11 == enc.m_pCore->GetVAType())
        pSurfaceHdl = (mfxHDL *)&surfacePair;
    else if (MFX_HW_D3D9 == enc.m_pCore->GetVAType())
        pSurfaceHdl = (mfxHDL *)&surfaceHDL;
    else if (MFX_HW_VAAPI == enc.m_pCore->GetVAType())
        pSurfaceHdl = (mfxHDL *)&surfaceHDL;
    else
        return MFX_ERR_UNDEFINED_BEHAVIOR;

    mfxFrameSurface1 * nativeSurf = task.surface;

    // WA for RGB swapping issue
    if (enc.m_vParam.mfx.FrameInfo.FourCC == MFX_FOURCC_RGB4)
    {
        mfxFrameData dstSurf = {};
        bool bExternalFrameLocked = false;

        if(enc.m_pCore->GetVAType() == MFX_HW_VAAPI )
        {
            enc.m_pCore->LockFrame(enc.m_raw.mids[task.m_idx], &dstSurf);
            MFX_CHECK(dstSurf.R != 0, MFX_ERR_LOCK_MEMORY);
            if (nativeSurf->Data.B == 0)
            {
                enc.m_pCore->LockExternalFrame(nativeSurf->Data.MemId, &nativeSurf->Data);
                bExternalFrameLocked = true;
            }
            MFX_CHECK(nativeSurf->Data.B != 0, MFX_ERR_LOCK_MEMORY);


            const int dstOrder[3] = {2, 1, 0};
            mfxSize roi = {nativeSurf->Info.Width, nativeSurf->Info.Height};
            mfxSize setroi = {nativeSurf->Info.Width*4, nativeSurf->Info.Height};
            if (0 == roi.width || 0 == roi.height)
                return MFX_ERR_UNDEFINED_BEHAVIOR;

            mfxU32 srsPitch = nativeSurf->Data.PitchLow + ((mfxU32)nativeSurf->Data.PitchHigh << 16);
            mfxU32 dstPitch = dstSurf.PitchLow + ((mfxU32)dstSurf.PitchHigh << 16);
            bool res = setPlaneROI<mfxU8>(0xff, dstSurf.R, dstPitch, setroi);
            MFX_CHECK(res, MFX_ERR_UNDEFINED_BEHAVIOR);
            res = swapChannels<mfxU8>(nativeSurf->Data.B, srsPitch,
                                    dstSurf.R, dstPitch,
                                    roi, dstOrder);
            MFX_CHECK(res, MFX_ERR_UNDEFINED_BEHAVIOR);

            if (bExternalFrameLocked)
            {
                sts = enc.m_pCore->UnlockExternalFrame(nativeSurf->Data.MemId, &nativeSurf->Data);
                MFX_CHECK_STS(sts);
            }
            sts = enc.m_pCore->UnlockFrame(enc.m_raw.mids[task.m_idx], &dstSurf);
            MFX_CHECK_STS(sts);
        }
        else
        {
            mfxFrameSurface1 dst;
            mfxU16 src_memtype;
            dstSurf.MemId = enc.m_raw.mids[task.m_idx];
            dst.Info = nativeSurf->Info;
            dst.Info.FourCC = MFX_FOURCC_BGR4;
            dst.Data = dstSurf;
            src_memtype = (mfxU16)((nativeSurf->Data.B == 0)?MFX_MEMTYPE_DXVA2_DECODER_TARGET:MFX_MEMTYPE_SYSTEM_MEMORY);
            src_memtype |= enc.m_isOpaqIn? MFX_MEMTYPE_INTERNAL_FRAME: MFX_MEMTYPE_EXTERNAL_FRAME;
            sts = enc.m_pCore->DoFastCopyWrapper(&dst,MFX_MEMTYPE_DXVA2_DECODER_TARGET|MFX_MEMTYPE_INTERNAL_FRAME,nativeSurf,
                src_memtype);
            MFX_CHECK_STS(sts);
        }
        sts = enc.m_pCore->GetFrameHDL(enc.m_raw.mids[task.m_idx], pSurfaceHdl);
    }
    else if (enc.m_vParam.IOPattern == MFX_IOPATTERN_IN_SYSTEM_MEMORY)
    {
        sts = FastCopyFrameBufferSys2Vid(
            enc.m_pCore,
            enc.m_raw.mids[task.m_idx],
            nativeSurf->Data,
            enc.m_vParam.mfx.FrameInfo);
        MFX_CHECK_STS(sts);

        sts = enc.m_pCore->GetFrameHDL(enc.m_raw.mids[task.m_idx], pSurfaceHdl);
    }
    else if (MFX_IOPATTERN_IN_OPAQUE_MEMORY == enc.m_vParam.IOPattern)
    {
        sts = enc.m_pCore->GetFrameHDL(nativeSurf->Data.MemId, pSurfaceHdl);
    }
    else
    {
        if (MFX_IOPATTERN_IN_VIDEO_MEMORY == enc.m_vParam.IOPattern)
            sts = enc.m_pCore->GetExternalFrameHDL(nativeSurf->Data.MemId, pSurfaceHdl);
        else
            return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
    MFX_CHECK_STS(sts);

    if (MFX_HW_D3D11 == enc.m_pCore->GetVAType())
    {
        MFX_CHECK(surfacePair.first != 0, MFX_ERR_UNDEFINED_BEHAVIOR);
        sts = enc.m_ddi->Execute(task, (mfxHDL)pSurfaceHdl);
    }
    else if (MFX_HW_D3D9 == enc.m_pCore->GetVAType())
    {
        MFX_CHECK(surfaceHDL != 0, MFX_ERR_UNDEFINED_BEHAVIOR);
        sts = enc.m_ddi->Execute(task, surfaceHDL);
    }
    else if (MFX_HW_VAAPI == enc.m_pCore->GetVAType())
    {
        MFX_CHECK(surfaceHDL != 0, MFX_ERR_UNDEFINED_BEHAVIOR);
        sts = enc.m_ddi->Execute(task, (mfxHDL)surfaceHDL);
    }

    return sts;
}

// Routine to query encdoing status from HW.  asyncronous part of encdoing
mfxStatus MFXVideoENCODEMJPEG_HW::TaskRoutineQueryFrame(
    void * state,
    void * param,
    mfxU32 /*threadNumber*/,
    mfxU32 /*callNumber*/)
{
    mfxStatus sts;
    MFXVideoENCODEMJPEG_HW & enc = *(MFXVideoENCODEMJPEG_HW*)state;
    DdiTask &task = *(DdiTask*)param;

    sts = enc.m_ddi->QueryStatus(task);
    MFX_CHECK_STS(sts);

    sts = enc.m_ddi->UpdateBitstream(enc.m_bitstream.mids[task.m_idx], task);
    MFX_CHECK_STS(sts);

    enc.m_pCore->DecreaseReference(&(task.surface->Data));

    return enc.m_TaskManager.RemoveTask(task);
}

mfxStatus MFXVideoENCODEMJPEG_HW::UpdateDeviceStatus(mfxStatus sts)
{
    if (sts == MFX_ERR_DEVICE_FAILED)
        m_deviceFailed = true;
    return sts;
}

mfxStatus MFXVideoENCODEMJPEG_HW::CheckDevice()
{
    return m_deviceFailed
        ? MFX_ERR_DEVICE_FAILED
        : MFX_ERR_NONE;
}

#endif // #if defined (MFX_ENABLE_MJPEG_VIDEO_ENCODE) && defined (MFX_VA)
