// Copyright (c) 2017-2019 Intel Corporation
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

#ifndef _MFX_ALLOC_WRAPPER_H_
#define _MFX_ALLOC_WRAPPER_H_

#include <vector>
#include <memory> // unique_ptr

#include "mfx_common.h"
#include "umc_memory_allocator.h"
#include "umc_frame_allocator.h"
#include "umc_frame_data.h"

#include "mfxvideo++int.h"

#define MFX_UMC_MAX_ALLOC_SIZE 128

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// mfx_UMC_MemAllocator - buffer allocator
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class mfx_UMC_MemAllocator : public UMC::MemoryAllocator
{
    DYNAMIC_CAST_DECL(mfx_UMC_MemAllocator, MemoryAllocator)

public:
    mfx_UMC_MemAllocator(void);
    virtual ~mfx_UMC_MemAllocator(void);

    // Initiates object
    virtual UMC::Status InitMem(UMC::MemoryAllocatorParams *pParams, VideoCORE* mfxCore);

    // Closes object and releases all allocated memory
    virtual UMC::Status Close();

    // Allocates or reserves physical memory and return unique ID
    // Sets lock counter to 0
    virtual UMC::Status Alloc(UMC::MemID *pNewMemID, size_t Size, uint32_t Flags, uint32_t Align = 16);

    // Lock() provides pointer from ID. If data is not in memory (swapped)
    // prepares (restores) it. Increases lock counter
    virtual void *Lock(UMC::MemID MID);

    // Unlock() decreases lock counter
    virtual UMC::Status Unlock(UMC::MemID MID);

    // Notifies that the data wont be used anymore. Memory can be free
    virtual UMC::Status Free(UMC::MemID MID);

    // Immediately deallocates memory regardless of whether it is in use (locked) or no
    virtual UMC::Status DeallocateMem(UMC::MemID MID);

protected:
    VideoCORE* m_pCore;
};


enum  {
    mfx_UMC_ReallocAllowed = 1,
} mfx_UMC_FrameAllocator_Flags;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// mfx_UMC_FrameAllocator
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class mfx_UMC_FrameAllocator : public UMC::FrameAllocator
{
    DYNAMIC_CAST_DECL(mfx_UMC_FrameAllocator, UMC::FrameAllocator)

public:
    mfx_UMC_FrameAllocator(void);
    virtual ~mfx_UMC_FrameAllocator(void);

    // Initiates object
    virtual UMC::Status InitMfx(UMC::FrameAllocatorParams *pParams,
                                VideoCORE* mfxCore,
                                const mfxVideoParam *params,
                                const mfxFrameAllocRequest *request,
                                mfxFrameAllocResponse *response,
                                bool isUseExternalFrames,
                                bool isSWplatform);

    // Closes object and releases all allocated memory
    virtual UMC::Status Close();

    // Allocates or reserves physical memory and returns unique ID
    // Sets lock counter to 0
    virtual UMC::Status Alloc(UMC::FrameMemID *pNewMemID, const UMC::VideoDataInfo * info, uint32_t flags);

    virtual UMC::Status GetFrameHandle(UMC::FrameMemID memId, void * handle);

    // Lock() provides pointer from ID. If data is not in memory (swapped)
    // prepares (restores) it. Increases lock counter
    virtual const UMC::FrameData* Lock(UMC::FrameMemID mid);

    // Unlock() decreases lock counter
    virtual UMC::Status Unlock(UMC::FrameMemID mid);

    // Notifies that the data won't be used anymore. Memory can be freed.
    virtual UMC::Status IncreaseReference(UMC::FrameMemID mid);

    // Notifies that the data won't be used anymore. Memory can be freed.
    virtual UMC::Status DecreaseReference(UMC::FrameMemID mid);

    virtual UMC::Status Reset();

    virtual mfxStatus SetCurrentMFXSurface(mfxFrameSurface1 *srf, bool isOpaq);

    virtual mfxFrameSurface1*  GetSurface(UMC::FrameMemID index, mfxFrameSurface1 *surface_work, const mfxVideoParam * videoPar);
    virtual mfxStatus          PrepareToOutput(mfxFrameSurface1 *surface_work, UMC::FrameMemID index, const mfxVideoParam * videoPar, bool isOpaq);
    mfxI32 FindSurface(mfxFrameSurface1 *surf, bool isOpaq);
    mfxI32 FindFreeSurface();

    void SetSfcPostProcessingFlag(bool flagToSet);

    void SetExternalFramesResponse(mfxFrameAllocResponse *response);
    mfxFrameSurface1 * GetInternalSurface(UMC::FrameMemID index);
    mfxFrameSurface1 * GetSurfaceByIndex(UMC::FrameMemID index);

    mfxMemId ConvertMemId(UMC::FrameMemID index)
    {
        return m_frameDataInternal.GetSurface(index).Data.MemId;
    };

protected:
    struct  surf_descr
    {
        surf_descr(mfxFrameSurface1* FrameSurface, bool isUsed):FrameSurface(FrameSurface),
                                                                isUsed(isUsed)
        {
        };
        surf_descr():FrameSurface(0),
                     isUsed(false)
        {
        };
        mfxFrameSurface1* FrameSurface;
        bool              isUsed;
    };

    virtual UMC::Status Free(UMC::FrameMemID mid);

    virtual mfxI32 AddSurface(mfxFrameSurface1 *surface);

    class InternalFrameData
    {
        class FrameRefInfo
        {
        public:
            FrameRefInfo();
            void Reset();

            mfxU32 m_referenceCounter;
        };

        typedef std::pair<mfxFrameSurface1, UMC::FrameData> FrameInfo;

    public:

        mfxFrameSurface1 & GetSurface(mfxU32 index);
        UMC::FrameData   & GetFrameData(mfxU32 index);
        void ResetFrameData(mfxU32 index);
        mfxU32 IncreaseRef(mfxU32 index);
        mfxU32 DecreaseRef(mfxU32 index);

        bool IsValidMID(mfxU32 index) const;

        void AddNewFrame(mfx_UMC_FrameAllocator * alloc, mfxFrameSurface1 *surface, UMC::VideoDataInfo * info);

        mfxU32 GetSize() const;

        void Close();
        void Reset();

        void Resize(mfxU32 size);

    private:
        std::vector<FrameInfo>  m_frameData;
        std::vector<FrameRefInfo>  m_frameDataRefs;
    };

    InternalFrameData m_frameDataInternal;

    std::vector<surf_descr> m_extSurfaces;

    mfxI32        m_curIndex;

    bool m_IsUseExternalFrames;
    bool m_sfcVideoPostProcessing;

    mfxFrameInfo m_surface_info;  // for copying

    UMC::VideoDataInfo m_info;

    VideoCORE* m_pCore;

    mfxFrameAllocResponse *m_externalFramesResponse;

    bool       m_isSWDecode;
    mfxU16     m_IOPattern;
};


class mfx_UMC_FrameAllocator_D3D : public mfx_UMC_FrameAllocator
{
public:
    virtual mfxStatus PrepareToOutput(mfxFrameSurface1 *surface_work, UMC::FrameMemID index, const mfxVideoParam * videoPar, bool isOpaq);
};

#if defined (MFX_ENABLE_MJPEG_VIDEO_DECODE)
class VideoVppJpegD3D9;

class mfx_UMC_FrameAllocator_D3D_Converter : public mfx_UMC_FrameAllocator_D3D
{
    std::unique_ptr<VideoVppJpegD3D9> m_pCc;

    mfxStatus InitVideoVppJpegD3D9(const mfxVideoParam *params);
    mfxStatus FindSurfaceByMemId(const UMC::FrameData* in, bool isOpaq, const mfxHDLPair &hdlPair,
                                 // output param
                                 mfxFrameSurface1 &surface);
public:
    virtual UMC::Status InitMfx(UMC::FrameAllocatorParams *pParams, 
                                VideoCORE* mfxCore, 
                                const mfxVideoParam *params, 
                                const mfxFrameAllocRequest *request, 
                                mfxFrameAllocResponse *response, 
                                bool isUseExternalFrames,
                                bool isSWplatform) override;

    // suppose that Close() calls Reset(), so override only Reset()
    virtual UMC::Status Reset() override;

    typedef struct
    {
        int32_t colorFormat;
        size_t UOffset;
        size_t VOffset;
    } JPEG_Info;

    void SetJPEGInfo(JPEG_Info * jpegInfo);

    mfxStatus StartPreparingToOutput(mfxFrameSurface1 *surface_work, UMC::FrameData* in, const mfxVideoParam * par, mfxU16 *taskId, bool isOpaq);
    mfxStatus CheckPreparingToOutput(mfxFrameSurface1 *surface_work, UMC::FrameData* in, const mfxVideoParam * par, mfxU16 taskId);

protected:

    mfxStatus ConvertToNV12(UMC::FrameMemID index, mfxFrameSurface1 *dst);
    JPEG_Info  m_jpegInfo;
};

#endif // #if defined (MFX_ENABLE_MJPEG_VIDEO_DECODE) && defined (MFX_VA_WIN)

#endif //_MFX_ALLOC_WRAPPER_H_
