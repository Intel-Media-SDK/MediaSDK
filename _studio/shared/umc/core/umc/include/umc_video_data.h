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

#ifndef __UMC_VIDEO_DATA_H__
#define __UMC_VIDEO_DATA_H__

#include "umc_defs.h"
#include "umc_structures.h"
#include "umc_media_data.h"

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

enum PictureStructure
{
    PS_TOP_FIELD                = 1,
    PS_BOTTOM_FIELD             = 2,
    PS_FRAME                    = PS_TOP_FIELD | PS_BOTTOM_FIELD,
    PS_TOP_FIELD_FIRST          = PS_FRAME | 4,
    PS_BOTTOM_FIELD_FIRST       = PS_FRAME | 8
};

struct LVASurface
{
    int32_t index;
    void*  data;
};
// converts display aspect ratio to pixel AR
// or vise versa with exchanged width and height
Status DARtoPAR(int32_t width, int32_t height, int32_t dar_h, int32_t dar_v,
                int32_t *par_h, int32_t *par_v);

class VideoData : public MediaData
{
  DYNAMIC_CAST_DECL(VideoData, MediaData)

public:
    struct PlaneInfo
    {
        uint8_t*   m_pPlane;         // pointer to plane data
        mfxSize m_ippSize;        // width and height of the plane
        int32_t   m_iSampleSize;    // sample size (in bytes)
        int32_t   m_iSamples;       // number of samples per plane element
        int32_t   m_iBitDepth;      // number of significant bits per sample (should be <= 8*m_iSampleSize)
        size_t   m_nPitch;         // plane pitch (should be >= width*m_iSamples*m_iSampleSize)
        size_t   m_nOffset;        // Offset from the beginning of aligned memory block
        size_t   m_nMemSize;       // size of occupied memory (pitch*height)
        int32_t   m_iWidthDiv;      // Horizontal downsampling factor
        int32_t   m_iHeightDiv;     // Vertical downsampling factor
    };

    // Default constructor
    VideoData(void);
    // Destructor
    virtual
    ~VideoData(void);

    // operator=
    VideoData & operator = (const VideoData &par);

    // Initialize. Only remembers image characteristics for future.
    virtual
    Status Init(int32_t iWidth,
                int32_t iHeight,
                ColorFormat cFormat,
                int32_t iBitDepth = 0);

    // Initialize. Only remembers image characteristics for future.
    // Should be followed by SetColorFormat
    virtual
    Status Init(int32_t iWidth,
                int32_t iHeight,
                int32_t iPlanes,
                int32_t iBitDepth = 8);

    // Allocate buffer for video data and initialize it.
    virtual
    Status Alloc(size_t requredSize = 0);

    // Reset all plane pointers, release memory if allocated by Alloc
    virtual
    Status ReleaseImage(void);

    // Release video data and all internal memory. Inherited.
    virtual
    Status Close(void);

    // Set buffer pointer, assign all pointers. Inherited.
    // VideoData parameters must have been prepared
    virtual
    Status SetBufferPointer(uint8_t *pbBuffer, size_t nSize);

    // Set common Alignment
    Status SetAlignment(int32_t iAlignment);
    // Get Alignment
    inline
    int32_t GetAlignment(void) const;

    // Set plane destination pointer
    Status SetPlanePointer(void *pDest, int32_t iPlaneNumber);
    // Get plane destination pointer
    inline
      void *GetPlanePointer(int32_t iPlaneNumber);

    // Set image dimensions
    Status SetImageSize(int32_t width, int32_t height);

    // Set plane pitch
    Status SetPlanePitch(size_t nPitch, int32_t iPlaneNumber);
    // Get plane pitch
    inline
    size_t GetPlanePitch(int32_t iPlaneNumber) const;

    // Set plane bitdepth
    Status SetPlaneBitDepth(int32_t iBitDepth, int32_t iPlaneNumber);
    // Get plane bitdepth
    inline
    int32_t GetPlaneBitDepth(int32_t iPlaneNumber) const;

    // Set plane sample size
    Status SetPlaneSampleSize(int32_t iSampleSize, int32_t iPlaneNumber);
    // Get plane sample size
    inline
    int32_t GetPlaneSampleSize(int32_t iPlaneNumber) const;

    // Set color format and planes' information
    Status SetColorFormat(ColorFormat cFormat);
    // Get color format
    inline
    ColorFormat GetColorFormat(void) const;

    // Set aspect Ratio
    inline
    Status SetAspectRatio(int32_t iHorzAspect, int32_t iVertAspect);
    // Get aspect Ratio
    inline
    Status GetAspectRatio(int32_t *piHorzAspect, int32_t *piVertAspect) const;

    // Set picture structure
    inline
    Status SetPictureStructure(PictureStructure picStructure);
    // Get picture structure
    inline
    PictureStructure GetPictureStructure(void) const;
    // Convert to other picture structure
    Status ConvertPictureStructure(PictureStructure newPicStructure);

    inline
    int32_t GetNumPlanes(void) const;
    inline
    int32_t GetWidth(void) const;
    inline
    int32_t GetHeight(void) const;

    // fills PlaneInfo structure
    Status GetPlaneInfo(PlaneInfo* pInfo, int32_t iPlaneNumber);

    // Returns the needed size of a buffer for mapping.
    virtual
    size_t GetMappingSize() const;

    // links plane pointers to surface using provided pitch
    // all pitches and plane info are updated according to current
    // color format.
    // Works only with FourCC formats, which define planes location,
    // like YV12 or NV12.
    virtual
    Status SetSurface(void* ptr, size_t nPitch);

    // Calculate pitch from mapping size
    virtual
    size_t GetPitchFromMappingSize(size_t mappingSize) const;

    // Crop
    virtual Status Crop(UMC::sRECT CropArea);

protected:

    PlaneInfo*       m_pPlaneData;    // pointer to allocated planes info

    int32_t           m_iPlanes;       // number of planes

    mfxSize         m_ippSize;       // dimension of the image

    ColorFormat      m_ColorFormat;   // color format of image
    PictureStructure m_picStructure;  // variants: progressive frame, top first, bottom first, only top, only bottom

    int32_t           m_iHorzAspect;   // aspect ratio: pixel width/height proportion
    int32_t           m_iVertAspect;   // default 1,1 - square pixels

    int32_t           m_iAlignment;    // default 1
    uint8_t*           m_pbAllocated;   // pointer to allocated image buffer

private:
  // Declare private copy constructor to avoid accidental assignment
  // and klocwork complaining.
  VideoData(const VideoData &);
};

// Get Alignment
inline
int32_t VideoData::GetAlignment(void) const
{
  return m_iAlignment;
} // int32_t VideoData::GetAlignment(void)

inline
void* VideoData::GetPlanePointer(int32_t iPlaneNumber)
{
    // check error(s)
    if ((m_iPlanes <= iPlaneNumber) ||
        (0 > iPlaneNumber) ||
        (NULL == m_pPlaneData))
        return NULL;

    return m_pPlaneData[iPlaneNumber].m_pPlane;

} // void *VideoData::GetPlanePointer(int32_t iPlaneNumber)

inline
int32_t VideoData::GetPlaneBitDepth(int32_t iPlaneNumber) const
{
    // check error(s)
    if ((m_iPlanes <= iPlaneNumber) ||
        (0 > iPlaneNumber) ||
        (NULL == m_pPlaneData))
        return 0;

  return m_pPlaneData[iPlaneNumber].m_iBitDepth;

} // int32_t VideoData::GetPlaneBitDepth(int32_t iPlaneNumber)

inline
int32_t VideoData::GetPlaneSampleSize(int32_t iPlaneNumber) const
{
    // check error(s)
    if ((m_iPlanes <= iPlaneNumber) ||
        (0 > iPlaneNumber) ||
        (NULL == m_pPlaneData))
        return 0;

  return m_pPlaneData[iPlaneNumber].m_iSampleSize;

} // int32_t VideoData::GetPlaneSampleSize(int32_t iPlaneNumber)

inline
size_t VideoData::GetPlanePitch(int32_t iPlaneNumber) const
{
    // check error(s)
    if ((m_iPlanes <= iPlaneNumber) ||
        (0 > iPlaneNumber) ||
        (NULL == m_pPlaneData))
        return 0;

  return m_pPlaneData[iPlaneNumber].m_nPitch;

} // size_t VideoData::GetPlanePitch(int32_t iPlaneNumber)

inline
ColorFormat VideoData::GetColorFormat(void) const
{
    return m_ColorFormat;

} // ColorFormat VideoData::GetColorFormat(void)

inline
Status VideoData::SetAspectRatio(int32_t iHorzAspect, int32_t iVertAspect)
{
    if ((1 > iHorzAspect) || (1 > iVertAspect))
        return UMC_ERR_INVALID_STREAM;

    m_iHorzAspect = iHorzAspect;
    m_iVertAspect = iVertAspect;

    return UMC_OK;

} // Status VideoData::SetAspectRatio(int32_t iHorzAspect, int32_t iVertAspect)

inline
Status VideoData::GetAspectRatio(int32_t *piHorzAspect, int32_t *piVertAspect) const
{
    if ((NULL == piHorzAspect) ||
        (NULL == piVertAspect))
        return UMC_ERR_NULL_PTR;

    *piHorzAspect = m_iHorzAspect;
    *piVertAspect = m_iVertAspect;

    return UMC_OK;

} // Status VideoData::GetAspectRatio(int32_t *piHorzAspect, int32_t *piVertAspect)

inline
Status VideoData::SetPictureStructure(PictureStructure picStructure)
{
    if ((PS_TOP_FIELD != picStructure) &&
        (PS_BOTTOM_FIELD != picStructure) &&
        (PS_FRAME != picStructure) &&
        (PS_TOP_FIELD_FIRST != picStructure) &&
        (PS_BOTTOM_FIELD_FIRST != picStructure))
        return UMC_ERR_INVALID_STREAM;

    m_picStructure = picStructure;

    return UMC_OK;

} // Status VideoData::SetPictureStructure(PictureStructure picStructure)

inline
PictureStructure VideoData::GetPictureStructure(void) const
{
    return m_picStructure;

} // PictureStructure VideoData::GetPictureStructure(void)

inline
int32_t VideoData::GetNumPlanes(void) const
{
    return m_iPlanes;

} // int32_t VideoData::GetNumPlanes(void)

inline
int32_t VideoData::GetWidth(void) const
{
    return m_ippSize.width;

} // int32_t VideoData::GetWidth(void)

inline
int32_t VideoData::GetHeight(void) const
{
    return m_ippSize.height;

} // int32_t VideoData::GetHeight(void)

} // namespace UMC

#endif // __UMC_VIDEO_DATA_H__
