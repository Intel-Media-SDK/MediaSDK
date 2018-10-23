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

#ifndef __LIBMFX_CORE_INTERFACE_H__
#define __LIBMFX_CORE_INTERFACE_H__

#include "mfx_common.h"
#include <mfxvideo++int.h>

// {1F5BB140-6BB4-416e-81FF-4A8C030FBDC6}
static const
MFX_GUID  MFXIVideoCORE_GUID =
{ 0x1f5bb140, 0x6bb4, 0x416e, { 0x81, 0xff, 0x4a, 0x8c, 0x3, 0xf, 0xbd, 0xc6 } };

// {3F3C724E-E7DC-469a-A062-B6A23102F7D2}
static const
MFX_GUID  MFXICORED3D_GUID =
{ 0x3f3c724e, 0xe7dc, 0x469a, { 0xa0, 0x62, 0xb6, 0xa2, 0x31, 0x2, 0xf7, 0xd2 } };

// {C9613F63-3EA3-4D8C-8B5C-96AF6DC2DB0F}
static const MFX_GUID  MFXICORED3D11_GUID =
{ 0xc9613f63, 0x3ea3, 0x4d8c, { 0x8b, 0x5c, 0x96, 0xaf, 0x6d, 0xc2, 0xdb, 0xf } };

// {B0FCB183-1A6D-4f00-8BAF-93F285ACEC93}
static const MFX_GUID MFXICOREVAAPI_GUID =
{ 0xb0fcb183, 0x1a6d, 0x4f00, { 0x8b, 0xaf, 0x93, 0xf2, 0x85, 0xac, 0xec, 0x93 } };

// {86dc1aab-eb20-47a2-a461-428a7bd60183}
static const MFX_GUID MFXICOREVDAAPI_GUID =
{ 0x86dc1aab, 0xeb20, 0x47a2, {0xa4, 0x61, 0x42, 0x8a, 0x7b, 0xd6, 0x01, 0x83 } };


// {EA851C02-7F04-4126-9045-48D8282434A5}
static const MFX_GUID MFXICORE_API_1_19_GUID =
{ 0xea851c02, 0x7f04, 0x4126, { 0x90, 0x45, 0x48, 0xd8, 0x28, 0x24, 0x34, 0xa5 } };


// {6ED94B99-DB70-4EBB-BC5C-C7E348FC2396}
static const
MFX_GUID  MFXIHWCAPS_GUID =
{ 0x6ed94b99, 0xdb70, 0x4ebb, {0xbc, 0x5c, 0xc7, 0xe3, 0x48, 0xfc, 0x23, 0x96 } };

// {0CF4CE38-EA46-456d-A179-8A026AE4E101}
static const
MFX_GUID  MFXIHWMBPROCRATE_GUID =
{0xcf4ce38, 0xea46, 0x456d, {0xa1, 0x79, 0x8a, 0x2, 0x6a, 0xe4, 0xe1, 0x1} };

// {6A0665ED-2DE0-403B-A5EE-944E5AAFA8E5}
static const
MFX_GUID  MFXID3D11DECODER_GUID =
{0x6a0665ed, 0x2de0, 0x403b, {0xa5, 0xee, 0x94, 0x4e, 0x5a, 0xaf, 0xa8, 0xe5} };

static const
MFX_GUID MFXICORECM_GUID =
{0xe0b78bba, 0x39d9, 0x48dc,{ 0x99, 0x29, 0xc5, 0xd6, 0x5e, 0xa, 0x6a, 0x66} };

// {1D143E80-4EA8-4238-989C-3A3ED894EFEF}
static const
MFX_GUID MFXICORECMCOPYWRAPPER_GUID =
{ 0x1d143e80, 0x4ea8, 0x4238, { 0x98, 0x9c, 0x3a, 0x3e, 0xd8, 0x94, 0xef, 0xef } };

static const
MFX_GUID  MFXIEXTERNALLOC_GUID =
{ 0x3e273bfb, 0x5e28, 0x4643, { 0x9f, 0x1d, 0x25, 0x4b, 0x0, 0xb, 0xeb, 0x96 } };


// {2AAFDAE8-F7BA-46ED-B277-B87E94F2D384}
static const
MFX_GUID MFXICMEnabledCore_GUID =
{ 0x2aafdae8, 0xf7ba, 0x46ed, { 0xb2, 0x77, 0xb8, 0x7e, 0x94, 0xf2, 0xd3, 0x84 } };

#ifdef MFX_ENABLE_MFE

//to keep core interface unchanged we need to define 2 guids:
// MFXMFEDDIENCODER_GUID - for returing MFE adapter by request - need for core to query other cores
// MFXMFEDDIENCODER_SEARCH_GUID - called by encoder to search through joined sessions
// cores by MFXMFEDDIENCODER_GUID, to verify adapter availability and return when exist.
// {E4E4823F-90D2-4945-823E-00B5F3F3184C}
static const
MFX_GUID MFXMFEDDIENCODER_GUID =
{ 0xe4e4823f, 0x90d2, 0x4945, { 0x82, 0x3e, 0x0, 0xb5, 0xf3, 0xf3, 0x18, 0x4c } };

// {AAA16189-4E5A-4DA9-BB97-4CD1B0BAAC73}
static const MFX_GUID MFXMFEDDIENCODER_SEARCH_GUID =
{ 0xaaa16189, 0x4e5a, 0x4da9, { 0xbb, 0x97, 0x4c, 0xd1, 0xb0, 0xba, 0xac, 0x73 } };

#endif

// {D53EF10E-D4CF-41A7-B1C2-D30FAB30BB64}
static const MFX_GUID MFXICORE_GT_CONFIG_GUID =
{ 0xd53ef10e, 0xd4cf, 0x41a7,{ 0xb1, 0xc2, 0xd3, 0xf, 0xab, 0x30, 0xbb, 0x64 } };

// {7DF28D19-889A-45C1-AA05-A4F7EFAE9528}
static const MFX_GUID MFXIFEIEnabled_GUID =
{ 0x7df28d19, 0x889a, 0x45c1,{ 0xaa, 0x5, 0xa4, 0xf7, 0xef, 0xae, 0x95, 0x28 } };

// Try to obtain required interface
// Declare a template to query an interface
template <class T> inline
T *QueryCoreInterface(VideoCORE* pUnk, const MFX_GUID &guid = T::getGuid())
{
    void *pInterface = NULL;

    // query the interface
    if (pUnk)
    {
        pInterface = pUnk->QueryCoreInterface(guid);
        // cast pointer returned to the required core interface
        return reinterpret_cast<T *>(pInterface);
    }
    else
    {
        return NULL;
    }

} // T *QueryCoreInterface(MFXIUnknown *pUnk, const MFX_GUID &guid)

class EncodeHWCaps
{
public:
    static const MFX_GUID getGuid()
    {
        return MFXIHWCAPS_GUID;
    }
    EncodeHWCaps()
        : m_encode_guid()
        , m_caps()
        , m_size()
    {
    };
    virtual ~EncodeHWCaps()
    {
        if (m_caps)
        {
            free(m_caps);
            m_caps = NULL;
        }
    };
    template <class CAPS>
    mfxStatus GetHWCaps(GUID encode_guid, CAPS *hwCaps, mfxU32 array_size = 1)
    {
        if (m_caps)
        {
            if (encode_guid == m_encode_guid && m_size == array_size)
            {
                CAPS* src = reinterpret_cast<CAPS*>(m_caps);
                std::copy(src, src + array_size, hwCaps);
                return MFX_ERR_NONE;
            }
        }
        return MFX_ERR_UNDEFINED_BEHAVIOR;

    }
    template <class CAPS>
    mfxStatus SetHWCaps(GUID encode_guid, CAPS *hwCaps, mfxU32 array_size = 1)
    {
        if (!m_caps)
        {
            m_encode_guid = encode_guid;
            m_size = array_size;
            m_caps = malloc(sizeof(CAPS)*array_size);
            if (!m_caps)
                return MFX_ERR_MEMORY_ALLOC;

            std::copy(hwCaps, hwCaps + array_size, reinterpret_cast<CAPS*>(m_caps));
        }
        else
        {
            m_encode_guid = encode_guid;
            m_size = array_size;
            std::copy(hwCaps, hwCaps + array_size, reinterpret_cast<CAPS*>(m_caps));
        }
        return MFX_ERR_NONE;

    }
protected:
    GUID   m_encode_guid;
    mfxHDL m_caps;
    mfxU32 m_size;
private:
    EncodeHWCaps(const EncodeHWCaps&);
    void operator=(const EncodeHWCaps&);

};

template <class T>
class ComPtrCore
{
public:
    static const MFX_GUID getGuid()
    {

#if !defined(MFX_ENABLE_MFE)
        return MFXID3D11DECODER_GUID;
#else
        return MFXMFEDDIENCODER_GUID;
#endif
    }
    ComPtrCore():m_pComPtr(NULL)
    {
    };
    virtual ~ComPtrCore()
    {
        if (m_pComPtr)
        {
            m_pComPtr->Release();
            m_pComPtr = NULL;
        }
    };
    ComPtrCore& operator = (T* ptr)
    {
        if (m_pComPtr != ptr)
        {
            if (m_pComPtr)
                m_pComPtr->Release();

            m_pComPtr = ptr;
        }
        return *this;
    };
    T* get()
    {
        return m_pComPtr;
    };

protected:
T* m_pComPtr;
};

#if defined (MFX_VA_LINUX)
    struct VAAPIInterface
    {
        static const MFX_GUID & getGuid()
        {
            return MFXICOREVAAPI_GUID;
        }
    };

#endif

struct CMEnabledCoreInterface
{
    static const MFX_GUID & getGuid()
    {
        return MFXICMEnabledCore_GUID;
    }

    virtual mfxStatus SetCmCopyStatus(bool enable) = 0;
    virtual ~CMEnabledCoreInterface() {}
};


#endif // __LIBMFX_CORE_INTERFACE_H__
/* EOF */
