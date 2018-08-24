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

/*
 * API version 1.0 functions
 */

FUNCTION(mfxStatus, MFXQueryIMPL, (mfxSession session, mfxIMPL *impl), (session, impl))
FUNCTION(mfxStatus, MFXQueryVersion, (mfxSession session, mfxVersion *version), (session, version))

// CORE interface functions
FUNCTION(mfxStatus, MFXVideoCORE_SetBufferAllocator, (mfxSession session, mfxBufferAllocator *allocator), (session, allocator))
FUNCTION(mfxStatus, MFXVideoCORE_SetFrameAllocator, (mfxSession session, mfxFrameAllocator *allocator), (session, allocator))
FUNCTION(mfxStatus, MFXVideoCORE_SetHandle, (mfxSession session, mfxHandleType type, mfxHDL hdl), (session, type, hdl))
FUNCTION(mfxStatus, MFXVideoCORE_GetHandle, (mfxSession session, mfxHandleType type, mfxHDL *hdl), (session, type, hdl))

FUNCTION(mfxStatus, MFXVideoCORE_SyncOperation, (mfxSession session, mfxSyncPoint syncp, mfxU32 wait), (session, syncp, wait))

// ENCODE interface functions
FUNCTION(mfxStatus, MFXVideoENCODE_Query, (mfxSession session, mfxVideoParam *in, mfxVideoParam *out), (session, in, out))
FUNCTION(mfxStatus, MFXVideoENCODE_QueryIOSurf, (mfxSession session, mfxVideoParam *par, mfxFrameAllocRequest *request), (session, par, request))
FUNCTION(mfxStatus, MFXVideoENCODE_Init, (mfxSession session, mfxVideoParam *par), (session, par))
FUNCTION(mfxStatus, MFXVideoENCODE_Reset, (mfxSession session, mfxVideoParam *par), (session, par))
FUNCTION(mfxStatus, MFXVideoENCODE_Close, (mfxSession session), (session))

FUNCTION(mfxStatus, MFXVideoENCODE_GetVideoParam, (mfxSession session, mfxVideoParam *par), (session, par))
FUNCTION(mfxStatus, MFXVideoENCODE_GetEncodeStat, (mfxSession session, mfxEncodeStat *stat), (session, stat))
FUNCTION(mfxStatus, MFXVideoENCODE_EncodeFrameAsync, (mfxSession session, mfxEncodeCtrl *ctrl, mfxFrameSurface1 *surface, mfxBitstream *bs, mfxSyncPoint *syncp), (session, ctrl, surface, bs, syncp))

// DECODE interface functions
FUNCTION(mfxStatus, MFXVideoDECODE_Query, (mfxSession session, mfxVideoParam *in, mfxVideoParam *out), (session, in, out))
FUNCTION(mfxStatus, MFXVideoDECODE_DecodeHeader, (mfxSession session, mfxBitstream *bs, mfxVideoParam *par), (session, bs, par))
FUNCTION(mfxStatus, MFXVideoDECODE_QueryIOSurf, (mfxSession session, mfxVideoParam *par, mfxFrameAllocRequest *request), (session, par, request))
FUNCTION(mfxStatus, MFXVideoDECODE_Init, (mfxSession session, mfxVideoParam *par), (session, par))
FUNCTION(mfxStatus, MFXVideoDECODE_Reset, (mfxSession session, mfxVideoParam *par), (session, par))
FUNCTION(mfxStatus, MFXVideoDECODE_Close, (mfxSession session), (session))

FUNCTION(mfxStatus, MFXVideoDECODE_GetVideoParam, (mfxSession session, mfxVideoParam *par), (session, par))
FUNCTION(mfxStatus, MFXVideoDECODE_GetDecodeStat, (mfxSession session, mfxDecodeStat *stat), (session, stat))
FUNCTION(mfxStatus, MFXVideoDECODE_SetSkipMode, (mfxSession session, mfxSkipMode mode), (session, mode))
FUNCTION(mfxStatus, MFXVideoDECODE_GetPayload, (mfxSession session, mfxU64 *ts, mfxPayload *payload), (session, ts, payload))
FUNCTION(mfxStatus, MFXVideoDECODE_DecodeFrameAsync, (mfxSession session, mfxBitstream *bs, mfxFrameSurface1 *surface_work, mfxFrameSurface1 **surface_out, mfxSyncPoint *syncp), (session, bs, surface_work, surface_out, syncp))

// VPP interface functions
FUNCTION(mfxStatus, MFXVideoVPP_Query, (mfxSession session, mfxVideoParam *in, mfxVideoParam *out), (session, in, out))
FUNCTION(mfxStatus, MFXVideoVPP_QueryIOSurf, (mfxSession session, mfxVideoParam *par, mfxFrameAllocRequest *request), (session, par, request))
FUNCTION(mfxStatus, MFXVideoVPP_Init, (mfxSession session, mfxVideoParam *par), (session, par))
FUNCTION(mfxStatus, MFXVideoVPP_Reset, (mfxSession session, mfxVideoParam *par), (session, par))
FUNCTION(mfxStatus, MFXVideoVPP_Close, (mfxSession session), (session))

FUNCTION(mfxStatus, MFXVideoVPP_GetVideoParam, (mfxSession session, mfxVideoParam *par), (session, par))
FUNCTION(mfxStatus, MFXVideoVPP_GetVPPStat, (mfxSession session, mfxVPPStat *stat), (session, stat))
FUNCTION(mfxStatus, MFXVideoVPP_RunFrameVPPAsync, (mfxSession session, mfxFrameSurface1 *in, mfxFrameSurface1 *out, mfxExtVppAuxData *aux, mfxSyncPoint *syncp), (session, in, out, aux, syncp))

/*
 * API version 1.1 functions
 */
FUNCTION(mfxStatus, MFXJoinSession, (mfxSession session, mfxSession child_session), (session, child_session))
FUNCTION(mfxStatus, MFXCloneSession, (mfxSession session, mfxSession *clone), (session, clone))
FUNCTION(mfxStatus, MFXDisjoinSession, (mfxSession session), (session))
FUNCTION(mfxStatus, MFXSetPriority, (mfxSession session, mfxPriority priority), (session, priority))
FUNCTION(mfxStatus, MFXGetPriority, (mfxSession session, mfxPriority *priority), (session, priority))

FUNCTION(mfxStatus, MFXVideoUSER_Register, (mfxSession session, mfxU32 type, const mfxPlugin *par), (session, type, par))
FUNCTION(mfxStatus, MFXVideoUSER_Unregister, (mfxSession session, mfxU32 type), (session, type))
FUNCTION(mfxStatus, MFXVideoUSER_ProcessFrameAsync, (mfxSession session, const mfxHDL *in, mfxU32 in_num, const mfxHDL *out, mfxU32 out_num, mfxSyncPoint *syncp), (session, in, in_num, out, out_num, syncp))

/*
 * API version 1.10 functions
 */
FUNCTION(mfxStatus, MFXVideoENC_Query,(mfxSession session, mfxVideoParam *in, mfxVideoParam *out), (session, in, out))
FUNCTION(mfxStatus, MFXVideoENC_QueryIOSurf,(mfxSession session, mfxVideoParam *par, mfxFrameAllocRequest *request), (session, par, request))
FUNCTION(mfxStatus, MFXVideoENC_Init,(mfxSession session, mfxVideoParam *par), (session, par))
FUNCTION(mfxStatus, MFXVideoENC_Reset,(mfxSession session, mfxVideoParam *par), (session, par))
FUNCTION(mfxStatus, MFXVideoENC_Close,(mfxSession session),(session))
FUNCTION(mfxStatus, MFXVideoENC_ProcessFrameAsync,(mfxSession session, mfxENCInput *in, mfxENCOutput *out, mfxSyncPoint *syncp),(session, in, out, syncp))

FUNCTION(mfxStatus, MFXVideoVPP_RunFrameVPPAsyncEx, (mfxSession session, mfxFrameSurface1 *in, mfxFrameSurface1 *work, mfxFrameSurface1 **out, mfxSyncPoint *syncp), (session, in, work, out, syncp))

/*
 * API version 1.13 functions
 */

FUNCTION(mfxStatus, MFXVideoPAK_Query, (mfxSession session, mfxVideoParam *in, mfxVideoParam *out), (session, in, out))
FUNCTION(mfxStatus, MFXVideoPAK_QueryIOSurf, (mfxSession session, mfxVideoParam *par, mfxFrameAllocRequest *request), (session, par, request))
FUNCTION(mfxStatus, MFXVideoPAK_Init, (mfxSession session, mfxVideoParam *par), (session, par))
FUNCTION(mfxStatus, MFXVideoPAK_Reset, (mfxSession session, mfxVideoParam *par), (session, par))
FUNCTION(mfxStatus, MFXVideoPAK_Close, (mfxSession session), (session))
FUNCTION(mfxStatus, MFXVideoPAK_ProcessFrameAsync, (mfxSession session, mfxPAKInput *in, mfxPAKOutput *out, mfxSyncPoint *syncp), (session, in, out, syncp))

/*
* API version 1.14 functions
*/
FUNCTION(mfxStatus, MFXDoWork, (mfxSession session), (session))
FUNCTION(mfxStatus, MFXInitEx, (mfxInitParam par, mfxSession session), (par, session))

/*
* API version 1.19 functions
*/

FUNCTION(mfxStatus, MFXVideoENC_GetVideoParam, (mfxSession session, mfxVideoParam *par), (session, par))
FUNCTION(mfxStatus, MFXVideoPAK_GetVideoParam, (mfxSession session, mfxVideoParam *par), (session, par))
FUNCTION(mfxStatus, MFXVideoCORE_QueryPlatform, (mfxSession session, mfxPlatform* platform), (session, platform))
FUNCTION(mfxStatus, MFXVideoUSER_GetPlugin, (mfxSession session, mfxU32 type, mfxPlugin *par), (session, type, par))