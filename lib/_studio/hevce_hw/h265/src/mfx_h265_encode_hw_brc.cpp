// Copyright (c) 2017 Intel Corporation
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

#include "mfx_config.h"
#if defined(MFX_ENABLE_H265_VIDEO_ENCODE)

#include "mfx_h265_encode_hw_brc.h"
#include "math.h"
#include <algorithm>
namespace MfxHwH265Encode
{

#define CLIPVAL(MINVAL, MAXVAL, VAL) (MFX_MAX(MINVAL, MFX_MIN(MAXVAL, VAL)))
mfxF64 const INIT_RATE_COEFF_VME[] = {
        1.109, 1.196, 1.225, 1.309, 1.369, 1.428, 1.490, 1.588, 1.627, 1.723, 1.800, 1.851, 1.916,
        2.043, 2.052, 2.140, 2.097, 2.096, 2.134, 2.221, 2.084, 2.153, 2.117, 2.014, 1.984, 2.006,
        1.801, 1.796, 1.682, 1.549, 1.485, 1.439, 1.248, 1.221, 1.133, 1.045, 0.990, 0.987, 0.895,
        0.921, 0.891, 0.887, 0.896, 0.925, 0.917, 0.942, 0.964, 0.997, 1.035, 1.098, 1.170, 1.275
    };
mfxF64 const QSTEP_VME[52] = {
        0.630,  0.707,  0.794,  0.891,  1.000,   1.122,   1.260,   1.414,   1.587,   1.782,   2.000,   2.245,   2.520,
        2.828,  3.175,  3.564,  4.000,  4.490,   5.040,   5.657,   6.350,   7.127,   8.000,   8.980,  10.079,  11.314,
    12.699, 14.254, 16.000, 17.959, 20.159,  22.627,  25.398,  28.509,  32.000,  35.919,  40.317,  45.255,  50.797,
    57.018, 64.000, 71.838, 80.635, 90.510, 101.594, 114.035, 128.000, 143.675, 161.270, 181.019, 203.187, 228.070
};
/*mfxF64 const QSTEP[52] = {
    0.82,    0.93,    1.11,    1.24,     1.47,    1.65,    1.93,    2.12,    2.49,    2.80,
    3.15,    3.53,    4.10,    4.55,     5.24,    5.88,    6.44,    7.20,    8.36,    8.92,
    10.26,  11.55,    12.93,  14.81,    17.65,   19.30,   22.30,   25.46,   28.97,   33.22,
    38.50,  43.07,    50.52,  55.70,    64.34,   72.55,   82.25,   93.12,   108.95, 122.40,
    130.39,148.69,   185.05, 194.89,   243.18,  281.61,  290.58,   372.38,  378.27, 406.62,
    468.34,525.69 
};*/


mfxF64 const INTRA_QSTEP_COEFF  = 2.0;
mfxF64 const INTRA_MODE_BITCOST = 0.0;
mfxF64 const INTER_MODE_BITCOST = 0.0;
mfxI32 const MAX_QP_CHANGE      = 2;
mfxF64 const LOG2_64            = 3;
mfxF64 const MIN_EST_RATE       = 0.3;
mfxF64 const NORM_EST_RATE      = 100.0;

mfxF64 const MIN_RATE_COEFF_CHANGE = 0.5;
mfxF64 const MAX_RATE_COEFF_CHANGE = 2.0;



mfxStatus VMEBrc::Init(MfxVideoParam &video, mfxI32 )
{
    m_qpUpdateRange = 20;
    mfxU32 RegressionWindow = 20;

    mfxF64 fr = mfxF64(video.mfx.FrameInfo.FrameRateExtN) / video.mfx.FrameInfo.FrameRateExtD;
    m_totNumMb = video.mfx.FrameInfo.Width * video.mfx.FrameInfo.Height / 256;
    m_initTargetRate = 1000 * video.mfx.TargetKbps / fr / m_totNumMb;
    m_targetRateMin = m_initTargetRate;
    m_targetRateMax = m_initTargetRate;

    m_laData.resize(0);

    for (mfxU32 qp = 0; qp < 52; qp++)
            m_rateCoeffHistory[qp].Reset(RegressionWindow, 100.0, 100.0 * INIT_RATE_COEFF_VME[qp]);

    m_framesBehind = 0;
    m_bitsBehind = 0.0;
    m_curQp = -1;
    m_curBaseQp = -1;
    m_lookAheadDep = 0;
    

    return MFX_ERR_NONE;
}

mfxStatus VMEBrc::SetFrameVMEData(const mfxExtLAFrameStatistics *pLaOut, mfxU32 width, mfxU32 height)
{
    mfxU32 resNum = 0;
    mfxU32 numLaFrames = pLaOut->NumFrame;
    mfxU32 k = height*width >> 7;

    UMC::AutomaticUMCMutex guard(m_mutex);

    while(resNum < pLaOut->NumStream) 
    {
        if (pLaOut->FrameStat[resNum*numLaFrames].Height == height &&
            pLaOut->FrameStat[resNum*numLaFrames].Width  == width) 
            break;
        resNum ++;
    }
    MFX_CHECK(resNum <  pLaOut->NumStream, MFX_ERR_UNDEFINED_BEHAVIOR);
    MFX_CHECK(pLaOut->NumFrame > 0, MFX_ERR_UNDEFINED_BEHAVIOR);

    mfxLAFrameInfo * pFrameData = pLaOut->FrameStat + numLaFrames*resNum;
    std::list<LaFrameData>::iterator it = m_laData.begin();
    while (m_laData.size()>0)
    {
        it = m_laData.begin();
        if (!((*it).bNotUsed))
            break;
        m_laData.pop_front();
    }

    // some frames can be stored already
    // start of stored sequence
    it = m_laData.begin();
     while (it != m_laData.end())
    {
        if ((*it).encOrder == pFrameData[0].FrameEncodeOrder)
            break;
        ++it;
    }
    mfxU32 ind  = 0;
    
    // check stored sequence
    while ((it != m_laData.end()) && (ind < numLaFrames))
    {
        MFX_CHECK((*it).encOrder == pFrameData[ind].FrameEncodeOrder, MFX_ERR_UNDEFINED_BEHAVIOR);
        ++ind;
        ++it;
    }
    MFX_CHECK(it == m_laData.end(), MFX_ERR_UNDEFINED_BEHAVIOR);

    // store a new data
    for (; ind < numLaFrames; ind++)
    {
        LaFrameData data = {};

        data.encOrder  = pFrameData[ind].FrameEncodeOrder;
        data.dispOrder = pFrameData[ind].FrameDisplayOrder;
        data.interCost = pFrameData[ind].InterCost;
        data.intraCost = pFrameData[ind].IntraCost;
        data.propCost  = pFrameData[ind].DependencyCost;
        data.bframe    = (pFrameData[ind].FrameType & MFX_FRAMETYPE_B) != 0;
        data.layer     = pFrameData[ind].Layer;

        MFX_CHECK(data.intraCost, MFX_ERR_UNDEFINED_BEHAVIOR);

        for (mfxU32 qp = 0; qp < 52; qp++)
        {
            data.estRate[qp] = ((mfxF64)pFrameData[ind].EstimatedRate[qp])/(QSTEP_VME[qp]*k);
            //printf("data.estRate[%d] %f\n", qp, data.estRate[qp]);
        }
        //printf("EncOrder %d, dispOrder %d, interCost %d,  intraCost %d, data.propCost %d\n", 
        //    data.encOrder,data.dispOrder, data.interCost, data.intraCost, data.propCost );
        m_laData.push_back(data);
    }
    if (m_lookAheadDep == 0)
        m_lookAheadDep = numLaFrames;
      
    //printf("VMEBrc::SetFrameVMEData m_laData[0].encOrder %d\n", pFrameData[0].FrameEncodeOrder);

    return MFX_ERR_NONE;
}

mfxU8 SelectQp(mfxF64 erate[52], mfxF64 budget, mfxU8 minQP)
{
    minQP = (minQP !=0) ? minQP : 1; //KW
    for (mfxU8 qp = minQP; qp < 52; qp++)
        if (erate[qp] < budget)
            return (erate[qp - 1] + erate[qp] < 2 * budget) ? qp - 1 : qp;
    return 51;
}


mfxF64 GetTotalRate(std::list<VMEBrc::LaFrameData>::iterator start, std::list<VMEBrc::LaFrameData>::iterator end, mfxI32 baseQp)
{
    mfxF64 totalRate = 0.0;
    std::list<VMEBrc::LaFrameData>::iterator it = start;
    for (; it!=end; ++it)
        totalRate += (*it).estRateTotal[CLIPVAL(0, 51, baseQp + (*it).deltaQp)];
    return totalRate;
}



 mfxI32 SelectQp(std::list<VMEBrc::LaFrameData>::iterator start, std::list<VMEBrc::LaFrameData>::iterator end, mfxF64 budget, mfxU8 minQP)
{
    mfxF64 prevTotalRate = GetTotalRate(start,end, minQP);
    for (mfxU8 qp = minQP+1; qp < 52; qp++)
    {
        mfxF64 totalRate = GetTotalRate(start,end, qp);
        if (totalRate < budget)
            return (prevTotalRate + totalRate < 2 * budget) ? qp - 1 : qp;
        else
            prevTotalRate = totalRate;
    }
    return 51;
}

mfxU8 GetFrameTypeLetter(mfxU32 frameType)
{
    mfxU32 ref = (frameType & MFX_FRAMETYPE_REF) ? 0 : 'a' - 'A';
    if (frameType & MFX_FRAMETYPE_I)
        return mfxU8('I' + ref);
    if (frameType & MFX_FRAMETYPE_P)
        return mfxU8('P' + ref);
    if (frameType & MFX_FRAMETYPE_B)
        return mfxU8('B' + ref);
    return 'x';
}




void VMEBrc::PreEnc(mfxU32 /*frameType*/, std::vector<VmeData *> const & /*vmeData*/, mfxU32 /*curEncOrder*/)
{
}


mfxU32 VMEBrc::Report(mfxU32 frameType, mfxU32 dataLength, mfxU32 /*userDataLength*/, mfxU32 /*repack*/, mfxU32  picOrder, mfxU32 /* maxFrameSize */, mfxU32 /* qp */)
{
    frameType; // unused

    UMC::AutomaticUMCMutex guard(m_mutex);

    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_INTERNAL, "LookAheadBrc2::Report");
    //printf("VMEBrc::Report order %d\n", picOrder);
    mfxF64 realRatePerMb = 8 * dataLength / mfxF64(m_totNumMb);

    m_framesBehind++;
    m_bitsBehind += realRatePerMb;
   
    // start of sequence
    mfxU32 numFrames = 0;
    std::list<LaFrameData>::iterator start = m_laData.begin();
    for(;start != m_laData.end(); ++start)
    {
        if ((*start).encOrder == picOrder)
            break;
    }
    //printf("Report: data len %d, realRatePerMb %f, framesBehin %d, bitsBehind %f, init target rate %f, qp %d\n", dataLength, realRatePerMb, m_framesBehind, m_bitsBehind, m_initTargetRate, (*start).qp);

    for (std::list<LaFrameData>::iterator it = start; it != m_laData.end(); ++it) numFrames++;
    numFrames = MFX_MIN(numFrames, m_lookAheadDep);

     mfxF64 framesBeyond = (mfxF64)(MFX_MAX(2, numFrames) - 1); 
     m_targetRateMax = (m_initTargetRate * (m_framesBehind + m_lookAheadDep - 1) - m_bitsBehind) / framesBeyond;
     m_targetRateMin = (m_initTargetRate * (m_framesBehind + framesBeyond  ) - m_bitsBehind) / framesBeyond;


    if (start != m_laData.end())
    {
        //mfxU32 level = (*start).layer < 8 ? (*start).layer : 7;
        mfxI32 curQp = (*start).qp;
        mfxF64 oldCoeff = m_rateCoeffHistory[curQp].GetCoeff();
        mfxF64 y = MFX_MAX(0.0, realRatePerMb);
        mfxF64 x = (*start).estRate[curQp];
        mfxF64 minY = NORM_EST_RATE * INIT_RATE_COEFF_VME[curQp] * MIN_RATE_COEFF_CHANGE;
        mfxF64 maxY = NORM_EST_RATE * INIT_RATE_COEFF_VME[curQp] * MAX_RATE_COEFF_CHANGE;
        y = CLIPVAL(minY, maxY, y / x * NORM_EST_RATE); 
        m_rateCoeffHistory[curQp].Add(NORM_EST_RATE, y);
        //mfxF64 ratio = m_rateCoeffHistory[curQp].GetCoeff() / oldCoeff;
        mfxF64 ratio = y / (oldCoeff*NORM_EST_RATE);
        //printf("oldCoeff %f, new %f, ratio %f\n",oldCoeff, y, ratio);
        for (mfxI32 i = -m_qpUpdateRange; i <= m_qpUpdateRange; i++)
            if (i != 0 && curQp + i >= 0 && curQp + i < 52)
            {
                mfxF64 r = ((ratio - 1.0) * (1.0 - ((mfxF64)abs(i))/((mfxF64)m_qpUpdateRange + 1.0)) + 1.0);
                m_rateCoeffHistory[curQp + i].Add(NORM_EST_RATE,
                    NORM_EST_RATE * m_rateCoeffHistory[curQp + i].GetCoeff() * r);
            }
        (*start).bNotUsed = 1;
    }
    return 0;
}

mfxI32 VMEBrc::GetQP(MfxVideoParam &video, Task &task )
{
    video;

    UMC::AutomaticUMCMutex guard(m_mutex);

    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_INTERNAL, "VMEBrc::GetQp");

    if (!m_laData.size())
        return 26;

    mfxF64 totalEstRate[52] = {}; 

     // start of sequence
    std::list<LaFrameData>::iterator start = m_laData.begin();
    while (start != m_laData.end())
    {
        if ((*start).encOrder == task.m_eo)
            break;
        ++start;
    }
    MFX_CHECK(start != m_laData.end(), MFX_ERR_UNDEFINED_BEHAVIOR);
    
    std::list<LaFrameData>::iterator it = start;
    mfxU32 numberOfFrames = 0;
    for(it = start;it != m_laData.end(); ++it) 
        numberOfFrames++;

    numberOfFrames = MFX_MIN(numberOfFrames, m_lookAheadDep);

   // fill totalEstRate
   it = start;
   for(mfxU32 i=0; i < numberOfFrames ; i++)
   {
        for (mfxU32 qp = 0; qp < 52; qp++)
        {
            
            (*it).estRateTotal[qp] = MFX_MAX(MIN_EST_RATE, m_rateCoeffHistory[qp].GetCoeff() * (*it).estRate[qp]);
            totalEstRate[qp] += (*it).estRateTotal[qp];        
        }
        ++it;
   }
   


   // calculate Qp
    mfxI32 maxDeltaQp = -100000;
    mfxI32 curQp = m_curBaseQp < 0 ? SelectQp(totalEstRate, m_targetRateMin * numberOfFrames, 1) : m_curBaseQp;
    mfxF64 strength = 0.03 * curQp + .75;

    // fill deltaQp
    it = start;
    for (mfxU32 i=0; i < numberOfFrames ; i++)
    {
        mfxU32 intraCost    = (*it).intraCost;
        mfxU32 interCost    = (*it).interCost;
        mfxU32 propCost     = (*it).propCost;
        mfxF64 ratio        = 1.0;//mfxF64(interCost) / intraCost;
        mfxF64 deltaQp      = log((intraCost + propCost * ratio) / intraCost) / log(2.0);
        (*it).deltaQp = (interCost >= intraCost * 0.9)
            ? -mfxI32(deltaQp * 2.0 * strength + 0.5)
            : -mfxI32(deltaQp * 1.0 * strength + 0.5);
        maxDeltaQp = MFX_MAX(maxDeltaQp, (*it).deltaQp);
        ++it;
    }
    
 
    it = start;
    for (mfxU32 i=0; i < numberOfFrames ; i++)
    {
        (*it).deltaQp -= maxDeltaQp;
        ++it;
    }

    
    {
        mfxU8 minQp = (mfxU8) SelectQp(start,m_laData.end(), m_targetRateMax * numberOfFrames, 1);
        mfxU8 maxQp = (mfxU8) SelectQp(start,m_laData.end(), m_targetRateMin * numberOfFrames, 1);

        if (m_curBaseQp < 0)
            m_curBaseQp = minQp; // first frame
        else if (m_curBaseQp < minQp)
            m_curBaseQp = CLIPVAL(m_curBaseQp - MAX_QP_CHANGE, m_curBaseQp + MAX_QP_CHANGE, minQp);
        else if (m_curQp > maxQp)
            m_curBaseQp = CLIPVAL(m_curBaseQp - MAX_QP_CHANGE, m_curBaseQp + MAX_QP_CHANGE, maxQp);
        else
            {}; // do not change qp if last qp guarantees target rate interval
    }
    m_curQp = CLIPVAL(1, 51, m_curBaseQp + (*start).deltaQp); // driver doesn't support qp=0
    //printf("%d) intra %d, inter %d, prop %d, delta %d, maxdelta %d, baseQP %d, qp %d \n",(*start).encOrder,(*start).intraCost, (*start).interCost, (*start).propCost, (*start).deltaQp, maxDeltaQp, m_curBaseQp,m_curQp );

    //printf("%d\t base QP %d\tdelta QP %d\tcurQp %d, rate (%f, %f), total rate %f (%f, %f), number of frames %d\n", 
    //    (*start).dispOrder, m_curBaseQp, (*start).deltaQp, m_curQp, (mfxF32)m_targetRateMax*numberOfFrames,(mfxF32)m_targetRateMin*numberOfFrames,  (mfxF32)GetTotalRate(start,m_laData.end(), m_curQp),  (mfxF32)GetTotalRate(start,m_laData.end(), m_curQp + 1), (mfxF32)GetTotalRate(start,m_laData.end(), m_curQp -1),numberOfFrames);
    //if ((*start).dispOrder > 1700 && (*start).dispOrder < 1800 )
    {
        //for (mfxU32 qp = 0; qp < 52; qp++)
        //{
        //    printf("....qp %d coeff hist %f\n", qp, (mfxF32)m_rateCoeffHistory[qp].GetCoeff());
       // }        
    }
    (*start).qp = m_curQp;

    return mfxU8(m_curQp);
}
BrcIface * CreateBrc(MfxVideoParam &video)

{
    if (video.isSWBRC())
    {
        if (video.mfx.RateControlMethod == MFX_RATECONTROL_LA_EXT)
            return new VMEBrc;
        else if (video.mfx.RateControlMethod == MFX_RATECONTROL_CBR || video.mfx.RateControlMethod == MFX_RATECONTROL_VBR)
            return new H265BRC;
    }
    return 0;
}

#define MFX_H265_BITRATE_SCALE 0
#define MFX_H265_CPBSIZE_SCALE 2



//--------------------------------- SW BRC -----------------------------------------------------------------------
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define MAX_DOUBLE                  1.7e+308    ///< max. value of double-type value
#define Saturate(min_val, max_val, val) MAX((min_val), MIN((max_val), (val)))

#define BRC_CLIP(val, minval, maxval) val = Saturate(minval, maxval, val)

#define SetQuantB() \
    mQuantB = ((mQuantP + mQuantPrev + 1) >> 1) + 1; \
    BRC_CLIP(mQuantB, 1, mQuantMax)

   mfxF64 const QSTEP1[88] = {
         0.630,  0.707,  0.794,  0.891,  1.000,   1.122,   1.260,   1.414,   1.587,   1.782,   2.000,   2.245,   2.520,
         2.828,  3.175,  3.564,  4.000,  4.490,   5.040,   5.657,   6.350,   7.127,   8.000,   8.980,  10.079,  11.314,
        12.699, 14.254, 16.000, 17.959, 20.159,  22.627,  25.398,  28.509,  32.000,  35.919,  40.317,  45.255,  50.797,
        57.018, 64.000, 71.838, 80.635, 90.510, 101.594, 114.035, 128.000, 143.675, 161.270, 181.019, 203.187, 228.070,
        256.000, 287.350, 322.540, 362.039, 406.375, 456.140, 512.000, 574.701, 645.080, 724.077, 812.749, 912.280,
        1024.000, 1149.401, 1290.159, 1448.155, 1625.499, 1824.561, 2048.000, 2298.802, 2580.318, 2896.309, 3250.997, 3649.121,
        4096.000, 4597.605, 5160.637, 5792.619, 6501.995, 7298.242, 8192.000, 9195.209, 10321.273, 11585.238, 13003.989, 14596.485
    };


mfxI32 QStep2QpFloor(mfxF64 qstep, mfxI32 qpoffset = 0) // QSTEP[qp] <= qstep, return 0<=qp<=51+mQuantOffset
{
    uint8_t qp = uint8_t(std::upper_bound(QSTEP1, QSTEP1 + 52 + qpoffset, qstep) - QSTEP1);
    return qp > 0 ? qp - 1 : 0;
}

mfxI32 Qstep2QP(mfxF64 qstep, mfxI32 qpoffset = 0) // return 0<=qp<=51+mQuantOffset
{
    mfxI32 qp = QStep2QpFloor(qstep, qpoffset);
    return (qp == 51 + qpoffset || qstep < (QSTEP1[qp] + QSTEP1[qp + 1]) / 2) ? qp : qp + 1;
}

mfxF64 QP2Qstep(mfxI32 qp, mfxI32 qpoffset = 0)
{
    return QSTEP1[MFX_MIN(51 + qpoffset, qp)];
}

#define BRC_QSTEP_SCALE_EXPONENT 0.7
#define BRC_RATE_CMPLX_EXPONENT 0.5

mfxStatus H265BRC::Close()
{
    mfxStatus status = MFX_ERR_NONE;
    if (!m_IsInit)
        return MFX_ERR_NOT_INITIALIZED;    
    
    memset(&m_par, 0, sizeof(m_par));
    memset(&m_hrdState, 0, sizeof(m_hrdState));
    ResetParams();
    m_IsInit = false;
    return status;
}

mfxStatus H265BRC::InitHRD()
{
    m_hrdState.bufFullness = m_hrdState.prevBufFullness= m_par.initialDelayInBytes << 3;
    m_hrdState.underflowQuant = 0;
    m_hrdState.overflowQuant = 999;
    m_hrdState.frameNum = 0;

    m_hrdState.minFrameSize = 0;
    m_hrdState.maxFrameSize = 0;
    
    return MFX_ERR_NONE;
}

mfxU32 H265BRC::GetInitQP()
{
    mfxI32 fs, fsLuma;

  fsLuma = m_par.width * m_par.height;
  fs = fsLuma;

  if (m_par.chromaFormat == MFX_CHROMAFORMAT_YUV420)
    fs += fsLuma / 2;
  else if (m_par.chromaFormat == MFX_CHROMAFORMAT_YUV422)
    fs += fsLuma;
  else if (m_par.chromaFormat == MFX_CHROMAFORMAT_YUV444)
    fs += fsLuma * 2;
  fs = fs * m_par.bitDepthLuma / 8;
  
 
  mfxF64 qstep = pow(1.5 * fs * m_par.frameRate / (m_par.targetbps), 0.8);
 
  mfxI32 q = Qstep2QP(qstep, mQuantOffset) + mQuantOffset;

  BRC_CLIP(q, 1, mQuantMax);

  return q;
}

mfxStatus H265BRC::SetParams( MfxVideoParam &par)
{
    MFX_CHECK(par.mfx.RateControlMethod == MFX_RATECONTROL_CBR || par.mfx.RateControlMethod == MFX_RATECONTROL_VBR, MFX_ERR_UNDEFINED_BEHAVIOR );

    m_par.rateControlMethod = par.mfx.RateControlMethod;
    m_par.bufferSizeInBytes  = ((par.BufferSizeInKB*1000) >> (1 + MFX_H265_CPBSIZE_SCALE)) << (1 + MFX_H265_CPBSIZE_SCALE);
    m_par.initialDelayInBytes =((par.InitialDelayInKB*1000) >> (1 + MFX_H265_CPBSIZE_SCALE)) << (1 + MFX_H265_CPBSIZE_SCALE);

    m_par.targetbps = (((par.TargetKbps*1000) >> (6 + MFX_H265_BITRATE_SCALE)) << (6 + MFX_H265_BITRATE_SCALE));
    m_par.maxbps =    (((par.MaxKbps*1000) >> (6 + MFX_H265_BITRATE_SCALE)) << (6 + MFX_H265_BITRATE_SCALE));

    MFX_CHECK (par.mfx.FrameInfo.FrameRateExtD != 0 && par.mfx.FrameInfo.FrameRateExtN != 0, MFX_ERR_UNDEFINED_BEHAVIOR);

    m_par.frameRate = (mfxF64)par.mfx.FrameInfo.FrameRateExtN / (mfxF64)par.mfx.FrameInfo.FrameRateExtD;
    m_par.width = par.mfx.FrameInfo.Width;
    m_par.height =par.mfx.FrameInfo.Height;
    m_par.chromaFormat = par.mfx.FrameInfo.ChromaFormat;
    m_par.bitDepthLuma = par.mfx.FrameInfo.BitDepthLuma;

    m_par.inputBitsPerFrame = m_par.maxbps / m_par.frameRate;
    m_par.gopPicSize = par.mfx.GopPicSize;
    m_par.gopRefDist = par.mfx.GopRefDist;
 
    return MFX_ERR_NONE;
}

mfxStatus H265BRC::Init( MfxVideoParam &par, mfxI32 enableRecode)
{
    mfxStatus sts = MFX_ERR_NONE;

    sts = SetParams(par);
    MFX_CHECK_STS(sts);

    //mLowres = video.LowresFactor ? 1 : 0;

    mRecode = enableRecode;

    if (mRecode)
        InitHRD();

    
    mBitsDesiredFrame = (mfxI32)(m_par.targetbps / m_par.frameRate);
    
    MFX_CHECK(mBitsDesiredFrame > 10, MFX_ERR_INVALID_VIDEO_PARAM);

    mQuantOffset = 6 * (m_par.bitDepthLuma - 8);
    mQuantMax = 51 + mQuantOffset;
    mQuantMin = 1;
    mMinQp = mQuantMin;


    mBitsDesiredTotal = 0;
    mBitsEncodedTotal = 0;

    mQuantUpdated = 1;
    mRecodedFrame_encOrder = 0;
    m_bRecodeFrame = false;

    mfxI32 q = GetInitQP();

    if (!mRecode) {
        if (q - 6 > 10)
            mQuantMin = MFX_MAX(10, q - 10);
        else
            mQuantMin = MFX_MAX(q - 6, 2);

        if (q < mQuantMin)
            q = mQuantMin;
    }

    mQuantPrev = mQuantI = mQuantP = mQuantB = mQPprev = q;

    mRCbap = 100;
    mRCqap = 100;
    mRCfap = 100;

    mRCq = q;
    mRCqa = mRCqa0 = 1. / (mfxF64)mRCq;
    mRCfa = mBitsDesiredFrame;
    mRCfa_short = mBitsDesiredFrame;

    mSChPoc = 0;
    mSceneChange = 0;
    mBitsEncodedPrev = mBitsDesiredFrame;
    mPicType = MFX_FRAMETYPE_I;

    mMaxQp = 999;
    mMinQp = -1;

    m_IsInit = true;



    return sts;
}




mfxStatus H265BRC::Reset(MfxVideoParam &par, mfxI32 )
{
    mfxStatus sts = MFX_ERR_NONE;
    MFX_CHECK (mRecode == 0, MFX_ERR_UNDEFINED_BEHAVIOR);

    sts = SetParams(par);
    MFX_CHECK_STS(sts);

    mBitsDesiredFrame = (mfxI32)(m_par.targetbps / m_par.frameRate);    
    MFX_CHECK(mBitsDesiredFrame > 10, MFX_ERR_INVALID_VIDEO_PARAM);

    mRCq = (mfxI32)(1./mRCqa * pow(mRCfa/mBitsDesiredFrame, 0.32) + 0.5); 
    BRC_CLIP(mRCq, 1, mQuantMax); 
    mQuantPrev = mQuantI = mQuantP = mQuantB = mQPprev = mRCq; 
    mRCqa = mRCqa0 = 1./mRCq; 
    mRCfa = mBitsDesiredFrame; 
    mRCfa_short = mBitsDesiredFrame; 


    return sts;
}

#define BRC_QSTEP_COMPL_EXPONENT 0.4
#define BRC_QSTEP_COMPL_EXPONENT_AVBR 0.2
#define BRC_CMPLX_DECAY 0.1
#define BRC_CMPLX_DECAY_AVBR 0.2
#define BRC_MIN_CMPLX 0.01
#define BRC_MIN_CMPLX_LAYER 0.05
#define BRC_MIN_CMPLX_LAYER_I 0.01

static const mfxF64 brc_qstep_factor[8] = {pow(2., -1./6.), 1., pow(2., 1./6.),   pow(2., 2./6.), pow(2., 3./6.), pow(2., 4./6.), pow(2., 5./6.), 2.};
static const mfxF64 minQstep = QSTEP1[0];
static const mfxF64 maxQstepChange = pow(2, 0.5);
static const mfxF64 predCoef = 1000000. / (1920 * 1080);

mfxU16 GetFrameType(Task &task)
{
    if (task.m_frameType & MFX_FRAMETYPE_I)
        return MFX_FRAMETYPE_I;
    if ((task.m_frameType & MFX_FRAMETYPE_B) && !task.m_ldb)
        return MFX_FRAMETYPE_B;

    return MFX_FRAMETYPE_P;
}

mfxI32 H265BRC::GetQP(MfxVideoParam &video, Task &task)
{
    //printf("mQuant I %d, P %d B %d, m_bRecodeFrame %d, mQuantOffset %d, max - min %d, %d\n ",mQuantI,mQuantP, mQuantB, m_bRecodeFrame, mQuantOffset, mQuantMax, mQuantMin ); 
    mfxI32 qp = mQuantB;

    if (task.m_eo == mRecodedFrame_encOrder && m_bRecodeFrame)
        qp = mQuantRecoded;
    else {
        mfxU16 type = GetFrameType(task);

        if (type == MFX_FRAMETYPE_I) 
            qp = mQuantI;
        else if (type == MFX_FRAMETYPE_P) 
            qp = mQuantP;
        else { 
            if (video.isBPyramid()) {
                qp = mQuantB + task.m_level - 1;
                BRC_CLIP(qp, 1, mQuantMax);
            } else
                qp = mQuantB;
        }
    }

    return qp - mQuantOffset;
}

mfxStatus H265BRC::SetQP(mfxI32 qp, mfxU16 frameType, bool bLowDelay)
{
    if (MFX_FRAMETYPE_B == frameType && !bLowDelay) {
        mQuantB = qp + mQuantOffset;
        BRC_CLIP(mQuantB, 1, mQuantMax);
    } else {
        mRCq = qp + mQuantOffset;
        BRC_CLIP(mRCq, 1, mQuantMax);
        mQuantI = mQuantP = mRCq;
    }
    return MFX_ERR_NONE;
}


mfxBRCStatus H265BRC::UpdateAndCheckHRD(mfxI32 frameBits, mfxF64 inputBitsPerFrame, mfxI32 recode)
{
    mfxBRCStatus ret = MFX_ERR_NONE;
    mfxF64 buffSizeInBits = m_par.bufferSizeInBytes <<3;

    if (!(recode & (MFX_BRC_EXT_FRAMESKIP - 1))) { // BRC_EXT_FRAMESKIP == 16
        m_hrdState.prevBufFullness = m_hrdState.bufFullness;
        m_hrdState.underflowQuant = 0;
        m_hrdState.overflowQuant = 999;
    } else { // frame is being recoded - restore buffer state
        m_hrdState.bufFullness = m_hrdState.prevBufFullness;
    }

    m_hrdState.maxFrameSize = (mfxI32)(m_hrdState.bufFullness - 1);
    m_hrdState.minFrameSize = (m_par.rateControlMethod != MFX_RATECONTROL_CBR ? 0 : (mfxI32)(m_hrdState.bufFullness + 1 + 1 + inputBitsPerFrame - buffSizeInBits));
    if (m_hrdState.minFrameSize < 0)
        m_hrdState.minFrameSize = 0;

   mfxF64  bufFullness = m_hrdState.bufFullness - frameBits;

    if (bufFullness < 2) {
        bufFullness = inputBitsPerFrame;
        ret = MFX_BRC_ERR_BIG_FRAME;
        if (bufFullness > buffSizeInBits)
            bufFullness = buffSizeInBits;
    } else {
        bufFullness += inputBitsPerFrame;
        if (bufFullness > buffSizeInBits - 1) {
            bufFullness = buffSizeInBits - 1;
            if (m_par.rateControlMethod == MFX_RATECONTROL_CBR)
                ret = MFX_BRC_ERR_SMALL_FRAME;
        }
    }
    if (MFX_ERR_NONE == ret)
        m_hrdState.frameNum++;
    else if ((recode & MFX_BRC_EXT_FRAMESKIP) || MFX_BRC_RECODE_EXT_PANIC == recode || MFX_BRC_RECODE_PANIC == recode) // no use in changing QP
        ret |= MFX_BRC_NOT_ENOUGH_BUFFER;

    m_hrdState.bufFullness = bufFullness;

    return ret;
}

#define BRC_SCENE_CHANGE_RATIO1 20.0
#define BRC_SCENE_CHANGE_RATIO2 10.0
#define BRC_RCFAP_SHORT 5

#define I_WEIGHT 1.2
#define P_WEIGHT 0.25
#define B_WEIGHT 0.2

#define BRC_MAX_LOAN_LENGTH 75
#define BRC_LOAN_RATIO 0.075

#define BRC_BIT_LOAN \
{ \
    if (picType == MFX_FRAMETYPE_I) { \
        if (mLoanLength) \
            bitsEncoded += mLoanLength * mLoanBitsPerFrame; \
        mLoanLength = video->GopPicSize; \
        if (mLoanLength > BRC_MAX_LOAN_LENGTH || mLoanLength == 0) \
            mLoanLength = BRC_MAX_LOAN_LENGTH; \
        mfxI32 bitsEncodedI = (mfxI32)((mfxF64)bitsEncoded  / (mLoanLength * BRC_LOAN_RATIO + 1)); \
        mLoanBitsPerFrame = (bitsEncoded - bitsEncodedI) / mLoanLength; \
        bitsEncoded = bitsEncodedI; \
    } else if (mLoanLength) { \
        bitsEncoded += mLoanBitsPerFrame; \
        mLoanLength--; \
    } \
}





void H265BRC::SetParamsForRecoding (mfxI32 encOrder)
{
    mQuantUpdated = 0;
    m_bRecodeFrame = true;
    mRecodedFrame_encOrder = encOrder;
}
 bool  H265BRC::isFrameBeforeIntra (mfxU32 order)
 {
     mfxI32 distance0 = m_par.gopPicSize*3/4;
     mfxI32 distance1 = m_par.gopPicSize - m_par.gopRefDist*3;
     return (order - mEOLastIntra) > (mfxU32)(MFX_MAX(distance0, distance1));
 }


mfxBRCStatus H265BRC::PostPackFrame(MfxVideoParam & /* par */, Task &task, mfxI32 totalFrameBits, mfxI32 overheadBits, mfxI32 repack)
{
    mfxBRCStatus Sts = MFX_ERR_NONE;

    mfxI32 bitsEncoded = totalFrameBits - overheadBits;
    mfxF64 e2pe;
    mfxI32 qp, qpprev;
    mfxU32 prevFrameType = mPicType;
    mfxU32 picType = GetFrameType(task);
    mfxI32 qpY = task.m_qpY;

    mPoc = task.m_poc;
    qpY += mQuantOffset;

    mfxI32 layer = ((picType == MFX_FRAMETYPE_I) ? 0 : ((picType == MFX_FRAMETYPE_P) ?  1 : 1 + MFX_MAX(1, task.m_level))); // should be 0 for I, 1 for P, etc. !!!
    mfxF64 qstep = QP2Qstep(qpY, mQuantOffset);

    if (picType == MFX_FRAMETYPE_I)
        mEOLastIntra = task.m_eo;

    if (!repack && mQuantUpdated <= 0) { // BRC reported buffer over/underflow but the application ignored it
        mQuantI = mQuantIprev;
        mQuantP = mQuantPprev;
        mQuantB = mQuantBprev;
        mRecode |= 2;
        mQp = mRCq;
        UpdateQuant(mBitsEncoded, totalFrameBits, layer);
    }

    mQuantIprev = mQuantI;
    mQuantPprev = mQuantP;
    mQuantBprev = mQuantB;

    mBitsEncoded = bitsEncoded;

    if (mSceneChange)
        if (mQuantUpdated == 1 && mPoc > mSChPoc + 1)
            mSceneChange &= ~16;


    mfxF64 buffullness = 1.e12; // a big number
    
    if (mRecode)
    {
        buffullness = repack ? m_hrdState.prevBufFullness : m_hrdState.bufFullness;
        Sts = UpdateAndCheckHRD(totalFrameBits, m_par.inputBitsPerFrame, repack);
    }

    qpprev = qp = mQp = qpY;

    mfxF64 fa_short0 = mRCfa_short;
    mRCfa_short += (bitsEncoded - mRCfa_short) / BRC_RCFAP_SHORT;

    {
        qstep = QP2Qstep(qp, mQuantOffset);
        mfxF64 qstep_prev = QP2Qstep(mQPprev, mQuantOffset);
        mfxF64 frameFactor = 1.0;
        mfxF64 targetFrameSize = MFX_MAX((mfxF64)mBitsDesiredFrame, mRCfa);
        if (picType & MFX_FRAMETYPE_I)
            frameFactor = 3.0;

        e2pe = bitsEncoded * sqrt(qstep) / (mBitsEncodedPrev * sqrt(qstep_prev));

        mfxF64 maxFrameSize;
        maxFrameSize = 2.5/9. * buffullness + 5./9. * targetFrameSize;
        BRC_CLIP(maxFrameSize, targetFrameSize, BRC_SCENE_CHANGE_RATIO2 * targetFrameSize * frameFactor);

        mfxF64 famax = 1./9. * buffullness + 8./9. * mRCfa;

        mfxI32 maxqp = mQuantMax;
        if (m_par.rateControlMethod == MFX_RATECONTROL_CBR && mRecode ) {
            maxqp = MFX_MIN(maxqp, m_hrdState.overflowQuant - 1);
        }

        if (bitsEncoded >  maxFrameSize && qp < maxqp ) {
            mfxF64 targetSizeScaled = maxFrameSize * 0.8;
            mfxF64 qstepnew = qstep * pow(bitsEncoded / targetSizeScaled, 0.9);
            mfxI32 qpnew = Qstep2QP(qstepnew, mQuantOffset);
            if (qpnew == qp)
              qpnew++;
            BRC_CLIP(qpnew, 1, maxqp);

            if (qpnew > qp) {
                mQp = mRCq = mQuantI = mQuantP = qpnew;
                if (picType & MFX_FRAMETYPE_B)
                    mQuantB = qpnew;
                else {
                    SetQuantB();
                }

                mRCfa_short = fa_short0;

                if (e2pe > BRC_SCENE_CHANGE_RATIO1) { // scene change, resetting BRC statistics
                  mRCfa = mBitsDesiredFrame;
                  mRCqa = 1./qpnew;
                  mQp = mRCq = mQuantI = mQuantP = mQuantB = mQuantPrev = qpnew;
                  mSceneChange |= 1;
                  if (picType != MFX_FRAMETYPE_B) {
                      mSceneChange |= 16;
                      mSChPoc = mPoc;
                  }
                  mRCfa_short = mBitsDesiredFrame;
                }
                if (mRecode) {
                    SetParamsForRecoding (task.m_eo);
                    m_hrdState.frameNum--;
                    mMinQp = qp;
                    mQuantRecoded = qpnew;
                    //printf("recode1 %d %d %d %d \n", task.m_eo, qpnew, bitsEncoded ,  (int)maxFrameSize);
                    return MFX_BRC_ERR_BIG_FRAME;
                }
            }
        }

        if (bitsEncoded >  maxFrameSize && qp == maxqp && picType != MFX_FRAMETYPE_I && mRecode && (!task.m_bSkipped) && isFrameBeforeIntra(task.m_eo)) //skip frames before intra
        {
            SetParamsForRecoding(task.m_eo);
            m_hrdState.frameNum--;
            return MFX_BRC_ERR_BIG_FRAME|MFX_BRC_NOT_ENOUGH_BUFFER;
        }

        if (mRCfa_short > famax && (!repack) && qp < maxqp) {

            mfxF64 qstepnew = qstep * mRCfa_short / (famax * 0.8);
            mfxI32 qpnew = Qstep2QP(qstepnew, mQuantOffset);
            if (qpnew == qp)
                qpnew++;
            BRC_CLIP(qpnew, 1, maxqp);

            mRCfa = mBitsDesiredFrame;
            mRCqa = 1./qpnew;
            mQp = mRCq = mQuantI = mQuantP = mQuantB = mQuantPrev = qpnew;

            mRCfa_short = mBitsDesiredFrame;

            if (mRecode) {
                SetParamsForRecoding (task.m_eo);
                m_hrdState.frameNum--;
                mMinQp = qp;
                mQuantRecoded = qpnew;

            }
        }
    }

    mPicType = picType;

    mfxF64 fa = mRCfa;
    bool oldScene = false;
    if ((mSceneChange & 16) && (mPoc < mSChPoc) && (mBitsEncoded * (0.9 * BRC_SCENE_CHANGE_RATIO1) < (mfxF64)mBitsEncodedP) && (mfxF64)mBitsEncoded < 1.5*fa)
        oldScene = true;

    if (Sts != MFX_BRC_OK && mRecode) {
        Sts = UpdateQuantHRD(totalFrameBits, Sts, overheadBits, layer, task.m_eo == mRecodedFrame_encOrder && m_bRecodeFrame);
        SetParamsForRecoding(task.m_eo);
        mPicType = prevFrameType;
        mRCfa_short = fa_short0;
    } else {
        if (mQuantUpdated == 0 && 1./qp < mRCqa)
            mRCqa += (1. / qp - mRCqa) / 16;
        else if (mQuantUpdated == 0)
            mRCqa += (1. / qp - mRCqa) / (mRCqap > 25 ? 25 : mRCqap);
        else
            mRCqa += (1. / qp - mRCqa) / mRCqap;

        BRC_CLIP(mRCqa, 1./mQuantMax , 1./1.);

        if (repack != MFX_BRC_RECODE_PANIC && repack != MFX_BRC_RECODE_EXT_PANIC && !oldScene) {
            mQPprev = qp;
            mBitsEncodedPrev = mBitsEncoded;

            Sts = UpdateQuant(bitsEncoded, totalFrameBits,  layer, task.m_eo == mRecodedFrame_encOrder && m_bRecodeFrame);

            if (mPicType != MFX_FRAMETYPE_B) {
                mQuantPrev = mQuantP;
                mBitsEncodedP = mBitsEncoded;
            }

            mQuantP = mQuantI = mRCq;
        }
        mQuantUpdated = 1;
        //    mMaxBitsPerPic = mMaxBitsPerPicNot0;

        if (mRecode)
        {
            m_hrdState.underflowQuant = 0;
            m_hrdState.overflowQuant = 999;
        }

        mMinQp = -1;
        mMaxQp = 999;
    }


    return Sts;
};
void H265BRC::ResetParams()
{
    mQuantI = mQuantP= mQuantB= mQuantMax= mQuantMin= mQuantPrev= mQuantOffset= mQPprev=0;
    mMinQp=0;
    mMaxQp=0;
    mBitsDesiredFrame=0;
    mQuantUpdated=0;
    mRecodedFrame_encOrder=0;
    m_bRecodeFrame=false;
    mPicType=0;
    mRecode=0;
    mBitsEncodedTotal= mBitsDesiredTotal=0;
    mQp=0;
    mRCfap= mRCqap= mRCbap= mRCq=0;
    mRCqa= mRCfa= mRCqa0=0;
    mRCfa_short=0;
    mQuantRecoded=0;
    mQuantIprev= mQuantPprev= mQuantBprev=0;
    mBitsEncoded=0;
    mSceneChange=0;
    mBitsEncodedP= mBitsEncodedPrev=0;
    mPoc= mSChPoc=0;
    mEOLastIntra = 0;
}
mfxBRCStatus H265BRC::UpdateQuant(mfxI32 bitEncoded, mfxI32 totalPicBits, mfxI32 , mfxI32 recode)
{
    mfxBRCStatus Sts = MFX_ERR_NONE;
    mfxF64  bo = 0, qs = 0, dq = 0;
    mfxI32  quant = (recode) ? mQuantRecoded : mQp;
    mfxU32 bitsPerPic = (mfxU32)mBitsDesiredFrame;
    mfxI64 totalBitsDeviation = 0;
    mfxF64 buffSizeInBits = m_par.bufferSizeInBytes <<3;

    if (mRecode & 2) {
        mRCfa = bitsPerPic;
        mRCqa = mRCqa0;
        mRecode &= ~2;
    }

    mBitsEncodedTotal += totalPicBits;
    mBitsDesiredTotal += bitsPerPic;
    totalBitsDeviation = mBitsEncodedTotal - mBitsDesiredTotal;

    if (mRecode) 
    {
        if (m_par.rateControlMethod == MFX_RATECONTROL_VBR) {
            mfxI64 targetFullness = MFX_MIN(m_par.initialDelayInBytes << 3, (mfxU32)buffSizeInBits / 2);
            mfxI64 minTargetFullness = MFX_MIN(mfxU32(buffSizeInBits / 2), m_par.targetbps * 2); // half bufsize or 2 sec
            if (targetFullness < minTargetFullness)
                targetFullness = minTargetFullness;
                mfxI64 bufferDeviation = targetFullness - (mfxI64)m_hrdState.bufFullness;
                if (bufferDeviation > totalBitsDeviation)
                    totalBitsDeviation = bufferDeviation;
        }
    }

    if (mPicType != MFX_FRAMETYPE_I || m_par.rateControlMethod == MFX_RATECONTROL_CBR || mQuantUpdated == 0)
        mRCfa += (bitEncoded - mRCfa) / mRCfap;
    SetQuantB();
    if (mQuantUpdated == 0)
        if (mQuantB < quant)
            mQuantB = quant;
    qs = pow(bitsPerPic / mRCfa, 2.0);
    dq = mRCqa * qs;

    mfxI32 bap = mRCbap;
    if (mRecode)
    {
        mfxF64 bfRatio = m_hrdState.bufFullness / mBitsDesiredFrame;
        if (totalBitsDeviation > 0) {
            bap = (mfxI32)bfRatio*3;
            bap = MFX_MAX(bap, 10);
            BRC_CLIP(bap, mRCbap/10, mRCbap);
        }
    }

    bo = (mfxF64)totalBitsDeviation / bap / mBitsDesiredFrame;
    BRC_CLIP(bo, -1.0, 1.0);

    dq = dq + (1./mQuantMax - dq) * bo;
    BRC_CLIP(dq, 1./mQuantMax, 1./mQuantMin);
    quant = (mfxI32) (1. / dq + 0.5);

    if (quant >= mRCq + 5)
        quant = mRCq + 3;
    else if (quant >= mRCq + 3)
        quant = mRCq + 2;
    else if (quant > mRCq + 1)
        quant = mRCq + 1;
    else if (quant <= mRCq - 5)
        quant = mRCq - 3;
    else if (quant <= mRCq - 3)
        quant = mRCq - 2;
    else if (quant < mRCq - 1)
        quant = mRCq - 1;

    mRCq = quant;

    if (mRecode)
    {
        mfxF64 qstep = QP2Qstep(quant, mQuantOffset);
        mfxF64 fullnessThreshold = MIN(bitsPerPic * 12, buffSizeInBits*3/16);
        qs = 1.0;
        if (bitEncoded > m_hrdState.bufFullness && mPicType != MFX_FRAMETYPE_I)
            qs = (mfxF64)bitEncoded / (m_hrdState.bufFullness);

        if (m_hrdState.bufFullness < fullnessThreshold && (mfxU32)totalPicBits > bitsPerPic)
            qs *= sqrt((mfxF64)fullnessThreshold * 1.3 / m_hrdState.bufFullness); // ??? is often useless (quant == quant_old)

        if (qs > 1.0) {
            qstep *= qs;
            quant = Qstep2QP(qstep, mQuantOffset);
            if (mRCq == quant)
                quant++;

            BRC_CLIP(quant, 1, mQuantMax);

            mQuantB = ((quant + quant) * 563 >> 10) + 1;
            BRC_CLIP(mQuantB, 1, mQuantMax);
            mRCq = quant;
        }
    }
    return Sts;
}

mfxBRCStatus H265BRC::UpdateQuantHRD(mfxI32 totalFrameBits, mfxBRCStatus sts, mfxI32 overheadBits, mfxI32 , mfxI32 recode)
{
    mfxI32 quant, quant_prev;
    mfxI32 wantedBits = (sts == MFX_BRC_ERR_BIG_FRAME ? m_hrdState.maxFrameSize * 3 / 4 : m_hrdState.minFrameSize * 5 / 4);
    mfxI32 bEncoded = totalFrameBits - overheadBits;
    mfxF64 qstep, qstep_new;

    wantedBits -= overheadBits;
    if (wantedBits <= 0) // possible only if BRC_ERR_BIG_FRAME
        return (sts | MFX_BRC_NOT_ENOUGH_BUFFER);

    if (recode)
        quant_prev = quant = mQuantRecoded;
    else
        //quant_prev = quant = (mPicType == MFX_FRAMETYPE_I) ? mQuantI : ((mPicType == MFX_FRAMETYPE_P) ? mQuantP : (layer > 0 ? mQuantB + layer - 1 : mQuantB));
        quant_prev = quant = mQp;

    if (sts & MFX_BRC_ERR_BIG_FRAME)
        m_hrdState.underflowQuant = quant;
    else if (sts & MFX_BRC_ERR_SMALL_FRAME)
        m_hrdState.overflowQuant = quant;

    qstep = QP2Qstep(quant, mQuantOffset);
    qstep_new = qstep * sqrt((mfxF64)bEncoded / wantedBits);
//    qstep_new = qstep * sqrt(sqrt((mfxF64)bEncoded / wantedBits));
    quant = Qstep2QP(qstep_new, mQuantOffset);
    BRC_CLIP(quant, 1, mQuantMax);

    if (sts & MFX_BRC_ERR_SMALL_FRAME) // overflow
    {
        mfxI32 qpMin = MFX_MAX(m_hrdState.underflowQuant, mMinQp);
        if (qpMin > 0) {
            if (quant < (qpMin + quant_prev + 1) >> 1)
                quant = (qpMin + quant_prev + 1) >> 1;
        }
        if (quant > quant_prev - 1)
            quant = quant_prev - 1;
        if (quant < m_hrdState.underflowQuant + 1)
            quant = m_hrdState.underflowQuant + 1;
        if (quant < mMinQp + 1 && quant_prev > mMinQp + 1)
            quant = mMinQp + 1;
    } 
    else // underflow
    {
        if (m_hrdState.overflowQuant <= mQuantMax) {
            if (quant > (quant_prev + m_hrdState.overflowQuant + 1) >> 1)
                quant = (quant_prev + m_hrdState.overflowQuant + 1) >> 1;
        }
        if (quant < quant_prev + 1)
            quant = quant_prev + 1;
        if (quant > m_hrdState.overflowQuant - 1)
            quant = m_hrdState.overflowQuant - 1;
    }

   if (quant == quant_prev)
        return (sts | MFX_BRC_NOT_ENOUGH_BUFFER);

    mQuantRecoded = quant;

    return sts;
}


void  H265BRC::GetMinMaxFrameSize(mfxI32 *minFrameSizeInBits, mfxI32 *maxFrameSizeInBits)
{
    if (minFrameSizeInBits)
      *minFrameSizeInBits = m_hrdState.minFrameSize;
    if (maxFrameSizeInBits)
      *maxFrameSizeInBits = m_hrdState.maxFrameSize;
}

}
#endif
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



