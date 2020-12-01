// Copyright (c) 2018-2020 Intel Corporation
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

#include <mfxvideo.h>

#include <mfx_session.h>
#include <mfx_trace.h>

#include "mediasdk_version.h"

#if (MFX_VERSION >= 1010)
  #define MFX_USE_VERSIONED_SESSION
#endif

// static section of the file
namespace
{

void* g_hModule = NULL; // DLL handle received in DllMain

} // namespace

mfxStatus MFXInit(mfxIMPL implParam, mfxVersion *ver, mfxSession *session)
{
    mfxInitParam par = {};

    par.Implementation = implParam;
    if (ver)
    {
        par.Version = *ver;
    }
    else
    {
        par.Version.Major = MFX_VERSION_MAJOR;
        par.Version.Minor = MFX_VERSION_MINOR;
    }
    par.ExternalThreads = 0;

    return MFXInitEx(par, session);

} // mfxStatus MFXInit(mfxIMPL impl, mfxVersion *ver, mfxHDL *session)

mfxStatus MFXInitEx(mfxInitParam par, mfxSession *session)
{
#if defined(MFX_USE_VERSIONED_SESSION)
    _mfxSession_1_10 * pSession = 0;
#else
    _mfxSession * pSession = 0;
#endif
    (void)g_hModule;
    mfxStatus mfxRes;
    int adapterNum = 0;
    mfxIMPL impl = par.Implementation & (MFX_IMPL_VIA_ANY - 1);
    mfxIMPL implInterface = par.Implementation & -MFX_IMPL_VIA_ANY;

    MFX_TRACE_INIT();
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_API, "ThreadName=MSDK app");
    }
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_API, "MFXInit");
    MFX_LTRACE_1(MFX_TRACE_LEVEL_API, "^ModuleHandle^libmfx=", "%p", g_hModule);

    // check the library version
    if ((MFX_VERSION_MAJOR != par.Version.Major) ||
        (MFX_VERSION_MINOR < par.Version.Minor))
    {
        return MFX_ERR_UNSUPPORTED;
    }

    // check error(s)
    if ((MFX_IMPL_AUTO != impl) &&
        (MFX_IMPL_AUTO_ANY != impl) &&
        (MFX_IMPL_HARDWARE_ANY != impl) &&
        (MFX_IMPL_HARDWARE != impl) &&
        (MFX_IMPL_HARDWARE2 != impl) &&
        (MFX_IMPL_HARDWARE3 != impl) &&
        (MFX_IMPL_HARDWARE4 != impl))
    {
        return MFX_ERR_UNSUPPORTED;
    }

    // if user did not specify MFX_IMPL_VIA_* treat it as MFX_IMPL_VIA_ANY
    if (!implInterface)
        implInterface = MFX_IMPL_VIA_ANY;

    if (!(implInterface & MFX_IMPL_AUDIO) &&
#if defined(MFX_VA_LINUX)
        (MFX_IMPL_VIA_VAAPI != implInterface) &&
#endif // MFX_VA_*
        (MFX_IMPL_VIA_ANY != implInterface))
    {
        return MFX_ERR_UNSUPPORTED;
    }

    // set the adapter number
    switch (impl)
    {
    case MFX_IMPL_HARDWARE2:
        adapterNum = 1;
        break;

    case MFX_IMPL_HARDWARE3:
        adapterNum = 2;
        break;

    case MFX_IMPL_HARDWARE4:
        adapterNum = 3;
        break;

    default:
        adapterNum = 0;
        break;

    }

    try
    {
        // reset output variable
        *session = 0;
        // prepare initialization parameters

        // create new session instance
#if defined(MFX_USE_VERSIONED_SESSION)
        pSession = new _mfxSession_1_10(adapterNum);
#else
        pSession = new _mfxSession(adapterNum);
#endif
        mfxInitParam init_param = par;
        init_param.Implementation = implInterface;

        mfxRes = pSession->InitEx(init_param);
    }
    catch(...)
    {
        mfxRes = MFX_ERR_MEMORY_ALLOC;
    }
    if (MFX_ERR_NONE != mfxRes &&
        MFX_WRN_PARTIAL_ACCELERATION != mfxRes)
    {
        if (pSession)
        {
            delete pSession;
        }

        return mfxRes;
    }

    // save the handle
    *session = dynamic_cast<_mfxSession *>(pSession);

    return mfxRes;

} // mfxStatus MFXInitEx(mfxInitParam par, mfxSession *session)

mfxStatus MFXDoWork(mfxSession session)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_API, "MFXDoWork");

    // check error(s)
    if (0 == session)
    {
        return MFX_ERR_INVALID_HANDLE;
    }

    MFXIUnknown * pInt = session->m_pScheduler;
    MFXIScheduler2 *newScheduler = 
        ::QueryInterface<MFXIScheduler2>(pInt, MFXIScheduler2_GUID);

    if (!newScheduler)
    {
        return MFX_ERR_UNSUPPORTED;
    }
    newScheduler->Release();

    mfxStatus res = newScheduler->DoWork();    

    return res;
} // mfxStatus MFXDoWork(mfxSession *session)

mfxStatus MFXClose(mfxSession session)
{
    mfxStatus mfxRes = MFX_ERR_NONE;

    // check error(s)
    MFX_CHECK(session, MFX_ERR_INVALID_HANDLE);

    try
    {
        // NOTE MFXClose function calls MFX_TRACE_CLOSE, so no tracing points should be
        // used after it. special care should be taken with MFX_AUTO_TRACE macro
        // since it inserts class variable on stack which calls to trace library in the
        // destructor.
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_API, "MFXClose");

        // parent session can't be closed,
        // because there is no way to let children know about parent's death.

        // child session should be uncoupled from the parent before closing.
        if (session->IsChildSession())
        {
            mfxRes = MFXDisjoinSession(session);
            MFX_CHECK_STS(mfxRes);
        }

        MFX_CHECK(!session->IsParentSession(), MFX_ERR_UNDEFINED_BEHAVIOR);

        // deallocate the object
#if defined(MFX_USE_VERSIONED_SESSION)
        _mfxSession_1_10 *newSession  = (_mfxSession_1_10 *)session;
        delete newSession;
#else
        delete session;
#endif
    }
    // handle error(s)
    catch(...)
    {
        // set the default error value
        mfxRes = MFX_ERR_UNKNOWN;
    }
#if defined(MFX_TRACE_ENABLE)
    MFX_TRACE_CLOSE();
#endif
    MFX_CHECK_STS(mfxRes);
    return mfxRes;

} // mfxStatus MFXClose(mfxHDL session)

void __attribute__ ((constructor)) dll_init(void)
{
}
