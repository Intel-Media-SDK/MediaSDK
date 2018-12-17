/* ****************************************************************************** *\

Copyright (C) 2012-2018 Intel Corporation.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
- Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
- Neither the name of Intel Corporation nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY INTEL CORPORATION "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL INTEL CORPORATION BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

File Name: mfx_library_iterator.h

\* ****************************************************************************** */

#if !defined(__MFX_LIBRARY_ITERATOR_H)
#define __MFX_LIBRARY_ITERATOR_H


#include <mfxvideo.h>

#if defined(MEDIASDK_USE_REGISTRY) || (!defined(MEDIASDK_DFP_LOADER) && !defined(MEDIASDK_UWP_PROCTABLE))
#include "mfx_win_reg_key.h"
#endif

#include "mfx_dispatcher.h"

namespace MFX
{

// declare desired storage ID
enum
{
    MFX_UNKNOWN_KEY             = -1,
    MFX_CURRENT_USER_KEY        = 0,
    MFX_LOCAL_MACHINE_KEY       = 1,
    MFX_APP_FOLDER              = 2,
    MFX_STORAGE_ID_FIRST    = MFX_CURRENT_USER_KEY,
    MFX_STORAGE_ID_LAST     = MFX_LOCAL_MACHINE_KEY
};

// Try to initialize using given implementation type. Select appropriate type automatically in case of MFX_IMPL_VIA_ANY.
// Params: adapterNum - in, pImplInterface - in/out, pVendorID - out, pDeviceID - out
mfxStatus SelectImplementationType(const mfxU32 adapterNum, mfxIMPL *pImplInterface, mfxU32 *pVendorID, mfxU32 *pDeviceID);

const mfxU32 msdk_disp_path_len = 1024;

class MFXLibraryIterator
{
public:
    // Default constructor
    MFXLibraryIterator(void);
    // Destructor
    ~MFXLibraryIterator(void);

    // Initialize the iterator
    mfxStatus Init(eMfxImplType implType, mfxIMPL implInterface, const mfxU32 adapterNum, int storageID);

    // Get the next library path
    mfxStatus SelectDLLVersion(wchar_t *pPath, size_t pathSize,
                               eMfxImplType *pImplType, mfxVersion minVersion);

    // Return interface type on which Intel adapter was found (if any): D3D9 or D3D11
    mfxIMPL GetImplementationType();

    // Retrun registry subkey name on which dll was selected after sucesfull call to selectDllVesion
    bool GetSubKeyName(wchar_t *subKeyName, size_t length) const;

    int  GetStorageID() const { return m_StorageID; }
protected:

    // Release the iterator
    void Release(void);

    // Initialize the registry iterator
    mfxStatus InitRegistry(eMfxImplType implType, mfxIMPL implInterface, const mfxU32 adapterNum, int storageID);
    // Initialize the app/module folder iterator
    mfxStatus InitFolder(eMfxImplType implType, mfxIMPL implInterface, const mfxU32 adapterNum, const wchar_t * path, const int storageID);


    eMfxImplType m_implType;                                    // Required library implementation
    mfxIMPL m_implInterface;                                    // Required interface (D3D9, D3D11)

    mfxU32 m_vendorID;                                          // (mfxU32) property of used graphic card
    mfxU32 m_deviceID;                                          // (mfxU32) property of used graphic card
    bool   m_bIsSubKeyValid;
    wchar_t m_SubKeyName[MFX_MAX_REGISTRY_KEY_NAME];            // registry subkey for selected module loaded
    int    m_StorageID;

#if defined(MEDIASDK_USE_REGISTRY) || (!defined(MEDIASDK_DFP_LOADER) && !defined(MEDIASDK_UWP_PROCTABLE))
    WinRegKey m_baseRegKey;                                     // (WinRegKey) main registry key
#endif

    mfxU32 m_lastLibIndex;                                      // (mfxU32) index of previously returned library
    mfxU32 m_lastLibMerit;                                      // (mfxU32) merit of previously returned library

    wchar_t  m_path[msdk_disp_path_len];

private:
    // unimplemented by intent to make this class non-copyable
    MFXLibraryIterator(const MFXLibraryIterator &);
    void operator=(const MFXLibraryIterator &);
};

} // namespace MFX

#endif // __MFX_LIBRARY_ITERATOR_H
