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

#ifdef CALLBACKS_COMMON
FUNCTION(mfxFrameAllocator, mfxStatus, Alloc, (mfxHDL pthis, mfxFrameAllocRequest *request, mfxFrameAllocResponse *response), (pthis, request, response))
FUNCTION(mfxFrameAllocator, mfxStatus, Lock, (mfxHDL pthis, mfxMemId mid, mfxFrameData *ptr), (pthis, mid, ptr))
FUNCTION(mfxFrameAllocator, mfxStatus, Unlock, (mfxHDL pthis, mfxMemId mid, mfxFrameData *ptr), (pthis, mid, ptr))
FUNCTION(mfxFrameAllocator, mfxStatus, GetHDL, (mfxHDL pthis, mfxMemId mid, mfxHDL *handle), (pthis, mid, handle))
FUNCTION(mfxFrameAllocator, mfxStatus, Free, (mfxHDL pthis, mfxFrameAllocResponse *response), (pthis, response))

FUNCTION(mfxExtBRC, mfxStatus, Init, (mfxHDL pthis, mfxVideoParam* par), (pthis, par))
FUNCTION(mfxExtBRC, mfxStatus, Reset, (mfxHDL pthis, mfxVideoParam* par), (pthis, par))
FUNCTION(mfxExtBRC, mfxStatus, Close, (mfxHDL pthis), (pthis))
FUNCTION(mfxExtBRC, mfxStatus, GetFrameCtrl, (mfxHDL pthis, mfxBRCFrameParam* par, mfxBRCFrameCtrl* ctrl), (pthis, par, ctrl))
FUNCTION(mfxExtBRC, mfxStatus, Update, (mfxHDL pthis, mfxBRCFrameParam* par, mfxBRCFrameCtrl* ctrl, mfxBRCFrameStatus* status), (pthis, par, ctrl, status))

FUNCTION(mfxCoreInterface, mfxStatus, GetCoreParam, (mfxHDL pthis, mfxCoreParam *par), (pthis, par))
FUNCTION(mfxCoreInterface, mfxStatus, GetHandle, (mfxHDL pthis, mfxHandleType type, mfxHDL *handle), (pthis, type, handle))
FUNCTION(mfxCoreInterface, mfxStatus, IncreaseReference, (mfxHDL pthis, mfxFrameData *fd), (pthis, fd))
FUNCTION(mfxCoreInterface, mfxStatus, DecreaseReference, (mfxHDL pthis, mfxFrameData *fd), (pthis, fd))
FUNCTION(mfxCoreInterface, mfxStatus, CopyFrame, (mfxHDL pthis, mfxFrameSurface1 *dst, mfxFrameSurface1 *src), (pthis, dst, src))
FUNCTION(mfxCoreInterface, mfxStatus, CopyBuffer, (mfxHDL pthis, mfxU8 *dst, mfxU32 size, mfxFrameSurface1 *src), (pthis, dst, size, src))
FUNCTION(mfxCoreInterface, mfxStatus, MapOpaqueSurface, (mfxHDL pthis, mfxU32  num, mfxU32  type, mfxFrameSurface1 **op_surf), (pthis, num, type, op_surf))
FUNCTION(mfxCoreInterface, mfxStatus, UnmapOpaqueSurface, (mfxHDL pthis, mfxU32  num, mfxU32  type, mfxFrameSurface1 **op_surf), (pthis, num, type, op_surf))
FUNCTION(mfxCoreInterface, mfxStatus, GetRealSurface, (mfxHDL pthis, mfxFrameSurface1 *op_surf, mfxFrameSurface1 **surf), (pthis, op_surf, surf))
FUNCTION(mfxCoreInterface, mfxStatus, GetOpaqueSurface, (mfxHDL pthis, mfxFrameSurface1 *surf, mfxFrameSurface1 **op_surf), (pthis, surf, op_surf))
FUNCTION(mfxCoreInterface, mfxStatus, CreateAccelerationDevice, (mfxHDL pthis, mfxHandleType type, mfxHDL *handle), (pthis, type, handle))
FUNCTION(mfxCoreInterface, mfxStatus, GetFrameHandle, (mfxHDL pthis, mfxFrameData *fd, mfxHDL *handle), (pthis, fd, handle))
FUNCTION(mfxCoreInterface, mfxStatus, QueryPlatform, (mfxHDL pthis, mfxPlatform *platform), (pthis, platform))
#endif // CALLBACKS_COMMON

#ifdef CALLBACKS_PLUGIN
FUNCTION(mfxPlugin, mfxStatus, PluginInit, (mfxHDL pthis, mfxCoreInterface *core), (pthis, core))
FUNCTION(mfxPlugin, mfxStatus, PluginClose, (mfxHDL pthis), (pthis))
FUNCTION(mfxPlugin, mfxStatus, GetPluginParam, (mfxHDL pthis, mfxPluginParam *par), (pthis, par))
FUNCTION(mfxPlugin, mfxStatus, Submit, (mfxHDL pthis, const mfxHDL *in, mfxU32 in_num, const mfxHDL *out, mfxU32 out_num, mfxThreadTask *task), (pthis, in, in_num, out, out_num, task))
FUNCTION(mfxPlugin, mfxStatus, Execute, (mfxHDL pthis, mfxThreadTask task, mfxU32 uid_p, mfxU32 uid_a), (pthis, task, uid_p, uid_a))
FUNCTION(mfxPlugin, mfxStatus, FreeResources, (mfxHDL pthis, mfxThreadTask task, mfxStatus sts), (pthis, task, sts))

FUNCTION(mfxVideoCodecPlugin, mfxStatus, Query, (mfxHDL pthis, mfxVideoParam *in, mfxVideoParam *out), (pthis, in, out))
FUNCTION(mfxVideoCodecPlugin, mfxStatus, QueryIOSurf, (mfxHDL pthis, mfxVideoParam *par, mfxFrameAllocRequest *in, mfxFrameAllocRequest *out), (pthis, par, in, out))
FUNCTION(mfxVideoCodecPlugin, mfxStatus, Init, (mfxHDL pthis, mfxVideoParam *par), (pthis, par))
FUNCTION(mfxVideoCodecPlugin, mfxStatus, Reset, (mfxHDL pthis, mfxVideoParam *par), (pthis, par))
FUNCTION(mfxVideoCodecPlugin, mfxStatus, Close, (mfxHDL pthis), (pthis))
FUNCTION(mfxVideoCodecPlugin, mfxStatus, GetVideoParam, (mfxHDL pthis, mfxVideoParam *par), (pthis, par))
FUNCTION(mfxVideoCodecPlugin, mfxStatus, EncodeFrameSubmit, (mfxHDL pthis, mfxEncodeCtrl *ctrl, mfxFrameSurface1 *surface, mfxBitstream *bs, mfxThreadTask *task), (pthis, ctrl, surface, bs, task))
FUNCTION(mfxVideoCodecPlugin, mfxStatus, DecodeHeader, (mfxHDL pthis, mfxBitstream *bs, mfxVideoParam *par), (pthis, bs, par))
FUNCTION(mfxVideoCodecPlugin, mfxStatus, GetPayload, (mfxHDL pthis, mfxU64 *ts, mfxPayload *payload), (pthis, ts, payload))
FUNCTION(mfxVideoCodecPlugin, mfxStatus, DecodeFrameSubmit, (mfxHDL pthis, mfxBitstream *bs, mfxFrameSurface1 *surface_work, mfxFrameSurface1 **surface_out, mfxThreadTask *task), (pthis, bs, surface_work, surface_out, task))
FUNCTION(mfxVideoCodecPlugin, mfxStatus, VPPFrameSubmit, (mfxHDL pthis, mfxFrameSurface1 *in, mfxFrameSurface1 *out, mfxExtVppAuxData *aux, mfxThreadTask *task), (pthis, in, out, aux, task))
FUNCTION(mfxVideoCodecPlugin, mfxStatus, VPPFrameSubmitEx, (mfxHDL pthis, mfxFrameSurface1 *in, mfxFrameSurface1 *surface_work, mfxFrameSurface1 **surface_out, mfxThreadTask *task), (pthis, in, surface_work, surface_out, task))
FUNCTION(mfxVideoCodecPlugin, mfxStatus, ENCFrameSubmit, (mfxHDL pthis, mfxENCInput *in, mfxENCOutput *out, mfxThreadTask *task), (pthis, in, out, task))
#endif // CALLBACKS_PLUGIN
