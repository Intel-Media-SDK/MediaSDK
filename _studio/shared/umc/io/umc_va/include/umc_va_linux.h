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

#ifndef __UMC_VA_LINUX_H__
#define __UMC_VA_LINUX_H__

#include "umc_va_base.h"


#include <mutex>

namespace UMC
{

#define UMC_VA_LINUX_INDEX_UNDEF -1

/* VACompBuffer --------------------------------------------------------------*/

class VACompBuffer : public UMCVACompBuffer
{
public:
    // constructor
    VACompBuffer(void);
    // destructor
    virtual ~VACompBuffer(void);

    // UMCVACompBuffer methods
    virtual void SetNumOfItem(int32_t num) { m_NumOfItem = num; };

    // VACompBuffer methods
    virtual Status SetBufferInfo   (int32_t _type, int32_t _id, int32_t _index = -1);
    virtual Status SetDestroyStatus(bool _destroy);

    virtual int32_t GetIndex(void)    { return m_index; }
    virtual int32_t GetID(void)       { return m_id; }
    virtual int32_t GetNumOfItem(void){ return m_NumOfItem; }
    virtual bool   NeedDestroy(void) { return m_bDestroy; }

protected:
    int32_t m_NumOfItem; //number of items in buffer
    int32_t m_index;
    int32_t m_id;
    bool   m_bDestroy;
};

/* LinuxVideoAcceleratorParams -----------------------------------------------*/

class LinuxVideoAcceleratorParams : public VideoAcceleratorParams
{
    DYNAMIC_CAST_DECL(LinuxVideoAcceleratorParams, VideoAcceleratorParams);

public:

    LinuxVideoAcceleratorParams(void)
    {
        m_Display            = NULL;
        m_bComputeVAFncsInfo = false;
        m_pConfigId          = NULL;
        m_pContext           = NULL;
        m_pKeepVAState       = NULL;
        m_CreateFlags        = VA_PROGRESSIVE;
    }

    VADisplay     m_Display;
    bool          m_bComputeVAFncsInfo;
    VAConfigID*   m_pConfigId;
    VAContextID*  m_pContext;
    bool*         m_pKeepVAState;
    int           m_CreateFlags;
};

/* LinuxVideoAccelerator -----------------------------------------------------*/

enum lvaFrameState
{
    lvaBeforeBegin = 0,
    lvaBeforeEnd   = 1,
    lvaNeedUnmap   = 2
};

class LinuxVideoAccelerator : public VideoAccelerator
{
    DYNAMIC_CAST_DECL(LinuxVideoAccelerator, VideoAccelerator);
public:
    // constructor
    LinuxVideoAccelerator (void);
    // destructor
    virtual ~LinuxVideoAccelerator(void);

    // VideoAccelerator methods
    virtual Status Init         (VideoAcceleratorParams* pInfo);
    virtual Status Close        (void);
    virtual Status BeginFrame   (int32_t FrameBufIndex);
    // gets buffer from cache if it exists or HW otherwise, buffers will be released in EndFrame
    virtual void* GetCompBuffer(int32_t buffer_type, UMCVACompBuffer **buf, int32_t size, int32_t index);
    virtual Status Execute      (void);
    virtual Status EndFrame     (void*);
    virtual int32_t GetSurfaceID (int32_t idx);

    // NOT implemented functions:
    virtual Status ReleaseBuffer(int32_t /*type*/)
    { return UMC_OK; };

    virtual Status ExecuteExtensionBuffer(void* /*x*/) { return UMC_ERR_UNSUPPORTED;}
    virtual Status ExecuteStatusReportBuffer(void* /*x*/, int32_t /*y*/)  { return UMC_ERR_UNSUPPORTED;}
    virtual Status SyncTask(int32_t index, void * error = NULL);
    virtual Status QueryTaskStatus(int32_t index, void * status, void * error);
    virtual bool IsIntelCustomGUID() const { return false;}
    virtual GUID GetDecoderGuid(){return m_guidDecoder;};
    virtual void GetVideoDecoder(void** /*handle*/) {};

protected:
    // VideoAcceleratorExt methods
    virtual Status AllocCompBuffers(void);
    virtual VACompBuffer* GetCompBufferHW(int32_t type, int32_t size, int32_t index = -1);

    // LinuxVideoAccelerator methods
    uint16_t GetDecodingError();

    void SetTraceStrings(uint32_t umc_codec);

protected:
    VADisplay     m_dpy;
    VAConfigID*   m_pConfigId;
    VAContextID*  m_pContext;
    bool*         m_pKeepVAState;
    lvaFrameState m_FrameState;

    int32_t   m_NumOfFrameBuffers;
    uint32_t   m_uiCompBuffersNum;
    uint32_t   m_uiCompBuffersUsed;
    std::mutex m_SyncMutex;
    VACompBuffer** m_pCompBuffers;

    const char * m_sDecodeTraceStart;
    const char * m_sDecodeTraceEnd;

    GUID m_guidDecoder;
};

}; // namespace UMC

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

    extern UMC::Status va_to_umc_res(VAStatus va_res);

#ifdef __cplusplus
}
#endif // __cplusplus


#endif // #ifndef __UMC_VA_LINUX_H__
