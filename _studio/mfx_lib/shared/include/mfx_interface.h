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

#ifndef __MFX_INTERFACE_H
#define __MFX_INTERFACE_H

#include <mfxdefs.h>
#include <stdio.h>


#pragma pack(1)

// Declare the internal GUID type
typedef
struct MFX_GUID
{
    mfxU32 Data1;
    mfxU16 Data2;
    mfxU16 Data3;
    mfxU8 Data4[8];

} MFX_GUID;

#pragma pack()

// Declare the function to compare GUIDs
inline
bool operator == (const MFX_GUID &guid0, const MFX_GUID &guid1)
{
    const mfxU32 *pGUID0 = (const mfxU32 *) &guid0;
    const mfxU32 *pGUID1 = (const mfxU32 *) &guid1;

    if ((pGUID0[0] != pGUID1[0]) ||
        (pGUID0[1] != pGUID1[1]) ||
        (pGUID0[2] != pGUID1[2]) ||
        (pGUID0[3] != pGUID1[3]))
    {
        return false;
    }

    return true;

} // bool operator == (const MFX_GUID &guid0, const MFX_GUID &guid1)

// Declare base interface class
class MFXIUnknown
{
public:
    virtual ~MFXIUnknown(void){}
    // Query another interface from the object. If the pointer returned is not NULL,
    // the reference counter is incremented automatically.
    virtual
    void *QueryInterface(const MFX_GUID &guid) = 0;

    // Increment reference counter of the object.
    virtual
    void AddRef(void) = 0;
    // Decrement reference counter of the object.
    // If the counter is equal to zero, destructor is called and
    // object is removed from the memory.
    virtual
    void Release(void) = 0;
    // Get the current reference counter value
    virtual
    mfxU32 GetNumRef(void) const = 0;
};

// Declaration of function to create object to support required interface
template <class T>
T* CreateInterfaceInstance(const MFX_GUID &guid);

// Declare a template to query an interface
template <class T> inline
T *QueryInterface(MFXIUnknown* &pUnk, const MFX_GUID &guid)
{
    void *pInterface = NULL;

    // have to create instance of required interface corresponding to GUID
    if (!pUnk)
    {
        pUnk = CreateInterfaceInstance<T>(guid);
    }

    // query the interface
    if (pUnk)
    {
        pInterface = pUnk->QueryInterface(guid);
    }

    // cast pointer returned to the required interface
    return (T *) pInterface;

} // T *QueryInterface(MFXIUnknown *pUnk, const MFX_GUID &guid)


template <class T>
class MFXIPtr
{
public:
    // Default constructor
    MFXIPtr(void) : m_pInterface(0)
    {
    }
    // Constructors
    MFXIPtr(const MFXIPtr &iPtr) : m_pInterface(0)
    {
        operator = (iPtr);
    }

    explicit
    MFXIPtr(void *pInterface) : m_pInterface(0)
    {
        operator = (pInterface);
    }

    // Destructor
    ~MFXIPtr(void)
    {
        Release();
    }

    // Cast operator
    T * operator -> (void) const
    {
        return m_pInterface;
    }

    MFXIPtr & operator = (const MFXIPtr &iPtr)
    {
        // release the interface before setting new one
        Release();

        // save the pointer to interface
        m_pInterface = iPtr.m_pInterface;
        if (m_pInterface)
        {
            // increment the reference counter
            m_pInterface->AddRef();
        }

        return *this;
    }

    MFXIPtr & operator = (void *pInterface)
    {
        // release the interface before setting new one
        Release();

        // save the pointer to interface
        m_pInterface = (T *) pInterface;
        // There is no need to increment the reference counter,
        // because it is supposed that the pointer is returned from
        // QueryInterface function.

        return *this;
    }

    inline
    bool operator == (const void *p) const
    {
        return (p == m_pInterface);
    }

    inline operator bool() const
    {
        return m_pInterface != 0;
    }


protected:
    void Release(void)
    {
        if (m_pInterface)
        {
            // decrement the reference counter of the interface
            m_pInterface->Release();
            m_pInterface = NULL;
        }
    }

    T *m_pInterface;                                            // (T *) pointer to the interface
};

template <class T> inline
bool operator == (void *p, const MFXIPtr<T> &ptr)
{
    return (ptr == p);

} // bool operator == (void *p, const MFXIPtr &ptr)

#endif // __MFX_INTERFACE_H
