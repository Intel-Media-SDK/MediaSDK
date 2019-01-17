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

#ifndef _MFX_MJPEG_TASK_H_
#define _MFX_MJPEG_TASK_H_

#include "mfx_common.h"
#include "umc_defs.h"
#if defined (MFX_ENABLE_MJPEG_VIDEO_DECODE)

#include <vector>
#include <memory>

class CJpegTaskBuffer
{
public:
    // Default construct
    CJpegTaskBuffer(void);
    // Destructor
    ~CJpegTaskBuffer(void);

    CJpegTaskBuffer(const CJpegTaskBuffer&) = delete;
    CJpegTaskBuffer(CJpegTaskBuffer&&) = delete;
    CJpegTaskBuffer& operator=(const CJpegTaskBuffer&) = delete;
    CJpegTaskBuffer& operator=(CJpegTaskBuffer&&) = delete;

    // Allocate the buffer
    mfxStatus Allocate(const size_t size);

    // Pointer to the buffer
    mfxU8* pBuf;
    // Buffer size
    size_t bufSize;
    // Data size
    size_t dataSize;

    // Size of header data
    size_t imageHeaderSize;
    // Array of pieces offsets
    std::vector<size_t> pieceOffset;
    // Array of pieces sizes
    std::vector<size_t> pieceSize;
    // Count of RST markers prior current piece
    std::vector<size_t> pieceRSTOffset;

    // Array of SOS segments (offsets)
    std::vector<size_t> scanOffset;
    // Array of SOS segments (sizes)
    std::vector<size_t> scanSize;
    // Array of tables before SOS (offsets)
    std::vector<size_t> scanTablesOffset;
    // Array of tables before SOS (sizes)
    std::vector<size_t> scanTablesSize;

    // Picture's time stamp
    double timeStamp;

    // Number of scans in the image
    mfxU32 numScans;

    // Number of independent pieces in the image
    mfxU32 numPieces;

    // The field position in interlaced case
    mfxU32 fieldPos;

protected:
    // Close the object
    void Close(void);
};

// Forward declaration of used classes
namespace UMC
{
class FrameData;
class MJPEGVideoDecoderMFX;
class MediaDataEx;
class VideoDecoderParams;
class FrameAllocator;

} // namespace UMC

class CJpegTask
{
public:
    // Default constructor
    CJpegTask(void);
    // Destructor
    ~CJpegTask(void);

    // Initialize the task object
    mfxStatus Initialize(UMC::VideoDecoderParams &params,
                         UMC::FrameAllocator *pFrameAllocator,
                         mfxU16 rotation,
                         mfxU16 chromaFormat,
                         mfxU16 colorFormat);

    // Reset the task, drop all counters
    void Reset(void);

    // Add a picture to the task
    mfxStatus AddPicture(UMC::MediaDataEx *pSrcData, const mfxU32  fieldPos);

    // Get the number of pictures collected
    inline
    mfxU32 NumPicCollected(void) const;

    // Get the number of pieces collected
    inline
    mfxU32 NumPiecesCollected(void) const;

    // Get the picture's buffer
    inline
    const CJpegTaskBuffer &GetPictureBuffer(mfxU32 picNum) const
    {
        return *(m_pics[picNum]);
    }

    // tasks parameters
    UMC::FrameData *dst;
    mfxFrameSurface1 *surface_work;
    mfxFrameSurface1 *surface_out;

    // Decoder's array
    std::unique_ptr<UMC::MJPEGVideoDecoderMFX> m_pMJPEGVideoDecoder;

protected:
    // Close the object, release all resources
    void Close(void);

    // Make sure that the buffer is big enough to collect the source
    mfxStatus CheckBufferSize(const size_t srcSize);

    // Number of pictures collected
    mfxU32 m_numPic;
    // Array with collected pictures
    std::vector<std::unique_ptr<CJpegTaskBuffer>> m_pics;

    // Number of scan data pieces. This number is used to request the number of
    // threads.
    mfxU32 m_numPieces;

};

//
// Inline members
//

inline
mfxU32 CJpegTask::NumPicCollected(void) const
{
    return m_numPic;

} // mfxU32 CJpegTask::NumPicCollected(void) const

inline
mfxU32 CJpegTask::NumPiecesCollected(void) const
{
    return m_numPieces;

} // mfxU32 CJpegTask::NumPiecesCollected(void) const

#endif // MFX_ENABLE_MJPEG_VIDEO_DECODE
#endif // _MFX_MJPEG_TASK_H_
