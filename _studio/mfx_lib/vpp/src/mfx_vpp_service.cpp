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

#include "mfx_common.h"

#if defined (MFX_ENABLE_VPP)
#include "mfx_vpp_service.h"

DynamicFramesPool::DynamicFramesPool( void )
{
    m_numFrames     = 0;
    m_mids          = NULL;
    m_core          = NULL;
    m_pSurface      = NULL;
    m_ptrDataTab    = NULL;
    m_savedLocked   = NULL;
    m_bLockedOutput = NULL;
    m_allocated     = false;
}

DynamicFramesPool::~DynamicFramesPool()
{
    Close();
}


mfxStatus DynamicFramesPool::Init( VideoCORE *core, mfxFrameAllocRequest* pRequest, bool needAlloc)
{
    mfxFrameAllocResponse response;
    mfxStatus sts;
    mfxU16    frame = 0;

    MFX_CHECK_NULL_PTR2( pRequest, core );

    m_core = core;

    m_numFrames = pRequest->NumFrameMin;

    m_pSurface    = new mfxFrameSurface1 [ m_numFrames ];
    m_ptrDataTab  = new mfxFrameData* [ m_numFrames ];
    m_savedLocked = new mfxU16 [ m_numFrames ];
    m_bLockedOutput = new bool [ m_numFrames ];

    sts = Reset( );
    MFX_CHECK_STS( sts );

    m_allocated = needAlloc;
    if (needAlloc)
    {
        sts = m_core->AllocFrames( pRequest, &response);
        MFX_CHECK_STS(sts);

        m_mids = response.mids;

        for( frame = 0; frame < m_numFrames; frame++ )
        {
            memset(&(m_pSurface[frame].Data),0,sizeof(mfxFrameData));
            m_pSurface[frame].Data.MemId = response.mids[frame];
            m_pSurface[frame].Info       = pRequest->Info;
        }
    }

    return MFX_ERR_NONE;
}


mfxStatus DynamicFramesPool::Reset( void )
{
    mfxU32 frame = 0;

    for( frame = 0; frame < m_numFrames; frame++ )
    {
        m_pSurface[frame].Data.Locked = 0;
        m_pSurface[frame].Data.MemId  = NULL;
        m_ptrDataTab[frame]           = NULL;
        m_savedLocked[frame]          = 0;
    }

    return MFX_ERR_NONE;
}


mfxStatus DynamicFramesPool::Close( void )
{
    mfxFrameAllocResponse response;
    mfxStatus             sts = MFX_ERR_NONE;
    mfxU32 frame = 0;

    for( frame = 0; frame < m_numFrames; frame++ )
    {
        mfxI32 i;

        for (i = 0; i < m_pSurface[frame].Data.Locked; i++)
        {
            sts = m_core->DecreaseReference( m_ptrDataTab[frame] );
        }
    }

    if (m_allocated)
    {
        if( m_numFrames)
        {
            response.mids = m_mids;
            response.NumFrameActual = m_numFrames;
            m_core->FreeFrames( &response );
        }
    }

    Reset();

    if( m_pSurface )
    {
        delete [] m_pSurface;
    }

    if( m_ptrDataTab )
    {
        delete [] m_ptrDataTab;
    }

    if( m_savedLocked )
    {
        delete [] m_savedLocked;
    }

    if( m_bLockedOutput )
    {
        delete [] m_bLockedOutput;
    }

    m_numFrames     = 0;
    m_mids          = NULL;
    m_core          = NULL;
    m_pSurface      = NULL;
    m_ptrDataTab    = NULL;
    m_savedLocked   = NULL;
    m_bLockedOutput = NULL;
    m_allocated     = false;

    return sts;
}


mfxStatus DynamicFramesPool::Lock( void )
{
    mfxStatus sts;
    mfxU16 frame;

    for (frame = 0; frame < m_numFrames; frame++)
    {
        sts = m_core->LockFrame(m_mids[frame], &(m_pSurface[frame].Data));
        MFX_CHECK_STS(sts);
    }

    return MFX_ERR_NONE;
}


mfxStatus DynamicFramesPool::Unlock( void )
{
    mfxU16 frame;
    mfxStatus sts;

    for(frame = 0; frame < m_numFrames; frame++)
    {
        sts = m_core->UnlockFrame(m_mids[frame], &(m_pSurface[frame].Data));
        MFX_CHECK_STS( sts );
    }

    return MFX_ERR_NONE;
}


mfxStatus DynamicFramesPool::GetFreeSurface(mfxFrameSurface1** ppSurface)
{
    mfxStatus sts;
    bool bError = true;
    mfxU16 frame;
    mfxU16 indx = 0;

    MFX_CHECK_NULL_PTR1(ppSurface);

    for (frame = 0; frame < m_numFrames; frame++)
    {
        if (0 == m_pSurface[frame].Data.Locked)
        {
            indx = frame;
            bError = false;
            break;
        }
    }

    if( bError )
    {
        *ppSurface = NULL;
        sts = MFX_ERR_NULL_PTR;
    }
    else
    {
        *ppSurface = &(m_pSurface[indx]);
        sts = MFX_ERR_NONE;
    }

    return sts;
}


mfxStatus DynamicFramesPool::Zero( void )
{

    m_numFrames     = 0;
    m_mids          = NULL;
    m_pSurface      = NULL;
    m_ptrDataTab    = NULL;
    m_savedLocked   = NULL;
    m_bLockedOutput = NULL;
    m_allocated     = false;


    return MFX_ERR_NONE;
}


mfxStatus DynamicFramesPool::SetCrop( mfxFrameInfo* pInfo )
{
    MFX_CHECK_NULL_PTR1( pInfo );

    /*mfxU16 frameIndex = 0;

    for( frameIndex = 0; frameIndex < m_numFrames; frameIndex++ )
    {
        m_pSurface[frameIndex].Info.CropX  = pInfo->CropX;
        m_pSurface[frameIndex].Info.CropY  = pInfo->CropY;
        m_pSurface[frameIndex].Info.CropW  = pInfo->CropW;
        m_pSurface[frameIndex].Info.CropH  = pInfo->CropH;
    }*/

    mfxFrameSurface1* pSurface;
    mfxStatus sts = GetFreeSurface( &pSurface );
    MFX_CHECK_STS( sts );

    pSurface->Info.CropX  = pInfo->CropX;
    pSurface->Info.CropY  = pInfo->CropY;
    pSurface->Info.CropW  = pInfo->CropW;
    pSurface->Info.CropH  = pInfo->CropH;

    return MFX_ERR_NONE;
}

mfxStatus DynamicFramesPool::PreProcessSync( void )
{

    mfxU16 frame = 0;
    mfxStatus sts;

    for( frame = 0; frame < m_numFrames; frame++ )
    {
        m_bLockedOutput[frame] = false;

        if (0 != m_pSurface[frame].Data.Locked)
        {
            if (m_ptrDataTab[frame]->Y == 0)
            {
                sts = m_core->LockExternalFrame(m_ptrDataTab[frame]->MemId, &m_pSurface[frame].Data);
                m_pSurface[frame].Data.MemId = m_ptrDataTab[frame]->MemId;
                MFX_CHECK_STS( sts );
                m_bLockedOutput[frame] = true;
            }
            else
            {
                // patch
                m_savedLocked[frame] = m_pSurface[frame].Data.Locked;
                m_pSurface[frame].Data = *m_ptrDataTab[frame];
                // patch
                m_pSurface[frame].Data.Locked = m_savedLocked[frame];
            }
        }

         m_savedLocked[frame] = m_pSurface[frame].Data.Locked;
    }

    return MFX_ERR_NONE;
}

mfxStatus DynamicFramesPool::PostProcessSync( void )
{

    mfxU16 frame = 0;
    mfxStatus sts;

    for( frame = 0; frame < m_numFrames; frame++ )
    {
        if (m_bLockedOutput[frame])
        {
            sts = m_core->UnlockExternalFrame(m_ptrDataTab[frame]->MemId, &m_pSurface[frame].Data);
            MFX_CHECK_STS( sts );
        }

        if (m_savedLocked[frame] < m_pSurface[frame].Data.Locked)
        {
            mfxI32 i;

            for (i = m_savedLocked[frame]; i < m_pSurface[frame].Data.Locked; i++)
            {
                sts = m_core->IncreaseReference( m_ptrDataTab[frame] );
            }
        }
        else if (m_savedLocked[frame] > m_pSurface[frame].Data.Locked)
        {
            mfxI32 i;

            for (i = m_pSurface[frame].Data.Locked; i < m_savedLocked[frame]; i++)
            {
                sts = m_core->DecreaseReference( m_ptrDataTab[frame] );
            }
        }
        m_savedLocked[frame] = m_pSurface[frame].Data.Locked;
    }

    return MFX_ERR_NONE;
}

mfxStatus DynamicFramesPool::GetFilledSurface(mfxFrameSurface1*  in,
                                              mfxFrameSurface1** ppSurface)
{
    mfxStatus sts;
    bool bError = true;
    mfxU16 frame;
    mfxU16 indx = 0;

    MFX_CHECK_NULL_PTR2(in, ppSurface);
    *ppSurface = NULL;

    for (frame = 0; frame < m_numFrames; frame++)
    {
        if (0 == m_pSurface[frame].Data.Locked)
        {
            indx = frame;
            bError = false;
            break;
        }
    }

    if( bError )
    {
        return MFX_ERR_NULL_PTR;
    }

    if (in->Data.Y == 0)
    {
        sts = m_core->LockExternalFrame(in->Data.MemId, &m_pSurface[indx].Data);
        m_pSurface[indx].Data.MemId = in->Data.MemId;
        m_pSurface[indx].Info = in->Info;
        MFX_CHECK_STS( sts );
        m_bLockedOutput[indx] = true;
    }
    else
    {
        m_pSurface[indx] = *in;
    }

    m_ptrDataTab[indx] = &in->Data;
    m_savedLocked[indx] = m_pSurface[indx].Data.Locked = 1;
    *ppSurface = &(m_pSurface[indx]);

    return MFX_ERR_NONE;
}

#endif // MFX_ENABLE_VPP
/* EOF */
