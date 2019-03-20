// Copyright (c) 2018-2019 Intel Corporation
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

#include "umc_defs.h"
#if defined (MFX_ENABLE_MJPEG_VIDEO_DECODE) && defined(MFX_ENABLE_SW_FALLBACK)
#include <string.h>
#include <assert.h>
#include "umc_video_data.h"
#include "umc_mjpeg_mfx_decode.h"
#include "membuffin.h"
#include "jpegdec.h"

#include <mfx_mjpeg_task.h>

namespace UMC
{

MJPEGVideoDecoderMFX::MJPEGVideoDecoderMFX(void)
{
    m_IsInit      = false;
    //m_firstFrame  = false;
    //m_firstField  = false;
    m_interleaved = false;
    m_interleavedScan = false;
    //m_needCloseFrame = false;
    m_needPostProcessing = false;

    m_color = JC_UNKNOWN;

    m_rotation    = 0;
    m_frameNo     = 0;
    m_frame       = nullptr;
    m_frameSampling = 0;

    m_frameAllocator = 0;

    std::fill(std::begin(m_pLastPicBuffer), std::end(m_pLastPicBuffer), nullptr);

    m_framePrecision = 0;
    m_frameChannels = 0;
    m_local_frame_time = 0;
    m_local_delta_frame_time = 0;
} // ctor

MJPEGVideoDecoderMFX::~MJPEGVideoDecoderMFX(void)
{
    Close();
} // dtor


Status MJPEGVideoDecoderMFX::Init(BaseCodecParams* lpInit)
{
    Status status;
    uint32_t i, numThreads;

    VideoDecoderParams* pDecoderParams;

    pDecoderParams = DynamicCast<VideoDecoderParams>(lpInit);

    if(0 == pDecoderParams)
        return UMC_ERR_NULL_PTR;

    status = Close();
    if(UMC_OK != status)
        return UMC_ERR_INIT;

    m_DecoderParams  = *pDecoderParams;

    m_IsInit       = true;
    //m_firstFrame   = true;
    m_interleaved  = false;
    m_interleavedScan = false;
    m_frameNo      = 0;
    m_frame        = 0;
    m_frameSampling = 0;
    m_needPostProcessing = false;
    m_rotation     = 0;

    // allocate the JPEG decoders
    numThreads = JPEG_MAX_THREADS;
    if ((m_DecoderParams.numThreads) &&
        (numThreads > (uint32_t) m_DecoderParams.numThreads))
    {
        numThreads = m_DecoderParams.numThreads;
    }
    m_dec.resize(numThreads);
    for (i = 0; i < numThreads; i += 1)
    {
        m_dec[i].reset(new CJPEGDecoder());
    }

    m_decBase = m_dec[0].get();

    m_local_delta_frame_time = 1.0/30;
    m_local_frame_time       = 0;

    if (pDecoderParams->info.framerate)
    {
        m_local_delta_frame_time = 1 / pDecoderParams->info.framerate;
    }

    return UMC_OK;
} // MJPEGVideoDecoderMFX::Init()


Status MJPEGVideoDecoderMFX::Reset(void)
{
    m_IsInit       = true;
    //m_firstFrame   = true;
    m_frameNo      = 0;
    m_interleaved  = false;
    m_interleavedScan = false;
    m_needPostProcessing = false;
    //m_needCloseFrame = false;
    m_frame        = 0;
    m_frameSampling = 0;
    m_rotation     = 0;

    m_frameData.Close();
    m_internalFrame.Close();
    for (auto& dec: m_dec)
    {
        dec->Reset();
    }
    m_local_frame_time = 0;

    std::fill(std::begin(m_pLastPicBuffer), std::end(m_pLastPicBuffer), nullptr);

    return UMC_OK;

} // MJPEGVideoDecoderMFX::Reset()


Status MJPEGVideoDecoderMFX::Close(void)
{
    m_IsInit       = false;
    //m_firstFrame   = false;
    //m_firstField   = false;
    m_interleaved  = false;
    m_interleavedScan = false;
    m_needPostProcessing = false;
    //m_needCloseFrame = false;
    m_frameNo      = 0;
    m_frame        = 0;
    m_frameSampling = 0;
    m_rotation     = 0;

    m_local_frame_time = 0;

    m_frameData.Close();
    m_internalFrame.Close();
    for (auto& dec: m_dec)
    {
        dec.reset(nullptr);
    }
    m_PostProcessing.reset(nullptr);

    return UMC_OK;
} // MJPEGVideoDecoderMFX::Close()

void MJPEGVideoDecoderMFX::AdjustFrameSize(mfxSize & size)
{
    size.width  = mfx::align2_value(size.width, 16);
    size.height = mfx::align2_value(size.height, m_interleaved ? 32 : 16);
}

ChromaType MJPEGVideoDecoderMFX::GetChromaType()
{
/*
 0:YUV400 (grayscale image) Y: h=1 v=1,
 1:YUV420           Y: h=2 v=2, Cb/Cr: h=1 v=1
 2:YUV422H_2Y       Y: h=2 v=1, Cb/Cr: h=1 v=1
 3:YUV444           Y: h=1 v=1, Cb/Cr: h=1 v=1
 4:YUV411           Y: h=4 v=1, Cb/Cr: h=1 v=1
 5:YUV422V_2Y       Y: h=1 v=2, Cb/Cr: h=1 v=1
 6:YUV422H_4Y       Y: h=2 v=2, Cb/Cr: h=1 v=2
 7:YUV422V_4Y       Y: h=2 v=2, Cb/Cr: h=2 v=1
*/

    if (m_dec[0]->m_jpeg_ncomp == 1)
    {
        return CHROMA_TYPE_YUV400;
    }

    ChromaType type = CHROMA_TYPE_YUV400;
    switch(m_dec[0]->m_ccomp[0].m_hsampling)
    {
    case 1: // YUV422V_2Y, YUV444
        if (m_dec[0]->m_ccomp[0].m_vsampling == 1) // YUV444
        {
            if (m_dec[0]->m_jpeg_color == JC_YCBCR)
                type = CHROMA_TYPE_YUV444;
            else if (m_dec[0]->m_jpeg_color == JC_RGB)
                type = CHROMA_TYPE_RGB;
            else
                assert(false);
        }
        else
        {
            assert((m_dec[0]->m_ccomp[0].m_vsampling == 2) && (m_dec[0]->m_ccomp[1].m_hsampling == 1));
            type = CHROMA_TYPE_YUV422V_2Y; // YUV422V_2Y
        }
        break;
    case 2: // YUV420, YUV422H_2Y, YUV422H_4Y, YUV422V_4Y
        if (m_dec[0]->m_ccomp[0].m_vsampling == 1)
        {
            assert(m_dec[0]->m_ccomp[1].m_vsampling == 1 && m_dec[0]->m_ccomp[1].m_hsampling == 1);
            type = CHROMA_TYPE_YUV422H_2Y; // YUV422H_2Y
        }
        else
        {
            assert(m_dec[0]->m_ccomp[0].m_vsampling == 2);

            if (m_dec[0]->m_ccomp[1].m_hsampling == 1 && m_dec[0]->m_ccomp[1].m_vsampling == 1)
                type = CHROMA_TYPE_YUV420; // YUV420;
            else
                type = (m_dec[0]->m_ccomp[1].m_hsampling == 1) ? CHROMA_TYPE_YUV422H_4Y : CHROMA_TYPE_YUV422V_4Y; // YUV422H_4Y, YUV422V_4Y
        }
        break;
    case 4: // YUV411
        assert(m_dec[0]->m_ccomp[0].m_vsampling == 1);
        type = CHROMA_TYPE_YUV411;
        break;
    default:
        assert(false);
        break;
    }

    return type;
}

JCOLOR MJPEGVideoDecoderMFX::GetColorType()
{
    JCOLOR color = JC_UNKNOWN;
    switch(m_dec[0]->m_jpeg_color)
    {
    case JC_UNKNOWN:
        color = JC_UNKNOWN;
        break;
    case JC_GRAY:
        color = JC_GRAY;
        break;
    case JC_YCBCR:
        color = JC_YCBCR;
        break;
    case JC_RGB:
        color = JC_RGB;
        break;
    default:
        assert(false);
        break;
    }

    return color;
}

Status MJPEGVideoDecoderMFX::FillVideoParam(mfxVideoParam *par, bool /*full*/)
{
    memset(par, 0, sizeof(mfxVideoParam));

    par->mfx.CodecId = MFX_CODEC_JPEG;

    par->mfx.FrameInfo.FourCC = MFX_FOURCC_NV12;

    mfxSize size = m_frameDims;
    AdjustFrameSize(size);

    par->mfx.FrameInfo.Width = mfxU16(size.width);
    par->mfx.FrameInfo.Height = mfxU16(size.height);

    par->mfx.FrameInfo.CropX = 0;
    par->mfx.FrameInfo.CropY = 0;
    par->mfx.FrameInfo.CropH = (mfxU16) (m_frameDims.height);
    par->mfx.FrameInfo.CropW = (mfxU16) (m_frameDims.width);

    par->mfx.FrameInfo.CropW = mfx::align2_value(par->mfx.FrameInfo.CropW, 2);
    par->mfx.FrameInfo.CropH = mfx::align2_value(par->mfx.FrameInfo.CropH, 2);

    par->mfx.FrameInfo.PicStruct = (mfxU8)(m_interleaved ? MFX_PICSTRUCT_FIELD_TFF : MFX_PICSTRUCT_PROGRESSIVE);
    par->mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;

    par->mfx.FrameInfo.AspectRatioW = 0;
    par->mfx.FrameInfo.AspectRatioH = 0;

    par->mfx.FrameInfo.FrameRateExtD = 1;
    par->mfx.FrameInfo.FrameRateExtN = 30;

    par->mfx.CodecProfile = 1;
    par->mfx.CodecLevel = 1;

    par->mfx.JPEGChromaFormat = GetMFXChromaFormat(GetChromaType());
    par->mfx.JPEGColorFormat = GetMFXColorFormat(GetColorType());
    par->mfx.Rotation  = MFX_ROTATION_0;
    par->mfx.InterleavedDec = (mfxU16)(m_interleavedScan ? MFX_SCANTYPE_INTERLEAVED : MFX_SCANTYPE_NONINTERLEAVED);

    par->mfx.NumThread = 0;

    return UMC_OK;
}

Status MJPEGVideoDecoderMFX::FillQuantTableExtBuf(mfxExtJPEGQuantTables* quantTables)
{
    if (!m_IsInit)
        return UMC_ERR_NOT_INITIALIZED;

    quantTables->NumTable = m_dec[0]->GetNumQuantTables();
    for(int i=0; i<quantTables->NumTable; i++)
    {
        m_dec[0]->FillQuantTable(i, quantTables->Qm[i]);
    }

    return UMC_OK;
}

Status MJPEGVideoDecoderMFX::FillHuffmanTableExtBuf(mfxExtJPEGHuffmanTables* huffmanTables)
{
    if (!m_IsInit)
        return UMC_ERR_NOT_INITIALIZED;

    huffmanTables->NumACTable = m_dec[0]->GetNumACTables();
    for(int i=0; i<huffmanTables->NumACTable; i++)
    {
        m_dec[0]->FillACTable(i, huffmanTables->ACTables[i].Bits, huffmanTables->ACTables[i].Values);
    }

    huffmanTables->NumDCTable = m_dec[0]->GetNumDCTables();
    for(int i=0; i<huffmanTables->NumDCTable; i++)
    {
        m_dec[0]->FillDCTable(i, huffmanTables->DCTables[i].Bits, huffmanTables->DCTables[i].Values);
    }

    return UMC_OK;
}

Status MJPEGVideoDecoderMFX::DecodeHeader(MediaData* in)
{
    if(0 == in)
        return UMC_ERR_NOT_ENOUGH_DATA;

    JpegFrameConstructor frameConstructor;
    frameConstructor.Init();

    MediaData dst;

    for ( ; in->GetDataSize() > 3; )
    {
        int32_t code = frameConstructor.CheckMarker(in);

        if (!code)
        {
            return UMC_ERR_NOT_ENOUGH_DATA;
        }

        if (code != JM_SOI)
        {
            frameConstructor.GetMarker(in, &dst);
        }
        else
        {
            break;
        }
    }

    Status sts = _GetFrameInfo((uint8_t*)in->GetDataPointer(), in->GetDataSize());

    if (sts == UMC_ERR_NOT_ENOUGH_DATA &&
        (!(in->GetFlags() & MediaData::FLAG_VIDEO_DATA_NOT_FULL_FRAME) ||
        (in->GetFlags() & MediaData::FLAG_VIDEO_DATA_END_OF_STREAM)))
    {
        return UMC_ERR_INVALID_STREAM;
    }

    return sts;
}

Status MJPEGVideoDecoderMFX::AllocateFrame()
{
    mfxSize size;
    size.height = m_DecoderParams.info.disp_clip_info.height;
    size.width = m_DecoderParams.info.disp_clip_info.width;

    VideoDataInfo info;
    info.Init(size.width, size.height, m_DecoderParams.info.color_format, 8);
    info.SetPictureStructure(m_interleaved ? VideoDataInfo::PS_FRAME : VideoDataInfo::PS_TOP_FIELD_FIRST);

    FrameMemID frmMID;
    Status sts = m_frameAllocator->Alloc(&frmMID, &info, 0);

    if (sts != UMC_OK)
    {
        return UMC_ERR_ALLOC;
    }

    const FrameData *frmData = m_frameAllocator->Lock(frmMID);
    if (!frmData)
        return UMC_ERR_LOCK;

    m_frameData = *frmData;
    if (frmData->GetPlaneMemoryInfo(0)->m_planePtr)
        m_frameData.m_locked = true;

    ColorFormat frm = NONE;
    m_needPostProcessing = false;

    if((JC_RGB == m_color || JC_GRAY == m_color) &&
       RGB32 == m_DecoderParams.info.color_format)
    {
        frm = RGB32;
    }
    else if((JC_YCBCR == m_color || JC_GRAY == m_color) &&
            NV12 == m_DecoderParams.info.color_format)
    {
        frm = NV12;
    }
    // WA to support YUY2 output when HW decoder uses SW fallback
    else if((JC_YCBCR == m_color || JC_GRAY == m_color) &&
            YUY2 == m_DecoderParams.info.color_format)
    {
        frm = NV12;
        m_needPostProcessing = true;
    }
    else if(JC_RGB == m_color &&
            NV12 == m_DecoderParams.info.color_format)
    {
        // single scan
        if(m_dec[0]->m_jpeg_ncomp == m_dec[0]->m_scans[0].ncomps)
        {
            frm = NV12;
        }
        // multiple scans
        else
        {
            frm = RGB32;
            m_needPostProcessing = true;
        }
    }
    // WA to support YUY2 output when HW decoder uses SW fallback
    else if(JC_RGB == m_color &&
            YUY2 == m_DecoderParams.info.color_format)
    {
        // single scan
        if(m_dec[0]->m_jpeg_ncomp == m_dec[0]->m_scans[0].ncomps)
        {
            frm = NV12;
            m_needPostProcessing = true;
        }
        // multiple scans
        else
        {
            frm = RGB32;
            m_needPostProcessing = true;
        }
    }
    else if(JC_YCBCR == m_color &&
            RGB32 == m_DecoderParams.info.color_format)
    {
        // single scan
        if(m_dec[0]->m_jpeg_ncomp == m_dec[0]->m_scans[0].ncomps)
        {
            frm = RGB32;
        }
        // multiple scans
        else
        {
            frm = YUV444;
            m_needPostProcessing = true;
        }
    }
    else
    {
        return UMC_ERR_UNSUPPORTED;
    }

    size = m_frameDims;
    AdjustFrameSize(size);
    m_internalFrame.Init(size.width, size.height, frm);

    if(m_needPostProcessing || m_rotation)
    {
        sts = m_internalFrame.Alloc();
        if (UMC_OK != sts)
            return UMC_ERR_ALLOC;
    }
    else
    {
        if (RGB32 == m_internalFrame.GetColorFormat())
        {
            m_internalFrame.SetPlanePointer(frmData->GetPlaneMemoryInfo(0)->m_planePtr, 0);
            m_internalFrame.SetPlanePitch(frmData->GetPlaneMemoryInfo(0)->m_pitch, 0);
        }
        else if (NV12 == m_internalFrame.GetColorFormat())
        {
            m_internalFrame.SetPlanePointer(frmData->GetPlaneMemoryInfo(0)->m_planePtr, 0);
            m_internalFrame.SetPlanePitch(frmData->GetPlaneMemoryInfo(0)->m_pitch, 0);
            m_internalFrame.SetPlanePointer(frmData->GetPlaneMemoryInfo(1)->m_planePtr, 1);
            m_internalFrame.SetPlanePitch(frmData->GetPlaneMemoryInfo(1)->m_pitch, 1);
        }
    }

    return UMC_OK;
}

FrameData *MJPEGVideoDecoderMFX::GetDst(void)
{
    return &m_frameData;

} // FrameData *MJPEGVideoDecoderMFX::GetDst(void)

Status MJPEGVideoDecoderMFX::SetRotation(uint16_t rotation)
{
    m_rotation = rotation;

    return UMC_OK;

} // MJPEGVideoDecoderMFX::SetRotation(uint16_t rotation)

Status MJPEGVideoDecoderMFX::SetColorSpace(uint16_t chromaFormat, uint16_t colorFormat)
{
    if(m_dec.empty() || m_dec.size() > JPEG_MAX_THREADS)
        return UMC_ERR_FAILED;

    for (auto& dec: m_dec)
    {
        dec->m_jpeg_color = GetUMCColorType(chromaFormat, colorFormat);
    }

    return UMC_OK;

} // MJPEGVideoDecoderMFX::SetRotation(uint16_t rotation)

Status MJPEGVideoDecoderMFX::DecodePicture(const CJpegTask &task,
                                           const mfxU32 threadNumber,
                                           const mfxU32 callNumber)
{
    Status umcRes = UMC_OK;
    mfxU32 picNum, pieceNum;
    mfxI32 curr_scan_no;
    JERRCODE jerr;
    int i;
/*
    if(0 == out)
        return UMC_ERR_NULL_PTR;
    *out = 0;*/
    MFX_LTRACE_1(MFX_TRACE_LEVEL_INTERNAL, "MJPEG, frame: ", "%d", m_frameNo);

    // find appropriate source picture buffer
    picNum = 0;
    pieceNum = callNumber;
    while (task.GetPictureBuffer(picNum).numPieces <= pieceNum)
    {
        pieceNum -= task.GetPictureBuffer(picNum).numPieces;
        picNum += 1;
    }
    const CJpegTaskBuffer &picBuffer = task.GetPictureBuffer(picNum);

    // check if there is a need to decode the header
    if (m_pLastPicBuffer[threadNumber] != &picBuffer)
    {
        int32_t nUsedBytes = 0;

        // set the source data
        umcRes = _DecodeHeader((uint8_t *) picBuffer.pBuf,
                               picBuffer.imageHeaderSize + picBuffer.scanSize[0],
                               &nUsedBytes, threadNumber);
        if (UMC_OK != umcRes)
        {
            return umcRes;
        }
        // save the pointer to the last decoded picture
        m_pLastPicBuffer[threadNumber] = &picBuffer;

        m_dec[threadNumber]->m_curr_scan = &m_dec[threadNumber]->m_scans[0];
    }

    // determinate scan number contained current piece
    if(picBuffer.scanOffset[0] <= picBuffer.pieceOffset[pieceNum] &&
       (picBuffer.pieceOffset[pieceNum] < picBuffer.scanOffset[1] || 0 == picBuffer.scanOffset[1]))
    {
        curr_scan_no = 0;
    }
    else if(picBuffer.scanOffset[1] <= picBuffer.pieceOffset[pieceNum] &&
        (picBuffer.pieceOffset[pieceNum] < picBuffer.scanOffset[2] || 0 == picBuffer.scanOffset[2]))
    {
        curr_scan_no = 1;
    }
    else if(picBuffer.scanOffset[2] <= picBuffer.pieceOffset[pieceNum])
    {
        curr_scan_no = 2;
    }
    else
    {
        return UMC_ERR_FAILED;
    }

    // check if there is a need to decode scan header and DRI segment
    if(m_dec[threadNumber]->m_curr_scan->scan_no != curr_scan_no)
    {
        for(i = 1; i <= curr_scan_no; i++)
        {
            m_dec[threadNumber]->m_curr_scan = &m_dec[threadNumber]->m_scans[i];

            if(picBuffer.scanTablesOffset[i] != 0)
            {
                int32_t nUsedBytes = 0;

                umcRes = _DecodeHeader((uint8_t *) picBuffer.pBuf + picBuffer.scanTablesOffset[i],
                                       picBuffer.scanTablesSize[i] + picBuffer.scanSize[i],
                                       &nUsedBytes, threadNumber);
                if (UMC_OK != umcRes)
                {
                    return umcRes;
                }
            }
        }
    }

    m_dec[threadNumber]->m_num_scans = picBuffer.numScans;

    // set the next piece to the decoder
    jerr = m_dec[threadNumber]->SetSource(picBuffer.pBuf + picBuffer.pieceOffset[pieceNum],
                                          picBuffer.pieceSize[pieceNum]);
    if(JPEG_OK != jerr)
        return UMC_ERR_FAILED;

    // decode a next piece from the picture
    umcRes = DecodePiece(picBuffer.fieldPos,
                         (mfxU32)picBuffer.pieceRSTOffset[pieceNum],
                         (mfxU32)(picBuffer.pieceRSTOffset[pieceNum+1] - picBuffer.pieceRSTOffset[pieceNum]),
                         threadNumber);

    if (UMC_OK != umcRes)
    {
        task.surface_out->Data.Corrupted = 1;
        return umcRes;
    }

    return UMC_OK;

} // Status MJPEGVideoDecoderMFX::DecodePicture(const CJpegTask &task,

Status MJPEGVideoDecoderMFX::PostProcessing(double pts)
{
    VideoData rotatedFrame;

    m_frameNo++;

    if (pts > -1.0)
    {
        m_local_frame_time = pts;
    }

    m_frameData.SetTime(m_local_frame_time);
    m_local_frame_time += m_local_delta_frame_time;

    if(m_rotation)
    {
        mfxSize size;
        uint8_t* pSrc[4];
        size_t pSrcPitch[4];
        uint8_t* pDst[4];
        size_t pDstPitch[4];
        Status sts = UMC_OK;

        size.height = m_DecoderParams.info.disp_clip_info.height;
        size.width = m_DecoderParams.info.disp_clip_info.width;

        rotatedFrame.Init(size.width, size.height, m_internalFrame.GetColorFormat());

        if(m_needPostProcessing)
        {
            sts = rotatedFrame.Alloc();
            if (UMC_OK != sts)
                return UMC_ERR_ALLOC;
        }
        else
        {
            if (RGB32 == m_internalFrame.GetColorFormat())
            {
                rotatedFrame.SetPlanePointer(m_frameData.GetPlaneMemoryInfo(0)->m_planePtr, 0);
                rotatedFrame.SetPlanePitch(m_frameData.GetPlaneMemoryInfo(0)->m_pitch, 0);
            }
            else if (NV12 == m_internalFrame.GetColorFormat())
            {
                rotatedFrame.SetPlanePointer(m_frameData.GetPlaneMemoryInfo(0)->m_planePtr, 0);
                rotatedFrame.SetPlanePitch(m_frameData.GetPlaneMemoryInfo(0)->m_pitch, 0);
                rotatedFrame.SetPlanePointer(m_frameData.GetPlaneMemoryInfo(1)->m_planePtr, 1);
                rotatedFrame.SetPlanePitch(m_frameData.GetPlaneMemoryInfo(1)->m_pitch, 1);
            }
        }

        switch(m_internalFrame.GetColorFormat())
        {
        case NV12:
            {
                pSrc[0] = (uint8_t*)m_internalFrame.GetPlanePointer(0);
                pSrc[1] = (uint8_t*)m_internalFrame.GetPlanePointer(1);
                pSrcPitch[0] = m_internalFrame.GetPlanePitch(0);
                pSrcPitch[1] = m_internalFrame.GetPlanePitch(1);

                pDst[0] = (uint8_t*)rotatedFrame.GetPlanePointer(0);
                pDst[1] = (uint8_t*)rotatedFrame.GetPlanePointer(1);
                pDstPitch[0] = rotatedFrame.GetPlanePitch(0);
                pDstPitch[1] = rotatedFrame.GetPlanePitch(1);

                switch(m_rotation)
                {
                case 90:

                    for(int32_t i=0; i<m_frameDims.width >> 1; i++)
                        for(int32_t j=0; j<m_frameDims.height >> 1; j++)
                        {
                            pDst[0][( i<<1   )*pDstPitch[0] + (j<<1)  ] = pSrc[0][(m_frameDims.height-1-(j<<1)) * pSrcPitch[0] + (i<<1)  ];
                            pDst[0][((i<<1)+1)*pDstPitch[0] + (j<<1)  ] = pSrc[0][(m_frameDims.height-1-(j<<1)) * pSrcPitch[0] + (i<<1)+1];
                            pDst[0][( i<<1   )*pDstPitch[0] + (j<<1)+1] = pSrc[0][(m_frameDims.height-2-(j<<1)) * pSrcPitch[0] + (i<<1)  ];
                            pDst[0][((i<<1)+1)*pDstPitch[0] + (j<<1)+1] = pSrc[0][(m_frameDims.height-2-(j<<1)) * pSrcPitch[0] + (i<<1)+1];

                            pDst[1][i*pDstPitch[1] + (j<<1)  ] = pSrc[1][((m_frameDims.height>>1)-1-j)*pSrcPitch[1] + (i<<1)  ];
                            pDst[1][i*pDstPitch[1] + (j<<1)+1] = pSrc[1][((m_frameDims.height>>1)-1-j)*pSrcPitch[1] + (i<<1)+1];
                        }
                    break;

                case 180:

                    for(int32_t i=0; i<m_frameDims.height >> 1; i++)
                        for(int32_t j=0; j<m_frameDims.width >> 1; j++)
                        {
                            pDst[0][( i<<1   )*pDstPitch[0] + (j<<1)  ] = pSrc[0][(m_frameDims.height-1-(i<<1)) * pSrcPitch[0] + m_frameDims.width-1-(j<<1)];
                            pDst[0][((i<<1)+1)*pDstPitch[0] + (j<<1)  ] = pSrc[0][(m_frameDims.height-2-(i<<1)) * pSrcPitch[0] + m_frameDims.width-1-(j<<1)];
                            pDst[0][( i<<1   )*pDstPitch[0] + (j<<1)+1] = pSrc[0][(m_frameDims.height-1-(i<<1)) * pSrcPitch[0] + m_frameDims.width-2-(j<<1)];
                            pDst[0][((i<<1)+1)*pDstPitch[0] + (j<<1)+1] = pSrc[0][(m_frameDims.height-2-(i<<1)) * pSrcPitch[0] + m_frameDims.width-2-(j<<1)];

                            pDst[1][i*pDstPitch[1] + (j<<1)  ] = pSrc[1][((m_frameDims.height>>1)-1-i)*pSrcPitch[1] + m_frameDims.width-2-(j<<1)];
                            pDst[1][i*pDstPitch[1] + (j<<1)+1] = pSrc[1][((m_frameDims.height>>1)-1-i)*pSrcPitch[1] + m_frameDims.width-1-(j<<1)];
                        }
                    break;

                case 270:

                    for(int32_t i=0; i<m_frameDims.width >> 1; i++)
                        for(int32_t j=0; j<m_frameDims.height >> 1; j++)
                        {
                            pDst[0][( i<<1   )*pDstPitch[0] + (j<<1)  ] = pSrc[0][((j<<1)  ) * pSrcPitch[0] + m_frameDims.width-1-(i<<1)];
                            pDst[0][((i<<1)+1)*pDstPitch[0] + (j<<1)  ] = pSrc[0][((j<<1)  ) * pSrcPitch[0] + m_frameDims.width-2-(i<<1)];
                            pDst[0][( i<<1   )*pDstPitch[0] + (j<<1)+1] = pSrc[0][((j<<1)+1) * pSrcPitch[0] + m_frameDims.width-1-(i<<1)];
                            pDst[0][((i<<1)+1)*pDstPitch[0] + (j<<1)+1] = pSrc[0][((j<<1)+1) * pSrcPitch[0] + m_frameDims.width-2-(i<<1)];

                            pDst[1][i*pDstPitch[1] + (j<<1)  ] = pSrc[1][j*pSrcPitch[1] + m_frameDims.width-2-(i<<1)];
                            pDst[1][i*pDstPitch[1] + (j<<1)+1] = pSrc[1][j*pSrcPitch[1] + m_frameDims.width-1-(i<<1)];
                        }
                    break;
                }
            }
            break;

        case RGB32:
            {
                pSrc[0] = (uint8_t*)m_internalFrame.GetPlanePointer(0);
                pSrcPitch[0] = m_internalFrame.GetPlanePitch(0);

                pDst[0] = (uint8_t*)rotatedFrame.GetPlanePointer(0);
                pDstPitch[0] = rotatedFrame.GetPlanePitch(0);

                switch(m_rotation)
                {
                case 90:

                    for(int32_t i=0; i<m_frameDims.width; i++)
                        for(int32_t j=0; j<m_frameDims.height; j++)
                        {
                            pDst[0][i*pDstPitch[0] + j*4    ] = pSrc[0][(m_frameDims.height-1-j)*pSrcPitch[0] + i*4    ];
                            pDst[0][i*pDstPitch[0] + j*4 + 1] = pSrc[0][(m_frameDims.height-1-j)*pSrcPitch[0] + i*4 + 1];
                            pDst[0][i*pDstPitch[0] + j*4 + 2] = pSrc[0][(m_frameDims.height-1-j)*pSrcPitch[0] + i*4 + 2];
                            pDst[0][i*pDstPitch[0] + j*4 + 3] = 0xFF;
                        }
                    break;

                case 180:

                    for(int32_t i=0; i<m_frameDims.height; i++)
                        for(int32_t j=0; j<m_frameDims.width; j++)
                        {
                            pDst[0][i*pDstPitch[0] + j*4    ] = pSrc[0][(m_frameDims.height-1-i)*pSrcPitch[0] + (m_frameDims.width-j)*4-4];
                            pDst[0][i*pDstPitch[0] + j*4 + 1] = pSrc[0][(m_frameDims.height-1-i)*pSrcPitch[0] + (m_frameDims.width-j)*4-3];
                            pDst[0][i*pDstPitch[0] + j*4 + 2] = pSrc[0][(m_frameDims.height-1-i)*pSrcPitch[0] + (m_frameDims.width-j)*4-2];
                            pDst[0][i*pDstPitch[0] + j*4 + 3] = 0xFF;
                        }
                    break;

                case 270:

                    for(int32_t i=0; i<m_frameDims.width; i++)
                        for(int32_t j=0; j<m_frameDims.height; j++)
                        {
                            pDst[0][i*pDstPitch[0] + j*4    ] = pSrc[0][j*pSrcPitch[0] + (m_frameDims.width-1-i)*4    ];
                            pDst[0][i*pDstPitch[0] + j*4 + 1] = pSrc[0][j*pSrcPitch[0] + (m_frameDims.width-1-i)*4 + 1];
                            pDst[0][i*pDstPitch[0] + j*4 + 2] = pSrc[0][j*pSrcPitch[0] + (m_frameDims.width-1-i)*4 + 2];
                            pDst[0][i*pDstPitch[0] + j*4 + 3] = 0xFF;
                        }
                    break;
                }
            }
            break;

        case YUV444:
            {
                pSrc[0] = (uint8_t*)m_internalFrame.GetPlanePointer(0);
                pSrc[1] = (uint8_t*)m_internalFrame.GetPlanePointer(1);
                pSrc[2] = (uint8_t*)m_internalFrame.GetPlanePointer(2);
                pSrcPitch[0] = m_internalFrame.GetPlanePitch(0);
                pSrcPitch[1] = m_internalFrame.GetPlanePitch(1);
                pSrcPitch[2] = m_internalFrame.GetPlanePitch(2);

                pDst[0] = (uint8_t*)rotatedFrame.GetPlanePointer(0);
                pDst[1] = (uint8_t*)rotatedFrame.GetPlanePointer(1);
                pDst[2] = (uint8_t*)rotatedFrame.GetPlanePointer(2);
                pDstPitch[0] = rotatedFrame.GetPlanePitch(0);
                pDstPitch[1] = rotatedFrame.GetPlanePitch(1);
                pDstPitch[2] = rotatedFrame.GetPlanePitch(2);

                switch(m_rotation)
                {
                case 90:

                    for(int32_t i=0; i<m_frameDims.width; i++)
                        for(int32_t j=0; j<m_frameDims.height; j++)
                        {
                            pDst[0][i*pDstPitch[0] + j] = pSrc[0][(m_frameDims.height-1-j) * pSrcPitch[0] + i];
                            pDst[1][i*pDstPitch[1] + j] = pSrc[1][(m_frameDims.height-1-j) * pSrcPitch[1] + i];
                            pDst[2][i*pDstPitch[2] + j] = pSrc[2][(m_frameDims.height-1-j) * pSrcPitch[2] + i];
                        }
                    break;

                case 180:

                    for(int32_t i=0; i<m_frameDims.height; i++)
                        for(int32_t j=0; j<m_frameDims.width; j++)
                        {
                            pDst[0][i*pDstPitch[0] + j] = pSrc[0][(m_frameDims.height-1-i) * pSrcPitch[0] + m_frameDims.width-1-j];
                            pDst[1][i*pDstPitch[1] + j] = pSrc[1][(m_frameDims.height-1-i) * pSrcPitch[1] + m_frameDims.width-1-j];
                            pDst[2][i*pDstPitch[2] + j] = pSrc[2][(m_frameDims.height-1-i) * pSrcPitch[2] + m_frameDims.width-1-j];
                        }
                    break;

                case 270:

                    for(int32_t i=0; i<m_frameDims.width; i++)
                        for(int32_t j=0; j<m_frameDims.height; j++)
                        {
                            pDst[0][i*pDstPitch[0] + j] = pSrc[0][j * pSrcPitch[0] + m_frameDims.width-1-i];
                            pDst[1][i*pDstPitch[1] + j] = pSrc[1][j * pSrcPitch[1] + m_frameDims.width-1-i];
                            pDst[2][i*pDstPitch[2] + j] = pSrc[2][j * pSrcPitch[2] + m_frameDims.width-1-i];
                        }
                    break;
                }
            }
            break;
            default:
                break;
        }

        m_internalFrame.Reset();
        m_internalFrame = rotatedFrame;
    }

    if(m_needPostProcessing)
    {
        mfxSize size;
        size.height = m_DecoderParams.info.disp_clip_info.height;
        size.width = m_DecoderParams.info.disp_clip_info.width;

        AdjustFrameSize(size);

        VideoData out;

        switch (m_DecoderParams.info.color_format)
        {
        case RGB32:
            out.Init(size.width, size.height, RGB32, 8);
            out.SetPlanePointer(m_frameData.GetPlaneMemoryInfo(0)->m_planePtr, 0);
            out.SetPlanePitch(m_frameData.GetPlaneMemoryInfo(0)->m_pitch, 0);
            break;
        case NV12:
            out.Init(size.width, size.height, NV12, 8);
            out.SetPlanePointer(m_frameData.GetPlaneMemoryInfo(0)->m_planePtr, 0);
            out.SetPlanePitch(m_frameData.GetPlaneMemoryInfo(0)->m_pitch, 0);
            out.SetPlanePointer(m_frameData.GetPlaneMemoryInfo(1)->m_planePtr, 1);
            out.SetPlanePitch(m_frameData.GetPlaneMemoryInfo(1)->m_pitch, 1);
            break;
        case YUY2:
            out.Init(size.width, size.height, YUY2, 8);
            out.SetPlanePointer(m_frameData.GetPlaneMemoryInfo(0)->m_planePtr, 0);
            out.SetPlanePitch(m_frameData.GetPlaneMemoryInfo(0)->m_pitch, 0);
            break;
        default:
            return UMC_ERR_UNSUPPORTED;
        }

        m_internalFrame.SetPictureStructure(m_interleaved ? PS_TOP_FIELD_FIRST : PS_FRAME);
        out.SetPictureStructure(m_internalFrame.GetPictureStructure());

        if (!m_PostProcessing)
        {
            m_PostProcessing.reset(createVideoProcessing());
        }

        Status status = m_PostProcessing->GetFrame(&m_internalFrame, &out);
        if(status != UMC_OK)
        {
            return status;
        }
    }

    rotatedFrame.Close();

    return UMC_OK;
}

Status MJPEGVideoDecoderMFX::_GetFrameInfo(const uint8_t* pBitStream, size_t nSize)
{
    int32_t   nchannels;
    int32_t   precision;
    JSS      sampling;
    JCOLOR   color;
    JERRCODE jerr;

    if (!m_IsInit)
        return UMC_ERR_NOT_INITIALIZED;

    jerr = m_dec[0]->SetSource(pBitStream, nSize);
    if(JPEG_OK != jerr)
        return UMC_ERR_FAILED;

    jerr = m_dec[0]->ReadHeader(
        &m_frameDims.width,&m_frameDims.height,&nchannels,&color,&sampling,&precision);

    if(JPEG_ERR_BUFF == jerr)
        return UMC_ERR_NOT_ENOUGH_DATA;

    if(JPEG_NOT_IMPLEMENTED == jerr)
        return UMC_ERR_NOT_IMPLEMENTED;

    if(JPEG_OK != jerr)
        return UMC_ERR_FAILED;

    m_frameSampling = (int) sampling;
    m_color = color;
    m_interleavedScan = m_dec[0]->IsInterleavedScan();

    // frame is interleaved if clip height is twice more than JPEG frame height
    if (m_DecoderParams.info.interlace_type != PROGRESSIVE)
    {
        m_interleaved = true;
        m_frameDims.height *= 2;
    }

    return UMC_OK;
} // MJPEGVideoDecoderMFX::_GetFrameInfo()

Status MJPEGVideoDecoderMFX::CloseFrame(void)
{
    m_frameData.Close();

    std::fill(std::begin(m_pLastPicBuffer), std::end(m_pLastPicBuffer), nullptr);

    return UMC_OK;

} // Status MJPEGVideoDecoderMFX::CloseFrame(void)

Status MJPEGVideoDecoderMFX::_DecodeHeader(const uint8_t* pBuf, size_t buflen, int32_t* cnt, const uint32_t threadNum)
{
    JSS      sampling;
    JERRCODE jerr;

    if (!m_IsInit)
        return UMC_ERR_NOT_INITIALIZED;

    jerr = m_dec[threadNum]->SetSource(pBuf, buflen);
    if(JPEG_OK != jerr)
        return UMC_ERR_FAILED;

    mfxSize size = {};

    jerr = m_dec[threadNum]->ReadHeader(
        &size.width, &size.height, &m_frameChannels, &m_color, &sampling, &m_framePrecision);

    if(JPEG_ERR_BUFF == jerr)
        return UMC_ERR_NOT_ENOUGH_DATA;

    if(JPEG_OK != jerr)
        return UMC_ERR_FAILED;

    bool sizeHaveChanged = (m_frameDims.width != size.width || (m_frameDims.height != size.height && m_frameDims.height != (size.height << 1)));

    if ((m_frameSampling != (int)sampling) || (m_frameDims.width && sizeHaveChanged))
    {
        m_dec[threadNum]->Seek(-m_dec[threadNum]->GetNumDecodedBytes(),UIC_SEEK_CUR);
        *cnt = 0;
        return UMC_ERR_NOT_ENOUGH_DATA;//UMC_WRN_INVALID_STREAM;
    }

    m_frameSampling = (int)sampling;

    *cnt = m_dec[threadNum]->GetNumDecodedBytes();
    return UMC_OK;
}

Status MJPEGVideoDecoderMFX::DecodePiece(const mfxU32 fieldNum,
                                         const mfxU32 restartNum,
                                         const mfxU32 restartsToDecode,
                                         const mfxU32 threadNum)
{
    int32_t   dstPlaneStep[4];
    uint8_t*   pDstPlane[4];
    int32_t   dstStep;
    uint8_t*   pDst;
    //JSS      sampling = (JSS)m_frameSampling;
    JERRCODE jerr = JPEG_OK;
    mfxSize dimension = m_frameDims;

    if (!m_IsInit)
        return UMC_ERR_NOT_INITIALIZED;

    if (RGB32 == m_internalFrame.GetColorFormat())
    {
        dstStep = (int32_t)m_internalFrame.GetPlanePitch(0);
        pDst = (uint8_t*)m_internalFrame.GetPlanePointer(0);
        if (m_interleaved)
        {
            if (fieldNum & 1)//!m_firstField)
                pDst += dstStep;
            dstStep *= 2;

            dimension.height /= 2;
        }

        jerr = m_dec[threadNum]->SetDestination(pDst, dstStep, dimension, m_frameChannels, JC_BGRA, JS_444);
    }
    else if (YUV444 == m_internalFrame.GetColorFormat())
    {
        pDstPlane[0] = (uint8_t*)m_internalFrame.GetPlanePointer(0);
        pDstPlane[1] = (uint8_t*)m_internalFrame.GetPlanePointer(1);
        pDstPlane[2] = (uint8_t*)m_internalFrame.GetPlanePointer(2);
        pDstPlane[3] = 0;

        dstPlaneStep[0] = (int32_t)m_internalFrame.GetPlanePitch(0);
        dstPlaneStep[1] = (int32_t)m_internalFrame.GetPlanePitch(1);
        dstPlaneStep[2] = (int32_t)m_internalFrame.GetPlanePitch(2);
        dstPlaneStep[3] = 0;

        if (m_interleaved)
        {
            for (int32_t i = 0; i < 3; i++)
            {
                if (fieldNum & 1)//!m_firstField)
                    pDstPlane[i] += dstPlaneStep[i];
                dstPlaneStep[i] *= 2;
            }

            dimension.height /= 2;
        }

        jerr = m_dec[threadNum]->SetDestination(pDstPlane, dstPlaneStep, dimension, m_frameChannels, JC_YCBCR, JS_444);
    }
    else if (NV12 == m_internalFrame.GetColorFormat())
    {
        pDstPlane[0] = (uint8_t*)m_internalFrame.GetPlanePointer(0);
        pDstPlane[1] = (uint8_t*)m_internalFrame.GetPlanePointer(1);
        pDstPlane[2] = 0;
        pDstPlane[3] = 0;

        dstPlaneStep[0] = (int32_t)m_internalFrame.GetPlanePitch(0);
        dstPlaneStep[1] = (int32_t)m_internalFrame.GetPlanePitch(1);
        dstPlaneStep[2] = 0;
        dstPlaneStep[3] = 0;

        if (m_interleaved)
        {
            for (int32_t i = 0; i < 3; i++)
            {
                if (fieldNum & 1)//!m_firstField)
                    pDstPlane[i] += dstPlaneStep[i];
                dstPlaneStep[i] *= 2;
            }

            dimension.height /= 2;
        }

        jerr = m_dec[threadNum]->SetDestination(pDstPlane, dstPlaneStep, dimension, m_frameChannels, JC_NV12, JS_420);
    }
    else
    {
        return UMC_ERR_FAILED;
    }

    if(JPEG_OK != jerr)
        return UMC_ERR_FAILED;

    jerr = m_dec[threadNum]->ReadData(restartNum, restartsToDecode);

    if(JPEG_ERR_BUFF == jerr)
        return UMC_ERR_NOT_ENOUGH_DATA;

    if(JPEG_OK != jerr)
        return UMC_ERR_FAILED;

    return UMC_OK;

} // Status MJPEGVideoDecoderMFX::DecodePiece(const mfxU32 fieldNum,

void MJPEGVideoDecoderMFX::SetFrameAllocator(FrameAllocator * frameAllocator)
{
    assert(frameAllocator);
    m_frameAllocator = frameAllocator;
}

ConvertInfo * MJPEGVideoDecoderMFX::GetConvertInfo()
{
    return 0;
}

Status MJPEGVideoDecoderMFX::FindStartOfImage(MediaData * in)
{
    CMemBuffInput source;
    JERRCODE jerr;

    if (!m_IsInit)
        return UMC_ERR_NOT_INITIALIZED;

    jerr = m_dec[0]->SetSource((uint8_t*) in->GetDataPointer(), in->GetDataSize());
    if(JPEG_OK != jerr)
        return UMC_ERR_FAILED;

    jerr = m_dec[0]->FindSOI();

    if(JPEG_ERR_BUFF == jerr)
        return UMC_ERR_NOT_ENOUGH_DATA;

    if(JPEG_OK != jerr)
        return UMC_ERR_FAILED;

    in->MoveDataPointer(m_dec[0]->m_BitStreamIn.GetNumUsedBytes());

    return UMC_OK;
}

} // end namespace UMC

#endif // MFX_ENABLE_MJPEG_VIDEO_DECODE && MFX_ENABLE_SW_FALLBACK
