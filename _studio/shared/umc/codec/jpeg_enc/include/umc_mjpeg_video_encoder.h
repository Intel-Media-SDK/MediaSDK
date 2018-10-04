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


#ifndef __UMC_MJPEG_VIDEO_ENCODER_H
#define __UMC_MJPEG_VIDEO_ENCODER_H

#include "umc_defs.h"
#if defined (MFX_ENABLE_MJPEG_VIDEO_ENCODE)

#include <vector>
#include <memory>
#include <mutex>

#include "umc_defs.h"
#include "umc_structures.h"
#include "umc_video_encoder.h"
#include "umc_video_data.h"

#include "mfxvideo++int.h"

// internal JPEG decoder object forward declaration
class CBaseStreamOutput;
class CJPEGEncoder;

namespace UMC
{

enum
{
    JPEG_ENC_MAX_THREADS_HW = 1,
    JPEG_ENC_MAX_THREADS = 4
};

enum
{
    JPEG_ENC_DEFAULT_NUM_PIECES = JPEG_ENC_MAX_THREADS
};


class MJPEGEncoderScan
{
public:
    MJPEGEncoderScan(): m_numPieces(0) {}
    ~MJPEGEncoderScan() {Close();}

    Status Init(uint32_t numPieces);
    Status Reset(uint32_t numPieces);
    void   Close();

    uint32_t GetNumPieces();

    // Number of restart intervals in scan
    uint32_t m_numPieces;
    // Array of pieces locations (number of BitstreamBuffer, contained the piece)
    std::vector<size_t> m_pieceLocation;
    // Array of pieces offsets
    std::vector<size_t> m_pieceOffset;
    // Array of pieces sizes
    std::vector<size_t> m_pieceSize;
};


class MJPEGEncoderPicture
{
public:
    MJPEGEncoderPicture();
   ~MJPEGEncoderPicture();

    uint32_t GetNumPieces();

    std::unique_ptr<VideoData>     m_sourceData;
    std::vector<MJPEGEncoderScan*> m_scans;

    uint32_t                         m_release_source_data;
};


class MJPEGEncoderFrame
{
public:
    MJPEGEncoderFrame() {Reset();}
   ~MJPEGEncoderFrame() {Reset();}

    void Reset();

    uint32_t GetNumPics();
    uint32_t GetNumPieces();
    Status GetPiecePosition(uint32_t pieceNum, uint32_t* fieldNum, uint32_t* scanNum, uint32_t* piecePosInField, uint32_t* piecePosInScan);

    std::vector<MJPEGEncoderPicture*> m_pics;
};


class MJPEGEncoderParams : public VideoEncoderParams
{
    DYNAMIC_CAST_DECL(MJPEGEncoderParams, VideoEncoderParams)

public:
    MJPEGEncoderParams()
    {
        quality          = 75;  // jpeg frame quality [1,100]
        restart_interval = 0;
        huffman_opt      = 0;
        point_transform  = 0;
        predictor        = 1;
        app0_units       = 0;
        app0_xdensity    = 1;
        app0_ydensity    = 1;
        threading_mode   = 0;
        buf_size         = 0;
        interleaved      = 0;
        chroma_format    = 0;
    }

    int32_t quality;
    int32_t huffman_opt;
    int32_t restart_interval;
    int32_t point_transform;
    int32_t predictor;
    int32_t app0_units;
    int32_t app0_xdensity;
    int32_t app0_ydensity;
    int32_t threading_mode;
    int32_t buf_size;
    int32_t interleaved;
    int32_t chroma_format;
};


class MJPEGVideoEncoder : public VideoEncoder
{
    DYNAMIC_CAST_DECL(MJPEGVideoEncoder, VideoEncoder)

public:
    MJPEGVideoEncoder(void);
    virtual ~MJPEGVideoEncoder(void);

    // Initialize for subsequent frame decoding.
    virtual Status Init(BaseCodecParams* init);

    // Reset decoder to initial state
    virtual Status Reset(void);

    // Close decoding & free all allocated resources
    virtual Status Close(void);

    // Get next frame
    virtual Status GetFrame(MediaData* in, MediaData* out);

    // 
    virtual Status EncodePiece(const mfxU32 threadNumber, const uint32_t numPiece);

    // Get codec working (initialization) parameter(s)
    virtual Status GetInfo(BaseCodecParams *info);

    // Do post processing
    virtual Status PostProcessing(MediaData* out);

    // Get the number of encoders allocated
    uint32_t NumEncodersAllocated(void);

    //
    uint32_t NumPicsCollected(void);

    //
    uint32_t NumPiecesCollected(void);

    //
    Status AddPicture(MJPEGEncoderPicture* pic);

    Status FillQuantTableExtBuf(mfxExtJPEGQuantTables* quantTables);
    Status FillHuffmanTableExtBuf(mfxExtJPEGHuffmanTables* huffmanTables);

    Status SetQuantTableExtBuf(mfxExtJPEGQuantTables* quantTables);
    Status SetHuffmanTableExtBuf(mfxExtJPEGHuffmanTables* huffmanTables);

    Status SetDefaultQuantTable(const mfxU16 quality);
    Status SetDefaultHuffmanTable();

    bool   IsQuantTableInited();
    bool   IsHuffmanTableInited();

protected:

    // JPEG encoders allocated
    std::vector<std::unique_ptr<CJPEGEncoder>> m_enc;
    // Bitstream buffer for each thread
    std::vector<std::unique_ptr<MediaData>>    m_pBitstreamBuffer;
    //
    std::unique_ptr<MJPEGEncoderFrame>  m_frame;

    std::mutex                          m_guard;

    bool                              m_IsInit;

    MJPEGEncoderParams m_EncoderParams;
};

} // end namespace UMC

#endif // MFX_ENABLE_MJPEG_VIDEO_ENCODE
#endif //__UMC_MJPEG_VIDEO_ENCODER_H
