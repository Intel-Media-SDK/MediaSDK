// Copyright (c) 2021 Intel Corporation
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
        "Performs VPP conversion over INPUT and optionally writes OUTPUT. If\n"
        "INPUT is not specified simulates input with empty frames filled with\n"
        "the color.\n"
        "\n"
        "Usage: %s [options] [INPUT] [OUTPUT]\n", ctx->program);
}

int main(int argc, char** argv)
{
    mfxStatus sts = MFX_ERR_NONE;
    bool bEnableInput;  // if true, removes all YUV file reading (which is replaced by pre-initialized surface data). Workload runs for 1000 frames.
    bool bEnableOutput; // if true, removes all output bitsteam file writing and printing the progress
    CmdOptions options;

    mfxU32 lut3DMemID = 0;
    mfxHDL hDevice;

    const mfxU32 outWidth  = 1280;
    const mfxU32 outHeight = 720;

    // 3DLUT file name, please make sure this file exists.
    const char*  lut3dFileName = "3dlut_65cubic.dat";

    // =====================================================================
    // Intel Media SDK Video Pre/Post Processing (VPP) pipeline setup
    // - Showcasing two VPP features
    //   - Resize (frame width and height is halved)
    //   - Denoise: Remove noise from frames
    //

    // Read options from the command line (if any is given)
    memset(&options, 0, sizeof(CmdOptions));
    options.ctx.options = OPTIONS_VPP;
    options.ctx.usage = usage;
    // Set default values:
    options.values.impl = MFX_IMPL_AUTO_ANY;

    // here we parse options
    ParseOptions(argc, argv, &options);

    if (!options.values.Width || !options.values.Height) {
        printf("error: input video geometry not set (mandatory)\n");
        return -1;
    }

    bEnableInput = (options.values.SourceName[0] != '\0');
    bEnableOutput = (options.values.SinkName[0] != '\0');
    // Open input P010 YUV file
    fileUniPtr fSource(nullptr, &CloseFile);
    if (bEnableInput) {
        fSource.reset(OpenFile(options.values.SourceName, "rb"));
        MSDK_CHECK_POINTER(fSource, MFX_ERR_NULL_PTR);
    }
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

    mfxFrameAllocator mfxAllocator;

    sts = Initialize(impl, ver, &session, &mfxAllocator);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // Initialize VPP parameters
    // - For video memory surfaces are used to store the raw frames
    //   (Note that when using HW acceleration video surfaces are prefered, for better performance)
    mfxVideoParam VPPParams;
    memset(&VPPParams, 0, sizeof(VPPParams));
    // Input data
    VPPParams.vpp.In.FourCC = MFX_FOURCC_P010;
    VPPParams.vpp.In.BitDepthLuma = 10;
    VPPParams.vpp.In.BitDepthChroma = 10;
    VPPParams.vpp.In.Shift = 1;
    VPPParams.vpp.In.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
    VPPParams.vpp.In.CropX = 0;
    VPPParams.vpp.In.CropY = 0;
    VPPParams.vpp.In.CropW = options.values.Width;
    VPPParams.vpp.In.CropH = options.values.Height;
    VPPParams.vpp.In.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
    VPPParams.vpp.In.FrameRateExtN = 30;
    VPPParams.vpp.In.FrameRateExtD = 1;
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
    VPPParams.vpp.Out.CropW = outWidth;
    VPPParams.vpp.Out.CropH = outHeight;
    VPPParams.vpp.Out.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
    VPPParams.vpp.Out.FrameRateExtN = 30;
    VPPParams.vpp.Out.FrameRateExtD = 1;
    // width must be a multiple of 16
    // height must be a multiple of 16 in case of frame picture and a multiple of 32 in case of field picture
    VPPParams.vpp.Out.Width = outWidth;
    VPPParams.vpp.Out.Height = outHeight;

    VPPParams.IOPattern =
        MFX_IOPATTERN_IN_VIDEO_MEMORY | MFX_IOPATTERN_OUT_VIDEO_MEMORY;

    // Create Media SDK VPP component
    MFXVideoVPP mfxVPP(session);

    // Query number of required surfaces for VPP
    mfxFrameAllocRequest VPPRequest[2];     // [0] - in, [1] - out
    memset(&VPPRequest, 0, sizeof(mfxFrameAllocRequest) * 2);
    sts = mfxVPP.QueryIOSurf(&VPPParams, VPPRequest);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    VPPRequest[0].Type |= WILL_WRITE; // This line is only required for Windows DirectX11 to ensure that surfaces can be written to by the application
    VPPRequest[1].Type |= WILL_READ; // This line is only required for Windows DirectX11 to ensure that surfaces can be retrieved by the application

    // Allocate required surfaces
    mfxFrameAllocResponse mfxResponseIn;
    mfxFrameAllocResponse mfxResponseOut;
    sts = mfxAllocator.Alloc(mfxAllocator.pthis, &VPPRequest[0], &mfxResponseIn);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
    sts = mfxAllocator.Alloc(mfxAllocator.pthis, &VPPRequest[1], &mfxResponseOut);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    mfxU16 nVPPSurfNumIn = mfxResponseIn.NumFrameActual;
    mfxU16 nVPPSurfNumOut = mfxResponseOut.NumFrameActual;

    printf("Test............................................\n");

    // Allocate surface headers (mfxFrameSurface1) for VPP
    std::vector<mfxFrameSurface1> pVPPSurfacesIn(nVPPSurfNumIn);    
    for (int i = 0; i < nVPPSurfNumIn; i++) {
        memset(&pVPPSurfacesIn[i], 0, sizeof(mfxFrameSurface1));
        pVPPSurfacesIn[i].Info = VPPParams.vpp.In;
        pVPPSurfacesIn[i].Data.MemId = mfxResponseIn.mids[i];  // MID (memory id) represent one D3D NV12 surface

        if (!bEnableInput) {
            ClearYUVSurfaceVMem(pVPPSurfacesIn[i].Data.MemId);
        }
    }

    std::vector<mfxFrameSurface1> pVPPSurfacesOut(nVPPSurfNumOut);
    mfxExtVideoSignalInfo videoSignalInfoOut[5];
    for (int i = 0; i < nVPPSurfNumOut; i++) {
        memset(&pVPPSurfacesOut[i], 0, sizeof(mfxFrameSurface1));

        pVPPSurfacesOut[i].Info = VPPParams.vpp.Out;
        pVPPSurfacesOut[i].Data.MemId = mfxResponseOut.mids[i];    // MID (memory id) represent one D3D NV12 surface
    }

    // Create 3DLut Memory to hold 3DLut data
    session.GetHandle(MFX_HANDLE_VA_DISPLAY, &hDevice);
    sts = Create3DLutMemory((void*)&lut3DMemID, hDevice, lut3dFileName);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // Initialize extended buffer for frame processing
    // - mfxExtVPPDoUse:   Define the processing algorithm to be used
    // - mfxExtVPP3DLut:   3DLUT configuration
    // - mfxExtVPPScaling: Scaling configuration
    // - mfxExtBuffer:     Add extended buffers to VPP parameter configuration
    mfxExtVPPDoUse extDoUse;
    memset(&extDoUse, 0, sizeof(extDoUse));
    mfxU32 tabDoUseAlg[2];
    extDoUse.Header.BufferId = MFX_EXTBUFF_VPP_DOUSE;
    extDoUse.Header.BufferSz = sizeof(mfxExtVPPDoUse);
    extDoUse.NumAlg = 2;
    extDoUse.AlgList = tabDoUseAlg;
    tabDoUseAlg[0] = MFX_EXTBUFF_VPP_3DLUT;
    tabDoUseAlg[1] = MFX_EXTBUFF_VPP_SCALING;

    mfxExtVPP3DLut lut3DConfig;
    memset(&lut3DConfig, 0, sizeof(lut3DConfig));
    lut3DConfig.Header.BufferId     = MFX_EXTBUFF_VPP_3DLUT;
    lut3DConfig.Header.BufferSz     = sizeof(mfxExtVPP3DLut);

    lut3DConfig.ChannelMapping              = MFX_3DLUT_CHANNEL_MAPPING_RGB_RGB;
    lut3DConfig.BufferType                  = MFX_RESOURCE_VA_SURFACE;
    lut3DConfig.VideoBuffer.DataType        = MFX_DATA_TYPE_U16;
    lut3DConfig.VideoBuffer.MemLayout       = MFX_3DLUT_MEMORY_LAYOUT_INTEL_65LUT;
    lut3DConfig.VideoBuffer.MemId           = &lut3DMemID;

    mfxExtVPPScaling scalingConfig;
    memset(&scalingConfig, 0, sizeof(scalingConfig));
    scalingConfig.Header.BufferId   = MFX_EXTBUFF_VPP_SCALING;
    scalingConfig.Header.BufferSz   = sizeof(mfxExtVPPScaling);
    scalingConfig.ScalingMode       = MFX_SCALING_MODE_LOWPOWER;

    mfxExtBuffer* ExtBuffer[3];
    ExtBuffer[0] = (mfxExtBuffer*) &extDoUse;
    ExtBuffer[1] = (mfxExtBuffer*) &lut3DConfig;
    ExtBuffer[2] = (mfxExtBuffer*) &scalingConfig;
    VPPParams.NumExtParam = 3;
    VPPParams.ExtParam = (mfxExtBuffer**) &ExtBuffer[0];

    // Initialize Media SDK VPP
    sts = mfxVPP.Init(&VPPParams);
    MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // ===================================
    // Start processing the frames
    //

    mfxTime tStart, tEnd;
    mfxGetTime(&tStart);

    int nSurfIdxIn = 0, nSurfIdxOut = 0;
    mfxSyncPoint syncp;
    mfxU32 nFrame = 0;

    //
    // Stage 1: Main processing loop
    //
    while (MFX_ERR_NONE <= sts || MFX_ERR_MORE_DATA == sts) {
        nSurfIdxIn = GetFreeSurfaceIndex(pVPPSurfacesIn);        // Find free input frame surface
        MSDK_CHECK_ERROR(MFX_ERR_NOT_FOUND, nSurfIdxIn, MFX_ERR_MEMORY_ALLOC);

        // Surface locking required when read/write video surfaces
        sts = mfxAllocator.Lock(mfxAllocator.pthis, pVPPSurfacesIn[nSurfIdxIn].Data.MemId, &(pVPPSurfacesIn[nSurfIdxIn].Data));
        MSDK_BREAK_ON_ERROR(sts);

        sts = LoadRawFrame(&pVPPSurfacesIn[nSurfIdxIn], fSource.get());        // Load frame from file into surface
        MSDK_BREAK_ON_ERROR(sts);

        sts = mfxAllocator.Unlock(mfxAllocator.pthis, pVPPSurfacesIn[nSurfIdxIn].Data.MemId, &(pVPPSurfacesIn[nSurfIdxIn].Data));
        MSDK_BREAK_ON_ERROR(sts);

        nSurfIdxOut = GetFreeSurfaceIndex(pVPPSurfacesOut);     // Find free output frame surface
        MSDK_CHECK_ERROR(MFX_ERR_NOT_FOUND, nSurfIdxOut, MFX_ERR_MEMORY_ALLOC);

        mfxExtVideoSignalInfo videoSignalInfoIn = {};
        videoSignalInfoIn.Header.BufferId     = MFX_EXTBUFF_VIDEO_SIGNAL_INFO;
        videoSignalInfoIn.Header.BufferSz     = sizeof(mfxExtVideoSignalInfo);
        videoSignalInfoIn.ColourPrimaries = 9;
        mfxExtBuffer* ExtBufferIn = (mfxExtBuffer*)&videoSignalInfoIn;
        pVPPSurfacesIn[nSurfIdxIn].Data.NumExtParam = 1;
        pVPPSurfacesIn[nSurfIdxIn].Data.ExtParam = (mfxExtBuffer**)&ExtBufferIn;

        mfxExtVideoSignalInfo videoSignalInfoOut = {};
        videoSignalInfoOut.Header.BufferId     = MFX_EXTBUFF_VIDEO_SIGNAL_INFO;
        videoSignalInfoOut.Header.BufferSz     = sizeof(mfxExtVideoSignalInfo);
        videoSignalInfoOut.ColourPrimaries = 1;
        mfxExtBuffer* ExtBufferOut = (mfxExtBuffer*)&videoSignalInfoOut;
        pVPPSurfacesOut[nSurfIdxOut].Data.NumExtParam = 1;
        pVPPSurfacesOut[nSurfIdxOut].Data.ExtParam = (mfxExtBuffer**)&ExtBufferOut;

        for (;;) {
            // Process a frame asychronously (returns immediately)
            sts = mfxVPP.RunFrameVPPAsync(&pVPPSurfacesIn[nSurfIdxIn], &pVPPSurfacesOut[nSurfIdxOut], NULL, &syncp);            
            if (MFX_WRN_DEVICE_BUSY == sts) {
                MSDK_SLEEP(1);  // Wait if device is busy, then repeat the same call
            } else
                break;
        }

        if (MFX_ERR_MORE_DATA == sts)   // Fetch more input surfaces for VPP
            continue;

        // MFX_ERR_MORE_SURFACE means output is ready but need more surface (example: Frame Rate Conversion 30->60)
        // * Not handled in this example!

        MSDK_BREAK_ON_ERROR(sts);

        sts = session.SyncOperation(syncp, 60000);      // Synchronize. Wait until frame processing is ready
        MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

        ++nFrame;
        if (bEnableOutput) {
            // Surface locking required when read/write video surfaces
            sts = mfxAllocator.Lock(mfxAllocator.pthis, pVPPSurfacesOut[nSurfIdxOut].Data.MemId, &(pVPPSurfacesOut[nSurfIdxOut].Data));
            MSDK_BREAK_ON_ERROR(sts);

            sts = WriteRawFrame(&pVPPSurfacesOut[nSurfIdxOut], fSink.get());
            MSDK_BREAK_ON_ERROR(sts);

            sts = mfxAllocator.Unlock(mfxAllocator.pthis, pVPPSurfacesOut[nSurfIdxOut].Data.MemId, &(pVPPSurfacesOut[nSurfIdxOut].Data));
            MSDK_BREAK_ON_ERROR(sts);

            printf("Frame number: %d\r", nFrame);
            fflush(stdout);
        }
    }

    // MFX_ERR_MORE_DATA means that the input file has ended, need to go to buffering loop, exit in case of other errors
    MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_DATA);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    //
    // Stage 2: Retrieve the buffered VPP frames
    //
    while (MFX_ERR_NONE <= sts) {
        nSurfIdxOut = GetFreeSurfaceIndex(pVPPSurfacesOut);     // Find free frame surface
        MSDK_CHECK_ERROR(MFX_ERR_NOT_FOUND, nSurfIdxOut, MFX_ERR_MEMORY_ALLOC);

        for (;;) {
            // Process a frame asychronously (returns immediately)
            sts = mfxVPP.RunFrameVPPAsync(NULL, &pVPPSurfacesOut[nSurfIdxOut], NULL, &syncp);
            if (MFX_WRN_DEVICE_BUSY == sts) {
                MSDK_SLEEP(1);  // Wait if device is busy, then repeat the same call
            } else
                break;
        }

        MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_SURFACE);
        MSDK_BREAK_ON_ERROR(sts);

        sts = session.SyncOperation(syncp, 60000);      // Synchronize. Wait until frame processing is ready
        MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

        ++nFrame;
        if (bEnableOutput) {
            // Surface locking required when read/write video surfaces
            sts = mfxAllocator.Lock(mfxAllocator.pthis, pVPPSurfacesOut[nSurfIdxOut].Data.MemId, &(pVPPSurfacesOut[nSurfIdxOut].Data));
            MSDK_BREAK_ON_ERROR(sts);

            sts = WriteRawFrame(&pVPPSurfacesOut[nSurfIdxOut], fSink.get());
            MSDK_BREAK_ON_ERROR(sts);

            sts = mfxAllocator.Unlock(mfxAllocator.pthis, pVPPSurfacesOut[nSurfIdxOut].Data.MemId, &(pVPPSurfacesOut[nSurfIdxOut].Data));
            MSDK_BREAK_ON_ERROR(sts);

            printf("Frame number: %d\r", nFrame);
            fflush(stdout);
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
    mfxVPP.Close();
    //session closed automatically on destruction

    mfxAllocator.Free(mfxAllocator.pthis, &mfxResponseIn);
    mfxAllocator.Free(mfxAllocator.pthis, &mfxResponseOut);

    sts = Release3DLutMemory(&lut3DMemID, &hDevice);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    Release();


    return 0;
}
