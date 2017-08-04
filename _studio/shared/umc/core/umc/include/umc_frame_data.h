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

#ifndef __UMC_FRAME_DATA_H__
#define __UMC_FRAME_DATA_H__

#include "umc_defs.h"
#include "umc_structures.h"
#include "umc_dynamic_cast.h"

#include "umc_frame_allocator.h"

#include <list>

/*
    USAGE MODEL:
    I. Initialization of VideoData parameters. It has to be done after construction
      or after Close.
      A. Simplest case. No additional planes, planes have equal bitdepth.
        1. Set required alignment for data with SetAlignment. Default is the sample size.
        2. Init(w,h,ColorFormat[,bitdepth]). Default bitdepth is derived from format.
      B. Advanced case
        1. Init(w,h,nplanes[,bitdepth]) and SetAlignment if required before or after.
        2. Modify bitdepth or sample size for planes where necessary
        3. Call SetColorFormat. It is the only moment to call this method.
      This stage fill all internal information about planes, including pitches.
      After this no more changes to parameters shall be introduced.
      Function GetMappingSize can be called now to determine required quantity of memory
      for all planes taking into account current alignment. All other Get methods except
      of GetPlanePointer are possible to use.

    II. Link to memory. These operations assign all plane pointers. After that
      MediaData::GetDataPointer will return aligned beginning of the first plane,
      MediaData::GetDataSize will return value equal to MappingSize,
      MediaData::GetBufferPointer can differ from DataPointer due to aligning
      Two ways:
      A. Allocation using Alloc. BufferSize will be MappingSize + alignment.
      B. Call SetBufferPointer(bufsize). After that BufferSize will be bufsize.
      Method ReleaseImage cancels this operations (unlink from memory).
      These methods only work with continuously located planes.

    III. Operations which don't change plane parameters, like SetFrameType, can be used at
      any moment. Operations SetPlanePointer and SetPlanePitch allow working with separate
      planes or without specified ColorFormat but have to be used with care. Functions like
      GetMappingSize and GetPlaneInfo can provide incorrect results.

    Note:
    parent class methods GetDataPointer, MoveDataPointer operator= shouldn't be used.
*/

namespace UMC
{

class VideoDataInfo
{
    DYNAMIC_CAST_DECL_BASE(VideoDataInfo)

public:

    enum PictureStructure
    {
        PS_TOP_FIELD                = 1,
        PS_BOTTOM_FIELD             = 2,
        PS_FRAME                    = PS_TOP_FIELD | PS_BOTTOM_FIELD,
        PS_TOP_FIELD_FIRST          = PS_FRAME | 4,
        PS_BOTTOM_FIELD_FIRST       = PS_FRAME | 8
    }; // DEBUG : to use types at umc_structure.h

    struct PlaneInfo
    {
        mfxSize m_ippSize;        // width and height of the plane
        int32_t   m_iSampleSize;    // sample size (in bytes)
        int32_t   m_iSamples;       // number of samples per plane element
        int32_t   m_iBitDepth;      // number of significant bits per sample (should be <= 8*m_iSampleSize)
        int32_t   m_iWidthScale;    // Horizontal downsampling factor
        int32_t   m_iHeightScale;   // Vertical downsampling factor
    };

    VideoDataInfo(void);
    virtual ~VideoDataInfo(void);

    // Initialize. Only remembers image characteristics for future.
    virtual Status Init(int32_t iWidth,
                int32_t iHeight,
                ColorFormat cFormat,
                int32_t iBitDepth = 8);

    void Close();

    inline int32_t GetPlaneBitDepth(uint32_t iPlaneNumber) const;

    Status SetPlaneSampleSize(int32_t iSampleSize, uint32_t iPlaneNumber);
    inline uint32_t GetPlaneSampleSize(uint32_t iPlaneNumber) const;

    inline ColorFormat GetColorFormat() const;

    inline Status SetAspectRatio(int32_t iHorzAspect, int32_t iVertAspect);
    inline Status GetAspectRatio(int32_t *piHorzAspect, int32_t *piVertAspect) const;

    inline Status SetPictureStructure(PictureStructure picStructure);
    inline PictureStructure GetPictureStructure() const;

    inline uint32_t GetNumPlanes() const;
    inline uint32_t GetWidth() const;
    inline uint32_t GetHeight() const;

    size_t GetSize() const;

    const PlaneInfo* GetPlaneInfo(uint32_t iPlaneNumber) const;

    void SetPadding();
    void GetPadding() const;

protected:

    enum
    {
        NUM_PLANES = 4
    };

    PlaneInfo        m_pPlaneData[NUM_PLANES]; // pointer to allocated planes info
    uint32_t           m_iPlanes;       // number of initialized planes

    mfxSize         m_ippSize;       // dimension of the image

    ColorFormat      m_ColorFormat;   // color format of image
    PictureStructure m_picStructure;  // variants: progressive frame, top first, bottom first, only top, only bottom

    int32_t           m_iHorzAspect;   // aspect ratio: pixel width/height proportion
    int32_t           m_iVertAspect;   // default 1,1 - square pixels

    // Set color format and planes' information
    Status SetColorFormat(ColorFormat cFormat);

    Status Init(int32_t iWidth,
                       int32_t iHeight,
                       int32_t iPlanes,
                       int32_t iBitDepth);

};

inline int32_t VideoDataInfo::GetPlaneBitDepth(uint32_t iPlaneNumber) const
{
    // check error(s)
    if (NUM_PLANES <= iPlaneNumber)
        return 0;

  return m_pPlaneData[iPlaneNumber].m_iBitDepth;

}

inline uint32_t VideoDataInfo::GetPlaneSampleSize(uint32_t iPlaneNumber) const
{
    // check error(s)
    if (NUM_PLANES <= iPlaneNumber)
        return 0;

  return m_pPlaneData[iPlaneNumber].m_iSampleSize;

}

inline ColorFormat VideoDataInfo::GetColorFormat(void) const
{
    return m_ColorFormat;
}

inline Status VideoDataInfo::SetAspectRatio(int32_t iHorzAspect, int32_t iVertAspect)
{
    if ((1 > iHorzAspect) || (1 > iVertAspect))
        return UMC_ERR_INVALID_STREAM;

    m_iHorzAspect = iHorzAspect;
    m_iVertAspect = iVertAspect;

    return UMC_OK;
}

inline Status VideoDataInfo::GetAspectRatio(int32_t *piHorzAspect, int32_t *piVertAspect) const
{
    if ((NULL == piHorzAspect) ||
        (NULL == piVertAspect))
        return UMC_ERR_NULL_PTR;

    *piHorzAspect = m_iHorzAspect;
    *piVertAspect = m_iVertAspect;

    return UMC_OK;
}

inline Status VideoDataInfo::SetPictureStructure(PictureStructure picStructure)
{
    if ((PS_TOP_FIELD != picStructure) &&
        (PS_BOTTOM_FIELD != picStructure) &&
        (PS_FRAME != picStructure) &&
        (PS_TOP_FIELD_FIRST != picStructure) &&
        (PS_BOTTOM_FIELD_FIRST != picStructure))
        return UMC_ERR_INVALID_STREAM;

    m_picStructure = picStructure;

    return UMC_OK;
}

inline VideoDataInfo::PictureStructure VideoDataInfo::GetPictureStructure() const
{
    return m_picStructure;
}

inline uint32_t VideoDataInfo::GetNumPlanes() const
{
    return m_iPlanes;
}

inline uint32_t VideoDataInfo::GetWidth() const
{
    return m_ippSize.width;
}

inline uint32_t VideoDataInfo::GetHeight() const
{
    return m_ippSize.height;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Time
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FrameTime
{
public:
    FrameTime();
    virtual ~FrameTime() {}

    virtual void Reset();

    // return time stamp of media data
    virtual double GetTime(void) const         { return m_pts_start; }

    // return time stamp of media data, start and end
    virtual Status GetTime(double& start, double& end) const;

    //  Set time stamp of media data block;
    virtual Status SetTime(double start, double end = 0);

private:
    double m_pts_start;        // (double) start media PTS
    double m_pts_end;          // (double) finish media PTS
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FrameData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FrameData : public FrameTime
{
public:

    struct PlaneMemoryInfo
    {
        uint8_t* m_planePtr;
        size_t m_pitch;
    };

    struct FrameAuxInfo
    {
        void*  ptr;
        size_t size;
        int    type;

        bool operator==(FrameAuxInfo const& i) const
        { return type == i.type; }
    };

    FrameData();
    FrameData(const FrameData & fd);

    virtual ~FrameData();

    const VideoDataInfo * GetInfo() const;

    const PlaneMemoryInfo * GetPlaneMemoryInfo(uint32_t plane) const;

    void Init(const VideoDataInfo * info, FrameMemID memID = FRAME_MID_INVALID, FrameAllocator * frameAlloc = 0);

    void Close();

    FrameMemID GetFrameMID() const;

    FrameMemID Release();

    void SetPlanePointer(uint8_t* planePtr, uint32_t plane, size_t pitch);

    void SetAuxInfo(void* ptr, size_t size, int type);
    void ClearAuxInfo(int type);

    FrameAuxInfo* GetAuxInfo(int type)
    {
        return
            const_cast<FrameAuxInfo*>(const_cast<FrameData const*>(this)->GetAuxInfo(type)) ;
    }

    FrameAuxInfo const* GetAuxInfo(int type) const;

    FrameData& operator=(const FrameData& );

    bool            m_locked;

protected:

    enum
    {
        NUM_PLANES = 4
    };

    VideoDataInfo            m_Info;
    FrameMemID               m_FrameMID;
    FrameAllocator*          m_FrameAlloc;

    PlaneMemoryInfo          m_PlaneInfo[NUM_PLANES];

    std::list<FrameAuxInfo>  m_AuxInfo;
};

} // namespace UMC

#endif // __UMC_FRAME_DATA_H__
