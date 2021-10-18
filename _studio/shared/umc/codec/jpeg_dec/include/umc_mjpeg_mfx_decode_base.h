// Copyright (c) 2017 Intel Corporation
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

#ifndef __UMC_MJPEG_VIDEO_DECODER_MFX_DECODE_BASE_H
#define __UMC_MJPEG_VIDEO_DECODER_MFX_DECODE_BASE_H

#include "umc_defs.h"
#if defined (MFX_ENABLE_MJPEG_VIDEO_DECODE)

#include <memory>

#include "umc_structures.h"
#include "umc_video_decoder.h"
#include "umc_media_data_ex.h"
#include "umc_frame_data.h"
#include "umc_frame_allocator.h"
#include "jpegdec_base.h"

#include "mfxvideo++int.h"

namespace UMC
{

typedef struct
{
    ChromaType colorFormat;
    size_t UOffset;
    size_t VOffset;
} ConvertInfo;

class MJPEGVideoDecoderBaseMFX
{
public:
    // Default constructor
    MJPEGVideoDecoderBaseMFX(void);

    // Destructor
    virtual ~MJPEGVideoDecoderBaseMFX(void);

    // Initialize for subsequent frame decoding.
    virtual Status Init(BaseCodecParams* init);

    // Reset decoder to initial state
    virtual Status Reset(void);

    // Close decoding & free all allocated resources
    virtual Status Close(void);

    virtual Status GetFrame(UMC::MediaDataEx *, UMC::FrameData** , const mfxU32  ) { return MFX_ERR_NONE; };

    virtual void SetFrameAllocator(FrameAllocator * frameAllocator);

    Status FillQuantTableExtBuf(mfxExtJPEGQuantTables* quantTables);

    Status FillHuffmanTableExtBuf(mfxExtJPEGHuffmanTables* huffmanTables);

    Status DecodeHeader(MediaData* in);

    Status FillVideoParam(mfxVideoParam *par, bool full);

    // Skip extra data at the begiging of stream
    Status FindStartOfImage(MediaData * in);

    // All memory sizes should come in size_t type
    Status _GetFrameInfo(const uint8_t* pBitStream, size_t nSize);

    Status SetRotation(uint16_t rotation);

protected:

    std::pair<JCOLOR, Status> GetColorType();
    std::pair<ChromaType, Status> GetChromaType();
    virtual void AdjustFrameSize(mfxSize & size);
    // Allocate the destination frame
    virtual Status AllocateFrame() { return MFX_ERR_NONE; };

    bool                    m_IsInit;
    bool                    m_interleaved;
    bool                    m_interleavedScan;
    VideoDecoderParams      m_DecoderParams;

    FrameData               m_frameData;
    JCOLOR                  m_color;
    uint16_t                m_rotation;

    mfxSize                 m_frameDims;
    int                     m_frameSampling;

    // JPEG decoders allocated
    std::unique_ptr<CJPEGDecoderBase> m_decoder;
    CJPEGDecoderBase * m_decBase;

    FrameAllocator *        m_frameAllocator;
};

inline mfxU16 GetMFXChromaFormat(ChromaType type)
{
    mfxU16 chromaFormat = MFX_CHROMAFORMAT_MONOCHROME;
    switch(type)
    {
    case CHROMA_TYPE_YUV400:
        chromaFormat = MFX_CHROMAFORMAT_MONOCHROME;
        break;
    case CHROMA_TYPE_YUV420:
        chromaFormat = MFX_CHROMAFORMAT_YUV420;
        break;
    case CHROMA_TYPE_YUV422H_2Y:
        chromaFormat = MFX_CHROMAFORMAT_YUV422H;
        break;
    case CHROMA_TYPE_YUV444:
    case CHROMA_TYPE_RGB:
        chromaFormat = MFX_CHROMAFORMAT_YUV444;
        break;
    case CHROMA_TYPE_YUV411:
        chromaFormat = MFX_CHROMAFORMAT_YUV411;
        break;
    case CHROMA_TYPE_YUV422V_2Y:
        chromaFormat = MFX_CHROMAFORMAT_YUV422V;
        break;
    case CHROMA_TYPE_YUV422H_4Y:
        chromaFormat = MFX_CHROMAFORMAT_YUV422H;
        break;
    case CHROMA_TYPE_YUV422V_4Y:
        chromaFormat = MFX_CHROMAFORMAT_YUV422V;
        break;
    default:
        VM_ASSERT(false);
        break;
    };

    return chromaFormat;
}

inline mfxU16 GetMFXColorFormat(JCOLOR color)
{
    mfxU16 colorFormat = MFX_JPEG_COLORFORMAT_UNKNOWN;
    switch(color)
    {
    case JC_UNKNOWN:
        colorFormat = MFX_JPEG_COLORFORMAT_UNKNOWN;
        break;
    case JC_GRAY:
        colorFormat = MFX_JPEG_COLORFORMAT_YCbCr;
        break;
    case JC_YCBCR:
        colorFormat = MFX_JPEG_COLORFORMAT_YCbCr;
        break;
    case JC_RGB:
        colorFormat = MFX_JPEG_COLORFORMAT_RGB;
        break;
    default:
        VM_ASSERT(false);
        break;
    };

    return colorFormat;
}

inline JCOLOR GetUMCColorType(uint16_t chromaFormat, uint16_t colorFormat)
{
    JCOLOR color = JC_UNKNOWN;

    switch(colorFormat)
    {
    case MFX_JPEG_COLORFORMAT_UNKNOWN:
        color = JC_UNKNOWN;
        break;
    case MFX_JPEG_COLORFORMAT_YCbCr:
        if(chromaFormat == MFX_CHROMAFORMAT_MONOCHROME)
        {
            color = JC_GRAY;
        }
        else
        {
            color = JC_YCBCR;
        }
        break;
    case MFX_JPEG_COLORFORMAT_RGB:
        color = JC_RGB;
        break;
    default:
        VM_ASSERT(false);
        break;
    };

    return color;
}

} // end namespace UMC

#endif // MFX_ENABLE_MJPEG_VIDEO_DECODE
#endif //__UMC_MJPEG_VIDEO_DECODER_MFX_DECODE_BASE_H
