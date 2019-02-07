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

#ifndef __MFX_MJPEG_ENCODE_HW_UTILS_H__
#define __MFX_MJPEG_ENCODE_HW_UTILS_H__

#include "mfx_common.h"

#if defined (MFX_ENABLE_MJPEG_VIDEO_ENCODE)

#if defined (MFX_VA_LINUX)
#include <va/va.h>
#include <va/va_enc_jpeg.h>

#endif

#include "mfxstructures.h"

#include <vector>
#include <memory>
#include <assert.h>

#include "vm_interlocked.h"
#include "umc_mutex.h"

#include "mfxstructures.h"
#include "mfxjpeg.h"

namespace MfxHwMJpegEncode
{
    #define  JPEG_VIDEO_SURFACE_NUM    4
    #define  JPEG_DDITASK_MAX_NUM      32

    enum
    {
        MFX_MEMTYPE_VIDEO_INT = MFX_MEMTYPE_FROM_ENCODE | MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET | MFX_MEMTYPE_INTERNAL_FRAME,
        MFX_MEMTYPE_VIDEO_EXT = MFX_MEMTYPE_FROM_ENCODE | MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET | MFX_MEMTYPE_EXTERNAL_FRAME
    };

    typedef struct {
        mfxU32    Baseline;
        mfxU32    Sequential;
        mfxU32    Huffman;

        mfxU32    NonInterleaved;
        mfxU32    Interleaved;

        mfxU32    MaxPicWidth;
        mfxU32    MaxPicHeight;

        mfxU32    SampleBitDepth;
        mfxU32    MaxNumComponent;
        mfxU32    MaxNumScan;
        mfxU32    MaxNumHuffTable;
        mfxU32    MaxNumQuantTable;
    } JpegEncCaps;

    typedef struct {
        mfxU16 header;
        mfxU8  lenH;
        mfxU8  lenL;
        mfxU8  s[5];
        mfxU8  versionH;
        mfxU8  versionL;
        mfxU8  units;
        mfxU16 xDensity;
        mfxU16 yDensity;
        mfxU8  xThumbnails;
        mfxU8  yThumbnails;
    } JpegApp0Data;

    typedef struct {
        mfxU16 header;
        mfxU8  lenH;
        mfxU8  lenL;
        mfxU8  s[5];
        mfxU8  versionH;
        mfxU8  versionL;
        mfxU8  flags0H;
        mfxU8  flags0L;
        mfxU8  flags1H;
        mfxU8  flags1L;
        mfxU8  transform;
    } JpegApp14Data;

    typedef struct {
        mfxU8 * data;
        mfxU32  length;
        mfxU32  maxLength;
    } JpegPayload;

    mfxStatus QueryHwCaps(
        VideoCORE * core,
        JpegEncCaps & hwCaps);

    bool IsJpegParamExtBufferIdSupported(
        mfxU32 id);

    mfxStatus CheckExtBufferId(
        mfxVideoParam const & par);

    mfxStatus CheckJpegParam(
        VideoCORE         * core,
        mfxVideoParam     & par,
        JpegEncCaps const & hwCaps);

    mfxStatus FastCopyFrameBufferSys2Vid(
        VideoCORE    * core,
        mfxMemId       vidMemId,
        mfxFrameData & sysSurf,
        mfxFrameInfo & frmInfo
        );

    struct ExecuteBuffers
    {
        ExecuteBuffers()
        {
            memset(&m_pps, 0, sizeof(m_pps));
            memset(&m_payload_base, 0, sizeof(m_payload_base));
            memset(&m_app0_data, 0, sizeof(m_app0_data));
            memset(&m_app14_data, 0, sizeof(m_app14_data));
        }

        mfxStatus Init(mfxVideoParam const *par, mfxEncodeCtrl const * ctrl, JpegEncCaps const * hwCaps);
        void      Close();

#if defined (MFX_VA_LINUX)
        VAEncPictureParameterBufferJPEG               m_pps;
        std::vector<VAEncSliceParameterBufferJPEG>    m_scan_list;
        std::vector<VAQMatrixBufferJPEG>              m_dqt_list;
        std::vector<VAHuffmanTableBufferJPEGBaseline> m_dht_list;

#endif
        std::vector<JpegPayload>                      m_payload_list;
        JpegPayload                                   m_payload_base;
        JpegApp0Data                                  m_app0_data;
        JpegApp14Data                                 m_app14_data;
    };

    typedef struct {
        mfxFrameSurface1 * surface;              // input raw surface
        mfxBitstream     * bs;                   // output bitstream
        mfxU32             m_idx;                // index of raw surface
        mfxU32             m_idxBS;              // index of bitstream surface (always equal as m_idx for now)
        mfxU32             lInUse;               // 0: free, 1: used.
        mfxU32             m_statusReportNumber;
        mfxU32             m_bsDataLength;       // output bitstream length
        bool               m_cleanDdiData;
        ExecuteBuffers   * m_pDdiData;
    } DdiTask;

    class TaskManager
    {
    public:
        TaskManager();
        ~TaskManager();

        mfxStatus Init(mfxU32 maxTaskNum);
        mfxStatus Reset();
        mfxStatus Close();

        mfxStatus AssignTask(DdiTask *& newTask);
        mfxStatus RemoveTask(DdiTask & task);
    private:
        DdiTask * m_pTaskList;
        mfxU32    m_TaskNum;

        UMC::Mutex    m_mutex;
    };

}; // namespace MfxHwMJpegEncode

#endif // #if defined (MFX_ENABLE_MJPEG_VIDEO_ENCODE) && defined (MFX_VA)
#endif // __MFX_MJPEG_ENCODE_HW_UTILS_H__
