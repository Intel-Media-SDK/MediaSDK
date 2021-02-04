// Copyright (c) 2018-2020 Intel Corporation
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

#include "mfx_h265_encode_hw_utils.h"
#include "mfx_h265_encode_hw_bs.h"

using namespace mfx;

namespace MfxHwH265Encode
{

template<class T, class Enable = void>
struct DefaultFiller
{
    static constexpr typename std::remove_reference<T>::type Get()
    {
        return typename std::remove_reference<T>::type();
    }
};

template<class T>
struct DefaultFiller<T, typename std::enable_if<
    std::is_arithmetic<
    typename std::remove_reference<T>::type
    >::value
>::type>
{
    static constexpr typename std::remove_reference<T>::type Get()
    {
        return typename std::remove_reference<T>::type(-1);
    }
};

template<class A>
void Remove(A &_from, size_t _where, size_t _num = 1)
{
    if (std::end(_from) < std::begin(_from) + _where + _num)
        throw std::out_of_range("Remove() target is out of container range");

    auto it = std::copy(std::begin(_from) + _where + _num, std::end(_from), std::begin(_from) + _where);
    std::fill(it, std::end(_from), DefaultFiller<decltype(*it)>::Get());
}

template<class T, class A>
void Insert(A& _to, mfxU32 _where, T const & _what)
{
    if (std::end(_to) <= std::begin(_to) + _where)
        throw std::out_of_range("Insert() target is out of container range");

    if (std::begin(_to) + _where + 1 != std::end(_to))
        std::copy_backward(std::begin(_to) + _where, std::end(_to) - 1, std::end(_to));

    _to[_where] = _what;
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
        return std::min<mfxU8>(7, GetPFrameLevel((order / (par.isField() ? 2 : 1)) % par.PPyrInterval, par.PPyrInterval));
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

namespace ExtBuffer
{
    bool Construct(mfxVideoParam const & par, mfxExtHEVCParam& buf, mfxExtBuffer* pBuffers[], mfxU16 & numbuffers, mfxU32 codedPicAlignment)
    {
        if (!Construct<mfxVideoParam, mfxExtHEVCParam>(par, buf, pBuffers, numbuffers))
        {
            buf.PicWidthInLumaSamples  = align2_value(par.mfx.FrameInfo.CropW > 0 ? (mfxU16)(par.mfx.FrameInfo.CropW + par.mfx.FrameInfo.CropX)  : par.mfx.FrameInfo.Width,  codedPicAlignment);
            buf.PicHeightInLumaSamples = align2_value(par.mfx.FrameInfo.CropH > 0 ? (mfxU16)(par.mfx.FrameInfo.CropH + par.mfx.FrameInfo.CropY)  : par.mfx.FrameInfo.Height, codedPicAlignment);

            return false;
        }
        if (buf.PicWidthInLumaSamples == 0)
            buf.PicWidthInLumaSamples  = align2_value(par.mfx.FrameInfo.CropW > 0 ? (mfxU16)(par.mfx.FrameInfo.CropW + par.mfx.FrameInfo.CropX)  : par.mfx.FrameInfo.Width,  codedPicAlignment);

        if (buf.PicHeightInLumaSamples == 0)
            buf.PicHeightInLumaSamples = align2_value(par.mfx.FrameInfo.CropH > 0 ? (mfxU16)(par.mfx.FrameInfo.CropH + par.mfx.FrameInfo.CropY)  : par.mfx.FrameInfo.Height, codedPicAlignment);

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
    : m_platform(MFX_HW_UNKNOWN)
    , BufferSizeInKB    (0)
    , InitialDelayInKB  (0)
    , TargetKbps        (0)
    , MaxKbps           (0)
    , LTRInterval       (0)
    , PPyrInterval      (0)
    , LCUSize           (0)
    , CodedPicAlignment (0)
    , HRDConformance    (false)
    , RawRef            (false)
    , bROIViaMBQP       (false)
    , bMBQPInput        (false)
    , RAPIntra          (false)
    , bFieldReord       (false)
    , bNonStandardReord (false)
{
    Zero(*(mfxVideoParam*)this);
}

MfxVideoParam::MfxVideoParam(MfxVideoParam const & par)
{
     m_platform = par.m_platform;
     Construct(par);

     m_vps = par.m_vps;
     m_sps = par.m_sps;
     m_pps = par.m_pps;

     CopyCalcParams(par);
}

MfxVideoParam::MfxVideoParam(mfxVideoParam const & par, eMFXHWType const & platform)
    : m_platform(platform)
    , BufferSizeInKB    (0)
    , InitialDelayInKB  (0)
    , TargetKbps        (0)
    , MaxKbps           (0)
    , LTRInterval       (0)
    , PPyrInterval      (0)
    , LCUSize           (0)
    , CodedPicAlignment (0)
    , HRDConformance    (false)
    , RawRef            (false)
    , bROIViaMBQP       (false)
    , bMBQPInput        (false)
    , RAPIntra          (false)
{
    Zero(*(mfxVideoParam*)this);
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
    m_platform = par.m_platform;
    Construct(par);
    CopyCalcParams(par);

    m_vps = par.m_vps;
    m_sps = par.m_sps;
    m_pps = par.m_pps;

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

    CodedPicAlignment = GetAlignmentByPlatform(m_platform);
    ExtBuffer::Construct(par, m_ext.HEVCParam, m_ext.m_extParam, base.NumExtParam, CodedPicAlignment);
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
#ifdef MFX_ENABLE_HEVCE_HDR_SEI
    ExtBuffer::Construct(par, m_ext.DisplayColour, m_ext.m_extParam, base.NumExtParam);
    ExtBuffer::Construct(par, m_ext.LightLevel, m_ext.m_extParam, base.NumExtParam);
#endif
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

    ExtBuffer::Set(par, m_ext.DDI);
    ExtBuffer::Set(par, m_ext.AVCTL);
    ExtBuffer::Set(par, m_ext.ROI);
    ExtBuffer::Set(par, m_ext.VSI);
    ExtBuffer::Set(par, m_ext.extBRC);
#ifdef MFX_ENABLE_HEVCE_DIRTY_RECT
    ExtBuffer::Set(par, m_ext.DirtyRect);
#endif
#ifdef MFX_ENABLE_HEVCE_HDR_SEI
    ExtBuffer::Set(par, m_ext.DisplayColour);
    ExtBuffer::Set(par, m_ext.LightLevel);
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
            std::copy(buf, buf + len, pSPSPPS->SPSBuffer);
            pSPSPPS->SPSBufSize = (mfxU16)len;

            if (pSPSPPS->PPSBuffer)
            {
                packer.GetPPS(buf, len);
                MFX_CHECK(pSPSPPS->PPSBufSize >= len, MFX_ERR_NOT_ENOUGH_BUFFER);

                std::copy(buf, buf + len, pSPSPPS->PPSBuffer);
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

        std::copy(buf, buf + len, pVPS->VPSBuffer);
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
#ifdef MFX_ENABLE_HEVCE_HDR_SEI
    bUnsupported += ExtBuffer::CheckBufferParams(m_ext.DisplayColour, true);
    bUnsupported += ExtBuffer::CheckBufferParams(m_ext.LightLevel, true);
#endif
    return !!bUnsupported;
}

void MfxVideoParam::SyncVideoToCalculableParam()
{
    mfxU32 multiplier = std::max<mfxU32>(mfx.BRCParamMultiplier, 1);

    BufferSizeInKB = mfx.BufferSizeInKB * multiplier;
    LTRInterval    = 0;
    PPyrInterval   = (mfx.NumRefFrame >0) ? std::min<mfxU32> (DEFAULT_PPYR_INTERVAL, mfx.NumRefFrame) : DEFAULT_PPYR_INTERVAL;

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
        maxVal32 = std::max(maxVal32, TargetKbps);

        if (mfx.RateControlMethod != MFX_RATECONTROL_AVBR)
            maxVal32 = std::max({maxVal32, MaxKbps, InitialDelayInKB});
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

    m_ext.HEVCParam.GeneralConstraintFlags = m_sps.general.rext_constraint_flags_0_31;
    m_ext.HEVCParam.GeneralConstraintFlags |= ((mfxU64)m_sps.general.rext_constraint_flags_32_42 << 32);

    mfx.NumRefFrame = m_sps.sub_layer[0].max_dec_pic_buffering_minus1;

#if (MFX_VERSION >= 1027)
    m_ext.CO3.TargetChromaFormatPlus1 = 1 +
#endif
        (mfx.FrameInfo.ChromaFormat = m_sps.chroma_format_idc);

    m_ext.HEVCParam.PicWidthInLumaSamples  = (mfxU16)m_sps.pic_width_in_luma_samples;
    m_ext.HEVCParam.PicHeightInLumaSamples = (mfxU16)m_sps.pic_height_in_luma_samples;

    fi.Width  = align2_value(m_ext.HEVCParam.PicWidthInLumaSamples,  HW_SURF_ALIGN_W);
    fi.Height = align2_value(m_ext.HEVCParam.PicHeightInLumaSamples, HW_SURF_ALIGN_H);

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

#if (MFX_VERSION >= 1027)
    m_ext.CO3.TargetBitDepthLuma =
#endif
        fi.BitDepthLuma = m_sps.bit_depth_luma_minus8 + 8;

#if (MFX_VERSION >= 1027)
    m_ext.CO3.TargetBitDepthChroma =
#endif
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

#if (MFX_VERSION >= 1026)
    m_ext.CO3.TransformSkip = (m_pps.transform_skip_enabled_flag ? (mfxU16)MFX_CODINGOPTION_ON : (mfxU16)MFX_CODINGOPTION_OFF);
#endif
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
    general.frame_only_constraint_flag  = !(mfx.FrameInfo.PicStruct & MFX_PICSTRUCT_FIELD_SINGLE);;
    general.level_idc                   = (mfxU8)(mfx.CodecLevel & 0xFF) * 3;

    if (mfx.CodecProfile == MFX_PROFILE_HEVC_REXT
        )
    {
        general.rext_constraint_flags_0_31  = (mfxU32)(m_ext.HEVCParam.GeneralConstraintFlags & 0xffffffff);
        general.rext_constraint_flags_32_42 = (mfxU32)(m_ext.HEVCParam.GeneralConstraintFlags >> 32);
    }

    slo.max_dec_pic_buffering_minus1    = mfx.NumRefFrame;
    slo.max_num_reorder_pics            = std::min(GetNumReorderFrames(mfx.GopRefDist - 1, isBPyramid(),isField(), bFieldReord), slo.max_dec_pic_buffering_minus1);
    slo.max_latency_increase_plus1      = 0;

    Zero(m_sps);
    ((LayersInfo&)m_sps) = ((LayersInfo&)m_vps);
    m_sps.video_parameter_set_id   = m_vps.video_parameter_set_id;
    m_sps.max_sub_layers_minus1    = m_vps.max_sub_layers_minus1;
    m_sps.temporal_id_nesting_flag = m_vps.temporal_id_nesting_flag;

    m_sps.seq_parameter_set_id              = 0;
#if (MFX_VERSION >= 1027)
    m_sps.chroma_format_idc                 = m_ext.CO3.TargetChromaFormatPlus1 - 1;
#else
    //hardcoded untill support for encoding in chroma format other than YUV420 support added.
    m_sps.chroma_format_idc                 = 1;
#endif
    m_sps.separate_colour_plane_flag        = 0;
    m_sps.pic_width_in_luma_samples         = m_ext.HEVCParam.PicWidthInLumaSamples;
    m_sps.pic_height_in_luma_samples        = m_ext.HEVCParam.PicHeightInLumaSamples;
    m_sps.conformance_window_flag           = 0;
#if (MFX_VERSION >= 1027)
    m_sps.bit_depth_luma_minus8             = std::max<mfxI32>(0, m_ext.CO3.TargetBitDepthLuma   - 8);
    m_sps.bit_depth_chroma_minus8           = std::max<mfxI32>(0, m_ext.CO3.TargetBitDepthChroma - 8);
#else
    m_sps.bit_depth_luma_minus8             = std::max<mfxI32>(0, mfx.FrameInfo.BitDepthLuma   - 8);
    m_sps.bit_depth_chroma_minus8           = std::max<mfxI32>(0, mfx.FrameInfo.BitDepthChroma - 8);
#endif
    m_sps.log2_max_pic_order_cnt_lsb_minus4 = (mfxU8)clamp<mfxI32>(CeilLog2(mfx.GopRefDist*(isField() ? 2 : 1) + slo.max_dec_pic_buffering_minus1) - 1, 0, 12);

    m_sps.log2_min_luma_coding_block_size_minus3   = 0;
    m_sps.log2_diff_max_min_luma_coding_block_size = CeilLog2(LCUSize) - 3 - m_sps.log2_min_luma_coding_block_size_minus3;
    m_sps.log2_min_transform_block_size_minus2     = 0;
    m_sps.log2_diff_max_min_transform_block_size   = 3;
    m_sps.max_transform_hierarchy_depth_inter      = 2;
    m_sps.max_transform_hierarchy_depth_intra      = 2;
    m_sps.scaling_list_enabled_flag                = 0;
#if (MFX_VERSION >= 1025)
    if (m_platform >= MFX_HW_CNL)
    {
        m_sps.amp_enabled_flag = 1; // only 1
        m_sps.sample_adaptive_offset_enabled_flag = !(m_ext.HEVCParam.SampleAdaptiveOffset & MFX_SAO_DISABLE);
    }
    else
#endif  //(MFX_VERSION >= 1025)
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
        DpbArray dpb;
        DpbFrame tmp;
        mfxU8 rpl[2][MAX_DPB_SIZE] = {};
        mfxU8 nRef[2] = {};
        STRPS rps;
        mfxI32 STDist = std::min<mfxI32>(mfx.GopPicSize, 128);
        bool moreLTR = !!LTRInterval;
        mfxI32 lastIPoc = 0;

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
                    mfxI32 layer = isBPyramid() ? clamp<mfxI32>(cur->m_level - 1, 0, 7) : 0;
                    nRef[0] = (mfxU8)CO3.NumRefActiveBL0[layer];
                    nRef[1] = (mfxU8)CO3.NumRefActiveBL1[layer];
                }
                else
                {
                    mfxI32 layer = PLayer(cur->m_poc - lastIPoc, *this);
                    nRef[0] = (mfxU8)CO3.NumRefActiveP[layer];
                    // on VDENC for LDB frames L1 must be completely identical to L0
                    nRef[1] = (mfxU8)(IsOn(mfx.LowPower) ? CO3.NumRefActiveP[layer] : std::min(CO3.NumRefActiveP[layer], m_ext.DDI.NumActiveRefBL1));
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
                /* WA: there is no m_idxRec for formal Task,
                but we need to set it to 0 to defer from non-initialized dpb[i] with m_idxRec = IDX_INVALID*/
                tmp.m_idxRec = 0;
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

    // QpModulation support
    if (m_platform >= MFX_HW_ICL)
    {
        if (mfx.GopRefDist == 1)
            m_sps.low_delay_mode = 1;

        if (m_platform < MFX_HW_TGL_LP)
        {
            if ((m_ext.CO2.BRefType == MFX_B_REF_PYRAMID) &&
                ((mfx.GopRefDist == 4) || (mfx.GopRefDist == 8)))
                m_sps.hierarchical_flag = 1;
        }
        else
        {
            if ((m_ext.CO2.BRefType == MFX_B_REF_PYRAMID) || (isTL() && NumTL() < 4))
                m_sps.hierarchical_flag = 1;

            if (IsOn(mfx.LowPower) && m_sps.low_delay_mode && m_sps.hierarchical_flag)
                m_sps.gop_ref_dist = 1 << (NumTL() - 1); // distance between anchor frames for driver
        }
    }

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

    if (mfx.CodecProfile == MFX_PROFILE_HEVC_REXT)
    {
        //TBD
        m_sps.transform_skip_rotation_enabled_flag    = 0;
        m_sps.transform_skip_context_enabled_flag     = 0;
        m_sps.implicit_rdpcm_enabled_flag             = 0;
        m_sps.explicit_rdpcm_enabled_flag             = 0;
        m_sps.extended_precision_processing_flag      = 0;
        m_sps.intra_smoothing_disabled_flag           = 0;
        m_sps.high_precision_offsets_enabled_flag     = 0;
        m_sps.persistent_rice_adaptation_enabled_flag = 0;
        m_sps.cabac_bypass_alignment_enabled_flag     = 0;

        m_sps.range_extension_flag =
               m_sps.transform_skip_rotation_enabled_flag
            || m_sps.transform_skip_context_enabled_flag
            || m_sps.implicit_rdpcm_enabled_flag
            || m_sps.explicit_rdpcm_enabled_flag
            || m_sps.extended_precision_processing_flag
            || m_sps.intra_smoothing_disabled_flag
            || m_sps.high_precision_offsets_enabled_flag
            || m_sps.persistent_rice_adaptation_enabled_flag
            || m_sps.cabac_bypass_alignment_enabled_flag;

        m_sps.extension_flag |= m_sps.range_extension_flag;
    }


    Zero(m_pps);
    m_pps.seq_parameter_set_id = m_sps.seq_parameter_set_id;

    m_pps.pic_parameter_set_id                  = 0;
    m_pps.dependent_slice_segments_enabled_flag = 0;
    m_pps.output_flag_present_flag              = 0;
    m_pps.num_extra_slice_header_bits           = 0;
    m_pps.sign_data_hiding_enabled_flag         = 0;
    m_pps.cabac_init_present_flag               = 0;
    m_pps.num_ref_idx_l0_default_active_minus1  = std::max(m_ext.DDI.NumActiveRefP, m_ext.DDI.NumActiveRefBL0) -1;
    m_pps.num_ref_idx_l1_default_active_minus1  = m_ext.DDI.NumActiveRefBL1 -1;
    m_pps.init_qp_minus26                       = 0;
    m_pps.constrained_intra_pred_flag           = 0;

#if (MFX_VERSION >= 1025)
    if (m_platform >= MFX_HW_CNL)
        m_pps.transform_skip_enabled_flag = IsOn(m_ext.CO3.TransformSkip);
    else
#endif
        m_pps.transform_skip_enabled_flag = 0;

    m_pps.cu_qp_delta_enabled_flag = IsOn(m_ext.CO3.EnableMBQP);

    if (m_ext.CO2.MaxSliceSize)
        m_pps.cu_qp_delta_enabled_flag = 1;
#ifdef MFX_ENABLE_HEVCE_ROI
    if (m_ext.ROI.NumROI)
        m_pps.cu_qp_delta_enabled_flag = 1;
#endif
    if (IsOn(mfx.LowPower))
        m_pps.cu_qp_delta_enabled_flag = 1;

#if (MFX_VERSION >= 1025)
    if ((m_platform >= MFX_HW_CNL))
    {   // according to spec only 3 and 0 are supported
        if (LCUSize == 64)
            m_pps.diff_cu_qp_delta_depth = 3;
        else
            m_pps.diff_cu_qp_delta_depth = 0;
    }
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
        mfxU16 nTCol  = std::max<mfxU16>(m_ext.HEVCTiles.NumTileColumns, 1);
        mfxU16 nTRow  = std::max<mfxU16>(m_ext.HEVCTiles.NumTileRows,    1);

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

#if (MFX_VERSION >= 1025)
    if (m_platform >= MFX_HW_CNL)
        m_pps.loop_filter_across_slices_enabled_flag = 1;
    else
#endif //(MFX_VERSION >= 1025)
        m_pps.loop_filter_across_slices_enabled_flag = 0;

    m_pps.deblocking_filter_control_present_flag  = 1;
    m_pps.deblocking_filter_disabled_flag = !!m_ext.CO2.DisableDeblockingIdc;
    m_pps.deblocking_filter_override_enabled_flag = 1; // to disable deblocking per frame
#if MFX_VERSION >= MFX_VERSION_NEXT
    if (!m_pps.deblocking_filter_disabled_flag)
    {
        m_pps.beta_offset_div2 = mfxI8(m_ext.CO3.DeblockingBetaOffset / 2);
        m_pps.tc_offset_div2 = mfxI8(m_ext.CO3.DeblockingAlphaTcOffset / 2);
    }
#endif

    m_pps.scaling_list_data_present_flag              = 0;
    m_pps.lists_modification_present_flag             = 1;
    m_pps.log2_parallel_merge_level_minus2            = 0;
    m_pps.slice_segment_header_extension_present_flag = 0;

    if (mfx.CodecProfile == MFX_PROFILE_HEVC_REXT)
    {
        //TBD
        m_pps.cross_component_prediction_enabled_flag   = 0;
        m_pps.chroma_qp_offset_list_enabled_flag        = 0;
        m_pps.log2_sao_offset_scale_luma                = 0;
        m_pps.log2_sao_offset_scale_chroma              = 0;
        m_pps.chroma_qp_offset_list_len_minus1          = 0;
        m_pps.diff_cu_chroma_qp_offset_depth            = 0;
        m_pps.log2_max_transform_skip_block_size_minus2 = 0;
        Zero(m_pps.cb_qp_offset_list);
        Zero(m_pps.cr_qp_offset_list);

        m_pps.range_extension_flag =
               m_pps.cross_component_prediction_enabled_flag
            || m_pps.chroma_qp_offset_list_enabled_flag
            || m_pps.log2_sao_offset_scale_luma
            || m_pps.log2_sao_offset_scale_chroma
            || m_pps.chroma_qp_offset_list_len_minus1
            || m_pps.diff_cu_chroma_qp_offset_depth
            || m_pps.log2_max_transform_skip_block_size_minus2
            || !IsZero(m_pps.cb_qp_offset_list)
            || !IsZero(m_pps.cr_qp_offset_list);

        m_pps.extension_flag |= m_pps.range_extension_flag;
    }

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

mfxStatus MfxVideoParam::GetSliceHeader(Task const & task, Task const & prevTask, MFX_ENCODE_CAPS_HEVC const & caps, Slice & s) const
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
                std::max<mfxU16>(nSTR[0] + nSTR[1] + nLTR, task.m_numRefActive[0]),
                std::max<mfxU16>(nSTR[0] + nSTR[1] + nLTR, task.m_numRefActive[1])
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
#if (MFX_VERSION >= 1026)
        mfxExtHEVCParam* rtHEVCParam = ExtBuffer::Get(task.m_ctrl);
        mfxU16 FrameSAO = (rtHEVCParam && rtHEVCParam->SampleAdaptiveOffset) ? rtHEVCParam->SampleAdaptiveOffset : m_ext.HEVCParam.SampleAdaptiveOffset;

        s.sao_luma_flag   = !!(FrameSAO & MFX_SAO_ENABLE_LUMA);
        s.sao_chroma_flag = !!(FrameSAO & MFX_SAO_ENABLE_CHROMA);
#else
        s.sao_luma_flag   = 0;
        s.sao_chroma_flag = 0;
#endif
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
                    std::min<mfxU16>(s.num_ref_idx_l0_active_minus1 + 1, caps.ddi_caps.MaxNum_WeightedPredL0),
                    std::min<mfxU16>(s.num_ref_idx_l1_active_minus1 + 1, caps.ddi_caps.MaxNum_WeightedPredL1)
                };

                for (mfxU16 l = 0; l < 2; l++)
                {
                    for (mfxU16 i = 0; i < sz[l]; i++)
                    {
                        if (pwt->LumaWeightFlag[l][i] && caps.ddi_caps.LumaWeightedPred)
                        {
                            s.pwt[l][i][Y][W] = pwt->Weights[l][i][Y][W];
                            s.pwt[l][i][Y][O] = pwt->Weights[l][i][Y][O];
                        }

                        if (pwt->ChromaWeightFlag[l][i] && caps.ddi_caps.ChromaWeightedPred)
                        {
                            s.pwt[l][i][Cb][W] = pwt->Weights[l][i][Cb][W];
                            s.pwt[l][i][Cb][O] = pwt->Weights[l][i][Cb][O];
                            s.pwt[l][i][Cr][W] = pwt->Weights[l][i][Cr][W];
                            s.pwt[l][i][Cr][O] = pwt->Weights[l][i][Cr][O];
                        }
                    }

                    if (task.m_ldb && Equal(task.m_refPicList[0], task.m_refPicList[1]))
                    {
                        mfxU8 *src = reinterpret_cast<mfxU8*> (&s.pwt[0]);
                        mfxU8 *dst = reinterpret_cast<mfxU8*> (&s.pwt[1]);
                        std::copy(src, src + sizeof(s.pwt[0]), dst);
                    }
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
#if MFX_VERSION < MFX_VERSION_NEXT
        s.deblocking_filter_override_flag = (s.deblocking_filter_disabled_flag != m_pps.deblocking_filter_disabled_flag);
#endif
        if (s.deblocking_filter_disabled_flag != m_pps.deblocking_filter_disabled_flag)
        {
            s.beta_offset_div2 = 0;
            s.tc_offset_div2 = 0;
        }
     }
#if MFX_VERSION >= MFX_VERSION_NEXT
     mfxExtCodingOption3 *ext3 = ExtBuffer::Get(task.m_ctrl);
     if (ext3 && (ext3->DeblockingAlphaTcOffset != m_ext.CO3.DeblockingAlphaTcOffset || ext3->DeblockingBetaOffset != m_ext.CO3.DeblockingBetaOffset)
              && m_pps.deblocking_filter_override_enabled_flag && !s.deblocking_filter_disabled_flag)
     {
         s.beta_offset_div2 = mfxI8(ext3->DeblockingBetaOffset / 2);
         s.tc_offset_div2 = mfxI8(ext3->DeblockingAlphaTcOffset / 2);
     }
    s.deblocking_filter_override_flag = s.deblocking_filter_disabled_flag || s.beta_offset_div2 || s.tc_offset_div2;
#endif
    s.loop_filter_across_slices_enabled_flag = m_pps.loop_filter_across_slices_enabled_flag;

    if (m_pps.tiles_enabled_flag || m_pps.entropy_coding_sync_enabled_flag)
    {
        s.num_entry_point_offsets = 0;

        assert(0 == s.num_entry_point_offsets);
    }
    return MFX_ERR_NONE;
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
        (idrPicDist && (fo + 1) % idrPicDist == 0)) {
        return (MFX_FRAMETYPE_P | MFX_FRAMETYPE_REF); // switch last B frame to P frame
    }


    return (MFX_FRAMETYPE_B);
}

mfxU8 GetDPBIdxByFO(DpbArray const & DPB, mfxU32 fo)
{
    for (mfxU8 i = 0; !isDpbEnd(DPB, i); i++)
    if (DPB[i].m_fo == fo)
        return i;
    return mfxU8(MAX_DPB_SIZE);
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
            //for (st0 = 0; st0 < end && dpb[st0].m_ltr; st0 ++); // st0 is first !ltr

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

    if (end < MAX_DPB_SIZE)
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
                    (void)bSecondField;
                    (void)bBottomField;
                    MFX_SORT_COMMON(RPL[0], numRefActive[0], Abs(DPB[RPL[0][_i]].m_poc - poc) < Abs(DPB[RPL[0][_j]].m_poc - poc));
#elif (HEVCE_FIELD_MODE == 1)
                    (void)bBottomField;
                    MFX_SORT_COMMON(RPL[0], numRefActive[0], (Abs(DPB[RPL[0][_i]].m_poc / 2 - poc / 2) * 2 + ((DPB[RPL[0][_i]].m_secondField == bSecondField) ? 0 : 1)) < (Abs(DPB[RPL[0][_j]].m_poc / 2 - poc / 2) * 2 + ((DPB[RPL[0][_j]].m_secondField == bSecondField) ? 0 : 1)));
#elif (HEVCE_FIELD_MODE == 2)
                    (void)bBottomField;
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

            // reorder STRs to POC descending order for L0
            MFX_SORT_COMMON(RPL[0], numRefActive[0], DPB[RPL[0][_i]].m_poc < DPB[RPL[0][_j]].m_poc);
            // reorder STRs to POC asscending order for L1
            MFX_SORT_COMMON(RPL[1], numRefActive[1], DPB[RPL[1][_i]].m_poc > DPB[RPL[1][_j]].m_poc);

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
            MaxRef[0] = std::min(pLCtrl->NumRefIdxL0Active, MaxRef[0]);

        if (pLCtrl->NumRefIdxL1Active)
            MaxRef[1] = std::min(pLCtrl->NumRefIdxL1Active, MaxRef[1]);

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

        for (mfxU16 i = 0; i < std::min<mfxU16>(l0, NumRefLX[1]); i ++)
            RPL[1][l1++] = RPL[0][i];
        //for (mfxI16 i = l0 - 1; i >= 0 && l1 < NumRefLX[1]; i --)
        //    RPL[1][l1++] = RPL[0][i];
    }
}

}; //namespace MfxHwH265Encode
#endif
