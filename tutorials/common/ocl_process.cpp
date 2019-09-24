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

#include "ocl_process.h"

#include <stdio.h>
#include <malloc.h>

#include <CL/opencl.h>
#include <CL/cl_dx9_media_sharing.h>

const char* g_kernelFileName =          "ocl_flip.cl";
const char* g_kernelFunctionName =      "Flip";

// 0 = in, 1 = out
#define OCL_IN                          0
#define OCL_OUT                         1

#define INIT_CL_EXT_FUNC(x)             x = (x ## _fn)clGetExtensionFunctionAddress(#x);
#define SAFE_OCL_FREE(P, FREE_FUNC)     { if (P) { FREE_FUNC(P); P = NULL; } }
#define MAX_PLATFORMS                   32
#define MAX_STRING_SIZE                 1024
#define EXT_DECLARE(_name) _name##_fn _name
#define EXT_INIT(_p, _name) _name = (_name##_fn) clGetExtensionFunctionAddressForPlatform((_p), #_name); res &= (_name != NULL);

EXT_DECLARE(clGetDeviceIDsFromDX9MediaAdapterKHR);
EXT_DECLARE(clCreateFromDX9MediaSurfaceKHR);
EXT_DECLARE(clEnqueueAcquireDX9MediaSurfacesKHR);
EXT_DECLARE(clEnqueueReleaseDX9MediaSurfacesKHR);
inline int InitDX9MediaFunctions(cl_platform_id platform) // get DX9 sharing functions
{
    bool res = true;
    EXT_INIT(platform,clGetDeviceIDsFromDX9MediaAdapterKHR);
    EXT_INIT(platform,clCreateFromDX9MediaSurfaceKHR);
    EXT_INIT(platform,clEnqueueAcquireDX9MediaSurfacesKHR);
    EXT_INIT(platform,clEnqueueReleaseDX9MediaSurfacesKHR);
    return res;
}



OCLProcess::OCLProcess()
    : m_pD3DDevice(NULL)
{
}

OCLProcess::~OCLProcess()
{
}

cl_int OCLProcess::OCLInit(IDirect3DDevice9Ex* pD3DDevice)
{
    m_pD3DDevice = pD3DDevice;

    cl_int clSts = CL_SUCCESS;

    // Determine the number of installed OpenCL platforms
    cl_uint num_platforms = 0;
    clSts = clGetPlatformIDs(0, NULL, &num_platforms);
    if (CL_SUCCESS != clSts) {
        printf("Failed to determine # of OCL platforms (%d)\n", clSts);
        return clSts;
    }

    // Get all of the handles to the installed OpenCL platforms
    cl_platform_id platforms[MAX_PLATFORMS];
    if (num_platforms > MAX_PLATFORMS)
        num_platforms = MAX_PLATFORMS;
    clSts = clGetPlatformIDs(num_platforms, platforms, NULL);
    if (CL_SUCCESS != clSts) {
        printf("Failed to get OCL platform IDs (%d)\n", clSts);
        return clSts;
    }

    // Find the platform handle for the installed Intel graphics driver
    char name[MAX_STRING_SIZE];
    for (unsigned int platform_index = 0; platform_index < num_platforms; platform_index++) {
        clSts = clGetPlatformInfo(platforms[platform_index], CL_PLATFORM_NAME, MAX_STRING_SIZE, name, NULL);
        if (CL_SUCCESS != clSts) {
            printf("Failed to get OCL platform info (%d)\n", clSts);
            return clSts;
        }

        if (strstr(name, "Intel")) { // Only Intel platforms supported
            printf("OpenCL platform \"%s\" is used\n", name);
            m_clPlatform = platforms[platform_index];
        }
    }
    if (0 == m_clPlatform) {
        printf("Didn't find an installed OCL compatible Intel driver !!!!\n");
        return CL_DEVICE_NOT_AVAILABLE;
    }

    // Check if surface sharing is available
    char    str[MAX_STRING_SIZE*4];
    size_t  len = 0;
    clSts = clGetPlatformInfo(m_clPlatform, CL_PLATFORM_EXTENSIONS, sizeof(str), str, &len);
    if (NULL == strstr(str, "cl_khr_d3d11_sharing") || CL_SUCCESS != clSts) {
        printf("DX11 sharing not available !!!! (%d)\n", clSts);
        return CL_DEVICE_NOT_AVAILABLE;
    }

    // Hook up the D3D sharing extensions
    int dx9_media_sharing = InitDX9MediaFunctions(m_clPlatform);
    if (!dx9_media_sharing) {
        printf("DX9 media sharing not available !!!!\n");
        return CL_DEVICE_NOT_AVAILABLE;
    }

    // Get device
    cl_uint numDevices = 0;
    cl_dx9_media_adapter_type_khr type = CL_ADAPTER_D3D9EX_KHR;
    cl_device_type reqDeviceType = CL_DEVICE_TYPE_GPU;
    cl_device_id devices[2];
    clSts = clGetDeviceIDsFromDX9MediaAdapterKHR(   m_clPlatform,
            1,
            &type,
            (void**)&m_pD3DDevice,
            CL_ALL_DEVICES_FOR_DX9_MEDIA_ADAPTER_KHR,
            sizeof(devices)/sizeof(devices[0]),
            devices,
            &numDevices);
    if (CL_SUCCESS != clSts) {
        printf("Get device IDs failed !!!! (%d)\n", clSts);
        return clSts;
    }

    cl_uint j = 0;
    for (cl_uint i=0; i<numDevices; i++) {
        //keep only req type
        cl_device_type type;
        clSts = clGetDeviceInfo(devices[i], CL_DEVICE_TYPE, sizeof(cl_device_type), &type, NULL);
        if (type != reqDeviceType) continue;
        devices[j++] = devices[i];
    }
    numDevices = j;
    const cl_context_properties props[] = {CL_CONTEXT_ADAPTER_D3D9EX_KHR, (cl_context_properties)m_pD3DDevice, NULL};
    m_clContext = clCreateContext(props, numDevices, devices, NULL, NULL, &clSts);
    if (CL_SUCCESS != clSts) {
        printf("Cannot initialize shared context !!!! (%d)\n", clSts);
        return clSts;
    }

    // Create a command queue
    m_clQueue = clCreateCommandQueue(m_clContext, devices[0], 0, &clSts);
    if (CL_SUCCESS != clSts) {
        printf("Command queue cannot be created !!!! (%d)\n", clSts);
        return clSts;
    }


    printf("OpenCL: Read and compile OCL kernel\n", name);

    char buildOptions[1024];
    sprintf_s(buildOptions, "-I. -Werror -cl-fast-relaxed-math");

    // Create a program object from the source file
    FILE* fp;
    errno_t ferr = fopen_s(&fp, g_kernelFileName, "rb");
    if (!fp || ferr) return CL_INVALID_PROGRAM;
    fseek(fp,0,SEEK_END);
    long flen = ftell(fp);
    fseek(fp,0,SEEK_SET);
    char* program_source = (char*)_alloca(flen+1);
    fread(program_source,1,flen,fp);
    ((char*)program_source)[flen] = '\0';
    fclose(fp);

    m_clProgram = clCreateProgramWithSource(m_clContext, 1, (const char**)&program_source, NULL, &clSts);
    if (CL_SUCCESS != clSts) {
        printf("Program cannot be created !!!! (%d)\n", clSts);
        return clSts;
    }

    // Build OCL kernel
    clSts = clBuildProgram(m_clProgram, 0, NULL, buildOptions, NULL, NULL);
    if (clSts == CL_BUILD_PROGRAM_FAILURE) {
        cl_int logStatus;
        char* buildLog = NULL;
        size_t buildLogSize = 0;
        logStatus = clGetProgramBuildInfo (m_clProgram, devices[0], CL_PROGRAM_BUILD_LOG, buildLogSize, buildLog, &buildLogSize);
        buildLog = (char*)malloc(buildLogSize);
        memset(buildLog, 0, buildLogSize);
        logStatus = clGetProgramBuildInfo (m_clProgram, devices[0], CL_PROGRAM_BUILD_LOG, buildLogSize, buildLog, NULL);
        printf("%s", buildLog);
        free(buildLog);
        return clSts;
    } else if (clSts) {
        printf("Unknown build program error !!!! (%d)\n", clSts);
        return clSts;
    }

    // Create the kernel objects
    m_clKernel = clCreateKernel(m_clProgram, g_kernelFunctionName, &clSts);
    if (CL_SUCCESS != clSts) {
        printf("Kernel cannot be created !!!! (%d)\n", clSts);
        return clSts;
    }

    return clSts;
}

cl_int OCLProcess::OCLPrepare(  int width,
                                int height,
                                mfxMemId* midsIn,
                                mfxU16 numSurfacesIn,
                                mfxMemId* midsOut,
                                mfxU16 numSurfacesOut)
{
    cl_int clSts = CL_SUCCESS;
    cl_dx9_surface_info_khr info;

    m_numSurfaces[OCL_IN]   = numSurfacesIn;
    m_numSurfaces[OCL_OUT]  = numSurfacesOut;
    m_mids[OCL_IN]          = midsIn;
    m_mids[OCL_OUT]         = midsOut;
    m_frameSizeY[0]         = width;
    m_frameSizeY[1]         = height;
    m_frameSizeUV[0]        = width/2;
    m_frameSizeUV[1]        = height/2;

    // 0 = in, 1 = out
    for (int i=0; i<2; i++) {
        // Create storage for OpenCL buffers
        m_pOCLBuffers[i] = new OCLBuffers[m_numSurfaces[i]];

        // Associate the shared buffer with the kernel object for surfaces....
        for (int idx=0; idx<m_numSurfaces[i]; idx++) {
            info.resource       = (IDirect3DSurface9*)m_mids[i][idx];
            info.shared_handle  = (HANDLE)m_mids[i][idx + m_numSurfaces[i]];

            // Y plane
            m_pOCLBuffers[i][idx].OCL_Y = clCreateFromDX9MediaSurfaceKHR(m_clContext, 0, CL_ADAPTER_D3D9EX_KHR, &info, 0, &clSts);
            if (clSts) {
                printf("clCreateFromDX9MediaSurfaceKHR(%d) err=%d\n", idx, clSts);
                return clSts;
            }

            // UV plane
            m_pOCLBuffers[i][idx].OCL_UV = clCreateFromDX9MediaSurfaceKHR(m_clContext, 0, CL_ADAPTER_D3D9EX_KHR, &info, 1, &clSts);
            if (clSts) {
                printf("clCreateFromDX9MediaSurfaceKHR(%d) err=%d\n", idx, clSts);
                return clSts;
            }
        }
    }

    // Work sizes for Y plane
    m_LocalWorkSizeY[0]     =  8;
    m_LocalWorkSizeY[1]     =  8;
    m_GlobalWorkSizeY[0]    =  m_LocalWorkSizeY[0]*(m_frameSizeY[0]/m_LocalWorkSizeY[0]);
    if (m_frameSizeY[0] % m_LocalWorkSizeY[0])
        m_GlobalWorkSizeY[0] += m_LocalWorkSizeY[0];
    m_GlobalWorkSizeY[1]    =  m_LocalWorkSizeY[1]*(m_frameSizeY[1]/m_LocalWorkSizeY[1]);
    if (m_frameSizeY[1] % m_LocalWorkSizeY[1])
        m_GlobalWorkSizeY[1] += m_LocalWorkSizeY[1];

    // Work size for UV planes
    m_LocalWorkSizeUV[0]    = 8;
    m_LocalWorkSizeUV[1]    = 8;
    m_GlobalWorkSizeUV[0]   = m_LocalWorkSizeUV[0]*(m_frameSizeUV[0]/m_LocalWorkSizeUV[0]);
    if (m_frameSizeUV[0] % m_LocalWorkSizeUV[0])
        m_GlobalWorkSizeUV[0] += m_LocalWorkSizeUV[0];
    m_GlobalWorkSizeUV[1]   = m_LocalWorkSizeUV[1]*(m_frameSizeUV[1]/m_LocalWorkSizeUV[1]);
    if (m_frameSizeUV[1] % m_LocalWorkSizeUV[1])
        m_GlobalWorkSizeUV[1] += m_LocalWorkSizeUV[1];

    return clSts;
}

cl_int OCLProcess::OCLProcessSurface(   int midIdxIn,
                                        int midIdxOut,
                                        cl_event* pclEventDone)
{
    cl_int clSts = CL_SUCCESS;

    // Set OCL buffer input surfaces
    m_oclSurfaces[0] = m_pOCLBuffers[OCL_IN][midIdxIn].OCL_Y;
    m_oclSurfaces[1] = m_pOCLBuffers[OCL_IN][midIdxIn].OCL_UV;
    // Set OCL buffer output surfaces
    m_oclSurfaces[2] = m_pOCLBuffers[OCL_OUT][midIdxOut].OCL_Y;
    m_oclSurfaces[3] = m_pOCLBuffers[OCL_OUT][midIdxOut].OCL_UV;


    clSts = clEnqueueAcquireDX9MediaSurfacesKHR(m_clQueue, 4, m_oclSurfaces, 0, NULL, NULL);
    if (clSts) printf("clEnqueueAcquireDX9MediaSurfacesKHR err=%d\n", clSts);


    // Enqueue kernel processing of Y plane
    clSts = clSetKernelArg(m_clKernel, 0, sizeof(cl_mem), &m_pOCLBuffers[OCL_IN][midIdxIn].OCL_Y);  // In
    if (clSts) printf("clSetKernelArg1 err=%d\n", clSts);
    clSts = clSetKernelArg(m_clKernel, 1, sizeof(cl_mem), &m_pOCLBuffers[OCL_OUT][midIdxOut].OCL_Y); // Out
    if (clSts) printf("clSetKernelArg2 err=%d\n", clSts);
    clSts = clSetKernelArg(m_clKernel, 2, sizeof(cl_int), &m_frameSizeY[1]); // Frame height
    if (clSts) printf("clSetKernelArg3 err=%d\n", clSts);

    clSts = clEnqueueNDRangeKernel(m_clQueue, m_clKernel, 2, NULL, m_GlobalWorkSizeY, m_LocalWorkSizeY, 0, NULL, NULL);
    if (clSts) printf("clEnqueueNDRangeKernel Y err=%d\n", clSts);

    // Enqueue kernel processing of UV plane
    clSts = clSetKernelArg(m_clKernel, 0, sizeof(cl_mem), &m_pOCLBuffers[OCL_IN][midIdxIn].OCL_UV);  // In
    if (clSts) printf("clSetKernelArg1 err=%d\n", clSts);
    clSts = clSetKernelArg(m_clKernel, 1, sizeof(cl_mem), &m_pOCLBuffers[OCL_OUT][midIdxOut].OCL_UV); // Out
    if (clSts) printf("clSetKernelArg2 err=%d\n", clSts);
    clSts = clSetKernelArg(m_clKernel, 2, sizeof(cl_int), &m_frameSizeUV[1]); // Frame height
    if (clSts) printf("clSetKernelArg3 err=%d\n", clSts);

    clSts = clEnqueueNDRangeKernel(m_clQueue, m_clKernel, 2, NULL, m_GlobalWorkSizeUV, m_LocalWorkSizeUV, 0, NULL, NULL);
    if (clSts) printf("clEnqueueNDRangeKernel UV err=%d\n", clSts);

    clSts = clEnqueueReleaseDX9MediaSurfacesKHR(m_clQueue, 4, m_oclSurfaces, 0, NULL, NULL);
    if (clSts) printf("clEnqueueReleaseDX9MediaSurfacesKHR err=%d\n", clSts);

    if (pclEventDone) { // Used for asyncronous processing cases
        clSts = clEnqueueMarkerWithWaitList(m_clQueue, 0, 0, pclEventDone);
        if (clSts) printf("clEnqueueMarkerWithWaitList err=%d\n", clSts);
    }

    clSts = clFlush(m_clQueue);
    if (clSts) printf("clFlush err=%d\n", clSts);

    return clSts;
}

void OCLProcess::OCLRelease()
{
    for (int i=0; i<2; i++) {
        for (int j=0; j<m_numSurfaces[i]; j++) {
            SAFE_OCL_FREE(m_pOCLBuffers[i][j].OCL_Y, clReleaseMemObject);
            SAFE_OCL_FREE(m_pOCLBuffers[i][j].OCL_UV, clReleaseMemObject);
        }
        delete [] m_pOCLBuffers[i];
    }

    SAFE_OCL_FREE(m_clProgram,  clReleaseProgram);
    SAFE_OCL_FREE(m_clKernel,   clReleaseKernel);
    SAFE_OCL_FREE(m_clQueue,    clReleaseCommandQueue);
    SAFE_OCL_FREE(m_clContext,  clReleaseContext);
}
