/******************************************************************************\
Copyright (c) 2018, Intel Corporation
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

#ifndef __FEI_UTILS_H__
#define __FEI_UTILS_H__

#include "sample_defs.h"

#include "mfxplugin.h"
#include "mfxplugin++.h"
#include "plugin_utils.h"
#include "plugin_loader.h"

#include "sample_utils.h"
#include "base_allocator.h"
#include "sample_hevc_fei_defs.h"
#include "fei_buffer_allocator.h"

class SurfacesPool
{
public:
    SurfacesPool(MFXFrameAllocator* allocator = NULL);
    ~SurfacesPool();

    mfxStatus AllocSurfaces(mfxFrameAllocRequest& request);
    void SetAllocator(MFXFrameAllocator* allocator);
    MFXFrameAllocator * GetAllocator(void)
    {
        return m_pAllocator;
    }
    mfxFrameSurface1* GetFreeSurface();
    mfxStatus LockSurface(mfxFrameSurface1* pSurf);
    mfxStatus UnlockSurface(mfxFrameSurface1* pSurf);

private:
    MFXFrameAllocator* m_pAllocator;
    std::vector<mfxFrameSurface1> m_pool;
    mfxFrameAllocResponse m_response;

private:
    DISALLOW_COPY_AND_ASSIGN(SurfacesPool);
};

/**********************************************************************************/

struct HevcTaskDSO;
class IYUVSource
{
public:
    IYUVSource(const SourceFrameInfo& inPars, SurfacesPool* sp)
        : m_inPars(inPars)
        , m_pOutSurfPool(sp)
    {
        MSDK_ZERO_MEMORY(m_frameInfo);
    }

    IYUVSource(IYUVSource const&) = delete;
    IYUVSource& operator=(IYUVSource const&) = delete;

    virtual ~IYUVSource() {}

    virtual void SetSurfacePool(SurfacesPool* sp)
    {
        m_pOutSurfPool = sp;
    }
    virtual mfxStatus SetBufferAllocator(std::shared_ptr<FeiBufferAllocator> & bufferAlloc)
    {
        return MFX_ERR_UNSUPPORTED;
    }
    virtual mfxStatus QueryIOSurf(mfxFrameAllocRequest* request) = 0;
    virtual mfxStatus PreInit() = 0;
    virtual mfxStatus Init()    = 0;
    virtual void      Close()   = 0;

    virtual mfxStatus GetActualFrameInfo(mfxFrameInfo & info) = 0;
    virtual mfxStatus GetFrame(mfxFrameSurface1* & pSurf) = 0;
    virtual mfxStatus GetFrame(HevcTaskDSO & task)  { return MFX_ERR_UNSUPPORTED;}

protected:
    SourceFrameInfo m_inPars;
    mfxFrameInfo    m_frameInfo;
    SurfacesPool*   m_pOutSurfPool;
};

class Decoder : public IYUVSource
{
public:
    Decoder(const SourceFrameInfo& inPars, SurfacesPool* sp, MFXVideoSession* session)
        : IYUVSource(inPars, sp)
        , m_session(session)
        , m_LastSyncp(0)
    {
        m_Bitstream.TimeStamp=(mfxU64)-1;
    }

    virtual ~Decoder()
    {
    }

    virtual mfxStatus QueryIOSurf(mfxFrameAllocRequest* request);
    virtual mfxStatus PreInit();
    virtual mfxStatus Init();
    virtual void      Close();

    virtual mfxStatus GetActualFrameInfo(mfxFrameInfo & info);
    virtual mfxStatus GetFrame(mfxFrameSurface1* & pSurf);

protected:
    mfxStatus InitDecParams(MfxVideoParamsWrapper & par);

protected:
    CSmplBitstreamReader           m_FileReader;
    mfxBitstreamWrapper            m_Bitstream;

    MFXVideoSession*               m_session;
    std::unique_ptr<MFXVideoDECODE>  m_DEC;
    MfxVideoParamsWrapper          m_par;

    mfxSyncPoint                   m_LastSyncp;

private:
    DISALLOW_COPY_AND_ASSIGN(Decoder);
};

/**********************************************************************************/

class FileHandler
{
public:
    FileHandler() : m_file(NULL) {}
    FileHandler(const msdk_char* _filename, const msdk_char* _mode)
        : m_file(NULL)
    {
        MSDK_FOPEN(m_file, _filename, _mode);
        if (m_file == NULL)
        {
            msdk_printf(MSDK_STRING("Can't open file %s in mode %s\n"), _filename, _mode);
            throw mfxError(MFX_ERR_NOT_INITIALIZED, "Opening file failed");
        }
    }
    ~FileHandler()
    {
        if (m_file)
        {
            fclose(m_file);
        }
    }

    template <typename T>
    mfxStatus Read(T* ptr, size_t size, size_t count)
    {
        if (m_file && (fread(ptr, size, count, m_file) != count))
            return MFX_ERR_DEVICE_FAILED;

        return MFX_ERR_NONE;
    }

    template <typename T>
    mfxStatus Write(T* ptr, size_t size, size_t count)
    {
        if (m_file && (fwrite(ptr, size, count, m_file) != count))
            return MFX_ERR_DEVICE_FAILED;

        return MFX_ERR_NONE;
    }

    mfxStatus Seek(mfxI32 offset, mfxI32 origin)
    {
        if (!m_file || fseek(m_file, offset, origin))
            return MFX_ERR_DEVICE_FAILED;

        return MFX_ERR_NONE;
    }

private:
    FILE* m_file;

private:
    DISALLOW_COPY_AND_ASSIGN(FileHandler);
};

/**********************************************************************************/

class HEVCEncodeParamsChecker
{
public:
    HEVCEncodeParamsChecker(mfxIMPL impl, mfxHDL device)
    {
        mfxStatus sts = m_session.Init(impl, NULL);
        if (MFX_ERR_NONE != sts) throw mfxError(MFX_ERR_NOT_INITIALIZED, "m_session.Init failed");

        sts = m_session.SetHandle(MFX_HANDLE_VA_DISPLAY, device);
        if (MFX_ERR_NONE != sts) throw mfxError(MFX_ERR_NOT_INITIALIZED, "m_session.SetHandle failed");

        mfxPluginUID pluginGuid = msdkGetPluginUID(impl, MSDK_VENCODE | MSDK_FEI, MFX_CODEC_HEVC);
        if (AreGuidsEqual(pluginGuid, MSDK_PLUGINGUID_NULL) == true)
            throw mfxError(MFX_ERR_NOT_INITIALIZED, "Can't load plug-in");

        m_plugin.reset(LoadPlugin(MFX_PLUGINTYPE_VIDEO_ENCODE, m_session, pluginGuid, 1));
        if (!m_plugin.get()) throw mfxError(MFX_ERR_NOT_INITIALIZED, "Can't load plug-in");

        m_encode.reset(new MFXVideoENCODE(m_session));
    }

    ~HEVCEncodeParamsChecker()
    {
    }

    mfxStatus Query(MfxVideoParamsWrapper & pars) // in/out
    {
        mfxStatus sts = m_encode->Query(&pars, &pars);
        MSDK_IGNORE_MFX_STS(sts, MFX_WRN_INCOMPATIBLE_VIDEO_PARAM);
        MSDK_CHECK_STATUS(sts, "HEVCEncodeParamsChecker Query failed");

        return sts;
    }

private:
    MFXVideoSession  m_session;
    std::unique_ptr<MFXPlugin>        m_plugin;
    std::unique_ptr<MFXVideoENCODE>   m_encode;

private:
    DISALLOW_COPY_AND_ASSIGN(HEVCEncodeParamsChecker);
};

/**********************************************************************************/

void DrawPixel(mfxI32 x, mfxI32 y, mfxU8 *pPic, mfxI32 nPicWidth, mfxI32 nPicHeight, mfxU8 u8Pixel);
void DrawLine(mfxI32 x0, mfxI32 y0, mfxI32 dx, mfxI32 dy, mfxU8 *pPic, mfxI32 nPicWidth, mfxI32 nPicHeight, mfxU8 u8Pixel);

/**********************************************************************************/

mfxStatus CopySurface(mfxFrameSurface1 & dst, const mfxFrameSurface1 & src);

#endif // #define __FEI_UTILS_H__
