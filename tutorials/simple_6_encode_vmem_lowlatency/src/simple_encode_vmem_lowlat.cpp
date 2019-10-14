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

#include <vector>
#include <unordered_map>
#include <algorithm>
#include <numeric>

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
    // Intel Media SDK encode pipeline setup
    // - In this example we are encoding an AVC (H.264) stream
    // - Video memory surfaces are used
    // - Configure pipeline for low latency
    //

    // Read options from the command line (if any is given)
    memset(&options, 0, sizeof(CmdOptions));
    options.ctx.options = OPTIONS_ENCODE | OPTION_MEASURE_LATENCY;
    options.ctx.usage = usage;
    // Set default values:
    options.values.impl = MFX_IMPL_AUTO_ANY;
    options.values.MeasureLatency = true;

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
    mfxIMPL impl = options.values.impl;
    mfxVersion ver = { {0, 1} };
    MFXVideoSession session;

    mfxFrameAllocator mfxAllocator;

    sts = Initialize(impl, ver, &session, &mfxAllocator);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // Initialize encoder parameters
    // - In this example we are encoding an AVC (H.264) stream
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

    // Configuration for low latency
    mfxEncParams.AsyncDepth = 1;    //1 is best for low latency
    mfxEncParams.mfx.GopRefDist = 1;        //1 is best for low latency, I and P frames only

    mfxExtCodingOption extendedCodingOptions;
    memset(&extendedCodingOptions, 0, sizeof(extendedCodingOptions));
    extendedCodingOptions.Header.BufferId = MFX_EXTBUFF_CODING_OPTION;
    extendedCodingOptions.Header.BufferSz = sizeof(extendedCodingOptions);
    extendedCodingOptions.MaxDecFrameBuffering = 1;
    mfxExtBuffer* extendedBuffers[1];
    extendedBuffers[0] = (mfxExtBuffer*) & extendedCodingOptions;
    mfxEncParams.ExtParam = extendedBuffers;
    mfxEncParams.NumExtParam = 1;
    // ---

    // Create Media SDK encoder
    MFXVideoENCODE mfxENC(session);

    // Validate video encode parameters (optional)
    // - In this example the validation result is written to same structure
    // - MFX_WRN_INCOMPATIBLE_VIDEO_PARAM is returned if some of the video parameters are not supported,
    //   instead the encoder will select suitable parameters closest matching the requested configuration
    sts = mfxENC.Query(&mfxEncParams, &mfxEncParams);
    MSDK_IGNORE_MFX_STS(sts, MFX_WRN_INCOMPATIBLE_VIDEO_PARAM);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // Query number of required surfaces for encoder
    mfxFrameAllocRequest EncRequest;
    memset(&EncRequest, 0, sizeof(EncRequest));
    sts = mfxENC.QueryIOSurf(&mfxEncParams, &EncRequest);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    EncRequest.Type |= WILL_WRITE; // This line is only required for Windows DirectX11 to ensure that surfaces can be written to by the application

    // Allocate required surfaces
    mfxFrameAllocResponse mfxResponse;
    sts = mfxAllocator.Alloc(mfxAllocator.pthis, &EncRequest, &mfxResponse);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    mfxU16 nEncSurfNum = mfxResponse.NumFrameActual;

    // Allocate surface headers (mfxFrameSurface1) for encoder
    mfxFrameSurface1** pmfxSurfaces = new mfxFrameSurface1 *[nEncSurfNum];
    MSDK_CHECK_POINTER(pmfxSurfaces, MFX_ERR_MEMORY_ALLOC);
    for (int i = 0; i < nEncSurfNum; i++) {
        pmfxSurfaces[i] = new mfxFrameSurface1;
        memset(pmfxSurfaces[i], 0, sizeof(mfxFrameSurface1));
        memcpy(&(pmfxSurfaces[i]->Info), &(mfxEncParams.mfx.FrameInfo), sizeof(mfxFrameInfo));
        pmfxSurfaces[i]->Data.MemId = mfxResponse.mids[i];
        if (bEnableInput) {
            ClearYUVSurfaceVMem(pmfxSurfaces[i]->Data.MemId);
        }
    }

    // Initialize the Media SDK encoder
    sts = mfxENC.Init(&mfxEncParams);
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
    // Start encoding the frames
    //

    mfxTime tStart, tEnd;
    mfxGetTime(&tStart);

    std::unordered_map < mfxU64, mfxTime > encInTimeMap;
    std::unordered_map < mfxU64, mfxTime > encOutTimeMap;
    mfxU64 currentTimeStamp = 0; // key value (in hash) to store timestamps

    if (options.values.MeasureLatency) {
        // Record e2e latency for first 1000 frames
        // - Store timing in map (for speed), where unique timestamp value is the key
        encInTimeMap.rehash(1000);
        encOutTimeMap.rehash(1000);
    }

    int nEncSurfIdx = 0;
    mfxSyncPoint syncp;
    mfxU32 nFrame = 0;

    //
    // Stage 1: Main encoding loop
    //
    while (MFX_ERR_NONE <= sts || MFX_ERR_MORE_DATA == sts) {
        nEncSurfIdx = GetFreeSurfaceIndex(pmfxSurfaces, nEncSurfNum);   // Find free frame surface
        MSDK_CHECK_ERROR(MFX_ERR_NOT_FOUND, nEncSurfIdx, MFX_ERR_MEMORY_ALLOC);

        // Surface locking required when read/write video surfaces
        sts = mfxAllocator.Lock(mfxAllocator.pthis, pmfxSurfaces[nEncSurfIdx]->Data.MemId, &(pmfxSurfaces[nEncSurfIdx]->Data));
        MSDK_BREAK_ON_ERROR(sts);

        sts = LoadRawFrame(pmfxSurfaces[nEncSurfIdx], fSource);
        MSDK_BREAK_ON_ERROR(sts);

        sts = mfxAllocator.Unlock(mfxAllocator.pthis, pmfxSurfaces[nEncSurfIdx]->Data.MemId, &(pmfxSurfaces[nEncSurfIdx]->Data));
        MSDK_BREAK_ON_ERROR(sts);

        for (;;) {
            if (options.values.MeasureLatency && (encInTimeMap.size() < 1000)) {
                mfxGetTime(&encInTimeMap[currentTimeStamp]);
                pmfxSurfaces[nEncSurfIdx]->Data.TimeStamp = currentTimeStamp;
            }

            // Encode a frame asychronously (returns immediately)
            sts = mfxENC.EncodeFrameAsync(NULL, pmfxSurfaces[nEncSurfIdx], &mfxBS, &syncp);

            // Since no splitter, artificial timestamp is used
            if (options.values.MeasureLatency && (encInTimeMap.size() < 1000)) {
                currentTimeStamp++;
            }

            if (MFX_ERR_NONE < sts && !syncp) {     // Repeat the call if warning and no output
                if (MFX_WRN_DEVICE_BUSY == sts)
                    MSDK_SLEEP(1);  // Wait if device is busy, then repeat the same call
            } else if (MFX_ERR_NONE < sts && syncp) {
                sts = MFX_ERR_NONE;     // Ignore warnings if output is available
                break;
            } else if (MFX_ERR_NOT_ENOUGH_BUFFER == sts) {
                // Allocate more bitstream buffer memory here if needed...
                break;
            } else
                break;
        }

        if (MFX_ERR_NONE == sts) {
            sts = session.SyncOperation(syncp, 60000);   // Synchronize. Wait until encoded frame is ready
            MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

            if (options.values.MeasureLatency && (encOutTimeMap.size() < 1000)) {
                mfxGetTime(&encOutTimeMap[mfxBS.TimeStamp]);

                /*printf( "finished encoding frame %d Type: %c latency: %f ms\n",
                        (int)mfxBS.TimeStamp,
                        mfxFrameTypeString(mfxBS.FrameType),
                        TimeDiffMsec(encOutTimeMap[mfxBS.TimeStamp], encInTimeMap[mfxBS.TimeStamp])); */
            }

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
    // Stage 2: Retrieve the buffered encoded frames
    //
    while (MFX_ERR_NONE <= sts) {
        for (;;) {
            if (options.values.MeasureLatency && (encInTimeMap.size() < 1000)) {
                mfxGetTime(&encInTimeMap[currentTimeStamp]);
                pmfxSurfaces[nEncSurfIdx]->Data.TimeStamp = currentTimeStamp;
            }

            // Encode a frame asychronously (returns immediately)
            sts = mfxENC.EncodeFrameAsync(NULL, NULL, &mfxBS, &syncp);

            // Since no splitter, artificial timestamp is used
            if (options.values.MeasureLatency && (encInTimeMap.size() < 1000)) {
                currentTimeStamp++;
            }

            if (MFX_ERR_NONE < sts && !syncp) {     // Repeat the call if warning and no output
                if (MFX_WRN_DEVICE_BUSY == sts)
                    MSDK_SLEEP(1);  // Wait if device is busy, then repeat the same call
            } else if (MFX_ERR_NONE < sts && syncp) {
                sts = MFX_ERR_NONE;     // Ignore warnings if output is available
                break;
            } else
                break;
        }

        if (MFX_ERR_NONE == sts) {
            sts = session.SyncOperation(syncp, 60000);   // Synchronize. Wait until encoded frame is ready
            MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

            if (options.values.MeasureLatency && (encOutTimeMap.size() < 1000)) {
                mfxGetTime(&encOutTimeMap[mfxBS.TimeStamp]);

                /*printf("finished encoding frame %d   latency: %f ms\n",
                       (int)mfxBS.TimeStamp,
                       TimeDiffMsec(encOutTimeMap[mfxBS.TimeStamp], encInTimeMap[mfxBS.TimeStamp])); */
            }

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

    if (options.values.MeasureLatency) {
        std::vector < double >frameLatencies;

        // Store all frame latencies in vector
        for (std::unordered_map < mfxU64, mfxTime >::iterator it = encInTimeMap.begin(); it != encInTimeMap.end(); ++it) {
            int tsKey = (*it).first;
            mfxTime startTime = (*it).second;
            mfxTime endTime = encOutTimeMap[tsKey];
            double frameLatency = TimeDiffMsec(endTime, startTime);
            if (frameLatency > 0)
                frameLatencies.push_back(frameLatency);
            //printf("[%4d]: %.2f\n", tsKey, frameLatency);
        }
        // Calculate overall latency metrics
        printf("Latency: AVG=%5.1f ms, MAX=%5.1f ms, MIN=%5.1f ms\n",
               std::accumulate(frameLatencies.begin(), frameLatencies.end(), 0.0) / frameLatencies.size(),
               *std::max_element(frameLatencies.begin(), frameLatencies.end()),
               *std::min_element(frameLatencies.begin(), frameLatencies.end()));
    }

    // ===================================================================
    // Clean up resources
    //  - It is recommended to close Media SDK components first, before releasing allocated surfaces, since
    //    some surfaces may still be locked by internal Media SDK resources.

    mfxENC.Close();
    // session closed automatically on destruction

    for (int i = 0; i < nEncSurfNum; i++)
        delete pmfxSurfaces[i];
    MSDK_SAFE_DELETE_ARRAY(pmfxSurfaces);
    MSDK_SAFE_DELETE_ARRAY(mfxBS.Data);

    mfxAllocator.Free(mfxAllocator.pthis, &mfxResponse);

    if (fSource) fclose(fSource);
    if (fSink) fclose(fSink);

    Release();

    return 0;
}
