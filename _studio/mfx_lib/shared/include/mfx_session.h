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

#if !defined(_MFX_SESSION_H)
#define _MFX_SESSION_H

#include <memory>

// base mfx headers
#include <mfxdefs.h>
#include <mfxstructures.h>
#include <mfxvideo++int.h>
#include <mfxaudio++int.h>
#include <mfxplugin.h>
#include "mfx_common.h"

// private headers
#include <mfx_interface_scheduler.h>
#include <libmfx_core_operation.h>

// WARNING: please do not change the type of _mfxSession.
// It is declared as 'struct' in the main header.h

template <class T>
class mfx_core_ptr
{
public:

    explicit mfx_core_ptr(T* ptr = 0)
        : m_bIsNew(false)
        , m_ptr(ptr)
    {
    };
    void reset (T* ptr = 0, bool isNew = true)
    {
        if (m_bIsNew)
            delete m_ptr;

        m_ptr = ptr;
        m_bIsNew = isNew;
    };
    T* get() const
    {
        return m_ptr;
    };
    virtual ~mfx_core_ptr()
    {
        if (m_bIsNew)
            delete m_ptr;

        m_ptr = 0;
    };
    T* operator->() const
    {
        return get();
    }

protected:
    mfx_core_ptr(mfx_core_ptr<T>& ptr);
    mfx_core_ptr<T>& operator=(mfx_core_ptr<T>& ptr);
    bool m_bIsNew;
    T*   m_ptr;
};



struct _mfxSession
{
    // Constructor
    _mfxSession(const mfxU32 adapterNum);
    // Destructor
    ~_mfxSession(void);

    // Clear state
    void Clear(void);

    // Initialize the session
    mfxStatus Init(mfxIMPL implInterface, mfxVersion *ver);

    // Attach to the original scheduler
    mfxStatus RestoreScheduler(void);
    // Release current scheduler
    mfxStatus ReleaseScheduler(void);

    // Declare session's components
    mfx_core_ptr<VideoCORE> m_pCORE;
    std::unique_ptr<AudioCORE> m_pAudioCORE;
    std::unique_ptr<VideoENCODE> m_pENCODE;
    std::unique_ptr<VideoDECODE> m_pDECODE;
    std::unique_ptr<AudioENCODE> m_pAudioENCODE;
    std::unique_ptr<AudioDECODE> m_pAudioDECODE;
    std::unique_ptr<VideoVPP> m_pVPP;
    std::unique_ptr<VideoENC> m_pENC;
    std::unique_ptr<VideoPAK> m_pPAK;
    std::unique_ptr<void *>   m_reserved;

    std::unique_ptr<VideoCodecUSER> m_plgDec;
    std::unique_ptr<VideoCodecUSER> m_plgEnc;
    std::unique_ptr<VideoCodecUSER> m_plgVPP;
    std::unique_ptr<VideoCodecUSER> m_plgGen;

    // Wrapper of interface for core object
    mfxCoreInterface m_coreInt;

    // Current implementation platform ID
    eMFXPlatform m_currentPlatform;
    // Current working HW adapter
    mfxU32 m_adapterNum;
    // Current working interface (D3D9 or so)
    mfxIMPL m_implInterface;

    // Pointer to the scheduler interface being used
    MFXIScheduler *m_pScheduler;
    // Priority of the given session instance
    mfxPriority m_priority;
    // API version requested by application
    mfxVersion  m_version;

    MFXIPtr<OperatorCORE> m_pOperatorCore;

    bool m_reserved1;
    bool m_reserved2;


    inline
    bool IsParentSession(void)
    {
        // if a session has m_pSchedulerAllocated != NULL
        // and the number of "Cores" > 1, then it's a parrent session.
        // and the number of "Cores" == 1, then it's a regular session.
        if (m_pSchedulerAllocated)
            return m_pOperatorCore->HaveJoinedSessions();
        else
            return false;
    }

    inline
    bool IsChildSession(void)
    {
        // child session has different references to active and allocated
        // scheduler. regular session has 2 references to the scheduler.
        // child session has only 1 reference to it.
        return (NULL == m_pSchedulerAllocated);
    }

    template<class T>
    T* Create(mfxVideoParam& par);

protected:
    // Release the object
    void Cleanup(void);

    // this variable is used to deteremine
    // if the object really owns the scheduler.
    MFXIUnknown *m_pSchedulerAllocated;                         // (MFXIUnknown *) pointer to the scheduler allocated

private:
    // Assignment operator is forbidden
    _mfxSession & operator = (const _mfxSession &);
};

#if defined(LINUX64)
  static_assert(sizeof(_mfxSession) == 440, "size_of_session_is_fixed");
#elif defined(LINUX32)
  static_assert(sizeof(_mfxSession) == 244, "size_of_session_is_fixed");
#endif


// {90567606-C57A-447F-8941-1F14597DA475}
static const
    MFX_GUID  MFXISession_1_10_GUID = {0x90567606, 0xc57a, 0x447f, {0x89, 0x41, 0x1f, 0x14, 0x59, 0x7d, 0xa4, 0x75}};

// {701A88BB-E482-4374-A08D-621641EC98B2}
//DEFINE_GUID(<<name>>,
//            {0x701a88bb, 0xe482, 0x4374, {0xa0, 0x8d, 0x62, 0x16, 0x41, 0xec, 0x98, 0xb2}};

class MFXISession_1_10: public MFXIUnknown
{
public:
    virtual ~MFXISession_1_10() {}

    // Finish initialization. Should be called before Init().
    virtual void SetAdapterNum(const mfxU32 adapterNum) = 0;

    virtual std::unique_ptr<VideoCodecUSER> & GetPreEncPlugin() = 0;
};

MFXIPtr<MFXISession_1_10> TryGetSession_1_10(mfxSession session);

class _mfxSession_1_10: public _mfxSession, public MFXISession_1_10
{
public:
    _mfxSession_1_10(mfxU32 adapterNum);

    // Destructor
    virtual ~_mfxSession_1_10(void);

    //
    // MFXISession_1_9 interface
    //

    void SetAdapterNum(const mfxU32 adapterNum);
    std::unique_ptr<VideoCodecUSER> &  GetPreEncPlugin();

    //
    // MFXIUnknown interface
    //

    // Query another interface from the object. If the pointer returned is not NULL,
    // the reference counter is incremented automatically.
    virtual
        void *QueryInterface(const MFX_GUID &guid);

    // Increment reference counter of the object.
    virtual
        void AddRef(void);
    // Decrement reference counter of the object.
    // If the counter is equal to zero, destructor is called and
    // object is removed from the memory.
    virtual
        void Release(void);
    // Get the current reference counter value
    virtual
        mfxU32 GetNumRef(void) const;

    virtual
        mfxStatus InitEx(mfxInitParam& par);

public:
    // Declare additional session's components
    std::unique_ptr<VideoCodecUSER> m_plgPreEnc;

protected:
    // Reference counters
    mfxU32 m_refCounter;
    mfxU16 m_externalThreads;
};


//
// DEFINES FOR IMPLICIT FUNCTIONS IMPLEMENTATION
//

#undef FUNCTION_IMPL
#define FUNCTION_IMPL(component, func_name, formal_param_list, actual_param_list) \
mfxStatus MFXVideo##component##_##func_name formal_param_list \
{ \
    MFX_CHECK(session, MFX_ERR_INVALID_HANDLE); \
    MFX_CHECK(session->m_p##component.get(), MFX_ERR_NOT_INITIALIZED); \
    try { \
        /* call the codec's method */ \
        return session->m_p##component->func_name actual_param_list; \
    } catch(...) { \
        return MFX_ERR_NULL_PTR; \
    } \
}

#undef FUNCTION_AUDIO_IMPL
#define FUNCTION_AUDIO_IMPL(component, func_name, formal_param_list, actual_param_list) \
    mfxStatus MFXAudio##component##_##func_name formal_param_list \
{ \
    MFX_CHECK(session, MFX_ERR_INVALID_HANDLE); \
    MFX_CHECK(session->m_pAudio##component.get(), MFX_ERR_NOT_INITIALIZED); \
    try { \
        /* call the codec's method */ \
        return session->m_pAudio##component->func_name actual_param_list; \
    } catch(...) { \
        return MFX_ERR_NULL_PTR; \
    } \
}


#undef FUNCTION_RESET_IMPL
#define FUNCTION_RESET_IMPL(component, func_name, formal_param_list, actual_param_list) \
mfxStatus MFXVideo##component##_##func_name formal_param_list \
{ \
    MFX_CHECK(session, MFX_ERR_INVALID_HANDLE); \
    MFX_CHECK(session->m_p##component.get(), MFX_ERR_NOT_INITIALIZED); \
    try { \
        /* wait until all tasks are processed */ \
        session->m_pScheduler->WaitForAllTasksCompletion(session->m_p##component.get()); \
        /* call the codec's method */ \
        return session->m_p##component->func_name actual_param_list; \
    } catch(...) { \
        return MFX_ERR_NULL_PTR; \
    } \
}

#undef FUNCTION_AUDIO_RESET_IMPL
#define FUNCTION_AUDIO_RESET_IMPL(component, func_name, formal_param_list, actual_param_list) \
    mfxStatus MFXAudio##component##_##func_name formal_param_list \
{ \
    MFX_CHECK(session, MFX_ERR_INVALID_HANDLE); \
    MFX_CHECK(session->m_pAudio##component.get(), MFX_ERR_NOT_INITIALIZED); \
    try { \
        /* wait until all tasks are processed */ \
        session->m_pScheduler->WaitForAllTasksCompletion(session->m_pAudio##component.get()); \
        /* call the codec's method */ \
        return session->m_pAudio##component->func_name actual_param_list; \
    } catch(...) { \
        return MFX_ERR_NULL_PTR; \
    } \
}

mfxStatus MFXInternalPseudoJoinSession(mfxSession session, mfxSession child_session);
mfxStatus MFXInternalPseudoDisjoinSession(mfxSession session);

#endif // _MFX_SESSION_H

