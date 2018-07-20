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


#include "assert.h"
#include "vm_shared_object.h"
#include "cmrt_cross_platform.h"

#if defined(LINUX64)
const vm_char * DLL_NAME_LINUX = VM_STRING("igfxcmrt64.so");
#elif defined(LINUX32)
const vm_char * DLL_NAME_LINUX = VM_STRING("igfxcmrt32.so");
#else
#error "undefined configuration"
#endif

#ifdef CMRT_EMU
    const char * FUNC_NAME_CREATE_CM_DEVICE  = "CreateCmDeviceEmu";
    const char * FUNC_NAME_CREATE_CM_DEVICE_EX  = "CreateCmDeviceEmu";
    const char * FUNC_NAME_DESTROY_CM_DEVICE = "DestroyCmDeviceEmu";
#else //CMRT_EMU
    const char * FUNC_NAME_CREATE_CM_DEVICE  = "CreateCmDevice";
    const char * FUNC_NAME_CREATE_CM_DEVICE_EX  = "CreateCmDeviceEx";
    const char * FUNC_NAME_DESTROY_CM_DEVICE = "DestroyCmDevice";
#endif //CMRT_EMU


#define IMPL_FOR_ALL(RET, FUNC, PROTO, PARAM)   \
    RET FUNC PROTO {                            \
        switch (m_platform) {                   \
        case VAAPI: return m_linux->FUNC PARAM; \
        default:    return CM_NOT_IMPLEMENTED;  \
        }                                       \
    }


#define CONVERT_FORMAT(FORMAT) (FORMAT)

#if defined(__clang__)
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wnon-virtual-dtor"
#elif defined(__GNUC__)
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#endif

enum { DX9=1, DX11=2, VAAPI=3 };

typedef INT (* CreateCmDeviceLinuxFuncTypeEx)(CmLinux::CmDevice *&, UINT &, VADisplay, UINT);
typedef INT (* CreateCmDeviceLinuxFuncType)(CmLinux::CmDevice *&, UINT &, VADisplay);
typedef INT (* DestroyCmDeviceVAAPIFuncType)(CmLinux::CmDevice *&);

class CmDeviceImpl : public CmDevice
{
public:
    vm_so_handle    m_dll;
    INT             m_platform;

    union
    {
        CmLinux::CmDevice *     m_linux;
    };

    virtual ~CmDeviceImpl(){}

    INT GetPlatform(){return m_platform;};

    INT GetDevice(AbstractDeviceHandle & pDevice)
    {
        (void)pDevice;

        switch (m_platform) {
        case VAAPI: return CM_NOT_IMPLEMENTED;
        default:    return CM_NOT_IMPLEMENTED;
        }
    }

    INT CreateSurface2D(mfxHDLPair D3DSurfPair, CmSurface2D *& pSurface)
    {
        switch (m_platform) {
        case VAAPI: return m_linux->CreateSurface2D(*(VASurfaceID*)D3DSurfPair.first, pSurface);
        default:    return CM_NOT_IMPLEMENTED;
        }
    }

    INT CreateSurface2D(AbstractSurfaceHandle pD3DSurf, CmSurface2D *& pSurface)
    {
        switch (m_platform) {
        case VAAPI: return m_linux->CreateSurface2D(*(VASurfaceID*)pD3DSurf, pSurface);
        default:    return CM_NOT_IMPLEMENTED;
        }
    }

    INT CreateSurface2DSubresource(AbstractSurfaceHandle pD3D11Texture2D, UINT subresourceCount, CmSurface2D ** ppSurfaces, UINT & createdSurfaceCount, UINT option)
    {
        (void)pD3D11Texture2D;
        (void)subresourceCount;
        (void)ppSurfaces;
        (void)createdSurfaceCount;
        (void)option;

        switch (m_platform) {
        case VAAPI: return CM_NOT_IMPLEMENTED;
        default:    return CM_NOT_IMPLEMENTED;
        }
    }

    INT CreateSurface2DbySubresourceIndex(AbstractSurfaceHandle pD3D11Texture2D, UINT FirstArraySlice, UINT FirstMipSlice, CmSurface2D *& pSurface)
    {
        (void)pD3D11Texture2D;
        (void)FirstArraySlice;
        (void)FirstMipSlice;
        (void)pSurface;

        switch (m_platform) {
        case VAAPI: return CM_NOT_IMPLEMENTED;
        default:    return CM_NOT_IMPLEMENTED;
        }
    }

    IMPL_FOR_ALL(INT, CreateBuffer, (UINT size, CmBuffer *& pSurface), (size, pSurface));
    IMPL_FOR_ALL(INT, CreateSurface2D, (UINT width, UINT height, CM_SURFACE_FORMAT format, CmSurface2D* & pSurface), (width, height, CONVERT_FORMAT(format), pSurface));
    IMPL_FOR_ALL(INT, CreateSurface3D, (UINT width, UINT height, UINT depth, CM_SURFACE_FORMAT format, CmSurface3D* & pSurface), (width, height, depth, CONVERT_FORMAT(format), pSurface));
    IMPL_FOR_ALL(INT, DestroySurface, (CmSurface2D *& pSurface), (pSurface));
    IMPL_FOR_ALL(INT, DestroySurface, (CmBuffer *& pSurface), (pSurface));
    IMPL_FOR_ALL(INT, DestroySurface, (CmSurface3D *& pSurface), (pSurface));
    IMPL_FOR_ALL(INT, CreateQueue, (CmQueue *& pQueue), (pQueue));
    IMPL_FOR_ALL(INT, LoadProgram, (void * pCommonISACode, const UINT size, CmProgram *& pProgram, const char * options), (pCommonISACode, size, pProgram, options));
    IMPL_FOR_ALL(INT, CreateKernel, (CmProgram * pProgram, const char * kernelName, CmKernel *& pKernel, const char * options), (pProgram, kernelName, pKernel, options));
    IMPL_FOR_ALL(INT, CreateKernel, (CmProgram * pProgram, const char * kernelName, const void * fncPnt, CmKernel *& pKernel, const char * options), (pProgram, kernelName, fncPnt, pKernel, options));
    IMPL_FOR_ALL(INT, CreateSampler, (const CM_SAMPLER_STATE & sampleState, CmSampler *& pSampler), (sampleState, pSampler));
    IMPL_FOR_ALL(INT, DestroyKernel, (CmKernel *& pKernel), (pKernel));
    IMPL_FOR_ALL(INT, DestroySampler, (CmSampler *& pSampler), (pSampler));
    IMPL_FOR_ALL(INT, DestroyProgram, (CmProgram *& pProgram), (pProgram));
    IMPL_FOR_ALL(INT, DestroyThreadSpace, (CmThreadSpace *& pTS), (pTS));
    IMPL_FOR_ALL(INT, CreateTask, (CmTask *& pTask), (pTask));
    IMPL_FOR_ALL(INT, DestroyTask, (CmTask *& pTask), (pTask));
    IMPL_FOR_ALL(INT, GetCaps, (CM_DEVICE_CAP_NAME capName, size_t & capValueSize, void * pCapValue), (capName, capValueSize, pCapValue));
#ifndef CMAPIUPDATE
    IMPL_FOR_ALL(INT, CreateVmeStateG6, (const VME_STATE_G6 & vmeState, CmVmeState *& pVmeState), (vmeState, pVmeState));
    IMPL_FOR_ALL(INT, DestroyVmeStateG6, (CmVmeState *& pVmeState), (pVmeState));
    IMPL_FOR_ALL(INT, CreateVmeSurfaceG6, (CmSurface2D * pCurSurface, CmSurface2D * pForwardSurface, CmSurface2D * pBackwardSurface, SurfaceIndex *& pVmeIndex), (pCurSurface, pForwardSurface, pBackwardSurface, pVmeIndex));
    IMPL_FOR_ALL(INT, DestroyVmeSurfaceG6, (SurfaceIndex *& pVmeIndex), (pVmeIndex));
#endif
    IMPL_FOR_ALL(INT, CreateThreadSpace, (UINT width, UINT height, CmThreadSpace *& pTS), (width, height, pTS));
    IMPL_FOR_ALL(INT, CreateBufferUP, (UINT size, void * pSystMem, CmBufferUP *& pSurface), (size, pSystMem, pSurface));
    IMPL_FOR_ALL(INT, DestroyBufferUP, (CmBufferUP *& pSurface), (pSurface));
    IMPL_FOR_ALL(INT, GetSurface2DInfo, (UINT width, UINT height, CM_SURFACE_FORMAT format, UINT & pitch, UINT & physicalSize), (width, height, CONVERT_FORMAT(format), pitch, physicalSize));
    IMPL_FOR_ALL(INT, CreateSurface2DUP, (UINT width, UINT height, CM_SURFACE_FORMAT format, void * pSysMem, CmSurface2DUP *& pSurface), (width, height, CONVERT_FORMAT(format), pSysMem, pSurface));
    IMPL_FOR_ALL(INT, DestroySurface2DUP, (CmSurface2DUP *& pSurface), (pSurface));
    IMPL_FOR_ALL(INT, CreateVmeSurfaceG7_5 , (CmSurface2D * pCurSurface, CmSurface2D ** pForwardSurface, CmSurface2D ** pBackwardSurface, const UINT surfaceCountForward, const UINT surfaceCountBackward, SurfaceIndex *& pVmeIndex), (pCurSurface, pForwardSurface, pBackwardSurface, surfaceCountForward, surfaceCountBackward, pVmeIndex));
    IMPL_FOR_ALL(INT, DestroyVmeSurfaceG7_5, (SurfaceIndex *& pVmeIndex), (pVmeIndex));
    IMPL_FOR_ALL(INT, CreateSampler8x8, (const CM_SAMPLER_8X8_DESCR & smplDescr, CmSampler8x8 *& psmplrState), (smplDescr, psmplrState));
    IMPL_FOR_ALL(INT, DestroySampler8x8, (CmSampler8x8 *& pSampler), (pSampler));
    IMPL_FOR_ALL(INT, CreateSampler8x8Surface, (CmSurface2D * p2DSurface, SurfaceIndex *& pDIIndex, CM_SAMPLER8x8_SURFACE surf_type, CM_SURFACE_ADDRESS_CONTROL_MODE mode), (p2DSurface, pDIIndex, surf_type, mode));
    IMPL_FOR_ALL(INT, DestroySampler8x8Surface, (SurfaceIndex* & pDIIndex), (pDIIndex));
    IMPL_FOR_ALL(INT, CreateThreadGroupSpace, (UINT thrdSpaceWidth, UINT thrdSpaceHeight, UINT grpSpaceWidth, UINT grpSpaceHeight, CmThreadGroupSpace *& pTGS), (thrdSpaceWidth, thrdSpaceHeight, grpSpaceWidth, grpSpaceHeight, pTGS));
    IMPL_FOR_ALL(INT, DestroyThreadGroupSpace, (CmThreadGroupSpace *& pTGS), (pTGS));
#ifdef CMAPIUPDATE
    IMPL_FOR_ALL(INT, SetL3Config, (const L3_CONFIG_REGISTER_VALUES * l3_c), (l3_c));
#else
    IMPL_FOR_ALL(INT, SetL3Config, (L3_CONFIG_REGISTER_VALUES * l3_c), (l3_c));
#endif
    IMPL_FOR_ALL(INT, SetSuggestedL3Config, (L3_SUGGEST_CONFIG l3_s_c), (l3_s_c));
    IMPL_FOR_ALL(INT, SetCaps, (CM_DEVICE_CAP_NAME capName, size_t capValueSize, void * pCapValue), (capName, capValueSize, pCapValue));
#ifndef CMAPIUPDATE
    IMPL_FOR_ALL(INT, CreateGroupedVAPlusSurface, (CmSurface2D * p2DSurface1, CmSurface2D * p2DSurface2, SurfaceIndex *& pDIIndex, CM_SURFACE_ADDRESS_CONTROL_MODE mode), (p2DSurface1, p2DSurface2, pDIIndex, mode));
    IMPL_FOR_ALL(INT, DestroyGroupedVAPlusSurface, (SurfaceIndex *& pDIIndex), (pDIIndex));
#endif
    IMPL_FOR_ALL(INT, CreateSamplerSurface2D, (CmSurface2D * p2DSurface, SurfaceIndex *& pSamplerSurfaceIndex), (p2DSurface, pSamplerSurfaceIndex));
    IMPL_FOR_ALL(INT, CreateSamplerSurface3D, (CmSurface3D * p3DSurface, SurfaceIndex *& pSamplerSurfaceIndex), (p3DSurface, pSamplerSurfaceIndex));
    IMPL_FOR_ALL(INT, DestroySamplerSurface, (SurfaceIndex *& pSamplerSurfaceIndex), (pSamplerSurfaceIndex));
#ifndef CMAPIUPDATE
    IMPL_FOR_ALL(INT, GetRTDllVersion, (CM_DLL_FILE_VERSION * pFileVersion), (pFileVersion));
    IMPL_FOR_ALL(INT, GetJITDllVersion, (CM_DLL_FILE_VERSION * pFileVersion), (pFileVersion));
#endif
    IMPL_FOR_ALL(INT, InitPrintBuffer, (size_t printbufsize), (printbufsize));
    IMPL_FOR_ALL(INT, FlushPrintBuffer, (), ());
};


INT CreateCmDevice(CmDevice *& pD, UINT & version, VADisplay va_dpy, UINT mode)
{
    CmDeviceImpl * device = new CmDeviceImpl;

    device->m_platform = VAAPI;
    device->m_dll = vm_so_load(DLL_NAME_LINUX);
    if (device->m_dll == 0)
    {
        delete device;
        return CM_FAILURE;
    }

    CreateCmDeviceLinuxFuncTypeEx createFunc = (CreateCmDeviceLinuxFuncTypeEx)vm_so_get_addr(device->m_dll, FUNC_NAME_CREATE_CM_DEVICE_EX);
    if (createFunc == 0)
    {
        delete device;
        return CM_FAILURE;
    }

    INT res = createFunc(device->m_linux, version, va_dpy, mode);
    if (res != CM_SUCCESS)
    {
        delete device;
        return CM_FAILURE;
    }

    pD = device;
    return CM_SUCCESS;
}


INT DestroyCmDevice(CmDevice *& pD)
{
    CmDeviceImpl * device = (CmDeviceImpl *)pD;

    if (pD == 0 || device->m_dll == 0)
        return CM_SUCCESS;

    INT res = CM_SUCCESS;

    if (vm_so_func destroyFunc = vm_so_get_addr(device->m_dll, FUNC_NAME_DESTROY_CM_DEVICE))
    {
        switch (device->GetPlatform())
        {
        case VAAPI:
            res = ((DestroyCmDeviceVAAPIFuncType)destroyFunc)(device->m_linux);
            break;
        }
    }

    vm_so_free(device->m_dll);

    device->m_dll  = 0;
    device->m_linux = 0;

    delete device;
    pD = 0;
    return res;
}

int ReadProgram(CmDevice * device, CmProgram *& program, const unsigned char * buffer, unsigned int len)
{
#ifdef CMRT_EMU
    buffer; len;
    return device->LoadProgram(0, 0, program, "nojitter");
#else //CMRT_EMU
    return device->LoadProgram((void *)buffer, len, program, "nojitter");
#endif //CMRT_EMU
}

int ReadProgramJit(CmDevice * device, CmProgram *& program, const unsigned char * buffer, unsigned int len)
{
#ifdef CMRT_EMU
    buffer; len;
    return device->LoadProgram(0, 0, program);
#else //CMRT_EMU
    return device->LoadProgram((void *)buffer, len, program);
#endif //CMRT_EMU
}

int CreateKernel(CmDevice * device, CmProgram * program, const char * kernelName, const void * fncPnt, CmKernel *& kernel, const char * options)
{
#ifdef CMRT_EMU
    return device->CreateKernel(program, kernelName, fncPnt, kernel, options);
#else //CMRT_EMU
    fncPnt;
    return device->CreateKernel(program, kernelName, kernel, options);
#endif //CMRT_EMU
}

CmEvent *CM_NO_EVENT = ((CmEvent *)(-1));

#if defined(__clang__)
  #pragma clang diagnostic pop
#elif defined(__GNUC__)
  #pragma GCC diagnostic pop
#endif
