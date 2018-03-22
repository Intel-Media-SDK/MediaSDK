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

#include <mfxvideo.h>

#include <mfx_session.h>
#include <mfx_trace.h>


#if (MFX_VERSION_MAJOR == 1) && (MFX_VERSION_MINOR >= 10)
  #define MFX_USE_VERSIONED_SESSION
#endif


// static section of the file
namespace
{

void* g_hModule = NULL; // DLL handle received in DllMain

} // namespace


/* These string constants set Media SDK version information for Linux, Android, OSX. */
#ifndef MFX_FILE_VERSION
#define MFX_FILE_VERSION "0.0.0.0"
#endif
#ifndef MFX_PRODUCT_VERSION
#define MFX_PRODUCT_VERSION "0.0.000.0000"
#endif

// Copyright strings
#if defined(mfxhw64_EXPORTS) || defined(mfxhw32_EXPORTS) || defined(mfxsw64_EXPORTS) || defined(mfxsw32_EXPORTS)

#if defined(LINUX_TARGET_PLATFORM_BDW)
const char* g_MfxProductName = "mediasdk_product_name: Intel(R) Media SDK";
#elif defined(LINUX_TARGET_PLATFORM_BXT) || defined (LINUX_TARGET_PLATFORM_BXTMIN)
const char* g_MfxProductName = "mediasdk_product_name: Intel(R) Media SDK 2017 for Embedded Linux";
#else
const char* g_MfxProductName = "mediasdk_product_name: Intel(R) Media Server Studio - SDK for Linux*";
#endif

const char* g_MfxCopyright = "mediasdk_copyright: Copyright(c) 2017 Intel Corporation";
const char* g_MfxFileVersion = "mediasdk_file_version: " MFX_FILE_VERSION;
const char* g_MfxProductVersion = "mediasdk_product_version: " MFX_PRODUCT_VERSION;

#endif // mfxhwXX_EXPORTS
#if defined(mfxaudiosw64_EXPORTS) || defined(mfxaudiosw32_EXPORTS)
#if defined(LINUX_TARGET_PLATFORM_BDW)
const char* g_MfxProductName = "mediasdk_product_name: Intel(R) Media Server Studio 2018 - Audio for Linux*";
#elif defined(LINUX_TARGET_PLATFORM_BXT) || defined (LINUX_TARGET_PLATFORM_BXTMIN)
const char* g_MfxProductName = "mediasdk_product_name: Intel(R) Media SDK 2017 for Embedded Linux";
#else
const char* g_MfxProductName = "mediasdk_product_name: Intel(R) Media Server Studio - Audio for Linux*";
#endif

const char* g_MfxCopyright = "mediasdk_copyright: Copyright(c) 2014-2018 Intel Corporation";
const char* g_MfxFileVersion = "mediasdk_file_version: " MFX_FILE_VERSION;
const char* g_MfxProductVersion = "mediasdk_product_version: " MFX_PRODUCT_VERSION;

#endif // mfxaudioswXX_EXPORTS


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
    mfxVersion libver;
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


    if (!(implInterface & MFX_IMPL_AUDIO) &&
        (0 != implInterface) &&
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

        //mfxRes = pSession->Init(implInterface, &par.Version);
        mfxRes = pSession->InitEx(init_param);

        // check the library version
        MFXQueryVersion(pSession, &libver);

        // check the numbers
        if ((libver.Major != par.Version.Major) ||
            (libver.Minor < par.Version.Minor))
        {
            mfxRes = MFX_ERR_UNSUPPORTED;
        }
    }
    catch(MFX_CORE_CATCH_TYPE)
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
    if (0 == session)
    {
        return MFX_ERR_INVALID_HANDLE;
    }

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
            if (MFX_ERR_NONE != mfxRes)
            {
                return mfxRes;
            }
        }
        
        if (session->IsParentSession())
        {
            return MFX_ERR_UNDEFINED_BEHAVIOR;
        }
        
        // deallocate the object
#if defined(MFX_USE_VERSIONED_SESSION)
        _mfxSession_1_10 *newSession  = (_mfxSession_1_10 *)session;
        delete newSession;
#else
        delete session;
#endif
    }
    // handle error(s)
    catch(MFX_CORE_CATCH_TYPE)
    {
        // set the default error value
        mfxRes = MFX_ERR_UNKNOWN;
        if (0 == session)
        {
            mfxRes = MFX_ERR_INVALID_HANDLE;
        }
    }
#if defined(MFX_TRACE_ENABLE)
    MFX_TRACE_CLOSE();
#endif
    return mfxRes;

} // mfxStatus MFXClose(mfxHDL session)

void __attribute__ ((constructor)) dll_init(void)
{
}
