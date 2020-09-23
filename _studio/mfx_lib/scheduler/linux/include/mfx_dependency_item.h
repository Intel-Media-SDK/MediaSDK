// Copyright (c) 2017-2020 Intel Corporation
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

#if !defined(__MFX_DEPENDENCY_ITEM_H)
#define __MFX_DEPENDENCY_ITEM_H

#include <mfxdefs.h>

// Forward declaration of used classes.
class mfxDependencyItemInterface;

// Declare base type to bind dependent items into lists.
struct MFX_DEPENDENCY_LIST_ITEM
{
    // Pointer to the dependent object
    mfxDependencyItemInterface *pObj;

    // Pointer to the next dependent item in the list
    MFX_DEPENDENCY_LIST_ITEM *pNext;
    // Pointer to the previous dependent item in the list
    MFX_DEPENDENCY_LIST_ITEM *pPrev;
};

class mfxDependencyItemInterface
{
public:

    virtual ~mfxDependencyItemInterface() {};

    //
    // Methods for the base object. Some others objects may be dependent on it.
    //

    // Add new item to the list of items
    // which are dependent on the current object.
    virtual
    void SetDependentItem(mfxDependencyItemInterface *pDependentObject,
                          int levelDependency) = 0;

    // The object is ready, and there is no more dependency for waiting items.
    // Unlink them from the dependency list, update status if any.
    virtual
    void ResolveDependencies(mfxStatus result) = 0;

    //
    // Methods for the dependent object. Such an object is need to wait until
    // some base objects become available or ready.
    //

    // Link the current object into the dependency list.
    virtual
    MFX_DEPENDENCY_LIST_ITEM *SetNextDependentItem(MFX_DEPENDENCY_LIST_ITEM *pNext,
                                                   int levelDependency) = 0;

    // Update the dependent object with the status of parent object
    virtual
    void OnDependencyResolved(mfxStatus result) = 0;

    // Check that all dependencies are resolved and
    // the current object can be processed.
    virtual
    bool IsDependenciesResolved(void) const = 0;

    // Get the status of dependencies resolved.
    virtual
    bool GetDependencyStatus(void) const = 0;

    // Reset the object into the initial state.
    virtual
    void ResetDependency(void) = 0;
};

template <const int dependency_level>
class mfxDependencyItem : public mfxDependencyItemInterface
{
public:
    mfxDependencyItem(void)
    {
        ResetDependency();
    }

    virtual
    ~mfxDependencyItem(void)
    {

    }

    // Add new item to the list of items
    // which are dependent on the current object.
    virtual
    void SetDependentItem(mfxDependencyItemInterface *pDependentObject,
                          int levelDependency)
    {
        MFX_DEPENDENCY_LIST_ITEM *pNewList;

        pNewList = pDependentObject->SetNextDependentItem(m_beginListObjects.pNext,
                                                          levelDependency);
        if (pNewList)
        {
            m_beginListObjects.pNext = pNewList;
            pNewList->pPrev = &m_beginListObjects;
        }
    }

    // The item is ready, and there is no more dependency for waiting items.
    // Unlink them from the dependency list, update status if any.
    virtual
    void ResolveDependencies(mfxStatus result)
    {
        MFX_DEPENDENCY_LIST_ITEM *pObjects = m_beginListObjects.pNext;

        // run over the dependent object and let them know,
        // that one of dependency is ready
        while (&m_endListObjects != pObjects && pObjects)
        {
            MFX_DEPENDENCY_LIST_ITEM *pTemp = pObjects->pNext;

            pObjects->pNext = 0;
            pObjects->pPrev = 0;
            pObjects->pObj->OnDependencyResolved(result);

            pObjects = pTemp;
        }

        // terminate the list of the dependent objects
        m_beginListObjects.pNext = &m_endListObjects;
        m_endListObjects.pPrev = &m_beginListObjects;
    }

    // Link the current item into the dependency list.
    virtual
    MFX_DEPENDENCY_LIST_ITEM *SetNextDependentItem(MFX_DEPENDENCY_LIST_ITEM *pNext,
                                                   int levelDependency)
    {
        // check error(s)
        if (levelDependency >= dependency_level)
        {
            return (MFX_DEPENDENCY_LIST_ITEM *) 0;
        }

        // link the given list to the object
        m_dependency[levelDependency].pNext = pNext;
        pNext->pPrev = m_dependency + levelDependency;

        // return the new beginning of the list
        return m_dependency + levelDependency;
    }

    // Update the dependent object with the status of the parent object
    virtual
    void OnDependencyResolved(mfxStatus result)
    {
        if (MFX_ERR_NONE != result)
        {
            int i;

            // unlink item from all dependency lists
            for (i = 0; i < dependency_level; i += 1)
            {
                if (m_dependency[i].pNext)
                {
                    m_dependency[i].pNext->pPrev = m_dependency[i].pPrev;
                }
                if (m_dependency[i].pPrev)
                {
                    m_dependency[i].pPrev->pNext = m_dependency[i].pNext;
                }
                m_dependency[i].pNext = 0;
                m_dependency[i].pPrev = 0;
            }
  
            // make all subsequent objects know about the error
            ResolveDependencies(MFX_ERR_ABORTED);
        }

        // Actually, there is may be additional activity in inherited classes.
    }

    // Check that all dependencies are resoulved and
    // the current item can be processed.
    virtual
    bool IsDependenciesResolved(void) const
    {
        bool bDependenciesResolved = true;
        int i;

        for (i = 0; i < dependency_level; i += 1)
        {
            if (m_dependency[i].pNext)
            {
                bDependenciesResolved = false;
                break;
            }
        }

        return bDependenciesResolved;
    }

    // Get the status of dependencies resolved.
    virtual
    bool GetDependencyStatus(void) const
    {
        return m_bStatusOk;
    }

    // Reset the object into the initial state.
    virtual
    void ResetDependency(void)
    {
        int i;

        m_bStatusOk = true;

        // terminate the dependent object list
        m_beginListObjects.pObj = 0;
        m_beginListObjects.pNext = &m_endListObjects;
        m_beginListObjects.pPrev = 0;
        m_endListObjects.pObj = 0;
        m_endListObjects.pNext = &m_beginListObjects;
        m_endListObjects.pPrev = 0;
        // reset all list entries
        for (i = 0; i < dependency_level; i += 1)
        {
            m_dependency[i].pObj = this;
            m_dependency[i].pNext = 0;
            m_dependency[i].pPrev = 0;
        }
    }

protected:

    // Status of the preceding items the current object dependent on
    bool m_bStatusOk;
    // Pointer to a list of objects, which are dependent on the current object.
    MFX_DEPENDENCY_LIST_ITEM m_beginListObjects;
    MFX_DEPENDENCY_LIST_ITEM m_endListObjects;
    // Pointer to the next item in the corresponding dependency list
    MFX_DEPENDENCY_LIST_ITEM m_dependency[dependency_level];
};

#endif // __MFX_DEPENDENCY_ITEM_H
