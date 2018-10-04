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

#include "mfx_common.h"
#if defined (MFX_ENABLE_MJPEG_VIDEO_ENCODE)

#ifndef _MFX_MPJEG_ENC_ENCODE_H_
#define _MFX_MPJEG_ENC_ENCODE_H_

#include "mfx_common_int.h"

#include "mfxvideo++int.h"
#include "mfxvideo.h"

#include "umc_mjpeg_video_encoder.h"

#include <mutex>
#include <queue>

class MJPEGEncodeTask
{
public:
    // Default constructor
    MJPEGEncodeTask(void);
    // Destructor
   ~MJPEGEncodeTask(void);

    // Initialize the task object
    mfxStatus Initialize(UMC::VideoEncoderParams* params);

    // Add a picture to the task
    mfxStatus AddSource(mfxFrameSurface1* surface, mfxFrameInfo* frameInfo, bool useAuxInput);

    // Calculate number of pieces to split the task between threads
    mfxU32    CalculateNumPieces(mfxFrameSurface1* surface, mfxFrameInfo* frameInfo);

    // Get the number of pictures collected
    mfxU32    NumPicsCollected(void);

    // Get the number of pieces collected
    mfxU32    NumPiecesCollected(void);

    // Reset the task, drop all counters
    void      Reset(void);

    mfxEncodeCtrl    *ctrl;
    mfxFrameSurface1 *surface;
    mfxBitstream     *bs;
    mfxFrameSurface1 auxInput;

    mfxU32           m_initialDataLength;

    std::unique_ptr<UMC::MJPEGVideoEncoder> m_pMJPEGVideoEncoder;

    mfxStatus EncodePiece(const mfxU32 threadNumber);

protected:
    // Close the object, release all resources
    void      Close(void);

    std::mutex m_guard;

    mfxU32 encodedPieces;
};

class MFXVideoENCODEMJPEG : public VideoENCODE
{
public:
    static mfxStatus Query(mfxVideoParam *in, mfxVideoParam *out);
    static mfxStatus QueryIOSurf(mfxVideoParam *par, mfxFrameAllocRequest *request);

    mfxTaskThreadingPolicy GetThreadingPolicy(void) override
    {
        return MFX_TASK_THREADING_INTRA;
    }

    MFXVideoENCODEMJPEG(VideoCORE *core, mfxStatus *sts);
    ~MFXVideoENCODEMJPEG() override;
    mfxStatus Init(mfxVideoParam *par) override;
    mfxStatus Reset(mfxVideoParam *par) override;
    mfxStatus Close(void) override;

    mfxStatus GetVideoParam(mfxVideoParam *par) override;
    mfxStatus GetFrameParam(mfxFrameParam *par) override;
    mfxStatus GetEncodeStat(mfxEncodeStat *stat) override;
    mfxStatus EncodeFrame(mfxEncodeCtrl *ctrl, mfxEncodeInternalParams *pInternalParams, mfxFrameSurface1 *surface, mfxBitstream *bs) override;
    mfxStatus EncodeFrameCheck(mfxEncodeCtrl *ctrl, mfxFrameSurface1 *surface, mfxBitstream *bs, mfxFrameSurface1 **reordered_surface, mfxEncodeInternalParams *pInternalParams) override;
    mfxStatus EncodeFrameCheck(mfxEncodeCtrl *ctrl, mfxFrameSurface1 *surface, mfxBitstream *bs, mfxFrameSurface1 **reordered_surface, mfxEncodeInternalParams *pInternalParams, MFX_ENTRY_POINT *pEntryPoint) override;
    mfxStatus CancelFrame(mfxEncodeCtrl * /*ctrl*/, mfxEncodeInternalParams * /*pInternalParams*/, mfxFrameSurface1 * /*surface*/, mfxBitstream * /*bs*/) override {return MFX_ERR_UNSUPPORTED;}

    mfxFrameSurface1 *GetOriginalSurface(mfxFrameSurface1 *surface);
    mfxStatus RunThread(MJPEGEncodeTask &task, mfxU32 threadNumber, mfxU32 callNumber);

protected:

    VideoCORE *m_core;

    mfxVideoParamWrapper    m_vFirstParam;
    mfxVideoParamWrapper    m_vParam;
    mfxFrameParam           m_mfxFrameParam;

    mfxFrameAllocResponse m_response;
    //mfxFrameAllocResponse m_response_alien;

    // Free tasks queue guard (if SW is used)
    std::mutex m_guard;
    // Free tasks queue (if SW is used)
    std::queue<MJPEGEncodeTask *> m_freeTasks;
    // Count of created tasks (if SW is used)
    mfxU16  m_tasksCount;

    MJPEGEncodeTask *pLastTask;

    std::unique_ptr<UMC::MJPEGEncoderParams> m_pUmcVideoParams;

    mfxU32  m_totalBits;
    mfxU32  m_frameCountSync;   // counter for sync. part
    mfxU32  m_frameCount;       // counter for Async. part
    mfxU32  m_encodedFrames;

    bool m_useAuxInput;
    //bool m_useSysOpaq;
    //bool m_useVideoOpaq;
    bool m_isOpaque;
    bool m_isInitialized;

    mfxExtJPEGQuantTables    m_checkedJpegQT;
    mfxExtJPEGHuffmanTables  m_checkedJpegHT;
    mfxExtOpaqueSurfaceAlloc m_checkedOpaqAllocReq;
    mfxExtBuffer*            m_pCheckedExt[3];

    //
    // Asynchronous processing functions
    //

    static
    mfxStatus MJPEGENCODERoutine(void *pState, void *pParam, mfxU32 threadNumber, mfxU32 callNumber);
    static
    mfxStatus MJPEGENCODECompleteProc(void *pState, void *pParam, mfxStatus taskRes);
};

#endif //_MFX_MPJEG_ENC_ENCODE_H_
#endif // MFX_ENABLE_MJPEG_VIDEO_ENCODE
