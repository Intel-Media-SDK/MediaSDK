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

#pragma once

#include "ts_alloc.h"

class tsSession
{
public:
    bool                m_initialized;
    bool                m_sw_fallback;
    bool                m_is_handle_set;
    mfxSession          m_session;
    mfxIMPL             m_impl;
    mfxVersion          m_version;
    mfxSyncPoint        m_syncpoint;
    tsBufferAllocator   m_buffer_allocator;
    mfxSession*         m_pSession;
    mfxVersion*         m_pVersion;
    frame_allocator*    m_pFrameAllocator;
    frame_allocator*    m_pVAHandle;
    tsExtBufType<mfxInitParam> m_init_par;

    tsSession(mfxIMPL impl = g_tsImpl, mfxVersion version = g_tsVersion);
    ~tsSession();

    mfxStatus MFXInit();
    mfxStatus MFXInit(mfxIMPL impl, mfxVersion *ver, mfxSession *session);

    mfxStatus MFXClose();
    mfxStatus MFXClose(mfxSession session);

    mfxStatus SyncOperation();
    mfxStatus SyncOperation(mfxSession session, mfxSyncPoint syncp, mfxU32 wait);

    mfxStatus SetFrameAllocator();
    mfxStatus SetFrameAllocator(mfxSession session, mfxFrameAllocator *allocator);

    mfxStatus MFX_CDECL MFXQueryIMPL(mfxSession session, mfxIMPL *impl);
    mfxStatus MFXQueryVersion(mfxSession session, mfxVersion *version);

    mfxStatus Load(mfxSession session, const mfxPluginUID *uid, mfxU32 version);
    mfxStatus UnLoad(mfxSession session, const mfxPluginUID *uid);
    mfxStatus SetHandle(mfxSession session, mfxHandleType type, mfxHDL handle);
    mfxStatus GetPlugin(mfxSession session, mfxU32 type, mfxPlugin *plugin);

    mfxStatus MFXInitEx();
    mfxStatus MFXInitEx(mfxInitParam par, mfxSession* session);
    mfxStatus MFXDoWork(mfxSession session);

    mfxStatus MFXJoinSession(mfxSession session);
    mfxStatus MFXDisjoinSession();
};

#define IS_FALLBACK_EXPECTED(b_fallback, status)          \
    if(b_fallback && (MFX_ERR_NONE == status.m_expected)) \
        status.expect(MFX_WRN_PARTIAL_ACCELERATION);
