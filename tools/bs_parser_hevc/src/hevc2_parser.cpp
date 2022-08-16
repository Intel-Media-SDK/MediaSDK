// Copyright (c) 2018-2019 Intel Corporation
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

#include "hevc2_parser.h"

using namespace BS_HEVC2;
using namespace BsReader2;
using namespace BsThread;

//Scheduler Parser::m_thread;

Parser::Parser(Bs32u mode)
    : SDParser((mode & PARSE_SSD_TC) == PARSE_SSD_TC)
    , m_mode(mode)
    , m_au(0)
    , m_bNewSequence(true)
{
    SetTraceLevel(TRACE_DEFAULT);
    SetEmulation(true);
    SetZero(true);
    m_lastNALU.p = 0;
    m_lastNALU.prealloc = false;
    std::fill_n(m_vps, 16, nullptr);
    std::fill_n(m_sps, 16, nullptr);
    std::fill_n(m_pps, 64, nullptr);
    memset(&m_prevPOC, 0, sizeof(m_prevPOC));
    m_pAllocator = &(BS_MEM::Allocator&)*this;

    m_asyncAUMax = 0;
    m_asyncAUCnt = 0;

    Bs32u hwThreads = 0;

    if (m_mode & ASYNC)
    {
        hwThreads    = std::max(1u, std::thread::hardware_concurrency());
        m_asyncAUMax = hwThreads * 2;
    }

    if (m_mode & PARALLEL_SD)
    {
#ifdef __BS_TRACE__
        Bs32u id = 0;
#endif
        m_sdt.resize(hwThreads);

        for (auto& sdt : m_sdt)
        {
#ifdef __BS_TRACE__
            char fname[] = "X_bs_thread.log";
            fname[0] = '0' + (id % 10);
            id++;

#if !defined(__GNUC__) && !defined(__clang__)
  #pragma warning(disable:4996)
#endif
            FILE* log = fopen(fname, "w");
            sdt.p.SetLog(log);
#endif

            sdt.locked = 0;
            sdt.p.m_pAllocator = &(BS_MEM::Allocator&)*this;
            sdt.p.SetEmulation(false);
            sdt.p.SetTraceLevel(TRACE_DEFAULT);
        }
    }

    if (m_mode & (PARALLEL_SD | PARALLEL_TILES))
        hwThreads++;

    if (hwThreads)
        m_thread.Init(hwThreads);

    m_auErr = BS_ERR_NONE;
    m_prevSP = 0;
}

Parser::~Parser()
{
    if (m_mode & ASYNC)
    {
        for (auto t : m_spAuToId)
            m_thread.Sync(t.second.AUDone, -1);
        m_thread.Close();
    }
}

BSErr Parser::open(const char* file_name)
{
    if (!Open(file_name))
        return BS_ERR_INVALID_PARAMS;

    Reset(this);

    return BS_ERR_NONE;
}

BSErr Parser::Lock(void* p)
{
    try
    {
        lock(p);
    }
    catch (std::bad_alloc())
    {
        return BS_ERR_MEM_ALLOC;
    }
    return BS_ERR_NONE;
}

BSErr Parser::Unlock(void* p)
{
    try
    {
        unlock(p);
    }
    catch (std::bad_alloc())
    {
        return BS_ERR_MEM_ALLOC;
    }
    return BS_ERR_NONE;
}

void Parser::set_trace_level(Bs32u level)
{
    SetTraceLevel(level);

    for (auto& t : m_sdt)
        t.p.SetTraceLevel(level);
}

inline bool isSuffix(NALU& nalu, bool nextBit){
    return (isSlice(nalu) && !nextBit)
        || (nalu.nal_unit_type == SUFFIX_SEI_NUT)
        || (nalu.nal_unit_type == FD_NUT)
        || (nalu.nal_unit_type == EOS_NUT)
        || (nalu.nal_unit_type == EOB_NUT)
        || ((nalu.nal_unit_type >= RSV_NVCL45) && (nalu.nal_unit_type <= RSV_NVCL47))
        || ((nalu.nal_unit_type >= UNSPEC56) && (nalu.nal_unit_type <= UNSPEC63));
}

struct ParallelAUPar
{
    Parser* p;
    NALU* pAU;
};

BsThread::State Parser::ParallelAU(void* par, unsigned int)
{
    Parser* p = ((ParallelAUPar*)par)->p;
    NALU* pAU = ((ParallelAUPar*)par)->pAU;

    if (p->m_lastNALU.p)
    {
        *pAU = *p->m_lastNALU.p;

        if (p->m_lastNALU.complete)
            p->bound(p->m_lastNALU.p, pAU);

        p->free(p->m_lastNALU.p);
    }
    else
        p->m_lastNALU.prealloc = true;

    p->m_lastNALU.p = pAU;

    p->m_auErr = p->ParseNextAu(pAU);

    p->free(par);

    if (p->m_auErr)
    {
        p->m_spAuToId[pAU].sts = p->m_auErr;
        return FAILED;
    }

    p->m_spAuToId[pAU].sts = BS_ERR_NONE;

    return DONE;
}

BsThread::State Parser::AUDone(void* par, unsigned int n)
{
    (void)n;
    SyncPoint* sp = (SyncPoint*)par;

    //Redundunt sync, must be resolved through dependencies
    /*if (n == 0)
        m_thread.Sync(sp->AU, 0);*/

    if (sp->sts)
        return FAILED;

    for (auto& t : sp->SD)
    {
        //Redundunt sync, must be resolved through dependencies
        /*if (!Ready(t.state))
            t.state = m_thread.Sync(t.sp, 0);*/

        if (!Ready(t.state))
        {
            //should never be reached if all dependencies are set correctly
            return WAITING;
        }

        if (t.state != DONE)
        {
            sp->sts = BS_ERR_UNKNOWN;
            return t.state; // FAILED/LOST
        }
    }

    //merge tiles
    NALU* base = 0;
    CTU* lastCTU = 0;
    for (auto& t : sp->SD)
    {
        NALU* cur = t.Slice;

        //forbidden_zero_bit == 0 is indicator for "real" slice
        //otherwise slice was constructed by tile threading and must be merged with previous "real"
        if (!t.Slice->forbidden_zero_bit)
        {
            if (base)
                sp->pAllocator->unlock(base);

            base = cur;
            lastCTU = &base->slice->ctu[base->slice->NumCTU - 1];
        }
        else if (base)
        {
            base->NumBytesInRbsp += cur->NumBytesInRbsp;

            if (!base->slice->ctu)
                base->slice->ctu = cur->slice->ctu;
            else
                lastCTU->Next = cur->slice->ctu;

            base->slice->NumCTU += cur->slice->NumCTU;

            lastCTU = &cur->slice->ctu[cur->slice->NumCTU - 1];

            sp->pAllocator->bound(cur->slice->ctu, base->slice);
            sp->pAllocator->free(t.Slice);
        }
        else
        {
            //1st slice must always be "real" unless things went really ugly
            sp->sts = BS_ERR_UNKNOWN;
            return FAILED;
        }
    }

    if (base)
        sp->pAllocator->unlock(base);

    return DONE;
}

struct SDTWaitPar
{
    Parser* p;
    SDThread* pSDT;
};

BsThread::State Parser::SDTWait(void* _par, unsigned int)
{
    SDTWaitPar* par = (SDTWaitPar*)_par;

    for (auto& t : par->p->m_sdt)
    {
        std::unique_lock<std::mutex> lockSDT(t.mtx);

        if (!t.locked)
        {
            t.locked++;
            par->pSDT = &t;
            return DONE;
        }
    }

    par->pSDT = 0;

    return WAITING;
}

BSErr Parser::sync(NALU* pAU)
{
    std::unique_lock<std::mutex> lock(m_mtx);

    if (!!m_spAuToId.count(pAU))
    {
        m_asyncAUCnt--;

        lock.unlock();

        if (DONE != m_thread.Sync(m_spAuToId[pAU].AUDone, -1))
            return m_spAuToId[pAU].sts;

        lock.lock();

        m_spAuToId.erase(pAU);
        return BS_ERR_NONE;
    }

    return BS_ERR_INVALID_PARAMS;
}

BSErr Parser::parse_next_au(NALU*& pAU)
{
    if (m_mode & PARALLEL_AU)
    {
        if (m_asyncAUCnt >= m_asyncAUMax)
        {
            NALU* spAU = 0;
            {
                std::unique_lock<std::mutex> lockMap(m_mtx);
                auto sp0 = std::min_element(m_spAuToId.begin(), m_spAuToId.end(),
                    [] (const std::pair<NALU*, SyncPoint>& p0,
                        const std::pair<NALU*, SyncPoint>& p1) { return p0.second.AU < p1.second.AU; } );
                spAU = sp0->first;
            }

            BSErr sts = sync(spAU);

            if (sts)
                return sts;
        }

        m_asyncAUCnt++;
        return ParseNextAuSubmit(pAU);
    }

    BSErr sts = ParseNextAu(pAU);

    if (!sts && (m_mode & PARALLEL_SD))
    {
        std::unique_lock<std::mutex> lockMap(m_mtx);
        auto& sp = m_spAuToId[pAU];

        if (!sp.SD.empty())
        {
            std::vector<BsThread::SyncPoint> dep;

            for (auto sd : sp.SD)
                dep.push_back(sd.sp);

            sp.sts = BS_ERR_NONE;
            sp.AU = sp.SD.front().sp;
            sp.pAllocator = this;

            auto spAUDone = sp.AUDone = m_thread.Submit(AUDone, &m_spAuToId[pAU], 0, (Bs32u)dep.size(), &dep[0]);

            lockMap.unlock();

            if (DONE != m_thread.Sync(spAUDone, -1))
                return BS_ERR_UNKNOWN;

            lockMap.lock();
        }

        m_spAuToId.erase(pAU);
    }

    return sts;
}

BSErr Parser::ParseNextAuSubmit(NALU*& pAU)
{
    std::unique_lock<std::mutex> lock(m_mtx);
    BsThread::State st = QUEUED;
    auto sp0 = std::min_element(m_spAuToId.begin(), m_spAuToId.end(),
        [] (const std::pair<NALU*, SyncPoint>& p0,
            const std::pair<NALU*, SyncPoint>& p1) { return p0.second.AU < p1.second.AU; } );
    bool AUSubmitted = false;

    if (!m_spAuToId.empty() && !(m_mode & PARALLEL_SD))
        st = m_thread.Sync(sp0->second.AU, 0);

    if (st == FAILED || st == LOST)
    {
        BSErr sts = m_auErr;
        m_auErr = BS_ERR_NONE;
        return sts;
    }

    auto pPar = alloc<ParallelAUPar>();
    pAU = alloc<NALU>();

    if (!!m_spAuToId.count(pAU))
    {
        m_thread.Abort(m_spAuToId[pAU].AUDone, -1);
        m_spAuToId.erase(pAU);
    }

    pPar->p   = this;
    pPar->pAU = pAU;

    for (Bs32u i = 0; ; i++)
    {
        try
        {
            if (!AUSubmitted)
            {
                m_spAuToId[pAU].AU = m_thread.Submit(ParallelAU, pPar, 0, !!m_prevSP, m_prevSP ? &m_spAuToId[m_prevSP].AU : 0);
                AUSubmitted = true;
            }
            if (m_mode & PARALLEL_SD)
            {
                m_spAuToId[pAU].AUDone = m_thread.Submit(AUDone, &m_spAuToId[pAU], 1, 1, &m_spAuToId[pAU].AU);
                m_thread.Detach(m_spAuToId[pAU].AU);
            }
            else
                m_spAuToId[pAU].AUDone = m_spAuToId[pAU].AU;
        }
        catch (TaskQueueOverflow&)
        {
            m_thread.WaitForAny(-1);

            if (i > m_asyncAUMax)
                return BS_ERR_UNKNOWN;

            continue;
        }

        break;
    }
    m_prevSP = pAU;

    return BS_ERR_NONE;
}

struct AutoUnlockSDT
{
    SDThread*& m_pSDT;
    AutoUnlockSDT(SDThread*& pSDT) : m_pSDT(pSDT) {}
    ~AutoUnlockSDT()
    {
        if (m_pSDT)
        {
            std::unique_lock<std::mutex> lock(m_pSDT->mtx);
            m_pSDT->locked--;
        }
    }
};

NALU* Parser::GetColPic(Slice& slice)
{
    if (slice.temporal_mvp_enabled_flag && slice.type != I)
    {
        if (slice.collocated_from_l0_flag)
            return m_ColPic[slice.L0[slice.collocated_ref_idx].POC];
        else
            return m_ColPic[slice.L1[slice.collocated_ref_idx].POC];
    }
    return 0;
}

void Parser::UpdateColPics(NALU* AU, Slice& slice)
{
    std::list<Bs32s> uselessPics;

    for (auto& c : m_ColPic)
    {
        if (   /*(NewPicture && slice.temporal_mvp_enabled_flag == 0)
            ||*/ m_DPBafter[c.first] == false)
        {
            unlock(c.second);
            uselessPics.push_back(c.first);
        }
    }

    for (auto& c : uselessPics)
    {
        m_ColPic.erase(c);
    }

    if (m_DPBafter[slice.POC] && m_ColPic[slice.POC] != AU)
    {
        if (m_ColPic[slice.POC])
            unlock(m_ColPic[slice.POC]);

        lock(AU);
        m_ColPic[slice.POC] = AU;
    }
}

BSErr Parser::ParseNextAu(NALU*& pAU)
{
    NALU* firstNALU = m_lastNALU.p;
    bool  picStarted = m_lastNALU.p && !m_lastNALU.prealloc && isSlice(*m_lastNALU.p);
    Bs32u auSize = 0;
    SDThread* pSDT = 0;
    std::vector<BsThread::SyncPoint> sliceDep;
    AutoUnlockSDT _au(pSDT);
    m_activeSPS = 0;

    try
    {
        for (;; auSize++)
        {
            NALUDesc& nalu = m_lastNALU;

            if (!nalu.p || nalu.complete || nalu.prealloc)
            {
                bool LongStartCode = NextStartCode(false);

                SetEmuBytes(0);

                if (!nalu.prealloc)
                {
                    if (nalu.p)
                        nalu.p = nalu.p->next = alloc<NALU>();
                    else
                        firstNALU = nalu.p = alloc<NALU>();
                }

                nalu.prealloc = false;
                nalu.complete = false;
                nalu.p->StartOffset = GetByteOffset() - (3 + LongStartCode);

                parseNUH(*nalu.p);

                nalu.NuhBytes = Bs32u(GetByteOffset() - nalu.p->StartOffset);

                if (!picStarted)
                    picStarted = isSlice(*m_lastNALU.p) && NextBits(1);
                else if (!isSuffix(*nalu.p, isSlice(*m_lastNALU.p) && NextBits(1)))
                    break;
            }

            if (!nalu.complete)
            {
                parseRBSP(*nalu.p);

                if (isSlice(*nalu.p))
                {
                    while (!m_postponedSEI.empty())
                    {
                        parseSEIPL(*m_postponedSEI.front());
                        m_postponedSEI.pop_front();
                    }

                    if (m_mode & PARSE_SSD)
                    {
                        if (m_mode & PARALLEL_SD)
                        {
                            nalu.p->NumBytesInRbsp = Bs32u(GetByteOffset() - nalu.p->StartOffset - nalu.NuhBytes - GetEmuBytes());
                            ParseSSDSubmit(pSDT, firstNALU);
                        }
                        else
                        {
                            m_cSlice->ctu = parseSSD(*nalu.p, GetColPic(*m_cSlice));
                            bound(m_cSlice->ctu, m_cSlice);
                        }

                        UpdateColPics(firstNALU, *m_cSlice);
                    }
                }
            }

            nalu.p->NumBytesInNalUnit = Bs32u((GetByteOffset() - nalu.p->StartOffset) - (nalu.NuhBytes - 2));
            if (!nalu.p->NumBytesInRbsp)
                nalu.p->NumBytesInRbsp = nalu.p->NumBytesInNalUnit - GetEmuBytes() - 2;

            try
            {
                NextStartCode(true);
            }
            catch (EndOfBuffer&)
            {
                nalu.complete = true;
            }

            if (!nalu.complete)
            {
                nalu.p->NumBytesInNalUnit = Bs32u(GetByteOffset() - nalu.p->StartOffset - (nalu.NuhBytes - 2));
                if (!nalu.p->NumBytesInRbsp)
                    nalu.p->NumBytesInRbsp = nalu.p->NumBytesInNalUnit - GetEmuBytes() - 2;
                nalu.complete = true;
            }

            TLStart(TRACE_SIZE);
            BS2_TRACE(nalu.p->NumBytesInNalUnit, nalu.NumBytesInNalUnit);
            BS2_TRACE(nalu.p->NumBytesInRbsp, nalu.NumBytesInRbsp);
            TLEnd();
        }
    }
    catch (EndOfBuffer&)
    {
        if (!auSize)
        {
            if (firstNALU)
                free(firstNALU);
            m_lastNALU.p = 0;
            return BS_ERR_MORE_DATA;
        }
    }
    catch (std::bad_alloc&)
    {
        return BS_ERR_MEM_ALLOC;
    }
    catch (Exception& ex)
    {
        return ex.Err();
    }
    catch (...)
    {
        return BS_ERR_UNKNOWN;
    }

    try
    {
        if (m_au)
            free(m_au);

        m_au = firstNALU;

        while (--auSize)
        {
            bound(firstNALU->next, firstNALU);
            firstNALU = firstNALU->next;
        }

        if (firstNALU == m_lastNALU.p)
            m_lastNALU.p = 0;

        firstNALU->next = 0;
    }
    catch (std::bad_alloc&)
    {
        return BS_ERR_MEM_ALLOC;
    }

    pAU = m_au;

    return BS_ERR_NONE;
}

struct SplitTilesPar
{
    Bs32u NumBytesInRbsp;
    Bs16u AddrInRs;
    Bs16u AddrInTs;
    Bs32u NumCtb;
};

Bs32u Parser::ParseSSDSubmit(SDThread*& pSDT, NALU* AU)
{
    std::unique_lock<std::mutex> lockMap(m_mtx, std::defer_lock);
    auto& nalu = m_lastNALU;
    Bs32u CurRBSP = 0;
    std::vector<BsThread::SyncPoint> sliceDep;
    size_t MaxRBSP = 5ull * RawCtuBits / 3 * PicSizeInCtbsY / 8;
    Slice* pSlice = m_cSlice;
    SDPar sdpar = {nalu.p, GetColPic(*pSlice), nalu.p->NumBytesInRbsp, PicSizeInCtbsY, false, false, nullptr};
    std::vector<SplitTilesPar> SplitTiles;
    Bs8u* pRBSP = 0;
    BsThread::SyncPoint spColPic;
    Bs16u SliceAddrRs = pSlice->slice_segment_address;

    auto GetSDT = [this] () -> SDThread*
    {
        auto it = std::find_if(m_sdt.begin(), m_sdt.end(),
                [](SDThread& t)
        {
            std::unique_lock<std::mutex> lockSDT(t.mtx);

            if (!t.locked)
            {
                t.locked++;
                return true;
            }
            return false;
        });

        if (it == m_sdt.end())
        {
            SDTWaitPar par = {this, 0};

            auto spw = m_thread.Submit(SDTWait, &par, 2, 0, 0);

            if (DONE != m_thread.Sync(spw, -1) || !par.pSDT)
                throw Exception(BS_ERR_UNKNOWN);

            return par.pSDT;
        }

        return &*it;
    };

    auto SubmitTask = [&] ()
    {
        BsThread::SyncPoint sp;

        for (;;)
        {
            try
            {
                sp = m_thread.Submit(ParallelSD, pSDT, 0, (Bs32u)sliceDep.size(), sliceDep.empty() ? 0 : &sliceDep[0]);
            }
            catch (TaskQueueOverflow&)
            {
                m_thread.WaitForAny(-1);
                continue;
            }
            break;
        }

        if (m_mode & PARALLEL_AU)
            m_thread.AddDependency(m_spAuToId[AU].AUDone, 1, &sp);

        m_thread.Detach(sp);

        sdpar.pTask->sp    = sp;
        sdpar.pTask->state = WAITING;
        sdpar.pTask->Slice = sdpar.Slice;
    };


    if (   pSlice->num_entry_point_offsets
        && colWidth.size() * rowHeight.size() > 1
        && (m_mode & PARALLEL_TILES))
    {
        Bs16u AddrInTs = CtbAddrRsToTs[pSlice->slice_segment_address];
        Bs16u Tid = TileId[AddrInTs];
        Bs16u EPid = 0;
        Bs32u TargetRBSP, LastRBSP;

        SplitTiles.resize(1);
        SplitTiles.back().AddrInTs = AddrInTs;
        SplitTiles.back().AddrInRs = pSlice->slice_segment_address;
        m_rbsp.resize(MaxRBSP);
        pRBSP = &m_rbsp[0];

        for (Bs16u i = 0; i < pSlice->num_entry_point_offsets; i++)
            CurRBSP += pSlice->entry_point_offset_minus1[i] + 1;

        SetEmulation(false);
        if (CurRBSP != ExtractData(&m_rbsp[0], (Bs32u)CurRBSP))
            throw InvalidSyntax();
        CurRBSP += LastRBSP = ExtractRBSP(&m_rbsp[CurRBSP], (Bs32u)MaxRBSP - CurRBSP);
        SetEmulation(true);

        sdpar.Emulation = true;

        TargetRBSP = Bs32u(CurRBSP / m_sdt.size());

        auto NewTile = [&] ()
        {
            AddrInTs += (colWidth[Tid % colWidth.size()] * rowHeight[Tid / colWidth.size()]);
            Tid = TileId[AddrInTs];

            if (SplitTiles.back().NumBytesInRbsp >= TargetRBSP)
            {
                SplitTilesPar t = {};
                t.AddrInTs = AddrInTs;
                t.AddrInRs = CtbAddrTsToRs[AddrInTs];
                SplitTiles.back().NumCtb = AddrInTs - SplitTiles.back().AddrInTs;
                t.NumCtb = PicSizeInCtbsY - AddrInTs;
                SplitTiles.push_back(t);
            }
        };

        if (   pSlice->pps->entropy_coding_sync_enabled_flag
            && pSlice->num_entry_point_offsets + 1 > rowHeight[Tid])
        {
            while (EPid < pSlice->num_entry_point_offsets)
            {
                SplitTiles.back().NumBytesInRbsp += pSlice->entry_point_offset_minus1[EPid++] + 1;

                if (EPid == rowHeight[Tid])
                    NewTile();
            }
        }
        else
        {
            while (EPid < pSlice->num_entry_point_offsets)
            {
                SplitTiles.back().NumBytesInRbsp += pSlice->entry_point_offset_minus1[EPid++] + 1;
                NewTile();
            }
        }

        SplitTiles.back().NumBytesInRbsp += LastRBSP;
    }

    lockMap.lock();
    if (sdpar.ColPic && !!m_spAuToId.count(sdpar.ColPic))
    {
        spColPic = m_spAuToId[sdpar.ColPic].AUDone;
        sliceDep.push_back(spColPic);
    }
    lockMap.unlock();

    if (SplitTiles.empty())
    {
        if (sdpar.ColPic)
            lock(sdpar.ColPic);

        lock(sdpar.Slice);

        if (!pSlice->dependent_slice_segment_flag)
        {
            if (pSDT && pSDT->locked)
            {
                std::unique_lock<std::mutex> lockSDT(pSDT->mtx);
                pSDT->locked--;
            }

            pSDT = GetSDT();
        }

        {
            std::unique_lock<std::mutex> lockSDT(pSDT->mtx);

            pSDT->locked++;

            if (!pSlice->dependent_slice_segment_flag)
            {
                pSDT->RBSPSize = 0;
                pSDT->RBSPOffset = 0;
                (Info&)pSDT->p = *(Info*)this;
                pSDT->rbsp.resize(MaxRBSP);
                sdpar.NewPicture = true;
            }
            else
            {
                if (NewPicture)
                    throw InvalidSyntax();

                lockMap.lock();
                sliceDep.push_back(m_spAuToId[AU].SD.back().sp);
                lockMap.unlock();
            }

            CurRBSP = ExtractRBSP(&pSDT->rbsp[pSDT->RBSPSize], Bs32u(MaxRBSP - pSDT->RBSPSize));
            nalu.p->NumBytesInRbsp += CurRBSP;
            pSDT->RBSPSize += CurRBSP;

            lockMap.lock();
            m_spAuToId[AU].pAllocator = this;
            m_spAuToId[AU].SD.push_back(SDDesc());
            sdpar.pTask = &m_spAuToId[AU].SD.back();
            lockMap.unlock();

            pSDT->par.push_back(sdpar);

            SubmitTask();
        }

        return CurRBSP;
    }


    for (Bs16u TGId = 0; TGId < SplitTiles.size(); TGId++)
    {
        sliceDep.resize(0);

        if (sdpar.ColPic)
        {
            lock(sdpar.ColPic);
            sliceDep.push_back(spColPic);
        }

        if (TGId > 0)
            pSDT = GetSDT();
        else if (!pSlice->dependent_slice_segment_flag)
        {
            if (pSDT && pSDT->locked)
            {
                std::unique_lock<std::mutex> lockSDT(pSDT->mtx);
                pSDT->locked--;
            }

            pSDT = GetSDT();
        }
        else
        {
            for (auto pN = AU; !!pN; pN = pN->next)
            {
                if (isSlice(*pN) && !pN->slice->dependent_slice_segment_flag)
                    SliceAddrRs = pN->slice->slice_segment_address;
            }

            lockMap.lock();
            sliceDep.push_back(m_spAuToId[AU].SD.back().sp);
            lockMap.unlock();
        }

        {
            std::unique_lock<std::mutex> lockSDT(pSDT->mtx);

            if (TGId > 0)
            {
                NALU* fakeNALU = alloc<NALU>();
                Slice* fakeSlice = alloc<Slice>(fakeNALU);

                *fakeNALU = *nalu.p;
                *fakeSlice = *pSlice;
                fakeNALU->slice = fakeSlice;
                fakeNALU->forbidden_zero_bit = 1;
                sdpar.Slice = fakeNALU;

                for (Bs16u i = SplitTiles[TGId - 1].AddrInTs; i < SplitTiles[TGId].AddrInTs; i++)
                    SliceAddrRsInTs[i] = SliceAddrRs;
            }
            else
            {
                if (pSlice->dependent_slice_segment_flag)
                    pSDT->locked++;
                lock(sdpar.Slice);
            }

            if (!pSlice->dependent_slice_segment_flag || TGId > 0)
            {
                pSDT->RBSPSize = 0;
                pSDT->RBSPOffset = 0;
                pSDT->rbsp.resize(MaxRBSP);
                sdpar.NewPicture = true;
            }

            memmove(&pSDT->rbsp[pSDT->RBSPSize], pRBSP, SplitTiles[TGId].NumBytesInRbsp);
            pRBSP += SplitTiles[TGId].NumBytesInRbsp;

            pSDT->RBSPSize += SplitTiles[TGId].NumBytesInRbsp;
            (Info&)pSDT->p = *(Info*)this;

            sdpar.Slice->NumBytesInRbsp = SplitTiles[TGId].NumBytesInRbsp + sdpar.HeaderSize;
            sdpar.Slice->slice->slice_segment_address = SplitTiles[TGId].AddrInRs;
            sdpar.Slice->slice->Split = (TGId + 1u) < SplitTiles.size();
            sdpar.NumCtb = SplitTiles[TGId].NumCtb;

            if (!sdpar.Slice->slice->Split)
                pSDT->locked++;

            lockMap.lock();
            m_spAuToId[AU].pAllocator = this;
            m_spAuToId[AU].SD.push_back(SDDesc());
            sdpar.pTask = &m_spAuToId[AU].SD.back();
            lockMap.unlock();

            pSDT->par.push_back(sdpar);

            SubmitTask();

            sdpar.HeaderSize = 0;
        }
    }

    return CurRBSP;
}

BsThread::State Parser::ParallelSD(void* self, unsigned int)
{
    SDThread& sdt = *(SDThread*)self;
    std::unique_lock<std::mutex> lock(sdt.mtx);
    SDPar par = sdt.par.front();
    BsThread::State st = DONE;

    lock.unlock();

    sdt.p.NewPicture = par.NewPicture;
    sdt.p.m_cSlice = par.Slice->slice;
    sdt.p.SetEmulation(par.Emulation);
    sdt.p.SetEmuBytes(0);

    try
    {
        sdt.p.Reset(&sdt.rbsp[0] + sdt.RBSPOffset, par.Slice->NumBytesInRbsp - par.HeaderSize);
        par.Slice->slice->ctu = sdt.p.parseSSD(*par.Slice, par.ColPic, par.NumCtb);
        sdt.p.m_pAllocator->bound(par.Slice->slice->ctu, par.Slice->slice);
    }
    catch(...)
    {
        st = FAILED;
    }

    if (par.pTask)
        par.pTask->state = st;

    lock.lock();

    sdt.RBSPOffset += (par.Slice->NumBytesInRbsp - par.HeaderSize);

    par.Slice->NumBytesInRbsp -= sdt.p.GetEmuBytes();

    if (par.ColPic)
        sdt.p.m_pAllocator->unlock(par.ColPic);
    sdt.par.pop_front();

    sdt.p.NewPicture = false;
    sdt.locked--;

    return st;
}
