// Copyright (c) 2018 Intel Corporation
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

#include "mfx_common.h"
#if defined(MFX_ENABLE_H265_VIDEO_ENCODE)

#include "mfx_common.h"

#include "mfx_h265_encode_hw_utils.h"
#include "mfx_h265_encode_hw_bs.h"
#include "vm_time.h"
#include <algorithm>
#include <functional>
#include <list>
#include <assert.h>
#include "mfx_common_int.h"
namespace MfxHwH265Encode
{

template<class T, class A> mfxStatus Insert(A& _to, mfxU32 _where, T const & _what)
{
    MFX_CHECK(_where + 1 < (sizeof(_to)/sizeof(_to[0])), MFX_ERR_UNDEFINED_BEHAVIOR);
    memmove(&_to[_where + 1], &_to[_where], sizeof(_to)-(_where + 1) * sizeof(_to[0]));
    _to[_where] = _what;
    return MFX_ERR_NONE;
}

template<class A> mfxStatus Remove(A& _from, mfxU32 _where, mfxU32 _num = 1)
{
    const mfxU32 S0 = sizeof(_from[0]);
    const mfxU32 S = sizeof(_from);
    const mfxU32 N = S / S0;

    MFX_CHECK(_where < N && _num <= (N - _where), MFX_ERR_UNDEFINED_BEHAVIOR);

    if (_where + _num < N)
        memmove(&_from[_where], &_from[_where + _num], S - ((_where + _num) * S0));

    memset(&_from[N - _num], IDX_INVALID, S0 * _num);

    return MFX_ERR_NONE;
}

mfxU32 CountL1(DpbArray const & dpb, mfxI32 poc)
{
    mfxU32 c = 0;
    for (mfxU32 i = 0; !isDpbEnd(dpb, i); i ++)
        c += dpb[i].m_poc > poc;
    return c;
}
bool IsIdr(mfxU32 type)
{
    return !!(type & MFX_FRAMETYPE_IDR);
}
bool IsB(mfxU32 type)
{
    return !!(type & MFX_FRAMETYPE_B);
}
bool IsP(mfxU32 type)
{
    return !!(type & MFX_FRAMETYPE_P);
}
bool IsRef(mfxU32 type)
{
    return !!(type & MFX_FRAMETYPE_REF);
}
mfxU32 GetEncodingOrder(mfxU32 displayOrder, mfxU32 begin, mfxU32 end, mfxU32 &level, mfxU32 before, bool & ref)
{
    assert(displayOrder >= begin);
    assert(displayOrder <  end);

    ref = (end - begin > 1);

    mfxU32 pivot = (begin + end) / 2;
    if (displayOrder == pivot)
        return level + before;

    level ++;
    if (displayOrder < pivot)
        return GetEncodingOrder(displayOrder, begin, pivot,  level , before, ref);
    else
        return GetEncodingOrder(displayOrder, pivot + 1, end, level, before + pivot - begin, ref);
}

mfxU32 GetBiFrameLocation(mfxU32 i, mfxU32 num, bool &ref, mfxU32 &level)
{
    ref  = false;
    level = 1;
    return GetEncodingOrder(i, 0, num, level, 0 ,ref);
}

mfxU8 GetPFrameLevel(mfxU32 i, mfxU32 num)
{
    if (i == 0 || i >= num) return 0;
    mfxU32 level = 1;
    mfxU32 begin = 0;
    mfxU32 end = num;
    mfxU32 t = (begin + end + 1) / 2;

    while (t != i)
    {
        level++;
        if (i > t)
            begin = t;
        else
            end = t;
        t = (begin + end + 1) / 2;
    }
    return (mfxU8)level;
}

mfxU8 PLayer(
    mfxU32                order, // (task.m_poc - prevTask.m_lastIPoc)
    MfxVideoParam const & par)
{
    if (par.isLowDelay())
    {
        return Min<mfxU8>(7, GetPFrameLevel((order / (par.isField() ? 2 : 1)) % par.PPyrInterval, par.PPyrInterval));
    }

    return 0;
}

template <class T> mfxU32 BPyrReorder(std::vector<T> brefs, bool bField)
{
    mfxU32 num = (mfxU32)brefs.size();
    if (brefs[0]->m_bpo == (mfxU32)MFX_FRAMEORDER_UNKNOWN)
    {
        bool bRef = false;     
        if (!bField)
        {
            for (mfxU32 i = 0; i < num; i++)
            {
                brefs[i]->m_bpo = GetBiFrameLocation(i, num, bRef, brefs[i]->m_level);
                if (bRef)
                    brefs[i]->m_frameType |= MFX_FRAMETYPE_REF;
            }
        }
        else
        {
            for (mfxU32 i = 0; i < num /2; i++)
            {
                brefs[2*i]->m_bpo = 2*GetBiFrameLocation(i, num/2, bRef, brefs[2*i]->m_level);
                brefs[2 * i]->m_level = 2 * brefs[2 * i]->m_level;
                brefs[2*i]->m_frameType |= MFX_FRAMETYPE_REF; // the first field is always reference
               
                 // second field is exist
                if ((2 * i + 1) < num)
                {
                    brefs[2 * i + 1]->m_bpo = 2*GetBiFrameLocation(i, num / 2, bRef, brefs[2*i +1]->m_level);
                    brefs[2 * i + 1]->m_level = 2 * brefs[2 * i + 1]->m_level; 
                    if (bRef)
                        brefs[2 * i + 1]->m_frameType |= MFX_FRAMETYPE_REF;
                }
            }
        }
    }
    mfxU32 minBPO = (mfxU32)MFX_FRAMEORDER_UNKNOWN;
    mfxU32 ind = 0;
    for (mfxU32 i = 0; i < num; i++)
    {
        if (brefs[i]->m_bpo < minBPO)
        {
            ind = i;
            minBPO = brefs[i]->m_bpo;
        }
    }
    return ind;
}
template <class T> bool IsL1Ready(
    DpbArray const & dpb,
    mfxI32 poc,
    T begin,
    T end,
    bool bField,
    bool flush)
{
    mfxU32 c = 0;
    mfxI32 pocL1 = 0;
    for (mfxU32 i = 0; !isDpbEnd(dpb, i); i++)
    {
        if (dpb[i].m_poc > poc)
        {
            c++;
            pocL1 = dpb[i].m_poc;
        }
    }
    if (c == 0)
        return false; // waiting for one non B frame
    else if ((!bField) || (c > 1))
        return true; //one non-B is enought for progressive and two for interlace
    else
    {
        //need to check if pair's reference field is present into m_reordering
        T top = begin;
        while (top != end && top->m_poc <= pocL1)
            top++;
        if (top != end)
        {
            if (IsB(top->m_frameType))
                return true; // only one non-B
            else
                return false; //wait for next nonB frame
        }
        else
        {
            if (flush)
                return true; //only one non-B (no input nonB)
            else
                return false; // wait for second field

        }
    }

}

template<class T> T Reorder(
    MfxVideoParam const & par,
    DpbArray const & dpb,
    T begin,
    T end,
    bool flush)
{
    T top  = begin;
    T b0 = end; // 1st non-ref B with L1 > 0
    std::vector<T> brefs;
    while (top != end && IsB(top->m_frameType))
    {
        if (IsL1Ready(dpb, top->m_poc, begin, end, par.isField(), flush))
        {
            if (par.isBPyramid())
                brefs.push_back(top);
            else if (IsRef(top->m_frameType))
            {
                if (b0 == end || (top->m_poc - b0->m_poc < par.isField() + 2))
                    return top;
            }
            else if (b0 == end)
                b0 = top;
        }
        top++;
    }

    if (!brefs.empty())
    {
        return brefs[BPyrReorder(brefs,par.isField())];
    }

    if (b0 != end)
        return b0;

    if (flush && top == end && begin != end)
    {
        top--;
        top->m_frameType = MFX_FRAMETYPE_P | MFX_FRAMETYPE_REF;
        if (top->m_secondField && top != begin)
        {
            top--;
            top->m_frameType = MFX_FRAMETYPE_P | MFX_FRAMETYPE_REF;
        }
    }
    //pair of P|P fields is replaced on B|P fields
    if ( par.bFieldReord && top != end && IsP(top->m_frameType))
    {
        if (!top->m_secondField)
        {
            T next = top;
            next++;
            if (next == end && !flush)
            {
                return next; // waiting for second field. It is needed for reordering.
            }
            if (next != end && IsP(next->m_frameType) && (next->m_secondField))
            {
                top->m_secondField = true;
                next->m_secondField = false;
                return next;
            }
        }
        else
        {
            top->m_frameType &= ~MFX_FRAMETYPE_P;
            top->m_frameType |= MFX_FRAMETYPE_B;
        }

    }

    return top;
}

MfxFrameAllocResponse::MfxFrameAllocResponse()
    : m_core(0)
    , m_isExternal(true)
{
    Zero(*(mfxFrameAllocResponse*)this);
}

MfxFrameAllocResponse::~MfxFrameAllocResponse()
{
    Free();
}

void MfxFrameAllocResponse::Free()
{
    if (m_core == 0)
        return;

    mfxFrameAllocator & fa = m_core->FrameAllocator();
    mfxCoreParam par = {};

    m_core->GetCoreParam(&par);

    if ((par.Impl & 0xF00) == MFX_IMPL_VIA_D3D11)
    {
        for (size_t i = 0; i < m_responseQueue.size(); i++)
        {
            fa.Free(fa.pthis, &m_responseQueue[i]);
        }
        m_responseQueue.resize(0);
    }
    else
    {
        if (mids)
        {
            NumFrameActual = m_numFrameActualReturnedByAllocFrames;
            fa.Free(fa.pthis, this);
            mids = 0;
        }
    }
    m_core = nullptr;
}

mfxStatus MfxFrameAllocResponse::Alloc(
    MFXCoreInterface *     core,
    mfxFrameAllocRequest & req,
    bool                   /*isCopyRequired*/)
{
    mfxFrameAllocator & fa = core->FrameAllocator();
    mfxCoreParam par = {};
    mfxFrameAllocResponse & response = *(mfxFrameAllocResponse*)this;

    core->GetCoreParam(&par);

    req.NumFrameSuggested = req.NumFrameMin;

    if ((par.Impl & 0xF00) == MFX_IMPL_VIA_D3D11)
    {
        mfxFrameAllocRequest tmp = req;
        tmp.NumFrameMin = tmp.NumFrameSuggested = 1;

        m_responseQueue.resize(req.NumFrameMin);
        m_mids.resize(req.NumFrameMin);

        for (int i = 0; i < req.NumFrameMin; i++)
        {
            mfxStatus sts = fa.Alloc(fa.pthis, &tmp, &m_responseQueue[i]);
            MFX_CHECK_STS(sts);
            m_mids[i] = m_responseQueue[i].mids[0];
        }

        mids = &m_mids[0];
        NumFrameActual = req.NumFrameMin;
    }
    else
    {
        mfxStatus sts = fa.Alloc(fa.pthis, &req, &response);
        MFX_CHECK_STS(sts);
    }

    if (NumFrameActual < req.NumFrameMin)
        return MFX_ERR_MEMORY_ALLOC;

    m_locked.resize(req.NumFrameMin, 0);
    std::fill(m_locked.begin(), m_locked.end(), 0);

    m_flag.resize(req.NumFrameMin, 0);
    std::fill(m_flag.begin(), m_flag.end(), 0);

    m_core = core;
    m_numFrameActualReturnedByAllocFrames = NumFrameActual;
    NumFrameActual = req.NumFrameMin;
    m_info = req.Info;
    m_isExternal = false;

    return MFX_ERR_NONE;
}

mfxU32 MfxFrameAllocResponse::FindFreeResourceIndex(mfxFrameSurface1* external_surf)
{
    if (m_isExternal && external_surf)
    {
        mfxU32 i = 0;

        if (0 == m_mids.size())
            m_info = external_surf->Info;

        for (i = 0; i < m_mids.size(); i ++)
        {
            if (m_mids[i] == external_surf->Data.MemId)
            {
                m_locked[i] = 0;
                m_flag[i]=0;
                return i;
            }
        }

        m_mids.push_back(external_surf->Data.MemId);
        m_locked.push_back(0);
        m_flag.push_back(0);

        mids = &m_mids[0];

        m_numFrameActualReturnedByAllocFrames = NumFrameActual = (mfxU16)m_mids.size();

        return i;
    }

    return MfxHwH265Encode::FindFreeResourceIndex(*this);
}

mfxU32 MfxFrameAllocResponse::Lock(mfxU32 idx)
{
    if (idx >= m_locked.size())
        return 0;
    assert(m_locked[idx] < 0xffffffff);
    return ++m_locked[idx];
}

void MfxFrameAllocResponse::ClearFlag(mfxU32 idx)
{
   assert (idx < m_flag.size());
   if (idx < m_flag.size())
   {
        m_flag[idx] = 0;
   }
}
void MfxFrameAllocResponse::SetFlag(mfxU32 idx, mfxU32 flag)
{
   assert (idx < m_flag.size());
   if (idx < m_flag.size())
   {
        m_flag[idx] |= flag;
   }
}
mfxU32 MfxFrameAllocResponse::GetFlag (mfxU32 idx)
{
   assert (idx < m_flag.size());
   if (idx < m_flag.size())
   {
        return m_flag[idx];
   }
   return 0;
}

void MfxFrameAllocResponse::Unlock()
{
    std::fill(m_locked.begin(), m_locked.end(), 0);
    std::fill(m_flag.begin(), m_flag.end(), 0);
}

mfxU32 MfxFrameAllocResponse::Unlock(mfxU32 idx)
{
    if (idx >= m_locked.size())
        return mfxU32(-1);
    assert(m_locked[idx] > 0);
    return --m_locked[idx];
}

mfxU32 MfxFrameAllocResponse::Locked(mfxU32 idx) const
{
    return (idx < m_locked.size()) ? m_locked[idx] : 1;
}

mfxU32 FindFreeResourceIndex(
    MfxFrameAllocResponse const & pool,
    mfxU32                        startingFrom)
{
    for (mfxU32 i = startingFrom; i < pool.NumFrameActual; i++)
        if (pool.Locked(i) == 0)
            return i;
    return 0xFFFFFFFF;
}

mfxMemId AcquireResource(
    MfxFrameAllocResponse & pool,
    mfxU32                  index)
{
    if (index > pool.NumFrameActual)
        return 0;
    pool.Lock(index);
    pool.ClearFlag(index);
    return pool.mids[index];
}

void ReleaseResource(
    MfxFrameAllocResponse & pool,
    mfxMemId                mid)
{
    for (mfxU32 i = 0; i < pool.NumFrameActual; i++)
    {
        if (pool.mids[i] == mid)
        {
            pool.Unlock(i);
            break;
        }
    }
}

mfxStatus GetNativeHandleToRawSurface(
    MFXCoreInterface &    core,
    MfxVideoParam const & video,
    Task const &          task,
    mfxHDLPair &          handle)
{
    mfxStatus sts = MFX_ERR_NONE;
    mfxFrameAllocator & fa = core.FrameAllocator();
    mfxExtOpaqueSurfaceAlloc const & opaq = video.m_ext.Opaque;
    mfxFrameSurface1 * surface = task.m_surf_real;

    Zero(handle);
    mfxHDL * nativeHandle = &handle.first;

    if (   video.IOPattern == MFX_IOPATTERN_IN_SYSTEM_MEMORY
        || (video.IOPattern == MFX_IOPATTERN_IN_OPAQUE_MEMORY && (opaq.In.Type & MFX_MEMTYPE_SYSTEM_MEMORY)))
        sts = fa.GetHDL(fa.pthis, task.m_midRaw, nativeHandle);
    else if (   video.IOPattern == MFX_IOPATTERN_IN_VIDEO_MEMORY
             || video.IOPattern == MFX_IOPATTERN_IN_OPAQUE_MEMORY)
    {
        if (task.m_midRaw == NULL)
            sts = core.GetFrameHandle(&surface->Data, nativeHandle);
        else
            sts = fa.GetHDL(fa.pthis, task.m_midRaw, nativeHandle);
    }
    else
        return (MFX_ERR_UNDEFINED_BEHAVIOR);

    if (nativeHandle == 0)
        return (MFX_ERR_UNDEFINED_BEHAVIOR);

    return sts;
}

mfxStatus CopyRawSurfaceToVideoMemory(
    MFXCoreInterface &    core,
    MfxVideoParam const & video,
    Task const &          task)
{
    mfxStatus sts = MFX_ERR_NONE;
    mfxExtOpaqueSurfaceAlloc const & opaq = video.m_ext.Opaque;
    mfxFrameSurface1 * surface = task.m_surf_real;

    if (   video.IOPattern == MFX_IOPATTERN_IN_SYSTEM_MEMORY
        || (video.IOPattern == MFX_IOPATTERN_IN_OPAQUE_MEMORY && (opaq.In.Type & MFX_MEMTYPE_SYSTEM_MEMORY)))
    {
        mfxFrameAllocator & fa = core.FrameAllocator();
        mfxFrameData d3dSurf = {};
        mfxFrameData sysSurf = surface->Data;
        d3dSurf.MemId = task.m_midRaw;
        bool needUnlock = false;

        mfxFrameSurface1 surfSrc = { {}, video.mfx.FrameInfo, sysSurf };
        mfxFrameSurface1 surfDst = { {}, video.mfx.FrameInfo, d3dSurf };

        if (surfDst.Info.FourCC == MFX_FOURCC_P010
            )
            surfDst.Info.Shift = 1; // convert to native shift in core.CopyFrame() if required

        if (!sysSurf.Y)
        {
            sts = fa.Lock(fa.pthis, sysSurf.MemId, &surfSrc.Data);
            MFX_CHECK_STS(sts);
            needUnlock = true;
        }

        //sts = fa.Lock(fa.pthis, d3dSurf.MemId, &surfDst.Data);
        //MFX_CHECK_STS(sts);

        sts = core.CopyFrame(&surfDst, &surfSrc);

        if (needUnlock)
        {
            sts = fa.Unlock(fa.pthis, sysSurf.MemId, &surfSrc.Data);
            MFX_CHECK_STS(sts);
        }

        //sts = fa.Unlock(fa.pthis, d3dSurf.MemId, &surfDst.Data);
        //MFX_CHECK_STS(sts);
    }

    return sts;
}

namespace ExtBuffer
{
    bool Construct(mfxVideoParam const & par, mfxExtHEVCParam& buf, mfxExtBuffer* pBuffers[], mfxU16 & numbuffers)
    {
        if (!Construct<mfxVideoParam, mfxExtHEVCParam>(par, buf, pBuffers, numbuffers))
        {
            buf.PicWidthInLumaSamples  = Align(par.mfx.FrameInfo.CropW > 0 ? (mfxU16)(par.mfx.FrameInfo.CropW + par.mfx.FrameInfo.CropX) : par.mfx.FrameInfo.Width, CODED_PIC_ALIGN_W);
            buf.PicHeightInLumaSamples = Align(par.mfx.FrameInfo.CropH > 0 ? (mfxU16)(par.mfx.FrameInfo.CropH + par.mfx.FrameInfo.CropY)  : par.mfx.FrameInfo.Height, CODED_PIC_ALIGN_H);

            return false;
        }
        if (buf.PicWidthInLumaSamples== 0)
            buf.PicWidthInLumaSamples  = Align(par.mfx.FrameInfo.CropW > 0 ? (mfxU16)(par.mfx.FrameInfo.CropW + par.mfx.FrameInfo.CropX) : par.mfx.FrameInfo.Width, CODED_PIC_ALIGN_W);
        else
            buf.PicWidthInLumaSamples  = Align(buf.PicWidthInLumaSamples, CODED_PIC_ALIGN_W);

        if (buf.PicHeightInLumaSamples == 0)
            buf.PicHeightInLumaSamples = Align(par.mfx.FrameInfo.CropH > 0 ? (mfxU16)(par.mfx.FrameInfo.CropH + par.mfx.FrameInfo.CropY)  : par.mfx.FrameInfo.Height, CODED_PIC_ALIGN_H);
        else
            buf.PicHeightInLumaSamples = Align(buf.PicHeightInLumaSamples, CODED_PIC_ALIGN_H);

        return true;
    }


    mfxStatus CheckBuffers(mfxVideoParam const & par, const mfxU32 allowed[], mfxU32 notDetected[], mfxU32 size)
    {
        MFX_CHECK_NULL_PTR1(par.ExtParam);
        bool bDublicated = false;
        mfxU32 j = 0;

        for (mfxU32 i = 0; i < par.NumExtParam; i++)
        {
            MFX_CHECK_NULL_PTR1(par.ExtParam[i]);
            for (j = 0; j < size; j++)
            {
                if (par.ExtParam[i]->BufferId == allowed[j])
                {
                    if (notDetected[j] == par.ExtParam[i]->BufferId)
                        notDetected[j] = 0;
                    else
                        bDublicated = true;
                    break;
                }
            }
            if (j >= size)
                return MFX_ERR_INVALID_VIDEO_PARAM;
        }
        return bDublicated ? MFX_ERR_UNDEFINED_BEHAVIOR : MFX_ERR_NONE;
    }
};

MfxVideoParam::MfxVideoParam()
    : BufferSizeInKB  (0)
    , InitialDelayInKB(0)
    , TargetKbps      (0)
    , MaxKbps         (0)
    , LTRInterval     (0)
    , PPyrInterval    (0)
    , LCUSize         (0)
    , HRDConformance  (false)
    , RawRef          (false)
    , bROIViaMBQP     (false)
    , bMBQPInput      (false)
    , RAPIntra        (false)
    , bFieldReord     (false)
    , bNonStandardReord (false)
{
    Zero(*(mfxVideoParam*)this);
    Zero(m_platform);
}

MfxVideoParam::MfxVideoParam(MfxVideoParam const & par)
{
     Construct(par);

     Copy(m_vps, par.m_vps);
     Copy(m_sps, par.m_sps);
     Copy(m_pps, par.m_pps);
     Copy(m_platform, par.m_platform);

     CopyCalcParams(par);
}

MfxVideoParam::MfxVideoParam(mfxVideoParam const & par)
    : BufferSizeInKB  (0)
    , InitialDelayInKB(0)
    , TargetKbps      (0)
    , MaxKbps         (0)
    , LTRInterval     (0)
    , PPyrInterval    (0)
    , LCUSize         (0)
    , HRDConformance  (false)
    , RawRef          (false)
    , bROIViaMBQP     (false)
    , bMBQPInput      (false)
    , RAPIntra        (false)
    , bFieldReord     (false)
    , bNonStandardReord(false)
{
    Zero(*(mfxVideoParam*)this);
    Zero(m_platform);
    Construct(par);
    SyncVideoToCalculableParam();
}

void MfxVideoParam::CopyCalcParams(MfxVideoParam const & par)
{
    BufferSizeInKB   = par.BufferSizeInKB;
    InitialDelayInKB = par.InitialDelayInKB;
    TargetKbps       = par.TargetKbps;
    MaxKbps          = par.MaxKbps;
    LTRInterval      = par.LTRInterval;
    PPyrInterval     = par.PPyrInterval;
    LCUSize          = par.LCUSize;
    HRDConformance   = par.HRDConformance;
    RawRef           = par.RawRef;
    bROIViaMBQP      = par.bROIViaMBQP;
    bMBQPInput       = par.bMBQPInput;
    RAPIntra         = par.RAPIntra;
    bFieldReord      = par.bFieldReord;
    bNonStandardReord = par.bNonStandardReord;
    SetTL(par.m_ext.AVCTL);

}

MfxVideoParam& MfxVideoParam::operator=(MfxVideoParam const & par)
{

    Construct(par);
    CopyCalcParams(par);

    Copy(m_vps, par.m_vps);
    Copy(m_sps, par.m_sps);
    Copy(m_pps, par.m_pps);

    m_slice.resize(par.m_slice.size());

    for (size_t i = 0; i< m_slice.size(); i++)
        m_slice[i] = par.m_slice[i];

    return *this;
}

MfxVideoParam& MfxVideoParam::operator=(mfxVideoParam const & par)
{
    Construct(par);
    SyncVideoToCalculableParam();
    return *this;
}


void MfxVideoParam::Construct(mfxVideoParam const & par)
{
    mfxVideoParam & base = *this;

    base.mfx = par.mfx;
    base.AsyncDepth = par.AsyncDepth;
    base.IOPattern = par.IOPattern;
    base.Protected = par.Protected;
    base.NumExtParam = 0;
    base.ExtParam = m_ext.m_extParam;

    ExtBuffer::Construct(par, m_ext.HEVCParam, m_ext.m_extParam, base.NumExtParam);
    ExtBuffer::Construct(par, m_ext.HEVCTiles, m_ext.m_extParam, base.NumExtParam);
    ExtBuffer::Construct(par, m_ext.Opaque, m_ext.m_extParam, base.NumExtParam);
    ExtBuffer::Construct(par, m_ext.CO,  m_ext.m_extParam, base.NumExtParam);
    ExtBuffer::Construct(par, m_ext.CO2, m_ext.m_extParam, base.NumExtParam);
    ExtBuffer::Construct(par, m_ext.CO3, m_ext.m_extParam, base.NumExtParam);
    ExtBuffer::Construct(par, m_ext.DDI, m_ext.m_extParam, base.NumExtParam);
    ExtBuffer::Construct(par, m_ext.AVCTL, m_ext.m_extParam, base.NumExtParam);

    ExtBuffer::Construct(par, m_ext.ResetOpt, m_ext.m_extParam, base.NumExtParam);

    ExtBuffer::Construct(par, m_ext.VSI, m_ext.m_extParam, base.NumExtParam);
    ExtBuffer::Construct(par, m_ext.extBRC, m_ext.m_extParam, base.NumExtParam);
    ExtBuffer::Construct(par, m_ext.ROI, m_ext.m_extParam, base.NumExtParam);
    ExtBuffer::Construct(par, m_ext.DirtyRect, m_ext.m_extParam, base.NumExtParam);
}

mfxStatus MfxVideoParam::FillPar(mfxVideoParam& par, bool query)
{
        SyncCalculableToVideoParam();
        AlignCalcWithBRCParamMultiplier();

        par.AllocId    = AllocId;
        par.mfx        = mfx;
        par.AsyncDepth = AsyncDepth;
        par.IOPattern  = IOPattern;
        par.Protected  = Protected;

        return GetExtBuffers(par, query);
}

mfxStatus MfxVideoParam::GetExtBuffers(mfxVideoParam& par, bool query)
{
    mfxStatus sts = MFX_ERR_NONE;

    ExtBuffer::Set(par, m_ext.HEVCParam);
    ExtBuffer::Set(par, m_ext.HEVCTiles);
    ExtBuffer::Set(par, m_ext.Opaque);
    ExtBuffer::Set(par, m_ext.CO);
    ExtBuffer::Set(par, m_ext.CO2);
    ExtBuffer::Set(par, m_ext.CO3);
    ExtBuffer::Set(par, m_ext.ResetOpt);

    if (ExtBuffer::Set(par, m_ext.DDI))
    {
    }
    ExtBuffer::Set(par, m_ext.AVCTL);
    ExtBuffer::Set(par, m_ext.ROI);
    ExtBuffer::Set(par, m_ext.VSI);
    ExtBuffer::Set(par, m_ext.extBRC);
#ifdef MFX_ENABLE_HEVCE_DIRTY_RECT
    ExtBuffer::Set(par, m_ext.DirtyRect);
#endif
    mfxExtCodingOptionSPSPPS * pSPSPPS = ExtBuffer::Get(par);
    if (pSPSPPS && !query)
    {
        HeaderPacker packer;
        mfxU8* buf = 0;
        mfxU32 len = 0;

        sts = packer.Reset(*this);
        MFX_CHECK_STS(sts);

        if (pSPSPPS->SPSBuffer)
        {
            packer.GetSPS(buf, len);
            MFX_CHECK(pSPSPPS->SPSBufSize >= len, MFX_ERR_NOT_ENOUGH_BUFFER);
            memcpy_s(pSPSPPS->SPSBuffer, len, buf, len);
            pSPSPPS->SPSBufSize = (mfxU16)len;

            if (pSPSPPS->PPSBuffer)
            {
                packer.GetPPS(buf, len);
                MFX_CHECK(pSPSPPS->PPSBufSize >= len, MFX_ERR_NOT_ENOUGH_BUFFER);

                memcpy_s(pSPSPPS->PPSBuffer, len, buf, len);
                pSPSPPS->PPSBufSize = (mfxU16)len;
            }
        }
    }
    mfxExtCodingOptionVPS * pVPS= ExtBuffer::Get(par);
    if (pVPS && pVPS->VPSBuffer && !query)
    {

        HeaderPacker packer;
        mfxU8* buf = 0;
        mfxU32 len = 0;

        sts = packer.Reset(*this);
        MFX_CHECK_STS(sts);

        packer.GetVPS(buf, len);
        MFX_CHECK(pVPS->VPSBufSize >= len, MFX_ERR_NOT_ENOUGH_BUFFER);

        memcpy_s(pVPS->VPSBuffer, len, buf, len);
        pVPS->VPSBufSize = (mfxU16)len;
    }


    return sts;
}

bool MfxVideoParam::CheckExtBufferParam()
{
    mfxU32 bUnsupported = 0;

    bUnsupported += ExtBuffer::CheckBufferParams(m_ext.HEVCParam, true);
    bUnsupported += ExtBuffer::CheckBufferParams(m_ext.HEVCTiles, true);
    bUnsupported += ExtBuffer::CheckBufferParams(m_ext.Opaque, true);
    bUnsupported += ExtBuffer::CheckBufferParams(m_ext.CO, true);
    bUnsupported += ExtBuffer::CheckBufferParams(m_ext.CO2, true);
    bUnsupported += ExtBuffer::CheckBufferParams(m_ext.CO3, true);

    bUnsupported += ExtBuffer::CheckBufferParams(m_ext.ResetOpt, true);

    bUnsupported += ExtBuffer::CheckBufferParams(m_ext.DDI, true);
    bUnsupported += ExtBuffer::CheckBufferParams(m_ext.AVCTL, true);
    bUnsupported += ExtBuffer::CheckBufferParams(m_ext.VSI, true);
    bUnsupported += ExtBuffer::CheckBufferParams(m_ext.extBRC, true);
    return !!bUnsupported;
}

void MfxVideoParam::SyncVideoToCalculableParam()
{
    mfxU32 multiplier = Max<mfxU32>(mfx.BRCParamMultiplier, 1);

    BufferSizeInKB = mfx.BufferSizeInKB * multiplier;
    LTRInterval    = 0;
    PPyrInterval = (mfx.NumRefFrame >0) ? Min<mfxU32> (DEFAULT_PPYR_INTERVAL, mfx.NumRefFrame) : DEFAULT_PPYR_INTERVAL;

    if (mfx.RateControlMethod != MFX_RATECONTROL_CQP)
    {
        InitialDelayInKB = mfx.InitialDelayInKB * multiplier;
        TargetKbps       = mfx.TargetKbps       * multiplier;
        MaxKbps          = mfx.MaxKbps          * multiplier;
    } else
    {
        InitialDelayInKB = 0;
        TargetKbps       = 0;
        MaxKbps          = 0;
    }

    HRDConformance = !(IsOff(m_ext.CO.NalHrdConformance) || IsOff(m_ext.CO.VuiNalHrdParameters) );
    RawRef         = false;
    bROIViaMBQP    = false;
    bMBQPInput     = false;
    RAPIntra       = !isField();
    bFieldReord    = false; /*isField() && isBPyramid()*/;
    bNonStandardReord = false;

    m_slice.resize(0);

    SetTL(m_ext.AVCTL);
}

void MfxVideoParam::SyncCalculableToVideoParam()
{
    mfxU32 maxVal32 = BufferSizeInKB;

    if (mfx.RateControlMethod != MFX_RATECONTROL_CQP)
    {
        maxVal32 = Max(maxVal32, TargetKbps);

        if (mfx.RateControlMethod != MFX_RATECONTROL_AVBR)
            maxVal32 = Max(maxVal32, Max(MaxKbps, InitialDelayInKB));
    }

    mfx.BRCParamMultiplier = mfxU16((maxVal32 + 0x10000) / 0x10000);
    mfx.BufferSizeInKB     = (mfxU16)CeilDiv(BufferSizeInKB, mfx.BRCParamMultiplier);

    if (mfx.RateControlMethod == MFX_RATECONTROL_CBR ||
        mfx.RateControlMethod == MFX_RATECONTROL_VBR ||
        mfx.RateControlMethod == MFX_RATECONTROL_AVBR||
        mfx.RateControlMethod == MFX_RATECONTROL_VCM ||
        mfx.RateControlMethod == MFX_RATECONTROL_QVBR ||
        mfx.RateControlMethod == MFX_RATECONTROL_LA_EXT)
    {
        mfx.TargetKbps = (mfxU16)CeilDiv(TargetKbps, mfx.BRCParamMultiplier);

        if (mfx.RateControlMethod != MFX_RATECONTROL_AVBR)
        {
            mfx.InitialDelayInKB = (mfxU16)CeilDiv(InitialDelayInKB, mfx.BRCParamMultiplier);
            mfx.MaxKbps          = (mfxU16)CeilDiv(MaxKbps, mfx.BRCParamMultiplier);
        }
    }
    mfx.NumSlice = (mfxU16)m_slice.size();
}

void MfxVideoParam::AlignCalcWithBRCParamMultiplier()
{
    if (!mfx.BRCParamMultiplier)
        return;

    BufferSizeInKB = mfx.BufferSizeInKB   * mfx.BRCParamMultiplier;

    if (mfx.RateControlMethod != MFX_RATECONTROL_CQP)
    {
        InitialDelayInKB = mfx.InitialDelayInKB * mfx.BRCParamMultiplier;
        TargetKbps       = mfx.TargetKbps       * mfx.BRCParamMultiplier;
        MaxKbps          = mfx.MaxKbps          * mfx.BRCParamMultiplier;
    }
}

mfxU8 GetAspectRatioIdc(mfxU16 w, mfxU16 h)
{
    if (w ==   0 || h ==  0) return  0;
    if (w ==   1 && h ==  1) return  1;
    if (w ==  12 && h == 11) return  2;
    if (w ==  10 && h == 11) return  3;
    if (w ==  16 && h == 11) return  4;
    if (w ==  40 && h == 33) return  5;
    if (w ==  24 && h == 11) return  6;
    if (w ==  20 && h == 11) return  7;
    if (w ==  32 && h == 11) return  8;
    if (w ==  80 && h == 33) return  9;
    if (w ==  18 && h == 11) return 10;
    if (w ==  15 && h == 11) return 11;
    if (w ==  64 && h == 33) return 12;
    if (w == 160 && h == 99) return 13;
    if (w ==   4 && h ==  3) return 14;
    if (w ==   3 && h ==  2) return 15;
    if (w ==   2 && h ==  1) return 16;
    return 255;
}

const mfxU8 AspectRatioByIdc[][2] =
{
    {  0,  0},
    {  1,  1},
    { 12, 11},
    { 10, 11},
    { 16, 11},
    { 40, 33},
    { 24, 11},
    { 20, 11},
    { 32, 11},
    { 80, 33},
    { 18, 11},
    { 15, 11},
    { 64, 33},
    {160, 99},
    {  4,  3},
    {  3,  2},
    {  2,  1},
};

struct FakeTask
{
    mfxI32 m_poc;
    mfxU16 m_frameType;
    mfxU8  m_tid;
    mfxU32 m_bpo;
    mfxU32 m_level;
    bool  m_secondField;
};

struct STRPSFreq : STRPS
{
    STRPSFreq(STRPS const & rps, mfxU32 n)
    {
        *(STRPS*)this = rps;
        N = n;
    }
    mfxU32 N;
};

class EqSTRPS
{
private:
    STRPS const & m_ref;
public:
    EqSTRPS(STRPS const & ref) : m_ref(ref) {};
    bool operator () (STRPSFreq const & cur) { return Equal<STRPS>(m_ref, cur); };
    EqSTRPS & operator = (EqSTRPS const &) { assert(0); return *this; }
};

bool Greater(STRPSFreq const & l, STRPSFreq const r)
{
    return (l.N > r.N);
}

mfxU32 NBitsUE(mfxU32 b)
{
    mfxU32 n = 1;

     if (!b)
         return n;

    b ++;

    while (b >> n)
        n ++;

    return n * 2 - 1;
};

template<class T> mfxU32 NBits(T const & list, mfxU8 nSet, STRPS const & rps, mfxU8 idx)
{
    mfxU32 n = (idx != 0);
    mfxU32 nPic = mfxU32(rps.num_negative_pics + rps.num_positive_pics);

    if (rps.inter_ref_pic_set_prediction_flag)
    {
        assert(idx > rps.delta_idx_minus1);
        STRPS const & ref = list[idx - rps.delta_idx_minus1 - 1];
        nPic = mfxU32(ref.num_negative_pics + ref.num_positive_pics);

        if (idx == nSet)
            n += NBitsUE(rps.delta_idx_minus1);

        n += 1;
        n += NBitsUE(rps.abs_delta_rps_minus1);
        n += nPic;

        for (mfxU32 i = 0; i <= nPic; i ++)
            if (!rps.pic[i].used_by_curr_pic_flag)
                n ++;

        return n;
    }

    n += NBitsUE(rps.num_negative_pics);
    n += NBitsUE(rps.num_positive_pics);

    for (mfxU32 i = 0; i < nPic; i ++)
        n += NBitsUE(rps.pic[i].delta_poc_sx_minus1) + 1;

    return n;
}

template<class T> void OptimizeSTRPS(T const & list, mfxU8 n, STRPS& oldRPS, mfxU8 idx)
{
    if (idx == 0)
        return;

    STRPS newRPS;
    int k = 0, i = 0, j = 0;

    for (k = idx - 1; k >= 0; k --)
    {
        STRPS const & refRPS = list[k];

        if ((refRPS.num_negative_pics + refRPS.num_positive_pics + 1)
             < (oldRPS.num_negative_pics + oldRPS.num_positive_pics))
            continue;

        newRPS = oldRPS;
        newRPS.inter_ref_pic_set_prediction_flag = 1;
        newRPS.delta_idx_minus1 = (idx - k - 1);

        if (newRPS.delta_idx_minus1 > 0 && idx < n)
            break;

        std::list<mfxI16> dPocs[2];
        mfxI16 dPoc = 0;
        bool found = false;

        for (i = 0; i < oldRPS.num_negative_pics + oldRPS.num_positive_pics; i ++)
        {
            dPoc = oldRPS.pic[i].DeltaPocSX;
            if (dPoc)
                dPocs[dPoc > 0].push_back(dPoc);

            for (j = 0; j < refRPS.num_negative_pics + refRPS.num_positive_pics; j ++)
            {
                dPoc = oldRPS.pic[i].DeltaPocSX - refRPS.pic[j].DeltaPocSX;
                if (dPoc)
                    dPocs[dPoc > 0].push_back(dPoc);
            }
        }

        dPocs[0].sort(std::greater<mfxI16>());
        dPocs[1].sort(std::less<mfxI16>());
        dPocs[0].unique();
        dPocs[1].unique();

        dPoc = 0;

        while ((!dPocs[0].empty() || !dPocs[1].empty()) && !found)
        {
            dPoc *= -1;
            bool positive = (dPoc > 0 && !dPocs[1].empty()) || dPocs[0].empty();
            dPoc = dPocs[positive].front();
            dPocs[positive].pop_front();

            for (i = 0; i <= refRPS.num_negative_pics + refRPS.num_positive_pics; i ++)
                newRPS.pic[i].used_by_curr_pic_flag = newRPS.pic[i].use_delta_flag = 0;

            i = 0;
            for (j = refRPS.num_negative_pics + refRPS.num_positive_pics - 1;
                 j >= refRPS.num_negative_pics; j --)
            {
                if (   oldRPS.pic[i].DeltaPocSX < 0
                    && oldRPS.pic[i].DeltaPocSX - refRPS.pic[j].DeltaPocSX == dPoc)
                {
                    newRPS.pic[j].used_by_curr_pic_flag = oldRPS.pic[i].used_by_curr_pic_sx_flag;
                    newRPS.pic[j].use_delta_flag = 1;
                    i ++;
                }
            }

            if (dPoc < 0 && oldRPS.pic[i].DeltaPocSX == dPoc)
            {
                j = refRPS.num_negative_pics + refRPS.num_positive_pics;
                newRPS.pic[j].used_by_curr_pic_flag = oldRPS.pic[i].used_by_curr_pic_sx_flag;
                newRPS.pic[j].use_delta_flag = 1;
                i ++;
            }

            for (j = 0; j < refRPS.num_negative_pics; j ++)
            {
                if (   oldRPS.pic[i].DeltaPocSX < 0
                    && oldRPS.pic[i].DeltaPocSX - refRPS.pic[j].DeltaPocSX == dPoc)
                {
                    newRPS.pic[j].used_by_curr_pic_flag = oldRPS.pic[i].used_by_curr_pic_sx_flag;
                    newRPS.pic[j].use_delta_flag = 1;
                    i ++;
                }
            }

            if (i != oldRPS.num_negative_pics)
                continue;

            for (j = refRPS.num_negative_pics - 1; j >= 0; j --)
            {
                if (   oldRPS.pic[i].DeltaPocSX > 0
                    && oldRPS.pic[i].DeltaPocSX - refRPS.pic[j].DeltaPocSX == dPoc)
                {
                    newRPS.pic[j].used_by_curr_pic_flag = oldRPS.pic[i].used_by_curr_pic_sx_flag;
                    newRPS.pic[j].use_delta_flag = 1;
                    i ++;
                }
            }

            if (dPoc > 0 && oldRPS.pic[i].DeltaPocSX == dPoc)
            {
                j = refRPS.num_negative_pics + refRPS.num_positive_pics;
                newRPS.pic[j].used_by_curr_pic_flag = oldRPS.pic[i].used_by_curr_pic_sx_flag;
                newRPS.pic[j].use_delta_flag = 1;
                i ++;
            }

            for (j = refRPS.num_negative_pics;
                 j < refRPS.num_negative_pics + refRPS.num_positive_pics; j ++)
            {
                if (   oldRPS.pic[i].DeltaPocSX > 0
                    && oldRPS.pic[i].DeltaPocSX - refRPS.pic[j].DeltaPocSX == dPoc)
                {
                    newRPS.pic[j].used_by_curr_pic_flag = oldRPS.pic[i].used_by_curr_pic_sx_flag;
                    newRPS.pic[j].use_delta_flag = 1;
                    i ++;
                }
            }

            found = (i == oldRPS.num_negative_pics + oldRPS.num_positive_pics);
        }

        if (found)
        {
            newRPS.delta_rps_sign       = (dPoc < 0);
            newRPS.abs_delta_rps_minus1 = Abs(dPoc) - 1;

            if (NBits(list, n, newRPS, idx) < NBits(list, n, oldRPS, idx))
                oldRPS = newRPS;
        }

        if (idx < n)
            break;
    }
}

void ReduceSTRPS(std::vector<STRPSFreq> & sets, mfxU32 NumSlice)
{
    if (sets.empty())
        return;

    STRPS  rps = sets.back();
    mfxU32 n = sets.back().N; //current RPS used for N frames
    mfxU8  nSet = mfxU8(sets.size());
    mfxU32 bits0 = //bits for RPS in SPS and SSHs
        NBits(sets, nSet, rps, nSet - 1) //bits for RPS in SPS
        + (CeilLog2(nSet) - CeilLog2(nSet - 1)) //diff of bits for STRPS num in SPS
        + (nSet > 1) * (NumSlice * CeilLog2((mfxU32)sets.size()) * n); //bits for RPS idx in SSHs

    //emulate removal of current RPS from SPS
    nSet --;
    rps.inter_ref_pic_set_prediction_flag = 0;
    OptimizeSTRPS(sets, nSet, rps, nSet);

    mfxU32 bits1 = NBits(sets, nSet, rps, nSet) * NumSlice * n;//bits for RPS in SSHs (no RPS in SPS)

    if (bits1 < bits0)
    {
        sets.pop_back();
        ReduceSTRPS(sets, NumSlice);
    }
}

bool isCurrLt(
    DpbArray const & DPB,
    mfxU8 const (&RPL)[2][MAX_DPB_SIZE],
    mfxU8 const (&numRefActive)[2],
    mfxI32 poc)
{
    for (mfxU32 i = 0; i < 2; i ++)
        for (mfxU32 j = 0; j < numRefActive[i]; j ++)
            if (poc == DPB[RPL[i][j]].m_poc)
                return DPB[RPL[i][j]].m_ltr;
    return false;
}

inline bool isCurrLt(Task const & task, mfxI32 poc)
{
    return isCurrLt(task.m_dpb[0], task.m_refPicList, task.m_numRefActive, poc);
}


void MfxVideoParam::SyncHeadersToMfxParam()
{
    mfxFrameInfo& fi = mfx.FrameInfo;
    mfxU16 SubWidthC[4] = { 1, 2, 2, 1 };
    mfxU16 SubHeightC[4] = { 1, 2, 1, 1 };
    mfxU16 cropUnitX = SubWidthC[m_sps.chroma_format_idc];
    mfxU16 cropUnitY = SubHeightC[m_sps.chroma_format_idc];

    mfx.CodecProfile = m_sps.general.profile_idc;
    mfx.CodecLevel = m_sps.general.level_idc / 3;

    if (m_sps.general.tier_flag)
        mfx.CodecLevel |= MFX_TIER_HEVC_HIGH;


    mfx.NumRefFrame = m_sps.sub_layer[0].max_dec_pic_buffering_minus1;

        (mfx.FrameInfo.ChromaFormat = m_sps.chroma_format_idc);

    m_ext.HEVCParam.PicWidthInLumaSamples  = (mfxU16)m_sps.pic_width_in_luma_samples;
    m_ext.HEVCParam.PicHeightInLumaSamples = (mfxU16)m_sps.pic_height_in_luma_samples;

    fi.Width = Align(m_ext.HEVCParam.PicWidthInLumaSamples, HW_SURF_ALIGN_W);
    fi.Height = Align(m_ext.HEVCParam.PicHeightInLumaSamples, HW_SURF_ALIGN_H);

    fi.CropX = 0;
    fi.CropY = 0;
    fi.CropW = fi.Width;
    fi.CropH = fi.Height;

    if (m_sps.conformance_window_flag)
    {
        fi.CropX += cropUnitX * (mfxU16)m_sps.conf_win_left_offset;
        fi.CropW -= cropUnitX * (mfxU16)m_sps.conf_win_left_offset;
        fi.CropW -= cropUnitX * (mfxU16)m_sps.conf_win_right_offset;
        fi.CropY += cropUnitY * (mfxU16)m_sps.conf_win_top_offset;
        fi.CropH -= cropUnitY * (mfxU16)m_sps.conf_win_top_offset;
        fi.CropH -= cropUnitY * (mfxU16)m_sps.conf_win_bottom_offset;
    }

        fi.BitDepthLuma = m_sps.bit_depth_luma_minus8 + 8;

        fi.BitDepthChroma = m_sps.bit_depth_chroma_minus8 + 8;

    if (m_sps.vui_parameters_present_flag)
    {
        VUI& vui = m_sps.vui;

        if (vui.aspect_ratio_info_present_flag)
        {
            if (vui.aspect_ratio_idc == 255)
            {
                fi.AspectRatioW = vui.sar_width;
                fi.AspectRatioH = vui.sar_height;
            }
            else if (vui.aspect_ratio_idc < sizeof(AspectRatioByIdc) / sizeof(AspectRatioByIdc[0]))
            {
                fi.AspectRatioW = AspectRatioByIdc[vui.aspect_ratio_idc][0];
                fi.AspectRatioH = AspectRatioByIdc[vui.aspect_ratio_idc][1];
            }
        }

        if (vui.timing_info_present_flag)
        {
            fi.FrameRateExtN = vui.time_scale;
            fi.FrameRateExtD = vui.num_units_in_tick;
        }

        if (vui.default_display_window_flag)
        {
            fi.CropX += cropUnitX * (mfxU16)vui.def_disp_win_left_offset;
            fi.CropW -= cropUnitX * (mfxU16)vui.def_disp_win_left_offset;
            fi.CropW -= cropUnitX * (mfxU16)vui.def_disp_win_right_offset;
            fi.CropY += cropUnitY * (mfxU16)vui.def_disp_win_top_offset;
            fi.CropH -= cropUnitY * (mfxU16)vui.def_disp_win_top_offset;
            fi.CropH -= cropUnitY * (mfxU16)vui.def_disp_win_bottom_offset;
        }

        if (m_sps.vui.hrd_parameters_present_flag)
        {
            HRDInfo& hrd = m_sps.vui.hrd;
            HRDInfo::SubLayer& sl0 = hrd.sl[0];
            HRDInfo::SubLayer::CPB& cpb0 = sl0.cpb[0];

            MaxKbps = ((cpb0.bit_rate_value_minus1 + 1) << (6 + hrd.bit_rate_scale)) / 1000;
            BufferSizeInKB = ((cpb0.cpb_size_value_minus1 + 1) << (4 + hrd.cpb_size_scale)) / 8000;

            if (cpb0.cbr_flag)
                mfx.RateControlMethod = MFX_RATECONTROL_CBR;
            else
                mfx.RateControlMethod = MFX_RATECONTROL_VBR;
        }
    }

    m_ext.DDI.NumActiveRefP = m_ext.DDI.NumActiveRefBL0 = m_pps.num_ref_idx_l0_default_active_minus1 + 1;
    m_ext.DDI.NumActiveRefBL1 = m_pps.num_ref_idx_l1_default_active_minus1 + 1;

    if (m_pps.tiles_enabled_flag)
    {
        m_ext.HEVCTiles.NumTileColumns = m_pps.num_tile_columns_minus1 + 1;
        m_ext.HEVCTiles.NumTileRows = m_pps.num_tile_rows_minus1 + 1;
    }

}

mfxU8 GetNumReorderFrames(mfxU32 BFrameRate, bool BPyramid,bool bField, bool bFieldReord){
    mfxU8 n = !!BFrameRate;
    if(BPyramid && n--){
        while(BFrameRate){
            BFrameRate >>= 1;
            n ++;
        }
    }
    return bField ? n*2 + (bFieldReord ? 1:0) : n;
}

void MfxVideoParam::SyncMfxToHeadersParam(mfxU32 numSlicesForSTRPSOpt)
{
    PTL& general = m_vps.general;
    SubLayerOrdering& slo = m_vps.sub_layer[NumTL() - 1];

    Zero(m_vps);
    m_vps.video_parameter_set_id    = 0;
    m_vps.reserved_three_2bits      = 3;
    m_vps.max_layers_minus1         = 0;
    m_vps.max_sub_layers_minus1     = mfxU16(NumTL() - 1);
    m_vps.temporal_id_nesting_flag  = 1;
    m_vps.reserved_0xffff_16bits    = 0xFFFF;
    m_vps.sub_layer_ordering_info_present_flag = 0;

    m_vps.timing_info_present_flag          = 1;
    m_vps.num_units_in_tick                 = mfx.FrameInfo.FrameRateExtD;
    m_vps.time_scale                        = mfx.FrameInfo.FrameRateExtN;
    m_vps.poc_proportional_to_timing_flag   = 0;
    m_vps.num_hrd_parameters                = 0;

    general.profile_space               = 0;
    general.tier_flag                   = !!(mfx.CodecLevel & MFX_TIER_HEVC_HIGH);
    general.profile_idc                 = (mfxU8)mfx.CodecProfile;
    general.profile_compatibility_flags = 1 << (31 - general.profile_idc);
    general.progressive_source_flag     = !!(mfx.FrameInfo.PicStruct & MFX_PICSTRUCT_PROGRESSIVE);
    general.interlaced_source_flag      = !(mfx.FrameInfo.PicStruct & MFX_PICSTRUCT_PROGRESSIVE);
    general.non_packed_constraint_flag  = 0;
    general.frame_only_constraint_flag  = 0;
    general.level_idc                   = (mfxU8)(mfx.CodecLevel & 0xFF) * 3;


    slo.max_dec_pic_buffering_minus1    = mfx.NumRefFrame;
    slo.max_num_reorder_pics            = Min(GetNumReorderFrames(mfx.GopRefDist - 1, isBPyramid(),isField(), bFieldReord), slo.max_dec_pic_buffering_minus1);
    slo.max_latency_increase_plus1      = 0;

    Zero(m_sps);
    ((LayersInfo&)m_sps) = ((LayersInfo&)m_vps);
    m_sps.video_parameter_set_id   = m_vps.video_parameter_set_id;
    m_sps.max_sub_layers_minus1    = m_vps.max_sub_layers_minus1;
    m_sps.temporal_id_nesting_flag = m_vps.temporal_id_nesting_flag;

    m_sps.seq_parameter_set_id              = 0;
    //hardcoded untill support for encoding in chroma format other than YUV420 support added.
    m_sps.chroma_format_idc                 = 1;
    m_sps.separate_colour_plane_flag        = 0;
    m_sps.pic_width_in_luma_samples         = m_ext.HEVCParam.PicWidthInLumaSamples;
    m_sps.pic_height_in_luma_samples        = m_ext.HEVCParam.PicHeightInLumaSamples;
    m_sps.conformance_window_flag           = 0;
    m_sps.bit_depth_luma_minus8             = Max(0, (mfxI32)mfx.FrameInfo.BitDepthLuma - 8);
    m_sps.bit_depth_chroma_minus8           = Max(0, (mfxI32)mfx.FrameInfo.BitDepthChroma - 8);
    m_sps.log2_max_pic_order_cnt_lsb_minus4 = (mfxU8)Clip3(0, 12, (mfxI32)CeilLog2(mfx.GopRefDist*(isField() ? 2 : 1) + slo.max_dec_pic_buffering_minus1) - 1);

    m_sps.log2_min_luma_coding_block_size_minus3   = 0;
    m_sps.log2_diff_max_min_luma_coding_block_size = CeilLog2(LCUSize) - 3 - m_sps.log2_min_luma_coding_block_size_minus3;
    m_sps.log2_min_transform_block_size_minus2     = 0;
    m_sps.log2_diff_max_min_transform_block_size   = 3;
    m_sps.max_transform_hierarchy_depth_inter      = 2;
    m_sps.max_transform_hierarchy_depth_intra      = 2;
    m_sps.scaling_list_enabled_flag                = 0;
    { // SKL/KBL
        m_sps.amp_enabled_flag = 0;
        m_sps.sample_adaptive_offset_enabled_flag = 0;
    }
    m_sps.pcm_enabled_flag = 0;

    assert(0 == m_sps.pcm_enabled_flag);

    if (!bNonStandardReord)
    {
        mfxExtCodingOption3& CO3 = m_ext.CO3;
        mfxU32 MaxPocLsb = (1<<(m_sps.log2_max_pic_order_cnt_lsb_minus4+4));
        std::list<FakeTask> frames;
        std::list<FakeTask>::iterator cur;
        std::vector<STRPSFreq> sets;
        std::vector<STRPSFreq>::iterator it;
        DpbArray dpb = {};
        DpbFrame tmp = {};
        mfxU8 rpl[2][MAX_DPB_SIZE] = {};
        mfxU8 nRef[2] = {};
        STRPS rps;
        mfxI32 STDist = Min<mfxI32>(mfx.GopPicSize, 128);
        bool moreLTR = !!LTRInterval;
        mfxI32 lastIPoc = 0;

        Fill(dpb, IDX_INVALID);


        for (mfxU32 i = 0; (moreLTR || sets.size() != 64); i++)
        {
            FakeTask new_frame = { static_cast<int>(i), GetFrameType(*this, i), 0, (mfxU32)MFX_FRAMEORDER_UNKNOWN, 0, (isField() && i%2 == 1) };

            frames.push_back(new_frame);



            {
                cur = Reorder(*this, dpb, frames.begin(), frames.end(), false);


            }

            if (cur == frames.end())
                continue;

            if (isTL())
            {
                cur->m_tid = GetTId(isField() ? cur->m_poc/2 : cur->m_poc);

                if (HighestTId() == cur->m_tid && (!isField() || cur->m_secondField))
                    cur->m_frameType &= ~MFX_FRAMETYPE_REF;

                for (mfxI16 j = 0; !isDpbEnd(dpb, j); j++)
                    if (dpb[j].m_tid > 0 && dpb[j].m_tid >= cur->m_tid)
                        Remove(dpb, j--);
            }

            if (   (i > 0 && (cur->m_frameType & MFX_FRAMETYPE_I))
                || (!moreLTR && cur->m_poc >= STDist))
                break;

            if (!(cur->m_frameType & MFX_FRAMETYPE_I) && cur->m_poc < STDist)
            {
                if (cur->m_frameType & MFX_FRAMETYPE_B)
                {
                    mfxI32 layer = isBPyramid() ? Clip3<mfxI32>(0, 7, cur->m_level - 1) : 0;
                    nRef[0] = (mfxU8)CO3.NumRefActiveBL0[layer];
                    nRef[1] = (mfxU8)CO3.NumRefActiveBL1[layer];
                }
                else
                {
                    mfxI32 layer = PLayer(cur->m_poc - lastIPoc, *this);
                    nRef[0] = (mfxU8)CO3.NumRefActiveP[layer];
                    nRef[1] = (mfxU8)Min(CO3.NumRefActiveP[layer], m_ext.DDI.NumActiveRefBL1);
                }

                ConstructRPL(*this, dpb, !!(cur->m_frameType & MFX_FRAMETYPE_B), cur->m_poc, cur->m_tid, cur->m_level, cur->m_secondField, isBFF()? !cur->m_secondField : cur->m_secondField, rpl, nRef);

                Zero(rps);
                ConstructSTRPS(dpb, rpl, nRef, cur->m_poc, rps);

                it = std::find_if(sets.begin(), sets.end(), EqSTRPS(rps));

                if (it == sets.end())
                    sets.push_back(STRPSFreq(rps, 1));
                else
                    it->N ++;
            }
            else
                lastIPoc = cur->m_poc;

            if (cur->m_frameType & MFX_FRAMETYPE_REF)
            {
                if (moreLTR)
                {
                    for (mfxU16 j = 0; !isDpbEnd(dpb,j); j ++)
                    {
                        if (dpb[j].m_ltr)
                        {
                            mfxU32 dPocCycleMSB = (cur->m_poc / MaxPocLsb - dpb[j].m_poc / MaxPocLsb);
                            mfxU32 dPocLSB      = dpb[j].m_poc - (cur->m_poc - dPocCycleMSB * MaxPocLsb - Lsb(cur->m_poc, MaxPocLsb));
                            bool skip           = false;

                            if (mfxU32(m_sps.log2_max_pic_order_cnt_lsb_minus4+5) <= CeilLog2(m_sps.num_long_term_ref_pics_sps))
                            {
                                moreLTR = false;
                            }
                            else
                            {
                                for (mfxU32 k = 0; k < m_sps.num_long_term_ref_pics_sps; k ++)
                                {
                                    if (m_sps.lt_ref_pic_poc_lsb_sps[k] == dPocLSB)
                                    {
                                        moreLTR = !(cur->m_poc >= (mfxI32)MaxPocLsb);
                                        skip    = true;
                                        break;
                                    }
                                }
                            }

                            if (!moreLTR || skip)
                                break;

                            m_sps.lt_ref_pic_poc_lsb_sps[m_sps.num_long_term_ref_pics_sps] = (mfxU16)dPocLSB;
                            m_sps.used_by_curr_pic_lt_sps_flag[m_sps.num_long_term_ref_pics_sps] = isCurrLt(dpb, rpl, nRef, dpb[j].m_poc);
                            m_sps.num_long_term_ref_pics_sps ++;

                            if (m_sps.num_long_term_ref_pics_sps == 32)
                                moreLTR = false;
                        }
                    }
                }

                tmp.m_poc = cur->m_poc;
                tmp.m_tid = cur->m_tid;
                tmp.m_secondField = (isField()&&(tmp.m_poc & 1));
                tmp.m_bottomField = (isBFF()!= tmp.m_secondField);
                UpdateDPB(*this, tmp, dpb);
            }

            frames.erase(cur);
        }

        std::sort(sets.begin(), sets.end(), Greater);
        assert(sets.size() <= 64);

        for (mfxU8 i = 0; i < sets.size(); i ++)
            OptimizeSTRPS(sets, (mfxU8)sets.size(), sets[i], i);


        ReduceSTRPS(sets, numSlicesForSTRPSOpt ? numSlicesForSTRPSOpt : mfx.NumSlice);

        for (it = sets.begin(); it != sets.end(); it ++)
            m_sps.strps[m_sps.num_short_term_ref_pic_sets++] = *it;

        m_sps.long_term_ref_pics_present_flag = 1; // !!LTRInterval;
    }

    m_sps.temporal_mvp_enabled_flag             = 1; // SKL ?
    m_sps.strong_intra_smoothing_enabled_flag   = 0; // SKL

    m_sps.vui_parameters_present_flag = 1;

    m_sps.vui.aspect_ratio_info_present_flag = 1;
    if (m_sps.vui.aspect_ratio_info_present_flag)
    {
        m_sps.vui.aspect_ratio_idc = GetAspectRatioIdc(mfx.FrameInfo.AspectRatioW, mfx.FrameInfo.AspectRatioH);
        if (m_sps.vui.aspect_ratio_idc == 255)
        {
            m_sps.vui.sar_width  = mfx.FrameInfo.AspectRatioW;
            m_sps.vui.sar_height = mfx.FrameInfo.AspectRatioH;
        }
    }

    {
        mfxFrameInfo& fi = mfx.FrameInfo;
        mfxU16 SubWidthC[4] = {1,2,2,1};
        mfxU16 SubHeightC[4] = {1,2,1,1};
        mfxU16 cropUnitX = SubWidthC[m_sps.chroma_format_idc];
        mfxU16 cropUnitY = SubHeightC[m_sps.chroma_format_idc];

        m_sps.conf_win_left_offset      = (fi.CropX / cropUnitX);
        m_sps.conf_win_right_offset     = (m_sps.pic_width_in_luma_samples - fi.CropW - fi.CropX) / cropUnitX;
        m_sps.conf_win_top_offset       = (fi.CropY / cropUnitY);
        m_sps.conf_win_bottom_offset    = (m_sps.pic_height_in_luma_samples - fi.CropH - fi.CropY) / cropUnitY;
        m_sps.conformance_window_flag   =    m_sps.conf_win_left_offset
                                          || m_sps.conf_win_right_offset
                                          || m_sps.conf_win_top_offset
                                          || m_sps.conf_win_bottom_offset;
    }
    {
        m_sps.vui.video_format                    = mfxU8(m_ext.VSI.VideoFormat);
        m_sps.vui.video_full_range_flag           = m_ext.VSI.VideoFullRange;
        m_sps.vui.colour_description_present_flag = m_ext.VSI.ColourDescriptionPresent;
        m_sps.vui.colour_primaries                = mfxU8(m_ext.VSI.ColourPrimaries);
        m_sps.vui.transfer_characteristics        = mfxU8(m_ext.VSI.TransferCharacteristics);
        m_sps.vui.matrix_coeffs                   = mfxU8(m_ext.VSI.MatrixCoefficients);
        m_sps.vui.video_signal_type_present_flag  =
                    m_ext.VSI.VideoFormat                    != 5 ||
                    m_ext.VSI.VideoFullRange                 != 0 ||
                    m_ext.VSI.ColourDescriptionPresent       != 0;
    }

    m_sps.vui.timing_info_present_flag = !!m_vps.timing_info_present_flag;
    if (m_sps.vui.timing_info_present_flag)
    {
        m_sps.vui.num_units_in_tick = m_vps.num_units_in_tick;
        m_sps.vui.time_scale        = m_vps.time_scale;
    }

    m_sps.vui.field_seq_flag = isField();
    if (IsOn(m_ext.CO.PicTimingSEI) || m_sps.vui.field_seq_flag
        || (m_vps.general.progressive_source_flag && m_vps.general.interlaced_source_flag))
    {
        m_sps.vui.frame_field_info_present_flag = 1; // spec requirement for interlace
    }

    if (IsOn(m_ext.CO.VuiNalHrdParameters))
    {
        HRDInfo& hrd = m_sps.vui.hrd;
        HRDInfo::SubLayer& sl0 = hrd.sl[0];
        HRDInfo::SubLayer::CPB& cpb0 = sl0.cpb[0];
        mfxU32 hrdBitrate = MaxKbps * 1000;
        mfxU32 cpbSize = BufferSizeInKB * 8000;

        m_sps.vui.hrd_parameters_present_flag = 1;

        hrd.nal_hrd_parameters_present_flag = 1;
        hrd.sub_pic_hrd_params_present_flag = 0;

        hrd.bit_rate_scale = 0;
        hrd.cpb_size_scale = 2;

        while (hrd.bit_rate_scale < 16 && (hrdBitrate & ((1 << (6 + hrd.bit_rate_scale + 1)) - 1)) == 0)
            hrd.bit_rate_scale++;
        while (hrd.cpb_size_scale < 16 && (cpbSize & ((1 << (4 + hrd.cpb_size_scale + 1)) - 1)) == 0)
            hrd.cpb_size_scale++;

        hrd.initial_cpb_removal_delay_length_minus1 = 23;
        hrd.au_cpb_removal_delay_length_minus1      = 23;
        hrd.dpb_output_delay_length_minus1          = 23;

        sl0.fixed_pic_rate_general_flag = 1;
        sl0.low_delay_hrd_flag          = 0;
        sl0.cpb_cnt_minus1              = 0;

        cpb0.bit_rate_value_minus1 = (hrdBitrate >> (6 + hrd.bit_rate_scale)) - 1;
        cpb0.cpb_size_value_minus1 = (cpbSize >> (4 + hrd.cpb_size_scale)) - 1;
        cpb0.cbr_flag              = (mfx.RateControlMethod == MFX_RATECONTROL_CBR);
    }



    Zero(m_pps);
    m_pps.seq_parameter_set_id = m_sps.seq_parameter_set_id;

    m_pps.pic_parameter_set_id                  = 0;
    m_pps.dependent_slice_segments_enabled_flag = 0;
    m_pps.output_flag_present_flag              = 0;
    m_pps.num_extra_slice_header_bits           = 0;
    m_pps.sign_data_hiding_enabled_flag         = 0;
    m_pps.cabac_init_present_flag               = 0;
    m_pps.num_ref_idx_l0_default_active_minus1  = Max(m_ext.DDI.NumActiveRefP, m_ext.DDI.NumActiveRefBL0) -1;
    m_pps.num_ref_idx_l1_default_active_minus1  = m_ext.DDI.NumActiveRefBL1 -1;
    m_pps.init_qp_minus26                       = 0;
    m_pps.constrained_intra_pred_flag           = 0;

        m_pps.transform_skip_enabled_flag = 0;

    m_pps.cu_qp_delta_enabled_flag = IsOn(m_ext.CO3.EnableMBQP);

    if (m_ext.CO2.MaxSliceSize)
        m_pps.cu_qp_delta_enabled_flag = 1;
#ifdef MFX_ENABLE_HEVCE_ROI
    if (m_ext.ROI.NumROI)
        m_pps.cu_qp_delta_enabled_flag = 1;
#endif


    m_pps.cb_qp_offset                          = 0;
    m_pps.cr_qp_offset                          = 0;
    m_pps.slice_chroma_qp_offsets_present_flag  = 0;

    if (isSWBRC())
    {
        m_pps.cb_qp_offset                          =  -1;
        m_pps.cr_qp_offset                          = - 1;
        m_pps.slice_chroma_qp_offsets_present_flag  =  1;
    }
    if (mfx.RateControlMethod == MFX_RATECONTROL_CQP)
    {
        m_pps.init_qp_minus26 = (mfx.GopPicSize == 1 ? mfx.QPI : (mfx.GopRefDist == 1 ? mfx.QPP : mfx.QPB)) - 26;
        m_pps.init_qp_minus26 -= 6 * m_sps.bit_depth_luma_minus8;
        // m_pps.cb_qp_offset = -1;
        // m_pps.cr_qp_offset = -1;
    }

    m_pps.slice_chroma_qp_offsets_present_flag  = 0;
#if defined(MFX_ENABLE_HEVCE_WEIGHTED_PREDICTION)
    m_pps.weighted_pred_flag = (m_ext.CO3.WeightedPred == MFX_WEIGHTED_PRED_EXPLICIT);
    m_pps.weighted_bipred_flag =
        (m_ext.CO3.WeightedBiPred == MFX_WEIGHTED_PRED_EXPLICIT) || (IsOn(m_ext.CO3.GPB) && m_pps.weighted_pred_flag);
#else
    m_pps.weighted_pred_flag                    = 0;
    m_pps.weighted_bipred_flag                  = 0;
#endif //defined(MFX_ENABLE_HEVCE_WEIGHTED_PREDICTION)
    m_pps.transquant_bypass_enabled_flag        = 0;
    m_pps.tiles_enabled_flag                    = 0;
    m_pps.entropy_coding_sync_enabled_flag      = 0;

    if (m_ext.HEVCTiles.NumTileColumns * m_ext.HEVCTiles.NumTileRows > 1)
    {
        mfxU16 nCol   = (mfxU16)CeilDiv(m_ext.HEVCParam.PicWidthInLumaSamples,  LCUSize);
        mfxU16 nRow   = (mfxU16)CeilDiv(m_ext.HEVCParam.PicHeightInLumaSamples, LCUSize);
        mfxU16 nTCol  = Max<mfxU16>(m_ext.HEVCTiles.NumTileColumns, 1);
        mfxU16 nTRow  = Max<mfxU16>(m_ext.HEVCTiles.NumTileRows, 1);

        m_pps.tiles_enabled_flag        = 1;
        m_pps.uniform_spacing_flag      = 1;
        m_pps.num_tile_columns_minus1   = nTCol - 1;
        m_pps.num_tile_rows_minus1      = nTRow - 1;

        for (mfxU16 i = 0; i < nTCol - 1; i ++)
            m_pps.column_width[i] = ((i + 1) * nCol) / nTCol - (i * nCol) / nTCol;

        for (mfxU16 j = 0; j < nTRow - 1; j ++)
            m_pps.row_height[j] = ((j + 1) * nRow) / nTRow - (j * nRow) / nTRow;

        m_pps.loop_filter_across_tiles_enabled_flag = 1;
    }

        m_pps.loop_filter_across_slices_enabled_flag = 0;

    m_pps.deblocking_filter_control_present_flag  = 1;
    m_pps.deblocking_filter_disabled_flag = !!m_ext.CO2.DisableDeblockingIdc;
    m_pps.deblocking_filter_override_enabled_flag = 1; // to disable deblocking per frame

    m_pps.scaling_list_data_present_flag              = 0;
    m_pps.lists_modification_present_flag             = 1;
    m_pps.log2_parallel_merge_level_minus2            = 0;
    m_pps.slice_segment_header_extension_present_flag = 0;


}

mfxU16 FrameType2SliceType(mfxU32 ft)
{
    if (ft & MFX_FRAMETYPE_B)
        return 0;
    if (ft & MFX_FRAMETYPE_P)
        return 1;
    return 2;
}

bool isCurrRef(
    DpbArray const & DPB,
    mfxU8 const (&RPL)[2][MAX_DPB_SIZE],
    mfxU8 const (&numRefActive)[2],
    mfxI32 poc)
{
    for (mfxU32 i = 0; i < 2; i ++)
        for (mfxU32 j = 0; j < numRefActive[i]; j ++)
            if (poc == DPB[RPL[i][j]].m_poc)
                return true;
    return false;
}

inline bool isCurrRef(Task const & task, mfxI32 poc)
{
    return isCurrRef(task.m_dpb[0], task.m_refPicList, task.m_numRefActive, poc);
}

void ConstructSTRPS(
    DpbArray const & DPB,
    mfxU8 const (&RPL)[2][MAX_DPB_SIZE],
    mfxU8 const (&numRefActive)[2],
    mfxI32 poc,
    STRPS& rps)
{
    mfxU32 i, nRef;

    for (i = 0, nRef = 0; !isDpbEnd(DPB, i); i ++)
    {
        if (DPB[i].m_ltr)
            continue;

        rps.pic[nRef].DeltaPocSX = (mfxI16)(DPB[i].m_poc - poc);
        rps.pic[nRef].used_by_curr_pic_sx_flag = isCurrRef(DPB, RPL, numRefActive, DPB[i].m_poc);

        rps.num_negative_pics += rps.pic[nRef].DeltaPocSX < 0;
        rps.num_positive_pics += rps.pic[nRef].DeltaPocSX > 0;
        nRef ++;
    }

    MFX_SORT_STRUCT(rps.pic, nRef, DeltaPocSX, >);
    MFX_SORT_STRUCT(rps.pic, rps.num_negative_pics, DeltaPocSX, <);

    for (i = 0; i < nRef; i ++)
    {
        mfxI16 prev  = (!i || i == rps.num_negative_pics) ? 0 : rps.pic[i-1].DeltaPocSX;
        rps.pic[i].delta_poc_sx_minus1 = Abs(rps.pic[i].DeltaPocSX - prev) - 1;
    }
}

bool Equal(STRPS const & l, STRPS const & r)
{
    //ignore inter_ref_pic_set_prediction_flag, check only DeltaPocSX

    if (   l.num_negative_pics != r.num_negative_pics
        || l.num_positive_pics != r.num_positive_pics)
        return false;

    for(mfxU8 i = 0; i < l.num_negative_pics + l.num_positive_pics; i ++)
        if (   l.pic[i].DeltaPocSX != r.pic[i].DeltaPocSX
            || l.pic[i].used_by_curr_pic_sx_flag != r.pic[i].used_by_curr_pic_sx_flag)
            return false;

    return true;
}

bool isForcedDeltaPocMsbPresent(
    Task const & prevTask,
    mfxI32 poc,
    mfxU32 MaxPocLsb)
{
    DpbArray const & DPB = prevTask.m_dpb[0];

    if (Lsb(prevTask.m_poc, MaxPocLsb) == Lsb(poc, MaxPocLsb))
        return true;

    for (mfxU16 i = 0; !isDpbEnd(DPB, i); i ++)
        if (DPB[i].m_poc != poc && Lsb(DPB[i].m_poc, MaxPocLsb) == Lsb(poc, MaxPocLsb))
            return true;

    return false;
}

mfxStatus MfxVideoParam::GetSliceHeader(Task const & task, Task const & prevTask, ENCODE_CAPS_HEVC const & caps, Slice & s) const
{
    bool  isP   = !!(task.m_frameType & MFX_FRAMETYPE_P);
    bool  isB   = !!(task.m_frameType & MFX_FRAMETYPE_B);

    mfxExtCodingOption2 *ext2 = ExtBuffer::Get(task.m_ctrl);
    Zero(s);

    s.first_slice_segment_in_pic_flag = 1;
    s.no_output_of_prior_pics_flag    = 0;
    s.pic_parameter_set_id            = m_pps.pic_parameter_set_id;

    if (!s.first_slice_segment_in_pic_flag)
    {
        if (m_pps.dependent_slice_segments_enabled_flag)
        {
            s.dependent_slice_segment_flag = 0;
        }

        s.segment_address = 0;
    }

    s.type = FrameType2SliceType(task.m_frameType);

    if (m_pps.output_flag_present_flag)
        s.pic_output_flag = 1;

    assert(0 == m_sps.separate_colour_plane_flag);

    if (task.m_shNUT != IDR_W_RADL && task.m_shNUT != IDR_N_LP)
    {
        mfxU32 i, j;
        mfxU16 nLTR = 0, nSTR[2] = {}, nDPBST = 0, nDPBLT = 0;
        mfxI32 STR[2][MAX_DPB_SIZE] = {};                           // used short-term references
        mfxI32 LTR[MAX_NUM_LONG_TERM_PICS] = {};                    // used long-term references
        DpbArray const & DPB = task.m_dpb[0];                       // DPB before encoding
        mfxU8 const (&RPL)[2][MAX_DPB_SIZE] = task.m_refPicList;    // Ref. Pic. List

        ConstructSTRPS(DPB, RPL, task.m_numRefActive, task.m_poc, s.strps);

        s.pic_order_cnt_lsb = (task.m_poc & ~(0xFFFFFFFF << (m_sps.log2_max_pic_order_cnt_lsb_minus4 + 4)));

        // count STRs and LTRs in DPB (including unused)
        for (i = 0; !isDpbEnd(DPB, i); i ++)
        {
            nDPBST += !DPB[i].m_ltr;
            nDPBLT += DPB[i].m_ltr;
        }

        //check for suitable ST RPS in SPS
        for (i = 0; i < m_sps.num_short_term_ref_pic_sets; i ++)
        {
            if (Equal(m_sps.strps[i], s.strps))
            {
                //use ST RPS from SPS
                s.short_term_ref_pic_set_sps_flag = 1;
                s.short_term_ref_pic_set_idx      = (mfxU8)i;
                break;
            }
        }

        if (!s.short_term_ref_pic_set_sps_flag)
            OptimizeSTRPS(m_sps.strps, m_sps.num_short_term_ref_pic_sets, s.strps, m_sps.num_short_term_ref_pic_sets);

        for (i = 0; i < mfxU32(s.strps.num_negative_pics + s.strps.num_positive_pics); i ++)
        {
            bool isAfter = (s.strps.pic[i].DeltaPocSX > 0);
            if (s.strps.pic[i].used_by_curr_pic_sx_flag)
                STR[isAfter][nSTR[isAfter]++] = task.m_poc + s.strps.pic[i].DeltaPocSX;
        }

        if (nDPBLT)
        {
            assert(m_sps.long_term_ref_pics_present_flag);

            mfxU32 MaxPocLsb  = (1<<(m_sps.log2_max_pic_order_cnt_lsb_minus4+4));
            mfxU32 dPocCycleMSBprev = 0;
            mfxI32 DPBLT[MAX_DPB_SIZE] = {};
            const mfxI32 InvalidPOC = -9000;

            for (i = 0, nDPBLT = 0; !isDpbEnd(DPB, i); i ++)
                if (DPB[i].m_ltr)
                    DPBLT[nDPBLT++] = DPB[i].m_poc;

            MFX_SORT(DPBLT, nDPBLT, <); // sort for DeltaPocMsbCycleLt (may only increase)

            for (nLTR = 0, j = 0; j < nDPBLT; j ++)
            {
                // insert LTR using lt_ref_pic_poc_lsb_sps
               for (i = 0; i < m_sps.num_long_term_ref_pics_sps; i ++)
               {
                   mfxU32 dPocCycleMSB = (task.m_poc / MaxPocLsb - DPBLT[j] / MaxPocLsb);
                   mfxU32 dPocLSB      = DPBLT[j] - (task.m_poc - dPocCycleMSB * MaxPocLsb - s.pic_order_cnt_lsb);

                   if (   dPocLSB == m_sps.lt_ref_pic_poc_lsb_sps[i]
                       && isCurrLt(task, DPBLT[j]) == !!m_sps.used_by_curr_pic_lt_sps_flag[i]
                       && dPocCycleMSB >= dPocCycleMSBprev)
                   {
                       Slice::LongTerm & curlt = s.lt[s.num_long_term_sps];

                       curlt.lt_idx_sps                    = i;
                       curlt.used_by_curr_pic_lt_flag      = !!m_sps.used_by_curr_pic_lt_sps_flag[i];
                       curlt.poc_lsb_lt                    = m_sps.lt_ref_pic_poc_lsb_sps[i];
                       curlt.delta_poc_msb_cycle_lt        = dPocCycleMSB - dPocCycleMSBprev;
                       curlt.delta_poc_msb_present_flag    = !!curlt.delta_poc_msb_cycle_lt || isForcedDeltaPocMsbPresent(prevTask, DPBLT[j], MaxPocLsb);
                       dPocCycleMSBprev                    = dPocCycleMSB;

                       s.num_long_term_sps ++;

                       if (curlt.used_by_curr_pic_lt_flag)
                       {
                          MFX_CHECK(nLTR < MAX_NUM_LONG_TERM_PICS, MFX_ERR_UNDEFINED_BEHAVIOR);
                          LTR[nLTR++] = DPBLT[j];
                       }

                       DPBLT[j] = InvalidPOC;
                       break;
                   }
               }
            }

            for (j = 0, dPocCycleMSBprev = 0; j < nDPBLT; j ++)
            {
                Slice::LongTerm & curlt = s.lt[s.num_long_term_sps + s.num_long_term_pics];

                if (DPBLT[j] == InvalidPOC)
                    continue; //already inserted using lt_ref_pic_poc_lsb_sps

                mfxU32 dPocCycleMSB = (task.m_poc / MaxPocLsb - DPBLT[j] / MaxPocLsb);
                mfxU32 dPocLSB      = DPBLT[j] - (task.m_poc - dPocCycleMSB * MaxPocLsb - s.pic_order_cnt_lsb);

                assert(dPocCycleMSB >= dPocCycleMSBprev);

                curlt.used_by_curr_pic_lt_flag      = isCurrLt(task, DPBLT[j]);
                curlt.poc_lsb_lt                    = dPocLSB;
                curlt.delta_poc_msb_cycle_lt        = dPocCycleMSB - dPocCycleMSBprev;
                curlt.delta_poc_msb_present_flag    = !!curlt.delta_poc_msb_cycle_lt || isForcedDeltaPocMsbPresent(prevTask, DPBLT[j], MaxPocLsb);
                dPocCycleMSBprev                    = dPocCycleMSB;

                s.num_long_term_pics ++;

                if (curlt.used_by_curr_pic_lt_flag)
                {
                    if (nLTR < MAX_NUM_LONG_TERM_PICS) //KW
                        LTR[nLTR++] = DPBLT[j];
                    else
                        assert(!"too much LTRs");
                }
            }
        }

        if (m_sps.temporal_mvp_enabled_flag)
            s.temporal_mvp_enabled_flag = 1;

        if (m_pps.lists_modification_present_flag)
        {
            mfxU16 rIdx = 0;
            mfxI32 RPLTempX[2][16] = {}; // default ref. list without modifications
            mfxU16 NumRpsCurrTempListX[2] =
            {
                Max<mfxU16>(nSTR[0] + nSTR[1] + nLTR, task.m_numRefActive[0]),
                Max<mfxU16>(nSTR[0] + nSTR[1] + nLTR, task.m_numRefActive[1])
            };

            for (j = 0; j < 2; j ++)
            {
                rIdx = 0;
                while (rIdx < NumRpsCurrTempListX[j])
                {
                    for (i = 0; i < nSTR[j] && rIdx < NumRpsCurrTempListX[j]; i ++)
                        RPLTempX[j][rIdx++] = STR[j][i];
                    for (i = 0; i < nSTR[!j] && rIdx < NumRpsCurrTempListX[j]; i ++)
                        RPLTempX[j][rIdx++] = STR[!j][i];
                    for (i = 0; i < nLTR && rIdx < NumRpsCurrTempListX[j]; i ++)
                        RPLTempX[j][rIdx++] = LTR[i];
                }

                for (rIdx = 0; rIdx < task.m_numRefActive[j]; rIdx ++)
                {
                    for (i = 0; i < NumRpsCurrTempListX[j] && DPB[RPL[j][rIdx]].m_poc != RPLTempX[j][i]; i ++);
                    s.list_entry_lx[j][rIdx] = (mfxU8)i;
                    s.ref_pic_list_modification_flag_lx[j] |= (i != rIdx);
                }
            }
        }
    }

    if (m_sps.sample_adaptive_offset_enabled_flag)
    {
        s.sao_luma_flag   = 0;
        s.sao_chroma_flag = 0;
    }

    if (isP || isB)
    {
        s.num_ref_idx_active_override_flag =
           (          m_pps.num_ref_idx_l0_default_active_minus1 + 1 != task.m_numRefActive[0]
            || (isB && m_pps.num_ref_idx_l1_default_active_minus1 + 1 != task.m_numRefActive[1]));

        s.num_ref_idx_l0_active_minus1 = task.m_numRefActive[0] - 1;

        if (isB)
            s.num_ref_idx_l1_active_minus1 = task.m_numRefActive[1] - 1;

        if (isB)
            s.mvd_l1_zero_flag = 0;

        if (m_pps.cabac_init_present_flag)
            s.cabac_init_flag = 0;

        if (s.temporal_mvp_enabled_flag)
            s.collocated_from_l0_flag = 1;

#if defined(MFX_ENABLE_HEVCE_WEIGHTED_PREDICTION)
        if (   (m_pps.weighted_pred_flag && isP)
            || (m_pps.weighted_bipred_flag && isB))
        {
            const mfxU16 Y = 0, Cb = 1, Cr = 2, W = 0, O = 1;
            mfxExtPredWeightTable* pwt = ExtBuffer::Get(task.m_ctrl);

            if (pwt)
            {
                s.luma_log2_weight_denom = pwt->LumaLog2WeightDenom;
                s.chroma_log2_weight_denom = pwt->ChromaLog2WeightDenom;
            }
            else
            {
                s.luma_log2_weight_denom = 6;
                s.chroma_log2_weight_denom = s.luma_log2_weight_denom;
            }

            for (mfxU16 l = 0; l < 2; l++)
            {
                for (mfxU16 i = 0; i < 16; i++)
                {
                    s.pwt[l][i][Y][W] = (1 << s.luma_log2_weight_denom);
                    s.pwt[l][i][Y][O] = 0;
                    s.pwt[l][i][Cb][W] = (1 << s.chroma_log2_weight_denom);
                    s.pwt[l][i][Cb][O] = 0;
                    s.pwt[l][i][Cr][W] = (1 << s.chroma_log2_weight_denom);
                    s.pwt[l][i][Cr][O] = 0;
                }
            }

            if (pwt)
            {
                mfxU16 sz[2] =
                {
                    Min<mfxU16>(s.num_ref_idx_l0_active_minus1 + 1, caps.MaxNum_WeightedPredL0),
                    Min<mfxU16>(s.num_ref_idx_l1_active_minus1 + 1, caps.MaxNum_WeightedPredL1)
                };

                for (mfxU16 l = 0; l < 2; l++)
                {
                    for (mfxU16 i = 0; i < sz[l]; i++)
                    {
                        if (pwt->LumaWeightFlag[l][i] && caps.LumaWeightedPred)
                        {
                            s.pwt[l][i][Y][W] = pwt->Weights[l][i][Y][W];
                            s.pwt[l][i][Y][O] = pwt->Weights[l][i][Y][O];
                        }

                        if (pwt->ChromaWeightFlag[l][i] && caps.ChromaWeightedPred)
                        {
                            s.pwt[l][i][Cb][W] = pwt->Weights[l][i][Cb][W];
                            s.pwt[l][i][Cb][O] = pwt->Weights[l][i][Cb][O];
                            s.pwt[l][i][Cr][W] = pwt->Weights[l][i][Cr][W];
                            s.pwt[l][i][Cr][O] = pwt->Weights[l][i][Cr][O];
                        }
                    }

                    if (task.m_ldb && Equal(task.m_refPicList[0], task.m_refPicList[1]))
                        Copy(s.pwt[1], s.pwt[0]);
                }
            }
        }
#else
        (void)caps;
        assert(0 == m_pps.weighted_pred_flag);
        assert(0 == m_pps.weighted_bipred_flag);
#endif //defined(MFX_ENABLE_HEVCE_WEIGHTED_PREDICTION)

        s.five_minus_max_num_merge_cand = 0;
    }

    if (mfx.RateControlMethod == MFX_RATECONTROL_CQP || mfx.RateControlMethod == MFX_RATECONTROL_LA_EXT)
        s.slice_qp_delta = mfxI8(task.m_qpY - (m_pps.init_qp_minus26 + 26));

    if (m_pps.slice_chroma_qp_offsets_present_flag)
    {
        s.slice_cb_qp_offset = 0;
        s.slice_cr_qp_offset = 0;
    }

     s.deblocking_filter_disabled_flag = m_pps.deblocking_filter_disabled_flag;
     s.beta_offset_div2     = m_pps.beta_offset_div2;
     s.tc_offset_div2       = m_pps.tc_offset_div2;
     s.deblocking_filter_override_flag = 0;

     if ( ext2 && ext2->DisableDeblockingIdc != m_ext.CO2.DisableDeblockingIdc && m_pps.deblocking_filter_override_enabled_flag)
     {
        s.deblocking_filter_disabled_flag = !!ext2->DisableDeblockingIdc;
        s.deblocking_filter_override_flag = (s.deblocking_filter_disabled_flag != m_pps.deblocking_filter_disabled_flag);

        if (s.deblocking_filter_override_flag)
        {
            s.beta_offset_div2 = 0;
            s.tc_offset_div2 = 0;
        }
     }

    s.loop_filter_across_slices_enabled_flag = m_pps.loop_filter_across_slices_enabled_flag;

    if (m_pps.tiles_enabled_flag || m_pps.entropy_coding_sync_enabled_flag)
    {
        s.num_entry_point_offsets = 0;

        assert(0 == s.num_entry_point_offsets);
    }
    return MFX_ERR_NONE;
}

void HRD::Init(const SPS &sps, mfxU32 InitialDelayInKB)
{
    VUI const & vui = sps.vui;
    HRDInfo const & hrd = sps.vui.hrd;
    HRDInfo::SubLayer::CPB const & cpb0 = hrd.sl[0].cpb[0];

    if (   !sps.vui_parameters_present_flag
        || !sps.vui.hrd_parameters_present_flag
        || !(hrd.nal_hrd_parameters_present_flag || hrd.vcl_hrd_parameters_present_flag))
    {
        m_bIsHrdRequired = false;
        return;
    }

    m_bIsHrdRequired = true;

    mfxU32 cpbSize        = (cpb0.cpb_size_value_minus1 + 1) << (4 + hrd.cpb_size_scale);
    m_cbrFlag             = !!cpb0.cbr_flag;
    m_bitrate             = (cpb0.bit_rate_value_minus1 + 1) << (6 + hrd.bit_rate_scale);
    m_maxCpbRemovalDelay  = 1 << (hrd.au_cpb_removal_delay_length_minus1 + 1);

    m_cpbSize90k          = mfxU32(90000. * cpbSize / m_bitrate);
    m_initCpbRemovalDelay = mfxU32(90000. * 8000. * InitialDelayInKB / m_bitrate);
    m_clockTick           = (mfxF64)vui.num_units_in_tick * 90000 / vui.time_scale;

    m_prevAuCpbRemovalDelayMinus1 = -1;
    m_prevAuCpbRemovalDelayMsb    = 0;
    m_prevAuFinalArrivalTime      = 0;
    m_prevBpAuNominalRemovalTime  = m_initCpbRemovalDelay;
    m_prevBpEncOrder              = 0;
}

void HRD::Reset(SPS const & sps)
{
    HRDInfo const & hrd = sps.vui.hrd;
    HRDInfo::SubLayer::CPB const & cpb0 = hrd.sl[0].cpb[0];

    if (m_bIsHrdRequired == false)
        return;

    mfxU32 cpbSize  = (cpb0.cpb_size_value_minus1 + 1) << (4 + hrd.cpb_size_scale);
    m_bitrate       = (cpb0.bit_rate_value_minus1 + 1) << (6 + hrd.bit_rate_scale);
    m_cpbSize90k    = mfxU32(90000. * cpbSize / m_bitrate);
}

void HRD::Update(mfxU32 sizeInbits, const Task &pic)
{
    bool bufferingPeriodPic = !!(pic.m_insertHeaders & INSERT_BPSEI);
    mfxF64 auNominalRemovalTime;

    if (m_bIsHrdRequired == false)
        return;

    if (pic.m_eo > 0)
    {
        mfxU32 auCpbRemovalDelayMinus1 = (pic.m_eo - m_prevBpEncOrder) - 1;
        // (D-1)
        mfxU32 auCpbRemovalDelayMsb = 0;

        if (!bufferingPeriodPic && (pic.m_eo - m_prevBpEncOrder) != 1)
        {
            auCpbRemovalDelayMsb = ((mfxI32)auCpbRemovalDelayMinus1 <= m_prevAuCpbRemovalDelayMinus1)
                ? m_prevAuCpbRemovalDelayMsb + m_maxCpbRemovalDelay
                : m_prevAuCpbRemovalDelayMsb;
        }

        m_prevAuCpbRemovalDelayMsb      = auCpbRemovalDelayMsb;
        m_prevAuCpbRemovalDelayMinus1   = auCpbRemovalDelayMinus1;

        // (D-2)
        mfxU32 auCpbRemovalDelayValMinus1 = auCpbRemovalDelayMsb + auCpbRemovalDelayMinus1;
        // (C-10, C-11)
        auNominalRemovalTime = m_prevBpAuNominalRemovalTime + m_clockTick * (auCpbRemovalDelayValMinus1 + 1);
    }
    else // (C-9)
        auNominalRemovalTime = m_initCpbRemovalDelay;

    // (C-3)
    mfxF64 initArrivalTime = m_prevAuFinalArrivalTime;

    if (!m_cbrFlag)
    {
        mfxF64 initArrivalEarliestTime = (bufferingPeriodPic)
            // (C-7)
            ? auNominalRemovalTime - m_initCpbRemovalDelay
            // (C-6)
            : auNominalRemovalTime - m_cpbSize90k;
        // (C-4)
        initArrivalTime = Max(m_prevAuFinalArrivalTime, initArrivalEarliestTime * m_bitrate);
    }
    // (C-8)
    mfxF64 auFinalArrivalTime = initArrivalTime + (mfxF64)sizeInbits * 90000;

    m_prevAuFinalArrivalTime = auFinalArrivalTime;

    if (bufferingPeriodPic)
    {
        m_prevBpAuNominalRemovalTime = auNominalRemovalTime;
        m_prevBpEncOrder = pic.m_eo;
    }

    /*fprintf (stderr, "FO: %d\ninitArrivalTime = %f\nauFinalArrivalTime = %f\nauNominalRemovalTime = %f\n",
        pic.m_fo, initArrivalTime / 90000 / m_bitrate, auFinalArrivalTime / 90000 / m_bitrate, auNominalRemovalTime / 90000);
    fflush(stderr);*/
}

mfxU32 HRD::GetInitCpbRemovalDelay(const Task &pic)
{
    mfxF64 auNominalRemovalTime;

    if (m_bIsHrdRequired == false)
        return 0;

    if (pic.m_eo > 0)
    {
        // (D-1)
        mfxU32 auCpbRemovalDelayMsb = 0;
        mfxU32 auCpbRemovalDelayMinus1 = pic.m_eo - m_prevBpEncOrder - 1;

        // (D-2)
        mfxU32 auCpbRemovalDelayValMinus1 = auCpbRemovalDelayMsb + auCpbRemovalDelayMinus1;
        // (C-10, C-11)
        auNominalRemovalTime = m_prevBpAuNominalRemovalTime + m_clockTick * (auCpbRemovalDelayValMinus1 + 1);

        // (C-17)
        mfxF64 deltaTime90k = auNominalRemovalTime - m_prevAuFinalArrivalTime / m_bitrate;

        m_initCpbRemovalDelay = m_cbrFlag
            // (C-19)
            ? (mfxU32)(deltaTime90k)
            // (C-18)
            : (mfxU32)Min(deltaTime90k, m_cpbSize90k);
    }

    return (mfxU32)m_initCpbRemovalDelay;
}

TaskManager::TaskManager()
    :m_bFieldMode(false)
    ,m_resetHeaders(0)
{
}

void TaskManager::Reset(bool bFieldMode, mfxU32 numTask, mfxU16 resetHeaders)
{
    if (numTask)
    {
        m_free.resize(numTask);
        m_reordering.resize(0);
        m_encoding.resize(0);
        m_querying.resize(0);
    }
    else
    {
        while (!m_querying.empty())
            vm_time_sleep(1);
    }
    m_resetHeaders = resetHeaders;
    m_bFieldMode = bFieldMode;
}

Task* TaskManager::New()
{
    UMC::AutomaticUMCMutex guard(m_listMutex);
    Task* pTask = 0;

    if (!m_free.empty())
    {
        pTask = &m_free.front();
        m_reordering.splice(m_reordering.end(), m_free, m_free.begin());
        Zero(*pTask);
        pTask->m_stage = FRAME_NEW;
    }

    return pTask;
}
Task* TaskManager::GetNewTask()
{
   UMC::AutomaticUMCMutex guard(m_listMutex);

    for (TaskList::iterator it = m_reordering.begin(); it != m_reordering.end(); it ++)
    {
        if (it->m_stage == FRAME_NEW)
        {
            return  &*it;
        }
    }
    return 0;
}
Task* TaskManager::Reorder(MfxVideoParam const & par, DpbArray const & dpb, bool flush)
{
    UMC::AutomaticUMCMutex guard(m_listMutex);

    TaskList::iterator begin = m_reordering.begin();
    TaskList::iterator end   = m_reordering.begin();

    while (end != m_reordering.end() && end->m_stage == FRAME_ACCEPTED)
    {
        if ((end != begin) && (end->m_frameType & MFX_FRAMETYPE_IDR))
        {
            flush = true;
            break;
        }
        end++;
    }
    TaskList::iterator top = MfxHwH265Encode::Reorder(par, dpb, begin, end, flush);
    if (top == end)
    {
        if (end != m_reordering.end() && end->m_stage == FRAME_REORDERED)
            return &*end; //formal task without surface
        else
            return 0;
    }

    if (m_resetHeaders)
    {
        top->m_insertHeaders |= m_resetHeaders;
        m_resetHeaders = 0;
    }
    return &*top;
}

void TaskManager::Submit(Task* pTask)
{
    UMC::AutomaticUMCMutex guard(m_listMutex);

    for (TaskList::iterator it = m_reordering.begin(); it != m_reordering.end(); it ++)
    {
        if (pTask == &*it)
        {
            m_encoding.splice(m_encoding.end(), m_reordering, it);
            pTask->m_stage |= FRAME_REORDERED;
            break;
        }
    }
}
Task* TaskManager::GetTaskForSubmit(bool bRealTask)
{
    UMC::AutomaticUMCMutex guard(m_listMutex);
    for (TaskList::iterator it = m_encoding.begin(); it != m_encoding.end(); it ++)
    {
        if (it->m_surf || (!bRealTask))
        {
            return &*it;
        }
    }
    return 0;
}
mfxStatus TaskManager::PutTasksForRecode(Task* pTask)
{
    UMC::AutomaticUMCMutex guard(m_listMutex);

    TaskList::iterator it_from = m_querying.begin();
    TaskList::iterator it_where =m_encoding.begin();

    for (;it_from != m_querying.end() && pTask != &*it_from; it_from ++) {}
    MFX_CHECK(it_from != m_querying.end(), MFX_ERR_UNDEFINED_BEHAVIOR);

    for (;it_where != m_encoding.end() && (it_where->m_stage & FRAME_SUBMITTED)!=0; it_where ++);

    m_encoding.splice(it_where, m_querying, it_from);
    return MFX_ERR_NONE;
}
void TaskManager::SubmitForQuery(Task* pTask)
{
    UMC::AutomaticUMCMutex guard(m_listMutex);

    for (TaskList::iterator it = m_encoding.begin(); it != m_encoding.end(); it ++)
    {
        if (pTask == &*it)
        {
            m_querying.splice(m_querying.end(), m_encoding, it);
            pTask->m_stage |= FRAME_SUBMITTED;
            break;
        }
    }
}
bool TaskManager::isSubmittedForQuery(Task* pTask)
{
    UMC::AutomaticUMCMutex guard(m_listMutex);

    for (TaskList::iterator it = m_querying.begin(); it != m_querying.end(); it ++)
    {
        if (pTask == &*it)
        {
            return true;
        }
    }
    return false;
}
Task* TaskManager::GetTaskForQuery()
{
    UMC::AutomaticUMCMutex guard(m_listMutex);
    if (m_querying.size() > 0)
    {
        return &(*m_querying.begin());
    }
    return 0;
}

void TaskManager::Ready(Task* pTask)
{
    UMC::AutomaticUMCMutex guard(m_listMutex);

    for (TaskList::iterator it = m_querying.begin(); it != m_querying.end(); it ++)
    {
        if (pTask == &*it)
        {
            m_free.splice(m_free.end(), m_querying, it);
            pTask->m_stage = 0;
            break;
        }
    }
}
void TaskManager::SkipTask(Task* pTask)
{
    UMC::AutomaticUMCMutex guard(m_listMutex);

    for (TaskList::iterator it = m_reordering.begin(); it != m_reordering.end(); it ++)
    {
        if (pTask == &*it)
        {
            m_free.splice(m_free.end(), m_reordering, it);
            pTask->m_stage = 0;
            break;
        }
    }
}


mfxU8 GetFrameType(
    MfxVideoParam const & video,
    mfxU32                frameOrder)
{
    mfxU32 gopOptFlag = video.mfx.GopOptFlag;
    mfxU32 gopPicSize = video.mfx.GopPicSize;
    mfxU32 gopRefDist = video.mfx.GopRefDist;
    mfxU32 idrPicDist = gopPicSize * (video.mfx.IdrInterval);

    if (gopPicSize == 0xffff) //infinite GOP
        idrPicDist = gopPicSize = 0xffffffff;

    bool bField = video.isField();
    mfxU32 fo = bField ? frameOrder / 2 : frameOrder;
    bool   bSecondField = bField ? (frameOrder & 1) != 0 : false;
    bool   bIdr = (idrPicDist ? fo % idrPicDist : fo) == 0;

    if (bIdr)
    {
        return bSecondField ? (MFX_FRAMETYPE_P | MFX_FRAMETYPE_REF) : (MFX_FRAMETYPE_I | MFX_FRAMETYPE_REF | MFX_FRAMETYPE_IDR);
    }

    if (fo % gopPicSize == 0)
        return (mfxU8)(bSecondField ? MFX_FRAMETYPE_P : MFX_FRAMETYPE_I) | MFX_FRAMETYPE_REF;

    if (fo % gopPicSize % gopRefDist == 0)
        return (MFX_FRAMETYPE_P | MFX_FRAMETYPE_REF);

    //if ((gopOptFlag & MFX_GOP_STRICT) == 0)
        if (((fo + 1) % gopPicSize == 0 && (gopOptFlag & MFX_GOP_CLOSED)) ||
            (idrPicDist && (fo + 1) % idrPicDist == 0))
            return (MFX_FRAMETYPE_P | MFX_FRAMETYPE_REF); // switch last B frame to P frame


    return (MFX_FRAMETYPE_B);
}

mfxU8 GetDPBIdxByFO(DpbArray const & DPB, mfxU32 fo)
{
    for (mfxU8 i = 0; !isDpbEnd(DPB, i); i++)
    if (DPB[i].m_fo == fo)
        return i;
    return mfxU8(MAX_DPB_SIZE);
}

mfxU8 GetDPBIdxByPoc(DpbArray const & DPB, mfxI32 poc)
{
    for (mfxU8 i = 0; !isDpbEnd(DPB, i); i++)
    if (DPB[i].m_poc == poc)
        return i;
    return mfxU8(MAX_DPB_SIZE);
}

void InitDPB(
    Task &        task,
    Task const &  prevTask,
    mfxExtAVCRefListCtrl * pLCtrl)
{
    if (   task.m_poc > task.m_lastRAP
        && prevTask.m_poc <= prevTask.m_lastRAP) // 1st TRAIL
    {
        Fill(task.m_dpb[TASK_DPB_ACTIVE], IDX_INVALID);

        // TODO: add mode to disable this check
        for (mfxU8 i = 0, j = 0; !isDpbEnd(prevTask.m_dpb[TASK_DPB_AFTER], i); i++)
        {
            const DpbFrame& ref = prevTask.m_dpb[TASK_DPB_AFTER][i];

            if (ref.m_poc == task.m_lastRAP || ref.m_ltr)
                task.m_dpb[TASK_DPB_ACTIVE][j++] = ref;
        }
    }
    else
    {
        Copy(task.m_dpb[TASK_DPB_ACTIVE], prevTask.m_dpb[TASK_DPB_AFTER]);
    }

    Copy(task.m_dpb[TASK_DPB_BEFORE], prevTask.m_dpb[TASK_DPB_AFTER]);

    {
        DpbArray& dpb = task.m_dpb[TASK_DPB_ACTIVE];

        for (mfxI16 i = 0; !isDpbEnd(dpb, i); i++)
            if (dpb[i].m_tid > 0 && dpb[i].m_tid >= task.m_tid)
                Remove(dpb, i--);

        if (pLCtrl)
        {
            for (mfxU16 i = 0; i < 16 && pLCtrl->RejectedRefList[i].FrameOrder != static_cast<mfxU32>(MFX_FRAMEORDER_UNKNOWN); i++)
            {
                mfxU16 idx = GetDPBIdxByFO(dpb, pLCtrl->RejectedRefList[i].FrameOrder);

                if (idx < MAX_DPB_SIZE && dpb[idx].m_ltr)
                    Remove(dpb, idx);
            }
        }
    }
}

void UpdateDPB(
    MfxVideoParam const & par,
    DpbFrame const & task,
    DpbArray & dpb,
    mfxExtAVCRefListCtrl * pLCtrl)
{
    mfxU16 end = 0; // DPB end
    mfxU16 st0 = 0; // first ST ref in DPB

    while (!isDpbEnd(dpb, end)) end ++;
    for (st0 = 0; st0 < end && dpb[st0].m_ltr; st0++);

    // frames stored in DPB in POC ascending order,
    // LTRs before STRs (use LTR-candidate as STR as long as it possible)

    MFX_SORT_STRUCT(dpb, st0, m_poc, > );
    MFX_SORT_STRUCT((dpb + st0), mfxU16(end - st0), m_poc, >);

    // sliding window over STRs
    if (end && end == par.mfx.NumRefFrame)
    {
        if (par.isLowDelay() && st0 == 0)  //P pyramid if no LTR. Pyramid is possible if NumRefFrame > 1
        {
            if (!par.isField() || GetFrameNum(true, dpb[1].m_poc, dpb[1].m_secondField) == GetFrameNum(true, dpb[0].m_poc, dpb[0].m_secondField))
                for (st0 = 1; ((GetFrameNum(par.isField(), dpb[st0].m_poc, dpb[st0].m_secondField) - (GetFrameNum(par.isField(), dpb[0].m_poc, dpb[0].m_secondField))) % par.PPyrInterval) == 0 && st0 < end; st0++);
        }
        else
        {
            for (st0 = 0; dpb[st0].m_ltr && st0 < end; st0 ++); // excess?

            if (par.LTRInterval)
            {
                //To do: Fix LTR for field mode (two LTR should be stored). Now LTR aren't supported.
                // mark/replace LTR in DPB
                if (!st0)
                {
                    dpb[st0].m_ltr = true;
                    st0 ++;
                }
                else if ((GetFrameNum(par.isField(), dpb[st0].m_poc, dpb[st0].m_secondField) - (GetFrameNum(par.isField(), dpb[0].m_poc, dpb[0].m_secondField))) >= (mfxI32)par.LTRInterval)
                {
                    dpb[st0].m_ltr = true;
                    st0 = 0;
                }
            }
        }

        Remove(dpb, st0 == end ? 0 : st0);
        end --;

    }

    if (end < MAX_DPB_SIZE) //just for KW
        dpb[end++] = task;
    else
        assert(!"DPB overflow, no space for new frame");

    if (pLCtrl)
    {
        bool sort = false;

        for (st0 = 0; dpb[st0].m_ltr && st0 < end; st0++);

        for (mfxU16 i = 0; i < 16 && pLCtrl->LongTermRefList[i].FrameOrder != static_cast<mfxU32>(MFX_FRAMEORDER_UNKNOWN); i++)
        {
            mfxU16 idx = GetDPBIdxByFO(dpb, pLCtrl->LongTermRefList[i].FrameOrder);

            if (idx < MAX_DPB_SIZE && !dpb[idx].m_ltr)
            {
                DpbFrame ltr = dpb[idx];
                ltr.m_ltr = true;
                Remove(dpb, idx);
                Insert(dpb, st0, ltr);
                st0++;
                sort = true;
            }
        }

        if (sort)
        {
            MFX_SORT_STRUCT(dpb, st0, m_poc, >);
        }
    }

    if (par.isBPyramid() && (task.m_ldb || task.m_codingType < CODING_TYPE_B) )
        for (mfxU16 i = 0; i < end - (par.isField()? 2 : 1); i ++)
            dpb[i].m_codingType = 0; //don't keep coding types for prev. mini-GOP

}


bool isLTR(
    DpbArray const & dpb,
    mfxU32 LTRInterval,
    mfxI32 poc)
{
    mfxI32 LTRCandidate = dpb[0].m_poc;

    for (mfxU16 i = 1; !isDpbEnd(dpb, i); i ++)
    {
        if (   dpb[i].m_poc > LTRCandidate
            && dpb[i].m_poc - LTRCandidate >= (mfxI32)LTRInterval)
        {
            LTRCandidate = dpb[i].m_poc;
            break;
        }
    }

    return (poc == LTRCandidate) || (LTRCandidate == 0 && poc >= (mfxI32)LTRInterval);
}

// 0 - the nearest  filds are used as reference in RPL
// 1 - the first reference is the same polarity field (polarity is first or second)
// 2 - the same polarity fields are used for reference (if possible) (polarity is first or second)
// 3 - filds from nearest frames if only one field, it should be the same polarity (polarity is top or bottom)
// 4 - the same of 3, but optimization for B pyramid (pyramid references has high priority)
#define HEVCE_FIELD_MODE 3

mfxU32 WeightForBPyrForw(
    MfxVideoParam const & par,
    DpbArray const & DPB,
    mfxI32 cur_poc,
    mfxU32 cur_level,
    bool   cur_bSecondField,
    DpbFrame refFrame)
{
    if (!par.isBPyramid() || refFrame.m_poc > cur_poc || refFrame.m_level)
        return 0;

    if (refFrame.m_level < cur_level) return 16;

    if ( cur_level == refFrame.m_level)
        return   (cur_bSecondField && GetFrameNum(par.isField(), cur_poc, cur_bSecondField) == GetFrameNum(par.isField(), refFrame.m_poc, refFrame.m_secondField)) ? 0 : 16;

    for (int i = 0; i < MAX_DPB_SIZE; i++)
    {
        if (DPB[i].m_poc >= 0 && 
            DPB[i].m_level == refFrame.m_level &&
            DPB[i].m_poc < cur_poc && 
            GetFrameNum(par.isField(), refFrame.m_poc, refFrame.m_secondField) < GetFrameNum(par.isField(), DPB[i].m_poc, DPB[i].m_secondField))
            return 16;
    }
    return 0;
 }

mfxU32 FieldDistance(mfxI32 poc, bool  bSecondField, DpbFrame refFrame)
{
    return Abs(GetFrameNum(true, refFrame.m_poc, refFrame.m_secondField) - GetFrameNum(true, poc, bSecondField)) * 2;
}
mfxU32 FieldDistancePolarity(mfxI32 poc, bool  bSecondField, bool  bBottomField, DpbFrame refFrame)
{
    return FieldDistance(poc, bSecondField, refFrame) + ((refFrame.m_bottomField == bBottomField) ? 0 : 1);
}
void ConstructRPL(
    MfxVideoParam const & par,
    DpbArray const & DPB,
    bool isB,
    mfxI32 poc,
    mfxU8  tid,
    mfxU32 /*level*/,
    bool  bSecondField,
    bool  bBottomField,
    mfxU8 (&RPL)[2][MAX_DPB_SIZE],
    mfxU8 (&numRefActive)[2],
    mfxExtAVCRefLists * pExtLists,
    mfxExtAVCRefListCtrl * pLCtrl)
{
    mfxU8 NumRefLX[2] = {numRefActive[0], numRefActive[1]};
    mfxU8& l0 = numRefActive[0];
    mfxU8& l1 = numRefActive[1];
    mfxU8 LTR[MAX_DPB_SIZE] = {};
    mfxU8 nLTR = 0;
    mfxU8 NumStRefL0 = (mfxU8)(NumRefLX[0]);

    l0 = l1 = 0;

    if (pExtLists)
    {
        for (mfxU32 i = 0; i < pExtLists->NumRefIdxL0Active; i ++)
        {
            mfxU8 idx = GetDPBIdxByFO(DPB, (mfxI32)pExtLists->RefPicList0[i].FrameOrder);

            if (idx < MAX_DPB_SIZE)
            {
                RPL[0][l0 ++] = idx;

                if (l0 == NumRefLX[0])
                    break;
            }
        }

        for (mfxU32 i = 0; i < pExtLists->NumRefIdxL1Active; i ++)
        {
            mfxU8 idx = GetDPBIdxByFO(DPB, (mfxI32)pExtLists->RefPicList1[i].FrameOrder);

            if (idx < MAX_DPB_SIZE)
                RPL[1][l1 ++] = idx;

            if (l1 == NumRefLX[1])
                break;
        }
    }

    if (l0 == 0)
    {
        l1 = 0;

        for (mfxU8 i = 0; !isDpbEnd(DPB, i); i ++)
        {
            if (DPB[i].m_tid > tid)
                continue;

            if (poc > DPB[i].m_poc)
            {
                if (DPB[i].m_ltr || (par.LTRInterval && isLTR(DPB, par.LTRInterval, DPB[i].m_poc)))
                    LTR[nLTR++] = i;
                else
                    RPL[0][l0++] = i;
            }
            else if (isB)
                RPL[1][l1++] = i;
        }

        if (pLCtrl)
        {
            // reorder STRs to POC descending order
            for (mfxU8 lx = 0; lx < 2; lx++)
                MFX_SORT_COMMON(RPL[lx], numRefActive[lx],
                    Abs(DPB[RPL[lx][_i]].m_poc - poc) > Abs(DPB[RPL[lx][_j]].m_poc-poc));

            while (nLTR)
                RPL[0][l0++] = LTR[--nLTR];
        }
        else
        {
            NumStRefL0 -= !!nLTR;

            if (l0 > NumStRefL0)
            {
                if (par.isField())
                {
#if (HEVCE_FIELD_MODE == 0)
                    bSecondField; bBottomField; level
                    MFX_SORT_COMMON(RPL[0], numRefActive[0], Abs(DPB[RPL[0][_i]].m_poc - poc) < Abs(DPB[RPL[0][_j]].m_poc - poc));
#elif (HEVCE_FIELD_MODE == 1)
                    bBottomField; level
                    MFX_SORT_COMMON(RPL[0], numRefActive[0], (Abs(DPB[RPL[0][_i]].m_poc / 2 - poc / 2) * 2 + ((DPB[RPL[0][_i]].m_secondField == bSecondField) ? 0 : 1)) < (Abs(DPB[RPL[0][_j]].m_poc / 2 - poc / 2) * 2 + ((DPB[RPL[0][_j]].m_secondField == bSecondField) ? 0 : 1)));
#elif (HEVCE_FIELD_MODE == 2)
                    bBottomField; level
                    MFX_SORT_COMMON(RPL[0], numRefActive[0], (Abs(DPB[RPL[0][_i]].m_poc / 2 - poc / 2) + ((DPB[RPL[0][_i]].m_secondField == bSecondField) ? 0 : 16)) < (Abs(DPB[RPL[0][_j]].m_poc / 2 - poc / 2) + ((DPB[RPL[0][_j]].m_secondField == bSecondField) ? 0 : 16)));
#elif (HEVCE_FIELD_MODE == 3)
                    MFX_SORT_COMMON(RPL[0], numRefActive[0], FieldDistancePolarity(poc, bSecondField, bBottomField, DPB[RPL[0][_i]]) < FieldDistancePolarity(poc, bSecondField, bBottomField, DPB[RPL[0][_j]]));
#elif (HEVCE_FIELD_MODE == 4)
                    MFX_SORT_COMMON(RPL[0], numRefActive[0], FieldDistancePolarity(poc, bSecondField, bBottomField, DPB[RPL[0][_i]] + WeightForBPyrForw(par, DPB, poc, cur_level, bSecondField, DPB[RPL[0][_i])) < (FieldDistancePolarity(poc, bSecondField, bBottomField, DPB[RPL[0][_j]] + WeightForBPyrForw(par, DPB, poc, cur_level, bSecondField, DPB[RPL[0][_j]))));                  
#endif
                }
                else
                {
                    MFX_SORT_COMMON(RPL[0], numRefActive[0], Abs(DPB[RPL[0][_i]].m_poc - poc) < Abs(DPB[RPL[0][_j]].m_poc - poc));
                }
                if (par.isLowDelay())
                {
                    // P pyramid
                    while (l0 > NumStRefL0)
                    {
                        mfxI32 i;
                        // !!! par.NumRefLX[0] used here as distance between "strong" STR, not NumRefActive for current frame
                        for (i = 0; (i < l0) && (((GetFrameNum(par.isField(), DPB[RPL[0][i]].m_poc, DPB[RPL[0][i]].m_secondField) - GetFrameNum(par.isField(), DPB[RPL[0][0]].m_poc, DPB[RPL[0][0]].m_secondField)) % par.PPyrInterval) == 0); i++);
                        Remove(RPL[0], (i >= (par.isField() ? l0 - 2 : l0 - 1) ? 0 : i));
                        l0--;
                    }
                }

                else
                {
                    Remove(RPL[0], (par.LTRInterval && !nLTR && l0 > 1), l0 - NumStRefL0);
                    l0 = NumStRefL0;
                }
            }
            if (l1 > NumRefLX[1])
            {
                if (par.isField())
                {
#if (HEVCE_FIELD_MODE == 0)
                        MFX_SORT_COMMON(RPL[1], numRefActive[1], Abs(DPB[RPL[1][_i]].m_poc - poc) > Abs(DPB[RPL[1][_j]].m_poc - poc));
#elif (HEVCE_FIELD_MODE == 1)
                        MFX_SORT_COMMON(RPL[1], numRefActive[1], (Abs(DPB[RPL[1][_i]].m_poc/2 - poc/2)*2  + ((DPB[RPL[1][_i]].m_secondField == bSecondField) ? 0 : 1)) > (Abs(DPB[RPL[1][_j]].m_poc/2 - poc/2)*2  + ((DPB[RPL[1][_j]].m_secondField == bSecondField) ? 0 : 1)));
#elif (HEVCE_FIELD_MODE == 2)
                        MFX_SORT_COMMON(RPL[1], numRefActive[1], (Abs(DPB[RPL[1][_i]].m_poc/2 - poc/2)  + ((DPB[RPL[1][_i]].m_secondField == bSecondField) ? 0 : 16)) > (Abs(DPB[RPL[1][_j]].m_poc/2 - poc/2)  + ((DPB[RPL[1][_j]].m_secondField == bSecondField) ? 0 : 16)));
#elif (HEVCE_FIELD_MODE == 3 || HEVCE_FIELD_MODE == 4)
                       MFX_SORT_COMMON(RPL[1], numRefActive[1], FieldDistancePolarity(poc, bSecondField, bBottomField, DPB[RPL[1][_i]]) > FieldDistancePolarity(poc, bSecondField, bBottomField, DPB[RPL[1][_j]]));
#endif
                }
                else
                {
                    MFX_SORT_COMMON(RPL[1], numRefActive[1], Abs(DPB[RPL[1][_i]].m_poc - poc) > Abs(DPB[RPL[1][_j]].m_poc - poc));
                }
                Remove(RPL[1], NumRefLX[1], l1 - NumRefLX[1]);
                l1 = (mfxU8)NumRefLX[1];

            }

            // reorder STRs to POC descending order
            for (mfxU8 lx = 0; lx < 2; lx++)
                MFX_SORT_COMMON(RPL[lx], numRefActive[lx],
                    DPB[RPL[lx][_i]].m_poc < DPB[RPL[lx][_j]].m_poc);

            if (nLTR)
            {
                MFX_SORT(LTR, nLTR, < );
                // use LTR as 2nd reference
                Insert(RPL[0], !!l0, LTR[0]);
                l0++;

                for (mfxU16 i = 1; i < nLTR && l0 < NumRefLX[0]; i++, l0++)
                    Insert(RPL[0], l0, LTR[i]);
            }
        }
    }

    assert(l0 > 0);

    if (pLCtrl)
    {
        mfxU16 MaxRef[2] = { NumRefLX[0], NumRefLX[1] };
        mfxU16 pref[2] = {};

        if (pLCtrl->NumRefIdxL0Active)
            MaxRef[0] = Min(pLCtrl->NumRefIdxL0Active, MaxRef[0]);

        if (pLCtrl->NumRefIdxL1Active)
            MaxRef[1] = Min(pLCtrl->NumRefIdxL1Active, MaxRef[1]);

        for (mfxU16 i = 0; i < 16 && pLCtrl->RejectedRefList[i].FrameOrder != static_cast<mfxU32>(MFX_FRAMEORDER_UNKNOWN); i++)
        {
            mfxU8 idx = GetDPBIdxByFO(DPB, pLCtrl->RejectedRefList[i].FrameOrder);

            if (idx < MAX_DPB_SIZE)
            {
                for (mfxU16 lx = 0; lx < 2; lx++)
                {
                    for (mfxU16 j = 0; j < numRefActive[lx]; j++)
                    {
                        if (RPL[lx][j] == idx)
                        {
                            Remove(RPL[lx], j);
                            numRefActive[lx]--;
                            break;
                        }
                    }
                }
            }
        }

        for (mfxU16 i = 0; i < 16 && pLCtrl->PreferredRefList[i].FrameOrder != static_cast<mfxU32>(MFX_FRAMEORDER_UNKNOWN); i++)
        {
            mfxU8 idx = GetDPBIdxByFO(DPB, pLCtrl->PreferredRefList[i].FrameOrder);

            if (idx < MAX_DPB_SIZE)
            {
                for (mfxU16 lx = 0; lx < 2; lx++)
                {
                    for (mfxU16 j = 0; j < numRefActive[lx]; j++)
                    {
                        if (RPL[lx][j] == idx)
                        {
                            Remove(RPL[lx], j);
                            Insert(RPL[lx], pref[lx]++, idx);
                            break;
                        }
                    }
                }
            }
        }

        for (mfxU16 lx = 0; lx < 2; lx++)
        {
            if (numRefActive[lx] > MaxRef[lx])
            {
                Remove(RPL[lx], MaxRef[lx], (numRefActive[lx] - MaxRef[lx]));
                numRefActive[lx] = (mfxU8)MaxRef[lx];
            }
        }

        if (l0 == 0)
            RPL[0][l0++] = 0;
    }

    assert(l0 > 0);

    if (isB && !l1 && l0)
        RPL[1][l1++] = RPL[0][l0-1];

    if (!isB)
    {
        l1 = 0; //ignore l1 != l0 in pExtLists for LDB (unsupported by HW)

        for (mfxU16 i = 0; i < Min<mfxU16>(l0, NumRefLX[1]); i ++)
            RPL[1][l1++] = RPL[0][i];
        //for (mfxI16 i = l0 - 1; i >= 0 && l1 < NumRefLX[1]; i --)
        //    RPL[1][l1++] = RPL[0][i];
    }
}

mfxU8 GetCodingType(Task const & task)
{
    mfxU8 t = CODING_TYPE_B;

    assert(task.m_frameType & (MFX_FRAMETYPE_I | MFX_FRAMETYPE_P | MFX_FRAMETYPE_B));

    if (task.m_frameType & MFX_FRAMETYPE_I)
        return CODING_TYPE_I;

    if (task.m_frameType & MFX_FRAMETYPE_P)
        return CODING_TYPE_P;

    if (task.m_ldb)
        return CODING_TYPE_B;

    for (mfxU8 i = 0; i < 2; i ++)
    {
        for (mfxU32 j = 0; j < task.m_numRefActive[i]; j ++)
        {
            if (task.m_dpb[0][task.m_refPicList[i][j]].m_ldb)
                continue; // don't count LDB as B here

            if (task.m_dpb[0][task.m_refPicList[i][j]].m_codingType > CODING_TYPE_B)
                return CODING_TYPE_B2;

            if (task.m_dpb[0][task.m_refPicList[i][j]].m_codingType == CODING_TYPE_B)
                t = CODING_TYPE_B1;
        }
    }

    return t;
}

mfxU8 GetSHNUT(Task const & task, bool RAPIntra)
{
    const bool isI   = !!(task.m_frameType & MFX_FRAMETYPE_I);
    const bool isRef = !!(task.m_frameType & MFX_FRAMETYPE_REF);
    const bool isIDR = !!(task.m_frameType & MFX_FRAMETYPE_IDR);

#if (MFX_VERSION >= 1025)
    if (task.m_ctrl.MfxNalUnitType)
    {
        switch (task.m_ctrl.MfxNalUnitType)
        {
        case MFX_HEVC_NALU_TYPE_TRAIL_R:
            if (task.m_poc > task.m_lastRAP && isRef)
                return TRAIL_R;
            break;
        case MFX_HEVC_NALU_TYPE_TRAIL_N:
            if (task.m_poc > task.m_lastRAP && !isRef)
                return TRAIL_N;
            break;
        case MFX_HEVC_NALU_TYPE_RASL_R:
            if (task.m_poc < task.m_lastRAP && isRef)
                return RASL_R;
            break;
        case MFX_HEVC_NALU_TYPE_RASL_N:
            if (task.m_poc < task.m_lastRAP && !isRef)
                return RASL_N;
            break;
        case MFX_HEVC_NALU_TYPE_RADL_R:
            if (task.m_poc < task.m_lastRAP && isRef)
                return RADL_R;
            break;
        case MFX_HEVC_NALU_TYPE_RADL_N:
            if (task.m_poc < task.m_lastRAP && !isRef)
                return RADL_N;
            break;
        case MFX_HEVC_NALU_TYPE_CRA_NUT:
            if (isI)
                return CRA_NUT;
            break;
        case MFX_HEVC_NALU_TYPE_IDR_W_RADL:
            if (isIDR)
                return IDR_W_RADL;
            break;
        case MFX_HEVC_NALU_TYPE_IDR_N_LP:
            if (isIDR)
                return IDR_N_LP;
            break;
        }
    }
#endif

    if (isIDR)
        return IDR_W_RADL;

    if (isI && RAPIntra)
    {
        //TODO: add mode with TRAIL_R only
        const DpbArray& DPB = task.m_dpb[TASK_DPB_AFTER];
        for (mfxU16 i = 0; !isDpbEnd(DPB, i); i++)
        {
            if (DPB[i].m_ltr && DPB[i].m_idxRec != task.m_idxRec)
            {
                //following frames may refer to prev. GOP
                return TRAIL_R;
            }
        }
        return CRA_NUT;
    }

    if (task.m_tid > 0)
    {
        if (isRef)
            return TSA_R;
        return TSA_N;
    }

    if (task.m_poc > task.m_lastRAP)
    {
        if (isRef)
            return TRAIL_R;
        return TRAIL_N;
    }

    if (isRef)
        return RASL_R;
    return RASL_N;
}

IntraRefreshState GetIntraRefreshState(
    MfxVideoParam const & video,
    mfxU32                frameOrderInGopDispOrder,
    mfxEncodeCtrl const * ctrl)
{
    IntraRefreshState state={};
    const mfxExtCodingOption2*   extOpt2Init = &(video.m_ext.CO2);
    const mfxExtCodingOption3 *  extOpt3Init = &(video.m_ext.CO3);

    state.firstFrameInCycle = false;
    if (extOpt2Init->IntRefType == 0)
        return state;

    mfxU32 refreshPeriod = extOpt3Init->IntRefCycleDist ? extOpt3Init->IntRefCycleDist : extOpt2Init->IntRefCycleSize;
    mfxU32 offsetFromStartOfGop = extOpt3Init->IntRefCycleDist ? refreshPeriod : 1; // 1st refresh cycle in GOP starts with offset

    mfxI32 frameOrderMinusOffset = frameOrderInGopDispOrder - offsetFromStartOfGop;
    if (frameOrderMinusOffset < 0)
        return state; // too early to start refresh

    mfxU32 frameOrderInRefreshPeriod = frameOrderMinusOffset % refreshPeriod;
    if (frameOrderInRefreshPeriod >= extOpt2Init->IntRefCycleSize)
        return state; // for current refresh period refresh cycle is already passed

    mfxU32 refreshDimension = extOpt2Init->IntRefType == HORIZ_REFRESH ? CeilDiv(video.m_ext.HEVCParam.PicHeightInLumaSamples, video.LCUSize) : CeilDiv(video.m_ext.HEVCParam.PicWidthInLumaSamples, video.LCUSize);
    mfxU16 intraStripeWidthInMBs = (mfxU16)((refreshDimension + video.m_ext.CO2.IntRefCycleSize - 1) / video.m_ext.CO2.IntRefCycleSize);

    // check if Intra refresh required for current frame
    mfxU32 numFramesWithoutRefresh = extOpt2Init->IntRefCycleSize - (refreshDimension + intraStripeWidthInMBs - 1) / intraStripeWidthInMBs;
    mfxU32 idxInRefreshCycle = frameOrderInRefreshPeriod;
    state.firstFrameInCycle = (idxInRefreshCycle == 0);
    mfxI32 idxInActualRefreshCycle = idxInRefreshCycle - numFramesWithoutRefresh;
    if (idxInActualRefreshCycle < 0)
        return state; // actual refresh isn't started yet within current refresh cycle, no Intra column/row required for current frame

    state.refrType = extOpt2Init->IntRefType;
    state.IntraSize = intraStripeWidthInMBs;
    state.IntraLocation = (mfxU16)idxInActualRefreshCycle * intraStripeWidthInMBs;
    // set QP for Intra macroblocks within refreshing line
    state.IntRefQPDelta = extOpt2Init->IntRefQPDelta;
    if (ctrl)
    {
        mfxExtCodingOption2 * extOpt2Runtime = ExtBuffer::Get(*ctrl);
        if (extOpt2Runtime && extOpt2Runtime->IntRefQPDelta <= 51 && extOpt2Runtime->IntRefQPDelta >= -51)
            state.IntRefQPDelta = extOpt2Runtime->IntRefQPDelta;
    }

    return state;
}

void ConfigureTask(
    Task &                   task,
    Task const &             prevTask,
    MfxVideoParam const &    par,
    ENCODE_CAPS_HEVC const & caps,
    mfxU32 &baseLayerOrder)
{
    const bool isI    = !!(task.m_frameType & MFX_FRAMETYPE_I);
    const bool isP    = !!(task.m_frameType & MFX_FRAMETYPE_P);
    const bool isB    = !!(task.m_frameType & MFX_FRAMETYPE_B);
    const bool isIDR  = !!(task.m_frameType & MFX_FRAMETYPE_IDR);
    const mfxExtCodingOption3& CO3 = par.m_ext.CO3;
    (void)(caps);

    const mfxU8 maxQP = mfxU8(51 + 6 * (par.mfx.FrameInfo.BitDepthLuma - 8));

    mfxExtAVCRefLists*      pExtLists = ExtBuffer::Get(task.m_ctrl);
    mfxExtAVCRefListCtrl*   pExtListCtrl = ExtBuffer::Get(task.m_ctrl);

    mfxU16 IntRefType = par.m_ext.CO2.IntRefType;
    task.m_SkipMode = 0;

    if (task.m_ctrl.SkipFrame)
    {
        mfxExtCodingOption2 *CO2RT = ExtBuffer::Get(task.m_ctrl);
        HevcSkipMode mode;
        mode.SetMode((CO2RT && CO2RT->SkipFrame) ? CO2RT->SkipFrame : par.m_ext.CO2.SkipFrame, par.Protected != 0);
        if (mode.NeedCurrentFrameSkipping() && (!(task.m_frameType & MFX_FRAMETYPE_REF) || (par.mfx.GopRefDist == 1 && (task.m_frameType & MFX_FRAMETYPE_P))))
        {
            task.m_SkipMode = mode.GetMode();
            task.m_frameType &= ~MFX_FRAMETYPE_REF;
        }
        else
        {
            task.m_ctrl.SkipFrame = 0;
            task.m_SkipMode = 0;
        }
    }
#if (MFX_VERSION >= 1025)
    if (!IsOn(par.m_ext.CO3.EnableNalUnitType) && task.m_ctrl.MfxNalUnitType!=0)
        task.m_ctrl.MfxNalUnitType = 0;
#endif


    mfxExtMBQP *mbqp = ExtBuffer::Get(task.m_ctrl);
    if (mbqp && mbqp->NumQPAlloc > 0)
        task.m_bCUQPMap = 1;
#ifdef MFX_ENABLE_HEVCE_ROI
    // process roi
    mfxExtEncoderROI const * parRoi = &par.m_ext.ROI;
    mfxExtEncoderROI * rtRoi = ExtBuffer::Get(task.m_ctrl);


    if (rtRoi && rtRoi->NumROI)
    {
        bool bTryROIViaMBQP;
        mfxStatus sts = CheckAndFixRoi(par, caps, rtRoi, bTryROIViaMBQP);
        if (sts == MFX_ERR_INVALID_VIDEO_PARAM  || (bTryROIViaMBQP && !par.bROIViaMBQP))
            parRoi = 0;
        else
            parRoi = (mfxExtEncoderROI const *)rtRoi;

    }
    if (parRoi && parRoi->NumROI && par.bROIViaMBQP)
        task.m_bCUQPMap = 1;

    task.m_numRoi = 0;
    if (parRoi && parRoi->NumROI && !par.bROIViaMBQP)
    {
        for (mfxU16 i = 0; i < parRoi->NumROI; i ++)
        {
            memcpy_s(&task.m_roi[i], sizeof(RoiData), &parRoi->ROI[i], sizeof(RoiData));
            task.m_numRoi ++;
        }
#if MFX_VERSION > 1021
        task.m_bPriorityToDQPpar = (par.isSWBRC() && task.m_roiMode == MFX_ROI_MODE_PRIORITY);
        if (par.mfx.RateControlMethod != MFX_RATECONTROL_CQP)
            task.m_roiMode = parRoi->ROIMode;
#endif // MFX_VERSION > 1021
    }

#else
    caps;
#endif // MFX_ENABLE_HEVCE_ROI
#ifdef MFX_ENABLE_HEVCE_DIRTY_RECT
    // DirtyRect
    mfxExtDirtyRect const * parDirtyRect = &par.m_ext.DirtyRect;
    mfxExtDirtyRect * rtDirtyRect = ExtBuffer::Get(task.m_ctrl);

    if (rtDirtyRect && rtDirtyRect->NumRect)
    {
        mfxStatus sts = CheckAndFixDirtyRect(caps, par, rtDirtyRect);
        if (sts == MFX_ERR_INVALID_VIDEO_PARAM)
            parDirtyRect = 0;
        else
            parDirtyRect = (mfxExtDirtyRect const *)rtDirtyRect;
    }

    task.m_numDirtyRect = 0;
    if (parDirtyRect && parDirtyRect->NumRect) {
        for (mfxU16 i = 0; i < parDirtyRect->NumRect; i++) {
            memcpy_s(&task.m_dirtyRect[i], sizeof(RectData), &parDirtyRect->Rect[i], sizeof(RectData));
            task.m_numDirtyRect++;
        }
    }
#endif

    if (task.m_tid == 0 && IntRefType)
    {
       if (isI)
            baseLayerOrder = 0;

        task.m_IRState = GetIntraRefreshState(
            par,
            baseLayerOrder ++,
            &(task.m_ctrl));
    }

     mfxU32 needRecoveryPointSei = (par.m_ext.CO.RecoveryPointSEI == MFX_CODINGOPTION_ON &&
        ((IntRefType && task.m_IRState.firstFrameInCycle && task.m_IRState.IntraLocation == 0) ||
        (IntRefType == 0 && isI)));

    mfxU32 needCpbRemovalDelay = isIDR || needRecoveryPointSei;

    if (par.isTL())
    {
        task.m_tid = isI ? 0 : par.GetTId((task.m_poc - prevTask.m_lastIPoc)/(par.isField()?2:1));

        if (par.HighestTId() == task.m_tid && (!par.isField() || task.m_secondField))
            task.m_frameType &= ~MFX_FRAMETYPE_REF;
    }

    const bool isRef = !!(task.m_frameType & MFX_FRAMETYPE_REF);

    if (par.mfx.RateControlMethod == MFX_RATECONTROL_CQP)
    {
        // set coding type and QP
        if (isB)
        {
            task.m_qpY = (mfxI8)par.mfx.QPB;
            if (par.mfx.RateControlMethod == MFX_RATECONTROL_CQP && par.isBPyramid())
            {
                if (task.m_level == 0)
                    task.m_qpY = (mfxI8)par.mfx.QPP;
                else
                // m_level starts from 1
                    task.m_qpY = (mfxI8)Clip3<mfxI32>(1, maxQP, par.m_ext.CO3.QPOffset[Clip3<mfxI32>(0, 7, task.m_level - 1)] + task.m_qpY);
            }
        }
        else if (isP)
        {
            // encode P as GPB
            task.m_qpY = (mfxI8)par.mfx.QPP;
            if (par.isLowDelay())
                task.m_qpY = (mfxI8)Clip3<mfxI32>(1, maxQP, par.m_ext.CO3.QPOffset[PLayer(task.m_poc - prevTask.m_lastIPoc, par)] + task.m_qpY);
        }
        else
        {
            assert(task.m_frameType & MFX_FRAMETYPE_I);
            task.m_qpY = (mfxI8)par.mfx.QPI;
        }
        if (task.m_secondField /*&& IsOn(par.m_ext.CO3.EnableQPOffset)*/)
        {
            task.m_qpY += 1;
        }
 

        if (task.m_ctrl.QP)
            task.m_qpY = (mfxI8)task.m_ctrl.QP;

        task.m_qpY -= 6 * par.m_sps.bit_depth_luma_minus8;

        if (task.m_qpY < 0 && (IsOn(par.mfx.LowPower) || par.m_platform.CodeName >= MFX_PLATFORM_KABYLAKE
            ))
            task.m_qpY = 0;
    }
    else if (par.mfx.RateControlMethod != MFX_RATECONTROL_LA_EXT)
        task.m_qpY = 0;

    if (IsOn(CO3.GPB) && isP)
    {
        // encode P as GPB
        task.m_frameType &= ~MFX_FRAMETYPE_P;
        task.m_frameType |= MFX_FRAMETYPE_B;
        task.m_ldb = true;
    }

    task.m_lastIPoc = prevTask.m_lastIPoc;
    task.m_lastRAP = prevTask.m_lastRAP;
    task.m_eo = prevTask.m_eo + 1;


    task.m_dpb_output_delay = (task.m_fo + par.m_sps.sub_layer[0].max_num_reorder_pics - task.m_eo);

    InitDPB(task, prevTask, pExtListCtrl);

    //construct ref lists
    Zero(task.m_numRefActive);
    Fill(task.m_refPicList, IDX_INVALID);

    if (isB)
    {
        mfxI32 layer = par.isBPyramid() ? Clip3<mfxI32>(0, 7, task.m_level - 1) : 0;
        task.m_numRefActive[0] = (mfxU8)CO3.NumRefActiveBL0[layer];
        task.m_numRefActive[1] = (mfxU8)CO3.NumRefActiveBL1[layer];
    }
    if (isP)
    {
        mfxI32 layer = PLayer(task.m_poc - prevTask.m_lastIPoc, par);
        task.m_numRefActive[0] = (mfxU8)CO3.NumRefActiveP[layer];
        task.m_numRefActive[1] = (mfxU8)Min(CO3.NumRefActiveP[layer], par.m_ext.DDI.NumActiveRefBL1);
    }

    if (!isI)
    {
        ConstructRPL(par, task.m_dpb[TASK_DPB_ACTIVE], isB, task.m_poc, task.m_tid, task.m_level, task.m_secondField, task.m_bottomField,
            task.m_refPicList, task.m_numRefActive, pExtLists, pExtListCtrl);
    }

    task.m_codingType = GetCodingType(task);

    if (isIDR)
        task.m_insertHeaders |= (INSERT_VPS | INSERT_SPS | INSERT_PPS);

    if (needCpbRemovalDelay && par.m_sps.vui.hrd_parameters_present_flag )
        task.m_insertHeaders |= INSERT_BPSEI;

    if (   par.m_sps.vui.frame_field_info_present_flag
        || par.m_sps.vui.hrd.nal_hrd_parameters_present_flag
        || par.m_sps.vui.hrd.vcl_hrd_parameters_present_flag)
        task.m_insertHeaders |= INSERT_PTSEI;

    if (IsOn(par.m_ext.CO2.RepeatPPS))  task.m_insertHeaders |= INSERT_PPS;
    if (IsOn(par.m_ext.CO.AUDelimiter)) task.m_insertHeaders |= INSERT_AUD;

    // update dpb
    if (isIDR)
        Fill(task.m_dpb[TASK_DPB_AFTER], IDX_INVALID);
    else
        Copy(task.m_dpb[TASK_DPB_AFTER], task.m_dpb[TASK_DPB_ACTIVE]);

    if (isRef)
    {
        if (isI)
            task.m_lastIPoc = task.m_poc;

        UpdateDPB(par, task, task.m_dpb[TASK_DPB_AFTER], pExtListCtrl);

        // is current frame will be used as LTR
        if (par.LTRInterval)
            task.m_ltr |= isLTR(task.m_dpb[TASK_DPB_AFTER], par.LTRInterval, task.m_poc);
    }


    task.m_shNUT = GetSHNUT(task, par.RAPIntra);
    if (task.m_shNUT == CRA_NUT || task.m_shNUT == IDR_W_RADL || task.m_shNUT == IDR_N_LP)
        task.m_lastRAP = task.m_poc;

    par.GetSliceHeader(task, prevTask, caps, task.m_sh);
    task.m_statusReportNumber = prevTask.m_statusReportNumber + 1;
    task.m_statusReportNumber = (task.m_statusReportNumber == 0) ? 1 : task.m_statusReportNumber;

}

mfxI64 CalcDTSFromPTS(
    mfxFrameInfo const & info,
    mfxU16               dpbOutputDelay,
    mfxU64               timeStamp)
{
    if (timeStamp != static_cast<mfxU64>(MFX_TIMESTAMP_UNKNOWN))
    {
        mfxF64 tcDuration90KHz = (mfxF64)info.FrameRateExtD / (info.FrameRateExtN ) * 90000; // calculate tick duration
        return mfxI64(timeStamp - tcDuration90KHz * dpbOutputDelay); // calculate DTS from PTS
    }

    return MFX_TIMESTAMP_UNKNOWN;
}


bool IsFrameToSkip(Task&  task, MfxFrameAllocResponse & poolRec, bool bSWBRC)
{
    if (task.m_bSkipped)
        return true;

    if ((task.m_frameType & MFX_FRAMETYPE_B) && (!task.m_ldb) && bSWBRC)
    {
        mfxU8 ind =  task.m_refPicList[1][0];
        if (ind < 15)
        {
            return poolRec.GetFlag(task.m_dpb[0][ind].m_idxRec)!=0;
        }
    }
    return false;
}

mfxStatus CodeAsSkipFrame(     MFXCoreInterface &            core,
                               MfxVideoParam const &  video,
                               Task&       task,
                               MfxFrameAllocResponse & poolSkip,
                               MfxFrameAllocResponse & poolRec)
{
    mfxStatus sts = MFX_ERR_NONE;

    if (task.m_midRaw == NULL)
    {

        task.m_idxRaw = (mfxU8)FindFreeResourceIndex(poolSkip);
        task.m_midRaw = AcquireResource(poolSkip, task.m_idxRaw);
        MFX_CHECK_NULL_PTR1(task.m_midRaw);
    }
    if (task.m_frameType & MFX_FRAMETYPE_I)
    {
        FrameLocker lock1(&core, task.m_midRaw);

        mfxU32 size = lock1.Pitch*video.mfx.FrameInfo.Height;
        memset(lock1.Y, 0, size);

        switch (video.mfx.FrameInfo.FourCC)
        {
        case MFX_FOURCC_NV12:
            memset(lock1.UV, 126, size >> 1);
            break;

        case MFX_FOURCC_P010:
            memset(lock1.UV, 0, size >> 1);
            break;

        default:
            return MFX_ERR_UNDEFINED_BEHAVIOR;
        }
    }
    else
    {
        mfxU8 ind = task.m_refPicList[0][0] ;
        MFX_CHECK(ind < 15, MFX_ERR_UNDEFINED_BEHAVIOR);

        DpbFrame& refFrame = task.m_dpb[0][ind];
        FrameLocker lock_dst(&core, task.m_midRaw);
        FrameLocker lock_src(&core, refFrame.m_midRec);

        mfxFrameSurface1 surfSrc = { {0,}, video.mfx.FrameInfo, lock_src };
        mfxFrameSurface1 surfDst = { {0,}, video.mfx.FrameInfo, lock_dst };

        sts = core.CopyFrame(&surfDst, &surfSrc);
        MFX_CHECK_STS(sts);

        //poolRec.SetFlag(refFrame.m_idxRec, 1);

    }
    poolRec.SetFlag(task.m_idxRec, 1);


    return sts;
}

mfxStatus GetCUQPMapBlockSize(mfxU16 frameWidth, mfxU16 frameHeight,
    mfxU16 CUQPWidth, mfxU16 CUHeight,
    mfxU16 &blkWidth,
    mfxU16 &blkHeight)
{

    MFX_CHECK(CUQPWidth && CUHeight, MFX_ERR_UNDEFINED_BEHAVIOR);
    mfxU16 BlkSizes[] = {4, 8, 16, 32, 64};
    mfxU32 numBlk = sizeof(BlkSizes) / sizeof(BlkSizes[0]);

    blkWidth = blkHeight = 0;

    for (mfxU32 blk = 0; blk < numBlk; blk++)
    {
        if (BlkSizes[blk] * CUQPWidth >= frameWidth)
        {
            blkWidth = BlkSizes[blk];
            break;
        }
    }
    for (mfxU32 blk = 0; blk < numBlk; blk++)
    {
        if (BlkSizes[blk] * CUHeight >= frameHeight)
        {
            blkHeight = BlkSizes[blk];
            break;
        }
    }
    MFX_CHECK(blkWidth &&  blkHeight &&
        (blkWidth   * (CUQPWidth - 1) < frameWidth) &&
        (blkHeight  * (CUHeight - 1) < frameHeight), MFX_ERR_UNDEFINED_BEHAVIOR);

    return MFX_ERR_NONE;
}

}; //namespace MfxHwH265Encode
#endif
