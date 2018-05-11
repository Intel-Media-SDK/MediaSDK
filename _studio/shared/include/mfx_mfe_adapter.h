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

#ifndef _MFX_MFE_ADAPTER_
#define _MFX_MFE_ADAPTER_

#if defined(MFX_VA_LINUX) && defined(MFX_ENABLE_MFE)
#include <va/va.h>
#include <vaapi_ext_interface.h>
#include <vector>
#include <list>
#include "vm_mutex.h"
#include "vm_cond.h"
#include "vm_time.h"
#include <mfxstructures.h>

class MFEVAAPIEncoder
{
    struct m_stream_ids_t
    {
        VAContextID ctx;
        mfxStatus   sts;
        bool interlace;
        mfxU8 fieldNum;
        bool isSubmitted;
        m_stream_ids_t( VAContextID _ctx,
                        mfxStatus _sts,
                        bool fields):
        ctx(_ctx),
        sts(_sts),
        interlace(fields),
        fieldNum(0),
        isSubmitted(false)
        {
        };
        inline void reset()
        {
            sts = MFX_ERR_NONE;
            fieldNum = 0;
            isSubmitted = false;
        };
        inline void resetField()
        {
            isSubmitted = false;
        };
        inline void fieldSubmitted()
        {
            fieldNum++;
            isSubmitted = true;
        };
        inline bool isFieldSubmitted()
        {
            return (fieldNum!=0 && isSubmitted);
        };
        inline bool isFrameSubmitted()
        {
            return isSubmitted && ((interlace && fieldNum == 2) || (fieldNum==1 && !interlace));
        };
    };

public:
    MFEVAAPIEncoder();

    virtual
        ~MFEVAAPIEncoder();
    mfxStatus Create(mfxExtMultiFrameParam const & par, VADisplay vaDisplay);


    mfxStatus Join(VAContextID ctx, bool doubleField);
    mfxStatus Disjoin(VAContextID ctx);
    mfxStatus Destroy();
    mfxStatus Submit(VAContextID context, vm_tick timeToWait, bool skipFrame);//time passed in vm_tick, so milliseconds to be multiplied by vm_frequency/1000

    virtual void AddRef();
    virtual void Release();

private:
    mfxU32      m_refCounter;

    vm_cond     m_mfe_wait;
    vm_mutex    m_mfe_guard;

    VADisplay      m_vaDisplay;
    VAMFContextID  m_mfe_context;

    // a pool (heap) of objects
    std::list<m_stream_ids_t> m_streams_pool;

    // a pool (heap) of objects
    std::list<m_stream_ids_t> m_submitted_pool;

    // a list of objects filled with context info ready to submit
    std::list<m_stream_ids_t> m_toSubmit;

    typedef std::list<m_stream_ids_t>::iterator StreamsIter_t;

    //A number of frames which will be submitted together (Combined)
    mfxU32 m_framesToCombine;

    //A desired number of frames which might be submitted. if
    // actual number of sessions less then it,  m_framesToCombine
    // will be adjusted
    mfxU32 m_maxFramesToCombine;

    // A counter frames collected for the next submission. These
    // frames will be submitted together either when get equal to
    // m_pipelineStreams or when collection timeout elapses.
    mfxU32 m_framesCollected;

    // We need contexts extracted from m_toSubmit to
    // place to a linear vector to pass them to vaMFSubmit
    std::vector<VAContextID> m_contexts;
    // store iterators to particular items
    std::vector<StreamsIter_t> m_streams;
    // store iterators to particular items
    std::map<VAContextID, StreamsIter_t> m_streamsMap;

    // currently up-to-to 3 frames worth combining
    static const mfxU32 MAX_FRAMES_TO_COMBINE = 3;
};
#endif // MFX_VA_LINUX && MFX_ENABLE_MFE

#endif /* _MFX_MFE_ADAPTER */
