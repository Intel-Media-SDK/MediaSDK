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

int main(int argc, char* argv[])
{
    mfxStatus sts = MFX_ERR_NONE;
    bool bEnableOutput; // if true, removes all YUV file writing
    CmdOptions options;

    // =====================================================================
    // Intel Media SDK decode pipeline setup
    // - In this example we are decoding an AVC (H.264) stream
    // - For simplistic memory management, system memory surfaces are used to store the decoded frames
    //   (Note that when using HW acceleration video surfaces are prefered, for better performance)
    //

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
    FILE* fSource;
    MSDK_FOPEN(fSource, options.values.SourceName, "rb");
    MSDK_CHECK_POINTER(fSource, MFX_ERR_NULL_PTR);

    // Create output YUV file
    FILE* fSink = NULL;
    if (bEnableOutput) {
        MSDK_FOPEN(fSink, options.values.SinkName, "wb");
        MSDK_CHECK_POINTER(fSink, MFX_ERR_NULL_PTR);
    }

    // Initialize Intel Media SDK session
    // - MFX_IMPL_AUTO_ANY selects HW acceleration if available (on any adapter)
    // - Version 1.0 is selected for greatest backwards compatibility.
    // OS specific notes
    // - On Windows both SW and HW libraries may present
    // - On Linux only HW library only is available
    //   If more recent API features are needed, change the version accordingly
    mfxIMPL impl = options.values.impl;
    mfxVersion ver = { {0, 1} };
    MFXVideoSession session;

    sts = Initialize(impl, ver, &session, NULL);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // Create Media SDK decoder
    MFXVideoDECODE mfxDEC(session);

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
    mfxBS.Data = new mfxU8[mfxBS.MaxLength];
    MSDK_CHECK_POINTER(mfxBS.Data, MFX_ERR_MEMORY_ALLOC);

    // Read a chunk of data from stream file into bit stream buffer
    // - Parse bit stream, searching for header and fill video parameters structure
    // - Abort if bit stream header is not found in the first bit stream buffer chunk
    sts = ReadBitStreamData(&mfxBS, fSource);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    sts = mfxDEC.DecodeHeader(&mfxBS, &mfxVideoParams);
    MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // Validate video decode parameters (optional)
    // sts = mfxDEC.Query(&mfxVideoParams, &mfxVideoParams);

    // Query number of required surfaces for decoder
    mfxFrameAllocRequest Request;
    memset(&Request, 0, sizeof(Request));
    sts = mfxDEC.QueryIOSurf(&mfxVideoParams, &Request);
    MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    mfxU16 numSurfaces = Request.NumFrameSuggested;

    // Allocate surfaces for decoder
    // - Width and height of buffer must be aligned, a multiple of 32
    // - Frame surface array keeps pointers all surface planes and general frame info
    mfxU16 width = (mfxU16) MSDK_ALIGN32(Request.Info.Width);
    mfxU16 height = (mfxU16) MSDK_ALIGN32(Request.Info.Height);
    mfxU8 bitsPerPixel = 12;        // NV12 format is a 12 bits per pixel format
    mfxU32 surfaceSize = width * height * bitsPerPixel / 8;
    mfxU8* surfaceBuffers = (mfxU8*) new mfxU8[surfaceSize * numSurfaces];

    // Allocate surface headers (mfxFrameSurface1) for decoder
    mfxFrameSurface1** pmfxSurfaces = new mfxFrameSurface1 *[numSurfaces];
    MSDK_CHECK_POINTER(pmfxSurfaces, MFX_ERR_MEMORY_ALLOC);
    for (int i = 0; i < numSurfaces; i++) {
        pmfxSurfaces[i] = new mfxFrameSurface1;
        memset(pmfxSurfaces[i], 0, sizeof(mfxFrameSurface1));
        memcpy(&(pmfxSurfaces[i]->Info), &(mfxVideoParams.mfx.FrameInfo), sizeof(mfxFrameInfo));
        pmfxSurfaces[i]->Data.Y = &surfaceBuffers[surfaceSize * i];
        pmfxSurfaces[i]->Data.U = pmfxSurfaces[i]->Data.Y + width * height;
        pmfxSurfaces[i]->Data.V = pmfxSurfaces[i]->Data.U + 1;
        pmfxSurfaces[i]->Data.Pitch = width;
    }

    // Initialize the Media SDK decoder
    sts = mfxDEC.Init(&mfxVideoParams);
    MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // ===============================================================
    // Start decoding the frames
    //

    mfxTime tStart, tEnd;
    mfxGetTime(&tStart);

    mfxSyncPoint syncp;
    mfxFrameSurface1* pmfxOutSurface = NULL;
    int nIndex = 0;
    mfxU32 nFrame = 0;

    //
    // Stage 1: Main decoding loop
    //
    while (MFX_ERR_NONE <= sts || MFX_ERR_MORE_DATA == sts || MFX_ERR_MORE_SURFACE == sts) {
        if (MFX_WRN_DEVICE_BUSY == sts)
            MSDK_SLEEP(1);  // Wait if device is busy, then repeat the same call to DecodeFrameAsync

        if (MFX_ERR_MORE_DATA == sts) {
            sts = ReadBitStreamData(&mfxBS, fSource);       // Read more data into input bit stream
            MSDK_BREAK_ON_ERROR(sts);
        }

        if (MFX_ERR_MORE_SURFACE == sts || MFX_ERR_NONE == sts) {
            nIndex = GetFreeSurfaceIndex(pmfxSurfaces, numSurfaces);        // Find free frame surface
            MSDK_CHECK_ERROR(MFX_ERR_NOT_FOUND, nIndex, MFX_ERR_MEMORY_ALLOC);
        }
        // Decode a frame asychronously (returns immediately)
        //  - If input bitstream contains multiple frames DecodeFrameAsync will start decoding multiple frames, and remove them from bitstream
        sts = mfxDEC.DecodeFrameAsync(&mfxBS, pmfxSurfaces[nIndex], &pmfxOutSurface, &syncp);

        // Ignore warnings if output is available,
        // if no output and no action required just repeat the DecodeFrameAsync call
        if (MFX_ERR_NONE < sts && syncp)
            sts = MFX_ERR_NONE;

        if (MFX_ERR_NONE == sts)
            sts = session.SyncOperation(syncp, 60000);      // Synchronize. Wait until decoded frame is ready

        if (MFX_ERR_NONE == sts) {
            ++nFrame;
            if (bEnableOutput) {
                sts = WriteRawFrame(pmfxOutSurface, fSink);
                MSDK_BREAK_ON_ERROR(sts);

                printf("Frame number: %d\r", nFrame);
                fflush(stdout);
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

        nIndex = GetFreeSurfaceIndex(pmfxSurfaces, numSurfaces);        // Find free frame surface
        MSDK_CHECK_ERROR(MFX_ERR_NOT_FOUND, nIndex, MFX_ERR_MEMORY_ALLOC);

        // Decode a frame asychronously (returns immediately)
        sts = mfxDEC.DecodeFrameAsync(NULL, pmfxSurfaces[nIndex], &pmfxOutSurface, &syncp);

        // Ignore warnings if output is available,
        // if no output and no action required just repeat the DecodeFrameAsync call
        if (MFX_ERR_NONE < sts && syncp)
            sts = MFX_ERR_NONE;

        if (MFX_ERR_NONE == sts)
            sts = session.SyncOperation(syncp, 60000);      // Synchronize. Waits until decoded frame is ready

        if (MFX_ERR_NONE == sts) {
            ++nFrame;
            if (bEnableOutput) {
                sts = WriteRawFrame(pmfxOutSurface, fSink);
                MSDK_BREAK_ON_ERROR(sts);

                printf("Frame number: %d\r", nFrame);
                fflush(stdout);
            }
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
    // session closed automatically on destruction

    for (int i = 0; i < numSurfaces; i++)
        delete pmfxSurfaces[i];
    MSDK_SAFE_DELETE_ARRAY(pmfxSurfaces);
    MSDK_SAFE_DELETE_ARRAY(mfxBS.Data);

    MSDK_SAFE_DELETE_ARRAY(surfaceBuffers);

    fclose(fSource);
    if (fSink) fclose(fSink);

    Release();

    return 0;
}
