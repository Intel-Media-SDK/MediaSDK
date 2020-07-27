/******************************************************************************\
Copyright (c) 2005-2020, Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

This sample was distributed or derived from the Intel's Media Samples package.
The original version of this sample may be obtained from https://software.intel.com/en-us/intel-media-server-studio
or https://software.intel.com/en-us/media-client-solutions-support.
\**********************************************************************************/

#include "mfx_samples_config.h"

#if defined(_WIN32) || defined(_WIN64)

#include "opencl_filter_dx9.h"
#include "sample_defs.h"

// INTEL DX9 sharing functions declared deprecated in OpenCL 1.2
#pragma warning( disable : 4996 )

#define DX9_MEDIA_SHARING
#define DX_MEDIA_SHARING
#define CL_DX9_MEDIA_SHARING_INTEL_EXT

#include <CL/cl_ext_intel.h>
#include <CL/cl_dx9_media_sharing_intel.h>

using std::endl;

DECL_CL_EXT_FUNC(clGetDeviceIDsFromDX9INTEL);
DECL_CL_EXT_FUNC(clCreateFromDX9MediaSurfaceINTEL);
DECL_CL_EXT_FUNC(clEnqueueAcquireDX9ObjectsINTEL);
DECL_CL_EXT_FUNC(clEnqueueReleaseDX9ObjectsINTEL);

OpenCLFilterDX9::OpenCLFilterDX9()
{
    m_requiredOclExtensions.push_back("cl_intel_dx9_media_sharing");

    for(size_t i=0;i<c_shared_surfaces_num;i++)
    {
        m_hSharedSurfaces[i] = NULL;
        m_pSharedSurfaces[i] = NULL;
    }
}

OpenCLFilterDX9::~OpenCLFilterDX9()
{
    m_SafeD3DDeviceManager.reset();
}

cl_int OpenCLFilterDX9::OCLInit(mfxHDL device)
{
    MSDK_CHECK_POINTER(device, MFX_ERR_NULL_PTR);

    // store IDirect3DDeviceManager9
    try
    {
        m_SafeD3DDeviceManager.reset(new SafeD3DDeviceManager(static_cast<IDirect3DDeviceManager9*>(device)));
    }
    catch (const std::exception &ex)
    {
        log.error() << ex.what() << endl;
        return CL_DEVICE_NOT_AVAILABLE;
    }

    return OpenCLFilterBase::OCLInit(device);
}

cl_int OpenCLFilterDX9::InitSurfaceSharingExtension()
{
    // Hook up the d3d sharing extension functions that we need
    if( !INIT_CL_EXT_FUNC(m_clplatform, clGetDeviceIDsFromDX9INTEL)
     || !INIT_CL_EXT_FUNC(m_clplatform, clCreateFromDX9MediaSurfaceINTEL)
     || !INIT_CL_EXT_FUNC(m_clplatform, clEnqueueAcquireDX9ObjectsINTEL)
     || !INIT_CL_EXT_FUNC(m_clplatform, clEnqueueReleaseDX9ObjectsINTEL))
    {
        log.error() << "OpenCLFilter: Couldn't get all of the media sharing routines" << endl;
        return CL_INVALID_PLATFORM;
    }

    return CL_SUCCESS;
}

cl_int OpenCLFilterDX9::InitDevice()
{
    log.debug() << "OpenCLFilter: Try to init OCL device" << endl;

    cl_int error = CL_SUCCESS;

    try
    {
        LockedD3DDevice pD3DDevice = m_SafeD3DDeviceManager->LockDevice();
        error = pfn_clGetDeviceIDsFromDX9INTEL(m_clplatform, CL_D3D9EX_DEVICE_INTEL, // note - you must use D3D9DeviceEx
                                           pD3DDevice.get(), CL_PREFERRED_DEVICES_FOR_DX9_INTEL, 1, &m_cldevice, NULL);
        if(error) {
            log.error() << "OpenCLFilter: clGetDeviceIDsFromDX9INTEL failed. Error code: " << error << endl;
            return error;
        }

        // Initialize the shared context
        cl_context_properties props[] =
        {
            CL_CONTEXT_D3D9EX_DEVICE_INTEL, (cl_context_properties)pD3DDevice.get(),
            CL_CONTEXT_PLATFORM, (cl_context_properties)m_clplatform,
            CL_CONTEXT_INTEROP_USER_SYNC, (cl_context_properties)CL_TRUE,
            0
        };
        m_clcontext = clCreateContext(props, 1, &m_cldevice, NULL, NULL, &error);
        if(error) {
            log.error() << "OpenCLFilter: clCreateContext failed. Error code: " << error << endl;
            return error;
        }
    }
    catch (const std::exception &ex)
    {
        log.error() << ex.what() << endl;
        return CL_DEVICE_NOT_AVAILABLE;
    }

    log.debug() << "OpenCLFilter: OCL device inited" << endl;

    return error;
}

cl_mem OpenCLFilterDX9::CreateSharedSurface(mfxMemId mid, int nView, bool bIsReadOnly)
{
    mfxHDLPair* mid_pair = static_cast<mfxHDLPair*>(mid);

    IDirect3DSurface9 *surf = (IDirect3DSurface9*)mid_pair->first;
    HANDLE handle = mid_pair->second;

    cl_int error = CL_SUCCESS;
    cl_mem mem = pfn_clCreateFromDX9MediaSurfaceINTEL(m_clcontext, bIsReadOnly ? CL_MEM_READ_ONLY : CL_MEM_READ_WRITE,
                                                    surf, handle, nView, &error);
    if (error) {
        log.error() << "clCreateFromDX9MediaSurfaceINTEL failed. Error code: " << error << endl;
        return 0;
    }
    return mem;
}

bool OpenCLFilterDX9::EnqueueAcquireSurfaces(cl_mem* surfaces, int nSurfaces)
{
    cl_int error = pfn_clEnqueueAcquireDX9ObjectsINTEL(m_clqueue, nSurfaces, surfaces, 0, NULL, NULL);
    if (error) {
        log.error() << "clEnqueueAcquireDX9ObjectsINTEL failed. Error code: " << error << endl;
        return false;
    }
    return true;
}

bool OpenCLFilterDX9::EnqueueReleaseSurfaces(cl_mem* surfaces, int nSurfaces)
{
    cl_int error = pfn_clEnqueueReleaseDX9ObjectsINTEL(m_clqueue, nSurfaces, surfaces, 0, NULL, NULL);
    if (error) {
        log.error() << "clEnqueueReleaseDX9ObjectsINTEL failed. Error code: " << error << endl;
        return false;
    }
    return true;
}

#endif // #if defined(_WIN32) || defined(_WIN64)
