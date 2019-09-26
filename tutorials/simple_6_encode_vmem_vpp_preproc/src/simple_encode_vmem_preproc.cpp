// Copyright (c) 2019 Intel Corporation
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

#include "common_utils.h"
#include "cmd_options.h"

static void usage(CmdOptionsCtx* ctx)
{
    printf(
        "Encodes INPUT and optionally writes OUTPUT. If INPUT is not specified\n"
        "simulates input with empty frames filled with the color.\n"
        "\n"
        "Usage: %s [options] [INPUT] [OUTPUT]\n", ctx->program);
}

int main(int argc, char** argv)
{
    mfxStatus sts = MFX_ERR_NONE;
    bool bEnableInput;  // if true, removes all YUV file reading (which is replaced by pre-initialized surface data). Workload runs for 1000 frames.
    bool bEnableOutput; // if true, removes all output bitsteam file writing and printing the progress
    CmdOptions options;

    // =====================================================================
    // Intel Media SDK VPP and encode pipeline setup.
    // - Showcasing RGB32 color conversion to NV12 via VPP then encode
    // - In this example we are encoding an AVC (H.264) stream
    // - Video memory surfaces are used
    //

    // Read options from the command line (if any is given)
    memset(&options, 0, sizeof(CmdOptions));
    options.ctx.options = OPTIONS_ENCODE;
    options.ctx.usage = usage;
    // Set default values:
    options.values.impl = MFX_IMPL_AUTO_ANY;

    // here we parse options
    ParseOptions(argc, argv, &options);

    if (!options.values.Width || !options.values.Height) {
        printf("error: input video geometry not set (mandatory)\n");
        return -1;
    }
    if (!options.values.Bitrate) {
        printf("error: bitrate not set (mandatory)\n");
        return -1;
    }
    if (!options.values.FrameRateN || !options.values.FrameRateD) {
        printf("error: framerate not set (mandatory)\n");
        return -1;
    }

    bEnableInput = (options.values.SourceName[0] != '\0');
    bEnableOutput = (options.values.SinkName[0] != '\0');
    // Open input YV12 YUV file
    FILE* fSource = NULL;
    if (bEnableInput) {
        MSDK_FOPEN(fSource, options.values.SourceName, "rb");
        MSDK_CHECK_POINTER(fSource, MFX_ERR_NULL_PTR);
    }

    // Create output elementary stream (ES) H.264 file
    FILE* fSink = NULL;
    if (bEnableOutput) {
        MSDK_FOPEN(fSink, options.values.SinkName, "wb");
        MSDK_CHECK_POINTER(fSink, MFX_ERR_NULL_PTR);
    }

    // Initialize Intel Media SDK session
    // - MFX_IMPL_AUTO_ANY selects HW acceleration if available (on any adapter)
    // - Version 1.0 is selected for greatest backwards compatibility.
    //   If more recent API features are needed, change the version accordingly
    mfxIMPL impl = options.values.impl;
    mfxVersion ver = { {0, 1} };
    MFXVideoSession session;

    mfxFrameAllocator mfxAllocator;

    sts = Initialize(impl, ver, &session, &mfxAllocator);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // Initialize encoder parameters
    mfxVideoParam mfxEncParams;
    memset(&mfxEncParams, 0, sizeof(mfxEncParams));
    mfxEncParams.mfx.CodecId = MFX_CODEC_AVC;
    mfxEncParams.mfx.TargetUsage = MFX_TARGETUSAGE_BALANCED;
    mfxEncParams.mfx.TargetKbps = options.values.Bitrate;
    mfxEncParams.mfx.RateControlMethod = MFX_RATECONTROL_VBR;
    mfxEncParams.mfx.FrameInfo.FrameRateExtN = options.values.FrameRateN;
    mfxEncParams.mfx.FrameInfo.FrameRateExtD = options.values.FrameRateD;
    mfxEncParams.mfx.FrameInfo.FourCC = MFX_FOURCC_NV12;
    mfxEncParams.mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
    mfxEncParams.mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
    mfxEncParams.mfx.FrameInfo.CropX = 0;
    mfxEncParams.mfx.FrameInfo.CropY = 0;
    mfxEncParams.mfx.FrameInfo.CropW = options.values.Width;
    mfxEncParams.mfx.FrameInfo.CropH = options.values.Height;
    // Width must be a multiple of 16
    // Height must be a multiple of 16 in case of frame picture and a multiple of 32 in case of field picture
    mfxEncParams.mfx.FrameInfo.Width = MSDK_ALIGN16(options.values.Width);
    mfxEncParams.mfx.FrameInfo.Height =
        (MFX_PICSTRUCT_PROGRESSIVE == mfxEncParams.mfx.FrameInfo.PicStruct) ?
        MSDK_ALIGN16(options.values.Height) :
        MSDK_ALIGN32(options.values.Height);

    mfxEncParams.IOPattern = MFX_IOPATTERN_IN_VIDEO_MEMORY;

    // Initialize VPP parameters
    mfxVideoParam VPPParams;
    memset(&VPPParams, 0, sizeof(VPPParams));
    // Input data
    VPPParams.vpp.In.FourCC = MFX_FOURCC_RGB4;
    VPPParams.vpp.In.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
    VPPParams.vpp.In.CropX = 0;
    VPPParams.vpp.In.CropY = 0;
    VPPParams.vpp.In.CropW = options.values.Width;
    VPPParams.vpp.In.CropH = options.values.Height;
    VPPParams.vpp.In.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
    VPPParams.vpp.In.FrameRateExtN = options.values.FrameRateN;
    VPPParams.vpp.In.FrameRateExtD = options.values.FrameRateD;
    // width must be a multiple of 16
    // height must be a multiple of 16 in case of frame picture and a multiple of 32 in case of field picture
    VPPParams.vpp.In.Width = MSDK_ALIGN16(options.values.Width);
    VPPParams.vpp.In.Height =
        (MFX_PICSTRUCT_PROGRESSIVE == VPPParams.vpp.In.PicStruct) ?
        MSDK_ALIGN16(options.values.Height) :
        MSDK_ALIGN32(options.values.Height);
    // Output data
    VPPParams.vpp.Out.FourCC = MFX_FOURCC_NV12;
    VPPParams.vpp.Out.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
    VPPParams.vpp.Out.CropX = 0;
    VPPParams.vpp.Out.CropY = 0;
    VPPParams.vpp.Out.CropW = options.values.Width;
    VPPParams.vpp.Out.CropH = options.values.Height;
    VPPParams.vpp.Out.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
    VPPParams.vpp.Out.FrameRateExtN = options.values.FrameRateN;
    VPPParams.vpp.Out.FrameRateExtD = options.values.FrameRateD;
    // width must be a multiple of 16
    // height must be a multiple of 16 in case of frame picture and a multiple of 32 in case of field picture
    VPPParams.vpp.Out.Width = MSDK_ALIGN16(VPPParams.vpp.Out.CropW);
    VPPParams.vpp.Out.Height =
        (MFX_PICSTRUCT_PROGRESSIVE == VPPParams.vpp.Out.PicStruct) ?
        MSDK_ALIGN16(VPPParams.vpp.Out.CropH) :
        MSDK_ALIGN32(VPPParams.vpp.Out.CropH);

    VPPParams.IOPattern = MFX_IOPATTERN_IN_VIDEO_MEMORY | MFX_IOPATTERN_OUT_VIDEO_MEMORY;

    // Create Media SDK encoder
    MFXVideoENCODE mfxENC(session);
    // Create Media SDK VPP component
    MFXVideoVPP mfxVPP(session);

    // Query number of required surfaces for encoder
    mfxFrameAllocRequest EncRequest;
    memset(&EncRequest, 0, sizeof(EncRequest));
    sts = mfxENC.QueryIOSurf(&mfxEncParams, &EncRequest);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // Query number of required surfaces for VPP
    mfxFrameAllocRequest VPPRequest[2];     // [0] - in, [1] - out
    memset(&VPPRequest, 0, sizeof(mfxFrameAllocRequest) * 2);
    sts = mfxVPP.QueryIOSurf(&VPPParams, VPPRequest);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    EncRequest.Type |= MFX_MEMTYPE_FROM_VPPOUT;     // surfaces are shared between VPP output and encode input

    // Determine the required number of surfaces for VPP input and for VPP output (encoder input)
    mfxU16 nSurfNumVPPIn = VPPRequest[0].NumFrameSuggested;
    mfxU16 nSurfNumVPPOutEnc = EncRequest.NumFrameSuggested + VPPRequest[1].NumFrameSuggested;

    EncRequest.NumFrameSuggested = nSurfNumVPPOutEnc;

    VPPRequest[0].Type |= WILL_WRITE; // This line is only required for Windows DirectX11 to ensure that surfaces can be written to by the application

    // Allocate required surfaces
    mfxFrameAllocResponse mfxResponseVPPIn;
    mfxFrameAllocResponse mfxResponseVPPOutEnc;
    sts = mfxAllocator.Alloc(mfxAllocator.pthis, &VPPRequest[0], &mfxResponseVPPIn);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
    sts = mfxAllocator.Alloc(mfxAllocator.pthis, &EncRequest, &mfxResponseVPPOutEnc);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // Allocate surface headers (mfxFrameSurface1) for VPPIn
    mfxFrameSurface1** pmfxSurfacesVPPIn = new mfxFrameSurface1 *[nSurfNumVPPIn];
    MSDK_CHECK_POINTER(pmfxSurfacesVPPIn, MFX_ERR_MEMORY_ALLOC);
    for (int i = 0; i < nSurfNumVPPIn; i++) {
        pmfxSurfacesVPPIn[i] = new mfxFrameSurface1;
        memset(pmfxSurfacesVPPIn[i], 0, sizeof(mfxFrameSurface1));
        memcpy(&(pmfxSurfacesVPPIn[i]->Info), &(VPPParams.vpp.In), sizeof(mfxFrameInfo));
        pmfxSurfacesVPPIn[i]->Data.MemId = mfxResponseVPPIn.mids[i];
        if (bEnableInput) {
            ClearRGBSurfaceVMem(pmfxSurfacesVPPIn[i]->Data.MemId);
        }
    }

    mfxFrameSurface1** pVPPSurfacesVPPOutEnc = new mfxFrameSurface1 *[nSurfNumVPPOutEnc];
    MSDK_CHECK_POINTER(pVPPSurfacesVPPOutEnc, MFX_ERR_MEMORY_ALLOC);
    for (int i = 0; i < nSurfNumVPPOutEnc; i++) {
        pVPPSurfacesVPPOutEnc[i] = new mfxFrameSurface1;
        memset(pVPPSurfacesVPPOutEnc[i], 0, sizeof(mfxFrameSurface1));
        memcpy(&(pVPPSurfacesVPPOutEnc[i]->Info), &(VPPParams.vpp.Out), sizeof(mfxFrameInfo));
        pVPPSurfacesVPPOutEnc[i]->Data.MemId = mfxResponseVPPOutEnc.mids[i];
    }

    // Disable default VPP operations
    mfxExtVPPDoNotUse extDoNotUse;
    memset(&extDoNotUse, 0, sizeof(mfxExtVPPDoNotUse));
    extDoNotUse.Header.BufferId = MFX_EXTBUFF_VPP_DONOTUSE;
    extDoNotUse.Header.BufferSz = sizeof(mfxExtVPPDoNotUse);
    extDoNotUse.NumAlg = 4;
    extDoNotUse.AlgList = new mfxU32[extDoNotUse.NumAlg];
    MSDK_CHECK_POINTER(extDoNotUse.AlgList, MFX_ERR_MEMORY_ALLOC);
    extDoNotUse.AlgList[0] = MFX_EXTBUFF_VPP_DENOISE;       // turn off denoising (on by default)
    extDoNotUse.AlgList[1] = MFX_EXTBUFF_VPP_SCENE_ANALYSIS;        // turn off scene analysis (on by default)
    extDoNotUse.AlgList[2] = MFX_EXTBUFF_VPP_DETAIL;        // turn off detail enhancement (on by default)
    extDoNotUse.AlgList[3] = MFX_EXTBUFF_VPP_PROCAMP;       // turn off processing amplified (on by default)

    // Add extended VPP buffers
    mfxExtBuffer* extBuffers[1];
    extBuffers[0] = (mfxExtBuffer*) & extDoNotUse;
    VPPParams.ExtParam = extBuffers;
    VPPParams.NumExtParam = 1;

    // Initialize the Media SDK encoder
    sts = mfxENC.Init(&mfxEncParams);
    MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // Initialize Media SDK VPP
    sts = mfxVPP.Init(&VPPParams);
    MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // Retrieve video parameters selected by encoder.
    // - BufferSizeInKB parameter is required to set bit stream buffer size
    mfxVideoParam par;
    memset(&par, 0, sizeof(par));
    sts = mfxENC.GetVideoParam(&par);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // Prepare Media SDK bit stream buffer
    mfxBitstream mfxBS;
    memset(&mfxBS, 0, sizeof(mfxBS));
    mfxBS.MaxLength = par.mfx.BufferSizeInKB * 1000;
    mfxBS.Data = new mfxU8[mfxBS.MaxLength];
    MSDK_CHECK_POINTER(mfxBS.Data, MFX_ERR_MEMORY_ALLOC);

    // ===================================
    // Start processing frames
    //

    mfxTime tStart, tEnd;
    mfxGetTime(&tStart);

    int nEncSurfIdx = 0;
    int nVPPSurfIdx = 0;
    mfxSyncPoint syncpVPP, syncpEnc;
    mfxU32 nFrame = 0;

    //
    // Stage 1: Main VPP/encoding loop
    //
    while (MFX_ERR_NONE <= sts || MFX_ERR_MORE_DATA == sts) {
        nVPPSurfIdx = GetFreeSurfaceIndex(pmfxSurfacesVPPIn, nSurfNumVPPIn);    // Find free input frame surface
        MSDK_CHECK_ERROR(MFX_ERR_NOT_FOUND, nVPPSurfIdx, MFX_ERR_MEMORY_ALLOC);

        // Surface locking required when read/write video surfaces
        sts = mfxAllocator.Lock(mfxAllocator.pthis, pmfxSurfacesVPPIn[nVPPSurfIdx]->Data.MemId, &(pmfxSurfacesVPPIn[nVPPSurfIdx]->Data));
        MSDK_BREAK_ON_ERROR(sts);

        sts = LoadRawRGBFrame(pmfxSurfacesVPPIn[nVPPSurfIdx], fSource);  // Load frame from file into surface
        MSDK_BREAK_ON_ERROR(sts);

        sts = mfxAllocator.Unlock(mfxAllocator.pthis, pmfxSurfacesVPPIn[nVPPSurfIdx]->Data.MemId, &(pmfxSurfacesVPPIn[nVPPSurfIdx]->Data));
        MSDK_BREAK_ON_ERROR(sts);

        nEncSurfIdx = GetFreeSurfaceIndex(pVPPSurfacesVPPOutEnc, nSurfNumVPPOutEnc);    // Find free output frame surface
        MSDK_CHECK_ERROR(MFX_ERR_NOT_FOUND, nEncSurfIdx, MFX_ERR_MEMORY_ALLOC);

        for (;;) {
            // Process a frame asychronously (returns immediately)
            sts = mfxVPP.RunFrameVPPAsync(pmfxSurfacesVPPIn[nVPPSurfIdx], pVPPSurfacesVPPOutEnc[nEncSurfIdx], NULL, &syncpVPP);
            if (MFX_WRN_DEVICE_BUSY == sts) {
                MSDK_SLEEP(1);  // Wait if device is busy, then repeat the same call
            } else
                break;
        }

        if (MFX_ERR_MORE_DATA == sts)
            continue;

        // MFX_ERR_MORE_SURFACE means output is ready but need more surface (example: Frame Rate Conversion 30->60)
        // * Not handled in this example!

        MSDK_BREAK_ON_ERROR(sts);

        for (;;) {
            // Encode a frame asychronously (returns immediately)
            sts = mfxENC.EncodeFrameAsync(NULL, pVPPSurfacesVPPOutEnc[nEncSurfIdx], &mfxBS, &syncpEnc);

            if (MFX_ERR_NONE < sts && !syncpEnc) {  // Repeat the call if warning and no output
                if (MFX_WRN_DEVICE_BUSY == sts)
                    MSDK_SLEEP(1);  // Wait if device is busy, then repeat the same call
            } else if (MFX_ERR_NONE < sts && syncpEnc) {
                sts = MFX_ERR_NONE;     // Ignore warnings if output is available
                break;
            } else if (MFX_ERR_NOT_ENOUGH_BUFFER == sts) {
                // Allocate more bitstream buffer memory here if needed...
                break;
            } else
                break;
        }

        if (MFX_ERR_NONE == sts) {
            sts = session.SyncOperation(syncpEnc, 60000);   // Synchronize. Wait until encoded frame is ready
            MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

            sts = WriteBitStreamFrame(&mfxBS, fSink);
            MSDK_BREAK_ON_ERROR(sts);

            ++nFrame;
            if (bEnableOutput) {
                printf("Frame number: %d\r", nFrame);
            }
        }
    }

    // MFX_ERR_MORE_DATA means that the input file has ended, need to go to buffering loop, exit in case of other errors
    MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_DATA);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    //
    // Stage 2: Retrieve the buffered VPP frames
    //
    while (MFX_ERR_NONE <= sts) {
        nEncSurfIdx = GetFreeSurfaceIndex(pVPPSurfacesVPPOutEnc, nSurfNumVPPOutEnc);    // Find free output frame surface
        MSDK_CHECK_ERROR(MFX_ERR_NOT_FOUND, nEncSurfIdx, MFX_ERR_MEMORY_ALLOC);

        for (;;) {
            // Process a frame asychronously (returns immediately)
            sts = mfxVPP.RunFrameVPPAsync(NULL, pVPPSurfacesVPPOutEnc[nEncSurfIdx], NULL, &syncpVPP);
            if (MFX_WRN_DEVICE_BUSY == sts) {
                MSDK_SLEEP(1);  // Wait if device is busy, then repeat the same call
            } else
                break;
        }

        MSDK_BREAK_ON_ERROR(sts);

        for (;;) {
            // Encode a frame asychronously (returns immediately)
            sts = mfxENC.EncodeFrameAsync(NULL, pVPPSurfacesVPPOutEnc[nEncSurfIdx], &mfxBS, &syncpEnc);

            if (MFX_ERR_NONE < sts && !syncpEnc) {  // Repeat the call if warning and no output
                if (MFX_WRN_DEVICE_BUSY == sts)
                    MSDK_SLEEP(1);  // Wait if device is busy, then repeat the same call
            } else if (MFX_ERR_NONE < sts && syncpEnc) {
                sts = MFX_ERR_NONE;     // Ignore warnings if output is available
                break;
            } else if (MFX_ERR_NOT_ENOUGH_BUFFER == sts) {
                // Allocate more bitstream buffer memory here if needed...
                break;
            } else
                break;
        }

        if (MFX_ERR_NONE == sts) {
            sts = session.SyncOperation(syncpEnc, 60000);   // Synchronize. Wait until encoded frame is ready
            MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

            sts = WriteBitStreamFrame(&mfxBS, fSink);
            MSDK_BREAK_ON_ERROR(sts);

            ++nFrame;
            if (bEnableOutput) {
                printf("Frame number: %d\r", nFrame);
            }
        }
    }

    // MFX_ERR_MORE_DATA indicates that there are no more buffered frames, exit in case of other errors
    MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_DATA);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    //
    // Stage 3: Retrieve the buffered encoder frames
    //
    while (MFX_ERR_NONE <= sts) {
        for (;;) {
            // Encode a frame asychronously (returns immediately)
            sts = mfxENC.EncodeFrameAsync(NULL, NULL, &mfxBS, &syncpEnc);

            if (MFX_ERR_NONE < sts && !syncpEnc) {  // Repeat the call if warning and no output
                if (MFX_WRN_DEVICE_BUSY == sts)
                    MSDK_SLEEP(1);  // Wait if device is busy, then repeat the same call
            } else if (MFX_ERR_NONE < sts && syncpEnc) {
                sts = MFX_ERR_NONE;     // Ignore warnings if output is available
                break;
            } else
                break;
        }

        if (MFX_ERR_NONE == sts) {
            sts = session.SyncOperation(syncpEnc, 60000);   // Synchronize. Wait until encoded frame is ready
            MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

            sts = WriteBitStreamFrame(&mfxBS, fSink);
            MSDK_BREAK_ON_ERROR(sts);

            ++nFrame;
            if (bEnableOutput) {
                printf("Frame number: %d\r", nFrame);
            }
        }
    }

    // MFX_ERR_MORE_DATA indicates that there are no more buffered frames, exit in case of other errors
    MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_DATA);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    mfxGetTime(&tEnd);
    double elapsed = TimeDiffMsec(tEnd, tStart) / 1000;
    double fps = ((double)nFrame / elapsed);
    printf("\nExecution time: %3.2f s (%3.2f fps)\n", elapsed, fps);

    // ===================================================================
    // Clean up resources
    //  - It is recommended to close Media SDK components first, before releasing allocated surfaces, since
    //    some surfaces may still be locked by internal Media SDK resources.

    mfxENC.Close();
    mfxVPP.Close();
    // session closed automatically on destruction

    for (int i = 0; i < nSurfNumVPPIn; i++)
        delete pmfxSurfacesVPPIn[i];
    MSDK_SAFE_DELETE_ARRAY(pmfxSurfacesVPPIn);
    for (int i = 0; i < nSurfNumVPPOutEnc; i++)
        delete pVPPSurfacesVPPOutEnc[i];
    MSDK_SAFE_DELETE_ARRAY(pVPPSurfacesVPPOutEnc);
    MSDK_SAFE_DELETE_ARRAY(mfxBS.Data);
    MSDK_SAFE_DELETE_ARRAY(extDoNotUse.AlgList);

    mfxAllocator.Free(mfxAllocator.pthis, &mfxResponseVPPIn);
    mfxAllocator.Free(mfxAllocator.pthis, &mfxResponseVPPOutEnc);

    if (fSource) fclose(fSource);
    if (fSink) fclose(fSink);

    Release();

    return 0;
}
