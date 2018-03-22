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

#if defined(MFX_VA_LINUX) && defined(MFX_ENABLE_MFE)

#include "mfx_mfe_adapter.h"
#include <va/va_backend.h>
#include <dlfcn.h>
#include "vm_interlocked.h"
#include <assert.h>
#include <iterator>

#ifndef MFX_CHECK_WITH_ASSERT
#define MFX_CHECK_WITH_ASSERT(EXPR, ERR) { assert(EXPR); MFX_CHECK(EXPR, ERR); }
#endif

#define CTX(dpy) (((VADisplayContextP)dpy)->pDriverContext)


MFEVAAPIEncoder::MFEVAAPIEncoder() :
      m_refCounter(1)
    , m_vaDisplay(0)
    , m_mfe_context(VA_INVALID_ID)
    , m_framesToCombine(0)
    , m_maxFramesToCombine(0)
    , m_framesCollected(0)
{
    vm_cond_set_invalid(&m_mfe_wait);
    vm_mutex_set_invalid(&m_mfe_guard);

    m_contexts.reserve(MAX_FRAMES_TO_COMBINE);
    m_streams.reserve(MAX_FRAMES_TO_COMBINE);
}

MFEVAAPIEncoder::~MFEVAAPIEncoder()
{
    Destroy();
}

void MFEVAAPIEncoder::AddRef()
{
    vm_interlocked_inc32(&m_refCounter);
}

void MFEVAAPIEncoder::Release()
{
    vm_interlocked_dec32(&m_refCounter);

    if (0 == m_refCounter)
        delete this;
}

mfxStatus MFEVAAPIEncoder::Create(mfxExtMultiFrameParam  const & par, VADisplay vaDisplay)
{
    assert(vaDisplay);

    if(par.MaxNumFrames == 1)
        return MFX_ERR_UNDEFINED_BEHAVIOR;//encoder should not create MFE for single frame as nothing to be combined.

    if (VA_INVALID_ID != m_mfe_context)
    {
        vm_mutex_lock(&m_mfe_guard);
        //TMP WA for SKL due to number of frames limitation in different scenarios:
        //to simplify submission process and not add additional checks for frame wait depending on input parameters
        //if there are encoder want to run less frames within the same MFE Adapter(parent session) align to less now
        //in general just if someone set 3, but another one set 2 after that(or we need to decrease due to parameters) - use 2 for all.
        m_maxFramesToCombine = m_maxFramesToCombine > par.MaxNumFrames ? par.MaxNumFrames : m_maxFramesToCombine;
        vm_mutex_unlock(&m_mfe_guard);
        return MFX_ERR_NONE;
    }

    m_vaDisplay = vaDisplay;

    if (VM_OK != vm_mutex_init(&m_mfe_guard))
    {
        return MFX_ERR_MEMORY_ALLOC;
    }

    if (VM_OK != vm_cond_init(&m_mfe_wait))
    {
        return MFX_ERR_MEMORY_ALLOC;
    }

    m_framesToCombine = 0;

    m_maxFramesToCombine = par.MaxNumFrames ?
            par.MaxNumFrames : MAX_FRAMES_TO_COMBINE;

    m_streams_pool.clear();
    m_toSubmit.clear();

    VAStatus vaSts = vaCreateMFContext(m_vaDisplay, &m_mfe_context);
    if (VA_STATUS_SUCCESS == vaSts)
        return MFX_ERR_NONE;
    else if (VA_STATUS_ERROR_UNIMPLEMENTED == vaSts)
        return MFX_ERR_UNSUPPORTED;
    else
        return MFX_ERR_DEVICE_FAILED;

}

mfxStatus MFEVAAPIEncoder::Join(VAContextID ctx, bool doubleField)
{
    vm_mutex_lock(&m_mfe_guard);//need to protect in case there are streams added/removed in runtime.

    VAStatus vaSts = vaMFAddContext(m_vaDisplay, m_mfe_context, ctx);

    mfxStatus sts = MFX_ERR_NONE;
    switch (vaSts)
    {
        //invalid context means we are adding context
        //with entry point or codec contradicting to already added
        //So it is not supported for single MFE adapter to use HEVC and AVC simultaneosly
    case VA_STATUS_ERROR_INVALID_CONTEXT:
        sts = MFX_ERR_UNDEFINED_BEHAVIOR;
        break;
        //entry point not supported by current driver implementation
    case VA_STATUS_ERROR_UNSUPPORTED_ENTRYPOINT:
        sts = MFX_ERR_UNSUPPORTED;
        break;
        //profile not supported
    case VA_STATUS_ERROR_UNSUPPORTED_PROFILE:
        sts = MFX_ERR_UNSUPPORTED;//keep separate for future possible expansion
        break;
    case VA_STATUS_SUCCESS:
        sts = MFX_ERR_NONE;
        break;
    default:
        sts = MFX_ERR_DEVICE_FAILED;
        break;
    }
    if (MFX_ERR_NONE == sts)
    {
        StreamsIter_t iter;
        // append the pool with a new item;
        m_streams_pool.push_back(m_stream_ids_t(ctx, MFX_ERR_NONE, doubleField));
        iter = m_streams_pool.end();
        m_streamsMap.insert(std::pair<VAContextID, StreamsIter_t>(ctx,--iter));
        // to deal with the situation when a number of sessions < requested
        if (m_framesToCombine < m_maxFramesToCombine)
            ++m_framesToCombine;
    }

    vm_mutex_unlock(&m_mfe_guard);
    return sts;
}

mfxStatus MFEVAAPIEncoder::Disjoin(VAContextID ctx)
{
    vm_mutex_lock(&m_mfe_guard);//need to protect in case there are streams added/removed in runtime
    std::map<VAContextID, StreamsIter_t>::iterator iter = m_streamsMap.find(ctx);

    VAStatus vaSts = vaMFReleaseContext(m_vaDisplay, m_mfe_context, ctx);

    if(iter == m_streamsMap.end())
    {
        vm_mutex_unlock(&m_mfe_guard);
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
    m_streams_pool.erase(iter->second);
    m_streamsMap.erase(iter);
    if (m_framesToCombine > 0 && m_framesToCombine >= m_maxFramesToCombine)
        --m_framesToCombine;
    vm_mutex_unlock(&m_mfe_guard);
    return (VA_STATUS_SUCCESS == vaSts)? MFX_ERR_NONE: MFX_ERR_DEVICE_FAILED;
}

mfxStatus MFEVAAPIEncoder::Destroy()
{
    vm_mutex_lock(&m_mfe_guard);
/* Disabled for now in case driver doesn't support properly.
    for(StreamsIter_t it = m_streams_pool.begin();it == m_streams_pool.end(); it++)
    {
        VAStatus vaSts = vaMFReleaseContext(m_vaDisplay, m_mfe_context, it->ctx);
    }
*/
    VAStatus vaSts = vaDestroyContext(m_vaDisplay, VAContextID(m_mfe_context));
    m_mfe_context = VA_INVALID_ID;
    vm_mutex_unlock(&m_mfe_guard);

    vm_mutex_destroy(&m_mfe_guard);
    vm_cond_destroy(&m_mfe_wait);
    m_streams_pool.clear();
    m_streamsMap.clear();
    MFX_CHECK_WITH_ASSERT(VA_STATUS_SUCCESS == vaSts, MFX_ERR_DEVICE_FAILED);

    return MFX_ERR_NONE;
}

mfxStatus MFEVAAPIEncoder::Submit(VAContextID context, vm_tick timeToWait, bool skipFrame)
{
    vm_mutex_lock(&m_mfe_guard);
    //stream in pool corresponding to particular context;
    StreamsIter_t cur_stream;
    //we can wait for less frames than expected by pipeline due to avilability.
    mfxU32 framesToSubmit = m_framesToCombine;
    if (m_streams_pool.empty())
    {
        //if current stream came to empty pool - others already submitted or in process of submission
        //start pool from the beggining
        while(!m_submitted_pool.empty())
        {
            m_submitted_pool.begin()->reset();
            m_streams_pool.splice(m_streams_pool.end(), m_submitted_pool,m_submitted_pool.begin());
        }
    }
    if(m_streams_pool.empty())
    {
        vm_mutex_unlock(&m_mfe_guard);
        return MFX_ERR_MEMORY_ALLOC;
    }

    //get curret stream by context
    std::map<VAContextID, StreamsIter_t>::iterator iter = m_streamsMap.find(context);
    if(iter == m_streamsMap.end())
    {
        vm_mutex_unlock(&m_mfe_guard);
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    cur_stream = iter->second;
    if(!cur_stream->isFrameSubmitted())
    {
        //if current context is not submitted - it is in stream pull
        m_toSubmit.splice(m_toSubmit.end(), m_streams_pool, cur_stream);
    }
    else
    {
        //if current stream submitted - it is in submitted pool
        //in this case mean current stream submitting it's frames faster then others,
        //at least for 2 reasons:
        //1 - most likely: frame rate is higher
        //2 - amount of streams big enough so first stream submit next frame earlier than last('s)
        //submit current
        //take it back from submitted pull
        m_toSubmit.splice(m_toSubmit.end(), m_submitted_pool, cur_stream);
        cur_stream->reset();//cleanup stream state
    }
    if (skipFrame)
    {
        //if frame is skipped - threat it as submitted without real submission
        //to not lock other streams waiting for it
        m_submitted_pool.splice(m_submitted_pool.end(), m_toSubmit, cur_stream);
        vm_mutex_unlock(&m_mfe_guard);
        return MFX_ERR_NONE;
    }
    ++m_framesCollected;
    if (m_streams_pool.empty())
    {
        //if streams are over in a pool - submit available frames
        //such approach helps to fix situation when we got remainder
        //less than number m_framesToCombine, so current stream does not
        //wait to get frames from other encoders in next cycle
        if (m_framesCollected < framesToSubmit)
            framesToSubmit = m_framesCollected;
    }

    if(!framesToSubmit)
    {
        vm_mutex_unlock(&m_mfe_guard);
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
    vm_tick start_tick = vm_time_get_tick();
    vm_tick spent_ticks = 0;

    while (m_framesCollected < framesToSubmit &&
           !cur_stream->isFieldSubmitted() &&
           timeToWait > spent_ticks)
    {
        vm_status res = vm_cond_timed_uwait(&m_mfe_wait, &m_mfe_guard,
                                          (timeToWait - spent_ticks));
        if ((VM_OK == res) || (VM_TIMEOUT == res))
        {
            spent_ticks = (vm_time_get_tick() - start_tick);
        }
        else
        {
            vm_mutex_unlock(&m_mfe_guard);
            return MFX_ERR_UNKNOWN;
        }
    }
    //for interlace we will return stream back to stream pool when first field submitted
    //to submit next one imediately after than, and don't count it as submitted
    if (!cur_stream->isFieldSubmitted())
    {
        // Form a linear array of contexts for submission
        for (StreamsIter_t it = m_toSubmit.begin(); it != m_toSubmit.end(); ++it)
        {
            if (!it->isFieldSubmitted())
            {
                m_contexts.push_back(it->ctx);
                m_streams.push_back(it);
            }
        }

        if (m_framesCollected < m_contexts.size() || m_contexts.empty())
        {
            for (std::vector<StreamsIter_t>::iterator it = m_streams.begin();
                 it != m_streams.end(); ++it)
            {
                (*it)->sts = MFX_ERR_UNDEFINED_BEHAVIOR;
            }
            // if cur_stream is not in m_streams (somehow)
            cur_stream->sts = MFX_ERR_UNDEFINED_BEHAVIOR;
        }
        else
        {
            VAStatus vaSts = vaMFSubmit(m_vaDisplay, m_mfe_context,
                                         &m_contexts[0], m_contexts.size());

            mfxStatus tmp_res = VA_STATUS_SUCCESS == vaSts ? MFX_ERR_NONE : MFX_ERR_DEVICE_FAILED;
            for (std::vector<StreamsIter_t>::iterator it = m_streams.begin();
                 it != m_streams.end(); ++it)
            {
                (*it)->fieldSubmitted();
                (*it)->sts = tmp_res;
            }
            m_framesCollected -= m_contexts.size();
        }
        // Broadcast is done before unlock for this case to simplify the code avoiding extra ifs
        vm_cond_broadcast(&m_mfe_wait);
        m_contexts.clear();
        m_streams.clear();
    }

    // This frame can be already submitted or errored
    // we have to return strm to the pool, release mutex and exit
    mfxStatus res = cur_stream->sts;
    if(cur_stream->isFrameSubmitted())
    {
        m_submitted_pool.splice(m_submitted_pool.end(), m_toSubmit, cur_stream);
    }
    else
    {
        cur_stream->resetField();
        m_streams_pool.splice(m_streams_pool.end(), m_toSubmit, cur_stream);
    }
    vm_mutex_unlock(&m_mfe_guard);
    return res;
}

#endif //MFX_VA_LINUX && MFX_ENABLE_MFE
