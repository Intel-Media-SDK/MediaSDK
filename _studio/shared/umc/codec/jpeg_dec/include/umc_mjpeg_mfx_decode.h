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

#ifndef __UMC_MJPEG_VIDEO_DECODER_MFX_DECODE_H
#define __UMC_MJPEG_VIDEO_DECODER_MFX_DECODE_H

#include "umc_defs.h"
#if defined (MFX_ENABLE_MJPEG_VIDEO_DECODE)

#include <memory>

#include "ippj.h"
#include "umc_structures.h"
#include "umc_video_decoder.h"

#include "umc_frame_data.h"
#include "umc_frame_allocator.h"
#include "jpegdec.h"
#include "umc_mjpeg_mfx_decode_base.h"
#include "umc_jpeg_frame_constructor.h"

#include "mfxvideo++int.h"

// internal JPEG decoder object forward declaration
class CBaseStreamInput;
class CJPEGDecoder;
class CJpegTask;
class CJpegTaskBuffer;

namespace UMC
{


enum
{
    JPEG_MAX_THREADS = 4
};

class MJPEGVideoDecoderMFX : public MJPEGVideoDecoderBaseMFX
{
public:
    // Default constructor
    MJPEGVideoDecoderMFX(void);

    // Destructor
    virtual ~MJPEGVideoDecoderMFX(void);

    // Initialize for subsequent frame decoding.
    Status Init(BaseCodecParams* init) override;

    // Reset decoder to initial state
    Status Reset(void) override;

    // Close decoding & free all allocated resources
    Status Close(void) override;

    virtual FrameData *GetDst(void);

    // Get next frame
    virtual Status DecodePicture(const CJpegTask &task, const mfxU32 threadNumber, const mfxU32 callNumber);

    void SetFrameAllocator(FrameAllocator * frameAllocator) override;

    Status DecodeHeader(MediaData* in);

    Status FillVideoParam(mfxVideoParam *par, bool full);

    Status FillQuantTableExtBuf(mfxExtJPEGQuantTables* quantTables);

    Status FillHuffmanTableExtBuf(mfxExtJPEGHuffmanTables* huffmanTables);

    virtual ConvertInfo * GetConvertInfo();

    ChromaType GetChromaType();

    JCOLOR GetColorType();

    // All memory sizes should come in size_t type
    Status _GetFrameInfo(const uint8_t* pBitStream, size_t nSize);

    // Allocate the destination frame
    Status AllocateFrame() override;

    // Close the frame being decoded
    Status CloseFrame(void);

    // Do post processing
    virtual Status PostProcessing(double ptr);

    // Get the number of decoders allocated
    inline
    mfxU32 NumDecodersAllocated(void) const;

    // Skip extra data at the begiging of stream
    Status FindStartOfImage(MediaData * in);

    Status SetRotation(uint16_t rotation);

    Status SetColorSpace(uint16_t chromaFormat, uint16_t colorFormat);

protected:

    void AdjustFrameSize(mfxSize & size) override;

    Status DecodePiece(const mfxU32 fieldNum,
                       const mfxU32 restartNum,
                       const mfxU32 restartsToDecode,
                       const mfxU32 threadNum);

    Status _DecodeHeader(const uint8_t* pBuf, size_t buflen, int32_t* nUsedBytes, const uint32_t threadNum);

    int32_t                  m_frameNo;

    VideoData               m_internalFrame;
    bool                    m_needPostProcessing;


    uint8_t*                  m_frame;
    int32_t                  m_frameChannels;
    int32_t                  m_framePrecision;

    // JPEG decoders allocated
    std::vector<std::unique_ptr<CJPEGDecoder>> m_dec;

    // Pointer to the last buffer decoded. It is required to check if header was already decoded.
    const CJpegTaskBuffer *m_pLastPicBuffer[JPEG_MAX_THREADS];

    double                  m_local_frame_time;
    double                  m_local_delta_frame_time;

    std::unique_ptr<BaseCodec>  m_PostProcessing; // (BaseCodec*) pointer to post processing
};

inline
mfxU32 MJPEGVideoDecoderMFX::NumDecodersAllocated(void) const
{
    return static_cast<mfxU32>(m_dec.size());

} // mfxU32 MJPEGVideoDecoderMFX::NumDecodersAllocated(void) const

} // end namespace UMC

#endif // MFX_ENABLE_MJPEG_VIDEO_DECODE
#endif //__UMC_MJPEG_VIDEO_DECODER_MFX_DECODE_H
