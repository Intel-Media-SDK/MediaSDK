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

#include "umc_defs.h"
#if defined (MFX_ENABLE_MJPEG_VIDEO_DECODE)
#include "vm_debug.h"
#include "umc_video_data.h"
#include "umc_mjpeg_mfx_decode_base.h"
#include "membuffin.h"
#include "umc_jpeg_frame_constructor.h"
#include "mfx_utils.h"

namespace UMC
{

MJPEGVideoDecoderBaseMFX::MJPEGVideoDecoderBaseMFX(void) : m_frameDims()
{
    m_IsInit      = false;
    m_interleaved = false;
    m_interleavedScan = false;
    m_rotation    = 0;
    m_frameSampling = 0;
    m_frameAllocator = 0;
    m_decBase = 0;
    m_color = JC_UNKNOWN;
}

MJPEGVideoDecoderBaseMFX::~MJPEGVideoDecoderBaseMFX(void)
{
    Close();
}

Status MJPEGVideoDecoderBaseMFX::Init(BaseCodecParams* lpInit)
{
    VideoDecoderParams* pDecoderParams = DynamicCast<VideoDecoderParams>(lpInit);

    if(0 == pDecoderParams)
        return UMC_ERR_NULL_PTR;

    Status status = Close();
    if(UMC_OK != status)
        return UMC_ERR_INIT;

    m_DecoderParams  = *pDecoderParams;

    m_IsInit       = true;
    m_interleaved  = false;
    m_interleavedScan = false;
    m_frameSampling = 0;
    m_rotation     = 0;

    m_decoder.reset(new CJPEGDecoderBase());
    m_decBase = m_decoder.get();
    return UMC_OK;
}

Status MJPEGVideoDecoderBaseMFX::Reset(void)
{
    m_IsInit       = true;
    m_interleaved  = false;
    m_interleavedScan = false;
    m_frameSampling = 0;
    m_rotation     = 0;

    m_frameData.Close();
    m_decBase->Reset();

    return UMC_OK;
}


Status MJPEGVideoDecoderBaseMFX::Close(void)
{
    m_IsInit       = false;
    m_interleaved  = false;
    m_interleavedScan = false;
    m_frameSampling = 0;
    m_rotation     = 0;
    m_frameData.Close();
    m_decBase = 0;
    return UMC_OK;
}

void MJPEGVideoDecoderBaseMFX::AdjustFrameSize(mfxSize & size)
{
    size.width  = mfx::align2_value(size.width, 16);
    size.height = mfx::align2_value(size.height, m_interleaved ? 32 : 16);
}

std::pair<ChromaType, Status> MJPEGVideoDecoderBaseMFX::GetChromaType()
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

    if (m_decBase->m_jpeg_ncomp == 1)
    {
        return { CHROMA_TYPE_YUV400, UMC_OK };
    }

    ChromaType type = CHROMA_TYPE_YUV400;
    switch(m_decBase->m_ccomp[0].m_hsampling)
    {
    case 1: // YUV422V_2Y, YUV444
        if (m_decBase->m_ccomp[0].m_vsampling == 1) // YUV444
        {
            if (m_decBase->m_jpeg_color == JC_YCBCR)
                type = CHROMA_TYPE_YUV444;
            else if (m_decBase->m_jpeg_color == JC_RGB)
                type = CHROMA_TYPE_RGB;
            else
                return { CHROMA_TYPE_YUV400, UMC_ERR_UNSUPPORTED };
        }
        else
        {
            if (m_decBase->m_ccomp[0].m_vsampling != 2 || m_decBase->m_ccomp[1].m_hsampling != 1)
            {
                return { CHROMA_TYPE_YUV400, UMC_ERR_UNSUPPORTED };
            }
            type = CHROMA_TYPE_YUV422V_2Y; // YUV422V_2Y
        }
        break;
    case 2: // YUV420, YUV422H_2Y, YUV422H_4Y, YUV422V_4Y
        if (m_decBase->m_ccomp[0].m_vsampling == 1)
        {
            if ( m_decBase->m_ccomp[1].m_vsampling != 1 || m_decBase->m_ccomp[1].m_hsampling != 1)
            {
                return { CHROMA_TYPE_YUV400, UMC_ERR_UNSUPPORTED };
            }
            type = CHROMA_TYPE_YUV422H_2Y; // YUV422H_2Y
        }
        else
        {
            if (m_decBase->m_ccomp[0].m_vsampling != 2)
            {
                return { CHROMA_TYPE_YUV400, UMC_ERR_UNSUPPORTED };
            }

            if (m_decBase->m_ccomp[1].m_hsampling == 1 && m_decBase->m_ccomp[1].m_vsampling == 1)
                type = CHROMA_TYPE_YUV420; // YUV420;
            else
                type = (m_decBase->m_ccomp[1].m_hsampling == 1) ? CHROMA_TYPE_YUV422H_4Y : CHROMA_TYPE_YUV422V_4Y; // YUV422H_4Y, YUV422V_4Y
        }
        break;
    case 4: // YUV411
        if (m_decBase->m_ccomp[0].m_vsampling != 1)
        {
            return { CHROMA_TYPE_YUV400, UMC_ERR_UNSUPPORTED };
        }
        type = CHROMA_TYPE_YUV411;
        break;
    default:
        return { CHROMA_TYPE_YUV400, UMC_ERR_UNSUPPORTED };
    }

    return { type, UMC_OK };
}

std::pair<JCOLOR, Status> MJPEGVideoDecoderBaseMFX::GetColorType()
{
    JCOLOR color = JC_UNKNOWN;
    switch(m_decBase->m_jpeg_color)
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
        return { JC_UNKNOWN, UMC_ERR_UNSUPPORTED };
    }

    return { color, UMC_OK };
}

Status MJPEGVideoDecoderBaseMFX::FillVideoParam(mfxVideoParam *par, bool /*full*/)
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

    std::pair<ChromaType, Status> chromaTypeRes = GetChromaType();
    if (chromaTypeRes.second != UMC_OK)
    {
        return chromaTypeRes.second;
    }
    par->mfx.JPEGChromaFormat = GetMFXChromaFormat(chromaTypeRes.first);

    std::pair<JCOLOR, Status> colorTypeRes = GetColorType();
    if (colorTypeRes.second != UMC_OK)
    {
        return colorTypeRes.second;
    }
    par->mfx.JPEGColorFormat = GetMFXColorFormat(colorTypeRes.first);
    par->mfx.Rotation  = MFX_ROTATION_0;
    par->mfx.InterleavedDec = (mfxU16)(m_interleavedScan ? MFX_SCANTYPE_INTERLEAVED : MFX_SCANTYPE_NONINTERLEAVED);

    par->mfx.NumThread = 0;

    return UMC_OK;
}

Status MJPEGVideoDecoderBaseMFX::FillQuantTableExtBuf(mfxExtJPEGQuantTables* quantTables)
{
    if (!m_IsInit)
        return UMC_ERR_NOT_INITIALIZED;

    quantTables->NumTable = m_decBase->GetNumQuantTables();
    for(int i=0; i<quantTables->NumTable; i++)
    {
        m_decBase->FillQuantTable(i, quantTables->Qm[i]);
    }

    return UMC_OK;
}

Status MJPEGVideoDecoderBaseMFX::FillHuffmanTableExtBuf(mfxExtJPEGHuffmanTables* huffmanTables)
{
    if (!m_IsInit)
        return UMC_ERR_NOT_INITIALIZED;

    huffmanTables->NumACTable = m_decBase->GetNumACTables();
    for(int i=0; i<huffmanTables->NumACTable; i++)
    {
        m_decBase->FillACTable(i, huffmanTables->ACTables[i].Bits, huffmanTables->ACTables[i].Values);
    }

    huffmanTables->NumDCTable = m_decBase->GetNumDCTables();
    for(int i=0; i<huffmanTables->NumDCTable; i++)
    {
        m_decBase->FillDCTable(i, huffmanTables->DCTables[i].Bits, huffmanTables->DCTables[i].Values);
    }

    return UMC_OK;
}

Status MJPEGVideoDecoderBaseMFX::DecodeHeader(MediaData* in)
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

Status MJPEGVideoDecoderBaseMFX::SetRotation(uint16_t /* rotation */)
{
    m_rotation = 0;
    return UMC_OK;
}

Status MJPEGVideoDecoderBaseMFX::_GetFrameInfo(const uint8_t* pBitStream, size_t nSize)
{
    int32_t   nchannels;
    int32_t   precision;
    JSS      sampling;
    JCOLOR   color;
    JERRCODE jerr;

    if (!m_IsInit)
        return UMC_ERR_NOT_INITIALIZED;

    jerr = m_decBase->SetSource(pBitStream,nSize);
    if(JPEG_OK != jerr)
        return UMC_ERR_FAILED;

    jerr = m_decBase->ReadHeader(
        &m_frameDims.width,&m_frameDims.height,&nchannels,&color,&sampling,&precision);

    if(JPEG_ERR_BUFF == jerr)
        return UMC_ERR_NOT_ENOUGH_DATA;

    if(JPEG_NOT_IMPLEMENTED == jerr)
        return UMC_ERR_NOT_IMPLEMENTED;

    if(JPEG_OK != jerr)
        return UMC_ERR_FAILED;

    m_frameSampling = (int) sampling;
    m_color = color;
    m_interleavedScan = m_decBase->IsInterleavedScan();

    // frame is interleaved if clip height is twice more than JPEG frame height
    if (m_DecoderParams.info.interlace_type != PROGRESSIVE)
    {
        m_interleaved = true;
        m_frameDims.height *= 2;
    }

    return UMC_OK;
}

void MJPEGVideoDecoderBaseMFX::SetFrameAllocator(FrameAllocator * frameAllocator)
{
    VM_ASSERT(frameAllocator);
    m_frameAllocator = frameAllocator;
}

Status MJPEGVideoDecoderBaseMFX::FindStartOfImage(MediaData * in)
{
    JERRCODE jerr;

    if (!m_IsInit)
        return UMC_ERR_NOT_INITIALIZED;

    jerr = m_decBase->SetSource((uint8_t*) in->GetDataPointer(), in->GetDataSize());
    if(JPEG_OK != jerr)
        return UMC_ERR_FAILED;

    jerr = m_decBase->FindSOI();

    if(JPEG_ERR_BUFF == jerr)
        return UMC_ERR_NOT_ENOUGH_DATA;

    if(JPEG_OK != jerr)
        return UMC_ERR_FAILED;

    in->MoveDataPointer(m_decBase->m_BitStreamIn.GetNumUsedBytes());

    return UMC_OK;
}

} // end namespace UMC

#endif // MFX_ENABLE_MJPEG_VIDEO_DECODE
