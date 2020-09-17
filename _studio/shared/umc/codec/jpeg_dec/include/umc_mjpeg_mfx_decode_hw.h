// Copyright (c) 2020 Intel Corporation
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

#ifndef __UMC_MJPEG_VIDEO_DECODER_MFX_DECODE_HW_H
#define __UMC_MJPEG_VIDEO_DECODER_MFX_DECODE_HW_H

#include "umc_defs.h"
#if defined (MFX_ENABLE_MJPEG_VIDEO_DECODE)

#include "umc_va_base.h"

#if defined(UMC_VA)

#include "umc_mjpeg_mfx_decode_base.h"
#include <set>


 #include <va/va_dec_jpeg.h>

namespace UMC
{
typedef struct tagJPEG_DECODE_SCAN_PARAMETER
{
    uint16_t        NumComponents;
    uint8_t         ComponentSelector[4];
    uint8_t         DcHuffTblSelector[4];
    uint8_t         AcHuffTblSelector[4];
    uint16_t        RestartInterval;
    uint32_t        MCUCount;
    uint16_t        ScanHoriPosition;
    uint16_t        ScanVertPosition;
    uint32_t        DataOffset;
    uint32_t        DataLength;
} JPEG_DECODE_SCAN_PARAMETER;

typedef struct tagJPEG_DECODE_QUERY_STATUS
{
    uint32_t        StatusReportFeedbackNumber;
    uint8_t         bStatus;
    uint8_t         reserved8bits;
    uint16_t        reserved16bits;
} JPEG_DECODE_QUERY_STATUS;

class MJPEGVideoDecoderMFX_HW : public MJPEGVideoDecoderBaseMFX
{
public:
    // Default constructor
    MJPEGVideoDecoderMFX_HW(void);

    // Destructor
    virtual ~MJPEGVideoDecoderMFX_HW(void);

    // Initialize for subsequent frame decoding.
    virtual Status Init(BaseCodecParams* init);

    // Reset decoder to initial state
    virtual Status Reset(void);

    // Close decoding & free all allocated resources
    virtual Status Close(void);

    // Allocate the destination frame
    virtual Status AllocateFrame();

    // Close the frame being decoded
    Status CloseFrame(UMC::FrameData** in, const mfxU32  fieldPos);

    // Get next frame
    virtual Status GetFrame(UMC::MediaDataEx *pSrcData, UMC::FrameData** out, const mfxU32  fieldPos);

    virtual ConvertInfo * GetConvertInfo();

    uint32_t GetStatusReportNumber() {return m_statusReportFeedbackCounter;}

    mfxStatus CheckStatusReportNumber(uint32_t statusReportFeedbackNumber, mfxU16* corrupted);

    void   SetFourCC(uint32_t fourCC) {m_fourCC = fourCC;}

protected:

    Status _DecodeField();

    Status _DecodeHeader(int32_t* nUsedBytes);

    virtual Status _DecodeField(MediaDataEx* in);


    Status PackHeaders(MediaData* src, JPEG_DECODE_SCAN_PARAMETER* obtainedScanParams, uint8_t* buffersForUpdate);
    Status PackPriorityParams();

    Status GetFrameHW(MediaDataEx* in);
    Status DefaultInitializationHuffmantables();

    uint16_t GetNumScans(MediaDataEx* in);

    uint32_t  m_statusReportFeedbackCounter;
    ConvertInfo m_convertInfo;

    uint32_t m_fourCC;

    Mutex m_guard;
    std::set<mfxU32> m_submittedTaskIndex;
    std::set<mfxU32> m_cachedReadyTaskIndex;
    std::set<mfxU32> m_cachedCorruptedTaskIndex;
    VideoAccelerator *      m_va;
};

} // end namespace UMC

#endif //#if defined(UMC_VA)
#endif // MFX_ENABLE_MJPEG_VIDEO_DECODE
#endif //__UMC_MJPEG_VIDEO_DECODER_MFX_DECODE_HW_H
