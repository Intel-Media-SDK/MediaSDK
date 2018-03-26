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

// EACH block should be declared here
#ifndef msdk_ts_DECLARE_BLOCK
    //break build
    #error improper usage of blocks declaration header
    #define msdk_ts_DECLARE_BLOCK(name) 
#endif

/* Global Functions */
msdk_ts_DECLARE_BLOCK(b_MFXInit);
msdk_ts_DECLARE_BLOCK(b_MFXClose);
msdk_ts_DECLARE_BLOCK(b_MFXQueryIMPL);
msdk_ts_DECLARE_BLOCK(b_MFXQueryVersion);
msdk_ts_DECLARE_BLOCK(b_MFXJoinSession);
msdk_ts_DECLARE_BLOCK(b_MFXDisjoinSession);
msdk_ts_DECLARE_BLOCK(b_MFXCloneSession);
msdk_ts_DECLARE_BLOCK(b_MFXSetPriority);
msdk_ts_DECLARE_BLOCK(b_MFXGetPriority);

/* VideoCORE */
msdk_ts_DECLARE_BLOCK(b_MFXVideoCORE_SetBufferAllocator);
msdk_ts_DECLARE_BLOCK(b_MFXVideoCORE_SetFrameAllocator);
msdk_ts_DECLARE_BLOCK(b_MFXVideoCORE_SetHandle);
msdk_ts_DECLARE_BLOCK(b_MFXVideoCORE_GetHandle);
msdk_ts_DECLARE_BLOCK(b_MFXVideoCORE_SyncOperation);

/* VideoENCODE */
msdk_ts_DECLARE_BLOCK(b_MFXVideoENCODE_Query);
msdk_ts_DECLARE_BLOCK(b_MFXVideoENCODE_QueryIOSurf);
msdk_ts_DECLARE_BLOCK(b_MFXVideoENCODE_Init);
msdk_ts_DECLARE_BLOCK(b_MFXVideoENCODE_Reset);
msdk_ts_DECLARE_BLOCK(b_MFXVideoENCODE_Close);

msdk_ts_DECLARE_BLOCK(b_MFXVideoENCODE_GetVideoParam);
msdk_ts_DECLARE_BLOCK(b_MFXVideoENCODE_GetEncodeStat);
msdk_ts_DECLARE_BLOCK(b_MFXVideoENCODE_EncodeFrameAsync);

/* VideoDECODE */
msdk_ts_DECLARE_BLOCK(b_MFXVideoDECODE_Query);
msdk_ts_DECLARE_BLOCK(b_MFXVideoDECODE_DecodeHeader);
msdk_ts_DECLARE_BLOCK(b_MFXVideoDECODE_QueryIOSurf);
msdk_ts_DECLARE_BLOCK(b_MFXVideoDECODE_Init);
msdk_ts_DECLARE_BLOCK(b_MFXVideoDECODE_Reset);
msdk_ts_DECLARE_BLOCK(b_MFXVideoDECODE_Close);

msdk_ts_DECLARE_BLOCK(b_MFXVideoDECODE_GetVideoParam);
msdk_ts_DECLARE_BLOCK(b_MFXVideoDECODE_GetDecodeStat);
msdk_ts_DECLARE_BLOCK(b_MFXVideoDECODE_SetSkipMode);
msdk_ts_DECLARE_BLOCK(b_MFXVideoDECODE_GetPayload);
msdk_ts_DECLARE_BLOCK(b_MFXVideoDECODE_DecodeFrameAsync);

/* VideoVPP */
msdk_ts_DECLARE_BLOCK(b_MFXVideoVPP_Query);
msdk_ts_DECLARE_BLOCK(b_MFXVideoVPP_QueryIOSurf);
msdk_ts_DECLARE_BLOCK(b_MFXVideoVPP_Init);
msdk_ts_DECLARE_BLOCK(b_MFXVideoVPP_Reset);
msdk_ts_DECLARE_BLOCK(b_MFXVideoVPP_Close);

msdk_ts_DECLARE_BLOCK(b_MFXVideoVPP_GetVideoParam);
msdk_ts_DECLARE_BLOCK(b_MFXVideoVPP_GetVPPStat);
msdk_ts_DECLARE_BLOCK(b_MFXVideoVPP_RunFrameVPPAsync);

/* VideoUser */
msdk_ts_DECLARE_BLOCK(b_MFXVideoUSER_Register);
msdk_ts_DECLARE_BLOCK(b_MFXVideoUSER_Unregister);
msdk_ts_DECLARE_BLOCK(b_MFXVideoUSER_ProcessFrameAsync);
msdk_ts_DECLARE_BLOCK(b_MFXVideoUSER_Load);
msdk_ts_DECLARE_BLOCK(b_MFXVideoUSER_UnLoad);


// audio core
msdk_ts_DECLARE_BLOCK(b_MFXInitAudio);
msdk_ts_DECLARE_BLOCK(b_MFXCloseAudio);

msdk_ts_DECLARE_BLOCK(b_MFXAudioQueryIMPL);
msdk_ts_DECLARE_BLOCK(b_MFXAudioQueryVersion);

//AudioENCODE
msdk_ts_DECLARE_BLOCK(b_MFXAudioENCODE_Query);
msdk_ts_DECLARE_BLOCK(b_MFXAudioENCODE_QueryIOSize);
msdk_ts_DECLARE_BLOCK(b_MFXAudioENCODE_Init);
msdk_ts_DECLARE_BLOCK(b_MFXAudioENCODE_Reset);
msdk_ts_DECLARE_BLOCK(b_MFXAudioENCODE_Close);
msdk_ts_DECLARE_BLOCK(b_MFXAudioENCODE_GetAudioParam);
msdk_ts_DECLARE_BLOCK(b_MFXAudioENCODE_EncodeFrameAsync);


//AudioDecode
msdk_ts_DECLARE_BLOCK(b_MFXAudioDECODE_Query);
msdk_ts_DECLARE_BLOCK(b_MFXAudioDECODE_QueryIOSize);
msdk_ts_DECLARE_BLOCK(b_MFXAudioDECODE_DecodeHeader);
msdk_ts_DECLARE_BLOCK(b_MFXAudioDECODE_Init);
msdk_ts_DECLARE_BLOCK(b_MFXAudioDECODE_Reset);
msdk_ts_DECLARE_BLOCK(b_MFXAudioDECODE_Close);
msdk_ts_DECLARE_BLOCK(b_MFXAudioDECODE_GetAudioParam);
msdk_ts_DECLARE_BLOCK(b_MFXAudioDECODE_DecodeFrameAsync);

/*verification*/
msdk_ts_DECLARE_BLOCK(v_CheckField);
msdk_ts_DECLARE_BLOCK(v_CheckField_NotEqual);
msdk_ts_DECLARE_BLOCK(v_CheckMfxRes);
msdk_ts_DECLARE_BLOCK(v_CheckMfxExtVPPDoUse);
