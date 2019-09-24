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

#include "mfxvideo.h"
#if (MFX_VERSION_MAJOR == 1) && (MFX_VERSION_MINOR < 8)
#include "mfxlinux.h"
#endif

#include "common_utils.h"
#include "common_vaapi.h"

/* =====================================================
 * Linux implementation of OS-specific utility functions
 */

mfxStatus Initialize(mfxIMPL impl, mfxVersion ver, MFXVideoSession* pSession, mfxFrameAllocator* pmfxAllocator, bool bCreateSharedHandles)
{
    mfxStatus sts = MFX_ERR_NONE;

    // Initialize Intel Media SDK Session
    sts = pSession->Init(impl, &ver);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // Create VA display
    mfxHDL displayHandle = { 0 };
    sts = CreateVAEnvDRM(&displayHandle);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // Provide VA display handle to Media SDK
    sts = pSession->SetHandle(static_cast < mfxHandleType >(MFX_HANDLE_VA_DISPLAY), displayHandle);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // If mfxFrameAllocator is provided it means we need to setup  memory allocator
    if (pmfxAllocator) {
        pmfxAllocator->pthis  = *pSession; // We use Media SDK session ID as the allocation identifier
        pmfxAllocator->Alloc  = simple_alloc;
        pmfxAllocator->Free   = simple_free;
        pmfxAllocator->Lock   = simple_lock;
        pmfxAllocator->Unlock = simple_unlock;
        pmfxAllocator->GetHDL = simple_gethdl;

        // Since we are using video memory we must provide Media SDK with an external allocator
        sts = pSession->SetFrameAllocator(pmfxAllocator);
        MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
    }

    return sts;
}

void Release()
{
    CleanupVAEnvDRM();
}

void mfxGetTime(mfxTime* timestamp)
{
    clock_gettime(CLOCK_REALTIME, timestamp);
}

double TimeDiffMsec(mfxTime tfinish, mfxTime tstart)
{
    double result;
    long long elapsed_nsec = tfinish.tv_nsec - tstart.tv_nsec;
    long long elapsed_sec = tfinish.tv_sec - tstart.tv_sec;

    //if (tstart.tv_sec==0) return -1;

    //timespec uses two fields -- check if borrowing necessary
    if (elapsed_nsec < 0) {
        elapsed_sec -= 1;
        elapsed_nsec += 1000000000;
    }
    //return total converted to milliseconds
    result = (double)elapsed_sec *1000.0;
    result += (double)elapsed_nsec / 1000000;

    return result;
}

void ClearYUVSurfaceVMem(mfxMemId memId)
{
    ClearYUVSurfaceVAAPI(memId);
}

void ClearRGBSurfaceVMem(mfxMemId memId)
{
    ClearRGBSurfaceVAAPI(memId);
}
