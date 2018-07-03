// Copyright (c) 2017-2018 Intel Corporation
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

#ifndef __CM_MEM_COPY_H__
#define __CM_MEM_COPY_H__

#include "mfxdefs.h"
#include "mfxstructures.h"
#include "skl_copy_kernel_genx_isa.h"
#if !(defined(AS_VPP_PLUGIN) || defined(UNIFIED_PLUGIN) || defined(AS_H264LA_PLUGIN))
#include "cht_copy_kernel_genx_isa.h"
#endif
#include "cnl_copy_kernel_genx_isa.h"
#include "icl_copy_kernel_genx_isa.h"
#include "icllp_copy_kernel_genx_isa.h"


#ifdef _MSVC_LANG
#pragma warning(disable: 4505)
#pragma warning(disable: 4100)
#pragma warning(disable: 4201)
#endif

#include "umc_mutex.h"

#include <algorithm>
#include <set>
#include <map>
#include <vector>

#include "cmrt_cross_platform.h"

typedef mfxI32 cmStatus;

#define BLOCK_PIXEL_WIDTH   (32)
#define BLOCK_HEIGHT        (8)

#define CM_MAX_GPUCOPY_SURFACE_WIDTH_IN_BYTE 65408
#define CM_MAX_GPUCOPY_SURFACE_HEIGHT        4088

#define CM_SUPPORTED_COPY_SIZE(ROI) (ROI.width <= CM_MAX_GPUCOPY_SURFACE_WIDTH_IN_BYTE && ROI.height <= CM_MAX_GPUCOPY_SURFACE_HEIGHT )
#define CM_ALIGNED(PTR) (!((mfxU64(PTR))&0xf))
#define CM_ALIGNED64(PTR) (!((mfxU64(PTR))&0x3f))

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

static bool operator < (const mfxHDLPair & l, const mfxHDLPair & r)
{
    return (l.first == r.first) ? (l.second < r.second) : (l.first < r.first);
};

class CmDevice;
class CmBuffer;
class CmBufferUP;
class CmSurface2D;
class CmEvent;
class CmQueue;
class CmProgram;
class CmKernel;
class SurfaceIndex;
class CmThreadSpace;
class CmTask;
struct IDirect3DSurface9;
struct IDirect3DDeviceManager9;


class CmCopyWrapper
{
public:

    // constructor
    CmCopyWrapper();

    // destructor
    virtual ~CmCopyWrapper(void);

    template <typename D3DAbstract>
    CmDevice* GetCmDevice(D3DAbstract *pD3D)
    {
        cmStatus cmSts = CM_SUCCESS;
        mfxU32 version;

        if (m_pCmDevice)
            return m_pCmDevice;

        cmSts = ::CreateCmDevice(m_pCmDevice, version, pD3D, CM_DEVICE_CREATE_OPTION_SCRATCH_SPACE_DISABLE);
        if (cmSts != CM_SUCCESS)
            return NULL;

        if (CM_1_0 > version)
        {
            return NULL;
        }

        return m_pCmDevice;
    };

#if defined(MFX_VA_LINUX)
    CmDevice* GetCmDevice(VADisplay dpy)
    {
        cmStatus cmSts = CM_SUCCESS;
        mfxU32 version;

        if (m_pCmDevice)
            return m_pCmDevice;

        cmSts = ::CreateCmDevice(m_pCmDevice, version, dpy, CM_DEVICE_CREATE_OPTION_SCRATCH_SPACE_DISABLE);
        if (cmSts != CM_SUCCESS)
            return NULL;

        if (CM_1_0 > version)
        {
            return NULL;
        }
        return m_pCmDevice;
    };
#endif

    // initialize available functionality
    mfxStatus Initialize(eMFXHWType hwtype = MFX_HW_UNKNOWN);
    mfxStatus InitializeSwapKernels(eMFXHWType hwtype = MFX_HW_UNKNOWN);

    // release object
    mfxStatus Release(void);

    // check input parameters
    mfxStatus IsCmCopySupported(mfxFrameSurface1 *pSurface, mfxSize roi);

    static bool CanUseCmCopy(mfxFrameSurface1 *pDst, mfxFrameSurface1 *pSrc);
    static bool isSinglePlainFormat(mfxU32 format);
    static bool isNeedSwapping(mfxFrameSurface1 *pDst, mfxFrameSurface1 *pSrc);
    static bool isNeedShift(mfxFrameSurface1 *pDst, mfxFrameSurface1 *pSrc);
    static bool isNV12LikeFormat(mfxU32 format);
    static int  getSizePerPixel(mfxU32 format);
    mfxStatus CopyVideoToVideo(mfxFrameSurface1 *pDst, mfxFrameSurface1 *pSrc);
    mfxStatus CopySysToVideo(mfxFrameSurface1 *pDst, mfxFrameSurface1 *pSrc);
    mfxStatus CopyVideoToSys(mfxFrameSurface1 *pDst, mfxFrameSurface1 *pSrc);

    mfxStatus CopyVideoToSystemMemoryAPI(mfxU8 *pDst, mfxU32 dstPitch, mfxU32 dstUVOffset, void *pSrc, mfxU32 srcPitch, mfxSize roi);
    mfxStatus CopyVideoToSystemMemory(mfxU8 *pDst, mfxU32 dstPitch, mfxU32 dstUVOffset, void *pSrc, mfxU32 srcPitch, mfxSize roi, mfxU32 format);

    mfxStatus CopySystemToVideoMemoryAPI(void *pDst, mfxU32 dstPitch, mfxU8 *pSrc, mfxU32 srcPitch, mfxU32 srcUVOffset, mfxSize roi);
    mfxStatus CopySystemToVideoMemory(void *pDst, mfxU32 dstPitch, mfxU8 *pSrc, mfxU32 srcPitch, mfxU32 srcUVOffset, mfxSize roi, mfxU32 format);
    mfxStatus CopyVideoToVideoMemoryAPI(void *pDst, void *pSrc, mfxSize roi);

    mfxStatus CopySwapVideoToSystemMemory(mfxU8 *pDst, mfxU32 dstPitch, mfxU32 dstUVOffset, void *pSrc, mfxU32 srcPitch, mfxSize roi, mfxU32 format);
    mfxStatus CopySwapSystemToVideoMemory(void *pDst, mfxU32 dstPitch, mfxU8 *pSrc, mfxU32 srcPitch, mfxU32 srcUVOffset, mfxSize roi, mfxU32 format);
    mfxStatus CopyShiftSystemToVideoMemory(void *pDst, mfxU32 dstPitch, mfxU8 *pSrc, mfxU32 srcPitch, mfxU32 srcUVOffset, mfxSize roi, mfxU32 bitshift, mfxU32 format);
    mfxStatus CopyShiftVideoToSystemMemory(mfxU8 *pDst, mfxU32 dstPitch, mfxU32 dstUVOffset, void *pSrc, mfxU32 srcPitch, mfxSize roi, mfxU32 bitshift, mfxU32 format);
    mfxStatus CopyMirrorVideoToSystemMemory(mfxU8 *pDst, mfxU32 dstPitch, mfxU32 dstUVOffset, void *pSrc, mfxU32 srcPitch, mfxSize roi, mfxU32 format);
    mfxStatus CopyMirrorSystemToVideoMemory(void *pDst, mfxU32 dstPitch, mfxU8 *pSrc, mfxU32 srcPitch, mfxU32 srcUVOffset, mfxSize roi, mfxU32 format);
    mfxStatus CopySwapVideoToVideoMemory(void *pDst, void *pSrc, mfxSize roi, mfxU32 format);
    mfxStatus CopyMirrorVideoToVideoMemory(void *pDst, void *pSrc, mfxSize roi, mfxU32 format);

    mfxStatus ReleaseCmSurfaces(void);
    mfxStatus EnqueueCopyNV12GPUtoCPU(   CmSurface2D* pSurface,
                                    unsigned char* pSysMem,
                                    int width,
                                    int height,
                                    const UINT widthStride,
                                    const UINT heightStride,
                                    mfxU32 format,
                                    const UINT option,
                                    CmEvent* & pEvent );
    mfxStatus EnqueueCopySwapRBGPUtoCPU(   CmSurface2D* pSurface,
                                    unsigned char* pSysMem,
                                    int width,
                                    int height,
                                    const UINT widthStride,
                                    const UINT heightStride,
                                    mfxU32 format,
                                    const UINT option,
                                    CmEvent* & pEvent );
    mfxStatus EnqueueCopyGPUtoCPU(   CmSurface2D* pSurface,
                                unsigned char* pSysMem,
                                int width,
                                int height,
                                const UINT widthStride,
                                const UINT heightStride,
                                mfxU32 format,
                                const UINT option,
                                CmEvent* & pEvent );

    mfxStatus EnqueueCopySwapRBCPUtoGPU(   CmSurface2D* pSurface,
                                    unsigned char* pSysMem,
                                    int width,
                                    int height,
                                    const UINT widthStride,
                                    const UINT heightStride,
                                    mfxU32 format,
                                    const UINT option,
                                    CmEvent* & pEvent );
    mfxStatus EnqueueCopyCPUtoGPU(   CmSurface2D* pSurface,
                                unsigned char* pSysMem,
                                int width,
                                int height,
                                const UINT widthStride,
                                const UINT heightStride,
                                mfxU32 format,
                                const UINT option,
                                CmEvent* & pEvent );
    mfxStatus EnqueueCopySwapRBGPUtoGPU(   CmSurface2D* pSurfaceIn,
                                    CmSurface2D* pSurfaceOut,
                                    int width,
                                    int height,
                                    mfxU32 format,
                                    const UINT option,
                                    CmEvent* & pEvent );
    mfxStatus EnqueueCopyMirrorGPUtoGPU(   CmSurface2D* pSurfaceIn,
                                    CmSurface2D* pSurfaceOut,
                                    int width,
                                    int height,
                                    mfxU32 format,
                                    const UINT option,
                                    CmEvent* & pEvent );
    mfxStatus EnqueueCopyMirrorNV12GPUtoCPU(   CmSurface2D* pSurface,
                                    unsigned char* pSysMem,
                                    int width,
                                    int height,
                                    const UINT widthStride,
                                    const UINT heightStride,
                                    mfxU32 format,
                                    const UINT option,
                                    CmEvent* & pEvent );
    mfxStatus EnqueueCopyMirrorNV12CPUtoGPU(   CmSurface2D* pSurface,
                                    unsigned char* pSysMem,
                                    int width,
                                    int height,
                                    const UINT widthStride,
                                    const UINT heightStride,
                                    mfxU32 format,
                                    const UINT option,
                                    CmEvent* & pEvent );
    mfxStatus EnqueueCopyShiftP010GPUtoCPU(   CmSurface2D* pSurface,
                                    unsigned char* pSysMem,
                                    int width,
                                    int height,
                                    const UINT widthStride,
                                    const UINT heightStride,
                                    mfxU32 format,
                                    const UINT option,
                                    int bitshift,
                                    CmEvent* & pEvent );
    mfxStatus EnqueueCopyShiftGPUtoCPU(CmSurface2D* pSurface,
                                    unsigned char* pSysMem,
                                    int width,
                                    int height,
                                    const UINT widthStride,
                                    const UINT heightStride,
                                    mfxU32 format,
                                    const UINT option,
                                    int bitshift,
                                    CmEvent* & pEvent);
    mfxStatus EnqueueCopyShiftP010CPUtoGPU( CmSurface2D* pSurface,
                                    unsigned char* pSysMem,
                                    int width,
                                    int height,
                                    const UINT widthStride,
                                    const UINT heightStride,
                                    mfxU32 format,
                                    const UINT option,
                                    int bitshift,
                                    CmEvent* & pEvent );
    mfxStatus EnqueueCopyShiftCPUtoGPU(CmSurface2D* pSurface,
                                    unsigned char* pSysMem,
                                    int width,
                                    int height,
                                    const UINT widthStride,
                                    const UINT heightStride,
                                    mfxU32 format,
                                    int bitshift,
                                    const UINT option,
                                    CmEvent* & pEvent);
    mfxStatus EnqueueCopyNV12CPUtoGPU(   CmSurface2D* pSurface,
                                    unsigned char* pSysMem,
                                    int width,
                                    int height,
                                    const UINT widthStride,
                                    const UINT heightStride,
                                    mfxU32 format,
                                    const UINT option,
                                    CmEvent* & pEvent );
protected:

    eMFXHWType m_HWType;
    CmDevice  *m_pCmDevice;
    CmProgram *m_pCmProgram;
    INT m_timeout;

    CmThreadSpace *m_pThreadSpace;

    CmQueue *m_pCmQueue;
    CmTask  *m_pCmTask1;
    CmTask  *m_pCmTask2;

    CmSurface2D *m_pCmSurface2D;
    CmBufferUP *m_pCmUserBuffer;

    SurfaceIndex *m_pCmSrcIndex;
    SurfaceIndex *m_pCmDstIndex;

    std::set<mfxU8 *> m_cachedObjects;

    std::map<void *, CmSurface2D *> m_tableCmRelations;
    std::map<mfxU8 *, CmBufferUP *> m_tableSysRelations;

    std::map<CmSurface2D *, SurfaceIndex *> m_tableCmIndex;
    std::map<CmBufferUP *,  SurfaceIndex *> m_tableSysIndex;

    std::map<mfxHDLPair, CmSurface2D *> m_tableCmRelations2;
    std::map<mfxU8 *, CmBufferUP *> m_tableSysRelations2;

    std::map<CmSurface2D *, SurfaceIndex *> m_tableCmIndex2;
    std::map<CmBufferUP *,  SurfaceIndex *> m_tableSysIndex2;

    /* It needs to destroy buffers and surfaces in strict order */
    std::vector<CmSurface2D*> m_surfacesInCreationOrder;
    std::vector<CmBufferUP*>  m_buffersInCreationOrder;
    UMC::Mutex m_guard;

    CmSurface2D * CreateCmSurface2D(void *pSrc, mfxU32 width, mfxU32 height, bool isSecondMode,
                                    std::map<mfxHDLPair, CmSurface2D *> & tableCmRelations,
                                    std::map<CmSurface2D *, SurfaceIndex *> & tableCmIndex);

    SurfaceIndex  * CreateUpBuffer(mfxU8 *pDst, mfxU32 memSize,
                                 std::map<mfxU8 *, CmBufferUP *> & tableSysRelations,
                                 std::map<CmBufferUP *,  SurfaceIndex *> & tableSysIndex);

};


#endif // __CM_MEM_COPY_H__
