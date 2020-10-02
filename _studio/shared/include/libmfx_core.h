// Copyright (c) 2017-2020 Intel Corporation
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

#ifndef __LIBMFX_CORE_H__
#define __LIBMFX_CORE_H__

#include <map>

#include "umc_mutex.h"
#include "libmfx_allocator.h"
#include "mfxvideo.h"
#include "mfxvideo++int.h"
#include "mfx_ext_buffers.h"
#include "fast_copy.h"
#include "libmfx_core_interface.h"

#include <memory>


class mfx_UMC_FrameAllocator;

// Virtual table size for CommonCORE should be considered fixed.
// Otherwise binary compatibility with already released plugins would be broken.

class CommonCORE : public VideoCORE
{
public:

    friend class FactoryCORE;

    virtual ~CommonCORE() override { Close(); }

    virtual mfxStatus GetHandle(mfxHandleType type, mfxHDL *handle)          override;
    virtual mfxStatus SetHandle(mfxHandleType type, mfxHDL handle)           override;

    virtual mfxStatus SetBufferAllocator(mfxBufferAllocator *)               override;
    virtual mfxStatus SetFrameAllocator(mfxFrameAllocator *allocator)        override;

    // Utility functions for memory access
    virtual mfxStatus AllocBuffer(mfxU32 nbytes, mfxU16 type, mfxMemId *mid) override;
    virtual mfxStatus LockBuffer(mfxMemId mid, mfxU8 **ptr)                  override;
    virtual mfxStatus UnlockBuffer(mfxMemId mid)                             override;
    virtual mfxStatus FreeBuffer(mfxMemId mid)                               override;

    // DEPRECATED
    virtual mfxStatus CheckHandle() override { return MFX_ERR_NONE; }

    virtual mfxStatus GetFrameHDL(mfxMemId mid, mfxHDL *handle, bool ExtendedSearch = true)               override;

    virtual mfxStatus AllocFrames(mfxFrameAllocRequest *request,
                                   mfxFrameAllocResponse *response, bool isNeedCopy = true)               override;

    virtual mfxStatus AllocFrames(mfxFrameAllocRequest *request,
                                   mfxFrameAllocResponse *response,
                                   mfxFrameSurface1 **pOpaqueSurface,
                                   mfxU32 NumOpaqueSurface)                                               override;

    virtual mfxStatus LockFrame(mfxMemId mid, mfxFrameData *ptr)                                          override;
    virtual mfxStatus UnlockFrame(mfxMemId mid, mfxFrameData *ptr = nullptr)                              override;
    virtual mfxStatus FreeFrames(mfxFrameAllocResponse *response, bool ExtendedSearch = true)             override;

    virtual mfxStatus LockExternalFrame(mfxMemId mid, mfxFrameData *ptr, bool ExtendedSearch = true)      override;
    virtual mfxStatus GetExternalFrameHDL(mfxMemId mid, mfxHDL *handle, bool ExtendedSearch = true)       override;
    virtual mfxStatus UnlockExternalFrame(mfxMemId mid, mfxFrameData *ptr=0, bool ExtendedSearch = true)  override;

    virtual mfxMemId MapIdx(mfxMemId mid)                                                                 override;

    // Get original Surface corresponding to OpaqueSurface
    virtual mfxFrameSurface1* GetNativeSurface(mfxFrameSurface1 *pOpqSurface, bool ExtendedSearch = true) override;
    // Get OpaqueSurface corresponding to Original
    virtual mfxFrameSurface1* GetOpaqSurface(mfxMemId mid, bool ExtendedSearch = true)                    override;

    // Increment Surface lock caring about opaq
    virtual mfxStatus IncreaseReference(mfxFrameData *ptr, bool ExtendedSearch = true)                    override;
    // Decrement Surface lock caring about opaq
    virtual mfxStatus DecreaseReference(mfxFrameData *ptr, bool ExtendedSearch = true)                    override;

    // no care about surface, opaq and all round. Just increasing reference
    virtual mfxStatus IncreasePureReference(mfxU16 &Locked)                                               override;
    // no care about surface, opaq and all round. Just decreasing reference
    virtual mfxStatus DecreasePureReference(mfxU16 &Locked)                                               override;

    // Get Video Accelerator.
    virtual void  GetVA(mfxHDL* phdl, mfxU16)                   override { *phdl = nullptr; }
    virtual mfxStatus CreateVA(mfxVideoParam *, mfxFrameAllocRequest *, mfxFrameAllocResponse *, UMC::FrameAllocator *) override { MFX_RETURN(MFX_ERR_UNSUPPORTED); }
    // Get the current working adapter's number
    virtual mfxU32 GetAdapterNumber()                           override { return 0; }
    //
    virtual eMFXPlatform GetPlatformType()                      override { return MFX_PLATFORM_SOFTWARE; }

    // Get Video Processing
    virtual void  GetVideoProcessing(mfxHDL* phdl)              override { *phdl = 0; }
    virtual mfxStatus CreateVideoProcessing(mfxVideoParam *)    override { MFX_RETURN(MFX_ERR_UNSUPPORTED); }

    // Get the current number of working threads
    virtual mfxU32 GetNumWorkingThreads()                       override { return m_numThreadsAvailable; }
    virtual void INeedMoreThreadsInside(const void *pComponent) override;

    virtual mfxStatus DoFastCopy(mfxFrameSurface1 *pDst, mfxFrameSurface1 *pSrc)         override;
    virtual mfxStatus DoFastCopyExtended(mfxFrameSurface1 *pDst, mfxFrameSurface1 *pSrc) override;

    virtual mfxStatus DoFastCopyWrapper(mfxFrameSurface1 *pDst, mfxU16 dstMemType, mfxFrameSurface1 *pSrc, mfxU16 srcMemType) override;

    // DEPRECATED
    virtual bool IsFastCopyEnabled()              override { return true; }

    virtual bool IsExternalFrameAllocator() const override;
    virtual eMFXHWType GetHWType()                override { return MFX_HW_UNKNOWN; }

    virtual mfxStatus CopyFrame(mfxFrameSurface1 *dst, mfxFrameSurface1 *src) override;

    virtual mfxStatus CopyBuffer(mfxU8 *, mfxU32, mfxFrameSurface1 *)         override { MFX_RETURN(MFX_ERR_UNKNOWN); }

    virtual mfxStatus CopyFrameEx(mfxFrameSurface1 *pDst, mfxU16 dstMemType, mfxFrameSurface1 *pSrc, mfxU16 srcMemType) override
    {
        return DoFastCopyWrapper(pDst, dstMemType, pSrc, srcMemType);
    }

    virtual mfxStatus IsGuidSupported(const GUID, mfxVideoParam *, bool)      override { return MFX_ERR_NONE; }

    virtual bool CheckOpaqueRequest(mfxFrameAllocRequest *request, mfxFrameSurface1 **pOpaqueSurface, mfxU32 NumOpaqueSurface, bool ExtendedSearch = true) override;

    virtual eMFXVAType GetVAType() const                   override { return MFX_HW_NO; }

    virtual bool SetCoreId(mfxU32 Id)                      override;
    virtual void* QueryCoreInterface(const MFX_GUID &guid) override;

    virtual mfxSession GetSession()                        override { return m_session; }

    virtual void SetWrapper(void* pWrp)                    override;

    virtual mfxU16 GetAutoAsyncDepth()                     override;

    virtual bool IsCompatibleForOpaq()                     override { return true; }

    // keep frame response structure describing plug-in memory surfaces
    void AddPluginAllocResponse(mfxFrameAllocResponse& response);

    // get response which corresponds required conditions: same mids and number
    mfxFrameAllocResponse* GetPluginAllocResponse(mfxFrameAllocResponse& temp_response);

    // non-virtual QueryPlatform, as we should not change vtable
    mfxStatus QueryPlatform(mfxPlatform* platform);

protected:

    CommonCORE(const mfxU32 numThreadsAvailable, const mfxSession session = nullptr);

    class API_1_19_Adapter : public IVideoCore_API_1_19
    {
    public:
        API_1_19_Adapter(CommonCORE * core) : m_core(core) {}
        virtual mfxStatus QueryPlatform(mfxPlatform* platform);

    private:
        CommonCORE *m_core;
    };


    virtual mfxStatus          DefaultAllocFrames(mfxFrameAllocRequest *request, mfxFrameAllocResponse *response);
    mfxFrameAllocator*         GetAllocatorAndMid(mfxMemId& mid);
    mfxBaseWideFrameAllocator* GetAllocatorByReq(mfxU16 type) const;
    virtual void               Close();
    mfxStatus                  FreeMidArray(mfxFrameAllocator* pAlloc, mfxFrameAllocResponse *response);
    mfxStatus                  RegisterMids(mfxFrameAllocResponse *response, mfxU16 memType, bool IsDefaultAlloc, mfxBaseWideFrameAllocator* pAlloc = 0);
    mfxStatus                  CheckTimingLog();

    bool                       GetUniqID(mfxMemId& mId);
    virtual mfxStatus          InternalFreeFrames(mfxFrameAllocResponse *response);
    bool IsEqual (const mfxFrameAllocResponse &resp1, const mfxFrameAllocResponse &resp2) const
    {
        if (resp1.NumFrameActual != resp2.NumFrameActual)
            return false;

        for (mfxU32 i=0; i < resp1.NumFrameActual; i++)
        {
            if (resp1.mids[i] != resp2.mids[i])
                return false;
        }
        return true;
    };

    //function checks if surfaces already allocated and mapped and request is consistent. Fill response if surfaces are correct
    virtual bool IsOpaqSurfacesAlreadyMapped(mfxFrameSurface1 **pOpaqueSurface, mfxU32 NumOpaqueSurface, mfxFrameAllocResponse *response, bool ExtendedSearch = true) override;

    typedef struct
    {
        mfxMemId InternalMid;
        bool isDefaultMem;
        mfxU16 memType;

    } MemDesc;

    typedef std::map<mfxMemId, MemDesc> CorrespTbl;
    typedef std::map<mfxMemId, mfxBaseWideFrameAllocator*> AllocQueue;
    typedef std::map<mfxMemId*, mfxMemId*> MemIDMap;

    typedef std::map<mfxFrameSurface1*, mfxFrameSurface1> OpqTbl;
    typedef std::map<mfxMemId, mfxFrameSurface1*> OpqTbl_MemId;
    typedef std::map<mfxFrameData*, mfxFrameSurface1*> OpqTbl_FrameData;
    typedef std::map<mfxFrameAllocResponse*, mfxU32> RefCtrTbl;


    CorrespTbl       m_CTbl;
    AllocQueue       m_AllocatorQueue;
    MemIDMap         m_RespMidQ;
    OpqTbl           m_OpqTbl;
    OpqTbl_MemId     m_OpqTbl_MemId;
    OpqTbl_FrameData m_OpqTbl_FrameData;
    RefCtrTbl        m_RefCtrTbl;

    // Number of available threads
    const
    mfxU32                                     m_numThreadsAvailable;
    // Handler to the owning session
    const
    mfxSession                                 m_session;

    // Common I/F
    mfxWideBufferAllocator                     m_bufferAllocator;
    mfxBaseWideFrameAllocator                  m_FrameAllocator;

    mfxU32                                     m_NumAllocators;
    mfxHDL                                     m_hdl;

    mfxHDL                                     m_DXVA2DecodeHandle;

    mfxHDL                                     m_D3DDecodeHandle;
    mfxHDL                                     m_D3DEncodeHandle;
    mfxHDL                                     m_D3DVPPHandle;

    bool                                       m_bSetExtBufAlloc;
    bool                                       m_bSetExtFrameAlloc;

    std::unique_ptr<mfxMemId[]>                m_pMemId;
    std::unique_ptr<mfxBaseWideFrameAllocator> m_pcAlloc;

    std::unique_ptr<FastCopy>                  m_pFastCopy;
    bool                                       m_bUseExtManager;
    UMC::Mutex                                 m_guard;

    bool                                       m_bIsOpaqMode;

    mfxU32                                     m_CoreId;

    mfx_UMC_FrameAllocator*                    m_pWrp;

    EncodeHWCaps                               m_encode_caps;
    EncodeHWCaps                               m_encode_mbprocrate;

    std::vector<mfxFrameAllocResponse>         m_PlugInMids;

    API_1_19_Adapter                           m_API_1_19;


    mfxU16                                     m_deviceId;

    CommonCORE & operator = (const CommonCORE &) = delete;
};

mfxStatus CoreDoSWFastCopy(mfxFrameSurface1 & dst, const mfxFrameSurface1 & src, int copyFlag);

#endif
