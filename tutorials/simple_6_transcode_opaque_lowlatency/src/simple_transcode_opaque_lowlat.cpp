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
        "Transcodes INPUT and optionally writes OUTPUT.\n"
        "\n"
        "Usage: %s [options] INPUT [OUTPUT]\n", ctx->program);
}

// ------------  H.264 bit stream FRAME reader utility -------------
mfxBitstream lastBs = { };

std::vector < mfxU8 > bsBuffer;

int FindSlice(mfxBitstream* pBS, int& pos2ndnalu)
{
    int nNalu = 0;
    size_t i = 0;
    for (i = pBS->DataOffset;
         nNalu < 2 && i + 3 < pBS->DataOffset + pBS->DataLength; i++) {
        if (pBS->Data[i] == 0 && pBS->Data[i + 1] == 0
            && pBS->Data[i + 2] == 1) {
            if (0 == nNalu) {
                int nType = pBS->Data[i + 3] & 0x1F;

                if (nType == 1 ||       //slice
                    nType == 5) //IDR slice
                    nNalu++;
            } else {
                //any backend nalu
                nNalu++;
            }
        }
    }
    if (nNalu == 2) {
        pos2ndnalu = (int)i;
    }
    return nNalu;
}

mfxStatus MoveMfxBitstream(mfxBitstream* pTarget, mfxBitstream* pSrc,
                           mfxU32 nBytes)
{
    MSDK_CHECK_POINTER(pTarget, MFX_ERR_NULL_PTR);
    MSDK_CHECK_POINTER(pSrc, MFX_ERR_NULL_PTR);

    mfxU32 nFreeSpaceTail = pTarget->MaxLength - pTarget->DataOffset - pTarget->DataLength;
    mfxU32 nFreeSpace = pTarget->MaxLength - pTarget->DataLength;

    if (!(pSrc->DataLength >= nBytes))
        return MFX_ERR_MORE_DATA;
    if (!(nFreeSpace >= nBytes))
        return MFX_ERR_NOT_ENOUGH_BUFFER;

    if (nFreeSpaceTail < nBytes) {
        memmove(pTarget->Data, pTarget->Data + pTarget->DataOffset, pTarget->DataLength);
        pTarget->DataOffset = 0;
    }
    memcpy(pTarget->Data + pTarget->DataOffset, pSrc->Data + pSrc->DataOffset, nBytes);
    pTarget->DataLength += nBytes;
    pSrc->DataLength -= nBytes;
    pSrc->DataOffset += nBytes;

    return MFX_ERR_NONE;
}

mfxStatus ReadNextBitStreamFrame(mfxBitstream* pBS, FILE* fSource)
{
    MSDK_CHECK_POINTER(pBS, MFX_ERR_NULL_PTR);
    pBS->DataFlag = MFX_BITSTREAM_COMPLETE_FRAME;

    if (lastBs.Data == NULL) {
        //alloc same bitstream
        bsBuffer.resize(pBS->MaxLength);
        lastBs.Data = &bsBuffer.front();
        lastBs.MaxLength = pBS->MaxLength;
    }
    //checking for available nalu
    int nNalu;
    int pos2ndNaluStart = 0;
    //check nalu in input bs, it always=1 if decoder didnt take a frame
    if ((nNalu = FindSlice(pBS, pos2ndNaluStart)) < 1) {
        //copy nalu from internal buffer
        if ((nNalu = FindSlice(&lastBs, pos2ndNaluStart)) < 2) {
            mfxStatus sts = ReadBitStreamData(&lastBs, fSource);
            if (MFX_ERR_MORE_DATA == sts) {
                //lets feed last nalu if present
                if (nNalu == 1) {
                    sts = MoveMfxBitstream(pBS, &lastBs, lastBs.DataLength);
                    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

                    return MFX_ERR_NONE;
                }

                return MFX_ERR_MORE_DATA;
            }
            //buffer is to small to accept whole frame
            if (!(FindSlice(&lastBs, pos2ndNaluStart) == 2))
                return MFX_ERR_NOT_ENOUGH_BUFFER;
        }
        mfxU32 naluLen = pos2ndNaluStart - lastBs.DataOffset;
        mfxStatus sts = MoveMfxBitstream(pBS, &lastBs, naluLen);
        MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
    }

    return MFX_ERR_NONE;
}

// ---------------------------------------------------------------------------

int main(int argc, char** argv)
{
    mfxStatus sts = MFX_ERR_NONE;
    bool bEnableOutput; // if true, removes all output bitsteam file writing and printing the progress
    CmdOptions options;

    // =====================================================================
    // Intel Media SDK transcode opaque pipeline setup
    // - In this example we are decoding and encoding an AVC (H.264) stream
    // - Opaque transcode pipeline configured for low latency
    // - Simplistic latency measurement technique used
    //

    // Read options from the command line (if any is given)
    memset(&options, 0, sizeof(CmdOptions));
    options.ctx.options = OPTIONS_TRANSCODE | OPTION_MEASURE_LATENCY;
    options.ctx.usage = usage;
    // Set default values:
    options.values.impl = MFX_IMPL_AUTO_ANY;
    options.values.MeasureLatency = true;

    // here we parse options
    ParseOptions(argc, argv, &options);

    if (!options.values.Bitrate) {
        printf("error: bitrate not set (mandatory)\n");
        return -1;
    }
    if (!options.values.FrameRateN || !options.values.FrameRateD) {
        printf("error: framerate not set (mandatory)\n");
        return -1;
    }
    if (!options.values.SourceName[0]) {
        printf("error: source file name not set (mandatory)\n");
        return -1;
    }

    bEnableOutput = (options.values.SinkName[0] != '\0');

    // Open input H.264 elementary stream (ES) file
    FILE* fSource;
    MSDK_FOPEN(fSource, options.values.SourceName, "rb");
    MSDK_CHECK_POINTER(fSource, MFX_ERR_NULL_PTR);

    // Create output elementary stream (ES) H.264 file
    FILE* fSink = NULL;
    if (bEnableOutput) {
        MSDK_FOPEN(fSink, options.values.SinkName, "wb");
        MSDK_CHECK_POINTER(fSink, MFX_ERR_NULL_PTR);
    }

    // Initialize Media SDK session
    // - MFX_IMPL_AUTO_ANY selects HW acceleration if available (on any adapter)
    // - Version 1.3 is selected since the opaque memory feature was added in this API release
    //   If more recent API features are needed, change the version accordingly
    mfxIMPL impl = options.values.impl;
    mfxVersion ver = { {3, 1} };      // Note: API 1.3 !
    MFXVideoSession session;
    sts = Initialize(impl, ver, &session, NULL);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // Create Media SDK decoder & encoder
    MFXVideoDECODE mfxDEC(session);
    MFXVideoENCODE mfxENC(session);

    // Set required video parameters for decode
    mfxVideoParam mfxDecParams;
    memset(&mfxDecParams, 0, sizeof(mfxDecParams));
    mfxDecParams.mfx.CodecId = MFX_CODEC_AVC;
    mfxDecParams.IOPattern = MFX_IOPATTERN_OUT_OPAQUE_MEMORY;

    // Configuration for low latency
    mfxDecParams.AsyncDepth = 1;

    // Prepare Media SDK bit stream buffer for decoder
    // - Arbitrary buffer size for this example
    mfxBitstream mfxBS;
    memset(&mfxBS, 0, sizeof(mfxBS));
    mfxBS.MaxLength = 1024 * 1024;
    mfxBS.Data = new mfxU8[mfxBS.MaxLength];
    MSDK_CHECK_POINTER(mfxBS.Data, MFX_ERR_MEMORY_ALLOC);

    // Read a chunk of data from stream file into bit stream buffer
    // - Parse bit stream, searching for header and fill video parameters structure
    // - Abort if bit stream header is not found in the first bit stream buffer chunk
    sts = ReadNextBitStreamFrame(&mfxBS, fSource);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    sts = mfxDEC.DecodeHeader(&mfxBS, &mfxDecParams);
    MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
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
    mfxEncParams.mfx.FrameInfo.CropW = mfxDecParams.mfx.FrameInfo.CropW;
    mfxEncParams.mfx.FrameInfo.CropH = mfxDecParams.mfx.FrameInfo.CropH;
    // width must be a multiple of 16
    // height must be a multiple of 16 in case of frame picture and a multiple of 32 in case of field picture
    mfxEncParams.mfx.FrameInfo.Width = MSDK_ALIGN16(mfxEncParams.mfx.FrameInfo.CropW);
    mfxEncParams.mfx.FrameInfo.Height =
        (MFX_PICSTRUCT_PROGRESSIVE == mfxEncParams.mfx.FrameInfo.PicStruct) ?
        MSDK_ALIGN16(mfxEncParams.mfx.FrameInfo.CropH) :
        MSDK_ALIGN32(mfxEncParams.mfx.FrameInfo.CropH);

    mfxEncParams.IOPattern = MFX_IOPATTERN_IN_OPAQUE_MEMORY;

    // Configuration for low latency
    mfxEncParams.AsyncDepth = 1;
    mfxEncParams.mfx.GopRefDist = 1;

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

    // Query number required surfaces for decoder
    mfxFrameAllocRequest DecRequest;
    memset(&DecRequest, 0, sizeof(DecRequest));
    sts = mfxDEC.QueryIOSurf(&mfxDecParams, &DecRequest);
    MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // Query number required surfaces for encoder
    mfxFrameAllocRequest EncRequest;
    memset(&EncRequest, 0, sizeof(EncRequest));
    sts = mfxENC.QueryIOSurf(&mfxEncParams, &EncRequest);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    mfxU16 nSurfNum = EncRequest.NumFrameSuggested + DecRequest.NumFrameSuggested;

    // Initialize shared surfaces for decoder and encoder
    // - Note that no buffer memory is allocated, for opaque memory this is handled by Media SDK internally
    // - Frame surface array keeps reference to all surfaces
    // - Opaque memory is configured with the mfxExtOpaqueSurfaceAlloc extended buffers
    mfxFrameSurface1** pSurfaces = new mfxFrameSurface1 *[nSurfNum];
    MSDK_CHECK_POINTER(pSurfaces, MFX_ERR_MEMORY_ALLOC);
    for (int i = 0; i < nSurfNum; i++) {
        pSurfaces[i] = new mfxFrameSurface1;
        MSDK_CHECK_POINTER(pSurfaces[i], MFX_ERR_MEMORY_ALLOC);
        memset(pSurfaces[i], 0, sizeof(mfxFrameSurface1));
        memcpy(&(pSurfaces[i]->Info), &(DecRequest.Info), sizeof(mfxFrameInfo));
    }

    mfxExtOpaqueSurfaceAlloc extOpaqueAllocDec;
    memset(&extOpaqueAllocDec, 0, sizeof(extOpaqueAllocDec));
    extOpaqueAllocDec.Header.BufferId = MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION;
    extOpaqueAllocDec.Header.BufferSz = sizeof(mfxExtOpaqueSurfaceAlloc);
    mfxExtBuffer* pExtParamsDec = (mfxExtBuffer*) & extOpaqueAllocDec;

    mfxExtOpaqueSurfaceAlloc extOpaqueAllocEnc;
    memset(&extOpaqueAllocEnc, 0, sizeof(extOpaqueAllocEnc));
    extOpaqueAllocEnc.Header.BufferId = MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION;
    extOpaqueAllocEnc.Header.BufferSz = sizeof(mfxExtOpaqueSurfaceAlloc);
    mfxExtBuffer* pExtParamsEnc = (mfxExtBuffer*) & extOpaqueAllocEnc;

    extOpaqueAllocDec.Out.Surfaces = pSurfaces;
    extOpaqueAllocDec.Out.NumSurface = nSurfNum;
    extOpaqueAllocDec.Out.Type = DecRequest.Type;
    memcpy(&extOpaqueAllocEnc.In, &extOpaqueAllocDec.Out, sizeof(extOpaqueAllocDec.Out));

    mfxDecParams.ExtParam = &pExtParamsDec;
    mfxDecParams.NumExtParam = 1;
    mfxEncParams.ExtParam = &pExtParamsEnc;
    mfxEncParams.NumExtParam = 1;

    // Initialize the Media SDK decoder
    sts = mfxDEC.Init(&mfxDecParams);
    MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

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

    // Prepare Media SDK bit stream buffer for encoder
    mfxBitstream mfxEncBS;
    memset(&mfxEncBS, 0, sizeof(mfxEncBS));
    mfxEncBS.MaxLength = par.mfx.BufferSizeInKB * 1000;
    mfxEncBS.Data = new mfxU8[mfxEncBS.MaxLength];
    MSDK_CHECK_POINTER(mfxEncBS.Data, MFX_ERR_MEMORY_ALLOC);

    // ===================================
    // Start transcoding the frames
    //

    mfxTime tStart, tEnd;
    mfxGetTime(&tStart);


    // Record e2e latency for first 1000 frames
    // - Store timing in map (for speed), where unique timestamp value is the key
    std::unordered_map < mfxU64, mfxTime > decInTimeMap;
    std::unordered_map < mfxU64, mfxTime > encOutTimeMap;
    mfxU64 currentTimeStamp = 0;    // key value to hashes storing timestamps
    mfxU32 bsInLength = 0;

    if (options.values.MeasureLatency) {
        decInTimeMap.rehash(1000);
        encOutTimeMap.rehash(1000);
    }

    mfxSyncPoint syncpD, syncpE;
    mfxFrameSurface1* pmfxOutSurface = NULL;
    int nIndex = 0;
    mfxU32 nFrame = 0;

    //
    // Stage 1: Main transcoding loop
    //
    while (MFX_ERR_NONE <= sts || MFX_ERR_MORE_DATA == sts || MFX_ERR_MORE_SURFACE == sts) {
        if (MFX_WRN_DEVICE_BUSY == sts)
            MSDK_SLEEP(1);  // just wait and then repeat the same call to DecodeFrameAsync

        if (MFX_ERR_MORE_DATA == sts) {
            sts = ReadNextBitStreamFrame(&mfxBS, fSource);  // Read more data to input bit stream
            MSDK_BREAK_ON_ERROR(sts);
        }

        if (MFX_ERR_MORE_SURFACE == sts || MFX_ERR_NONE == sts) {
            nIndex = GetFreeSurfaceIndex(pSurfaces, nSurfNum);      // Find free frame surface
            MSDK_CHECK_ERROR(MFX_ERR_NOT_FOUND, nIndex, MFX_ERR_MEMORY_ALLOC);
        }
        if (options.values.MeasureLatency && (decInTimeMap.size() < 1000)) {
            mfxBS.TimeStamp = currentTimeStamp;
            mfxGetTime(&decInTimeMap[mfxBS.TimeStamp]);
            bsInLength = mfxBS.DataLength;
        }

        // For low latency
        mfxBS.DataFlag = MFX_BITSTREAM_COMPLETE_FRAME;



        // Decode a frame asychronously (returns immediately)
        sts =
            mfxDEC.DecodeFrameAsync(&mfxBS, pSurfaces[nIndex], &pmfxOutSurface, &syncpD);

        // Since no splitter, artificial timestamp insertion used (TimeStampCalc could also be used)
        if (options.values.MeasureLatency && (decInTimeMap.size() < 1000)) {
            if (MFX_ERR_MORE_SURFACE == sts || MFX_ERR_NONE == sts) {       // check if decode operation was successful

                if (bsInLength != mfxBS.DataLength) {   // check if data was consumed
                    currentTimeStamp++;
                }
            }
        }

        // Ignore warnings if output is available,
        // if no output and no action required just repeat the DecodeFrameAsync call
        if (MFX_ERR_NONE < sts && syncpD)
            sts = MFX_ERR_NONE;

        if (MFX_ERR_NONE == sts) {
            for (;;) {
                // Encode a frame asychronously (returns immediately)
                sts = mfxENC.EncodeFrameAsync(NULL, pmfxOutSurface, &mfxEncBS, &syncpE);

                if (MFX_ERR_NONE < sts && !syncpE) {    // repeat the call if warning and no output
                    if (MFX_WRN_DEVICE_BUSY == sts)
                        MSDK_SLEEP(1);  // wait if device is busy
                } else if (MFX_ERR_NONE < sts && syncpE) {
                    sts = MFX_ERR_NONE;     // ignore warnings if output is available
                    break;
                } else if (MFX_ERR_NOT_ENOUGH_BUFFER == sts) {
                    // Allocate more bitstream buffer memory here if needed...
                    break;
                } else
                    break;
            }

            if (MFX_ERR_MORE_DATA == sts) {
                // MFX_ERR_MORE_DATA indicates encoder need more input, request more surfaces from previous operation
                sts = MFX_ERR_NONE;
                continue;
            }

            if (MFX_ERR_NONE == sts) {
                sts = session.SyncOperation(syncpE, 60000);  // Synchronize. Wait until frame processing is ready
                MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

                if (options.values.MeasureLatency && (encOutTimeMap.size() < 1000)) {
                    mfxGetTime(&encOutTimeMap[mfxEncBS.TimeStamp]);

                    /*printf( "Finished encoding frame %d Type: %c latency: %f ms\n",
                            (int)mfxEncBS.TimeStamp,
                            mfxFrameTypeString(mfxEncBS.FrameType),
                            TimeDiffMsec(encOutTimeMap[mfxEncBS.TimeStamp], decInTimeMap[mfxEncBS.TimeStamp]));*/
                }

                sts = WriteBitStreamFrame(&mfxEncBS, fSink);
                MSDK_BREAK_ON_ERROR(sts);

                ++nFrame;
                if (bEnableOutput)
                    printf("Frame number: %d\r", nFrame);
            }
        }
    }

    // MFX_ERR_MORE_DATA means that file has ended, need to go to buffering loop, exit in case of other errors
    MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_DATA);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    //
    // Stage 2: Retrieve the buffered decoded frames
    //
    while (MFX_ERR_NONE <= sts || MFX_ERR_MORE_SURFACE == sts) {
        if (MFX_WRN_DEVICE_BUSY == sts)
            MSDK_SLEEP(1);

        nIndex = GetFreeSurfaceIndex(pSurfaces, nSurfNum);      // Find free frame surface
        MSDK_CHECK_ERROR(MFX_ERR_NOT_FOUND, nIndex, MFX_ERR_MEMORY_ALLOC);

        // Decode a frame asychronously (returns immediately)
        sts = mfxDEC.DecodeFrameAsync(NULL, pSurfaces[nIndex], &pmfxOutSurface, &syncpD);

        // Ignore warnings if output is available,
        // if no output and no action required just repeat the DecodeFrameAsync call
        if (MFX_ERR_NONE < sts && syncpD)
            sts = MFX_ERR_NONE;

        if (MFX_ERR_NONE == sts) {
            for (;;) {
                // Encode a frame asychronously (returns immediately)
                sts = mfxENC.EncodeFrameAsync(NULL, pmfxOutSurface, &mfxEncBS, &syncpE);

                if (MFX_ERR_NONE < sts && !syncpE) {    // repeat the call if warning and no output
                    if (MFX_WRN_DEVICE_BUSY == sts)
                        MSDK_SLEEP(1);  // wait if device is busy
                } else if (MFX_ERR_NONE < sts && syncpE) {
                    sts = MFX_ERR_NONE;     // ignore warnings if output is available
                    break;
                } else if (MFX_ERR_NOT_ENOUGH_BUFFER == sts) {
                    // Allocate more bitstream buffer memory here if needed...
                    break;
                } else
                    break;
            }

            if (MFX_ERR_MORE_DATA == sts) {
                // MFX_ERR_MORE_DATA indicates encoder need more input, request more surfaces from previous operation
                sts = MFX_ERR_NONE;
                continue;
            }

            if (MFX_ERR_NONE == sts) {
                sts = session.SyncOperation(syncpE, 60000);  // Synchronize. Wait until frame processing is ready
                MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

                if (options.values.MeasureLatency && (encOutTimeMap.size() < 1000)) {
                    mfxGetTime(&encOutTimeMap[mfxEncBS.TimeStamp]);

                    /*printf( "Finished encoding frame %d Type: %c latency: %f ms\n",
                            (int)mfxEncBS.TimeStamp,
                            mfxFrameTypeString(mfxEncBS.FrameType),
                            TimeDiffMsec(encOutTimeMap[mfxEncBS.TimeStamp], decInTimeMap[mfxEncBS.TimeStamp]));*/
                }

                sts = WriteBitStreamFrame(&mfxEncBS, fSink);
                MSDK_BREAK_ON_ERROR(sts);

                ++nFrame;
                if (bEnableOutput) {
                    printf("Frame number: %d\r", nFrame);
                }
            }
        }
    }

    // MFX_ERR_MORE_DATA indicates that all decode buffers has been fetched, exit in case of other errors
    MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_DATA);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    //
    // Stage 3: Retrieve the buffered encoded frames
    //
    while (MFX_ERR_NONE <= sts) {
        for (;;) {
            // Encode a frame asychronously (returns immediately)
            sts = mfxENC.EncodeFrameAsync(NULL, NULL, &mfxEncBS, &syncpE);

            if (MFX_ERR_NONE < sts && !syncpE) {    // repeat the call if warning and no output
                if (MFX_WRN_DEVICE_BUSY == sts)
                    MSDK_SLEEP(1);  // Wait if device is busy, then repeat the same call
            } else if (MFX_ERR_NONE < sts && syncpE) {
                sts = MFX_ERR_NONE;     // ignore warnings if output is available
                break;
            } else
                break;
        }

        if (MFX_ERR_NONE == sts) {
            sts = session.SyncOperation(syncpE, 60000);  // Synchronize. Wait until frame processing is ready
            MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

            if (options.values.MeasureLatency && (encOutTimeMap.size() < 1000))
                mfxGetTime(&encOutTimeMap[mfxEncBS.TimeStamp]);

            sts = WriteBitStreamFrame(&mfxEncBS, fSink);
            MSDK_BREAK_ON_ERROR(sts);

            ++nFrame;
            if (bEnableOutput) {
                printf("Frame number: %d\r", nFrame);
                fflush(stdout);
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
        for (std::unordered_map < mfxU64, mfxTime >::iterator it = decInTimeMap.begin(); it != decInTimeMap.end(); ++it) {
            int tsKey = (*it).first;
            mfxTime startTime = (*it).second;
            mfxTime endTime = encOutTimeMap[tsKey];
            double frameLatency = TimeDiffMsec(endTime, startTime);
            if (frameLatency>0)
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
    mfxDEC.Close();
    // session closed automatically on destruction

    for (int i = 0; i < nSurfNum; i++)
        delete pSurfaces[i];
    MSDK_SAFE_DELETE_ARRAY(pSurfaces);
    MSDK_SAFE_DELETE_ARRAY(mfxBS.Data);
    MSDK_SAFE_DELETE_ARRAY(mfxEncBS.Data);

    fclose(fSource);
    if (fSink) fclose(fSink);

    Release();

    return 0;
}
