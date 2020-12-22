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

#include <assert.h>

#include <mfx_scheduler_core.h>
#include <libmfx_core_interface.h>
#include "mfx_session.h"
#include "libmfx_core.h"
#include "mfx_utils.h"

#include "mfx_common_int.h"
#include "vm_interlocked.h"

#include "mfx_umc_alloc_wrapper.h"

#include "vm_sys_info.h"

using namespace std;
//
// THE OTHER CORE FUNCTIONS HAVE IMPLICIT IMPLEMENTATION
//

FUNCTION_IMPL(CORE, SetBufferAllocator, (mfxSession session, mfxBufferAllocator *allocator), (allocator))
FUNCTION_IMPL(CORE, SetFrameAllocator, (mfxSession session, mfxFrameAllocator *allocator), (allocator))
FUNCTION_IMPL(CORE, SetHandle, (mfxSession session, mfxHandleType type, mfxHDL hdl), (type, hdl))
FUNCTION_IMPL(CORE, GetHandle, (mfxSession session, mfxHandleType type, mfxHDL *hdl), (type, hdl))

mfxStatus MFXVideoCORE_QueryPlatform(mfxSession session, mfxPlatform* platform)
{
    MFX_CHECK(session,                MFX_ERR_INVALID_HANDLE);
    MFX_CHECK(session->m_pCORE.get(), MFX_ERR_NOT_INITIALIZED);
    MFX_CHECK_NULL_PTR1(platform);

    try
    {
        /* call the codec's method */
        IVideoCore_API_1_19 * pInt = QueryCoreInterface<IVideoCore_API_1_19>(session->m_pCORE.get(), MFXICORE_API_1_19_GUID);
        if (pInt)
        {
            return pInt->QueryPlatform(platform);
        }

        platform = {};
        MFX_RETURN(MFX_ERR_UNSUPPORTED);
    }
    catch (...)
    {
        MFX_RETURN(MFX_ERR_NULL_PTR);
    }
}


mfxStatus CommonCORE::API_1_19_Adapter::QueryPlatform(mfxPlatform* platform)
{
    return m_core->QueryPlatform(platform);
}

mfxStatus CommonCORE::AllocBuffer(mfxU32 nbytes, mfxU16 type, mfxMemId *mid)
{
    UMC::AutomaticUMCMutex guard(m_guard);
    return (*m_bufferAllocator.bufferAllocator.Alloc)(m_bufferAllocator.bufferAllocator.pthis,nbytes, type, mid);
}
mfxStatus CommonCORE::LockBuffer(mfxMemId mid, mfxU8 **ptr)
{
    UMC::AutomaticUMCMutex guard(m_guard);
    return (*m_bufferAllocator.bufferAllocator.Lock)(m_bufferAllocator.bufferAllocator.pthis, mid, ptr);
}
mfxStatus CommonCORE::UnlockBuffer(mfxMemId mid)
{
    UMC::AutomaticUMCMutex guard(m_guard);
    return (*m_bufferAllocator.bufferAllocator.Unlock)(m_bufferAllocator.bufferAllocator.pthis,mid);
}
mfxStatus CommonCORE::FreeBuffer(mfxMemId mid)
{
    UMC::AutomaticUMCMutex guard(m_guard);
    return (*m_bufferAllocator.bufferAllocator.Free)(m_bufferAllocator.bufferAllocator.pthis,mid);
}

mfxStatus CommonCORE::AllocFrames(mfxFrameAllocRequest *request,
                                  mfxFrameAllocResponse *response,
                                  mfxFrameSurface1 **pOpaqueSurface,
                                  mfxU32 NumOpaqueSurface)
{
    m_bIsOpaqMode = true;

    mfxStatus sts;
    if(!request || !response)
        return MFX_ERR_NULL_PTR;
    if (!pOpaqueSurface)
        return MFX_ERR_MEMORY_ALLOC;

    if (0 == NumOpaqueSurface)
        return MFX_ERR_MEMORY_ALLOC;

    if (!CheckOpaqueRequest(request, pOpaqueSurface, NumOpaqueSurface))
        return MFX_ERR_MEMORY_ALLOC;

    if (IsOpaqSurfacesAlreadyMapped(pOpaqueSurface, NumOpaqueSurface, response))
    {
        return MFX_ERR_NONE;
    }

    sts = AllocFrames(request, response);
    MFX_CHECK_STS(sts);

    OpqTbl::iterator opq_it;
    for (mfxU32 i = 0; i < response->NumFrameActual; i++)
    {
        mfxFrameSurface1 surf = {};
        surf.Info = request->Info;
        //sts = LockFrame(response->mids[i], &surf.Data);
        //MFX_CHECK_STS(sts);
        surf.Data.MemId = response->mids[i];
        surf.Data.MemType = request->Type;
        opq_it = m_OpqTbl.insert(std::make_pair(pOpaqueSurface[i], surf)).first;

        // filling helper tables
        m_OpqTbl_MemId.insert(std::make_pair(opq_it->second.Data.MemId, pOpaqueSurface[i]));
        m_OpqTbl_FrameData.insert(std::make_pair(&opq_it->second.Data, pOpaqueSurface[i]));
    }
    mfxFrameAllocResponse* pResp = new mfxFrameAllocResponse;
    *pResp = *response;

    m_RefCtrTbl.insert(pair<mfxFrameAllocResponse*, mfxU32>(pResp, 1));
    return sts;
}

mfxStatus CommonCORE::AllocFrames(mfxFrameAllocRequest *request,
                                  mfxFrameAllocResponse *response, bool )
{
    UMC::AutomaticUMCMutex guard(m_guard);
    mfxStatus sts = MFX_ERR_NONE;
    try
    {
        MFX_CHECK_NULL_PTR2(request, response);
        mfxFrameAllocRequest temp_request = *request;

        // external allocator doesn't know how to allocate opaque surfaces
        // we can treat opaque as internal
        if (temp_request.Type & MFX_MEMTYPE_OPAQUE_FRAME)
        {
            temp_request.Type -= MFX_MEMTYPE_OPAQUE_FRAME;
            temp_request.Type |= MFX_MEMTYPE_INTERNAL_FRAME;
        }

        // external allocator
        if (m_bSetExtFrameAlloc && !(request->Type & MFX_MEMTYPE_INTERNAL_FRAME))
        {
            sts = (*m_FrameAllocator.frameAllocator.Alloc)(m_FrameAllocator.frameAllocator.pthis, &temp_request, response);

            if (MFX_ERR_UNSUPPORTED == sts)
            {
                // Default Allocator is used for internal memory allocation only
                if (request->Type & MFX_MEMTYPE_EXTERNAL_FRAME)
                    return sts;

                return this->DefaultAllocFrames(request, response);
            }
            else if (MFX_ERR_NONE == sts)
            {
                sts = RegisterMids(response, request->Type, false);
                MFX_CHECK_STS(sts);
            }
        }
        else
        {
            // Default Allocator is used for internal memory allocation only
            if (request->Type & MFX_MEMTYPE_EXTERNAL_FRAME)
                return MFX_ERR_MEMORY_ALLOC;

            return this->DefaultAllocFrames(request, response);
        }
    }
    catch(...)
    {
        return MFX_ERR_MEMORY_ALLOC;
    }
    MFX_LTRACE_I(MFX_TRACE_LEVEL_PARAMS, sts);
    return sts;
}
mfxStatus CommonCORE::DefaultAllocFrames(mfxFrameAllocRequest *request, mfxFrameAllocResponse *response)
{
    mfxStatus sts = MFX_ERR_NONE;
    if ((request->Type & MFX_MEMTYPE_DXVA2_DECODER_TARGET)||
        (request->Type & MFX_MEMTYPE_DXVA2_PROCESSOR_TARGET))
        // should be SW
        return MFX_ERR_UNSUPPORTED;

    //if (request->Type & MFX_MEMTYPE_INTERNAL_FRAME) // TBD - only internal frames can be allocated
    {
        mfxBaseWideFrameAllocator* pAlloc = GetAllocatorByReq(request->Type);
        // VPP, ENC, PAK can request frames for several times
        if (pAlloc)
            return MFX_ERR_MEMORY_ALLOC;

        m_pcAlloc.reset(new mfxWideSWFrameAllocator(request->Type));
        pAlloc = m_pcAlloc.get();

        // set frame allocator
        pAlloc->frameAllocator.pthis = pAlloc;
        // set buffer allocator for current frame single allocator
        pAlloc->wbufferAllocator.bufferAllocator = m_bufferAllocator.bufferAllocator;
        sts =  (*pAlloc->frameAllocator.Alloc)(pAlloc->frameAllocator.pthis, request, response);
        MFX_CHECK_STS(sts);
        sts = RegisterMids(response, request->Type, true, pAlloc);
        MFX_CHECK_STS(sts);
    }
    ++m_NumAllocators;
    m_pcAlloc.release();
    return sts;
}
mfxStatus CommonCORE::LockFrame(mfxMemId mid, mfxFrameData *ptr)
{
    UMC::AutomaticUMCMutex guard(m_guard);

    try
    {
        MFX_CHECK_HDL(mid);
        MFX_CHECK_NULL_PTR1(ptr);
        mfxFrameAllocator* pAlloc = GetAllocatorAndMid(mid);
        if (!pAlloc)
            return MFX_ERR_INVALID_HANDLE;
        return (*pAlloc->Lock)(pAlloc->pthis, mid, ptr);
    }
    catch(...)
    {
        return MFX_ERR_INVALID_HANDLE;
    }
}
mfxStatus CommonCORE::GetFrameHDL(mfxMemId mid, mfxHDL* handle, bool ExtendedSearch)
{
    mfxStatus sts;
    try
    {
        MFX_CHECK_HDL(mid);
        MFX_CHECK_NULL_PTR1(handle);

        mfxFrameAllocator* pAlloc = GetAllocatorAndMid(mid);
        if (!pAlloc)
        {

            // we couldn't define behavior if external allocator did not set
            if (ExtendedSearch)// try to find in another cores
            {
                sts = m_session->m_pOperatorCore->DoFrameOperation(&VideoCORE::GetFrameHDL, mid, handle);
                if (MFX_ERR_NONE == sts)
                    return sts;
            }
            return MFX_ERR_UNDEFINED_BEHAVIOR;
        }
        else
            return (*pAlloc->GetHDL)(pAlloc->pthis, mid, handle);

    }
    catch(...)
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
}
mfxStatus CommonCORE::UnlockFrame(mfxMemId mid, mfxFrameData *ptr)
{
    UMC::AutomaticUMCMutex guard(m_guard);

    try
    {
        MFX_CHECK_HDL(mid);
        mfxFrameAllocator* pAlloc = GetAllocatorAndMid(mid);
        if (!pAlloc)
            return MFX_ERR_INVALID_HANDLE;

        return (*pAlloc->Unlock)(pAlloc->pthis, mid, ptr);
    }
    catch(...)
    {
        return MFX_ERR_INVALID_HANDLE;
    }
}
mfxStatus CommonCORE::FreeFrames(mfxFrameAllocResponse *response, bool ExtendedSearch)
{
    mfxStatus sts = MFX_ERR_NONE;
    if(!response)
        return MFX_ERR_NULL_PTR;
    if (m_RefCtrTbl.size())
    {
        {
            UMC::AutomaticUMCMutex guard(m_guard);
            RefCtrTbl::iterator ref_it;
            for (ref_it = m_RefCtrTbl.begin(); ref_it != m_RefCtrTbl.end(); ref_it++)
            {
                if (IsEqual(*ref_it->first, *response))
                {
                    ref_it->second--;
                    if (0 == ref_it->second)
                    {
                        delete ref_it->first;
                        m_RefCtrTbl.erase(ref_it);
                        for (mfxU32 i = 0; i < response->NumFrameActual; i++)
                        {
                            OpqTbl_MemId::iterator memIdTbl_it = m_OpqTbl_MemId.find(response->mids[i]);
                            assert(m_OpqTbl_MemId.end() != memIdTbl_it);
                            if (m_OpqTbl_MemId.end() != memIdTbl_it)
                            {
                                mfxFrameSurface1* surf = memIdTbl_it->second;
                                OpqTbl::iterator opqTbl_it = m_OpqTbl.find(surf);
                                assert(m_OpqTbl.end() != opqTbl_it);
                                if (m_OpqTbl.end() != opqTbl_it)
                                {
                                    OpqTbl_FrameData::iterator frameDataTbl_it = m_OpqTbl_FrameData.find(&opqTbl_it->second.Data);
                                    assert(m_OpqTbl_FrameData.end() != frameDataTbl_it);
                                    if (m_OpqTbl_FrameData.end() != frameDataTbl_it)
                                    {
                                        m_OpqTbl_FrameData.erase(frameDataTbl_it);
                                    }
                                    m_OpqTbl.erase(opqTbl_it);
                                }
                                m_OpqTbl_MemId.erase(memIdTbl_it);
                            }
                        }
                        return InternalFreeFrames(response);
                    }
                    else
                    {
                        // clean up resources allocated in IsOpaqSurfacesAlreadyMapped
                        MemIDMap::iterator it = m_RespMidQ.find(response->mids);
                        if (m_RespMidQ.end() != it)
                        {
                            // Means that FreeFrames call is done from a component
                            // which operated with already mapped surfaces.
                            if (response->mids != ref_it->first->mids)
                            {
                                delete[] response->mids;
                                m_RespMidQ.erase(it);
                            }
                        }
                    }
                    return sts;
                }
            }
        }
        if (ExtendedSearch)
        {
            sts = m_session->m_pOperatorCore->DoCoreOperation(&VideoCORE::FreeFrames, response);
            if (MFX_ERR_UNDEFINED_BEHAVIOR == sts) // no opaq surfaces found
                return InternalFreeFrames(response);
            else
                return sts;
        }
    }
    else if (ExtendedSearch)
    {
        sts = m_session->m_pOperatorCore->DoCoreOperation(&VideoCORE::FreeFrames, response);
        if (MFX_ERR_UNDEFINED_BEHAVIOR == sts) // no opaq surfaces found
            return InternalFreeFrames(response);
        else
            return sts;
    }
    return MFX_ERR_UNDEFINED_BEHAVIOR;
}
mfxStatus CommonCORE::InternalFreeFrames(mfxFrameAllocResponse *response)
{
    UMC::AutomaticUMCMutex guard(m_guard);
    try
    {
        MFX_CHECK_NULL_PTR1(response);
        MFX_CHECK(response->NumFrameActual, MFX_ERR_NONE);
        mfxStatus sts = MFX_ERR_NONE;
        m_pMemId.reset(new mfxMemId[response->NumFrameActual]);
        mfxFrameAllocator* pAlloc;
        CorrespTbl::iterator ctbl_it;
        AllocQueue::iterator it;
        ctbl_it = m_CTbl.find(response->mids[0]);
        if (ctbl_it == m_CTbl.end())
            return MFX_ERR_INVALID_HANDLE;
        bool IsDefaultMem = ctbl_it->second.isDefaultMem;
        mfxMemId extMem = response->mids[0];
        mfxFrameAllocator* pFirstAlloc = GetAllocatorAndMid(extMem);
        MFX_CHECK_NULL_PTR1(pFirstAlloc);
        // checking and correspond parameters
        for (mfxU32 i = 0; i < response->NumFrameActual; i++)
        {
            extMem = response->mids[i];
            ctbl_it = m_CTbl.find(response->mids[i]);
            if (m_CTbl.end() == ctbl_it)
                return MFX_ERR_INVALID_HANDLE;
            m_pMemId[i] = ctbl_it->second.InternalMid;
            pAlloc = GetAllocatorAndMid(extMem);
            // all frames should be allocated by one allocator
            if ((IsDefaultMem != ctbl_it->second.isDefaultMem)||
                (pAlloc != pFirstAlloc))
                return MFX_ERR_INVALID_HANDLE;
        }
        // save first mid. Need for future internal memory free
        extMem = response->mids[0];
        sts = FreeMidArray(pFirstAlloc, response);
        MFX_CHECK_STS(sts);
        // delete self allocator
        if (IsDefaultMem)
        {
            it = m_AllocatorQueue.find(extMem);
            if (m_AllocatorQueue.end() == it)
                return MFX_ERR_INVALID_HANDLE;
            if (it->second)
            {
                delete it->second;
                it->second = 0;
            }
        }
        // delete self queues
        for (mfxU32 i = 0; i < response->NumFrameActual; i++)
        {
            ctbl_it = m_CTbl.find(response->mids[i]);
            if (IsDefaultMem)
                m_AllocatorQueue.erase(response->mids[i]);
            m_CTbl.erase(ctbl_it);
        }
        m_pMemId.reset();
        // we sure about response->mids
        delete[] response->mids;
        response->mids = 0;
        return sts;
    }
    catch(...)
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
}
mfxStatus  CommonCORE::LockExternalFrame(mfxMemId mid, mfxFrameData *ptr, bool ExtendedSearch)
{
    mfxStatus sts;

    try
    {
        {
            UMC::AutomaticUMCMutex guard(m_guard);

            // if exist opaque surface - take a look in them (internal surfaces)
            if (m_OpqTbl.size())
            {
                sts = LockFrame(mid, ptr);
                if (MFX_ERR_NONE == sts)
                    return sts;
            }
            MFX_CHECK_NULL_PTR1(ptr);

            if (m_bSetExtFrameAlloc)
            {
                mfxFrameAllocator* pAlloc = &m_FrameAllocator.frameAllocator;
                return (*pAlloc->Lock)(pAlloc->pthis, mid, ptr);
            }
        }
        // we couldn't define behavior if external allocator did not set
        // try to find in another cores
        if (ExtendedSearch)// try to find in another cores
        {
            sts = m_session->m_pOperatorCore->DoFrameOperation(&VideoCORE::LockExternalFrame, mid, ptr);
            if (MFX_ERR_NONE == sts)
                return sts;
        }
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
    catch(...)
    {
        return MFX_ERR_LOCK_MEMORY;
    }
}
mfxStatus  CommonCORE::GetExternalFrameHDL(mfxMemId mid, mfxHDL *handle, bool ExtendedSearch)
{
    mfxStatus sts;
    try
    {
        MFX_CHECK_NULL_PTR1(handle);

        if (m_bSetExtFrameAlloc)
        {
            mfxFrameAllocator* pAlloc = &m_FrameAllocator.frameAllocator;
            return (*pAlloc->GetHDL)(pAlloc->pthis, mid, handle);
        }

        // we couldn't define behavior if external allocator did not set
        if (ExtendedSearch)// try to find in another cores
        {
            sts = m_session->m_pOperatorCore->DoFrameOperation(&VideoCORE::GetExternalFrameHDL, mid, handle);
            if (MFX_ERR_NONE == sts)
                return sts;
        }
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
    catch(...)
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
}
mfxStatus  CommonCORE::UnlockExternalFrame(mfxMemId mid, mfxFrameData *ptr, bool ExtendedSearch)
{
    mfxStatus sts;
    if(!ptr)
        return MFX_ERR_NULL_PTR;
    try
    {
        {
            UMC::AutomaticUMCMutex guard(m_guard);
            // if exist opaque surface - take a look in them (internal surfaces)
            if (m_OpqTbl.size())
            {
                sts = UnlockFrame(mid, ptr);
                if (MFX_ERR_NONE == sts)
                    return sts;
            }

            if (m_bSetExtFrameAlloc)
            {
                mfxFrameAllocator* pAlloc = &m_FrameAllocator.frameAllocator;
                return (*pAlloc->Unlock)(pAlloc->pthis, mid, ptr);
            }
        }
        // we couldn't define behavior if external allocator did not set
        if (ExtendedSearch)// try to find in another cores
        {
            sts = m_session->m_pOperatorCore->DoFrameOperation(&VideoCORE::UnlockExternalFrame, mid, ptr);
            if (MFX_ERR_NONE == sts)
                return sts;
        }
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
    catch(...)
    {
        return MFX_ERR_LOCK_MEMORY;
    }

}
mfxMemId CommonCORE::MapIdx(mfxMemId mid)
{
    UMC::AutomaticUMCMutex guard(m_guard);
    if (0 == mid)
        return 0;

    CorrespTbl::iterator ctbl_it;
    ctbl_it = m_CTbl.find(mid);
    if (m_CTbl.end() == ctbl_it)
        return 0;
    else
        return ctbl_it->second.InternalMid;
}
mfxFrameSurface1* CommonCORE::GetNativeSurface(mfxFrameSurface1 *pOpqSurface, bool ExtendedSearch)
{
    if (0 == pOpqSurface)
        return 0;

    {
        UMC::AutomaticUMCMutex guard(m_guard);
        OpqTbl::iterator oqp_it;
        oqp_it = m_OpqTbl.find(pOpqSurface);
        if (m_OpqTbl.end() != oqp_it)
            return &oqp_it->second;
    }

    if (ExtendedSearch)
        return m_session->m_pOperatorCore->GetSurface(&VideoCORE::GetNativeSurface, pOpqSurface);

    return 0;

}
mfxFrameSurface1* CommonCORE::GetOpaqSurface(mfxMemId mid, bool ExtendedSearch)
{
    if (0 == mid)
        return 0;

    {
        UMC::AutomaticUMCMutex guard(m_guard);
        OpqTbl_MemId::iterator opq_it = m_OpqTbl_MemId.find(mid);
        if (m_OpqTbl_MemId.end() != opq_it) {
            return opq_it->second;
        }
    }

    if (ExtendedSearch)
        return m_session->m_pOperatorCore->GetSurface(&VideoCORE::GetOpaqSurface, mid);

    return 0;
}
mfxStatus CommonCORE::FreeMidArray(mfxFrameAllocator* pAlloc, mfxFrameAllocResponse *response)
{
    UMC::AutomaticUMCMutex guard(m_guard);
    MemIDMap::iterator it = m_RespMidQ.find(response->mids);
    if (m_RespMidQ.end() == it)
        return MFX_ERR_INVALID_HANDLE;

    mfxFrameAllocResponse sResponse = *response;
    mfxStatus sts;
    sResponse.mids = it->second;
    sts = (*pAlloc->Free)(pAlloc->pthis, &sResponse);
    MFX_CHECK_STS(sts);
    m_RespMidQ.erase(it);
    return sts;
}

mfxStatus CommonCORE::RegisterMids(mfxFrameAllocResponse *response, mfxU16 memType, bool IsDefaultAlloc, mfxBaseWideFrameAllocator* pAlloc)
{
    m_pMemId.reset(new mfxMemId[response->NumFrameActual]);
    mfxMemId mId;
    for (mfxU32 i = 0; i < response->NumFrameActual; i++)
    {
        MemDesc ds;
        if (!GetUniqID(mId))
        {
            return MFX_ERR_UNDEFINED_BEHAVIOR;
        }

        // add in queue only self allocators
        // need to define SW or HW allocation mode
        if (IsDefaultAlloc)
            m_AllocatorQueue.insert(pair<mfxMemId, mfxBaseWideFrameAllocator*>(mId, pAlloc));
        ds.InternalMid = response->mids[i];
        ds.isDefaultMem = IsDefaultAlloc;

        // set render target memory description
        ds.memType = memType;
        m_CTbl.insert(pair<mfxMemId, MemDesc>(mId, ds));
        m_pMemId[i] = mId;
    }
    m_RespMidQ.insert(pair<mfxMemId*, mfxMemId*>(m_pMemId.get(), response->mids));
    response->mids = m_pMemId.release();

    return  MFX_ERR_NONE;
}

CommonCORE::CommonCORE(const mfxU32 numThreadsAvailable, const mfxSession session) :
    m_numThreadsAvailable(numThreadsAvailable),
    m_session(session),
    m_NumAllocators(0),
    m_hdl(NULL),
    m_DXVA2DecodeHandle(NULL),
    m_D3DDecodeHandle(NULL),
    m_D3DEncodeHandle(NULL),
    m_D3DVPPHandle(NULL),
    m_bSetExtBufAlloc(false),
    m_bSetExtFrameAlloc(false),
    m_bUseExtManager(false),
    m_bIsOpaqMode(false),
    m_CoreId(0),
    m_pWrp(NULL),
    m_API_1_19(this),
    m_deviceId(0)
{
    m_bufferAllocator.bufferAllocator.pthis = &m_bufferAllocator;
    CheckTimingLog();
}

void CommonCORE::Close()
{
    m_CTbl.clear();
    m_AllocatorQueue.clear();
    m_OpqTbl_MemId.clear();
    m_OpqTbl_FrameData.clear();
    m_OpqTbl.clear();
    MemIDMap::iterator it;
    while(m_RespMidQ.size())
    {
        // now its NOT a mistake situation
        // all mids should be freed already except opaque shared surfaces
        it = m_RespMidQ.begin();
        delete[] it->first;
        m_RespMidQ.erase(it);
    }
}

mfxStatus CommonCORE::GetHandle(mfxHandleType type, mfxHDL *handle)
{
    MFX_CHECK_NULL_PTR1(handle);
    UMC::AutomaticUMCMutex guard(m_guard);

    switch (type)
    {

#if defined(LINUX32) || defined(LINUX64) || defined(MFX_VA_LINUX)
    case MFX_HANDLE_VA_DISPLAY:
        MFX_CHECK(m_hdl, MFX_ERR_NOT_FOUND);
        *handle = m_hdl;
        break;
#endif
    default:
        MFX_RETURN(MFX_ERR_NOT_FOUND);
    }

    return MFX_ERR_NONE;
} // mfxStatus CommonCORE::GetHandle(mfxHandleType type, mfxHDL *handle)

mfxStatus CommonCORE::SetHandle(mfxHandleType type, mfxHDL hdl)
{
    MFX_CHECK_HDL(hdl);

    ignore = type;

    MFX_RETURN(MFX_ERR_INVALID_HANDLE);
}// mfxStatus CommonCORE::SetHandle(mfxHandleType type, mfxHDL handle)

static inline mfxPlatform MakePlatform(eMFXHWType type, mfxU16 device_id)
{
    mfxPlatform platform = {};

    #if (MFX_VERSION >= 1031)
    platform.MediaAdapterType = MFX_MEDIA_INTEGRATED;
#endif

    switch (type)
    {
    case MFX_HW_SNB    : platform.CodeName = MFX_PLATFORM_SANDYBRIDGE;   break;
    case MFX_HW_IVB    : platform.CodeName = MFX_PLATFORM_IVYBRIDGE;     break;
    case MFX_HW_HSW    :
    case MFX_HW_HSW_ULT: platform.CodeName = MFX_PLATFORM_HASWELL;       break;
    case MFX_HW_VLV    : platform.CodeName = MFX_PLATFORM_BAYTRAIL;      break;
    case MFX_HW_BDW    : platform.CodeName = MFX_PLATFORM_BROADWELL;     break;
    case MFX_HW_CHT    : platform.CodeName = MFX_PLATFORM_CHERRYTRAIL;   break;
    case MFX_HW_SCL    : platform.CodeName = MFX_PLATFORM_SKYLAKE;       break;
    case MFX_HW_APL    : platform.CodeName = MFX_PLATFORM_APOLLOLAKE;    break;
    case MFX_HW_KBL    : platform.CodeName = MFX_PLATFORM_KABYLAKE;      break;
#if (MFX_VERSION >= 1025)
    case MFX_HW_GLK    : platform.CodeName = MFX_PLATFORM_GEMINILAKE;    break;
    case MFX_HW_CFL    : platform.CodeName = MFX_PLATFORM_COFFEELAKE;    break;
    case MFX_HW_CNL    : platform.CodeName = MFX_PLATFORM_CANNONLAKE;    break;
#endif
#if (MFX_VERSION >= 1027)
    case MFX_HW_ICL    :
    case MFX_HW_ICL_LP : platform.CodeName = MFX_PLATFORM_ICELAKE;       break;
#endif
#if (MFX_VERSION >= 1031)
    case MFX_HW_EHL    : platform.CodeName = MFX_PLATFORM_ELKHARTLAKE;   break;
    case MFX_HW_JSL    : platform.CodeName = MFX_PLATFORM_JASPERLAKE;    break;
    case MFX_HW_RKL    :
    case MFX_HW_TGL_LP : platform.CodeName = MFX_PLATFORM_TIGERLAKE;     break;
    case MFX_HW_DG1    :
                         platform.MediaAdapterType = MFX_MEDIA_DISCRETE;
                         platform.CodeName = MFX_PLATFORM_TIGERLAKE;     break;
    case MFX_HW_ADL_S  : platform.CodeName = MFX_PLATFORM_ALDERLAKE_S;   break;

#endif
    default:
#if (MFX_VERSION >= 1031)
                         platform.MediaAdapterType = MFX_MEDIA_UNKNOWN;
#endif
                         platform.CodeName = MFX_PLATFORM_UNKNOWN;       break;
    }

    platform.DeviceId = device_id;

    return platform;
}

mfxStatus CommonCORE::QueryPlatform(mfxPlatform* platform)
{
    MFX_CHECK_NULL_PTR1(platform);

    MFX_CHECK(m_hdl || MFX_HW_VAAPI != GetVAType(), MFX_ERR_UNDEFINED_BEHAVIOR);

    *platform = MakePlatform(GetHWType(), m_deviceId);

    return MFX_ERR_NONE;
} // mfxStatus CommonCORE::QueryPlatform(mfxPlatform* platform)

mfxStatus CommonCORE::SetBufferAllocator(mfxBufferAllocator *allocator)
{
    UMC::AutomaticUMCMutex guard(m_guard);
    if (!allocator)
        return MFX_ERR_NONE;

    if (!m_bSetExtBufAlloc)
    {
        m_bufferAllocator.bufferAllocator = *allocator;
        m_bSetExtBufAlloc = true;
        return MFX_ERR_NONE;
    }
    else
        return MFX_ERR_UNDEFINED_BEHAVIOR;
}
mfxFrameAllocator* CommonCORE::GetAllocatorAndMid(mfxMemId& mid)
{
    UMC::AutomaticUMCMutex guard(m_guard);
    CorrespTbl::iterator ctbl_it = m_CTbl.find(mid);
    if (m_CTbl.end() == ctbl_it)
        return 0;
    if (!ctbl_it->second.isDefaultMem)
    {
        if (m_bSetExtFrameAlloc)
        {
            mid = ctbl_it->second.InternalMid;
            return &m_FrameAllocator.frameAllocator;
        }
        else // error
        {
            mid = 0;
            return 0;
        }
    }
    else
    {
        AllocQueue::iterator it = m_AllocatorQueue.find(mid);
        if (it == m_AllocatorQueue.end())
        {
            mid = 0;
            return 0;
        }
        else
        {
            mid = ctbl_it->second.InternalMid;
            return &it->second->frameAllocator;
        }

    }
}
mfxBaseWideFrameAllocator* CommonCORE::GetAllocatorByReq(mfxU16 type) const
{
    AllocQueue::const_iterator it = m_AllocatorQueue.begin();
    while (it != m_AllocatorQueue.end())
    {
        // external frames should be allocated at once
        // internal frames can be allocated many times
        if ((it->second->type == type)&&
            (it->second->type & MFX_MEMTYPE_EXTERNAL_FRAME))
            return it->second;
        ++it;
    }
    return 0;
}
mfxStatus CommonCORE::SetFrameAllocator(mfxFrameAllocator *allocator)
{
    UMC::AutomaticUMCMutex guard(m_guard);
    if (!allocator)
        return MFX_ERR_NONE;

    if (!m_bSetExtFrameAlloc)
    {
        m_FrameAllocator.frameAllocator = *allocator;
        m_bSetExtFrameAlloc = true;
        m_session->m_coreInt.FrameAllocator = *allocator;
        return MFX_ERR_NONE;
    }
    else
        return MFX_ERR_UNDEFINED_BEHAVIOR;

}

// no care about surface, opaq and all round. Just increasing reference
mfxStatus CommonCORE::IncreasePureReference(mfxU16& Locked)
{
    UMC::AutomaticUMCMutex guard(m_guard);

    MFX_CHECK(Locked <= 65534, MFX_ERR_LOCK_MEMORY);

    vm_interlocked_inc16((volatile uint16_t*)&Locked);
    return MFX_ERR_NONE;
}// CommonCORE::IncreasePureReference(mfxFrameData *ptr)

// no care about surface, opaq and all round. Just increasing reference
mfxStatus CommonCORE::DecreasePureReference(mfxU16& Locked)
{
    UMC::AutomaticUMCMutex guard(m_guard);

    MFX_CHECK(Locked != 0, MFX_ERR_LOCK_MEMORY);

    vm_interlocked_dec16((volatile uint16_t*)&Locked);
    return MFX_ERR_NONE;
}// CommonCORE::IncreasePureReference(mfxFrameData *ptr)

mfxStatus CommonCORE::IncreaseReference(mfxFrameData *ptr, bool ExtendedSearch)
{
    MFX_CHECK_NULL_PTR1(ptr);
    if (ptr->Locked > 65534)
    {
        return MFX_ERR_LOCK_MEMORY;
    }
    else
    {
        {
            UMC::AutomaticUMCMutex guard(m_guard);
            // Opaque surface synchronization
            if (m_bIsOpaqMode)
            {
                OpqTbl_FrameData::iterator opq_it = m_OpqTbl_FrameData.find(ptr);
                if (m_OpqTbl_FrameData.end() != opq_it)
                {
                    vm_interlocked_inc16((volatile uint16_t*)&(opq_it->second->Data.Locked));
                    vm_interlocked_inc16((volatile uint16_t*)&ptr->Locked);
                    return MFX_ERR_NONE;
                }
            }
        }

        // we don't find in self queue let find in neigb cores
        if (ExtendedSearch)
        {
            // makes sense to remove ans tay only error return
            if (MFX_ERR_NONE != m_session->m_pOperatorCore->DoCoreOperation(&VideoCORE::IncreaseReference, ptr))
                return IncreasePureReference(ptr->Locked);
            else
                return MFX_ERR_NONE;


        }
        return MFX_ERR_INVALID_HANDLE;
    }
}

mfxStatus CommonCORE::DecreaseReference(mfxFrameData *ptr, bool ExtendedSearch)
{
    MFX_CHECK_NULL_PTR1(ptr);
    // should be positive
    if (ptr->Locked < 1)
    {
        return MFX_ERR_LOCK_MEMORY;
    }
    else
    {
        {
            UMC::AutomaticUMCMutex guard(m_guard);
            // Opaque surface synchronization
            if (m_bIsOpaqMode)
            {
                OpqTbl_FrameData::iterator opq_it = m_OpqTbl_FrameData.find(ptr);
                if (m_OpqTbl_FrameData.end() != opq_it)
                {
                    vm_interlocked_dec16((volatile uint16_t*)&(opq_it->second->Data.Locked));
                    vm_interlocked_dec16((volatile uint16_t*)&ptr->Locked);
                    return MFX_ERR_NONE;
                }
            }
        }

        // we dont find in self queue let find in neigb cores
        if (ExtendedSearch)
        {
            // makes sence to remove ans tay only error return
            if (MFX_ERR_NONE != m_session->m_pOperatorCore->DoCoreOperation(&VideoCORE::DecreaseReference, ptr))
                return DecreasePureReference(ptr->Locked);
            else
                return MFX_ERR_NONE;
        }
        return MFX_ERR_INVALID_HANDLE;
    }
}

void CommonCORE::INeedMoreThreadsInside(const void *pComponent)
{
    if ((m_session) &&
        (m_session->m_pScheduler))
    {
        ignore = MFX_STS_TRACE(m_session->m_pScheduler->ResetWaitingStatus(pComponent));
    }

} // void CommonCORE::INeedMoreThreadsInside(const void *pComponent)

bool CommonCORE::IsExternalFrameAllocator() const
{
    return m_bSetExtFrameAlloc;
}

mfxStatus CommonCORE::DoFastCopyWrapper(mfxFrameSurface1 *pDst, mfxU16 dstMemType, mfxFrameSurface1 *pSrc, mfxU16 srcMemType)
{
    mfxStatus sts = MFX_ERR_NONE;

    mfxFrameSurface1 srcTempSurface, dstTempSurface;

    mfxMemId srcMemId, dstMemId;

    memset(&srcTempSurface, 0, sizeof(mfxFrameSurface1));
    memset(&dstTempSurface, 0, sizeof(mfxFrameSurface1));

    // save original mem ids
    srcMemId = pSrc->Data.MemId;
    dstMemId = pDst->Data.MemId;

    mfxU8* srcPtr = GetFramePointer(pSrc->Info.FourCC, pSrc->Data);
    mfxU8* dstPtr = GetFramePointer(pDst->Info.FourCC, pDst->Data);

    srcTempSurface.Info = pSrc->Info;
    dstTempSurface.Info = pDst->Info;

    srcTempSurface.Data.MemId = srcMemId;
    dstTempSurface.Data.MemId = dstMemId;

    bool isSrcLocked = false;
    bool isDstLocked = false;

    if (srcMemType & MFX_MEMTYPE_EXTERNAL_FRAME)
    {
        //if (srcMemType & MFX_MEMTYPE_SYSTEM_MEMORY)
        {
            if (NULL == srcPtr)
            {
                // only if pointers are absence
                sts = LockExternalFrame(srcMemId, &srcTempSurface.Data);
                MFX_CHECK_STS(sts);

                isSrcLocked = true;
            }
            else
            {
                srcTempSurface.Data = pSrc->Data;
            }

            srcTempSurface.Data.MemId = 0;
        }
    }
    else if (srcMemType & MFX_MEMTYPE_INTERNAL_FRAME)
    {
        //if (srcMemType & MFX_MEMTYPE_SYSTEM_MEMORY)
        {
            if (NULL == srcPtr)
            {
                sts = LockFrame(srcMemId, &srcTempSurface.Data);
                MFX_CHECK_STS(sts);

                isSrcLocked = true;
            }
            else
            {
                srcTempSurface.Data = pSrc->Data;
            }

            srcTempSurface.Data.MemId = 0;
        }
    }

    if (dstMemType & MFX_MEMTYPE_EXTERNAL_FRAME)
    {
        //if (dstMemType & MFX_MEMTYPE_SYSTEM_MEMORY)
        {
            if (NULL == dstPtr)
            {
                // only if pointers are absence
                sts = LockExternalFrame(dstMemId, &dstTempSurface.Data);
                MFX_CHECK_STS(sts);

                isDstLocked = true;
            }
            else
            {
                dstTempSurface.Data = pDst->Data;
            }

            dstTempSurface.Data.MemId = 0;
        }
    }
    else if (dstMemType & MFX_MEMTYPE_INTERNAL_FRAME)
    {
        if (dstMemType & MFX_MEMTYPE_SYSTEM_MEMORY)
        {
            if (NULL == dstPtr)
            {
                // only if pointers are absence
                sts = LockFrame(dstMemId, &dstTempSurface.Data);
                MFX_CHECK_STS(sts);

                isDstLocked = true;
            }
            else
            {
                dstTempSurface.Data = pDst->Data;
            }

            dstTempSurface.Data.MemId = 0;
        }
    }

    // check that external allocator was set
    if ((NULL != pDst->Data.MemId || NULL != pSrc->Data.MemId) &&  false == m_bSetExtFrameAlloc)
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    mfxStatus fcSts = DoFastCopyExtended(&dstTempSurface, &srcTempSurface);

    if (true == isSrcLocked)
    {
        if (srcMemType & MFX_MEMTYPE_EXTERNAL_FRAME)
        {
            sts = UnlockExternalFrame(srcMemId, &srcTempSurface.Data);
            MFX_CHECK_STS(fcSts);
            MFX_CHECK_STS(sts);
        }
        else if (srcMemType & MFX_MEMTYPE_INTERNAL_FRAME)
        {
            sts = UnlockFrame(srcMemId, &srcTempSurface.Data);
            MFX_CHECK_STS(fcSts);
            MFX_CHECK_STS(sts);
        }
    }

    if (true == isDstLocked)
    {
        if (dstMemType & MFX_MEMTYPE_EXTERNAL_FRAME)
        {
            sts = UnlockExternalFrame(dstMemId, &dstTempSurface.Data);
            MFX_CHECK_STS(fcSts);
            MFX_CHECK_STS(sts);
        }
        else if (dstMemType & MFX_MEMTYPE_INTERNAL_FRAME)
        {
            sts = UnlockFrame(dstMemId, &dstTempSurface.Data);
            MFX_CHECK_STS(fcSts);
            MFX_CHECK_STS(sts);
        }
    }

    return fcSts;
}

mfxStatus CommonCORE::DoFastCopy(mfxFrameSurface1 *dst, mfxFrameSurface1 *src)
{
    UMC::AutomaticUMCMutex guard(m_guard);

    MFX_CHECK_NULL_PTR2(src, dst);

    mfxSize roi = { min(src->Info.Width, dst->Info.Width), min(src->Info.Height, dst->Info.Height) };
    MFX_CHECK(roi.width && roi.height, MFX_ERR_UNDEFINED_BEHAVIOR);

    if(!m_pFastCopy)
    {
        m_pFastCopy.reset(new FastCopy());
    }

    mfxU8 *pDst = dst->Data.Y, *pSrc = src->Data.Y;
    MFX_CHECK_NULL_PTR2(pSrc, pDst);

    mfxU32 srcPitch = src->Data.PitchLow + ((mfxU32)src->Data.PitchHigh << 16);
    mfxU32 dstPitch = dst->Data.PitchLow + ((mfxU32)dst->Data.PitchHigh << 16);

    switch (dst->Info.FourCC)
    {
    case MFX_FOURCC_NV12:

        MFX_SAFE_CALL(m_pFastCopy->Copy(pDst, dstPitch, pSrc, srcPitch, roi, COPY_SYS_TO_SYS));

        roi.height >>= 1;

        pSrc = src->Data.UV;
        pDst = dst->Data.UV;

        MFX_CHECK_NULL_PTR2(pSrc, pDst);

        return m_pFastCopy->Copy(pDst, dstPitch, pSrc, srcPitch, roi, COPY_SYS_TO_SYS);

    case MFX_FOURCC_YV12:

        MFX_SAFE_CALL(m_pFastCopy->Copy(pDst, dstPitch, pSrc, srcPitch, roi, COPY_SYS_TO_SYS));

        roi.height >>= 1;

        pSrc = src->Data.U;
        pDst = dst->Data.U;

        MFX_CHECK_NULL_PTR2(pSrc, pDst);

        roi.width >>= 1;

        srcPitch >>= 1;
        dstPitch >>= 1;

        MFX_SAFE_CALL(m_pFastCopy->Copy((mfxU8 *)pDst, dstPitch, (mfxU8 *)pSrc, srcPitch, roi, COPY_SYS_TO_SYS));

        pSrc = src->Data.V;
        pDst = dst->Data.V;

        MFX_CHECK_NULL_PTR2(pSrc, pDst);

        return m_pFastCopy->Copy((mfxU8 *)pDst, dstPitch, (mfxU8 *)pSrc, srcPitch, roi, COPY_SYS_TO_SYS);

    case MFX_FOURCC_YUY2:

        roi.width *= 2;

        return m_pFastCopy->Copy(pDst, dstPitch, pSrc, srcPitch, roi, COPY_SYS_TO_SYS);

    case MFX_FOURCC_P8:

        return m_pFastCopy->Copy(pDst, dstPitch, pSrc, srcPitch, roi, COPY_SYS_TO_SYS);

    default:
        MFX_RETURN(MFX_ERR_UNSUPPORTED);
    }

    return MFX_ERR_NONE;
}

mfxStatus CoreDoSWFastCopy(mfxFrameSurface1 & dst, const mfxFrameSurface1 & src, int copyFlag)
{
    mfxSize roi = { min(src.Info.Width, dst.Info.Width), min(src.Info.Height, dst.Info.Height) };

    // check that region of interest is valid
    MFX_CHECK(roi.width && roi.height, MFX_ERR_UNDEFINED_BEHAVIOR);

    uint32_t srcPitch = src.Data.PitchLow + ((mfxU32)src.Data.PitchHigh << 16);
    uint32_t dstPitch = dst.Data.PitchLow + ((mfxU32)dst.Data.PitchHigh << 16);

    switch (dst.Info.FourCC)
    {
    case MFX_FOURCC_P010:
#if (MFX_VERSION >= 1031)
    case MFX_FOURCC_P016:
#endif

        if (src.Info.Shift != dst.Info.Shift)
        {
            mfxU8 lshift = 0;
            mfxU8 rshift = 0;
            if (src.Info.Shift != 0)
                rshift = (uint8_t)(16 - dst.Info.BitDepthLuma);
            else
                lshift = (uint8_t)(16 - dst.Info.BitDepthLuma);

            // CopyAndShift operates with 2-byte words, no need to multiply width by 2
            MFX_SAFE_CALL(FastCopy::CopyAndShift((mfxU16*)(dst.Data.Y), dstPitch, (mfxU16 *)src.Data.Y, srcPitch, roi, lshift, rshift, copyFlag));

            roi.height >>= 1;

            return FastCopy::CopyAndShift((mfxU16*)(dst.Data.UV), dstPitch, (mfxU16 *)src.Data.UV, srcPitch, roi, lshift, rshift, copyFlag);
        }
        else
        {
            roi.width <<= 1;

            MFX_SAFE_CALL(FastCopy::Copy(dst.Data.Y, dstPitch, src.Data.Y, srcPitch, roi, copyFlag));

            roi.height >>= 1;

            return FastCopy::Copy(dst.Data.UV, dstPitch, src.Data.UV, srcPitch, roi, copyFlag);
        }


    case MFX_FOURCC_P210:
        roi.width <<= 1;

        MFX_SAFE_CALL(FastCopy::Copy(dst.Data.Y, dstPitch, src.Data.Y, srcPitch, roi, copyFlag));

        return FastCopy::Copy(dst.Data.UV, dstPitch, src.Data.UV, srcPitch, roi, copyFlag);

    case MFX_FOURCC_NV12:
        MFX_SAFE_CALL(FastCopy::Copy(dst.Data.Y, dstPitch, src.Data.Y, srcPitch, roi, copyFlag));

        roi.height >>= 1;
        return FastCopy::Copy(dst.Data.UV, dstPitch, src.Data.UV, srcPitch, roi, copyFlag);

    case MFX_FOURCC_NV16:
        MFX_SAFE_CALL(FastCopy::Copy(dst.Data.Y, dstPitch, src.Data.Y, srcPitch, roi, copyFlag));

        return FastCopy::Copy(dst.Data.UV, dstPitch, src.Data.UV, srcPitch, roi, copyFlag);

    case MFX_FOURCC_YV12:

        MFX_SAFE_CALL(FastCopy::Copy(dst.Data.Y, dstPitch, src.Data.Y, srcPitch, roi, copyFlag));

        roi.width  >>= 1;
        roi.height >>= 1;

        srcPitch >>= 1;
        dstPitch >>= 1;

        MFX_SAFE_CALL(FastCopy::Copy(dst.Data.U, dstPitch, src.Data.U, srcPitch, roi, copyFlag));

        return FastCopy::Copy(dst.Data.V, dstPitch, src.Data.V, srcPitch, roi, copyFlag);

    case MFX_FOURCC_UYVY:
        roi.width *= 2;

        return FastCopy::Copy(dst.Data.U, dstPitch, src.Data.U, srcPitch, roi, copyFlag);

    case MFX_FOURCC_YUY2:
        roi.width *= 2;

        return FastCopy::Copy(dst.Data.Y, dstPitch, src.Data.Y, srcPitch, roi, copyFlag);

#if (MFX_VERSION >= 1027)
    case MFX_FOURCC_Y210:
#if (MFX_VERSION >= 1031)
    case MFX_FOURCC_Y216:
#endif

        MFX_CHECK_NULL_PTR1(src.Data.Y);

        //we use 8u copy, so we need to increase ROI to handle 16 bit samples
#if defined(_WIN32) || defined(_WIN64)
        if (src.Info.Shift != dst.Info.Shift)
        {
            roi.width *= 2; // CopyAndShift operates with 2-byte words
            mfxU8 lshift = 0;
            mfxU8 rshift = 0;
            if (src.Info.Shift != 0)
                rshift = (mfxU8)(16 - dst.Info.BitDepthLuma);
            else
                lshift = (mfxU8)(16 - dst.Info.BitDepthLuma);

            return FastCopy::CopyAndShift((mfxU16*)(dst.Data.Y), dstPitch, (mfxU16 *)src.Data.Y, srcPitch, roi, lshift, rshift, copyFlag);
        }
        else
#endif
        {
            roi.width *= 4;
            return FastCopy::Copy(dst.Data.Y, dstPitch, src.Data.Y, srcPitch, roi, copyFlag);
        }


    case MFX_FOURCC_Y410:
    {
        MFX_CHECK_NULL_PTR1(dst.Data.Y410);

        mfxU8* ptrDst = (mfxU8*)dst.Data.Y410;
        mfxU8* ptrSrc = (mfxU8*)src.Data.Y410;

        roi.width *= 4;

        return FastCopy::Copy(ptrDst, dstPitch, ptrSrc, srcPitch, roi, copyFlag);
    }
#endif

#if (MFX_VERSION >= 1031)
    case MFX_FOURCC_Y416:
        MFX_CHECK_NULL_PTR1(src.Data.U16);

        //we use 8u copy, so we need to increase ROI to handle 16 bit samples
#if defined(_WIN32) || defined(_WIN64)
        if (src.Info.Shift != dst.Info.Shift)
        {
            roi.width *= 4; // CopyAndShift operates with 2-byte words
            mfxU8 lshift = 0;
            mfxU8 rshift = 0;
            if (src.Info.Shift != 0)
                rshift = (mfxU8)(16 - dst.Info.BitDepthLuma);
            else
                lshift = (mfxU8)(16 - dst.Info.BitDepthLuma);

            return FastCopy::CopyAndShift(dst.Data.U16, dstPitch, src.Data.U16, srcPitch, roi, lshift, rshift, copyFlag);
        }
        else
#endif
        {
            roi.width *= 8;
            return FastCopy::Copy((mfxU8*)dst.Data.U16, dstPitch, (mfxU8*)src.Data.U16, srcPitch, roi, copyFlag);
        }
#endif

#if defined (MFX_ENABLE_FOURCC_RGB565)
    case MFX_FOURCC_RGB565:
    {
        mfxU8* ptrSrc = src.Data.B;
        mfxU8* ptrDst = dst.Data.B;

        roi.width *= 2;

        return FastCopy::Copy(ptrDst, dstPitch, ptrSrc, srcPitch, roi, copyFlag);
    }
#endif // MFX_ENABLE_FOURCC_RGB565

    case MFX_FOURCC_RGB3:
    {
        mfxU8* ptrSrc = min({ src.Data.R, src.Data.G, src.Data.B });
        mfxU8* ptrDst = min({ dst.Data.R, dst.Data.G, dst.Data.B });

        roi.width *= 3;

        return FastCopy::Copy(ptrDst, dstPitch, ptrSrc, srcPitch, roi, copyFlag);
    }
#ifdef MFX_ENABLE_RGBP
    case MFX_FOURCC_RGBP:
    {
        mfxU8* ptrSrc = src.Data.B;
        mfxU8* ptrDst = dst.Data.B;
        MFX_SAFE_CALL(FastCopy::Copy(ptrDst, dstPitch, ptrSrc, srcPitch, roi, copyFlag));

        ptrSrc = src.Data.G;
        ptrDst = dst.Data.G;
        MFX_SAFE_CALL(FastCopy::Copy(ptrDst, dstPitch, ptrSrc, srcPitch, roi, copyFlag));

        ptrSrc = src.Data.R;
        ptrDst = dst.Data.R;

        return FastCopy::Copy(ptrDst, dstPitch, ptrSrc, srcPitch, roi, copyFlag);
    }
#endif

    case MFX_FOURCC_AYUV:
    case MFX_FOURCC_RGB4:
    case MFX_FOURCC_BGR4:
    case MFX_FOURCC_A2RGB10:
    {
        mfxU8* ptrSrc = min({ src.Data.R, src.Data.G, src.Data.B });
        mfxU8* ptrDst = min({ dst.Data.R, dst.Data.G, dst.Data.B });

        roi.width *= 4;

        return FastCopy::Copy(ptrDst, dstPitch, ptrSrc, srcPitch, roi, copyFlag);
    }
    case MFX_FOURCC_ARGB16:
    case MFX_FOURCC_ABGR16:
    {
        mfxU8* ptrSrc = min({ src.Data.R, src.Data.G, src.Data.B });
        mfxU8* ptrDst = min({ dst.Data.R, dst.Data.G, dst.Data.B });

        roi.width *= 8;

        return FastCopy::Copy(ptrDst, dstPitch, ptrSrc, srcPitch, roi, copyFlag);
    }
    case MFX_FOURCC_P8:
        return FastCopy::Copy(dst.Data.Y, dstPitch, src.Data.Y, srcPitch, roi, copyFlag);

    default:
        MFX_RETURN(MFX_ERR_UNSUPPORTED);
    }
}

mfxStatus CommonCORE::DoFastCopyExtended(mfxFrameSurface1 *pDst, mfxFrameSurface1 *pSrc)
{
    // up mutex
    UMC::AutomaticUMCMutex guard(m_guard);

    mfxStatus sts;

    sts = CheckFrameData(pSrc);
    MFX_CHECK_STS(sts);

    sts = CheckFrameData(pDst);
    MFX_CHECK_STS(sts);

    // CheckFrameData should be added

    // check that only memId or pointer are passed
    // otherwise don't know which type of memory copying is requested
    if (
        (NULL != pDst->Data.Y && NULL != pDst->Data.MemId) ||
        (NULL != pSrc->Data.Y && NULL != pSrc->Data.MemId)
        )
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    bool isSrcLocked = false;
    bool isDstLocked = false;

    int copyFlag = COPY_SYS_TO_SYS;

    if (NULL != pSrc->Data.MemId)
    {
        // lock external frame
        sts = LockExternalFrame(pSrc->Data.MemId, &pSrc->Data);
        MFX_CHECK_STS(sts);
        isSrcLocked = true;
        copyFlag = COPY_VIDEO_TO_SYS;
    }

    if (NULL != pDst->Data.MemId)
    {
        sts = LockExternalFrame(pDst->Data.MemId, &pDst->Data);
        MFX_CHECK_STS(sts);
        isDstLocked = true;
        copyFlag = COPY_SYS_TO_VIDEO;
    }

    // system memories were passed
    // use common way to copy frames
    sts = CoreDoSWFastCopy(*pDst, *pSrc, copyFlag);

    if (isDstLocked)
    {
        // unlock external frame
        sts = UnlockExternalFrame(pDst->Data.MemId, &pDst->Data);
        MFX_CHECK_STS(sts);
    }

    if (isSrcLocked)
    {
        // unlock external frame
        sts = UnlockExternalFrame(pSrc->Data.MemId, &pSrc->Data);
        MFX_CHECK_STS(sts);
    }

    return MFX_ERR_NONE;
}

mfxStatus CommonCORE::CopyFrame(mfxFrameSurface1 *dst, mfxFrameSurface1 *src)
{
    if(!dst || !src)
        return MFX_ERR_NULL_PTR;
    if(!LumaIsNull(src) && !LumaIsNull(dst)) //input video frame is locked or system, call old copy function
    {
        mfxU16 srcMemType = MFX_MEMTYPE_SYSTEM_MEMORY | MFX_MEMTYPE_EXTERNAL_FRAME;
        mfxU16 dstMemType = MFX_MEMTYPE_SYSTEM_MEMORY | MFX_MEMTYPE_EXTERNAL_FRAME;

        return DoFastCopyWrapper(dst, dstMemType, src, srcMemType);
    }
    else if(src->Data.MemId && !LumaIsNull(dst))
    {
        mfxHDLPair srcHandle = {};
        mfxU16 srcMemType = MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET;
        mfxU16 dstMemType = MFX_MEMTYPE_SYSTEM_MEMORY;
        mfxStatus sts = GetExternalFrameHDL(src->Data.MemId, (mfxHDL *)&srcHandle);
        if(MFX_ERR_UNDEFINED_BEHAVIOR == sts)
            srcMemType |= MFX_MEMTYPE_INTERNAL_FRAME;
        else
            srcMemType |= MFX_MEMTYPE_EXTERNAL_FRAME;

        dstMemType |= MFX_MEMTYPE_EXTERNAL_FRAME;

        sts = DoFastCopyWrapper(dst, dstMemType, src, srcMemType);
        MFX_CHECK_STS(sts);
        return sts;
    }
    else if(!LumaIsNull(src) && dst->Data.MemId)
    {
        mfxHDLPair dstHandle = {};
        mfxU16 dstMemType = MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET;
        mfxU16 srcMemType = MFX_MEMTYPE_SYSTEM_MEMORY;
        mfxStatus sts = GetExternalFrameHDL(dst->Data.MemId, (mfxHDL *)&dstHandle);
        if(MFX_ERR_UNDEFINED_BEHAVIOR == sts)
            dstMemType |= MFX_MEMTYPE_INTERNAL_FRAME;
        else
            dstMemType |= MFX_MEMTYPE_EXTERNAL_FRAME;

        srcMemType |= MFX_MEMTYPE_EXTERNAL_FRAME;

        sts = DoFastCopyWrapper(dst, dstMemType, src, srcMemType);
        MFX_CHECK_STS(sts);
        return sts;
    }
    else if(src->Data.MemId && dst->Data.MemId)
    {
        mfxHDLPair dstHandle = {};
        mfxHDLPair srcHandle = {};
        mfxU16 dstMemType = MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET;
        mfxU16 srcMemType = MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET;
        mfxStatus sts = GetExternalFrameHDL(dst->Data.MemId, (mfxHDL *)&dstHandle);
        if(MFX_ERR_UNDEFINED_BEHAVIOR == sts)
            dstMemType |= MFX_MEMTYPE_INTERNAL_FRAME;
        else
            dstMemType |= MFX_MEMTYPE_EXTERNAL_FRAME;
        sts = GetExternalFrameHDL(src->Data.MemId, (mfxHDL *)&srcHandle);
        if(MFX_ERR_UNDEFINED_BEHAVIOR == sts)
            srcMemType |= MFX_MEMTYPE_INTERNAL_FRAME;
        else
            srcMemType |= MFX_MEMTYPE_EXTERNAL_FRAME;

        sts = DoFastCopyWrapper(dst, dstMemType, src, srcMemType);
        return sts;
    }
    else
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
}


mfxStatus CommonCORE::CheckTimingLog()
{

    return MFX_ERR_NONE;
}


bool CommonCORE::CheckOpaqueRequest(mfxFrameAllocRequest *request,
                                    mfxFrameSurface1 **pOpaqueSurface,
                                    mfxU32 NumOpaqueSurface,
                                    bool ExtendedSearch)
{
    if (!pOpaqueSurface || !request)
        return false;

    if (request->NumFrameMin != NumOpaqueSurface)
        return false;

    if (!(request->Type  & MFX_MEMTYPE_OPAQUE_FRAME))
        return false;

    if (m_OpqTbl.size())
    {
        OpqTbl::iterator oqp_it;
        bool isOpaqAllocated = false;
        mfxU32 i = 0;
        // each opaq surface has correspondence in already allocated OR
        // no one opaq surface sholud already allocated
        for (;i < request->NumFrameMin;i++)
        {
            oqp_it = m_OpqTbl.find(pOpaqueSurface[i]);
            if (oqp_it != m_OpqTbl.end())
            {
                isOpaqAllocated = true;
            }
            else if (isOpaqAllocated)
                return false;
        }
    }
    if (ExtendedSearch)// try to find in another cores
    {
        bool sts = m_session->m_pOperatorCore->CheckOpaqRequest(request, pOpaqueSurface, NumOpaqueSurface);
        return sts;
    }

    return true;

}

bool CommonCORE::IsOpaqSurfacesAlreadyMapped(mfxFrameSurface1 **pOpaqueSurface,
                                             mfxU32 NumOpaqueSurface,
                                             mfxFrameAllocResponse *response,
                                             bool ExtendedSearch)
{
    if (!pOpaqueSurface || !response)
        return false;

    {
        UMC::AutomaticUMCMutex guard(m_guard);

        mfxU32 i = 0;
        OpqTbl::iterator oqp_it;
        oqp_it = m_OpqTbl.find(pOpaqueSurface[i]);
        // consistent already checked in CheckOpaqueRequest function
        if (oqp_it != m_OpqTbl.end())
        {
            m_pMemId.reset();
            response->mids = new mfxMemId[NumOpaqueSurface];

            for (; i < NumOpaqueSurface; i++)
            {
                oqp_it = m_OpqTbl.find(pOpaqueSurface[i]);
                if (oqp_it == m_OpqTbl.end())
                    return false;

                response->mids[i] = oqp_it->second.Data.MemId;
            }

            response->NumFrameActual = (mfxU16)NumOpaqueSurface;

            RefCtrTbl::iterator ref_it;
            for (ref_it = m_RefCtrTbl.begin(); ref_it != m_RefCtrTbl.end(); ref_it++)
            {
                if (IsEqual(*ref_it->first, *response))
                {
                    ref_it->second++;

                    mfxFrameAllocResponse* opaq_response = ref_it->first;

                    MemIDMap::iterator it = m_RespMidQ.find(opaq_response->mids);
                    if (m_RespMidQ.end() == it)
                        return false;

                    mfxMemId * native_mids = it->second;

                    m_RespMidQ.insert(pair<mfxMemId*, mfxMemId*>(response->mids, native_mids));
                    return true;
                }
            }
            return false; // unexpected behavior
        }
    }

    if (ExtendedSearch)
    {
        bool sts = m_session->m_pOperatorCore->IsOpaqSurfacesAlreadyMapped(pOpaqueSurface, NumOpaqueSurface, response);
        return sts;
    }

    return false;

}

// 15 bits - uniq surface Id
// 15 bits score Id
bool CommonCORE::GetUniqID(mfxMemId& id)
{
    size_t count = 1;
    CorrespTbl::iterator ctbl_it;
    for (; count < (1 << 15); count++)
    {
        ctbl_it = m_CTbl.find((mfxMemId)(count | (m_CoreId << 15)));
        if (ctbl_it == m_CTbl.end())
        {
            id = (mfxMemId)(count | (m_CoreId << 15));
            return true;
        }
    }
    return false;
}

bool  CommonCORE::SetCoreId(mfxU32 Id)
{
    if (m_CoreId < (1 << 15))
    {
        m_CoreId = Id;
        return true;
    }
    return false;
}

void* CommonCORE::QueryCoreInterface(const MFX_GUID &guid)
{
    if (MFXIVideoCORE_GUID == guid)
        return (void*) this;

    if (MFXIEXTERNALLOC_GUID == guid && m_bSetExtFrameAlloc)
        return &m_FrameAllocator.frameAllocator;

    if (MFXICORE_API_1_19_GUID == guid)
    {
        return &m_API_1_19;
    }

    return nullptr;
}

void CommonCORE::SetWrapper(void* pWrp)
{
    m_pWrp = (mfx_UMC_FrameAllocator *)pWrp;
}

mfxU16 CommonCORE::GetAutoAsyncDepth()
{
    return (mfxU16)vm_sys_info_get_cpu_num();
}


// keep frame response structure describing plug-in memory surfaces
void CommonCORE::AddPluginAllocResponse(mfxFrameAllocResponse& response)
{
    m_PlugInMids.push_back(response);
}

// get response which corresponds required conditions: same mids and number
mfxFrameAllocResponse *CommonCORE::GetPluginAllocResponse(mfxFrameAllocResponse& temp_response)
{
    std::vector<mfxFrameAllocResponse>::iterator ref_it;
    for (ref_it = m_PlugInMids.begin(); ref_it != m_PlugInMids.end(); ref_it++)
    {
        if (IsEqual(*ref_it, temp_response))
        {
            temp_response = *ref_it;
            m_PlugInMids.erase(ref_it);
            return &temp_response;
        }
    }
    return NULL;

}
