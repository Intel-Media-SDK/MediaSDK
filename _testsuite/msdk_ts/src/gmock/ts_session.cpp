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

#include "ts_session.h"

tsSession::tsSession(mfxIMPL impl, mfxVersion version)
    : m_initialized(false)
    , m_sw_fallback(false)
    , m_is_handle_set(false)
    , m_session(0)
    , m_impl(impl)
    , m_version(version)
    , m_syncpoint(0)
    , m_pSession(&m_session)
    , m_pVersion(&m_version)
    , m_pFrameAllocator(0)
    , m_pVAHandle(0)
    , m_init_par()
{
}

tsSession::~tsSession()
{
    if(m_initialized)
    {
        MFXClose();
    }
#if (defined(LINUX32) || defined(LINUX64))
    if (m_pVAHandle)
    {
        delete m_pVAHandle;
    }
#endif
}

mfxStatus tsSession::MFXInit()
{
    return MFXInit(m_impl, m_pVersion, m_pSession);
}

mfxStatus tsSession::MFXInit(mfxIMPL impl, mfxVersion *ver, mfxSession *session)
{
    TRACE_FUNC3( MFXInit, impl, ver, session );
    g_tsStatus.check(::MFXInit(impl, ver, session));
    TS_TRACE(ver);
    TS_TRACE(session);

    m_initialized = (g_tsStatus.get() >= 0);

#if (defined(LINUX32) || defined(LINUX64))
    // SetHandle on Linux is always required for HW
    if (m_initialized && g_tsImpl != MFX_IMPL_SOFTWARE)
    {
        mfxHDL hdl;
        mfxHandleType type;
        if (!m_pVAHandle)
        {
            m_pVAHandle = new frame_allocator(
                    frame_allocator::HARDWARE,
                    frame_allocator::ALLOC_MAX,
                    frame_allocator::ENABLE_ALL,
                    frame_allocator::ALLOC_EMPTY);
        }
        m_pVAHandle->get_hdl(type, hdl);
        SetHandle(m_session, type, hdl);
        m_is_handle_set = (g_tsStatus.get() >= 0);
    }
#endif

    return g_tsStatus.get();
}

mfxStatus tsSession::MFXClose()
{
    return MFXClose(m_session);
}

mfxStatus tsSession::MFXClose(mfxSession session)
{
    TRACE_FUNC1( MFXClose, session );
    g_tsStatus.check(::MFXClose(session));
    m_initialized = false;

    return g_tsStatus.get();
}

mfxStatus tsSession::SyncOperation()
{
    return SyncOperation(m_session, m_syncpoint, MFX_INFINITE);
}

mfxStatus tsSession::SyncOperation(mfxSession session, mfxSyncPoint syncp, mfxU32 wait)
{
    TRACE_FUNC3(MFXVideoCORE_SyncOperation, session, syncp, wait);
    g_tsStatus.check( MFXVideoCORE_SyncOperation(session, syncp, wait) );

    return g_tsStatus.get();
}

mfxStatus tsSession::SetFrameAllocator()
{
    return SetFrameAllocator(m_session, m_pFrameAllocator);
}

mfxStatus tsSession::SetFrameAllocator(mfxSession session, mfxFrameAllocator *allocator)
{
    TRACE_FUNC2(MFXVideoCORE_SetFrameAllocator, session, allocator);
    g_tsStatus.check( MFXVideoCORE_SetFrameAllocator(session, allocator) );
    return g_tsStatus.get();
}

mfxStatus tsSession::Load(mfxSession session, const mfxPluginUID *uid, mfxU32 version)
{
    TRACE_FUNC3(MFXVideoUSER_Load, session, uid, version);
    g_tsStatus.check( MFXVideoUSER_Load(session, uid, version) );
    return g_tsStatus.get();
}

mfxStatus tsSession::UnLoad(mfxSession session, const mfxPluginUID *uid)
{
    TRACE_FUNC2(MFXVideoUSER_UnLoad, session, uid);
    g_tsStatus.check( MFXVideoUSER_UnLoad(session, uid) );
    return g_tsStatus.get();
}

mfxStatus tsSession::SetHandle(mfxSession session, mfxHandleType type, mfxHDL handle)
{
    TRACE_FUNC3(MFXVideoCORE_SetHandle, session, type, handle);
    g_tsStatus.check( MFXVideoCORE_SetHandle(session, type, handle) );
    m_is_handle_set = (g_tsStatus.get() >= 0);
    return g_tsStatus.get();
}

mfxStatus tsSession::GetPlugin(mfxSession session, mfxU32 type, mfxPlugin *plugin)
{
    TRACE_FUNC3(MFXVideoUSER_GetPlugin, session, type, plugin);
    g_tsStatus.check(MFXVideoUSER_GetPlugin(session, type, plugin));
    return g_tsStatus.get();
}

mfxStatus tsSession::MFXQueryVersion(mfxSession session, mfxVersion *version)
{
    TRACE_FUNC2(MFXQueryVersion, session, version);
    g_tsStatus.check( ::MFXQueryVersion(session, version) );
    return g_tsStatus.get();
}

mfxStatus tsSession::MFXInitEx()
{
    return MFXInitEx(m_init_par, &m_session);
}

mfxStatus tsSession::MFXInitEx(mfxInitParam par, mfxSession* session)
{
    TRACE_FUNC2(mfxInitParam, par, session);
    g_tsStatus.check( ::MFXInitEx(par, session) );
    return g_tsStatus.get();
}

mfxStatus tsSession::MFXDoWork(mfxSession session)
{
    TRACE_FUNC1(MFXDoWork, session);
    g_tsStatus.check( ::MFXDoWork(session) );
    return g_tsStatus.get();
}

mfxStatus tsSession::MFXJoinSession(mfxSession session)
{
    TRACE_FUNC2(MFXJoinSession, m_session, session);
    g_tsStatus.check( ::MFXJoinSession(m_session, session) );
    return g_tsStatus.get();
}

mfxStatus tsSession::MFXDisjoinSession()
{
    TRACE_FUNC1(MFXJoinSession, m_session);
    g_tsStatus.check( ::MFXDisjoinSession(m_session) );
    return g_tsStatus.get();
}
