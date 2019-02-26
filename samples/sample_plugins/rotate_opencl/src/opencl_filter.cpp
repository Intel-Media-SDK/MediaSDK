/******************************************************************************\
Copyright (c) 2005-2018, Intel Corporation
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

#include <fstream>
#include <stdexcept>

#include "opencl_filter.h"
#include "sample_defs.h"

using std::endl;

OpenCLFilterBase::OpenCLFilterBase() : log(std::clog)
{
    m_bInit              = false;
    m_pAlloc             = NULL;
    m_activeKernel       = 0;

    for(size_t i=0; i < c_ocl_surface_buffers_num;i++)
    {
        m_clbuffer[i] = NULL;
    }

    m_cldevice           = 0;
    m_clplatform         = 0;
    m_clqueue            = 0;
    m_clcontext          = 0;

    m_currentWidth       = 0;
    m_currentHeight      = 0;
}

OpenCLFilterBase::~OpenCLFilterBase() {
    for(unsigned i=0;i<m_kernels.size();i++)
    {
        SAFE_OCL_FREE(m_kernels[i].clprogram, clReleaseProgram);
        SAFE_OCL_FREE(m_kernels[i].clkernelY, clReleaseKernel);
        SAFE_OCL_FREE(m_kernels[i].clkernelUV, clReleaseKernel);
    }

    SAFE_OCL_FREE(m_clqueue,clReleaseCommandQueue);
    SAFE_OCL_FREE(m_clcontext,clReleaseContext);
}

mfxStatus OpenCLFilterBase::SetAllocator(mfxFrameAllocator *pAlloc)
{
    m_pAlloc = pAlloc;
    return MFX_ERR_NONE;
}

cl_int OpenCLFilterBase::ReleaseResources() {
    cl_int error = CL_SUCCESS;

    for(size_t i=0; i < c_ocl_surface_buffers_num; i++)
    {
        if (m_clbuffer[i])
          error = clReleaseMemObject( m_clbuffer[i] );
        if(error) {
            log.error() << "clReleaseMemObject failed. Error code: " << error << endl;
            return error;
        }
    }

    error = clFinish( m_clqueue );
    if(error)
    {
        log.error() << "clFinish failed. Error code: " << error << endl;
        return error;
    }

    for(size_t i=0;i<c_ocl_surface_buffers_num;i++)
    {
        m_clbuffer[i] = NULL;
    }

    return error;
}

cl_int OpenCLFilterBase::OCLInit(mfxHDL /*device*/)
{
    cl_int error = CL_SUCCESS;

    error = InitPlatform();
    if (error) return error;

    error = InitSurfaceSharingExtension();
    if (error) return error;

    error = InitDevice();
    if (error) return error;

    error = BuildKernels();
    if (error) return error;

    // Create a command queue
    log.debug() << "OpenCLFilter: Creating command queue" << endl;
    m_clqueue = clCreateCommandQueue(m_clcontext, m_cldevice, 0, &error);
    if (error) return error;
    log.debug() << "OpenCLFilter: Command queue created" << endl;

    m_bInit = true;

    return error;
}

cl_int OpenCLFilterBase::InitPlatform()
{
    cl_int error = CL_SUCCESS;

    // Determine the number of installed OpenCL platforms
    cl_uint num_platforms = 0;
    error = clGetPlatformIDs(0, NULL, &num_platforms);
    if(error)
    {
        log.error() << "OpenCLFilter: Couldn't get platform IDs. \
                        Make sure your platform \
                        supports OpenCL and can find a proper library." << endl;
        return error;
    }

    // Get all of the handles to the installed OpenCL platforms
    std::vector<cl_platform_id> platforms(num_platforms);
    error = clGetPlatformIDs(num_platforms, &platforms[0], &num_platforms);
    if(error) {
        log.error() << "OpenCLFilter: Failed to get OCL platform IDs. Error Code; " << error;
        return error;
    }

    // Find the platform handle for the installed Gen driver
    const size_t max_string_size = 1024;
    char platform[max_string_size];
    cl_device_id device_ids[2] = {0};
    for (unsigned int platform_index = 0; platform_index < num_platforms; platform_index++)
    {
        error = clGetPlatformInfo(platforms[platform_index], CL_PLATFORM_NAME, max_string_size, platform, NULL);
        if(error) return error;

        // Choose only GPU devices
        if (clGetDeviceIDs(platforms[platform_index], CL_DEVICE_TYPE_GPU,
            sizeof(device_ids)/sizeof(device_ids[0]), device_ids, 0) != CL_SUCCESS)
            continue;

        if(strstr(platform, "Intel")) // Use only Intel platfroms
        {
            log.info() << "OpenCL platform \"" << platform << "\" is used" << endl;
            m_clplatform = platforms[platform_index];
        }
    }
    if (0 == m_clplatform)
    {
        log.error() << "OpenCLFilter: Didn't find an Intel platform!" << endl;
        return CL_INVALID_PLATFORM;
    }

    return error;
}

cl_int OpenCLFilterBase::BuildKernels()
{
    log.debug() << "OpenCLFilter: Reading and compiling OCL kernels" << endl;

    cl_int error = CL_SUCCESS;

    char buildOptions[] = "-I. -Werror -cl-fast-relaxed-math";

    for(unsigned int i = 0; i < m_kernels.size(); i++)
    {
        // Create a program object from the source file
        const char *program_source_buf = m_kernels[i].program_source.c_str();

        log.debug() << "OpenCLFilter: program source:\n" << program_source_buf << endl;
        m_kernels[i].clprogram = clCreateProgramWithSource(m_clcontext, 1, &program_source_buf, NULL, &error);
        if(error) {
            log.error() << "OpenCLFilter: clCreateProgramWithSource failed. Error code: " << error << endl;
            return error;
        }

        // Build OCL kernel
        error = clBuildProgram(m_kernels[i].clprogram, 1, &m_cldevice, buildOptions, NULL, NULL);
        if (error == CL_BUILD_PROGRAM_FAILURE)
        {
            size_t buildLogSize = 0;
            cl_int logStatus = clGetProgramBuildInfo (m_kernels[i].clprogram, m_cldevice, CL_PROGRAM_BUILD_LOG, 0, NULL, &buildLogSize);
            std::vector<char> buildLog(buildLogSize + 1);
            logStatus = clGetProgramBuildInfo (m_kernels[i].clprogram, m_cldevice, CL_PROGRAM_BUILD_LOG, buildLogSize, &buildLog[0], NULL);
            log.error() << std::string(buildLog.begin(), buildLog.end()) << endl;
            return error;
        }
        else if (error)
            return error;

        // Create the kernel objects
        m_kernels[i].clkernelY = clCreateKernel(m_kernels[i].clprogram, m_kernels[i].kernelY_FuncName.c_str(), &error);
        if (error) {
            log.error() << "OpenCLFilter: clCreateKernel failed. Error code: " << error << endl;
            return error;
        }

        m_kernels[i].clkernelUV = clCreateKernel(m_kernels[i].clprogram, m_kernels[i].kernelUV_FuncName.c_str(), &error);
        if (error) {
            log.error() << "OpenCLFilter: clCreateKernel failed. Error code: " << error << endl;
            return error;
        }
    }

    log.debug() << "OpenCLFilter: " << m_kernels.size() << " kernels built" << endl;

    return error;
}

cl_int OpenCLFilterBase::AddKernel(const char* filename, const char* kernelY_name, const char* kernelUV_name)
{
    OCL_YUV_kernel kernel;
    kernel.program_source = std::string(filename);
    kernel.kernelY_FuncName = std::string(kernelY_name);
    kernel.kernelUV_FuncName = std::string(kernelUV_name);
    kernel.clprogram = 0;
    kernel.clkernelY = kernel.clkernelUV = 0;
    m_kernels.push_back(kernel);
    return CL_SUCCESS;
}

cl_int OpenCLFilterBase::SelectKernel(unsigned kNo)
{
    if (!m_bInit)
        return CL_DEVICE_NOT_FOUND;

    if (kNo >= m_kernels.size())
        return CL_INVALID_PROGRAM;

    m_activeKernel = kNo;

    return CL_SUCCESS;
}

cl_int OpenCLFilterBase::SetKernelArgs()
{
    cl_int error = CL_SUCCESS;

    // set kernelY parameters
    error = clSetKernelArg(m_kernels[m_activeKernel].clkernelY, 0, sizeof(cl_mem), &m_clbuffer[0]);
    if(error) {
        log.error() << "clSetKernelArg failed. Error code: " << error << endl;
        return error;
    }
    error = clSetKernelArg(m_kernels[m_activeKernel].clkernelY, 1, sizeof(cl_mem), &m_clbuffer[2]);
    if(error) {
        log.error() << "clSetKernelArg failed. Error code: " << error << endl;
        return error;
    }

    // set kernelUV parameters
    error = clSetKernelArg(m_kernels[m_activeKernel].clkernelUV, 0, sizeof(cl_mem), &m_clbuffer[1]);
    if(error) {
        log.error() << "clSetKernelArg failed. Error code: " << error << endl;
        return error;
    }
    error = clSetKernelArg(m_kernels[m_activeKernel].clkernelUV, 1, sizeof(cl_mem), &m_clbuffer[3]);
    if(error) {
        log.error() << "clSetKernelArg failed. Error code: " << error << endl;
        return error;
    }

    return error;
}

cl_int OpenCLFilterBase::ProcessSurface(int width, int height, mfxMemId pSurfIn, mfxMemId pSurfOut)
{
    cl_int error = CL_SUCCESS;

    error = PrepareSharedSurfaces(width, height, pSurfIn, pSurfOut);
    if (error)
    {
        ReleaseResources();
        return error;
    }

    error = ProcessSurface();
    if (error)
    {
        ReleaseResources();
        return error;
    }

    error =  ReleaseResources();
    return error;
}

cl_int OpenCLFilterBase::ProcessSurface()
{
    cl_int error = CL_SUCCESS;

    if (!m_bInit)
        error = CL_DEVICE_NOT_FOUND;

    if (m_clbuffer[0] && CL_SUCCESS == error)
    {
        cl_mem    surfaces[4] = { m_clbuffer[0],
            m_clbuffer[1],
            m_clbuffer[2],
            m_clbuffer[3] };

        if (!EnqueueAcquireSurfaces(surfaces, sizeof(surfaces)/sizeof(cl_mem)))
            return CL_DEVICE_NOT_AVAILABLE;

        // enqueue kernels
        error = clEnqueueNDRangeKernel(m_clqueue, m_kernels[m_activeKernel].clkernelY, 2, NULL, m_GlobalWorkSizeY, m_LocalWorkSizeY, 0, NULL, NULL);
        if (error) {
            log.error() << "clEnqueueNDRangeKernel for Y plane failed. Error code: " << error << endl;
            return error;
        }
        error = clEnqueueNDRangeKernel(m_clqueue, m_kernels[m_activeKernel].clkernelUV, 2, NULL, m_GlobalWorkSizeUV, m_LocalWorkSizeUV, 0, NULL, NULL);
        if (error) {
            log.error() << "clEnqueueNDRangeKernel for UV plane failed. Error code: " << error << endl;
            return error;
        }

        if (!EnqueueReleaseSurfaces(surfaces, sizeof(surfaces)/sizeof(cl_mem)))
            return CL_DEVICE_NOT_AVAILABLE;

        // flush & finish the command queue
        error = clFlush(m_clqueue);
        if (error) {
            log.error() << "clFlush failed. Error code: " << error << endl;
            return error;
        }
        error = clFinish(m_clqueue);
        if (error) {
            log.error() << "clFinish failed. Error code: " << error << endl;
            return error;
        }
    }
    return error;
}

std::string getPathToExe()
{
    const size_t module_length = 1024;
    char module_name[module_length];

#if defined(_WIN32) || defined(_WIN64)
    GetModuleFileNameA(0, module_name, module_length);
#else
    char id[module_length];
    sprintf(id, "/proc/%d/exe", getpid());
    ssize_t count = readlink(id, module_name, module_length-1);
    if (count == -1)
        return std::string("");
    module_name[count] = '\0';
#endif

    std::string exePath(module_name);
    return exePath.substr(0, exePath.find_last_of("\\/") + 1);
}

std::string readFile(const char *filename)
{
    std::cout << "Info: try to open file (" << filename << ") in the current directory"<<endl;
    std::ifstream input(filename, std::ios::in | std::ios::binary);

    if(!input.good())
    {
        // look in folder with executable
        input.clear();

        std::string module_name = getPathToExe() + std::string(filename);

        std::cout << "Info: try to open file: " << module_name << endl;
        input.open(module_name.c_str(), std::ios::binary);
    }

    if (!input)
        throw std::logic_error((std::string("Error_opening_file_\"") + std::string(filename) + std::string("\"")).c_str());

    input.seekg(0, std::ios::end);
    std::vector<char> program_source(static_cast<int>(input.tellg()));
    input.seekg(0);

    input.read(&program_source[0], program_source.size());

    return std::string(program_source.begin(), program_source.end());
}

cl_int OpenCLFilterBase::PrepareSharedSurfaces(int width, int height, mfxMemId surf_in, mfxMemId surf_out)
{
    if (!m_bInit)
        return CL_DEVICE_NOT_FOUND;

    m_currentWidth = width;
    m_currentHeight = height;

    // Setup OpenCL buffers etc.
    if (!m_clbuffer[0]) // Initialize OCL buffers in case of new workload
    {
        // Associate the shared buffer with the kernel object
        m_clbuffer[0] = CreateSharedSurface(surf_in, 0, true);
        m_clbuffer[1] = CreateSharedSurface(surf_in, 1, true);
        m_clbuffer[2] = CreateSharedSurface(surf_out, 0, false);
        m_clbuffer[3] = CreateSharedSurface(surf_out, 1, false);
        if (!m_clbuffer[0] || !m_clbuffer[1] || !m_clbuffer[2] || !m_clbuffer[3])
            return CL_DEVICE_NOT_FOUND;

        // Work sizes for Y plane
        m_GlobalWorkSizeY[0] = m_currentWidth;
        m_GlobalWorkSizeY[1] = m_currentHeight;
        m_LocalWorkSizeY[0] = chooseLocalSize(m_GlobalWorkSizeY[0], 8);
        m_LocalWorkSizeY[1] = chooseLocalSize(m_GlobalWorkSizeY[1], 8);
        m_GlobalWorkSizeY[0] = m_LocalWorkSizeY[0] * (m_GlobalWorkSizeY[0] / m_LocalWorkSizeY[0]);
        m_GlobalWorkSizeY[1] = m_LocalWorkSizeY[1] * (m_GlobalWorkSizeY[1] / m_LocalWorkSizeY[1]);

        // Work size for UV plane
        m_GlobalWorkSizeUV[0] = m_currentWidth / 2;
        m_GlobalWorkSizeUV[1] = m_currentHeight / 2;
        m_LocalWorkSizeUV[0] = chooseLocalSize(m_GlobalWorkSizeUV[0], 8);
        m_LocalWorkSizeUV[1] = chooseLocalSize(m_GlobalWorkSizeUV[1], 8);
        m_GlobalWorkSizeUV[0] = m_LocalWorkSizeUV[0] * (m_GlobalWorkSizeUV[0] / m_LocalWorkSizeUV[0]);
        m_GlobalWorkSizeUV[1] = m_LocalWorkSizeUV[1] * (m_GlobalWorkSizeUV[1] / m_LocalWorkSizeUV[1]);

        cl_int error = SetKernelArgs();
        if (error) return error;
    }
    return CL_SUCCESS;
}
