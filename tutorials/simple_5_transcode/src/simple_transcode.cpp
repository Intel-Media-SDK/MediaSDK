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
        "Transcodes INPUT and optionally writes OUTPUT.\n"
        "\n"
        "Usage: %s [options] INPUT [OUTPUT]\n", ctx->program);
}

int main(int argc, char** argv)
{
    mfxStatus sts = MFX_ERR_NONE;
    bool bEnableOutput; // if true, removes all output bitsteam file writing and printing the progress
    CmdOptions options;

    // =====================================================================
    // Intel Media SDK transcode pipeline setup
    // - In this example we are decoding and encoding an AVC (H.264) stream
    // - For simplistic memory management, system memory surfaces are used to store the decoded frames
    //   (Note that when using HW acceleration D3D surfaces are prefered, for better performance)
    //

    // Read options from the command line (if any is given)
    memset(&options, 0, sizeof(CmdOptions));
    options.ctx.options = OPTIONS_TRANSCODE;
    options.ctx.usage = usage;
    // Set default values:
    options.values.impl = MFX_IMPL_AUTO_ANY;

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
    fileUniPtr fSource(OpenFile(options.values.SourceName, "rb"), &CloseFile);
    MSDK_CHECK_POINTER(fSource, MFX_ERR_NULL_PTR);

    // Create output elementary stream (ES) H.264 file
    fileUniPtr fSink(nullptr, &CloseFile);
    if (bEnableOutput) {
        fSink.reset(OpenFile(options.values.SinkName, "wb"));
        MSDK_CHECK_POINTER(fSink, MFX_ERR_NULL_PTR);
    }

    // Initialize Media SDK session
    // - MFX_IMPL_AUTO_ANY selects HW acceleration if available (on any adapter)
    // - Version 1.0 is selected for greatest backwards compatibility.
    //   If more recent API features are needed, change the version accordingly
    mfxIMPL impl = options.values.impl;
    mfxVersion ver = { {0, 1} };
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
    mfxDecParams.IOPattern = MFX_IOPATTERN_OUT_SYSTEM_MEMORY;

    // Prepare Media SDK bit stream buffer for decoder
    // - Arbitrary buffer size for this example
    mfxBitstream mfxBS;
    memset(&mfxBS, 0, sizeof(mfxBS));
    mfxBS.MaxLength = 1024 * 1024;
    std::vector<mfxU8> bstData(mfxBS.MaxLength);
    mfxBS.Data = bstData.data();


    // Read a chunk of data from stream file into bit stream buffer
    // - Parse bit stream, searching for header and fill video parameters structure
    // - Abort if bit stream header is not found in the first bit stream buffer chunk
    sts = ReadBitStreamData(&mfxBS, fSource.get());
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

    mfxEncParams.IOPattern = MFX_IOPATTERN_IN_SYSTEM_MEMORY;

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

    // Allocate shared surfaces for decoder and encoder
    // - Width and height of buffer must be aligned, a multiple of 32
    // - Frame surface array keeps pointers all surface planes and general frame info
    mfxU16 width = (mfxU16) MSDK_ALIGN32(EncRequest.Info.Width);
    mfxU16 height = (mfxU16) MSDK_ALIGN32(EncRequest.Info.Height);
    mfxU8 bitsPerPixel = 12;        // NV12 format is a 12 bits per pixel format
    mfxU32 surfaceSize = width * height * bitsPerPixel / 8;
    std::vector<mfxU8> surfaceBuffersData(surfaceSize * nSurfNum);
    mfxU8* surfaceBuffers = surfaceBuffersData.data();

    std::vector<mfxFrameSurface1> pSurfaces(nSurfNum);
    for (int i = 0; i < nSurfNum; i++) {
        memset(&pSurfaces[i], 0, sizeof(mfxFrameSurface1));
        pSurfaces[i].Info = mfxEncParams.mfx.FrameInfo;
        pSurfaces[i].Data.Y = &surfaceBuffers[surfaceSize * i];
        pSurfaces[i].Data.U = pSurfaces[i].Data.Y + width * height;
        pSurfaces[i].Data.V = pSurfaces[i].Data.U + 1;
        pSurfaces[i].Data.Pitch = width;
    }

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
    std::vector<mfxU8> bstEncData(mfxEncBS.MaxLength);
    mfxEncBS.Data = bstEncData.data();


    // ===================================
    // Start transcoding the frames
    //

    mfxTime tStart, tEnd;
    mfxGetTime(&tStart);

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
            sts = ReadBitStreamData(&mfxBS, fSource.get());       // Read more data to input bit stream
            MSDK_BREAK_ON_ERROR(sts);
        }

        if (MFX_ERR_MORE_SURFACE == sts || MFX_ERR_NONE == sts) {
            nIndex = GetFreeSurfaceIndex(pSurfaces);      // Find free frame surface
            MSDK_CHECK_ERROR(MFX_ERR_NOT_FOUND, nIndex, MFX_ERR_MEMORY_ALLOC);
        }
        // Decode a frame asychronously (returns immediately)
        sts = mfxDEC.DecodeFrameAsync(&mfxBS, &pSurfaces[nIndex], &pmfxOutSurface, &syncpD);

        // Ignore warnings if output is available,
        // if no output and no action required just repeat the DecodeFrameAsync call
        if (MFX_ERR_NONE < sts && syncpD)
            sts = MFX_ERR_NONE;

        // skip the buffer in mfxBS when MFX_ERR_INCOMPATIBLE_VIDEO_PARAM
        if (MFX_ERR_INCOMPATIBLE_VIDEO_PARAM == sts)
        {
            mfxBS.DataOffset += mfxBS.DataLength;
            mfxBS.DataLength = 0;
            sts = MFX_ERR_MORE_DATA;
            continue;
        }

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
                sts = session.SyncOperation(syncpE, 60000);     // Synchronize. Wait until frame processing is ready
                MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

                sts = WriteBitStreamFrame(&mfxEncBS, fSink.get());
                MSDK_BREAK_ON_ERROR(sts);

                ++nFrame;
                if (bEnableOutput && (nFrame%100 == 0)) {
                    printf("Frame number: %d\r", nFrame);
                    fflush(stdout);
                }
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

        nIndex = GetFreeSurfaceIndex(pSurfaces);      // Find free frame surface
        MSDK_CHECK_ERROR(MFX_ERR_NOT_FOUND, nIndex, MFX_ERR_MEMORY_ALLOC);

        // Decode a frame asychronously (returns immediately)
        sts = mfxDEC.DecodeFrameAsync(NULL, &pSurfaces[nIndex], &pmfxOutSurface, &syncpD);

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
                sts = session.SyncOperation(syncpE, 60000);     // Synchronize. Wait until frame processing is ready
                MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

                sts = WriteBitStreamFrame(&mfxEncBS, fSink.get());
                MSDK_BREAK_ON_ERROR(sts);

                ++nFrame;
                if (bEnableOutput) {
                    printf("Frame number: %d\r", nFrame);
                    fflush(stdout);
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
            sts = session.SyncOperation(syncpE, 60000);     // Synchronize. Wait until frame processing is ready
            MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

            sts = WriteBitStreamFrame(&mfxEncBS, fSink.get());
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

    // ===================================================================
    // Clean up resources
    //  - It is recommended to close Media SDK components first, before releasing allocated surfaces, since
    //    some surfaces may still be locked by internal Media SDK resources.

    mfxENC.Close();
    mfxDEC.Close();
    // session closed automatically on destruction

    Release();

    return 0;
}
