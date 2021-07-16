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

#include "mfx_common.h"

#include "cm_mem_copy.h"

#include "mfx_common_int.h"

typedef const mfxU8 mfxUC8;

#define ALIGN128(X) (((mfxU32)((X)+127)) & (~ (mfxU32)127))

#define CHECK_CM_STATUS(_sts, _ret)              \
        if (CM_SUCCESS != _sts)                  \
        {                                           \
            return _ret;                        \
        }

#define CHECK_CM_STATUS_RET_NULL(_sts, _ret)              \
        if (CM_SUCCESS != _sts)                  \
        {                                           \
            return NULL;                        \
        }

#define CHECK_CM_NULL_PTR(_ptr, _ret)              \
        if (NULL == _ptr)                  \
        {                                           \
            return _ret;                        \
        }

CmCopyWrapper::CmCopyWrapper()
    : m_guard()
{
    m_HWType = MFX_HW_UNKNOWN;
    m_pCmProgram = NULL;
    m_pCmDevice  = NULL;

    m_pCmSurface2D = NULL;
    m_pCmUserBuffer = NULL;

    m_pCmSrcIndex = NULL;
    m_pCmDstIndex = NULL;

    m_pCmQueue = NULL;
    m_pCmTask1 = NULL;
    m_pCmTask2 = NULL;

    m_pThreadSpace = NULL;


    m_cachedObjects.clear();

    m_tableCmRelations.clear();
    m_tableSysRelations.clear();

    m_tableCmIndex.clear();
    m_tableSysIndex.clear();

    m_tableCmRelations2.clear();
    m_tableSysRelations2.clear();

    m_tableCmIndex2.clear();
    m_tableSysIndex2.clear();

    m_surfacesInCreationOrder.clear();
    m_buffersInCreationOrder.clear();
    m_timeout = 0;
} // CmCopyWrapper::CmCopyWrapper(void)
CmCopyWrapper::~CmCopyWrapper(void)
{
    Release();

} // CmCopyWrapper::~CmCopyWrapper(void)

#define ADDRESS_PAGE_ALIGNMENT_MASK_X64             0xFFFFFFFFFFFFF000ULL
#define ADDRESS_PAGE_ALIGNMENT_MASK_X86             0xFFFFF000
#define INNER_LOOP                   (4)

#define CHECK_CM_HR(HR) \
    if(HR != CM_SUCCESS)\
    {\
        if(pTS)           m_pCmDevice->DestroyThreadSpace(pTS);\
        if(pGPUCopyTask)  m_pCmDevice->DestroyTask(pGPUCopyTask);\
        if(pCMBufferUP)   m_pCmDevice->DestroyBufferUP(pCMBufferUP);\
        if(pInternalEvent)m_pCmQueue->DestroyEvent(pInternalEvent);\
        return MFX_ERR_DEVICE_FAILED;\
    }

bool CmCopyWrapper::isSinglePlainFormat(mfxU32 format)
{
    switch (format)
    {
    case MFX_FOURCC_NV12:
    case MFX_FOURCC_P010:
    case MFX_FOURCC_YV12:
    case MFX_FOURCC_NV16:
    case MFX_FOURCC_P210:
#if (MFX_VERSION >= 1031)
    case MFX_FOURCC_P016:
#endif
        return false;
    case MFX_FOURCC_BGR4:
    case MFX_FOURCC_RGB4:
    case MFX_FOURCC_P8:
    case MFX_FOURCC_A2RGB10:
    case MFX_FOURCC_ARGB16:
    case MFX_FOURCC_ABGR16:
    case MFX_FOURCC_R16:
    case MFX_FOURCC_AYUV:
    case MFX_FOURCC_AYUV_RGB4:
    case MFX_FOURCC_UYVY:
    case MFX_FOURCC_YUY2:
#if (MFX_VERSION >= 1027)
    case MFX_FOURCC_Y210:
    case MFX_FOURCC_Y410:
#endif
#if (MFX_VERSION >= 1031)
    case MFX_FOURCC_Y216:
    case MFX_FOURCC_Y416:
#endif
#ifdef MFX_ENABLE_RGBP
    case MFX_FOURCC_RGBP:
#endif

        return true;
    }
    return false;
}

bool CmCopyWrapper::isNV12LikeFormat(mfxU32 format)
{
    switch (format)
    {
    case MFX_FOURCC_NV12:
    case MFX_FOURCC_P010:
#if (MFX_VERSION >= 1031)
    case MFX_FOURCC_P016:
#endif
        return true;
    }
    return false;
}

int CmCopyWrapper::getSizePerPixel(mfxU32 format)
{
    switch (format)
    {
    case MFX_FOURCC_P8:
        return 1;
    case MFX_FOURCC_R16:
    case MFX_FOURCC_UYVY:
    case MFX_FOURCC_YUY2:
        return 2;
    case MFX_FOURCC_BGR4:
    case MFX_FOURCC_RGB4:
    case MFX_FOURCC_A2RGB10:
    case MFX_FOURCC_AYUV:
    case MFX_FOURCC_AYUV_RGB4:
#if (MFX_VERSION >= 1027)
    case MFX_FOURCC_Y410:
    case MFX_FOURCC_Y210:
#endif
#if (MFX_VERSION >= 1031)
    case MFX_FOURCC_Y216:
#endif
        return 4;
#if (MFX_VERSION >= 1031)
    case MFX_FOURCC_Y416:
#endif
    case MFX_FOURCC_ARGB16:
    case MFX_FOURCC_ABGR16:
        return 8;
    }
    return 0;
}
bool CmCopyWrapper::isNeedSwapping(mfxFrameSurface1 *pDst, mfxFrameSurface1 *pSrc)
{
    return (pDst->Info.FourCC == MFX_FOURCC_BGR4 && pSrc->Info.FourCC == MFX_FOURCC_RGB4) ||
           (pDst->Info.FourCC == MFX_FOURCC_RGB4 && pSrc->Info.FourCC == MFX_FOURCC_BGR4) ||
           (pDst->Info.FourCC == MFX_FOURCC_ABGR16 && pSrc->Info.FourCC == MFX_FOURCC_ARGB16) ||
           (pDst->Info.FourCC == MFX_FOURCC_ARGB16 && pSrc->Info.FourCC == MFX_FOURCC_ABGR16);
}
bool CmCopyWrapper::isNeedShift(mfxFrameSurface1 *pDst, mfxFrameSurface1 *pSrc)
{
    bool shift = pDst->Info.Shift != pSrc->Info.Shift && pDst->Info.FourCC == pSrc->Info.FourCC;
    //no support for shift in single plane formats currently.
    switch (pDst->Info.FourCC)
    {
    case MFX_FOURCC_P010:
#if (MFX_VERSION >= 1027)
    case MFX_FOURCC_Y210:
#endif
#if (MFX_VERSION >= 1031)
    case MFX_FOURCC_Y216:
    case MFX_FOURCC_P016:
    case MFX_FOURCC_Y416:
#endif
        return shift;
    }
    return false;
}
SurfaceIndex * CmCopyWrapper::CreateUpBuffer(mfxU8 *pDst, mfxU32 memSize,
                                           std::map<mfxU8 *, CmBufferUP *> & tableSysRelations,
                                           std::map<CmBufferUP *,  SurfaceIndex *> & tableSysIndex)
{
    cmStatus cmSts = 0;

    CmBufferUP *pCmUserBuffer;
    SurfaceIndex *pCmDstIndex;

    std::map<mfxU8 *, CmBufferUP *>::iterator it;
    it = tableSysRelations.find(pDst);

    if (tableSysRelations.end() == it)
    {
        UMC::AutomaticUMCMutex guard(m_guard);
        cmSts = m_pCmDevice->CreateBufferUP(memSize, pDst, pCmUserBuffer);
        CHECK_CM_STATUS_RET_NULL(cmSts, MFX_ERR_DEVICE_FAILED);

        tableSysRelations.insert(std::pair<mfxU8 *, CmBufferUP *>(pDst, pCmUserBuffer));

        cmSts = pCmUserBuffer->GetIndex(pCmDstIndex);
        CHECK_CM_STATUS_RET_NULL(cmSts, MFX_ERR_DEVICE_FAILED);

        tableSysIndex.insert(std::pair<CmBufferUP *, SurfaceIndex *>(pCmUserBuffer, pCmDstIndex));

        m_buffersInCreationOrder.push_back(pCmUserBuffer);
    }
    else
    {
        std::map<CmBufferUP *,  SurfaceIndex *>::iterator itInd;
        itInd = tableSysIndex.find((CmBufferUP *)(it->second));
        pCmDstIndex = itInd->second;
    }

    return pCmDstIndex;

} // CmBufferUP * CmCopyWrapper::CreateUpBuffer(mfxU8 *pDst, mfxU32 memSize)
mfxStatus CmCopyWrapper::EnqueueCopySwapRBGPUtoCPU(   CmSurface2D* pSurface,
                                    unsigned char* pSysMem,
                                    int width,
                                    int height,
                                    const UINT widthStride,
                                    const UINT heightStride,
                                    mfxU32 format,
                                    const UINT option,
                                    CmEvent* & pEvent )
{
    (void)option;
    (void)pEvent;

    INT             hr                      = CM_SUCCESS;
    UINT            sizePerPixel            = (format==MFX_FOURCC_ARGB16||format==MFX_FOURCC_ABGR16)? 8: 4;//RGB now
    UINT            stride_in_bytes         = widthStride;
    UINT            stride_in_dwords        = 0;
    UINT            height_stride_in_rows   = heightStride;
    UINT            AddedShiftLeftOffset    = 0;
    size_t          pLinearAddress          = (size_t)pSysMem;
    size_t          pLinearAddressAligned   = 0;
    CmKernel        *m_pCmKernel            = 0;
    CmBufferUP        *pCMBufferUP          = 0;
    SurfaceIndex    *pBufferIndexCM     = NULL;
    SurfaceIndex    *pSurf2DIndexCM     = NULL;
    CmThreadSpace   *pTS                = NULL;
    CmTask          *pGPUCopyTask       = NULL;
    CmEvent         *pInternalEvent     = NULL;

    UINT            threadWidth             = 0;
    UINT            threadHeight            = 0;
    UINT            threadNum               = 0;
    UINT            width_dword             = 0;
    UINT            width_byte              = 0;
    UINT            copy_width_byte         = 0;
    UINT            copy_height_row         = 0;
    UINT            slice_copy_height_row   = 0;
    UINT            sliceCopyBufferUPSize   = 0;
    INT             totalBufferUPSize       = 0;
    UINT            start_x                 = 0;
    UINT            start_y                 = 0;



    if ( !pSurface )
    {
        return MFX_ERR_NULL_PTR;
    }
    width_byte                      = width * sizePerPixel;

   //Align the width regarding stride
   if(stride_in_bytes == 0)
   {
        stride_in_bytes = width_byte;
   }

   if(height_stride_in_rows == 0)
   {
        height_stride_in_rows = height;
   }

    // the actual copy region
    copy_width_byte = std::min      (stride_in_bytes, width_byte);
    copy_height_row = std::min<UINT>(height_stride_in_rows, height);

    // Make sure stride and start address of system memory is 16-byte aligned.
    // if no padding in system memory , stride_in_bytes = width_byte.
    if(stride_in_bytes & 0xf)
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
    if((pLinearAddress & 0xf) || (pLinearAddress == 0))
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    //Calculate actual total size of system memory

    totalBufferUPSize = stride_in_bytes * height_stride_in_rows;

    pLinearAddress  = (size_t)pSysMem;

    while (totalBufferUPSize > 0)
    {
#if defined(LINUX64)//64-bit
            pLinearAddressAligned        = pLinearAddress & ADDRESS_PAGE_ALIGNMENT_MASK_X64;
#else  //32-bit
            pLinearAddressAligned        = pLinearAddress & ADDRESS_PAGE_ALIGNMENT_MASK_X86;
#endif

        //Calculate  Left Shift offset
        AddedShiftLeftOffset = (UINT)(pLinearAddress - pLinearAddressAligned);
        totalBufferUPSize   += AddedShiftLeftOffset;
        if (totalBufferUPSize > CM_MAX_1D_SURF_WIDTH)
        {
            slice_copy_height_row = ((CM_MAX_1D_SURF_WIDTH - AddedShiftLeftOffset)/(stride_in_bytes*(BLOCK_HEIGHT * INNER_LOOP))) * (BLOCK_HEIGHT * INNER_LOOP);
            sliceCopyBufferUPSize = slice_copy_height_row * stride_in_bytes + AddedShiftLeftOffset;
        }
        else
        {
            slice_copy_height_row = copy_height_row;
            sliceCopyBufferUPSize = totalBufferUPSize;
        }

        pBufferIndexCM = CreateUpBuffer((mfxU8*)pLinearAddressAligned,sliceCopyBufferUPSize,m_tableSysRelations2,m_tableSysIndex2);
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->CreateKernel(m_pCmProgram, CM_KERNEL_FUNCTION(surfaceCopy_readswap_32x32), m_pCmKernel);
        CHECK_CM_HR(hr);
        MFX_CHECK(m_pCmKernel, MFX_ERR_DEVICE_FAILED);

        hr = pSurface->GetIndex( pSurf2DIndexCM );
        CHECK_CM_HR(hr);
        threadWidth = ( UINT )ceil( ( double )copy_width_byte/BLOCK_PIXEL_WIDTH/4 );
        threadHeight = ( UINT )ceil( ( double )slice_copy_height_row/BLOCK_HEIGHT/INNER_LOOP );
        threadNum = threadWidth * threadHeight;
        hr = m_pCmKernel->SetThreadCount( threadNum );
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->CreateThreadSpace( threadWidth, threadHeight, pTS );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 1, sizeof( SurfaceIndex ), pBufferIndexCM );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 0, sizeof( SurfaceIndex ), pSurf2DIndexCM );
        CHECK_CM_HR(hr);
        width_dword = (UINT)ceil((double)width_byte / 4);
        stride_in_dwords = (UINT)ceil((double)stride_in_bytes / 4);

        hr = m_pCmKernel->SetKernelArg( 2, sizeof( UINT ), &stride_in_dwords );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 3, sizeof( UINT ), &height_stride_in_rows );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 4, sizeof( UINT ), &AddedShiftLeftOffset );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 5, sizeof( UINT ), &threadHeight );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 6, sizeof( UINT ), &width_dword );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 7, sizeof( UINT ), &slice_copy_height_row );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 8, sizeof( UINT ), &sizePerPixel );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 9, sizeof( UINT ), &start_x );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 10, sizeof( UINT ), &start_y );
        CHECK_CM_HR(hr);

        hr = m_pCmDevice->CreateTask(pGPUCopyTask);
        CHECK_CM_HR(hr);
        hr = pGPUCopyTask->AddKernel( m_pCmKernel );
        CHECK_CM_HR(hr);
        hr = m_pCmQueue->Enqueue( pGPUCopyTask, pInternalEvent, pTS );
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->DestroyTask(pGPUCopyTask);
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->DestroyThreadSpace(pTS);
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->DestroyKernel(m_pCmKernel);
        CHECK_CM_HR(hr);
        pLinearAddress += sliceCopyBufferUPSize - AddedShiftLeftOffset;
        totalBufferUPSize -= sliceCopyBufferUPSize;
        copy_height_row -= slice_copy_height_row;
        start_x = 0;
        start_y += slice_copy_height_row;
        if(totalBufferUPSize > 0)   //Intermediate event, we don't need it
        {
            hr = m_pCmQueue->DestroyEvent(pInternalEvent);
        }
        else //Last one event, need keep or destroy it
        {
            hr = pInternalEvent->WaitForTaskFinished(m_timeout);
            if(hr == CM_EXCEED_MAX_TIMEOUT)
                return MFX_ERR_GPU_HANG;
            else
                CHECK_CM_HR(hr);
            hr = m_pCmQueue->DestroyEvent(pInternalEvent);
        }
        CHECK_CM_HR(hr);
    }

    return MFX_ERR_NONE;
}

mfxStatus CmCopyWrapper::EnqueueCopyGPUtoCPU(   CmSurface2D* pSurface,
                                    unsigned char* pSysMem,
                                    int width,
                                    int height,
                                    const UINT widthStride,
                                    const UINT heightStride,
                                    mfxU32 format,
                                    const UINT option,
                                    CmEvent* & pEvent )
{
    (void)option;
    (void)pEvent;

    INT             hr                      = CM_SUCCESS;
    UINT            stride_in_bytes         = widthStride;
    UINT            stride_in_dwords        = 0;
    UINT            height_stride_in_rows   = heightStride;
    UINT            AddedShiftLeftOffset    = 0;
    size_t          pLinearAddress          = (size_t)pSysMem;
    size_t          pLinearAddressAligned   = 0;
    CmKernel        *m_pCmKernel            = 0;
    CmBufferUP        *pCMBufferUP          = 0;
    SurfaceIndex    *pBufferIndexCM     = NULL;
    SurfaceIndex    *pSurf2DIndexCM     = NULL;
    CmThreadSpace   *pTS                = NULL;
    CmTask          *pGPUCopyTask       = NULL;
    CmEvent         *pInternalEvent     = NULL;

    UINT            threadWidth             = 0;
    UINT            threadHeight            = 0;
    UINT            threadNum               = 0;
    UINT            width_dword             = 0;
    UINT            width_byte              = 0;
    UINT            copy_width_byte         = 0;
    UINT            copy_height_row         = 0;
    UINT            slice_copy_height_row   = 0;
    UINT            sliceCopyBufferUPSize   = 0;
    INT             totalBufferUPSize       = 0;
    UINT            start_x                 = 0;
    UINT            start_y                 = 0;


    UINT            sizePerPixel = getSizePerPixel(format);

    if (sizePerPixel == 0)
        return MFX_ERR_UNDEFINED_BEHAVIOR;

    if ( !pSurface )
    {
        return MFX_ERR_NULL_PTR;
    }
    width_byte                      = width * sizePerPixel;

   //Align the width regarding stride
   if(stride_in_bytes == 0)
   {
        stride_in_bytes = width_byte;
   }

   if(height_stride_in_rows == 0)
   {
        height_stride_in_rows = height;
   }

    // the actual copy region
    copy_width_byte = std::min      (stride_in_bytes, width_byte);
    copy_height_row = std::min<UINT>(height_stride_in_rows, height);

    // Make sure stride and start address of system memory is 16-byte aligned.
    // if no padding in system memory , stride_in_bytes = width_byte.
    if(stride_in_bytes & 0xf)
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
    if((pLinearAddress & 0xf) || (pLinearAddress == 0))
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    //Calculate actual total size of system memory

    totalBufferUPSize = stride_in_bytes * height_stride_in_rows;

    pLinearAddress  = (size_t)pSysMem;


    while (totalBufferUPSize > 0)
    {
#if defined(LINUX64)//64-bit
            pLinearAddressAligned        = pLinearAddress & ADDRESS_PAGE_ALIGNMENT_MASK_X64;
#else  //32-bit
            pLinearAddressAligned        = pLinearAddress & ADDRESS_PAGE_ALIGNMENT_MASK_X86;
#endif

        //Calculate  Left Shift offset
        AddedShiftLeftOffset = (UINT)(pLinearAddress - pLinearAddressAligned);
        totalBufferUPSize   += AddedShiftLeftOffset;
        if (totalBufferUPSize > CM_MAX_1D_SURF_WIDTH)
        {
            slice_copy_height_row = ((CM_MAX_1D_SURF_WIDTH - AddedShiftLeftOffset)/(stride_in_bytes*(BLOCK_HEIGHT * INNER_LOOP))) * (BLOCK_HEIGHT * INNER_LOOP);
            sliceCopyBufferUPSize = slice_copy_height_row * stride_in_bytes + AddedShiftLeftOffset;
        }
        else
        {
            slice_copy_height_row = copy_height_row;
            sliceCopyBufferUPSize = totalBufferUPSize;
        }

        pBufferIndexCM = CreateUpBuffer((mfxU8*)pLinearAddressAligned,sliceCopyBufferUPSize,m_tableSysRelations2,m_tableSysIndex2);
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->CreateKernel(m_pCmProgram, CM_KERNEL_FUNCTION(surfaceCopy_read_32x32), m_pCmKernel);
        CHECK_CM_HR(hr);
        MFX_CHECK(m_pCmKernel, MFX_ERR_DEVICE_FAILED);

        hr = pSurface->GetIndex( pSurf2DIndexCM );
        CHECK_CM_HR(hr);
        threadWidth = ( UINT )ceil( ( double )copy_width_byte/BLOCK_PIXEL_WIDTH/4 );
        threadHeight = ( UINT )ceil( ( double )slice_copy_height_row/BLOCK_HEIGHT/INNER_LOOP );
        threadNum = threadWidth * threadHeight;
        hr = m_pCmKernel->SetThreadCount( threadNum );
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->CreateThreadSpace( threadWidth, threadHeight, pTS );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 1, sizeof( SurfaceIndex ), pBufferIndexCM );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 0, sizeof( SurfaceIndex ), pSurf2DIndexCM );
        CHECK_CM_HR(hr);
        width_dword = (UINT)ceil((double)width_byte / 4);
        stride_in_dwords = (UINT)ceil((double)stride_in_bytes / 4);

        hr = m_pCmKernel->SetKernelArg( 2, sizeof( UINT ), &stride_in_dwords );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 3, sizeof( UINT ), &height_stride_in_rows );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 4, sizeof( UINT ), &AddedShiftLeftOffset );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 5, sizeof( UINT ), &threadHeight );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 6, sizeof( UINT ), &width_dword );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 7, sizeof( UINT ), &slice_copy_height_row );
        CHECK_CM_HR(hr);
        /*hr = m_pCmKernel->SetKernelArg( 8, sizeof( UINT ), &sizePerPixel );
        CHECK_CM_HR(hr);*/
        hr = m_pCmKernel->SetKernelArg( 8, sizeof( UINT ), &start_x );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 9, sizeof( UINT ), &start_y );
        CHECK_CM_HR(hr);

        hr = m_pCmDevice->CreateTask(pGPUCopyTask);
        CHECK_CM_HR(hr);
        hr = pGPUCopyTask->AddKernel( m_pCmKernel );
        CHECK_CM_HR(hr);
        hr = m_pCmQueue->Enqueue( pGPUCopyTask, pInternalEvent, pTS );
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->DestroyTask(pGPUCopyTask);
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->DestroyThreadSpace(pTS);
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->DestroyKernel(m_pCmKernel);
        CHECK_CM_HR(hr);
        pLinearAddress += sliceCopyBufferUPSize - AddedShiftLeftOffset;
        totalBufferUPSize -= sliceCopyBufferUPSize;
        copy_height_row -= slice_copy_height_row;
        start_x = 0;
        start_y += slice_copy_height_row;
        if(totalBufferUPSize > 0)   //Intermediate event, we don't need it
        {
            hr = m_pCmQueue->DestroyEvent(pInternalEvent);
        }
        else //Last one event, need keep or destroy it
        {
            hr = pInternalEvent->WaitForTaskFinished(m_timeout);
            if(hr == CM_EXCEED_MAX_TIMEOUT)
                return MFX_ERR_GPU_HANG;
            else
                CHECK_CM_HR(hr);
            hr = m_pCmQueue->DestroyEvent(pInternalEvent);
        }
        CHECK_CM_HR(hr);
    }

    return MFX_ERR_NONE;
}
mfxStatus CmCopyWrapper::EnqueueCopyShiftGPUtoCPU(CmSurface2D* pSurface,
                                    unsigned char* pSysMem,
                                    int width,
                                    int height,
                                    const UINT widthStride,
                                    const UINT heightStride,
                                    mfxU32 format,
                                    const UINT,
                                    int bitshift,
                                    CmEvent* &)
{
    INT             hr = CM_SUCCESS;
    UINT            stride_in_bytes = widthStride;
    UINT            stride_in_dwords = 0;
    UINT            height_stride_in_rows = heightStride;
    UINT            AddedShiftLeftOffset = 0;
    size_t          pLinearAddress = (size_t)pSysMem;
    size_t          pLinearAddressAligned = 0;
    CmKernel        *m_pCmKernel = 0;
    CmBufferUP        *pCMBufferUP = 0;
    SurfaceIndex    *pBufferIndexCM = NULL;
    SurfaceIndex    *pSurf2DIndexCM = NULL;
    CmThreadSpace   *pTS = NULL;
    CmTask          *pGPUCopyTask = NULL;
    CmEvent         *pInternalEvent = NULL;

    UINT            threadWidth = 0;
    UINT            threadHeight = 0;
    UINT            threadNum = 0;
    UINT            width_dword = 0;
    UINT            width_byte = 0;
    UINT            copy_width_byte = 0;
    UINT            copy_height_row = 0;
    UINT            slice_copy_height_row = 0;
    UINT            sliceCopyBufferUPSize = 0;
    INT             totalBufferUPSize = 0;
    UINT            start_x = 0;
    UINT            start_y = 0;


    UINT            sizePerPixel = getSizePerPixel(format);

    if (sizePerPixel == 0)
        return MFX_ERR_UNDEFINED_BEHAVIOR;

    if (!pSurface)
    {
        return MFX_ERR_NULL_PTR;
    }
    width_byte = width * sizePerPixel;

    //Align the width regarding stride
    if (stride_in_bytes == 0)
    {
        stride_in_bytes = width_byte;
    }

    if (height_stride_in_rows == 0)
    {
        height_stride_in_rows = height;
    }

    // the actual copy region
    copy_width_byte = std::min      (stride_in_bytes, width_byte);
    copy_height_row = std::min<UINT>(height_stride_in_rows, height);

    // Make sure stride and start address of system memory is 16-byte aligned.
    // if no padding in system memory , stride_in_bytes = width_byte.
    if (stride_in_bytes & 0xf)
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
    if ((pLinearAddress & 0xf) || (pLinearAddress == 0))
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    //Calculate actual total size of system memory

    totalBufferUPSize = stride_in_bytes * height_stride_in_rows;

    pLinearAddress = (size_t)pSysMem;


    while (totalBufferUPSize > 0)
    {
#if defined(LINUX64)//64-bit
        pLinearAddressAligned = pLinearAddress & ADDRESS_PAGE_ALIGNMENT_MASK_X64;
#else  //32-bit
        pLinearAddressAligned = pLinearAddress & ADDRESS_PAGE_ALIGNMENT_MASK_X86;
#endif

        //Calculate  Left Shift offset
        AddedShiftLeftOffset = (UINT)(pLinearAddress - pLinearAddressAligned);
        totalBufferUPSize += AddedShiftLeftOffset;
        if (totalBufferUPSize > CM_MAX_1D_SURF_WIDTH)
        {
            slice_copy_height_row = ((CM_MAX_1D_SURF_WIDTH - AddedShiftLeftOffset) / (stride_in_bytes*(BLOCK_HEIGHT * INNER_LOOP))) * (BLOCK_HEIGHT * INNER_LOOP);
            sliceCopyBufferUPSize = slice_copy_height_row * stride_in_bytes + AddedShiftLeftOffset;
        }
        else
        {
            slice_copy_height_row = copy_height_row;
            sliceCopyBufferUPSize = totalBufferUPSize;
        }

        pBufferIndexCM = CreateUpBuffer((mfxU8*)pLinearAddressAligned, sliceCopyBufferUPSize, m_tableSysRelations2, m_tableSysIndex2);
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->CreateKernel(m_pCmProgram, CM_KERNEL_FUNCTION(surfaceCopy_read_shift_32x32), m_pCmKernel);
        CHECK_CM_HR(hr);
        MFX_CHECK(m_pCmKernel, MFX_ERR_DEVICE_FAILED);

        hr = pSurface->GetIndex(pSurf2DIndexCM);
        CHECK_CM_HR(hr);
        threadWidth = (UINT)ceil((double)copy_width_byte / BLOCK_PIXEL_WIDTH / 4);
        threadHeight = (UINT)ceil((double)slice_copy_height_row / BLOCK_HEIGHT / INNER_LOOP);
        threadNum = threadWidth * threadHeight;
        hr = m_pCmKernel->SetThreadCount(threadNum);
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->CreateThreadSpace(threadWidth, threadHeight, pTS);
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg(1, sizeof(SurfaceIndex), pBufferIndexCM);
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg(0, sizeof(SurfaceIndex), pSurf2DIndexCM);
        CHECK_CM_HR(hr);
        width_dword = (UINT)ceil((double)width_byte / 4);
        stride_in_dwords = (UINT)ceil((double)stride_in_bytes / 4);

        hr = m_pCmKernel->SetKernelArg(2, sizeof(UINT), &stride_in_dwords);
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg(3, sizeof(UINT), &height_stride_in_rows);
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg(4, sizeof(UINT), &AddedShiftLeftOffset);
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg(5, sizeof(UINT), &bitshift);
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg(6, sizeof(UINT), &threadHeight);
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg(7, sizeof(UINT), &width_dword);
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg(8, sizeof(UINT), &slice_copy_height_row);
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg(9, sizeof(UINT), &start_x);
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg(10, sizeof(UINT), &start_y);
        CHECK_CM_HR(hr);

        hr = m_pCmDevice->CreateTask(pGPUCopyTask);
        CHECK_CM_HR(hr);
        hr = pGPUCopyTask->AddKernel(m_pCmKernel);
        CHECK_CM_HR(hr);
        hr = m_pCmQueue->Enqueue(pGPUCopyTask, pInternalEvent, pTS);
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->DestroyTask(pGPUCopyTask);
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->DestroyThreadSpace(pTS);
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->DestroyKernel(m_pCmKernel);
        CHECK_CM_HR(hr);
        pLinearAddress += sliceCopyBufferUPSize - AddedShiftLeftOffset;
        totalBufferUPSize -= sliceCopyBufferUPSize;
        copy_height_row -= slice_copy_height_row;
        start_x = 0;
        start_y += slice_copy_height_row;
        if (totalBufferUPSize > 0)   //Intermediate event, we don't need it
        {
            hr = m_pCmQueue->DestroyEvent(pInternalEvent);
        }
        else //Last one event, need keep or destroy it
        {
            hr = pInternalEvent->WaitForTaskFinished(m_timeout);
            if (hr == CM_EXCEED_MAX_TIMEOUT)
                return MFX_ERR_GPU_HANG;
            else
                CHECK_CM_HR(hr);
            hr = m_pCmQueue->DestroyEvent(pInternalEvent);
        }
        CHECK_CM_HR(hr);
    }

    return MFX_ERR_NONE;
}
mfxStatus CmCopyWrapper::EnqueueCopySwapRBCPUtoGPU(   CmSurface2D* pSurface,
                                    unsigned char* pSysMem,
                                    int width,
                                    int height,
                                    const UINT widthStride,
                                    const UINT heightStride,
                                    mfxU32 format,
                                    const UINT option,
                                    CmEvent* & pEvent )
{
    (void)option;
    (void)pEvent;

    INT             hr                      = CM_SUCCESS;
    UINT            sizePerPixel            = (format==MFX_FOURCC_ARGB16||format==MFX_FOURCC_ABGR16)? 8: 4;//RGB now
    UINT            stride_in_bytes         = widthStride;
    UINT            stride_in_dwords        = 0;
    UINT            height_stride_in_rows   = heightStride;
    UINT            AddedShiftLeftOffset    = 0;
    size_t          pLinearAddress          = (size_t)pSysMem;
    size_t          pLinearAddressAligned   = 0;

    CmKernel        *m_pCmKernel            = 0;
    CmBufferUP      *pCMBufferUP          = 0;
    SurfaceIndex    *pBufferIndexCM     = NULL;
    SurfaceIndex    *pSurf2DIndexCM     = NULL;
    CmThreadSpace   *pTS                = NULL;
    CmTask          *pGPUCopyTask       = NULL;
    CmEvent         *pInternalEvent     = NULL;

    UINT            threadWidth             = 0;
    UINT            threadHeight            = 0;
    UINT            threadNum               = 0;
    UINT            width_byte              = 0;
    UINT            copy_width_byte         = 0;
    UINT            copy_height_row         = 0;
    UINT            slice_copy_height_row   = 0;
    UINT            sliceCopyBufferUPSize   = 0;
    INT             totalBufferUPSize       = 0;
    UINT            start_x                 = 0;
    UINT            start_y                 = 0;



    if ( !pSurface )
    {
        return MFX_ERR_NULL_PTR;
    }
    width_byte                      = width * sizePerPixel;

   //Align the width regarding stride
   if(stride_in_bytes == 0)
   {
        stride_in_bytes = width_byte;
   }

   if(height_stride_in_rows == 0)
   {
        height_stride_in_rows = height;
   }

    // the actual copy region
    copy_width_byte = std::min      (stride_in_bytes, width_byte);
    copy_height_row = std::min<UINT>(height_stride_in_rows, height);

    // Make sure stride and start address of system memory is 16-byte aligned.
    // if no padding in system memory , stride_in_bytes = width_byte.
    if(stride_in_bytes & 0xf)
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
    if((pLinearAddress & 0xf) || (pLinearAddress == 0))
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    //Calculate actual total size of system memory

    totalBufferUPSize = stride_in_bytes * height_stride_in_rows;

    pLinearAddress  = (size_t)pSysMem;


    while (totalBufferUPSize > 0)
    {
#if defined(LINUX64)//64-bit
            pLinearAddressAligned        = pLinearAddress & ADDRESS_PAGE_ALIGNMENT_MASK_X64;
#else  //32-bit
            pLinearAddressAligned        = pLinearAddress & ADDRESS_PAGE_ALIGNMENT_MASK_X86;
#endif

        //Calculate  Left Shift offset
        AddedShiftLeftOffset = (UINT)(pLinearAddress - pLinearAddressAligned);
        totalBufferUPSize   += AddedShiftLeftOffset;
        if (totalBufferUPSize > CM_MAX_1D_SURF_WIDTH)
        {
            slice_copy_height_row = ((CM_MAX_1D_SURF_WIDTH - AddedShiftLeftOffset)/(stride_in_bytes*(BLOCK_HEIGHT * INNER_LOOP))) * (BLOCK_HEIGHT * INNER_LOOP);
            sliceCopyBufferUPSize = slice_copy_height_row * stride_in_bytes + AddedShiftLeftOffset;
        }
        else
        {
            slice_copy_height_row = copy_height_row;
            sliceCopyBufferUPSize = totalBufferUPSize;
        }

        pBufferIndexCM = CreateUpBuffer((mfxU8*)pLinearAddressAligned,sliceCopyBufferUPSize,m_tableSysRelations2,m_tableSysIndex2);
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->CreateKernel(m_pCmProgram, CM_KERNEL_FUNCTION(surfaceCopy_writeswap_32x32), m_pCmKernel);
        CHECK_CM_HR(hr);

        MFX_CHECK(m_pCmKernel, MFX_ERR_DEVICE_FAILED);

        hr = pSurface->GetIndex( pSurf2DIndexCM );
        CHECK_CM_HR(hr);
        threadWidth = ( UINT )ceil( ( double )copy_width_byte/BLOCK_PIXEL_WIDTH/4 );
        threadHeight = ( UINT )ceil( ( double )slice_copy_height_row/BLOCK_HEIGHT/INNER_LOOP );
        threadNum = threadWidth * threadHeight;
        hr = m_pCmKernel->SetThreadCount( threadNum );
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->CreateThreadSpace( threadWidth, threadHeight, pTS );
        CHECK_CM_HR(hr);

        m_pCmKernel->SetKernelArg( 0, sizeof( SurfaceIndex ), pBufferIndexCM);
        CHECK_CM_HR(hr);
        m_pCmKernel->SetKernelArg( 1, sizeof( SurfaceIndex ), pSurf2DIndexCM);
        CHECK_CM_HR(hr);


        stride_in_dwords = (UINT)ceil((double)stride_in_bytes / 4);

        hr = m_pCmKernel->SetKernelArg( 2, sizeof( UINT ), &stride_in_dwords );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 3, sizeof( UINT ), &slice_copy_height_row );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 4, sizeof( UINT ), &AddedShiftLeftOffset );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 5, sizeof( UINT ), &threadHeight );
        CHECK_CM_HR(hr);

        //this only works for the kernel surfaceCopy_write_32x32
        hr = m_pCmKernel->SetKernelArg( 6, sizeof( UINT ), &sizePerPixel );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 7, sizeof( UINT ), &start_x );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 8, sizeof( UINT ), &start_y );
        CHECK_CM_HR(hr);

        hr = m_pCmDevice->CreateTask(pGPUCopyTask);
        CHECK_CM_HR(hr);
        hr = pGPUCopyTask->AddKernel( m_pCmKernel );
        CHECK_CM_HR(hr);
        hr = m_pCmQueue->Enqueue( pGPUCopyTask, pInternalEvent, pTS );
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->DestroyTask(pGPUCopyTask);
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->DestroyThreadSpace(pTS);
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->DestroyKernel(m_pCmKernel);
        CHECK_CM_HR(hr);
        pLinearAddress += sliceCopyBufferUPSize - AddedShiftLeftOffset;
        totalBufferUPSize -= sliceCopyBufferUPSize;
        copy_height_row -= slice_copy_height_row;
        start_x = 0;
        start_y += slice_copy_height_row;
        if(totalBufferUPSize > 0)   //Intermediate event, we don't need it
        {
            hr = m_pCmQueue->DestroyEvent(pInternalEvent);
        }
        else //Last one event, need keep or destroy it
        {
            hr = pInternalEvent->WaitForTaskFinished(m_timeout);
            if(hr == CM_EXCEED_MAX_TIMEOUT)
                return MFX_ERR_GPU_HANG;
            else
                CHECK_CM_HR(hr);
            hr = m_pCmQueue->DestroyEvent(pInternalEvent);
        }
        CHECK_CM_HR(hr);
    }

    return MFX_ERR_NONE;
}

mfxStatus CmCopyWrapper::EnqueueCopyCPUtoGPU(   CmSurface2D* pSurface,
                                    unsigned char* pSysMem,
                                    int width,
                                    int height,
                                    const UINT widthStride,
                                    const UINT heightStride,
                                    mfxU32 format,
                                    const UINT option,
                                    CmEvent* & pEvent )
{
    (void)option;
    (void)pEvent;

    INT             hr                      = CM_SUCCESS;
    UINT            stride_in_bytes         = widthStride;
    UINT            stride_in_dwords        = 0;
    UINT            height_stride_in_rows   = heightStride;
    UINT            AddedShiftLeftOffset    = 0;
    size_t          pLinearAddress          = (size_t)pSysMem;
    size_t          pLinearAddressAligned   = 0;

    CmKernel        *m_pCmKernel            = 0;
    CmBufferUP      *pCMBufferUP          = 0;
    SurfaceIndex    *pBufferIndexCM     = NULL;
    SurfaceIndex    *pSurf2DIndexCM     = NULL;
    CmThreadSpace   *pTS                = NULL;
    CmTask          *pGPUCopyTask       = NULL;
    CmEvent         *pInternalEvent     = NULL;

    UINT            threadWidth             = 0;
    UINT            threadHeight            = 0;
    UINT            threadNum               = 0;
    UINT            width_byte              = 0;
    UINT            copy_width_byte         = 0;
    UINT            copy_height_row         = 0;
    UINT            slice_copy_height_row   = 0;
    UINT            sliceCopyBufferUPSize   = 0;
    INT             totalBufferUPSize       = 0;
    UINT            start_x                 = 0;
    UINT            start_y                 = 0;

    UINT            sizePerPixel = getSizePerPixel(format);

    if (sizePerPixel == 0)
        return MFX_ERR_UNDEFINED_BEHAVIOR;

    if ( !pSurface )
    {
        return MFX_ERR_NULL_PTR;
    }
    width_byte                      = width * sizePerPixel;

   //Align the width regarding stride
   if(stride_in_bytes == 0)
   {
        stride_in_bytes = width_byte;
   }

   if(height_stride_in_rows == 0)
   {
        height_stride_in_rows = height;
   }

    // the actual copy region
    copy_width_byte = std::min      (stride_in_bytes, width_byte);
    copy_height_row = std::min<UINT>(height_stride_in_rows, height);

    // Make sure stride and start address of system memory is 16-byte aligned.
    // if no padding in system memory , stride_in_bytes = width_byte.
    if(stride_in_bytes & 0xf)
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
    if((pLinearAddress & 0xf) || (pLinearAddress == 0))
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    //Calculate actual total size of system memory

    totalBufferUPSize = stride_in_bytes * height_stride_in_rows;

    pLinearAddress  = (size_t)pSysMem;


    while (totalBufferUPSize > 0)
    {
#if defined(LINUX64)//64-bit
            pLinearAddressAligned        = pLinearAddress & ADDRESS_PAGE_ALIGNMENT_MASK_X64;
#else  //32-bit
            pLinearAddressAligned        = pLinearAddress & ADDRESS_PAGE_ALIGNMENT_MASK_X86;
#endif

        //Calculate  Left Shift offset
        AddedShiftLeftOffset = (UINT)(pLinearAddress - pLinearAddressAligned);
        totalBufferUPSize   += AddedShiftLeftOffset;
        if (totalBufferUPSize > CM_MAX_1D_SURF_WIDTH)
        {
            slice_copy_height_row = ((CM_MAX_1D_SURF_WIDTH - AddedShiftLeftOffset)/(stride_in_bytes*(BLOCK_HEIGHT * INNER_LOOP))) * (BLOCK_HEIGHT * INNER_LOOP);
            sliceCopyBufferUPSize = slice_copy_height_row * stride_in_bytes + AddedShiftLeftOffset;
        }
        else
        {
            slice_copy_height_row = copy_height_row;
            sliceCopyBufferUPSize = totalBufferUPSize;
        }

        pBufferIndexCM = CreateUpBuffer((mfxU8*)pLinearAddressAligned,sliceCopyBufferUPSize,m_tableSysRelations2,m_tableSysIndex2);
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->CreateKernel(m_pCmProgram, CM_KERNEL_FUNCTION(surfaceCopy_write_32x32), m_pCmKernel);
        CHECK_CM_HR(hr);

        MFX_CHECK(m_pCmKernel, MFX_ERR_DEVICE_FAILED);

        hr = pSurface->GetIndex( pSurf2DIndexCM );
        CHECK_CM_HR(hr);
        threadWidth = ( UINT )ceil( ( double )copy_width_byte/BLOCK_PIXEL_WIDTH/4 );
        threadHeight = ( UINT )ceil( ( double )slice_copy_height_row/BLOCK_HEIGHT/INNER_LOOP );
        threadNum = threadWidth * threadHeight;
        hr = m_pCmKernel->SetThreadCount( threadNum );
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->CreateThreadSpace( threadWidth, threadHeight, pTS );
        CHECK_CM_HR(hr);

        m_pCmKernel->SetKernelArg( 0, sizeof( SurfaceIndex ), pBufferIndexCM);
        CHECK_CM_HR(hr);
        m_pCmKernel->SetKernelArg( 1, sizeof( SurfaceIndex ), pSurf2DIndexCM);
        CHECK_CM_HR(hr);


        stride_in_dwords = (UINT)ceil((double)stride_in_bytes / 4);

        hr = m_pCmKernel->SetKernelArg( 2, sizeof( UINT ), &stride_in_dwords );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 3, sizeof( UINT ), &slice_copy_height_row );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 4, sizeof( UINT ), &AddedShiftLeftOffset );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 5, sizeof( UINT ), &threadHeight );
        CHECK_CM_HR(hr);

        //this only works for the kernel surfaceCopy_write_32x32
        /*hr = m_pCmKernel->SetKernelArg( 6, sizeof( UINT ), &sizePerPixel );
        CHECK_CM_HR(hr);*/
        hr = m_pCmKernel->SetKernelArg( 6, sizeof( UINT ), &start_x );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 7, sizeof( UINT ), &start_y );
        CHECK_CM_HR(hr);

        hr = m_pCmDevice->CreateTask(pGPUCopyTask);
        CHECK_CM_HR(hr);
        hr = pGPUCopyTask->AddKernel( m_pCmKernel );
        CHECK_CM_HR(hr);
        hr = m_pCmQueue->Enqueue( pGPUCopyTask, pInternalEvent, pTS );
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->DestroyTask(pGPUCopyTask);
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->DestroyThreadSpace(pTS);
        CHECK_CM_HR(hr);

        hr = m_pCmDevice->DestroyKernel(m_pCmKernel);
        CHECK_CM_HR(hr);
        pLinearAddress += sliceCopyBufferUPSize - AddedShiftLeftOffset;
        totalBufferUPSize -= sliceCopyBufferUPSize;
        copy_height_row -= slice_copy_height_row;
        start_x = 0;
        start_y += slice_copy_height_row;
        if(totalBufferUPSize > 0)   //Intermediate event, we don't need it
        {
            hr = m_pCmQueue->DestroyEvent(pInternalEvent);
        }
        else //Last one event, need keep or destroy it
        {
            hr = pInternalEvent->WaitForTaskFinished(m_timeout);
            if(hr == CM_EXCEED_MAX_TIMEOUT)
                return MFX_ERR_GPU_HANG;
            else
                CHECK_CM_HR(hr);
            hr = m_pCmQueue->DestroyEvent(pInternalEvent);
        }
        CHECK_CM_HR(hr);
    }

    return MFX_ERR_NONE;
}
mfxStatus CmCopyWrapper::EnqueueCopyShiftCPUtoGPU(CmSurface2D* pSurface,
    unsigned char* pSysMem,
    int width,
    int height,
    const UINT widthStride,
    const UINT heightStride,
    mfxU32 format,
    int,
    const UINT,
    CmEvent* &)
{
    INT             hr = CM_SUCCESS;
    UINT            stride_in_bytes = widthStride;
    UINT            stride_in_dwords = 0;
    UINT            height_stride_in_rows = heightStride;
    UINT            AddedShiftLeftOffset = 0;
    size_t          pLinearAddress = (size_t)pSysMem;
    size_t          pLinearAddressAligned = 0;

    CmKernel        *m_pCmKernel = 0;
    CmBufferUP      *pCMBufferUP = 0;
    SurfaceIndex    *pBufferIndexCM = NULL;
    SurfaceIndex    *pSurf2DIndexCM = NULL;
    CmThreadSpace   *pTS = NULL;
    CmTask          *pGPUCopyTask = NULL;
    CmEvent         *pInternalEvent = NULL;

    UINT            threadWidth = 0;
    UINT            threadHeight = 0;
    UINT            threadNum = 0;
    UINT            width_byte = 0;
    UINT            copy_width_byte = 0;
    UINT            copy_height_row = 0;
    UINT            slice_copy_height_row = 0;
    UINT            sliceCopyBufferUPSize = 0;
    INT             totalBufferUPSize = 0;
    UINT            start_x = 0;
    UINT            start_y = 0;

    UINT            sizePerPixel = getSizePerPixel(format);

    if (sizePerPixel == 0)
        return MFX_ERR_UNDEFINED_BEHAVIOR;

    if (!pSurface)
    {
        return MFX_ERR_NULL_PTR;
    }
    width_byte = width * sizePerPixel;

    //Align the width regarding stride
    if (stride_in_bytes == 0)
    {
        stride_in_bytes = width_byte;
    }

    if (height_stride_in_rows == 0)
    {
        height_stride_in_rows = height;
    }

    // the actual copy region
    copy_width_byte = std::min      (stride_in_bytes, width_byte);
    copy_height_row = std::min<UINT>(height_stride_in_rows, height);

    // Make sure stride and start address of system memory is 16-byte aligned.
    // if no padding in system memory , stride_in_bytes = width_byte.
    if (stride_in_bytes & 0xf)
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
    if ((pLinearAddress & 0xf) || (pLinearAddress == 0))
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    //Calculate actual total size of system memory

    totalBufferUPSize = stride_in_bytes * height_stride_in_rows;

    pLinearAddress = (size_t)pSysMem;


    while (totalBufferUPSize > 0)
    {
#if defined(LINUX64)//64-bit
        pLinearAddressAligned = pLinearAddress & ADDRESS_PAGE_ALIGNMENT_MASK_X64;
#else  //32-bit
        pLinearAddressAligned = pLinearAddress & ADDRESS_PAGE_ALIGNMENT_MASK_X86;
#endif

        //Calculate  Left Shift offset
        AddedShiftLeftOffset = (UINT)(pLinearAddress - pLinearAddressAligned);
        totalBufferUPSize += AddedShiftLeftOffset;
        if (totalBufferUPSize > CM_MAX_1D_SURF_WIDTH)
        {
            slice_copy_height_row = ((CM_MAX_1D_SURF_WIDTH - AddedShiftLeftOffset) / (stride_in_bytes*(BLOCK_HEIGHT * INNER_LOOP))) * (BLOCK_HEIGHT * INNER_LOOP);
            sliceCopyBufferUPSize = slice_copy_height_row * stride_in_bytes + AddedShiftLeftOffset;
        }
        else
        {
            slice_copy_height_row = copy_height_row;
            sliceCopyBufferUPSize = totalBufferUPSize;
        }

        pBufferIndexCM = CreateUpBuffer((mfxU8*)pLinearAddressAligned, sliceCopyBufferUPSize, m_tableSysRelations2, m_tableSysIndex2);
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->CreateKernel(m_pCmProgram, CM_KERNEL_FUNCTION(surfaceCopy_write_shift_32x32), m_pCmKernel);
        CHECK_CM_HR(hr);

        MFX_CHECK(m_pCmKernel, MFX_ERR_DEVICE_FAILED);

        hr = pSurface->GetIndex(pSurf2DIndexCM);
        CHECK_CM_HR(hr);
        threadWidth = (UINT)ceil((double)copy_width_byte / BLOCK_PIXEL_WIDTH / 4);
        threadHeight = (UINT)ceil((double)slice_copy_height_row / BLOCK_HEIGHT / INNER_LOOP);
        threadNum = threadWidth * threadHeight;
        hr = m_pCmKernel->SetThreadCount(threadNum);
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->CreateThreadSpace(threadWidth, threadHeight, pTS);
        CHECK_CM_HR(hr);

        m_pCmKernel->SetKernelArg(0, sizeof(SurfaceIndex), pBufferIndexCM);
        CHECK_CM_HR(hr);
        m_pCmKernel->SetKernelArg(1, sizeof(SurfaceIndex), pSurf2DIndexCM);
        CHECK_CM_HR(hr);


        stride_in_dwords = (UINT)ceil((double)stride_in_bytes / 4);

        hr = m_pCmKernel->SetKernelArg(2, sizeof(UINT), &stride_in_dwords);
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg(3, sizeof(UINT), &slice_copy_height_row);
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg(4, sizeof(UINT), &AddedShiftLeftOffset);
        CHECK_CM_HR(hr);

        hr = m_pCmKernel->SetKernelArg(5, sizeof( UINT ), &sizePerPixel );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg(6, sizeof(UINT), &threadHeight);
        CHECK_CM_HR(hr);

        hr = m_pCmKernel->SetKernelArg(7, sizeof(UINT), &start_x);
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg(8, sizeof(UINT), &start_y);
        CHECK_CM_HR(hr);

        hr = m_pCmDevice->CreateTask(pGPUCopyTask);
        CHECK_CM_HR(hr);
        hr = pGPUCopyTask->AddKernel(m_pCmKernel);
        CHECK_CM_HR(hr);
        hr = m_pCmQueue->Enqueue(pGPUCopyTask, pInternalEvent, pTS);
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->DestroyTask(pGPUCopyTask);
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->DestroyThreadSpace(pTS);
        CHECK_CM_HR(hr);

        hr = m_pCmDevice->DestroyKernel(m_pCmKernel);
        CHECK_CM_HR(hr);
        pLinearAddress += sliceCopyBufferUPSize - AddedShiftLeftOffset;
        totalBufferUPSize -= sliceCopyBufferUPSize;
        copy_height_row -= slice_copy_height_row;
        start_x = 0;
        start_y += slice_copy_height_row;
        if (totalBufferUPSize > 0)   //Intermediate event, we don't need it
        {
            hr = m_pCmQueue->DestroyEvent(pInternalEvent);
        }
        else //Last one event, need keep or destroy it
        {
            hr = pInternalEvent->WaitForTaskFinished(m_timeout);
            if (hr == CM_EXCEED_MAX_TIMEOUT)
                return MFX_ERR_GPU_HANG;
            else
                CHECK_CM_HR(hr);
            hr = m_pCmQueue->DestroyEvent(pInternalEvent);
        }
        CHECK_CM_HR(hr);
    }

    return MFX_ERR_NONE;
}

mfxStatus CmCopyWrapper::EnqueueCopySwapRBGPUtoGPU(   CmSurface2D* pSurfaceIn,
                                    CmSurface2D* pSurfaceOut,
                                    int width,
                                    int height,
                                    mfxU32 format,
                                    const UINT option,
                                    CmEvent* & pEvent )
{
    (void)option;
    (void)pEvent;

    INT             hr                      = CM_SUCCESS;
    UINT            sizePerPixel            = (format==MFX_FOURCC_ARGB16||format==MFX_FOURCC_ABGR16)? 8: 4;//RGB now

    SurfaceIndex    *pSurf2DIndexCM_In  = NULL;
    SurfaceIndex    *pSurf2DIndexCM_Out = NULL;
    CmThreadSpace   *pTS                = NULL;
    CmTask          *pGPUCopyTask       = NULL;
    CmEvent         *pInternalEvent     = NULL;

    CmKernel        *m_pCmKernel            = 0;
    CmBufferUP      *pCMBufferUP            = 0;
    UINT            threadWidth             = 0;
    UINT            threadHeight            = 0;
    UINT            threadNum               = 0;



    if ( !pSurfaceIn || !pSurfaceOut )
    {
        return MFX_ERR_NULL_PTR;
    }

    hr = m_pCmDevice->CreateKernel(m_pCmProgram, CM_KERNEL_FUNCTION(SurfaceCopySwap_2DTo2D_32x32), m_pCmKernel);
    CHECK_CM_HR(hr);

    MFX_CHECK(m_pCmKernel, MFX_ERR_DEVICE_FAILED);

    hr = pSurfaceOut->GetIndex( pSurf2DIndexCM_Out );
    CHECK_CM_HR(hr);
    hr = pSurfaceIn->GetIndex( pSurf2DIndexCM_In );
    CHECK_CM_HR(hr);

    threadWidth = ( UINT )ceil( ( double )width/BLOCK_PIXEL_WIDTH );
    threadHeight = ( UINT )ceil( ( double )height/BLOCK_HEIGHT/INNER_LOOP );
    threadNum = threadWidth * threadHeight;
    hr = m_pCmKernel->SetThreadCount( threadNum );
    CHECK_CM_HR(hr);
    hr = m_pCmDevice->CreateThreadSpace( threadWidth, threadHeight, pTS );
    CHECK_CM_HR(hr);

    m_pCmKernel->SetKernelArg( 0, sizeof( SurfaceIndex ), pSurf2DIndexCM_In);
    m_pCmKernel->SetKernelArg( 1, sizeof( SurfaceIndex ), pSurf2DIndexCM_Out);

    hr = m_pCmKernel->SetKernelArg( 2, sizeof( UINT ), &threadHeight );
    CHECK_CM_HR(hr);
    hr = m_pCmKernel->SetKernelArg( 3, sizeof( UINT ), &sizePerPixel );
    CHECK_CM_HR(hr);

    hr = m_pCmDevice->CreateTask(pGPUCopyTask);
    CHECK_CM_HR(hr);
    hr = pGPUCopyTask->AddKernel( m_pCmKernel );
    CHECK_CM_HR(hr);
    hr = m_pCmQueue->Enqueue( pGPUCopyTask, pInternalEvent, pTS );
    CHECK_CM_HR(hr);

    hr = m_pCmDevice->DestroyTask(pGPUCopyTask);
    CHECK_CM_HR(hr);
    hr = m_pCmDevice->DestroyThreadSpace(pTS);
    CHECK_CM_HR(hr);
    hr = m_pCmDevice->DestroyKernel(m_pCmKernel);
    CHECK_CM_HR(hr);
    hr = pInternalEvent->WaitForTaskFinished(m_timeout);

    if(hr == CM_EXCEED_MAX_TIMEOUT)
        return MFX_ERR_GPU_HANG;
    else
        CHECK_CM_HR(hr);
    hr = m_pCmQueue->DestroyEvent(pInternalEvent);
    CHECK_CM_HR(hr);

    return MFX_ERR_NONE;
}


mfxStatus CmCopyWrapper::EnqueueCopyMirrorGPUtoGPU(   CmSurface2D* pSurfaceIn,
                                    CmSurface2D* pSurfaceOut,
                                    int width,
                                    int height,
                                    mfxU32 format,
                                    const UINT option,
                                    CmEvent* & pEvent )
{
    (void)format;
    (void)option;
    (void)pEvent;

    INT             hr                      = CM_SUCCESS;
//    UINT            sizePerPixel            = (format==MFX_FOURCC_ARGB16||format==MFX_FOURCC_ABGR16)? 8: 4;//RGB now

    SurfaceIndex    *pSurf2DIndexCM_In  = NULL;
    SurfaceIndex    *pSurf2DIndexCM_Out = NULL;
    CmThreadSpace   *pTS                = NULL;
    CmTask          *pGPUCopyTask       = NULL;
    CmEvent         *pInternalEvent     = NULL;

    CmKernel        *m_pCmKernel            = 0;
    CmBufferUP      *pCMBufferUP            = 0;
    UINT            threadWidth             = 0;
    UINT            threadHeight            = 0;
    UINT            threadNum               = 0;



    if ( !pSurfaceIn || !pSurfaceOut )
    {
        return MFX_ERR_NULL_PTR;
    }

    hr = m_pCmDevice->CreateKernel(m_pCmProgram, CM_KERNEL_FUNCTION(SurfaceMirror_2DTo2D_NV12), m_pCmKernel);
    CHECK_CM_HR(hr);

    MFX_CHECK(m_pCmKernel, MFX_ERR_DEVICE_FAILED);

    hr = pSurfaceOut->GetIndex( pSurf2DIndexCM_Out );
    CHECK_CM_HR(hr);
    hr = pSurfaceIn->GetIndex( pSurf2DIndexCM_In );
    CHECK_CM_HR(hr);

    threadWidth = ( UINT )ceil( ( double )width/BLOCK_PIXEL_WIDTH );
    threadHeight = ( UINT )ceil( ( double )height/BLOCK_HEIGHT );
    threadNum = threadWidth * threadHeight;
    hr = m_pCmKernel->SetThreadCount( threadNum );
    CHECK_CM_HR(hr);
    hr = m_pCmDevice->CreateThreadSpace( threadWidth, threadHeight, pTS );
    CHECK_CM_HR(hr);

    m_pCmKernel->SetKernelArg( 0, sizeof( SurfaceIndex ), pSurf2DIndexCM_In);
    m_pCmKernel->SetKernelArg( 1, sizeof( SurfaceIndex ), pSurf2DIndexCM_Out);

    hr = m_pCmKernel->SetKernelArg( 2, sizeof( UINT ), &width );
    CHECK_CM_HR(hr);
    hr = m_pCmKernel->SetKernelArg( 3, sizeof( UINT ), &height );
    CHECK_CM_HR(hr);

    hr = m_pCmDevice->CreateTask(pGPUCopyTask);
    CHECK_CM_HR(hr);
    hr = pGPUCopyTask->AddKernel( m_pCmKernel );
    CHECK_CM_HR(hr);
    hr = m_pCmQueue->Enqueue( pGPUCopyTask, pInternalEvent, pTS );
    CHECK_CM_HR(hr);

    hr = m_pCmDevice->DestroyTask(pGPUCopyTask);
    CHECK_CM_HR(hr);
    hr = m_pCmDevice->DestroyThreadSpace(pTS);
    CHECK_CM_HR(hr);
    hr = m_pCmDevice->DestroyKernel(m_pCmKernel);
    CHECK_CM_HR(hr);
    hr = pInternalEvent->WaitForTaskFinished(m_timeout);

    if(hr == CM_EXCEED_MAX_TIMEOUT)
        return MFX_ERR_GPU_HANG;
    else
        CHECK_CM_HR(hr);
    hr = m_pCmQueue->DestroyEvent(pInternalEvent);
    CHECK_CM_HR(hr);

    return MFX_ERR_NONE;
}

mfxStatus CmCopyWrapper::EnqueueCopyMirrorNV12GPUtoCPU(   CmSurface2D* pSurface,
                                    unsigned char* pSysMem,
                                    int width,
                                    int height,
                                    const UINT widthStride,
                                    const UINT heightStride,
                                    mfxU32 format,
                                    const UINT option,
                                    CmEvent* & pEvent )
{
    (void)format;
    (void)option;
    (void)pEvent;

    INT             hr                      = CM_SUCCESS;
    UINT            stride_in_bytes         = widthStride;
    UINT            stride_in_dwords        = 0;
    UINT            height_stride_in_rows   = heightStride;
    UINT            AddedShiftLeftOffset    = 0;
    size_t          pLinearAddress          = (size_t)pSysMem;
    size_t          pLinearAddressAligned   = 0;
    CmKernel        *m_pCmKernel            = 0;
    CmBufferUP        *pCMBufferUP          = 0;
    SurfaceIndex    *pBufferIndexCM     = NULL;
    SurfaceIndex    *pSurf2DIndexCM     = NULL;
    CmThreadSpace   *pTS                = NULL;
    CmTask          *pGPUCopyTask       = NULL;
    CmEvent         *pInternalEvent     = NULL;

    UINT            threadWidth             = 0;
    UINT            threadHeight            = 0;
    UINT            threadNum               = 0;
    UINT            width_dword             = 0;
    UINT            width_byte              = 0;
    UINT            copy_width_byte         = 0;
    UINT            copy_height_row         = 0;
    UINT            slice_copy_height_row   = 0;
    UINT            sliceCopyBufferUPSize   = 0;
    INT             totalBufferUPSize       = 0;



    if ( !pSurface )
    {
        return MFX_ERR_NULL_PTR;
    }
    width_byte                      = width;

   //Align the width regarding stride
   if(stride_in_bytes == 0)
   {
        stride_in_bytes = width_byte;
   }

   if(height_stride_in_rows == 0)
   {
        height_stride_in_rows = height;
   }

    // the actual copy region
    copy_width_byte = std::min      (stride_in_bytes, width_byte);
    copy_height_row = std::min<UINT>(height_stride_in_rows, height);

    // Make sure stride and start address of system memory is 16-byte aligned.
    // if no padding in system memory , stride_in_bytes = width_byte.
    if(stride_in_bytes & 0xf)
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
    if((pLinearAddress & 0xf) || (pLinearAddress == 0))
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    //Calculate actual total size of system memory

    totalBufferUPSize = stride_in_bytes * height_stride_in_rows + stride_in_bytes * height/2;

    pLinearAddress  = (size_t)pSysMem;


    while (totalBufferUPSize > 0)
    {
#if defined(LINUX64)//64-bit
            pLinearAddressAligned        = pLinearAddress & ADDRESS_PAGE_ALIGNMENT_MASK_X64;
#else  //32-bit
            pLinearAddressAligned        = pLinearAddress & ADDRESS_PAGE_ALIGNMENT_MASK_X86;
#endif

        //Calculate  Left Shift offset
        AddedShiftLeftOffset = (UINT)(pLinearAddress - pLinearAddressAligned);
        totalBufferUPSize   += AddedShiftLeftOffset;
        if (totalBufferUPSize > CM_MAX_1D_SURF_WIDTH)
        {
            slice_copy_height_row = ((CM_MAX_1D_SURF_WIDTH - AddedShiftLeftOffset)/(stride_in_bytes*(BLOCK_HEIGHT * INNER_LOOP))) * (BLOCK_HEIGHT * INNER_LOOP);
            sliceCopyBufferUPSize = slice_copy_height_row * stride_in_bytes * 3 / 2  + AddedShiftLeftOffset;
            return MFX_ERR_DEVICE_FAILED;
        }
        else
        {
            slice_copy_height_row = copy_height_row;
            sliceCopyBufferUPSize = totalBufferUPSize;
        }
        pBufferIndexCM = CreateUpBuffer((mfxU8*)pLinearAddressAligned,sliceCopyBufferUPSize,m_tableSysRelations2,m_tableSysIndex2);
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->CreateKernel(m_pCmProgram, CM_KERNEL_FUNCTION(surfaceMirror_read_NV12), m_pCmKernel);
        CHECK_CM_HR(hr);
        MFX_CHECK(m_pCmKernel, MFX_ERR_DEVICE_FAILED);

        hr = pSurface->GetIndex( pSurf2DIndexCM );
        CHECK_CM_HR(hr);
        threadWidth = ( UINT )ceil( ( double )copy_width_byte/BLOCK_PIXEL_WIDTH/4 );
        threadHeight = ( UINT )ceil( ( double )slice_copy_height_row/BLOCK_HEIGHT );
        threadNum = threadWidth * threadHeight;
        hr = m_pCmKernel->SetThreadCount( threadNum );
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->CreateThreadSpace( threadWidth, threadHeight, pTS );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 1, sizeof( SurfaceIndex ), pBufferIndexCM );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 0, sizeof( SurfaceIndex ), pSurf2DIndexCM );
        CHECK_CM_HR(hr);
        width_dword = (UINT)ceil((double)width_byte / 4);
        stride_in_dwords = (UINT)ceil((double)stride_in_bytes / 4);

        hr = m_pCmKernel->SetKernelArg( 2, sizeof( UINT ), &stride_in_dwords );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 3, sizeof( UINT ), &height);
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 4, sizeof( UINT ), &AddedShiftLeftOffset );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 5, sizeof( UINT ), &width_dword );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 6, sizeof( UINT ), &height_stride_in_rows );
        CHECK_CM_HR(hr);
        //hr = m_pCmKernel->SetKernelArg( 5, sizeof( UINT ), &threadHeight );
        //CHECK_CM_HR(hr);
        //hr = m_pCmKernel->SetKernelArg( 7, sizeof( UINT ), &slice_copy_height_row );
        //CHECK_CM_HR(hr);
        /*hr = m_pCmKernel->SetKernelArg( 9, sizeof( UINT ), &start_x );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 10, sizeof( UINT ), &start_y );
        CHECK_CM_HR(hr);*/

        hr = m_pCmDevice->CreateTask(pGPUCopyTask);
        CHECK_CM_HR(hr);
        hr = pGPUCopyTask->AddKernel( m_pCmKernel );
        CHECK_CM_HR(hr);
        hr = m_pCmQueue->Enqueue( pGPUCopyTask, pInternalEvent, pTS );
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->DestroyTask(pGPUCopyTask);
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->DestroyThreadSpace(pTS);
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->DestroyKernel(m_pCmKernel);
        CHECK_CM_HR(hr);
        pLinearAddress += sliceCopyBufferUPSize - AddedShiftLeftOffset;
        totalBufferUPSize -= sliceCopyBufferUPSize;
        copy_height_row -= slice_copy_height_row;
        if(totalBufferUPSize > 0)   //Intermediate event, we don't need it
        {
            hr = m_pCmQueue->DestroyEvent(pInternalEvent);
        }
        else //Last one event, need keep or destroy it
        {
            hr = pInternalEvent->WaitForTaskFinished(m_timeout);
            if(hr == CM_EXCEED_MAX_TIMEOUT)
                return MFX_ERR_GPU_HANG;
            else
                CHECK_CM_HR(hr);
            hr = m_pCmQueue->DestroyEvent(pInternalEvent);
        }
        CHECK_CM_HR(hr);
    }

    return MFX_ERR_NONE;
}

mfxStatus CmCopyWrapper::EnqueueCopyNV12GPUtoCPU(   CmSurface2D* pSurface,
                                    unsigned char* pSysMem,
                                    int width,
                                    int height,
                                    const UINT widthStride,
                                    const UINT heightStride,
                                    mfxU32 format,
                                    const UINT option,
                                    CmEvent* & pEvent )
{
    (void)option;
    (void)pEvent;

    INT             hr                      = CM_SUCCESS;
    UINT            stride_in_bytes         = widthStride;
    UINT            height_stride_in_rows   = heightStride;
    UINT            AddedShiftLeftOffset    = 0;
    UINT            byte_per_pixel           = (format==MFX_FOURCC_P010
#if (MFX_VERSION >= 1031)
        || format == MFX_FOURCC_P016
#endif
        )?2:1;
    size_t          pLinearAddress          = (size_t)pSysMem;
    size_t          pLinearAddressAligned   = 0;
    CmKernel        *m_pCmKernel            = 0;
    CmBufferUP        *pCMBufferUP          = 0;
    SurfaceIndex    *pBufferIndexCM     = NULL;
    SurfaceIndex    *pSurf2DIndexCM     = NULL;
    CmThreadSpace   *pTS                = NULL;
    CmTask          *pGPUCopyTask       = NULL;
    CmEvent         *pInternalEvent     = NULL;

    UINT            threadWidth             = 0;
    UINT            threadHeight            = 0;
    UINT            threadNum               = 0;
    UINT            width_dword             = 0;
    UINT            width_byte              = 0;
    UINT            copy_width_byte         = 0;
    UINT            copy_height_row         = 0;
    UINT            slice_copy_height_row   = 0;
    UINT            sliceCopyBufferUPSize   = 0;
    INT             totalBufferUPSize       = 0;



    if ( !pSurface )
    {
        return MFX_ERR_NULL_PTR;
    }
    width_byte                      = width*byte_per_pixel;

   //Align the width regarding stride
   if(stride_in_bytes == 0)
   {
        stride_in_bytes = width_byte;
   }

   if(height_stride_in_rows == 0)
   {
        height_stride_in_rows = height;
   }

    // the actual copy region
    copy_width_byte = std::min      (stride_in_bytes, width_byte);
    copy_height_row = std::min<UINT>(height_stride_in_rows, height);

    // Make sure stride and start address of system memory is 16-byte aligned.
    // if no padding in system memory , stride_in_bytes = width_byte.
    if(stride_in_bytes & 0xf)
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
    if((pLinearAddress & 0xf) || (pLinearAddress == 0))
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    //Calculate actual total size of system memory

    totalBufferUPSize = stride_in_bytes * height_stride_in_rows + stride_in_bytes * height/2;
    if (totalBufferUPSize > CM_MAX_1D_SURF_WIDTH || height>4088)//not supported by kernel now, to be fixed in case of customer requirements
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
    pLinearAddress  = (size_t)pSysMem;


    while (totalBufferUPSize > 0)
    {
#if defined(LINUX64)//64-bit
            pLinearAddressAligned        = pLinearAddress & ADDRESS_PAGE_ALIGNMENT_MASK_X64;
#else  //32-bit
            pLinearAddressAligned        = pLinearAddress & ADDRESS_PAGE_ALIGNMENT_MASK_X86;
#endif

        //Calculate  Left Shift offset
        AddedShiftLeftOffset = (UINT)(pLinearAddress - pLinearAddressAligned);
        totalBufferUPSize   += AddedShiftLeftOffset;
        if (totalBufferUPSize > CM_MAX_1D_SURF_WIDTH)
        {
            slice_copy_height_row = ((CM_MAX_1D_SURF_WIDTH - AddedShiftLeftOffset)/(stride_in_bytes*(BLOCK_HEIGHT * INNER_LOOP))) * (BLOCK_HEIGHT * INNER_LOOP);
            sliceCopyBufferUPSize = slice_copy_height_row * stride_in_bytes * 3 / 2  + AddedShiftLeftOffset;
            return MFX_ERR_DEVICE_FAILED;
        }
        else
        {
            slice_copy_height_row = copy_height_row;
            sliceCopyBufferUPSize = totalBufferUPSize;
        }
        //map CmBufferUP instead of map/unmap each time each time for better performance and CPU utilization.
        pBufferIndexCM = CreateUpBuffer((mfxU8*)pLinearAddressAligned,sliceCopyBufferUPSize,m_tableSysRelations2,m_tableSysIndex2);
        //hr = m_pCmDevice->CreateBufferUP(  sliceCopyBufferUPSize, ( void * )pLinearAddressAligned, pCMBufferUP );
        MFX_CHECK_NULL_PTR1(pBufferIndexCM);
        hr = m_pCmDevice->CreateKernel(m_pCmProgram, CM_KERNEL_FUNCTION(surfaceCopy_read_NV12), m_pCmKernel);
        CHECK_CM_HR(hr);
        MFX_CHECK(m_pCmKernel, MFX_ERR_DEVICE_FAILED);

        //MFX_CHECK(pCMBufferUP, MFX_ERR_DEVICE_FAILED);
        //hr = pCMBufferUP->GetIndex( pBufferIndexCM );
        //CHECK_CM_HR(hr);
        std::map<mfxU8 *, CmBufferUP *>::iterator it;
        hr = pSurface->GetIndex( pSurf2DIndexCM );
        CHECK_CM_HR(hr);
        threadWidth = ( UINT )ceil( ( double )copy_width_byte/BLOCK_PIXEL_WIDTH/4 );
        threadHeight = ( UINT )ceil( ( double )slice_copy_height_row/BLOCK_HEIGHT );
        threadNum = threadWidth * threadHeight;
        hr = m_pCmKernel->SetThreadCount( threadNum );
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->CreateThreadSpace( threadWidth, threadHeight, pTS );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 1, sizeof( SurfaceIndex ), pBufferIndexCM );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 0, sizeof( SurfaceIndex ), pSurf2DIndexCM );
        CHECK_CM_HR(hr);
        width_dword = (UINT)ceil((double)width_byte / 4);

        hr = m_pCmKernel->SetKernelArg( 2, sizeof( UINT ), &width_dword );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 3, sizeof( UINT ), &height);
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 4, sizeof( UINT ), &AddedShiftLeftOffset );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 5, sizeof( UINT ), &heightStride );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 6, sizeof( UINT ), &widthStride );
        CHECK_CM_HR(hr);

        hr = m_pCmDevice->CreateTask(pGPUCopyTask);
        CHECK_CM_HR(hr);
        hr = pGPUCopyTask->AddKernel( m_pCmKernel );
        CHECK_CM_HR(hr);
        hr = m_pCmQueue->Enqueue( pGPUCopyTask, pInternalEvent, pTS );
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->DestroyTask(pGPUCopyTask);
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->DestroyThreadSpace(pTS);
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->DestroyKernel(m_pCmKernel);
        CHECK_CM_HR(hr);
        pLinearAddress += sliceCopyBufferUPSize - AddedShiftLeftOffset;
        totalBufferUPSize -= sliceCopyBufferUPSize;
        copy_height_row -= slice_copy_height_row;
        if(totalBufferUPSize > 0)   //Intermediate event, we don't need it
        {
            hr = m_pCmQueue->DestroyEvent(pInternalEvent);
        }
        else //Last one event, need keep or destroy it
        {
            hr = pInternalEvent->WaitForTaskFinished(m_timeout);
            if(hr == CM_EXCEED_MAX_TIMEOUT)
                return MFX_ERR_GPU_HANG;
            else
                CHECK_CM_HR(hr);
            hr = m_pCmQueue->DestroyEvent(pInternalEvent);
        }
        CHECK_CM_HR(hr);
    }

    return MFX_ERR_NONE;
}

mfxStatus CmCopyWrapper::EnqueueCopyMirrorNV12CPUtoGPU(CmSurface2D* pSurface,
                                    unsigned char* pSysMem,
                                    int width,
                                    int height,
                                    const UINT widthStride,
                                    const UINT heightStride,
                                    mfxU32 format,
                                    const UINT option,
                                    CmEvent* & pEvent )
{
    (void)format;
    (void)option;
    (void)pEvent;

    INT             hr                      = CM_SUCCESS;
    UINT            stride_in_bytes         = widthStride;
    UINT            stride_in_dwords        = 0;
    UINT            height_stride_in_rows   = heightStride;
    UINT            AddedShiftLeftOffset    = 0;
    size_t          pLinearAddress          = (size_t)pSysMem;
    size_t          pLinearAddressAligned   = 0;
    CmKernel        *m_pCmKernel            = 0;
    CmBufferUP        *pCMBufferUP          = 0;
    SurfaceIndex    *pBufferIndexCM     = NULL;
    SurfaceIndex    *pSurf2DIndexCM     = NULL;
    CmThreadSpace   *pTS                = NULL;
    CmTask          *pGPUCopyTask       = NULL;
    CmEvent         *pInternalEvent     = NULL;

    UINT            threadWidth             = 0;
    UINT            threadHeight            = 0;
    UINT            threadNum               = 0;
    UINT            width_dword             = 0;
    UINT            width_byte              = 0;
    UINT            copy_width_byte         = 0;
    UINT            copy_height_row         = 0;
    UINT            slice_copy_height_row   = 0;
    UINT            sliceCopyBufferUPSize   = 0;
    INT             totalBufferUPSize       = 0;



    if ( !pSurface )
    {
        return MFX_ERR_NULL_PTR;
    }
    width_byte                      = width;

   //Align the width regarding stride
   if(stride_in_bytes == 0)
   {
        stride_in_bytes = width_byte;
   }

   if(height_stride_in_rows == 0)
   {
        height_stride_in_rows = height;
   }

    // the actual copy region
    copy_width_byte = std::min      (stride_in_bytes, width_byte);
    copy_height_row = std::min<UINT>(height_stride_in_rows, height);

    // Make sure stride and start address of system memory is 16-byte aligned.
    // if no padding in system memory , stride_in_bytes = width_byte.
    if(stride_in_bytes & 0xf)
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
    if((pLinearAddress & 0xf) || (pLinearAddress == 0))
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    //Calculate actual total size of system memory

    totalBufferUPSize = stride_in_bytes * height_stride_in_rows + stride_in_bytes * height/2;

    pLinearAddress  = (size_t)pSysMem;

    while (totalBufferUPSize > 0)
    {
#if defined(LINUX64)//64-bit
            pLinearAddressAligned        = pLinearAddress & ADDRESS_PAGE_ALIGNMENT_MASK_X64;
#else  //32-bit
            pLinearAddressAligned        = pLinearAddress & ADDRESS_PAGE_ALIGNMENT_MASK_X86;
#endif

        //Calculate  Left Shift offset
        AddedShiftLeftOffset = (UINT)(pLinearAddress - pLinearAddressAligned);
        totalBufferUPSize   += AddedShiftLeftOffset;
        if (totalBufferUPSize > CM_MAX_1D_SURF_WIDTH)
        {
            slice_copy_height_row = ((CM_MAX_1D_SURF_WIDTH - AddedShiftLeftOffset)/(stride_in_bytes*(BLOCK_HEIGHT * INNER_LOOP))) * (BLOCK_HEIGHT * INNER_LOOP);
            sliceCopyBufferUPSize = slice_copy_height_row * stride_in_bytes * 3 / 2  + AddedShiftLeftOffset;
            return MFX_ERR_DEVICE_FAILED;
        }
        else
        {
            slice_copy_height_row = copy_height_row;
            sliceCopyBufferUPSize = totalBufferUPSize;
        }

        pBufferIndexCM = CreateUpBuffer((mfxU8*)pLinearAddressAligned,sliceCopyBufferUPSize,m_tableSysRelations2,m_tableSysIndex2);
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->CreateKernel(m_pCmProgram, CM_KERNEL_FUNCTION(surfaceMirror_write_NV12), m_pCmKernel);
        CHECK_CM_HR(hr);
        MFX_CHECK(m_pCmKernel, MFX_ERR_DEVICE_FAILED);

        hr = pSurface->GetIndex( pSurf2DIndexCM );
        CHECK_CM_HR(hr);
        threadWidth = ( UINT )ceil( ( double )copy_width_byte/BLOCK_PIXEL_WIDTH/4 );
        threadHeight = ( UINT )ceil( ( double )slice_copy_height_row/BLOCK_HEIGHT );
        threadNum = threadWidth * threadHeight;
        hr = m_pCmKernel->SetThreadCount( threadNum );
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->CreateThreadSpace( threadWidth, threadHeight, pTS );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 0, sizeof( SurfaceIndex ), pBufferIndexCM );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 1, sizeof( SurfaceIndex ), pSurf2DIndexCM );
        CHECK_CM_HR(hr);
        width_dword = (UINT)ceil((double)width_byte / 4);
        stride_in_dwords = (UINT)ceil((double)stride_in_bytes / 4);

        hr = m_pCmKernel->SetKernelArg( 2, sizeof( UINT ), &stride_in_dwords );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 3, sizeof( UINT ), &height_stride_in_rows);
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 4, sizeof( UINT ), &AddedShiftLeftOffset );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 5, sizeof( UINT ), &width_dword );
        CHECK_CM_HR(hr);


        hr = m_pCmDevice->CreateTask(pGPUCopyTask);
        CHECK_CM_HR(hr);
        hr = pGPUCopyTask->AddKernel( m_pCmKernel );
        CHECK_CM_HR(hr);
        hr = m_pCmQueue->Enqueue( pGPUCopyTask, pInternalEvent, pTS );
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->DestroyTask(pGPUCopyTask);
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->DestroyThreadSpace(pTS);
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->DestroyKernel(m_pCmKernel);
        CHECK_CM_HR(hr);
        pLinearAddress += sliceCopyBufferUPSize - AddedShiftLeftOffset;
        totalBufferUPSize -= sliceCopyBufferUPSize;
        copy_height_row -= slice_copy_height_row;
        if(totalBufferUPSize > 0)   //Intermediate event, we don't need it
        {
            hr = m_pCmQueue->DestroyEvent(pInternalEvent);
        }
        else //Last one event, need keep or destroy it
        {
            hr = pInternalEvent->WaitForTaskFinished(m_timeout);
            if(hr == CM_EXCEED_MAX_TIMEOUT)
                return MFX_ERR_GPU_HANG;
            else
                CHECK_CM_HR(hr);
            hr = m_pCmQueue->DestroyEvent(pInternalEvent);
        }
        CHECK_CM_HR(hr);
    }

    return MFX_ERR_NONE;
}

mfxStatus CmCopyWrapper::EnqueueCopyNV12CPUtoGPU(CmSurface2D* pSurface,
                                    unsigned char* pSysMem,
                                    int width,
                                    int height,
                                    const UINT widthStride,
                                    const UINT heightStride,
                                    mfxU32 format,
                                    const UINT option,
                                    CmEvent* & pEvent )
{
    (void)option;
    (void)pEvent;

    INT             hr                      = CM_SUCCESS;
    UINT            stride_in_bytes         = widthStride;
    UINT            height_stride_in_rows   = heightStride;
    UINT            byte_per_pixel          = (format==MFX_FOURCC_P010
#if (MFX_VERSION >= 1031)
        || format == MFX_FOURCC_P016
#endif
        )?2:1;
    UINT            AddedShiftLeftOffset    = 0;
    size_t          pLinearAddress          = (size_t)pSysMem;
    size_t          pLinearAddressAligned   = 0;
    CmKernel        *m_pCmKernel            = 0;
    CmBufferUP      *pCMBufferUP          = 0;
    SurfaceIndex    *pBufferIndexCM     = NULL;
    SurfaceIndex    *pSurf2DIndexCM     = NULL;
    CmThreadSpace   *pTS                = NULL;
    CmTask          *pGPUCopyTask       = NULL;
    CmEvent         *pInternalEvent     = NULL;

    UINT            threadWidth             = 0;
    UINT            threadHeight            = 0;
    UINT            threadNum               = 0;
    UINT            width_dword             = 0;
    UINT            width_byte              = 0;
    UINT            copy_width_byte         = 0;
    UINT            copy_height_row         = 0;
    UINT            slice_copy_height_row   = 0;
    UINT            sliceCopyBufferUPSize   = 0;
    INT             totalBufferUPSize       = 0;

    if ( !pSurface )
    {
        return MFX_ERR_NULL_PTR;
    }
    width_byte                      = width*byte_per_pixel;

    //Align the width regarding stride
    if(stride_in_bytes == 0)
    {
         stride_in_bytes = width_byte;
    }

    if(height_stride_in_rows == 0)
    {
         height_stride_in_rows = height;
    }

    // the actual copy region
    copy_width_byte = std::min      (stride_in_bytes, width_byte);
    copy_height_row = std::min<UINT>(height_stride_in_rows, height);

    // Make sure stride and start address of system memory is 16-byte aligned.
    // if no padding in system memory , stride_in_bytes = width_byte.
    if(stride_in_bytes & 0xf)
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
    if((pLinearAddress & 0xf) || (pLinearAddress == 0))
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    //Calculate actual total size of system memory

    totalBufferUPSize = stride_in_bytes * height_stride_in_rows + stride_in_bytes * height/2;
    if (totalBufferUPSize > CM_MAX_1D_SURF_WIDTH || height > 4088)//not supported by kernel now, to be fixed in case of customer requirements
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
    pLinearAddress  = (size_t)pSysMem;


    while (totalBufferUPSize > 0)
    {
#if defined(LINUX64)//64-bit
            pLinearAddressAligned        = pLinearAddress & ADDRESS_PAGE_ALIGNMENT_MASK_X64;
#else  //32-bit
            pLinearAddressAligned        = pLinearAddress & ADDRESS_PAGE_ALIGNMENT_MASK_X86;
#endif

        //Calculate  Left Shift offset
        AddedShiftLeftOffset = (UINT)(pLinearAddress - pLinearAddressAligned);
        totalBufferUPSize   += AddedShiftLeftOffset;
        if (totalBufferUPSize > CM_MAX_1D_SURF_WIDTH)
        {
            slice_copy_height_row = ((CM_MAX_1D_SURF_WIDTH - AddedShiftLeftOffset)/(stride_in_bytes*(BLOCK_HEIGHT * INNER_LOOP))) * (BLOCK_HEIGHT * INNER_LOOP);
            sliceCopyBufferUPSize = slice_copy_height_row * stride_in_bytes * 3 / 2  + AddedShiftLeftOffset;
            return MFX_ERR_DEVICE_FAILED;
        }
        else
        {
            slice_copy_height_row = copy_height_row;
            sliceCopyBufferUPSize = totalBufferUPSize;
        }
        //map CmBufferUP instead of map/unmap each time for better performance and CPU utilization.
        pBufferIndexCM = CreateUpBuffer((mfxU8*)pLinearAddressAligned,sliceCopyBufferUPSize,m_tableSysRelations2,m_tableSysIndex2);
        MFX_CHECK_NULL_PTR1(pBufferIndexCM);
        hr = m_pCmDevice->CreateKernel(m_pCmProgram, CM_KERNEL_FUNCTION(surfaceCopy_write_NV12), m_pCmKernel);
        CHECK_CM_HR(hr);
        MFX_CHECK(m_pCmKernel, MFX_ERR_DEVICE_FAILED);

        //MFX_CHECK(pCMBufferUP, MFX_ERR_DEVICE_FAILED);
        //hr = pCMBufferUP->GetIndex( pBufferIndexCM );
        CHECK_CM_HR(hr);
        hr = pSurface->GetIndex( pSurf2DIndexCM );
        CHECK_CM_HR(hr);
        threadWidth = ( UINT )ceil( ( double )copy_width_byte/BLOCK_PIXEL_WIDTH/4 );
        threadHeight = ( UINT )ceil( ( double )slice_copy_height_row/BLOCK_HEIGHT );
        threadNum = threadWidth * threadHeight;
        hr = m_pCmKernel->SetThreadCount( threadNum );
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->CreateThreadSpace( threadWidth, threadHeight, pTS );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 0, sizeof( SurfaceIndex ), pBufferIndexCM );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 1, sizeof( SurfaceIndex ), pSurf2DIndexCM );
        CHECK_CM_HR(hr);
        width_dword = (UINT)ceil((double)width_byte / 4);

        hr = m_pCmKernel->SetKernelArg( 2, sizeof( UINT ), &width_dword );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 3, sizeof( UINT ), &height);
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 4, sizeof( UINT ), &AddedShiftLeftOffset );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 5, sizeof( UINT ), &stride_in_bytes );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 6, sizeof( UINT ), &height_stride_in_rows );
        CHECK_CM_HR(hr);

        hr = m_pCmDevice->CreateTask(pGPUCopyTask);
        CHECK_CM_HR(hr);
        hr = pGPUCopyTask->AddKernel( m_pCmKernel );
        CHECK_CM_HR(hr);
        hr = m_pCmQueue->Enqueue( pGPUCopyTask, pInternalEvent, pTS );
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->DestroyTask(pGPUCopyTask);
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->DestroyThreadSpace(pTS);
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->DestroyKernel(m_pCmKernel);
        CHECK_CM_HR(hr);
        pLinearAddress += sliceCopyBufferUPSize - AddedShiftLeftOffset;
        totalBufferUPSize -= sliceCopyBufferUPSize;
        copy_height_row -= slice_copy_height_row;
        if(totalBufferUPSize > 0)   //Intermediate event, we don't need it
        {
            hr = m_pCmQueue->DestroyEvent(pInternalEvent);
        }
        else //Last one event, need keep or destroy it
        {
            hr = pInternalEvent->WaitForTaskFinished(m_timeout);
            if(hr == CM_EXCEED_MAX_TIMEOUT)
                return MFX_ERR_GPU_HANG;
            else
                CHECK_CM_HR(hr);
            hr = m_pCmQueue->DestroyEvent(pInternalEvent);
        }
        CHECK_CM_HR(hr);
    }

    return MFX_ERR_NONE;
}

mfxStatus CmCopyWrapper::EnqueueCopyShiftP010GPUtoCPU(   CmSurface2D* pSurface,
                                    unsigned char* pSysMem,
                                    int width,
                                    int height,
                                    const UINT widthStride,
                                    const UINT heightStride,
                                    mfxU32 format,
                                    const UINT option,
                                    int bitshift,
                                    CmEvent* & pEvent )
{
    (void)format;
    (void)option;
    (void)pEvent;

    INT             hr                      = CM_SUCCESS;
    UINT            stride_in_bytes         = widthStride;
    UINT            stride_in_dwords        = 0;
    UINT            height_stride_in_rows   = heightStride;
    UINT            AddedShiftLeftOffset    = 0;
    size_t          pLinearAddress          = (size_t)pSysMem;
    size_t          pLinearAddressAligned   = 0;
    CmKernel        *m_pCmKernel            = 0;
    CmBufferUP        *pCMBufferUP          = 0;
    SurfaceIndex    *pBufferIndexCM     = NULL;
    SurfaceIndex    *pSurf2DIndexCM     = NULL;
    CmThreadSpace   *pTS                = NULL;
    CmTask          *pGPUCopyTask       = NULL;
    CmEvent         *pInternalEvent     = NULL;//(CmEvent*)-1;

    UINT            threadWidth             = 0;
    UINT            threadHeight            = 0;
    UINT            threadNum               = 0;
    UINT            width_dword             = 0;
    UINT            width_byte              = 0;
    UINT            copy_width_byte         = 0;
    UINT            copy_height_row         = 0;
    UINT            slice_copy_height_row   = 0;
    UINT            sliceCopyBufferUPSize   = 0;
    INT             totalBufferUPSize       = 0;



    if ( !pSurface )
    {
        return MFX_ERR_NULL_PTR;
    }
    width_byte                      = width*2;

   //Align the width regarding stride
   if(stride_in_bytes == 0)
   {
        stride_in_bytes = width_byte;
   }

   if(height_stride_in_rows == 0)
   {
        height_stride_in_rows = height;
   }

    // the actual copy region
    copy_width_byte = std::min      (stride_in_bytes, width_byte);
    copy_height_row = std::min<UINT>(height_stride_in_rows, height);

    // Make sure stride and start address of system memory is 16-byte aligned.
    // if no padding in system memory , stride_in_bytes = width_byte.
    if(stride_in_bytes & 0xf)
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
    if((pLinearAddress & 0xf) || (pLinearAddress == 0))
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    //Calculate actual total size of system memory

    totalBufferUPSize = stride_in_bytes * height_stride_in_rows + stride_in_bytes * height/2;

    pLinearAddress  = (size_t)pSysMem;


    while (totalBufferUPSize > 0)
    {
#if defined(LINUX64)//64-bit
            pLinearAddressAligned        = pLinearAddress & ADDRESS_PAGE_ALIGNMENT_MASK_X64;
#else  //32-bit
            pLinearAddressAligned        = pLinearAddress & ADDRESS_PAGE_ALIGNMENT_MASK_X86;
#endif

        //Calculate  Left Shift offset
        AddedShiftLeftOffset = (UINT)(pLinearAddress - pLinearAddressAligned);
        totalBufferUPSize   += AddedShiftLeftOffset;
        if (totalBufferUPSize > CM_MAX_1D_SURF_WIDTH)
        {
            slice_copy_height_row = ((CM_MAX_1D_SURF_WIDTH - AddedShiftLeftOffset)/(stride_in_bytes*(BLOCK_HEIGHT * INNER_LOOP))) * (BLOCK_HEIGHT * INNER_LOOP);
            sliceCopyBufferUPSize = slice_copy_height_row * stride_in_bytes * 3 / 2  + AddedShiftLeftOffset;
            return MFX_ERR_DEVICE_FAILED;
        }
        else
        {
            slice_copy_height_row = copy_height_row;
            sliceCopyBufferUPSize = totalBufferUPSize;
        }

        pBufferIndexCM = CreateUpBuffer((mfxU8*)pLinearAddressAligned,sliceCopyBufferUPSize,m_tableSysRelations2,m_tableSysIndex2);
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->CreateKernel(m_pCmProgram, CM_KERNEL_FUNCTION(surfaceCopy_read_P010_shift), m_pCmKernel);
        CHECK_CM_HR(hr);
        MFX_CHECK(m_pCmKernel, MFX_ERR_DEVICE_FAILED);

        CHECK_CM_HR(hr);
        hr = pSurface->GetIndex( pSurf2DIndexCM );
        CHECK_CM_HR(hr);
        threadWidth = ( UINT )ceil( ( double )copy_width_byte/BLOCK_PIXEL_WIDTH/4 );
        threadHeight = ( UINT )ceil( ( double )slice_copy_height_row/BLOCK_HEIGHT );
        threadNum = threadWidth * threadHeight;
        hr = m_pCmKernel->SetThreadCount( threadNum );
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->CreateThreadSpace( threadWidth, threadHeight, pTS );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 1, sizeof( SurfaceIndex ), pBufferIndexCM );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 0, sizeof( SurfaceIndex ), pSurf2DIndexCM );
        CHECK_CM_HR(hr);
        width_dword = (UINT)ceil((double)width_byte / 4);
        stride_in_dwords = (UINT)ceil((double)stride_in_bytes / 4);

        hr = m_pCmKernel->SetKernelArg( 2, sizeof( UINT ), &width_dword );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 3, sizeof( UINT ), &height);
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 4, sizeof( UINT ), &AddedShiftLeftOffset );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 5, sizeof( UINT ), &bitshift );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 6, sizeof( UINT ), &stride_in_dwords );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 7, sizeof( UINT ), &height_stride_in_rows );
        CHECK_CM_HR(hr);

        hr = m_pCmDevice->CreateTask(pGPUCopyTask);
        CHECK_CM_HR(hr);
        hr = pGPUCopyTask->AddKernel( m_pCmKernel );
        CHECK_CM_HR(hr);
        hr = m_pCmQueue->Enqueue( pGPUCopyTask, pInternalEvent, pTS );
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->DestroyTask(pGPUCopyTask);
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->DestroyThreadSpace(pTS);
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->DestroyKernel(m_pCmKernel);
        CHECK_CM_HR(hr);
        pLinearAddress += sliceCopyBufferUPSize - AddedShiftLeftOffset;
        totalBufferUPSize -= sliceCopyBufferUPSize;
        copy_height_row -= slice_copy_height_row;
        if(totalBufferUPSize > 0)   //Intermediate event, we don't need it
        {
            hr = m_pCmQueue->DestroyEvent(pInternalEvent);
        }
        else //Last one event
        {
            hr = pInternalEvent->WaitForTaskFinished(m_timeout);
            if(hr == CM_EXCEED_MAX_TIMEOUT)
                return MFX_ERR_GPU_HANG;
            else
                CHECK_CM_HR(hr);
            hr = m_pCmQueue->DestroyEvent(pInternalEvent);
        }
        CHECK_CM_HR(hr);
    }

    return MFX_ERR_NONE;
}
mfxStatus CmCopyWrapper::EnqueueCopyShiftP010CPUtoGPU(   CmSurface2D* pSurface,
                                    unsigned char* pSysMem,
                                    int width,
                                    int height,
                                    const UINT widthStride,
                                    const UINT heightStride,
                                    mfxU32 format,
                                    const UINT option,
                                    int bitshift,
                                    CmEvent* & pEvent )
{
    (void)format;
    (void)option;
    (void)pEvent;

    INT             hr                      = CM_SUCCESS;
    UINT            stride_in_bytes         = widthStride;
    UINT            stride_in_dwords        = 0;
    UINT            height_stride_in_rows   = heightStride;
    UINT            AddedShiftLeftOffset    = 0;
    size_t          pLinearAddress          = (size_t)pSysMem;
    size_t          pLinearAddressAligned   = 0;

    CmKernel        *m_pCmKernel        = NULL;
    CmBufferUP      *pCMBufferUP        = NULL;
    SurfaceIndex    *pBufferIndexCM     = NULL;
    SurfaceIndex    *pSurf2DIndexCM     = NULL;
    CmThreadSpace   *pTS                = NULL;
    CmTask          *pGPUCopyTask       = NULL;
    CmEvent         *pInternalEvent     = NULL;

    UINT            threadWidth             = 0;
    UINT            threadHeight            = 0;
    UINT            threadNum               = 0;
    UINT            width_byte              = 0;
    UINT            copy_width_byte         = 0;
    UINT            copy_height_row         = 0;
    UINT            slice_copy_height_row   = 0;
    UINT            sliceCopyBufferUPSize   = 0;
    INT             totalBufferUPSize       = 0;



    if ( !pSurface )
    {
        return MFX_ERR_NULL_PTR;
    }
    width_byte                      = width * 2;

   //Align the width regarding stride
   if(stride_in_bytes == 0)
   {
        stride_in_bytes = width_byte;
   }

   if(height_stride_in_rows == 0)
   {
        height_stride_in_rows = height;
   }

    // the actual copy region
    copy_width_byte = std::min      (stride_in_bytes, width_byte);
    copy_height_row = std::min<UINT>(height_stride_in_rows, height);

    // Make sure stride and start address of system memory is 16-byte aligned.
    // if no padding in system memory , stride_in_bytes = width_byte.
    if(stride_in_bytes & 0xf)
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
    if((pLinearAddress & 0xf) || (pLinearAddress == 0))
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    //Calculate actual total size of system memory

    totalBufferUPSize = stride_in_bytes * height_stride_in_rows + stride_in_bytes * height/2;

    pLinearAddress  = (size_t)pSysMem;


    while (totalBufferUPSize > 0)
    {
#if defined(LINUX64)//64-bit
            pLinearAddressAligned        = pLinearAddress & ADDRESS_PAGE_ALIGNMENT_MASK_X64;
#else  //32-bit
            pLinearAddressAligned        = pLinearAddress & ADDRESS_PAGE_ALIGNMENT_MASK_X86;
#endif

        //Calculate  Left Shift offset
        AddedShiftLeftOffset = (UINT)(pLinearAddress - pLinearAddressAligned);
        totalBufferUPSize   += AddedShiftLeftOffset;
        if (totalBufferUPSize > CM_MAX_1D_SURF_WIDTH)
        {
            slice_copy_height_row = ((CM_MAX_1D_SURF_WIDTH - AddedShiftLeftOffset)/(stride_in_bytes*(BLOCK_HEIGHT * INNER_LOOP))) * (BLOCK_HEIGHT * INNER_LOOP);
            sliceCopyBufferUPSize = slice_copy_height_row * stride_in_bytes + AddedShiftLeftOffset;
        }
        else
        {
            slice_copy_height_row = copy_height_row;
            sliceCopyBufferUPSize = totalBufferUPSize;
        }

        pBufferIndexCM = CreateUpBuffer((mfxU8*)pLinearAddressAligned,sliceCopyBufferUPSize,m_tableSysRelations2,m_tableSysIndex2);
        hr = m_pCmDevice->CreateKernel(m_pCmProgram, CM_KERNEL_FUNCTION(surfaceCopy_write_P010_shift), m_pCmKernel);
        CHECK_CM_HR(hr);

        MFX_CHECK(m_pCmKernel, MFX_ERR_DEVICE_FAILED);

        hr = pSurface->GetIndex( pSurf2DIndexCM );
        CHECK_CM_HR(hr);
        threadWidth = ( UINT )ceil( ( double )copy_width_byte/BLOCK_PIXEL_WIDTH/4 );
        threadHeight = ( UINT )ceil( ( double )slice_copy_height_row/BLOCK_HEIGHT );
        threadNum = threadWidth * threadHeight;
        hr = m_pCmKernel->SetThreadCount( threadNum );
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->CreateThreadSpace( threadWidth, threadHeight, pTS );
        CHECK_CM_HR(hr);

        m_pCmKernel->SetKernelArg( 0, sizeof( SurfaceIndex ), pBufferIndexCM);
        CHECK_CM_HR(hr);
        m_pCmKernel->SetKernelArg( 1, sizeof( SurfaceIndex ), pSurf2DIndexCM);
        CHECK_CM_HR(hr);
        stride_in_dwords = (UINT)ceil((double)stride_in_bytes / 4);
        hr = m_pCmKernel->SetKernelArg( 2, sizeof( UINT ), &stride_in_dwords );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 3, sizeof( UINT ), &height_stride_in_rows );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 4, sizeof( UINT ), &AddedShiftLeftOffset );
        CHECK_CM_HR(hr);
        hr = m_pCmKernel->SetKernelArg( 5, sizeof( UINT ), &bitshift );
        CHECK_CM_HR(hr);


        hr = m_pCmDevice->CreateTask(pGPUCopyTask);
        CHECK_CM_HR(hr);
        hr = pGPUCopyTask->AddKernel( m_pCmKernel );
        CHECK_CM_HR(hr);
        hr = m_pCmQueue->Enqueue( pGPUCopyTask, pInternalEvent, pTS );
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->DestroyTask(pGPUCopyTask);
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->DestroyThreadSpace(pTS);
        CHECK_CM_HR(hr);
        hr = m_pCmDevice->DestroyKernel(m_pCmKernel);
        CHECK_CM_HR(hr);
        pLinearAddress += sliceCopyBufferUPSize - AddedShiftLeftOffset;
        totalBufferUPSize -= sliceCopyBufferUPSize;
        copy_height_row -= slice_copy_height_row;
        if(totalBufferUPSize > 0)   //Intermediate event, we don't need it
        {
            hr = m_pCmQueue->DestroyEvent(pInternalEvent);
        }
        else //Last one event, need keep or destroy it
        {
            hr = pInternalEvent->WaitForTaskFinished(m_timeout);
            if(hr == CM_EXCEED_MAX_TIMEOUT)
                return MFX_ERR_GPU_HANG;
            else
                CHECK_CM_HR(hr);
            hr = m_pCmQueue->DestroyEvent(pInternalEvent);
        }
        CHECK_CM_HR(hr);
    }

    return MFX_ERR_NONE;
}
mfxStatus CmCopyWrapper::Initialize(eMFXHWType hwtype)
{
    cmStatus cmSts = CM_SUCCESS;

    if (!m_pCmDevice)
        return MFX_ERR_DEVICE_FAILED;

    m_HWType = hwtype;
    if (m_HWType == MFX_HW_UNKNOWN)
        return MFX_ERR_UNDEFINED_BEHAVIOR;

    m_timeout = m_HWType >= MFX_HW_ICL ? CM_MAX_TIMEOUT_SIM : CM_MAX_TIMEOUT_MS;

    if(hwtype >= MFX_HW_BDW)
    {
        mfxStatus mfxSts = InitializeSwapKernels(hwtype);
        MFX_CHECK_STS(mfxSts);
    }
    cmSts = m_pCmDevice->CreateQueue(m_pCmQueue);
    CHECK_CM_STATUS(cmSts, MFX_ERR_DEVICE_FAILED);
    m_tableCmRelations2.clear();
    m_tableCmIndex2.clear();
    m_tableSysRelations2.clear();
    m_tableSysIndex2.clear();

    return MFX_ERR_NONE;

} // mfxStatus CmCopyWrapper::Initialize(void)
mfxStatus CmCopyWrapper::InitializeSwapKernels(eMFXHWType hwtype)
{
    cmStatus cmSts = CM_SUCCESS;

    if (!m_pCmDevice)
        return MFX_ERR_DEVICE_FAILED;

    switch (hwtype)
    {
#ifdef MFX_ENABLE_KERNELS
    case MFX_HW_BDW:
    case MFX_HW_CHT:
        cmSts = m_pCmDevice->LoadProgram((void*)genx_copy_kernel_gen8,sizeof(genx_copy_kernel_gen8),m_pCmProgram,"nojitter");
        break;
    case MFX_HW_SCL:
    case MFX_HW_APL:
    case MFX_HW_KBL:
    case MFX_HW_GLK:
    case MFX_HW_CFL:
        cmSts = m_pCmDevice->LoadProgram((void*)genx_copy_kernel_gen9,sizeof(genx_copy_kernel_gen9),m_pCmProgram,"nojitter");
        break;
    case MFX_HW_ICL:
        cmSts = m_pCmDevice->LoadProgram((void*)genx_copy_kernel_gen11,sizeof(genx_copy_kernel_gen11),m_pCmProgram,"nojitter");
        break;
    case MFX_HW_ICL_LP:
#if (MFX_VERSION >= 1031)
    case MFX_HW_JSL:
    case MFX_HW_EHL:
#endif
        cmSts = m_pCmDevice->LoadProgram((void*)genx_copy_kernel_gen11lp,sizeof(genx_copy_kernel_gen11lp),m_pCmProgram,"nojitter");
        break;
#if (MFX_VERSION >= 1031)
    case MFX_HW_TGL_LP:
    case MFX_HW_DG1:
    case MFX_HW_RKL:
    case MFX_HW_ADL_S:
        cmSts = m_pCmDevice->LoadProgram((void*)genx_copy_kernel_gen12lp,sizeof(genx_copy_kernel_gen12lp),m_pCmProgram,"nojitter");
        break;
#endif
#endif
    default:
        cmSts = CM_FAILURE;
        break;
    }
    CHECK_CM_STATUS(cmSts, MFX_ERR_DEVICE_FAILED);

    return MFX_ERR_NONE;

} // mfxStatus CmCopyWrapper::Initialize(void)

mfxStatus CmCopyWrapper::ReleaseCmSurfaces(void)
{
    UMC::AutomaticUMCMutex guard(m_guard);

    if (m_pCmDevice) {
        for (std::vector<CmBufferUP*>::reverse_iterator item = m_buffersInCreationOrder.rbegin(); item != m_buffersInCreationOrder.rend(); ++item) {
            m_pCmDevice->DestroyBufferUP(*item);
        }
        for (std::vector<CmSurface2D*>::reverse_iterator item = m_surfacesInCreationOrder.rbegin(); item != m_surfacesInCreationOrder.rend(); ++item) {
            m_pCmDevice->DestroySurface(*item);
        }
    }

    m_buffersInCreationOrder.clear();
    m_surfacesInCreationOrder.clear();

    m_tableCmRelations2.clear();
    m_tableSysRelations2.clear();

    m_tableCmIndex2.clear();
    m_tableSysIndex2.clear();

    return MFX_ERR_NONE;
}
mfxStatus CmCopyWrapper::Release(void)
{

    ReleaseCmSurfaces();

    if (m_pCmProgram)
    {
        m_pCmDevice->DestroyProgram(m_pCmProgram);
    }

    m_pCmProgram = NULL;

    if (m_pThreadSpace)
    {
        m_pCmDevice->DestroyThreadSpace(m_pThreadSpace);
    }

    m_pThreadSpace = NULL;

    if (m_pCmTask1)
    {
        m_pCmDevice->DestroyTask(m_pCmTask1);
    }

    m_pCmTask1 = NULL;

    if (m_pCmTask2)
    {
        m_pCmDevice->DestroyTask(m_pCmTask2);
    }

    m_pCmTask2 = NULL;

    if (m_pCmDevice)
    {
        DestroyCmDevice(m_pCmDevice);
    }

    m_pCmDevice = NULL;

    return MFX_ERR_NONE;

} // mfxStatus CmCopyWrapper::Release(void)

CmSurface2D * CmCopyWrapper::CreateCmSurface2D(const mfxHDLPair & hdl, mfxU32 width, mfxU32 height, bool isSecondMode,
                                               std::map<mfxHDLPair, CmSurface2D *> & tableCmRelations,
                                               std::map<CmSurface2D *, SurfaceIndex *> & tableCmIndex)
{
    cmStatus cmSts = 0;

    CmSurface2D *pCmSurface2D = nullptr;
    SurfaceIndex *pCmSrcIndex = nullptr;

    std::map<mfxHDLPair, CmSurface2D *>::iterator it = tableCmRelations.find(hdl);


    if (tableCmRelations.end() == it)
    {
        UMC::AutomaticUMCMutex guard(m_guard);
        if (true == isSecondMode)
        {

#if defined(MFX_VA_LINUX)
            m_pCmDevice->CreateSurface2D(width, height, (CM_SURFACE_FORMAT)(MFX_MAKEFOURCC('N', 'V', '1', '2')), pCmSurface2D);
#endif
        }
        else
        {
            cmSts = m_pCmDevice->CreateSurface2D(hdl, pCmSurface2D);

            CHECK_CM_STATUS_RET_NULL(cmSts, MFX_ERR_DEVICE_FAILED);
            tableCmRelations.insert(std::pair<mfxHDLPair, CmSurface2D *>(hdl, pCmSurface2D));
        }

        cmSts = pCmSurface2D->GetIndex(pCmSrcIndex);
        CHECK_CM_STATUS_RET_NULL(cmSts, MFX_ERR_DEVICE_FAILED);

        tableCmIndex.insert(std::pair<CmSurface2D *, SurfaceIndex *>(pCmSurface2D, pCmSrcIndex));
        m_surfacesInCreationOrder.push_back(pCmSurface2D);
    }
    else
    {
        pCmSurface2D = it->second;
    }

    return pCmSurface2D;
}


mfxStatus CmCopyWrapper::IsCmCopySupported(mfxFrameSurface1 *pSurface, mfxSize roi)
{
    if ((roi.width & 15) || (roi.height & 7))
    {
        return MFX_ERR_UNSUPPORTED;
    }

    if(pSurface->Info.FourCC != MFX_FOURCC_NV12)
    {
        return MFX_ERR_UNSUPPORTED;
    }

    if(pSurface->Data.UV - pSurface->Data.Y != pSurface->Data.Pitch * pSurface->Info.Height)
    {
        return MFX_ERR_UNSUPPORTED;
    }

    return MFX_ERR_NONE;

} // mfxStatus CmCopyWrapper::IsCmCopySupported(mfxFrameSurface1 *pSurface, mfxSize roi)

mfxStatus CmCopyWrapper::CopySystemToVideoMemoryAPI(mfxMemId dst, mfxU32 dstPitch, mfxU8 *pSrc, mfxU32 srcPitch, mfxU32 srcUVOffset, mfxSize roi)
{
    (void)dstPitch;

    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "CmCopyWrapper::CopySystemToVideoMemoryAPI");
    cmStatus cmSts = 0;

    CmEvent* e = NULL;
    //CM_STATUS sts;
    mfxStatus status = MFX_ERR_NONE;

    mfxU32 width  = roi.width;
    mfxU32 height = roi.height;

     // create or find already associated cm surface 2d
    CmSurface2D *pCmSurface2D;
    mfxHDLPair pair = {dst, nullptr};

    pCmSurface2D = CreateCmSurface2D(pair, width, height, false, m_tableCmRelations2, m_tableCmIndex2);
    CHECK_CM_NULL_PTR(pCmSurface2D, MFX_ERR_DEVICE_FAILED);

    cmSts = m_pCmQueue->EnqueueCopyCPUToGPUFullStride(pCmSurface2D, pSrc, srcPitch, srcUVOffset, CM_FASTCOPY_OPTION_NONBLOCKING, e);

    if (CM_SUCCESS == cmSts)
    {
        cmSts = e->WaitForTaskFinished(m_timeout);
        if(cmSts == CM_EXCEED_MAX_TIMEOUT)
        {
            status = MFX_ERR_GPU_HANG;
        }
    }
    else
    {
        status = MFX_ERR_DEVICE_FAILED;
    }
    m_pCmQueue->DestroyEvent(e);

    return status;
}
mfxStatus CmCopyWrapper::CopySystemToVideoMemory(mfxMemId dst, mfxU32 dstPitch, mfxU8 *pSrc, mfxU32 srcPitch, mfxU32 srcUVOffset, mfxSize roi, mfxU32 format)
{
    (void)dstPitch;

    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "CmCopyWrapper::CopySystemToVideoMemory");
    cmStatus cmSts = 0;

    CmEvent* e = CM_NO_EVENT;
    //CM_STATUS sts;
    mfxStatus status = MFX_ERR_NONE;

    mfxU32 width  = roi.width;
    mfxU32 height = roi.height;

     // create or find already associated cm surface 2d
    CmSurface2D *pCmSurface2D;
    mfxHDLPair pair = {dst, nullptr};

    pCmSurface2D = CreateCmSurface2D(pair, width, height, false, m_tableCmRelations2, m_tableCmIndex2);
    CHECK_CM_NULL_PTR(pCmSurface2D, MFX_ERR_DEVICE_FAILED);
    if(isSinglePlainFormat(format))
        status = EnqueueCopyCPUtoGPU(pCmSurface2D, pSrc, roi.width, roi.height, srcPitch, srcUVOffset, format, CM_FASTCOPY_OPTION_BLOCKING, e);
    else
        status = EnqueueCopyNV12CPUtoGPU(pCmSurface2D, pSrc, roi.width, roi.height, srcPitch, srcUVOffset, format, CM_FASTCOPY_OPTION_BLOCKING, e);

    if (status == MFX_ERR_GPU_HANG || status == MFX_ERR_NONE)
    {
        return status;
    }
    else
    {
        cmSts = m_pCmQueue->EnqueueCopyCPUToGPUFullStride(pCmSurface2D, pSrc, srcPitch, srcUVOffset, CM_FASTCOPY_OPTION_BLOCKING, e);

        if (CM_SUCCESS == cmSts)
        {
            status = MFX_ERR_NONE;
        }
        else if(cmSts == CM_EXCEED_MAX_TIMEOUT)
        {
            status = MFX_ERR_GPU_HANG;
        }
        else
        {
            status = MFX_ERR_DEVICE_FAILED;
        }
    }
    return status;
}
mfxStatus CmCopyWrapper::CopySwapSystemToVideoMemory(mfxMemId dst, mfxU32 dstPitch, mfxU8 *pSrc, mfxU32 srcPitch, mfxU32 srcUVOffset, mfxSize roi, mfxU32 format)
{
    (void)dstPitch;

    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "CmCopyWrapper::ARGBSwapSystemToVideo");
    CmEvent* e = CM_NO_EVENT;
    mfxU32 width  = roi.width;
    mfxU32 height = roi.height;

     // create or find already associated cm surface 2d
    CmSurface2D *pCmSurface2D;
    mfxHDLPair pair = {dst, nullptr};

    pCmSurface2D = CreateCmSurface2D(pair, width, height, false, m_tableCmRelations2, m_tableCmIndex2);
    CHECK_CM_NULL_PTR(pCmSurface2D, MFX_ERR_DEVICE_FAILED);

    return EnqueueCopySwapRBCPUtoGPU( pCmSurface2D, pSrc,roi.width,roi.height, srcPitch, srcUVOffset,format, CM_FASTCOPY_OPTION_BLOCKING, e);

}
mfxStatus CmCopyWrapper::CopyShiftSystemToVideoMemory(const mfxMemId dst, mfxU32 dstPitch, mfxU8 *pSrc, mfxU32 srcPitch, mfxU32 srcUVOffset, mfxSize roi, mfxU32 bitshift, mfxU32 format)
{
    (void)dstPitch;

    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "CmCopyWrapper::ShiftSystemToVideo");
    CmEvent* e = CM_NO_EVENT;
    mfxU32 width  = roi.width;
    mfxU32 height = roi.height;

     // create or find already associated cm surface 2d
    CmSurface2D *pCmSurface2D;
    mfxHDLPair pair = {dst, nullptr};

    pCmSurface2D = CreateCmSurface2D(pair, width, height, false, m_tableCmRelations2, m_tableCmIndex2);
    CHECK_CM_NULL_PTR(pCmSurface2D, MFX_ERR_DEVICE_FAILED);
    if (isSinglePlainFormat(format))
        return EnqueueCopyShiftCPUtoGPU(pCmSurface2D, pSrc, roi.width, roi.height, srcPitch, srcUVOffset, format, CM_FASTCOPY_OPTION_BLOCKING, bitshift, e);
    else
        return EnqueueCopyShiftP010CPUtoGPU(pCmSurface2D, pSrc, roi.width, roi.height, srcPitch, srcUVOffset, 0, CM_FASTCOPY_OPTION_BLOCKING, bitshift, e);

}

mfxStatus CmCopyWrapper::CopyShiftVideoToSystemMemory(mfxU8 *pDst, mfxU32 dstPitch, mfxU32 dstUVOffset, mfxMemId src, mfxU32 srcPitch, mfxSize roi, mfxU32 bitshift, mfxU32 format)
{
    (void)srcPitch;

    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "CmCopyWrapper::ShiftVideoToSystem");
    CmEvent* e = CM_NO_EVENT;
    mfxU32 width  = roi.width;
    mfxU32 height = roi.height;

    // create or find already associated cm surface 2d
    CmSurface2D *pCmSurface2D;
    mfxHDLPair pair = {src, nullptr};

    pCmSurface2D = CreateCmSurface2D(pair, width, height, false, m_tableCmRelations2, m_tableCmIndex2);
    CHECK_CM_NULL_PTR(pCmSurface2D, MFX_ERR_DEVICE_FAILED);
    if(isSinglePlainFormat(format))
        return EnqueueCopyShiftGPUtoCPU(pCmSurface2D, pDst, roi.width, roi.height, dstPitch, dstUVOffset, format, CM_FASTCOPY_OPTION_BLOCKING, bitshift, e);
    else
        return EnqueueCopyShiftP010GPUtoCPU(pCmSurface2D, pDst, roi.width, roi.height, dstPitch, dstUVOffset, 0, CM_FASTCOPY_OPTION_BLOCKING, bitshift, e);
}

mfxStatus CmCopyWrapper::CopyVideoToSystemMemoryAPI(mfxU8 *pDst, mfxU32 dstPitch, mfxU32 dstUVOffset, mfxMemId src, mfxU32 srcPitch, mfxSize roi)
{
    (void)srcPitch;

    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "CmCopyWrapper::CopyVideoToSystemMemoryAPI");
    cmStatus cmSts = 0;
    CmEvent* e = NULL;
    mfxStatus status = MFX_ERR_NONE;
    mfxU32 width  = roi.width;
    mfxU32 height = roi.height;
    CmSurface2D *pCmSurface2D;
    mfxHDLPair pair = {src, nullptr};

    pCmSurface2D = CreateCmSurface2D(pair, width, height, false, m_tableCmRelations2, m_tableCmIndex2);
    CHECK_CM_NULL_PTR(pCmSurface2D, MFX_ERR_DEVICE_FAILED);

    cmSts = m_pCmQueue->EnqueueCopyGPUToCPUFullStride(pCmSurface2D, pDst, dstPitch, dstUVOffset, CM_FASTCOPY_OPTION_NONBLOCKING, e);

    if (CM_SUCCESS == cmSts)
    {
        cmSts = e->WaitForTaskFinished(m_timeout);
        if (cmSts == CM_EXCEED_MAX_TIMEOUT)
        {
            status = MFX_ERR_GPU_HANG;
        }
    }
    else
    {
        status = MFX_ERR_DEVICE_FAILED;
    }
    m_pCmQueue->DestroyEvent(e);

    return status;
}

mfxStatus CmCopyWrapper::CopyVideoToSystemMemory(mfxU8 *pDst, mfxU32 dstPitch, mfxU32 dstUVOffset, mfxMemId src, mfxU32 srcPitch, mfxSize roi, mfxU32 format)
{
    (void)srcPitch;

    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "CmCopyWrapper::CopyVideoToSystemMemory");
    cmStatus cmSts = 0;
    CmEvent* e = CM_NO_EVENT;
    mfxStatus status = MFX_ERR_NONE;
    mfxU32 width  = roi.width;
    mfxU32 height = roi.height;

    CmSurface2D *pCmSurface2D;
    mfxHDLPair pair = {src, nullptr};

    pCmSurface2D = CreateCmSurface2D(pair, width, height, false, m_tableCmRelations2, m_tableCmIndex2);
    CHECK_CM_NULL_PTR(pCmSurface2D, MFX_ERR_DEVICE_FAILED);
    if(isSinglePlainFormat(format))
        status = EnqueueCopyGPUtoCPU(pCmSurface2D, pDst, roi.width, roi.height, dstPitch, dstUVOffset, format, CM_FASTCOPY_OPTION_BLOCKING, e);
    else
        status = EnqueueCopyNV12GPUtoCPU(pCmSurface2D, pDst, roi.width, roi.height, dstPitch, dstUVOffset, format, CM_FASTCOPY_OPTION_BLOCKING, e);

    if (status == MFX_ERR_GPU_HANG || status == MFX_ERR_NONE)
    {
        return status;
    }
    else
    {
        cmSts = m_pCmQueue->EnqueueCopyGPUToCPUFullStride(pCmSurface2D, pDst, dstPitch, dstUVOffset, CM_FASTCOPY_OPTION_BLOCKING, e);

        if (CM_SUCCESS == cmSts)
        {
            status = MFX_ERR_NONE;
        }
        else if(cmSts == CM_EXCEED_MAX_TIMEOUT)
        {
            status = MFX_ERR_GPU_HANG;
        }
        else
        {
            status = MFX_ERR_DEVICE_FAILED;
        }
    }
    return status;
}

mfxStatus CmCopyWrapper::CopySwapVideoToSystemMemory(mfxU8 *pDst, mfxU32 dstPitch, mfxU32 dstUVOffset, mfxMemId src, mfxU32 /* srcPitch */, mfxSize roi, mfxU32 format)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "CmCopyWrapper::ARGBSwapVideoToSystem");
    CmEvent* e = CM_NO_EVENT;
    mfxU32 width  = roi.width;
    mfxU32 height = roi.height;

    // create or find already associated cm surface 2d
    CmSurface2D *pCmSurface2D;
    mfxHDLPair pair = {src, nullptr};

    pCmSurface2D = CreateCmSurface2D(pair, width, height, false, m_tableCmRelations2, m_tableCmIndex2);
    CHECK_CM_NULL_PTR(pCmSurface2D, MFX_ERR_DEVICE_FAILED);
    return EnqueueCopySwapRBGPUtoCPU(pCmSurface2D, pDst, roi.width, roi.height, dstPitch, dstUVOffset, format, CM_FASTCOPY_OPTION_BLOCKING, e);

}
mfxStatus CmCopyWrapper::CopyMirrorVideoToSystemMemory(mfxU8 *pDst, mfxU32 dstPitch, mfxU32 dstUVOffset, mfxMemId src, mfxU32 /* srcPitch */, mfxSize roi, mfxU32 format)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "CmCopyWrapper::MirrorVideoToSystem");
    CmEvent* e = CM_NO_EVENT;
    mfxU32 width  = roi.width;
    mfxU32 height = roi.height;

    // create or find already associated cm surface 2d
    CmSurface2D *pCmSurface2D;
    mfxHDLPair pair = {src, nullptr};

    pCmSurface2D = CreateCmSurface2D(pair, width, height, false, m_tableCmRelations2, m_tableCmIndex2);
    CHECK_CM_NULL_PTR(pCmSurface2D, MFX_ERR_DEVICE_FAILED);
    return EnqueueCopyMirrorNV12GPUtoCPU(pCmSurface2D, pDst, roi.width, roi.height, dstPitch, dstUVOffset, format, CM_FASTCOPY_OPTION_BLOCKING, e);

}
mfxStatus CmCopyWrapper::CopyMirrorSystemToVideoMemory(mfxMemId dst, mfxU32 /* dstPitch */, mfxU8 *pSrc, mfxU32 srcPitch, mfxU32 srcUVOffset, mfxSize roi, mfxU32 format)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "CmCopyWrapper::MirrorSystemToVideo");
    CmEvent* e = CM_NO_EVENT;
    mfxU32 width  = roi.width;
    mfxU32 height = roi.height;

    // create or find already associated cm surface 2d
    CmSurface2D *pCmSurface2D;
    mfxHDLPair pair = {dst, nullptr};

    pCmSurface2D = CreateCmSurface2D(pair, width, height, false, m_tableCmRelations2, m_tableCmIndex2);
    CHECK_CM_NULL_PTR(pCmSurface2D, MFX_ERR_DEVICE_FAILED);
    return EnqueueCopyMirrorNV12CPUtoGPU(pCmSurface2D, pSrc, roi.width, roi.height, srcPitch, srcUVOffset, format, CM_FASTCOPY_OPTION_BLOCKING, e);

}
mfxStatus CmCopyWrapper::CopyVideoToVideoMemoryAPI(mfxMemId dst, mfxMemId src, mfxSize roi)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "CmCopyWrapper::CopyVideoToVideoMemoryAPI");
    cmStatus cmSts = 0;

    CmEvent* e = NULL;
    mfxStatus status = MFX_ERR_NONE;

    mfxU32 width  = roi.width;
    mfxU32 height = roi.height;

    // create or find already associated cm surface 2d
    CmSurface2D *pDstCmSurface2D;
    mfxHDLPair pairDst = {dst, nullptr};
    pDstCmSurface2D = CreateCmSurface2D(pairDst, width, height, false, m_tableCmRelations2, m_tableCmIndex2);
    CHECK_CM_NULL_PTR(pDstCmSurface2D, MFX_ERR_DEVICE_FAILED);

    CmSurface2D *pSrcCmSurface2D;
    mfxHDLPair pairSrc = {src, nullptr};
    pSrcCmSurface2D = CreateCmSurface2D(pairSrc, width, height, false, m_tableCmRelations2, m_tableCmIndex2);
    CHECK_CM_NULL_PTR(pSrcCmSurface2D, MFX_ERR_DEVICE_FAILED);

#ifdef CMAPIUPDATE
    cmSts = m_pCmQueue->EnqueueCopyGPUToGPU(pDstCmSurface2D, pSrcCmSurface2D, CM_FASTCOPY_OPTION_NONBLOCKING, e);
#else
    cmSts = m_pCmQueue->EnqueueCopyGPUToGPU(pDstCmSurface2D, pSrcCmSurface2D, e);
#endif

    if (CM_SUCCESS == cmSts)
    {
        cmSts = e->WaitForTaskFinished(m_timeout);
        if (cmSts == CM_EXCEED_MAX_TIMEOUT)
        {
            status = MFX_ERR_GPU_HANG;
        }
    }
    else
    {
        status = MFX_ERR_DEVICE_FAILED;
    }
    m_pCmQueue->DestroyEvent(e);
    return status;
}
mfxStatus CmCopyWrapper::CopySwapVideoToVideoMemory(mfxMemId dst, mfxMemId src, mfxSize roi, mfxU32 format)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "CmCopyWrapper::ARGBSwapVideoToVideo");
    CmEvent* e = NULL;

    mfxU32 width  = roi.width;
    mfxU32 height = roi.height;

    // create or find already associated cm surface 2d
    CmSurface2D *pDstCmSurface2D;
    mfxHDLPair pairDst = {dst, nullptr};
    pDstCmSurface2D = CreateCmSurface2D(pairDst, width, height, false, m_tableCmRelations2, m_tableCmIndex2);
    CHECK_CM_NULL_PTR(pDstCmSurface2D, MFX_ERR_DEVICE_FAILED);

    CmSurface2D *pSrcCmSurface2D;
    mfxHDLPair pairSrc = {src, nullptr};
    pSrcCmSurface2D = CreateCmSurface2D(pairSrc, width, height, false, m_tableCmRelations2, m_tableCmIndex2);
    CHECK_CM_NULL_PTR(pSrcCmSurface2D, MFX_ERR_DEVICE_FAILED);

    return EnqueueCopySwapRBGPUtoGPU(pSrcCmSurface2D, pDstCmSurface2D, width, height, format, CM_FASTCOPY_OPTION_BLOCKING, e);
}
mfxStatus CmCopyWrapper::CopyMirrorVideoToVideoMemory(mfxMemId dst, mfxMemId src, mfxSize roi, mfxU32 format)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "CmCopyWrapper::MirrorVideoToVideo");
    CmEvent* e = NULL;

    mfxU32 width  = roi.width;
    mfxU32 height = roi.height;

    // create or find already associated cm surface 2d
    CmSurface2D *pDstCmSurface2D;
    mfxHDLPair pair = {dst, nullptr};
    pDstCmSurface2D = CreateCmSurface2D(pair, width, height, false, m_tableCmRelations2, m_tableCmIndex2);
    CHECK_CM_NULL_PTR(pDstCmSurface2D, MFX_ERR_DEVICE_FAILED);

    CmSurface2D *pSrcCmSurface2D;
    pair = {src, nullptr};
    pSrcCmSurface2D = CreateCmSurface2D(pair, width, height, false, m_tableCmRelations2, m_tableCmIndex2);
    CHECK_CM_NULL_PTR(pSrcCmSurface2D, MFX_ERR_DEVICE_FAILED);

    return EnqueueCopyMirrorGPUtoGPU(pSrcCmSurface2D, pDstCmSurface2D, width, height, format, CM_FASTCOPY_OPTION_BLOCKING, e);
}

#define CM_RGB_MAX_GPUCOPY_SURFACE_HEIGHT        4088

#define CM_RGB_SUPPORTED_COPY_SIZE(ROI) (ROI.width <= CM_RGB_MAX_GPUCOPY_SURFACE_HEIGHT && ROI.height <= CM_RGB_MAX_GPUCOPY_SURFACE_HEIGHT )

bool CmCopyWrapper::CheckSurfaceContinuouslyAllocated(const mfxFrameSurface1 &surf)
{
    mfxU32 stride_in_bytes = surf.Data.PitchLow + ((mfxU32)surf.Data.PitchHigh << 16);

    switch (surf.Info.FourCC)
    {
    // Packed formats like YUY2, UYVY, AYUV, Y416, Y210, Y410, A2RGB10, RGB565, RGB3 
    // does not need to be handled here - they got only one plane with all the luma and chroma data interleaved

    // Handling semi-planar formats
    case MFX_FOURCC_P010:
    case MFX_FOURCC_P016:
    case MFX_FOURCC_P210:
    case MFX_FOURCC_NV12:
    case MFX_FOURCC_NV16:
        {
            size_t luma_size_in_bytes = stride_in_bytes * mfx::align2_value(surf.Info.Height, 32);
            return surf.Data.Y + luma_size_in_bytes == surf.Data.UV;
            break;
        }

    // Handling planar formats
    case MFX_FOURCC_YV12:
        {
            size_t luma_size_in_bytes = stride_in_bytes * mfx::align2_value(surf.Info.Height, 32);
            size_t chroma_size_in_bytes = luma_size_in_bytes / 4; //stride of the V plane is half the stride of the Y plane; and the V plane contains half as many lines as the Y plane
            return surf.Data.Y + luma_size_in_bytes == surf.Data.V && surf.Data.V + chroma_size_in_bytes == surf.Data.U;
            break;
        }

    // Handling RGB-like formats
    case MFX_FOURCC_RGBP:
        {
            size_t channel_size_in_bytes = stride_in_bytes * mfx::align2_value(surf.Info.Height, 32);
            return surf.Data.B + channel_size_in_bytes == surf.Data.G && surf.Data.G + channel_size_in_bytes == surf.Data.R;
            break;
        }
    }
    return true;
}

bool CmCopyWrapper::CanUseCmCopy(mfxFrameSurface1 *pDst, mfxFrameSurface1 *pSrc)
{
    mfxSize roi = {std::min(pSrc->Info.Width, pDst->Info.Width), std::min(pSrc->Info.Height, pDst->Info.Height)};

    mfxU8* srcPtr = GetFramePointer(pSrc->Info.FourCC, pSrc->Data);
    mfxU8* dstPtr = GetFramePointer(pDst->Info.FourCC, pDst->Data);

    if (NULL != pSrc->Data.MemId && NULL != pDst->Data.MemId)
    {
        if (pDst->Info.FourCC != MFX_FOURCC_YV12 && CM_SUPPORTED_COPY_SIZE(roi))
        {
            return true;
        }
    }
    else if (NULL != pSrc->Data.MemId && NULL != dstPtr)
    {
        if (!CM_ALIGNED(pDst->Data.Pitch))
            return false;

        mfxI64 verticalPitch = (mfxI64)(pDst->Data.UV - pDst->Data.Y);
        verticalPitch = (verticalPitch % pDst->Data.Pitch)? 0 : verticalPitch / pDst->Data.Pitch;
        if (isNV12LikeFormat(pDst->Info.FourCC) && isNV12LikeFormat(pSrc->Info.FourCC) && CM_ALIGNED(pDst->Data.Y) && CM_ALIGNED(pDst->Data.UV) && CM_SUPPORTED_COPY_SIZE(roi) && verticalPitch >= pDst->Info.Height && verticalPitch <= 16384)
        {
            return CheckSurfaceContinuouslyAllocated(*pDst);
        }
        else if(isSinglePlainFormat(pDst->Info.FourCC) && isSinglePlainFormat(pSrc->Info.FourCC) && pSrc->Info.Shift == pDst->Info.Shift && CM_ALIGNED(dstPtr) && CM_SUPPORTED_COPY_SIZE(roi))
        {
            return true;
        }
        else
            return false;
    }
    else if (NULL != srcPtr && NULL != dstPtr)
    {
        return false;
    }
    else if (NULL != srcPtr && NULL != pDst->Data.MemId)
    {
        if (!CM_ALIGNED(pSrc->Data.Pitch))
            return false;

        mfxI64 verticalPitch = (mfxI64)(pSrc->Data.UV - pSrc->Data.Y);
        verticalPitch = (verticalPitch % pSrc->Data.Pitch)? 0 : verticalPitch / pSrc->Data.Pitch;

        if (isNV12LikeFormat(pDst->Info.FourCC) && isNV12LikeFormat(pSrc->Info.FourCC) && CM_ALIGNED(pSrc->Data.Y) && CM_ALIGNED(pSrc->Data.UV) && CM_SUPPORTED_COPY_SIZE(roi) && verticalPitch >= pSrc->Info.Height && verticalPitch <= 16384)
        {
            return true;
        }
        else if(isSinglePlainFormat(pDst->Info.FourCC) && isSinglePlainFormat(pSrc->Info.FourCC) && pSrc->Info.Shift == pDst->Info.Shift && CM_ALIGNED(srcPtr) && CM_SUPPORTED_COPY_SIZE(roi))
        {
            return true;
        }
    }

    return false;
}

mfxStatus CmCopyWrapper::CopyVideoToVideo(mfxFrameSurface1 *pDst, mfxFrameSurface1 *pSrc)
{
    mfxSize roi = {std::min(pSrc->Info.Width, pDst->Info.Width), std::min(pSrc->Info.Height, pDst->Info.Height)};

    // check that region of interest is valid
    if (0 == roi.width || 0 == roi.height || m_HWType == MFX_HW_UNKNOWN)
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    if (NULL != pSrc->Data.MemId && NULL != pDst->Data.MemId)
    {
        if(isNeedSwapping(pSrc,pDst))
            return CopySwapVideoToVideoMemory(pDst->Data.MemId,pSrc->Data.MemId, roi, pDst->Info.FourCC);
        else
            return CopyVideoToVideoMemoryAPI(pDst->Data.MemId,pSrc->Data.MemId, roi);
    }

    return MFX_ERR_UNDEFINED_BEHAVIOR;
}

mfxStatus CmCopyWrapper::CopyVideoToSys(mfxFrameSurface1 *pDst, mfxFrameSurface1 *pSrc)
{
    mfxSize roi = {std::min(pSrc->Info.Width, pDst->Info.Width), std::min(pSrc->Info.Height, pDst->Info.Height)};

    // check that region of interest is valid
    if (0 == roi.width || 0 == roi.height || m_HWType == MFX_HW_UNKNOWN)
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    mfxU32 dstPitch = pDst->Data.PitchLow + ((mfxU32)pDst->Data.PitchHigh << 16);

    mfxU8* dstPtr = GetFramePointer(pDst->Info.FourCC, pDst->Data);

    if (NULL != pSrc->Data.MemId && NULL != dstPtr)
    {
        if (!CM_ALIGNED(pDst->Data.Pitch) )
            return MFX_ERR_UNDEFINED_BEHAVIOR;

        mfxI64 verticalPitch = (mfxI64)(pDst->Data.UV - pDst->Data.Y);
        verticalPitch = (verticalPitch % dstPitch)? 0 : verticalPitch / dstPitch;
#ifdef MFX_ENABLE_RGBP
        if (pDst->Info.FourCC == MFX_FOURCC_RGBP)
        {
            verticalPitch = (mfxI64)(pDst->Data.G - pDst->Data.B);
            verticalPitch = (verticalPitch % dstPitch)? 0 : verticalPitch / dstPitch;
        }
#endif

        if (isNeedShift(pSrc, pDst) && CM_ALIGNED(dstPtr) && CM_SUPPORTED_COPY_SIZE(roi) && verticalPitch >= pDst->Info.Height && verticalPitch <= 16384)
        {
            return CopyShiftVideoToSystemMemory(dstPtr, pDst->Data.Pitch,(mfxU32)verticalPitch, pSrc->Data.MemId, 0, roi, 16-pDst->Info.BitDepthLuma, pDst->Info.FourCC);
        }
        else if (isNV12LikeFormat(pDst->Info.FourCC) && CM_ALIGNED(dstPtr) && CM_SUPPORTED_COPY_SIZE(roi) && verticalPitch >= pDst->Info.Height && verticalPitch <= 16384)
        {
            if (m_HWType >= MFX_HW_SCL)
                return CopyVideoToSystemMemory(dstPtr, pDst->Data.Pitch, (mfxU32)verticalPitch, pSrc->Data.MemId, pDst->Info.Height, roi, pDst->Info.FourCC);
            else
                return CopyVideoToSystemMemoryAPI(dstPtr, pDst->Data.Pitch, (mfxU32)verticalPitch, pSrc->Data.MemId, 0, roi);
        }
        else if (isNeedSwapping(pSrc, pDst) && CM_ALIGNED(dstPtr) && CM_SUPPORTED_COPY_SIZE(roi))
        {
            return CopySwapVideoToSystemMemory(dstPtr, pDst->Data.Pitch, (mfxU32)pSrc->Info.Height,pSrc->Data.MemId, 0, roi, pDst->Info.FourCC);
        }
        else if (isSinglePlainFormat(pDst->Info.FourCC) && isSinglePlainFormat(pSrc->Info.FourCC) && pSrc->Info.FourCC == pDst->Info.FourCC && pSrc->Info.Shift == pDst->Info.Shift && CM_ALIGNED(dstPtr) && CM_SUPPORTED_COPY_SIZE(roi))
        {
            if (m_HWType >= MFX_HW_SCL)
                return CopyVideoToSystemMemory(dstPtr, pDst->Data.Pitch, (mfxU32)verticalPitch, pSrc->Data.MemId, pDst->Info.Height, roi, pDst->Info.FourCC);
            else
                return CopyVideoToSystemMemoryAPI(dstPtr, pDst->Data.Pitch, (mfxU32)pDst->Info.Height, pSrc->Data.MemId, 0, roi);
        }
    }

    return MFX_ERR_UNDEFINED_BEHAVIOR;
}

mfxStatus CmCopyWrapper::CopySysToVideo(mfxFrameSurface1 *pDst, mfxFrameSurface1 *pSrc)
{
    mfxSize roi = {std::min(pSrc->Info.Width, pDst->Info.Width), std::min(pSrc->Info.Height, pDst->Info.Height)};

    mfxU8* srcPtr = GetFramePointer(pSrc->Info.FourCC, pSrc->Data);

    // check that region of interest is valid
    if (0 == roi.width || 0 == roi.height || m_HWType == MFX_HW_UNKNOWN)
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    if (NULL != srcPtr && NULL != pDst->Data.MemId)
    {
        if (!CM_ALIGNED(pSrc->Data.Pitch) )
            return MFX_ERR_UNDEFINED_BEHAVIOR;

        // source are placed in system memory, destination is in video memory
        // use common way to copy frames from system to video, most faster
        mfxI64 verticalPitch = (mfxI64)(pSrc->Data.UV - pSrc->Data.Y);
        verticalPitch = (verticalPitch % pSrc->Data.Pitch)? 0 : verticalPitch / pSrc->Data.Pitch;

        if (isNeedShift(pSrc, pDst) && CM_ALIGNED(srcPtr) && CM_SUPPORTED_COPY_SIZE(roi) && verticalPitch >= pSrc->Info.Height && verticalPitch <= 16384)
        {
            return CopyShiftSystemToVideoMemory(pDst->Data.MemId, 0, pSrc->Data.Y, pSrc->Data.Pitch,(mfxU32)verticalPitch, roi, 16 - pSrc->Info.BitDepthLuma, pDst->Info.FourCC);
        }
        else if (isNV12LikeFormat(pSrc->Info.FourCC) && CM_ALIGNED(srcPtr) && CM_SUPPORTED_COPY_SIZE(roi) && verticalPitch >= pSrc->Info.Height && verticalPitch <= 16384)
        {
            if (m_HWType >= MFX_HW_SCL)
                return CopySystemToVideoMemory(pDst->Data.MemId, 0, pSrc->Data.Y, pSrc->Data.Pitch, (mfxU32)verticalPitch, roi, pDst->Info.FourCC);
            else
                return CopySystemToVideoMemoryAPI(pDst->Data.MemId, 0, pSrc->Data.Y, pSrc->Data.Pitch, (mfxU32)verticalPitch, roi);
        }
        else if (isNeedSwapping(pSrc, pDst) && CM_ALIGNED(srcPtr) && CM_SUPPORTED_COPY_SIZE(roi))
        {
            return CopySwapSystemToVideoMemory(pDst->Data.MemId, 0, srcPtr, pSrc->Data.Pitch, (mfxU32)pSrc->Info.Height, roi, pDst->Info.FourCC);
        }
        else if (isSinglePlainFormat(pDst->Info.FourCC) && isSinglePlainFormat(pSrc->Info.FourCC) && pSrc->Info.FourCC == pDst->Info.FourCC && pSrc->Info.Shift == pDst->Info.Shift && CM_ALIGNED(srcPtr) && CM_SUPPORTED_COPY_SIZE(roi))
        {
            if (m_HWType >= MFX_HW_SCL)
                return CopySystemToVideoMemory(pDst->Data.MemId, 0, srcPtr, pSrc->Data.Pitch, (mfxU32)pSrc->Info.Height, roi, pDst->Info.FourCC);
            else
                return CopySystemToVideoMemoryAPI(pDst->Data.MemId, 0, srcPtr, pSrc->Data.Pitch, (mfxU32)pDst->Info.Height, roi);
        }
    }

    return MFX_ERR_UNDEFINED_BEHAVIOR;
}
