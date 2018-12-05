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

#ifndef _MFX_H264_LA_H_
#define _MFX_H264_LA_H_

#if defined(MFX_ENABLE_H264_VIDEO_ENCODE_HW) && defined(MFX_ENABLE_LA_H264_VIDEO_HW)

#include "mfx_enc_ext.h"
#include "mfxla.h"
#include "mfx_h264_encode_hw_utils.h"
#include "cmrt_cross_platform.h"

#include <memory>
#include <list>
#include <vector>

#define REF_FORW  0
#define REF_BACKW 1

namespace MfxEncLA
{
#define MAX_RESOLUTIONS 10

typedef struct 
{
    mfxFrameSurface1*   pFrame;
    mfxU32              encFrameOrder;
    mfxU32              dispFrameOrder;
    mfxU32              poc;
    mfxU16              frameType;
    mfxU16              layer;
}sLAFrameInfo;

typedef struct
{
    sLAFrameInfo        InputFrame;
    sLAFrameInfo        RefFrame[2];
    mfxU8               bFilledForVME;
}sLAInputTask;

typedef struct
{
    mfxU32              numInput;
    mfxU32              pocInput;
    mfxU32              numBuffered;
    mfxU32              numBufferedVME;
    bool                bLastFrameChecked;
}sLASyncContext;

typedef struct
{
    mfxU32                      EncOrder;
    mfxU32                      DispOrder;
    CmSurface2D *               CmRaw;
    CmSurface2D *               CmRawLA;
    MfxHwH264Encode::VmeData *  VmeData;
    CmBufferUP *                CmMb;
    mfxMemId                    RawFrame;
    mfxU8                       bUsed;
}sVMEFrameInfo;

typedef struct
{
    mfxU32                      numInputFrames;
    bool                        bPreEncLastFrames;
}sLAAsyncContext;

typedef struct
{
    mfxU32  IntraCost;
    mfxU32  InterCost;
    mfxU32  DependencyCost; 
    mfxU64 EstimatedRate[52];
}sSumVMEInfo; 




//TO DO: this task must be devided on 2 tasks: VME task and summury task (it will reduce memory)
typedef struct
{
    sLAFrameInfo                frameInfo;    
    MfxHwH264Encode::VmeData *  m_vmeData; 
    MfxHwH264Encode::VmeData *  m_vmeDataRef[2];
    sSumVMEInfo                 VMESum[MAX_RESOLUTIONS]; // for each resolution 
}sLASummaryTask;

typedef struct
{
    CmBuffer *                  m_cmCurbe;      // control structure for ME & HME kernels
    SurfaceIndex *              m_cmRefs;       // Surface for ME kernelv
    SurfaceIndex *              m_cmRefsLa;     // Surface for 2X kernel
    void *                      m_cmMbSys;      // pointer to associated system memory buffer   
    CmEvent *                   m_event; 
    sVMEFrameInfo               m_Curr;    
    sVMEFrameInfo               m_Ref[2];
    sLAInputTask                m_TaskInfo;
}sLADdiTask;

typedef struct
{
    mfxFrameSurface1* reordered_surface;
    bool              bFrameLASubmitted;
    mfxFrameSurface1* output_surface;
    mfxENCOutput*     stat;
}sAsyncParams;

template<class T> inline void Zero(std::vector<T> & vec)   { if (vec.size() > 0) memset(&vec[0], 0, sizeof(T) * vec.size()); }
class VMERefList
{
protected:
    std::vector<sVMEFrameInfo>    RefList;
public:
    void Init(mfxU32 numRef,mfxU32 nAsync, bool bPyramid)
    {
        RefList.resize(nAsync + numRef + (bPyramid? 2:0));
        Zero(RefList);
    }
    sVMEFrameInfo* GetByEncOrder(mfxU32 encOrder)
    {
        std::vector<sVMEFrameInfo>::iterator i = RefList.begin();
        while (i != RefList.end())
        {
            if ((*i).bUsed && (*i).EncOrder == encOrder)
                return &(*i);
            ++i;
        }
        return NULL;    
    }
    sVMEFrameInfo* GetFree()
    {
        std::vector<sVMEFrameInfo>::iterator i = RefList.begin();
        while (i != RefList.end())
        {
            if (!(*i).bUsed)
                return &(*i);
            ++ i;
        }
        return NULL;     
    }
    sVMEFrameInfo* GetOldest()
    {
        sVMEFrameInfo* pOldest = 0;
        std::vector<sVMEFrameInfo>::iterator i = RefList.begin();
        while (i != RefList.end())
        {
            if ((*i).bUsed)
            {
                if (pOldest == 0 || pOldest->DispOrder >(*i).DispOrder )
                {
                    pOldest = &(*i);
                }
            }
            ++i;
        }
        return pOldest;     
    }
    size_t GetSize()
    {
        return RefList.size();
    }

};



class CmContextLA : public MfxHwH264Encode::CmContext
{
public:

    CmContextLA() : MfxHwH264Encode::CmContext()    {};
    CmContextLA(MfxHwH264Encode::MfxVideoParam const & video, CmDevice * cmDevice, VideoCORE * core) : 
        MfxHwH264Encode::CmContext(video,cmDevice,core) {};

    CmEvent * RunVme(
        sLADdiTask const & task,
        mfxU32          qp);

    mfxStatus QueryVme(
        sLADdiTask const & task,
        CmEvent *       e);

    void SetCurbeData(
        MfxHwH264Encode::CURBEData & curbeData,
        sLADdiTask const &task,
        mfxU32           qp);
};

}; // namespace

bool bEnc_LA(mfxVideoParam *par);

class VideoENC_LA:  public VideoENC_Ext
{
public:

     VideoENC_LA(VideoCORE *core, mfxStatus *sts);

    // Destructor
    virtual
    ~VideoENC_LA(void);

    virtual
    mfxStatus Init(mfxVideoParam *par) ;
    virtual
    mfxStatus Reset(mfxVideoParam *par);
    virtual
    mfxStatus Close(void);
    virtual
    mfxTaskThreadingPolicy GetThreadingPolicy(void) {return MFX_TASK_THREADING_DEDICATED;}

    static 
    mfxStatus Query(VideoCORE*, mfxVideoParam *in, mfxVideoParam *out);
    static 
    mfxStatus QueryIOSurf(VideoCORE*, mfxVideoParam *par, mfxFrameAllocRequest *request);

    virtual
    mfxStatus GetVideoParam(mfxVideoParam *par);
    
    virtual
    mfxStatus RunFrameVmeENCCheck(  mfxENCInput *in, 
                                    mfxENCOutput *out,
                                    MFX_ENTRY_POINT pEntryPoints[],
                                    mfxU32 &numEntryPoints);
    virtual 
    void* GetDstForSync(MFX_ENTRY_POINT& pEntryPoints);
    virtual 
    void* GetSrcForSync(MFX_ENTRY_POINT& pEntryPoints);


    virtual
    mfxStatus RunFrameVmeENC(mfxENCInput *in, mfxENCOutput *out);
    

public:
    mfxStatus ResetTaskCounters();
    
    mfxStatus SubmitFrameLA(mfxFrameSurface1 *pInSurface);
    mfxStatus QueryFrameLA (mfxFrameSurface1 *pInSurface, mfxENCOutput *out);


protected:

    mfxStatus  InsertTaskWithReordenig(MfxEncLA::sLAInputTask &newTask, bool bEndOfSequence);
    mfxStatus  InsertTaskWithReordenigBPyr(MfxEncLA::sLAInputTask &newTask,bool bEndOfSequence);
    mfxFrameSurface1*   GetFrameToVME();

    inline bool bInputFrameReady()
    {
        return ((m_syncTasks.size()>0) && (m_syncTasks.front().InputFrame.pFrame != 0));
    }
    mfxStatus InitVMEData(MfxEncLA::sVMEFrameInfo *   pVME, mfxU32  EncOrder, mfxU32  DispOrder, mfxFrameSurface1* RawFrame, CmBufferUP * CmMb);
    mfxStatus FreeUnusedVMEData(MfxEncLA::sVMEFrameInfo *pVME);


    

private:

    bool                    m_bInit;
    VideoCORE*              m_core;
    mfxExtLAControl         m_LaControl;
    mfxFrameAllocRequest    m_Request[2];


private:
    std::list <mfxFrameSurface1*>               m_SurfacesForOutput;

    std::vector<MfxEncLA::sLAInputTask>         m_miniGop;
    mfxI32                                      m_numLastB;
    std::list<MfxEncLA::sLAInputTask>           m_syncTasks;
    std::list<MfxEncLA::sLADdiTask>             m_VMETasks;
    UMC::Mutex                                  m_listMutex;
    MfxEncLA::sLASyncContext                    m_LASyncContext;
    MfxEncLA::sLAAsyncContext                   m_LAAsyncContext;
    MfxEncLA::VMERefList                        m_VMERefList;
    std::list<MfxEncLA::sLASummaryTask>         m_OutputTasks;

private:

    MfxHwH264Encode::CmDevicePtr                    m_cmDevice;
    std::unique_ptr<MfxEncLA::CmContextLA>            m_cmCtx;
    MfxHwH264Encode::MfxVideoParam                  m_video;
    std::vector<MfxHwH264Encode::VmeData>           m_vmeDataStorage;
        
    MfxHwH264Encode::MfxFrameAllocResponse          m_mb;
    MfxHwH264Encode::MfxFrameAllocResponse          m_curbe;
    MfxHwH264Encode::MfxFrameAllocResponse          m_rawLa;
    MfxHwH264Encode::MfxFrameAllocResponse          m_raw;
    MfxHwH264Encode::MfxFrameAllocResponse          m_opaqResponse;
};

#endif
#endif

