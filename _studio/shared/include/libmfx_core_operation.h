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

#ifndef __LIBMFX_CORE_OPERATOR_H__
#define __LIBMFX_CORE_OPERATOR_H__

#include <vector>
#include <vm_interlocked.h>
#include <umc_mutex.h>

class VideoCORE;

class OperatorCORE
{
public:
    OperatorCORE(VideoCORE* pCore)
        : m_refCounter(1)
        , m_CoreCounter(0)
    {
        m_Cores.push_back(pCore);
        pCore->SetCoreId(0);
    };

    mfxStatus AddCore(VideoCORE* pCore)
    {
        UMC::AutomaticUMCMutex guard(m_guard);

        if (m_Cores.size() == 0xFFFF)
            return MFX_ERR_MEMORY_ALLOC;

        m_Cores.push_back(pCore);
        pCore->SetCoreId(++m_CoreCounter);
        m_CoreCounter = (m_CoreCounter == 0xFFFF)?0:m_CoreCounter;

        return MFX_ERR_NONE;
    }

    void RemoveCore(VideoCORE* pCore)
    {
        UMC::AutomaticUMCMutex guard(m_guard);
        std::vector<VideoCORE*>::iterator it = m_Cores.begin();
        for (;it != m_Cores.end();it++)
        {
            if (*it == pCore)
            {
                m_Cores.erase(it);
                return;
            }
        }
    }

    // functor to run fuction from child cores
    bool  IsOpaqSurfacesAlreadyMapped(mfxFrameSurface1 **pOpaqueSurface, mfxU32 NumOpaqueSurface, mfxFrameAllocResponse *response)
    {
        UMC::AutomaticUMCMutex guard(m_guard);
        bool sts;
        std::vector<VideoCORE*>::iterator it = m_Cores.begin();

        for (;it != m_Cores.end();it++)
        {
            sts = (*it)->IsOpaqSurfacesAlreadyMapped(pOpaqueSurface, NumOpaqueSurface, response, false);
            if (sts)
                return sts;
        }
        return false;
    }

    // functor to run fuction from child cores
    bool CheckOpaqRequest(mfxFrameAllocRequest *request, mfxFrameSurface1 **pOpaqueSurface, mfxU32 NumOpaqueSurface)
    {
        UMC::AutomaticUMCMutex guard(m_guard);
        bool sts;
        std::vector<VideoCORE*>::iterator it = m_Cores.begin();

        for (;it != m_Cores.end();it++)
        {
            sts = (*it)->CheckOpaqueRequest(request, pOpaqueSurface, NumOpaqueSurface, false);
            if (!sts)
                return sts;
        }
        return true;
    }

    // functor to run fuction from child cores
    template <typename func, typename arg, typename arg2>
    mfxStatus DoFrameOperation(func functor, arg par, arg2 out)
    {
        UMC::AutomaticUMCMutex guard(m_guard);
        mfxStatus sts;
        std::vector<VideoCORE*>::iterator it = m_Cores.begin();

        for (;it != m_Cores.end();it++)
        {
            sts = ((*it)->*functor)(par, out, false);
            // if it is correct Core we can return
            if (MFX_ERR_NONE == sts)
                return sts;
        }
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    // functor to run fuction from child cores
    template <typename func, typename arg>
    mfxStatus DoCoreOperation(func functor, arg par)
    {
        UMC::AutomaticUMCMutex guard(m_guard);
        mfxStatus sts;
        std::vector<VideoCORE*>::iterator it = m_Cores.begin();

        for (;it != m_Cores.end();it++)
        {
            sts = ((*it)->*functor)(par, false);
            // if it is correct Core we can return
            if (MFX_ERR_NONE == sts)
                return sts;
        }
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    // functor to get instance from child cores
    template <typename obj, typename func, typename arg>
    void* QueryGUID(func functor, arg par)
    {
        UMC::AutomaticUMCMutex guard(m_guard);
        std::vector<VideoCORE*>::iterator it = m_Cores.begin();

        for (;it != m_Cores.end();it++)
        {
            obj* object = (obj*)((*it)->*functor)(par);
            // if it is correct Core we can return
            if (object->get())
                return (void*)(object->get());
        }
        return NULL;
    }

    // functor to run fuction from child cores
    template <typename func, typename arg>
    mfxFrameSurface1* GetSurface(func functor, arg par)
    {
        UMC::AutomaticUMCMutex guard(m_guard);
        mfxFrameSurface1* pSurf;
        std::vector<VideoCORE*>::iterator it = m_Cores.begin();
        for (;it != m_Cores.end();it++)
        {
            pSurf = ((*it)->*functor)(par, false);
            // if it is correct Core we can return
            if (pSurf)
                return pSurf;
        }
        return 0;
    }

    // Increment reference counter of the object.
    virtual
    void AddRef(void)
    {
        vm_interlocked_inc32(&m_refCounter);
    };
    // Decrement reference counter of the object.
    // If the counter is equal to zero, destructor is called and
    // object is removed from the memory.
    virtual
    void Release(void)
    {
        vm_interlocked_dec32(&m_refCounter);

        if (0 == m_refCounter)
        {
            delete this;
        }
    };
    // Get the current reference counter value
    virtual
    mfxU32 GetNumRef(void)
    {
        return m_refCounter;
    };
    // Check if we have joined sessions
    bool HaveJoinedSessions()
    {
        return m_Cores.size() > 1;
    }

private:

    virtual ~OperatorCORE()
    {
        m_Cores.clear();
    };
    // self and child cores
    std::vector<VideoCORE*>  m_Cores;

    // Reference counters
    mfxU32 m_refCounter;

    UMC::Mutex m_guard;

    mfxU32     m_CoreCounter;

    // Forbid the assignment operator
    OperatorCORE & operator = (const OperatorCORE &);

};
#endif


