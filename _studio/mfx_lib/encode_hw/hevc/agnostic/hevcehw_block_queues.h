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

DEF_BLOCK_Q(Query0, Q0, void, mfxVideoParam&)
DEF_BLOCK_Q(Query1NoCaps, Q1NC, mfxStatus, const mfxVideoParam&, mfxVideoParam&, StorageRW&)
DEF_BLOCK_Q(Query1WithCaps, Q1WC, mfxStatus, const mfxVideoParam&, mfxVideoParam&, StorageRW&)
DEF_BLOCK_Q(QueryIOSurf, QIS, mfxStatus, const mfxVideoParam&, mfxFrameAllocRequest& request, StorageRW&)
DEF_BLOCK_Q(SetDefaults, SD, void, mfxVideoParam&, StorageW&, StorageRW&)
DEF_BLOCK_Q(InitExternal, IE, mfxStatus, const mfxVideoParam&, StorageRW&, StorageRW&)
DEF_BLOCK_Q(InitInternal, II, mfxStatus, StorageRW&, StorageRW&)
DEF_BLOCK_Q(InitAlloc, IA, mfxStatus, StorageRW&, StorageRW&)
DEF_BLOCK_Q(Reset, R, mfxStatus, const mfxVideoParam&, StorageRW&, StorageRW&)
DEF_BLOCK_Q(ResetState, RS, mfxStatus, StorageRW&, StorageRW&)
DEF_BLOCK_Q(FrameSubmit, FS, mfxStatus, mfxEncodeCtrl*, mfxFrameSurface1*, mfxBitstream&, StorageW&, StorageRW&)
DEF_BLOCK_Q(AsyncRoutine, AR, mfxStatus, StorageW&, StorageW&)
DEF_BLOCK_Q(AllocTask, AT, mfxStatus, StorageW&, StorageRW&)
DEF_BLOCK_Q(InitTask, IT, mfxStatus, mfxEncodeCtrl*, mfxFrameSurface1*, mfxBitstream*, StorageW&, StorageW&)
DEF_BLOCK_Q(PreReorderTask, PreRT, mfxStatus, StorageW&, StorageW&)
DEF_BLOCK_Q(PostReorderTask, PostRT, mfxStatus, StorageW&, StorageW&)
DEF_BLOCK_Q(SubmitTask, ST, mfxStatus, StorageW&, StorageW&)
DEF_BLOCK_Q(QueryTask, QT, mfxStatus, StorageW&, StorageW&)
DEF_BLOCK_Q(FreeTask, FT, mfxStatus, StorageW&, StorageW&)
DEF_BLOCK_Q(Close, CLS, void, StorageW&)
DEF_BLOCK_Q(GetVideoParam, GVP, void, mfxVideoParam&, StorageR&)
