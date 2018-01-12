/******************************************************************************\
Copyright (c) 2017, Intel Corporation
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

#include <cstdlib>

#include "sample_defs.h"
#include "ref_list_manager.h"

namespace HevcRplUtils
{

    struct BasePredicateForRefPicure
    {
        typedef HevcDpbArray             Dpb;

        BasePredicateForRefPicure(Dpb const & dpb)
        : m_dpb(dpb)
        {
        }

        Dpb const & m_dpb;
    };

    struct PocDistanceIsLess : public BasePredicateForRefPicure
    {
        PocDistanceIsLess(Dpb const & dpb, mfxU32 poc)
        : BasePredicateForRefPicure(dpb)
        , m_poc(poc)
        {
        }

        bool operator ()(size_t l, size_t r) const
        {
            return
                (std::abs(m_dpb[l].m_poc - m_poc)) <
                (std::abs(m_dpb[r].m_poc - m_poc));
        }
        mfxU32 m_poc;
    };

    struct PocDistanceIsGreater : public BasePredicateForRefPicure
    {
        PocDistanceIsGreater(Dpb const & dpb, mfxU32 poc)
        : BasePredicateForRefPicure(dpb)
        , m_poc(poc)
        {
        }

        bool operator ()(size_t l, size_t r) const
        {
            return
                (std::abs(m_dpb[l].m_poc - m_poc)) >
                (std::abs(m_dpb[r].m_poc - m_poc));
        }
        mfxU32 m_poc;
    };

    struct InterlacePocDistanceIsLess : public BasePredicateForRefPicure
    {
        InterlacePocDistanceIsLess(Dpb const & dpb, mfxU32 poc, bool IsSecondField, bool IsBottomField)
        : BasePredicateForRefPicure(dpb)
        , m_poc(poc)
        , m_IsSecondField(IsSecondField)
        , m_IsBottomField(IsBottomField)
        {
        }

        bool operator ()(size_t l, size_t r) const
        {
            mfxI32 currFrameNum = GetFrameNum(true, m_poc, m_IsSecondField);
            return
                (std::abs(GetFrameNum(true, m_dpb[l].m_poc, m_dpb[l].m_secondField) - currFrameNum) * 2 + ((m_dpb[l].m_bottomField == m_IsBottomField) ? 0 : 1)) <
                (std::abs(GetFrameNum(true, m_dpb[r].m_poc, m_dpb[r].m_secondField) - currFrameNum) * 2 + ((m_dpb[r].m_bottomField == m_IsBottomField) ? 0 : 1));
        }

        mfxU32 m_poc;
        bool   m_IsSecondField;
        bool   m_IsBottomField;
    };

    struct InterlacePocDistanceIsGreater : public BasePredicateForRefPicure
    {
        InterlacePocDistanceIsGreater(Dpb const & dpb, mfxU32 poc, bool IsSecondField, bool IsBottomField)
        : BasePredicateForRefPicure(dpb)
        , m_poc(poc)
        , m_IsSecondField(IsSecondField)
        , m_IsBottomField(IsBottomField)
        {
        }

        bool operator ()(size_t l, size_t r) const
        {
            mfxI32 currFrameNum = GetFrameNum(true, m_poc, m_IsSecondField);
            return
                (std::abs(GetFrameNum(true, m_dpb[l].m_poc, m_dpb[l].m_secondField) - currFrameNum) * 2 + ((m_dpb[l].m_bottomField == m_IsBottomField) ? 0 : 1)) >
                (std::abs(GetFrameNum(true, m_dpb[r].m_poc, m_dpb[r].m_secondField) - currFrameNum) * 2 + ((m_dpb[r].m_bottomField == m_IsBottomField) ? 0 : 1));
        }

        mfxU32 m_poc;
        bool   m_IsSecondField;
        bool   m_IsBottomField;
    };

    struct RefPocIsGreater : public BasePredicateForRefPicure
    {
        RefPocIsGreater(Dpb const & dpb)
        : BasePredicateForRefPicure(dpb)
        {
        }

        bool operator ()(size_t l, size_t r) const
        {
            return m_dpb[l].m_poc > m_dpb[r].m_poc;
        }
    };

    bool SortByPoc(const HevcDpbFrame & l, const HevcDpbFrame & r)
    {
        return l.m_poc < r.m_poc;
    }

    enum NALU_TYPE
    {
        TRAIL_N = 0,
        TRAIL_R,
        TSA_N,
        TSA_R,
        STSA_N,
        STSA_R,
        RADL_N,
        RADL_R,
        RASL_N,
        RASL_R,
        RSV_VCL_N10,
        RSV_VCL_R11,
        RSV_VCL_N12,
        RSV_VCL_R13,
        RSV_VCL_N14,
        RSV_VCL_R15,
        BLA_W_LP,
        BLA_W_RADL,
        BLA_N_LP,
        IDR_W_RADL,
        IDR_N_LP,
        CRA_NUT,
        RSV_IRAP_VCL22,
        RSV_IRAP_VCL23,
        RSV_VCL24,
        RSV_VCL25,
        RSV_VCL26,
        RSV_VCL27,
        RSV_VCL28,
        RSV_VCL29,
        RSV_VCL30,
        RSV_VCL31,
        VPS_NUT,
        SPS_NUT,
        PPS_NUT,
        AUD_NUT,
        EOS_NUT,
        EOB_NUT,
        FD_NUT,
        PREFIX_SEI_NUT,
        SUFFIX_SEI_NUT,
        RSV_NVCL41,
        RSV_NVCL42,
        RSV_NVCL43,
        RSV_NVCL44,
        RSV_NVCL45,
        RSV_NVCL46,
        RSV_NVCL47,
        UNSPEC48,
        UNSPEC49,
        UNSPEC50,
        UNSPEC51,
        UNSPEC52,
        UNSPEC53,
        UNSPEC54,
        UNSPEC55,
        UNSPEC56,
        UNSPEC57,
        UNSPEC58,
        UNSPEC59,
        UNSPEC60,
        UNSPEC61,
        UNSPEC62,
        UNSPEC63,
        num_NALU_TYPE
    };

    template<class T, class A> void Insert(A& _to, mfxU32 _where, T const & _what)
    {
        if (_where + 1 >= (sizeof(_to) / sizeof(_to[0])))
            throw mfxError(MFX_ERR_UNDEFINED_BEHAVIOR, "Wrong ranges!");

        memmove(&_to[_where + 1], &_to[_where], sizeof(_to) - (_where + 1) * sizeof(_to[0]));
        _to[_where] = _what;
    }

    template<class A> void Remove(A& _from, mfxU32 _where, mfxU32 _num = 1)
    {
        const mfxU32 S0 = sizeof(_from[0]);
        const mfxU32 S = sizeof(_from);
        const mfxU32 N = S / S0;

        if (_where >= N && _num <= (N - _where))
            throw mfxError(MFX_ERR_UNDEFINED_BEHAVIOR, "Wrong ranges!");

        if (_where + _num < N)
            memmove(&_from[_where], &_from[_where + _num], S - ((_where + _num) * S0));

        memset(&_from[N - _num], IDX_INVALID, S0 * _num);
    }

    inline bool isValid(HevcDpbFrame const & frame) { return IDX_INVALID != frame.m_idxRec; }
    inline bool isDpbEnd(HevcDpbArray const & dpb, mfxU32 idx) { return idx >= MAX_DPB_SIZE || !isValid(dpb[idx]); }

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

    template <class T> mfxU32 BPyrReorder(std::vector<T> brefs)
    {
        mfxU32 num = (mfxU32)brefs.size();
        if (brefs[0]->m_bpo == (mfxU32)MFX_FRAMEORDER_UNKNOWN)
        {
            bool bRef = false;

            for(mfxU32 i = 0; i < (mfxU32)brefs.size(); i++)
            {
                brefs[i]->m_bpo = GetBiFrameLocation(i,num, bRef, brefs[i]->m_level);
                if (bRef)
                    brefs[i]->m_frameType |= MFX_FRAMETYPE_REF;
            }
        }
        mfxU32 minBPO =(mfxU32)MFX_FRAMEORDER_UNKNOWN;
        mfxU32 ind = 0;
        for(mfxU32 i = 0; i < (mfxU32)brefs.size(); i++)
        {
            if (brefs[i]->m_bpo < minBPO)
            {
                ind = i;
                minBPO = brefs[i]->m_bpo;
            }
        }
        return ind;
    }

    mfxU32 CountL1(HevcDpbArray const & dpb, mfxI32 poc)
    {
        mfxU32 c = 0;
        for (mfxU32 i = 0; !isDpbEnd(dpb, i); i ++)
            c += dpb[i].m_poc > poc;
        return c;
    }

    mfxU8 GetDPBIdxByFO(HevcDpbArray const & DPB, mfxU32 fo)
    {
        for (mfxU8 i = 0; !isDpbEnd(DPB, i); i++)
        if (DPB[i].m_fo == fo)
            return i;
        return mfxU8(MAX_DPB_SIZE);
    }

    bool isBPyramid(MfxVideoParamsWrapper const & par)
    {
        mfxExtCodingOption2 * CO2 = par;
        return CO2 ? CO2->BRefType == MFX_B_REF_PYRAMID : false;
    }

    bool isPPyramid(MfxVideoParamsWrapper const & par)
    {
        mfxExtCodingOption3 * CO3 = par;
        return CO3 ? CO3->PRefType == MFX_P_REF_PYRAMID : false;
    }

    template<class T> T FindFrameToEncode(
        MfxVideoParamsWrapper const & par,
        HevcDpbArray const & dpb,
        T begin,
        T end,
        bool flush,
        bool bFields)
    {
        T top  = begin;
        T b0 = end; // 1st non-ref B with L1 > 0
        std::vector<T> brefs;

        while ( top != end && (top->m_frameType & MFX_FRAMETYPE_B))
        {
            if (CountL1(dpb, top->m_poc) && (!top->m_secondField))
            {
                if (isBPyramid(par))
                    brefs.push_back(top);
                else if (top->m_frameType & MFX_FRAMETYPE_REF)
                {
                    if (b0 == end || (top->m_poc - b0->m_poc < bFields + 2))
                        return top;
                }
                else if (b0 == end)
                    b0 = top;
            }
            top ++;
        }

        if (!brefs.empty())
        {
            return brefs[BPyrReorder(brefs)];
        }

        if (b0 != end)
            return b0;

        if (flush && top == end && begin != end)
        {
            top --;
            top->m_frameType = MFX_FRAMETYPE_P | MFX_FRAMETYPE_REF;
            if (top->m_secondField && top != begin)
            {
                top--;
                top->m_frameType = MFX_FRAMETYPE_P | MFX_FRAMETYPE_REF;
            }
        }

        return top;
    }

    void InitDPB(
        HevcTask &        task,
        HevcTask const &  prevTask)
    {
        if (task.m_poc > task.m_lastRAP &&
            prevTask.m_poc <= prevTask.m_lastRAP) // 1st TRAIL
        {
            Fill(task.m_dpb[TASK_DPB_ACTIVE], IDX_INVALID);

            for (mfxU8 i = 0, j = 0; !isDpbEnd(prevTask.m_dpb[TASK_DPB_AFTER], i); i++)
            {
                const HevcDpbFrame& ref = prevTask.m_dpb[TASK_DPB_AFTER][i];

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
            HevcDpbArray& dpb = task.m_dpb[TASK_DPB_ACTIVE];

            for (mfxI16 i = 0; !isDpbEnd(dpb, i); i++)
                if (dpb[i].m_tid > 0 && dpb[i].m_tid >= task.m_tid)
                    Remove(dpb, i--);
        }
    }

    void UpdateDPB(
        MfxVideoParamsWrapper const & par,
        HevcDpbFrame const & task,
        HevcDpbArray & dpb)
    {
        mfxU16 end = 0; // DPB end
        mfxU16 st0 = 0; // first ST ref in DPB
        static const mfxU16 maxNumRefL0 = 3;

        while (!isDpbEnd(dpb, end)) end ++;
        for (st0 = 0; st0 < end && dpb[st0].m_ltr; st0++);

        // frames stored in DPB in POC ascending order,
        // LTRs before STRs (use LTR-candidate as STR as long as it possible)
        std::sort(dpb, dpb + st0, SortByPoc);
        std::sort(dpb + st0, dpb + (mfxU16)(end - st0), SortByPoc);

        // sliding window over STRs
        if (end && end == par.mfx.NumRefFrame)
        {
            if (isPPyramid(par) && st0 == 0)
            {
                if (isField(par) && (GetFrameNum(true, dpb[1].m_poc, dpb[1].m_secondField) != GetFrameNum(true, dpb[0].m_poc, dpb[0].m_secondField)))
                    st0 = 0;
                else
                    for (st0 = 1; ((GetFrameNum(isField(par), dpb[st0].m_poc, dpb[st0].m_secondField) - (GetFrameNum(isField(par), dpb[0].m_poc, dpb[0].m_secondField))) % maxNumRefL0 ) == 0 && st0 < end; st0++);
            }
            else
            {
                for (st0 = 0; st0 < end && dpb[st0].m_ltr; st0++);
            }

            Remove(dpb, st0 == end ? 0 : st0);
            end --;

        }

        if (end < MAX_DPB_SIZE)
            dpb[end++] = task;
        else
            throw mfxError(MFX_ERR_UNDEFINED_BEHAVIOR, "DPB overflow, no space for new frame!");
    }

    mfxU8 GetFrameType(
        MfxVideoParamsWrapper const & video,
        mfxU32                frameOrder)
    {
        mfxU32 gopOptFlag = video.mfx.GopOptFlag;
        mfxU32 gopPicSize = video.mfx.GopPicSize;
        mfxU32 gopRefDist = video.mfx.GopRefDist;
        mfxU32 idrPicDist = gopPicSize * (video.mfx.IdrInterval);

        if (gopPicSize == 0xffff) //infinite GOP
            idrPicDist = gopPicSize = 0xffffffff;


        bool bField = isField(video);
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

        if (((fo + 1) % gopPicSize == 0 && (gopOptFlag & MFX_GOP_CLOSED)) ||
            (idrPicDist && (fo + 1) % idrPicDist == 0))
            return (MFX_FRAMETYPE_P | MFX_FRAMETYPE_REF); // switch last B frame to P frame


        return MFX_FRAMETYPE_B;
    }

    bool isLTR(
        HevcDpbArray const & dpb,
        mfxU32 LTRInterval,
        mfxI32 poc)
    {
        mfxI32 LTRCandidate = dpb[0].m_poc;

        for (mfxU16 i = 1; !isDpbEnd(dpb, i); i++)
        {
            if (dpb[i].m_poc > LTRCandidate
                && dpb[i].m_poc - LTRCandidate >= (mfxI32)LTRInterval)
            {
                LTRCandidate = dpb[i].m_poc;
                break;
            }
        }

        return (poc == LTRCandidate) || (LTRCandidate == 0 && poc >= (mfxI32)LTRInterval);
    }

    void ConstructRPL(
        MfxVideoParamsWrapper const & par,
        HevcDpbArray const & DPB,
        bool   isB,
        mfxI32 poc,
        mfxU8  tid,
        bool   bSecondField,
        bool   bBottomField,
        mfxU8(&RPL)[2][MAX_DPB_SIZE],
        mfxU8(&numRefActive)[2])
    {
        mfxU8 NumRefLX[2] = { numRefActive[0], numRefActive[1] };
        mfxU8& l0 = numRefActive[0];
        mfxU8& l1 = numRefActive[1];
        mfxU8 LTR[MAX_DPB_SIZE] = {};
        mfxU8 nLTR = 0;
        mfxU8 NumStRefL0 = (mfxU8)(NumRefLX[0]);

        const mfxU32 LTRInterval = 0; // currently not enabled

        l0 = l1 = 0;

        for (mfxU8 i = 0; !isDpbEnd(DPB, i); i++)
        {
            if (DPB[i].m_tid > tid)
                continue;

            if (poc > DPB[i].m_poc)
            {
                if (DPB[i].m_ltr || (LTRInterval && isLTR(DPB, LTRInterval, DPB[i].m_poc)))
                    LTR[nLTR++] = i;
                else
                    RPL[0][l0++] = i;
            }
            else if (isB)
                RPL[1][l1++] = i;
        }

        NumStRefL0 -= !!nLTR;

        if (l0 > NumStRefL0)
        {
            if (isField(par))
            {
                std::sort(&RPL[0][0], &RPL[0][numRefActive[0]], InterlacePocDistanceIsLess(DPB, poc, bSecondField, bBottomField));
            }
            else
            {
                std::sort(&RPL[0][0], &RPL[0][numRefActive[0]], PocDistanceIsLess(DPB, poc));
            }
            if (isPPyramid(par))
            {
                while (l0 > NumStRefL0)
                {
                    mfxI32 i;
                    mfxI32 k = isField(par) ? 2 : 1;

                    // NumRefLX[0] used here as distance between "strong" STR, not NumRefActive for current frame
                    for (i = 0; (i < l0) && (((DPB[RPL[0][0]].m_poc/k - DPB[RPL[0][i]].m_poc/k) % NumRefLX[0]) == 0); i++);

                    Remove(RPL[0], (i >= l0 - 1) ? 0 : i);
                    l0--;
                }
            }
            else
            {
                Remove(RPL[0], (LTRInterval && !nLTR && l0 > 1), l0 - NumStRefL0);
                l0 = NumStRefL0;
            }
        }
        if (l1 > NumRefLX[1])
        {
            if (isField(par))
            {
                std::sort(&RPL[1][0], &RPL[1][numRefActive[1]], InterlacePocDistanceIsLess(DPB, poc, bSecondField, bBottomField));
            }
            else
            {
                std::sort(&RPL[1][0], &RPL[1][numRefActive[1]], PocDistanceIsLess(DPB, poc));
            }
            Remove(RPL[1], NumRefLX[1], l1 - NumRefLX[1]);
            l1 = (mfxU8)NumRefLX[1];
        }

        // reorder STRs to POC descending order
        for (mfxU8 lx = 0; lx < 2; lx++)
            std::sort(&RPL[lx][0], &RPL[lx][numRefActive[lx]], RefPocIsGreater(DPB));

        if (nLTR)
        {
            std::sort(LTR, LTR + nLTR, std::less<mfxU8>());
            // use LTR as 2nd reference
            Insert(RPL[0], !!l0, LTR[0]);
            l0++;

            for (mfxU16 i = 1; i < nLTR && l0 < NumRefLX[0]; i++, l0++)
                Insert(RPL[0], l0, LTR[i]);
        }

        assert(l0 > 0);

        if (isB && !l1 && l0)
            RPL[1][l1++] = RPL[0][l0 - 1];

        if (!isB)
        {
            l1 = 0; //ignore l1 != l0

            for (mfxU16 i = 0; i < std::min<mfxU16>(l0, NumRefLX[1]); i++)
                RPL[1][l1++] = RPL[0][i];
        }
    }

    mfxU8 GetSHNUT(HevcTask const & task, bool RAPIntra)
    {
        const bool isI   = !!(task.m_frameType & MFX_FRAMETYPE_I);
        const bool isRef = !!(task.m_frameType & MFX_FRAMETYPE_REF);
        const bool isIDR = !!(task.m_frameType & MFX_FRAMETYPE_IDR);

        if (isIDR)
            return IDR_W_RADL;

        if (isI && RAPIntra)
        {
            const HevcDpbArray& DPB = task.m_dpb[TASK_DPB_AFTER];
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
            return isRef ? TRAIL_R : TRAIL_N;
        }

        if (isRef)
            return RASL_R;
        return RASL_N;
    }
}

using namespace HevcRplUtils;

HevcTask* EncodeOrderControl::ReorderFrame(mfxFrameSurface1 * surface)
{
    HevcTask* free_task = 0;

    if (surface)
    {
        if (!m_free.empty())
        {
            free_task = &m_free.front();
            m_reordering.splice(m_reordering.end(), m_free, m_free.begin());

            msdk_atomic_inc16((volatile mfxU16*)&surface->Data.Locked);
        }
        else
        {
            assert(!"No free task in pool");
        }
    }

    if (free_task)
    {
        free_task->Reset();

        free_task->m_surf = surface;
        free_task->m_frameType = GetFrameType(m_par, m_frameOrder - m_lastIDR);

        if (free_task->m_frameType & MFX_FRAMETYPE_IDR)
            m_lastIDR = m_frameOrder;

        free_task->m_poc = m_frameOrder - m_lastIDR;
        free_task->m_fo =  m_frameOrder;
        free_task->m_bpo = (mfxU32)MFX_FRAMEORDER_UNKNOWN;
        free_task->m_secondField = isField(m_par) ? (!!(free_task->m_fo & 1)) : false;
        free_task->m_idxRec = m_frameOrder & 0x7f; // Workaround to get unique idx and != IDX_INVALID (0xff)
        free_task->m_bottomField = false;
        if (isField(m_par))
        {
            free_task->m_bottomField = (free_task->m_surf->Info.PicStruct & (MFX_PICSTRUCT_FIELD_TFF | MFX_PICSTRUCT_FIELD_BFF)) == 0 ?
                (isBFF(m_par) != free_task->m_secondField) :
                (free_task->m_surf->Info.PicStruct & MFX_PICSTRUCT_FIELD_BFF) != 0;
        }

        m_frameOrder ++;
    }

    HevcTask* task_to_encode = 0;
    {
        bool flush = !surface;
        TaskList::iterator begin = m_reordering.begin();
        TaskList::iterator end   = m_reordering.begin();

        while (end != m_reordering.end())
        {
            if ((end != begin) && (end->m_frameType & MFX_FRAMETYPE_IDR))
            {
                flush = true;
                break;
            }
            end++;
        }

        if (isField(m_par) && m_lastFieldInfo.bFirstField())
        {
            while (begin != end && !m_lastFieldInfo.isCorrespondSecondField(&*begin))
                    begin++;

            if (begin != end)
            {
                m_lastFieldInfo.CorrectTaskInfo(&*begin);
                return &*begin;
            }
            begin = m_reordering.begin();
        }
        if (isField(m_par))
        {
            if (begin != end && (begin->m_frameType & MFX_FRAMETYPE_IDR) != 0 && begin->m_secondField)
                return &*begin;
        }

        TaskList::iterator top = FindFrameToEncode(m_par, m_lastTask.m_dpb[TASK_DPB_AFTER], begin, end, flush, isField(m_par));
        if (top != end)
            task_to_encode = &*top;
    }

    if (task_to_encode)
    {
        for (TaskList::iterator it = m_reordering.begin(); it != m_reordering.end(); it ++)
        {
            if (task_to_encode == &*it)
            {
                m_encoding.splice(m_encoding.end(), m_reordering, it);
                break;
            }
        }

        task_to_encode->m_surf->Data.FrameOrder = task_to_encode->m_fo;

        // get a free pointer for a downscaled surface which will be filled in PreENC class
        std::vector<mfxFrameSurface1*>::iterator it = std::find(m_ds_pSurf.begin(), m_ds_pSurf.end(), (mfxFrameSurface1*)NULL);
        task_to_encode->m_ds_surf = it != m_ds_pSurf.end() ? &(*it) : NULL;

        HevcTask & task = *task_to_encode;
        task.m_lastIPoc = m_lastTask.m_lastIPoc;
        task.m_lastRAP  = m_lastTask.m_lastRAP;

        InitDPB(task, m_lastTask);

        // update dpb
        {
            if (task.m_frameType & MFX_FRAMETYPE_IDR)
                Fill(task.m_dpb[TASK_DPB_AFTER], IDX_INVALID);
            else
                Copy(task.m_dpb[TASK_DPB_AFTER], task.m_dpb[TASK_DPB_ACTIVE]);

            if (task.m_frameType & MFX_FRAMETYPE_REF)
            {
                if (task.m_frameType & MFX_FRAMETYPE_I)
                    task.m_lastIPoc = task.m_poc;

                UpdateDPB(m_par, task, task.m_dpb[TASK_DPB_AFTER]);
            }
        }

        task.m_shNUT = GetSHNUT(task, !isField(m_par));
        if (task.m_shNUT == CRA_NUT || task.m_shNUT == IDR_W_RADL)
            task.m_lastRAP = task.m_poc;

        m_lastTask = task;
    }

    return task_to_encode;
}

void EncodeOrderControl::ConstructRPL(HevcTask & task)
{
    MSDK_ZERO_MEMORY(task.m_numRefActive);
    Fill(task.m_refPicList, IDX_INVALID);

    mfxExtCodingOption3 * CO3 = m_par;
    if (!CO3)
        throw mfxError(MFX_ERR_NULL_PTR, "mfxExtCodingOption3 is not defined in ConstructRPL");

    if (task.m_frameType & MFX_FRAMETYPE_B)
    {
        mfxI32 layer = isBPyramid(m_par) ? Clip3<mfxI32>(0, 7, task.m_level - 1) : 0;

        task.m_numRefActive[0] = (mfxU8)CO3->NumRefActiveBL0[layer];
        task.m_numRefActive[1] = (mfxU8)CO3->NumRefActiveBL1[layer];
    }

    if (task.m_frameType & MFX_FRAMETYPE_P)
    {
        task.m_numRefActive[0] = (mfxU8)CO3->NumRefActiveP[0];
    }

    if ( !(task.m_frameType & MFX_FRAMETYPE_I) )
    {
        HevcRplUtils::ConstructRPL(m_par, task.m_dpb[TASK_DPB_ACTIVE], !!(task.m_frameType & MFX_FRAMETYPE_B),
                                   task.m_poc, task.m_tid, task.m_secondField, task.m_bottomField, task.m_refPicList, task.m_numRefActive);
    }
}

void Release(PreENCOutput & statistic)
{
    if (statistic.m_mv)
        msdk_atomic_dec16((volatile mfxU16*)&statistic.m_mv->m_locked);
    statistic.m_mv = NULL;

    if (statistic.m_mb)
        msdk_atomic_dec16((volatile mfxU16*)&statistic.m_mb->m_locked);
    statistic.m_mb = NULL;
}

void EncodeOrderControl::ReleaseResources(HevcTask & task)
{
    if (m_lockRawRef)
    {
        if (!(task.m_frameType & MFX_FRAMETYPE_REF))
        {
            if (task.m_surf)
            {
                msdk_atomic_dec16((volatile mfxU16*)&task.m_surf->Data.Locked);
                task.m_surf = NULL;
            }
            if (task.m_ds_surf && *task.m_ds_surf)
            {
                msdk_atomic_dec16((volatile mfxU16*)&(*task.m_ds_surf)->Data.Locked);
                *task.m_ds_surf = NULL;
            }
        }

        for (mfxU16 i = 0, j = 0; !isDpbEnd(task.m_dpb[TASK_DPB_BEFORE], i); i ++)
        {
            for (j = 0; !isDpbEnd(task.m_dpb[TASK_DPB_AFTER], j); j ++)
                if (task.m_dpb[TASK_DPB_BEFORE][i].m_idxRec == task.m_dpb[TASK_DPB_AFTER][j].m_idxRec)
                    break;

            if (isDpbEnd(task.m_dpb[TASK_DPB_AFTER], j))
            {
                if (task.m_dpb[TASK_DPB_BEFORE][i].m_surf)
                {
                    msdk_atomic_dec16((volatile mfxU16*)&task.m_dpb[TASK_DPB_BEFORE][i].m_surf->Data.Locked);
                    task.m_dpb[TASK_DPB_BEFORE][i].m_surf = NULL;
                }
                if (task.m_dpb[TASK_DPB_BEFORE][i].m_ds_surf && *task.m_dpb[TASK_DPB_BEFORE][i].m_ds_surf)
                {
                    msdk_atomic_dec16((volatile mfxU16*)&(*task.m_dpb[TASK_DPB_BEFORE][i].m_ds_surf)->Data.Locked);
                    *task.m_dpb[TASK_DPB_BEFORE][i].m_ds_surf = NULL;
                }
            }
        }
    }
    else
    {
        if (task.m_surf)
        {
            // NB! Here we release an input surface, however even in non-m_lockRawRef mode
            // dpb still contains (but doesn't use) pointers to this task.m_surf object.
            // Need to consider cleaning up DPB.
            msdk_atomic_dec16((volatile mfxU16*)&task.m_surf->Data.Locked);
            task.m_surf = NULL;
        }
    }

    std::for_each(task.m_preEncOutput.begin(), task.m_preEncOutput.end(), Release);
    task.m_preEncOutput.clear();
}
