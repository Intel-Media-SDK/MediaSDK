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


class mfx_UMC_FrameAllocator;

// Virtual table size for CommonCORE should be considered fixed.
// Otherwise binary compatibility with already released plugins would be broken.

class CommonCORE : public VideoCORE
{
public:

    friend class FactoryCORE;

    virtual ~CommonCORE(void);

    virtual mfxStatus GetHandle(mfxHandleType type, mfxHDL *handle);
    virtual mfxStatus SetHandle(mfxHandleType type, mfxHDL handle);

    virtual mfxStatus SetBufferAllocator(mfxBufferAllocator *allocator);
    virtual mfxStatus SetFrameAllocator(mfxFrameAllocator *allocator);

    // Utility functions for memory access
    virtual mfxStatus  AllocBuffer(mfxU32 nbytes, mfxU16 type, mfxMemId *mid);
    virtual mfxStatus  LockBuffer(mfxMemId mid, mfxU8 **ptr);
    virtual mfxStatus  UnlockBuffer(mfxMemId mid);
    virtual mfxStatus  FreeBuffer(mfxMemId mid);

    // DEPRECATED
    virtual mfxStatus  CheckHandle();

    virtual mfxStatus  GetFrameHDL(mfxMemId mid, mfxHDL *handle, bool ExtendedSearch = true);

    virtual mfxStatus  AllocFrames(mfxFrameAllocRequest *request, 
                                   mfxFrameAllocResponse *response, bool isNeedCopy = true);
   
    virtual mfxStatus  AllocFrames(mfxFrameAllocRequest *request, 
                                   mfxFrameAllocResponse *response,
                                   mfxFrameSurface1 **pOpaqueSurface, 
                                   mfxU32 NumOpaqueSurface);

    virtual mfxStatus  LockFrame(mfxMemId mid, mfxFrameData *ptr);
    virtual mfxStatus  UnlockFrame(mfxMemId mid, mfxFrameData *ptr=0);
    virtual mfxStatus  FreeFrames(mfxFrameAllocResponse *response, bool ExtendedSearch = true);

    virtual mfxStatus  LockExternalFrame(mfxMemId mid, mfxFrameData *ptr, bool ExtendedSearch = true);
    virtual mfxStatus  GetExternalFrameHDL(mfxMemId mid, mfxHDL *handle, bool ExtendedSearch = true);
    virtual mfxStatus  UnlockExternalFrame(mfxMemId mid, mfxFrameData *ptr=0, bool ExtendedSearch = true);

    virtual mfxMemId MapIdx(mfxMemId mid);

    // Get original Surface corresponding to OpaqueSurface
    virtual mfxFrameSurface1* GetNativeSurface(mfxFrameSurface1 *pOpqSurface, bool ExtendedSearch = true);
    // Get OpaqueSurface corresponding to Original
    virtual mfxFrameSurface1* GetOpaqSurface(mfxMemId mid, bool ExtendedSearch = true);



    // Increment Surface lock caring about opaq
    virtual mfxStatus  IncreaseReference(mfxFrameData *ptr, bool ExtendedSearch = true);
    // Decrement Surface lock caring about opaq
    virtual mfxStatus  DecreaseReference(mfxFrameData *ptr, bool ExtendedSearch = true);

    // no care about surface, opaq and all round. Just increasing reference
    virtual mfxStatus IncreasePureReference(mfxU16 &Locked);
    // no care about surface, opaq and all round. Just decreasing reference
    virtual mfxStatus DecreasePureReference(mfxU16 &Locked);

    // Get Video Accelerator.
    virtual void  GetVA(mfxHDL* phdl, mfxU16 type);
    virtual mfxStatus CreateVA(mfxVideoParam * , mfxFrameAllocRequest *, mfxFrameAllocResponse *, UMC::FrameAllocator *) { return MFX_ERR_UNSUPPORTED; }
    // Get the current working adapter's number
    virtual mfxU32 GetAdapterNumber(void) {return 0;}
    //
    virtual eMFXPlatform GetPlatformType() {return  MFX_PLATFORM_SOFTWARE;}

    // Get Video Processing
    virtual void  GetVideoProcessing(mfxHDL* phdl) {*phdl = 0;}
    virtual mfxStatus CreateVideoProcessing(mfxVideoParam *) { return MFX_ERR_UNSUPPORTED; }

    // Get the current number of working threads
    virtual mfxU32 GetNumWorkingThreads(void) {return m_numThreadsAvailable;}
    virtual void INeedMoreThreadsInside(const void *pComponent);

    virtual mfxStatus DoFastCopy(mfxFrameSurface1 *pDst, mfxFrameSurface1 *pSrc);
    virtual mfxStatus DoFastCopyExtended(mfxFrameSurface1 *pDst, mfxFrameSurface1 *pSrc);

    virtual mfxStatus DoFastCopyWrapper(mfxFrameSurface1 *pDst, mfxU16 dstMemType, mfxFrameSurface1 *pSrc, mfxU16 srcMemType);

    // DEPRECATED
    virtual bool IsFastCopyEnabled(void);

    virtual bool IsExternalFrameAllocator() const;
    virtual eMFXHWType   GetHWType() { return MFX_HW_UNKNOWN; }

    virtual
    mfxStatus CopyFrame(mfxFrameSurface1 *dst, mfxFrameSurface1 *src);

    virtual
    mfxStatus CopyBuffer(mfxU8 * /*dst*/, mfxU32 /*dst_size*/, mfxFrameSurface1 * /*src*/) {return MFX_ERR_UNKNOWN;}

    virtual
    mfxStatus CopyFrameEx(mfxFrameSurface1 *pDst, mfxU16 dstMemType, mfxFrameSurface1 *pSrc, mfxU16 srcMemType) {return DoFastCopyWrapper(pDst, dstMemType, pSrc, srcMemType);}

    virtual mfxStatus IsGuidSupported(const GUID /*guid*/, mfxVideoParam * /*par*/, bool /*isEncoder = false*/) { return MFX_ERR_NONE; };

    bool CheckOpaqueRequest(mfxFrameAllocRequest *request, mfxFrameSurface1 **pOpaqueSurface, mfxU32 NumOpaqueSurface, bool ExtendedSearch = true);
    
    virtual eMFXVAType   GetVAType() const {return MFX_HW_NO; };

    virtual bool SetCoreId(mfxU32 Id);
    virtual void* QueryCoreInterface(const MFX_GUID &guid);

    virtual mfxSession GetSession() {return m_session;}

    virtual void SetWrapper(void* pWrp);

    virtual mfxU16 GetAutoAsyncDepth();

    virtual bool IsCompatibleForOpaq() {return true;};  

    // keep frame response structure dwscribing plug-in memory surfaces
    void AddPluginAllocResponse(mfxFrameAllocResponse& response);

    // get response which corresponds required conditions: same mids and number
    mfxFrameAllocResponse* GetPluginAllocResponse(mfxFrameAllocResponse& temp_response);

    // non-virtual QueryPlatform, as we should not change vtable
    mfxStatus QueryPlatform(mfxPlatform* platform);

protected:
    
    CommonCORE(const mfxU32 numThreadsAvailable, const mfxSession session = NULL);

    template <class T, bool isSingle>
    class s_ptr
    {
    public:
        s_ptr():m_ptr(0)
        {
        };
        ~s_ptr()
        {
            reset(0);
        }
        T* get()
        {
            return m_ptr;
        }
        T* pop()
        {
            T* ptr = m_ptr;
            m_ptr = 0;
            return ptr;
        }
        void reset(T* ptr = NULL)
        {
            if (m_ptr)
            {
                if (isSingle)
                    delete m_ptr;
                else
                    delete[] m_ptr;
            }
            m_ptr = ptr;
        }
    protected:
        T* m_ptr;
    private:
        s_ptr(const s_ptr&);
        void operator =(s_ptr &);
    };

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
    bool IsOpaqSurfacesAlreadyMapped(mfxFrameSurface1 **pOpaqueSurface, mfxU32 NumOpaqueSurface, mfxFrameAllocResponse *response, bool ExtendedSearch = true);

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


    CorrespTbl m_CTbl;
    AllocQueue m_AllocatorQueue;
    MemIDMap   m_RespMidQ;
    OpqTbl     m_OpqTbl;
    OpqTbl_MemId m_OpqTbl_MemId;
    OpqTbl_FrameData m_OpqTbl_FrameData;
    RefCtrTbl  m_RefCtrTbl;

    // Number of available threads
    const
    mfxU32 m_numThreadsAvailable;
    // Handler to the owning session
    const
    mfxSession m_session;

    // Common I/F
    mfxWideBufferAllocator m_bufferAllocator;
    mfxBaseWideFrameAllocator m_FrameAllocator;

    mfxU32 m_NumAllocators;
    mfxHDL                     m_hdl;

    mfxHDL                     m_DXVA2DecodeHandle;

    mfxHDL                     m_D3DDecodeHandle;
    mfxHDL                     m_D3DEncodeHandle;
    mfxHDL                     m_D3DVPPHandle;

    bool m_bSetExtBufAlloc;
    bool m_bSetExtFrameAlloc;

    s_ptr<mfxMemId, false> m_pMemId;
    s_ptr<mfxBaseWideFrameAllocator, true> m_pcAlloc;

    s_ptr<FastCopy, true> m_pFastCopy;
    bool m_bUseExtManager;
    UMC::Mutex m_guard;

    bool        m_bIsOpaqMode;

    mfxU32      m_CoreId;

    mfx_UMC_FrameAllocator *m_pWrp;

    EncodeHWCaps  m_encode_caps;
    EncodeHWCaps  m_encode_mbprocrate;

    std::vector<mfxFrameAllocResponse> m_PlugInMids;

    API_1_19_Adapter m_API_1_19;

private:
    // Forbid the assignment operator
    CommonCORE & operator = (const CommonCORE &);
};

mfxStatus CoreDoSWFastCopy(mfxFrameSurface1 *pDst, mfxFrameSurface1 *pSrc, int copyFlag);

#endif
