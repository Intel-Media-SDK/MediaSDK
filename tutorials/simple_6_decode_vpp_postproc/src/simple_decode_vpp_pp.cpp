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
        "Decodes INPUT and optionally writes OUTPUT.\n"
        "\n"
        "Usage: %s [options] INPUT [OUTPUT]\n", ctx->program);
}

int main(int argc, char** argv)
{
    mfxStatus sts = MFX_ERR_NONE;
    bool bEnableOutput; // if true, removes all YUV file writing
    CmdOptions options;

    // =====================================================================
    // Intel Media SDK decode pipeline setup
    // - In this example we are decoding an AVC (H.264) stream
    // - For simplistic memory management, system memory surfaces are used to store the decoded frames
    //   (Note that when using HW acceleration video surfaces are prefered, for better performance)
    //   VPP frame processing included in pipeline (resize)

    // Read options from the command line (if any is given)
    memset(&options, 0, sizeof(CmdOptions));
    options.ctx.options = OPTIONS_DECODE;
    options.ctx.usage = usage;
    // Set default values:
    options.values.impl = MFX_IMPL_AUTO_ANY;

    // here we parse options
    ParseOptions(argc, argv, &options);

    if (!options.values.SourceName[0]) {
        printf("error: source file name not set (mandatory)\n");
        return -1;
    }

    bEnableOutput = (options.values.SinkName[0] != '\0');
    // Open input H.264 elementary stream (ES) file
    fileUniPtr fSource(OpenFile(options.values.SourceName, "rb"), &CloseFile);
    MSDK_CHECK_POINTER(fSource, MFX_ERR_NULL_PTR);

    // Create output YUV file
    fileUniPtr fSink(nullptr, &CloseFile);
    if (bEnableOutput) {
        fSink.reset(OpenFile(options.values.SinkName, "wb"));
        MSDK_CHECK_POINTER(fSink, MFX_ERR_NULL_PTR);
    }

    // Initialize Intel Media SDK session
    // - MFX_IMPL_AUTO_ANY selects HW acceleration if available (on any adapter)
    // - Version 1.0 is selected for greatest backwards compatibility.
    //   If more recent API features are needed, change the version accordingly
    mfxIMPL impl = options.values.impl;
    mfxVersion ver = { {0, 1} };
    MFXVideoSession session;
    sts = Initialize(impl, ver, &session, NULL);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // Create Media SDK decoder
    MFXVideoDECODE mfxDEC(session);
    // Create Media SDK VPP component
    MFXVideoVPP mfxVPP(session);

    // Set required video parameters for decode
    mfxVideoParam mfxVideoParams;
    memset(&mfxVideoParams, 0, sizeof(mfxVideoParams));
    mfxVideoParams.mfx.CodecId = MFX_CODEC_AVC;
    mfxVideoParams.IOPattern = MFX_IOPATTERN_OUT_SYSTEM_MEMORY;

    // Prepare Media SDK bit stream buffer
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

    sts = mfxDEC.DecodeHeader(&mfxBS, &mfxVideoParams);
    MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // Initialize VPP parameters
    // - For simplistic memory management, system memory surfaces are used to store the raw frames
    //   (Note that when using HW acceleration D3D surfaces are prefered, for better performance)
    mfxVideoParam VPPParams;
    memset(&VPPParams, 0, sizeof(VPPParams));
    // Input data
    VPPParams.vpp.In.FourCC = MFX_FOURCC_NV12;
    VPPParams.vpp.In.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
    VPPParams.vpp.In.CropX = 0;
    VPPParams.vpp.In.CropY = 0;
    VPPParams.vpp.In.CropW = mfxVideoParams.mfx.FrameInfo.CropW;
    VPPParams.vpp.In.CropH = mfxVideoParams.mfx.FrameInfo.CropH;
    VPPParams.vpp.In.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
    VPPParams.vpp.In.FrameRateExtN = 30;
    VPPParams.vpp.In.FrameRateExtD = 1;
    // width must be a multiple of 16
    // height must be a multiple of 16 in case of frame picture and a multiple of 32 in case of field picture
    VPPParams.vpp.In.Width = MSDK_ALIGN16(VPPParams.vpp.In.CropW);
    VPPParams.vpp.In.Height =
        (MFX_PICSTRUCT_PROGRESSIVE == VPPParams.vpp.In.PicStruct) ?
        MSDK_ALIGN16(VPPParams.vpp.In.CropH) :
        MSDK_ALIGN32(VPPParams.vpp.In.CropH);
    // Output data
    VPPParams.vpp.Out.FourCC = MFX_FOURCC_NV12;
    VPPParams.vpp.Out.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
    VPPParams.vpp.Out.CropX = 0;
    VPPParams.vpp.Out.CropY = 0;
    VPPParams.vpp.Out.CropW = VPPParams.vpp.In.CropW / 2;   // Resize to half size resolution
    VPPParams.vpp.Out.CropH = VPPParams.vpp.In.CropH / 2;
    VPPParams.vpp.Out.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
    VPPParams.vpp.Out.FrameRateExtN = 30;
    VPPParams.vpp.Out.FrameRateExtD = 1;
    // width must be a multiple of 16
    // height must be a multiple of 16 in case of frame picture and a multiple of 32 in case of field picture
    VPPParams.vpp.Out.Width = MSDK_ALIGN16(VPPParams.vpp.Out.CropW);
    VPPParams.vpp.Out.Height =
        (MFX_PICSTRUCT_PROGRESSIVE == VPPParams.vpp.Out.PicStruct) ?
        MSDK_ALIGN16(VPPParams.vpp.Out.CropH) :
        MSDK_ALIGN32(VPPParams.vpp.Out.CropH);

    VPPParams.IOPattern = MFX_IOPATTERN_IN_SYSTEM_MEMORY | MFX_IOPATTERN_OUT_SYSTEM_MEMORY;

    // Query number of required surfaces for decoder
    mfxFrameAllocRequest DecRequest;
    memset(&DecRequest, 0, sizeof(DecRequest));
    sts = mfxDEC.QueryIOSurf(&mfxVideoParams, &DecRequest);
    MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // Query number of required surfaces for VPP
    mfxFrameAllocRequest VPPRequest[2];     // [0] - in, [1] - out
    memset(&VPPRequest, 0, sizeof(mfxFrameAllocRequest) * 2);
    sts = mfxVPP.QueryIOSurf(&VPPParams, VPPRequest);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // Determine the required number of surfaces for decoder output (VPP input) and for VPP output
    mfxU16 nSurfNumDecVPP = DecRequest.NumFrameSuggested + VPPRequest[0].NumFrameSuggested;
    mfxU16 nSurfNumVPPOut = VPPRequest[1].NumFrameSuggested;

    // Allocate surfaces for decoder and VPP In
    // - Width and height of buffer must be aligned, a multiple of 32
    // - Frame surface array keeps pointers all surface planes and general frame info
    mfxU16 width = (mfxU16) MSDK_ALIGN32(DecRequest.Info.Width);
    mfxU16 height = (mfxU16) MSDK_ALIGN32(DecRequest.Info.Height);
    mfxU8 bitsPerPixel = 12;        // NV12 format is a 12 bits per pixel format
    mfxU32 surfaceSize = width * height * bitsPerPixel / 8;
    std::vector<mfxU8> surfaceBuffersData(surfaceSize * nSurfNumDecVPP);
    mfxU8* surfaceBuffers = surfaceBuffersData.data();

    std::vector<mfxFrameSurface1> pmfxSurfaces(nSurfNumDecVPP);
    for (int i = 0; i < nSurfNumDecVPP; i++) {
        memset(&pmfxSurfaces[i], 0, sizeof(mfxFrameSurface1));
        pmfxSurfaces[i].Info = mfxVideoParams.mfx.FrameInfo;
        pmfxSurfaces[i].Data.Y = &surfaceBuffers[surfaceSize * i];
        pmfxSurfaces[i].Data.U = pmfxSurfaces[i].Data.Y + width * height;
        pmfxSurfaces[i].Data.V = pmfxSurfaces[i].Data.U + 1;
        pmfxSurfaces[i].Data.Pitch = width;
    }

    // Allocate surfaces for VPP Out
    // - Width and height of buffer must be aligned, a multiple of 32
    // - Frame surface array keeps pointers all surface planes and general frame info
    width = (mfxU16) MSDK_ALIGN32(VPPRequest[1].Info.Width);
    height = (mfxU16) MSDK_ALIGN32(VPPRequest[1].Info.Height);
    bitsPerPixel = 12;      // NV12 format is a 12 bits per pixel format
    surfaceSize = width * height * bitsPerPixel / 8;
    std::vector<mfxU8> surfaceBuffersData2(surfaceSize * nSurfNumVPPOut);
    mfxU8* surfaceBuffers2 = surfaceBuffersData2.data();

    std::vector<mfxFrameSurface1> pmfxSurfaces2(nSurfNumVPPOut);
    for (int i = 0; i < nSurfNumVPPOut; i++) {
        memset(&pmfxSurfaces2[i], 0, sizeof(mfxFrameSurface1));
        pmfxSurfaces2[i].Info = VPPParams.vpp.Out;
        pmfxSurfaces2[i].Data.Y = &surfaceBuffers2[surfaceSize * i];
        pmfxSurfaces2[i].Data.U = pmfxSurfaces2[i].Data.Y + width * height;
        pmfxSurfaces2[i].Data.V = pmfxSurfaces2[i].Data.U + 1;
        pmfxSurfaces2[i].Data.Pitch = width;
    }

    // Initialize the Media SDK decoder
    sts = mfxDEC.Init(&mfxVideoParams);
    MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // Initialize Media SDK VPP
    sts = mfxVPP.Init(&VPPParams);
    MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // ===============================================================
    // Start decoding the frames from the stream
    //
    mfxTime tStart, tEnd;
    mfxGetTime(&tStart);

    mfxSyncPoint syncpD;
    mfxSyncPoint syncpV;
    mfxFrameSurface1* pmfxOutSurface = NULL;
    int nIndex = 0;
    int nIndex2 = 0;
    mfxU32 nFrame = 0;

    //
    // Stage 1: Main decoding loop
    //
    while (MFX_ERR_NONE <= sts || MFX_ERR_MORE_DATA == sts || MFX_ERR_MORE_SURFACE == sts) {
        if (MFX_WRN_DEVICE_BUSY == sts)
            MSDK_SLEEP(1);  // Wait if device is busy, then repeat the same call to DecodeFrameAsync

        if (MFX_ERR_MORE_DATA == sts) {
            sts = ReadBitStreamData(&mfxBS, fSource.get());       // Read more data into input bit stream
            MSDK_BREAK_ON_ERROR(sts);
        }

        if (MFX_ERR_MORE_SURFACE == sts || MFX_ERR_NONE == sts) {
            nIndex = GetFreeSurfaceIndex(pmfxSurfaces);     // Find free frame surface
            MSDK_CHECK_ERROR(MFX_ERR_NOT_FOUND, nIndex, MFX_ERR_MEMORY_ALLOC);
        }
        // Decode a frame asychronously (returns immediately)
        sts = mfxDEC.DecodeFrameAsync(&mfxBS, &pmfxSurfaces[nIndex], &pmfxOutSurface, &syncpD);

        // Ignore warnings if output is available,
        // if no output and no action required just repeat the DecodeFrameAsync call
        if (MFX_ERR_NONE < sts && syncpD)
            sts = MFX_ERR_NONE;

        if (MFX_ERR_NONE == sts) {
            nIndex2 = GetFreeSurfaceIndex(pmfxSurfaces2);   // Find free frame surface
            MSDK_CHECK_ERROR(MFX_ERR_NOT_FOUND, nIndex2, MFX_ERR_MEMORY_ALLOC);

            for (;;) {
                // Process a frame asychronously (returns immediately)
                sts = mfxVPP.RunFrameVPPAsync(pmfxOutSurface, &pmfxSurfaces2[nIndex2], NULL, &syncpV);

                if (MFX_ERR_NONE < sts && !syncpV) {    // repeat the call if warning and no output
                    if (MFX_WRN_DEVICE_BUSY == sts)
                        MSDK_SLEEP(1);  // wait if device is busy
                } else if (MFX_ERR_NONE < sts && syncpV) {
                    sts = MFX_ERR_NONE;     // ignore warnings if output is available
                    break;
                } else
                    break;  // not a warning
            }

            // VPP needs more data, let decoder decode another frame as input
            if (MFX_ERR_MORE_DATA == sts) {
                continue;
            } else if (MFX_ERR_MORE_SURFACE == sts) {
                // Not relevant for the illustrated workload! Therefore not handled.
                // Relevant for cases when VPP produces more frames at output than consumes at input. E.g. framerate conversion 30 fps -> 60 fps
                break;
            } else
                MSDK_BREAK_ON_ERROR(sts);
        }

        if (MFX_ERR_NONE == sts)
            sts = session.SyncOperation(syncpV, 60000);     // Synchronize. Wait until decoded frame is ready

        if (MFX_ERR_NONE == sts) {
            ++nFrame;
            if (bEnableOutput) {
                sts = WriteRawFrame(&pmfxSurfaces2[nIndex2], fSink.get());
                MSDK_BREAK_ON_ERROR(sts);

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
            MSDK_SLEEP(1);  // Wait if device is busy, then repeat the same call to DecodeFrameAsync

        nIndex = GetFreeSurfaceIndex(pmfxSurfaces);     // Find free frame surface
        MSDK_CHECK_ERROR(MFX_ERR_NOT_FOUND, nIndex, MFX_ERR_MEMORY_ALLOC);

        // Decode a frame asychronously (returns immediately)
        sts = mfxDEC.DecodeFrameAsync(NULL, &pmfxSurfaces[nIndex], &pmfxOutSurface, &syncpD);

        // Ignore warnings if output is available,
        // if no output and no action required just repeat the DecodeFrameAsync call
        if (MFX_ERR_NONE < sts && syncpD)
            sts = MFX_ERR_NONE;

        if (MFX_ERR_NONE == sts) {
            nIndex2 = GetFreeSurfaceIndex(pmfxSurfaces2);   // Find free frame surface
            MSDK_CHECK_ERROR(MFX_ERR_NOT_FOUND, nIndex2, MFX_ERR_MEMORY_ALLOC);

            for (;;) {
                // Process a frame asychronously (returns immediately)
                sts = mfxVPP.RunFrameVPPAsync(pmfxOutSurface, &pmfxSurfaces2[nIndex2], NULL, &syncpV);

                if (MFX_ERR_NONE < sts && !syncpV) {    // repeat the call if warning and no output
                    if (MFX_WRN_DEVICE_BUSY == sts)
                        MSDK_SLEEP(1);  // wait if device is busy
                } else if (MFX_ERR_NONE < sts && syncpV) {
                    sts = MFX_ERR_NONE;     // ignore warnings if output is available
                    break;
                } else
                    break;  // not a warning
            }

            // VPP needs more data, let decoder decode another frame as input
            if (MFX_ERR_MORE_DATA == sts) {
                continue;
            } else if (MFX_ERR_MORE_SURFACE == sts) {
                // Not relevant for the illustrated workload! Therefore not handled.
                // Relevant for cases when VPP produces more frames at output than consumes at input. E.g. framerate conversion 30 fps -> 60 fps
                break;
            } else
                MSDK_BREAK_ON_ERROR(sts);
        }

        if (MFX_ERR_NONE == sts)
            sts = session.SyncOperation(syncpV, 60000);     // Synchronize. Waits until decoded frame is ready

        if (MFX_ERR_NONE == sts) {
            ++nFrame;
            if (bEnableOutput) {
                sts = WriteRawFrame(&pmfxSurfaces2[nIndex2], fSink.get());
                MSDK_BREAK_ON_ERROR(sts);

                printf("Frame number: %d\r", nFrame);
            }
        }
    }

    // MFX_ERR_MORE_DATA means that decoder is done with buffered frames, need to go to VPP buffering loop, exit in case of other errors
    MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_DATA);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    //
    // Stage 3: Retrieve the buffered VPP frames
    //
    while (MFX_ERR_NONE <= sts) {
        nIndex2 = GetFreeSurfaceIndex(pmfxSurfaces2);   // Find free frame surface
        MSDK_CHECK_ERROR(MFX_ERR_NOT_FOUND, nIndex2, MFX_ERR_MEMORY_ALLOC);

        // Process a frame asychronously (returns immediately)
        sts = mfxVPP.RunFrameVPPAsync(NULL, &pmfxSurfaces2[nIndex2], NULL, &syncpV);
        MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_SURFACE);
        MSDK_BREAK_ON_ERROR(sts);

        sts = session.SyncOperation(syncpV, 60000);     // Synchronize. Wait until frame processing is ready
        MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

        ++nFrame;
        if (bEnableOutput) {
            sts = WriteRawFrame(&pmfxSurfaces2[nIndex2], fSink.get());
            MSDK_BREAK_ON_ERROR(sts);

            printf("Frame number: %d\r", nFrame);
        }
    }

    // MFX_ERR_MORE_DATA indicates that all buffers has been fetched, exit in case of other errors
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

    mfxDEC.Close();
    mfxVPP.Close();
    // session closed automatically on destruction

    Release();

    return 0;
}
