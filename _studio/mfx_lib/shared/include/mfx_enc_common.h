// Copyright (c) 2017-2019 Intel Corporation
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

#ifndef _MFX_ENC_COMMON_H_
#define _MFX_ENC_COMMON_H_

#include "umc_defs.h"
#include "mfxdefs.h"
#include "mfxstructures.h"
#include "umc_structures.h"
#include "mfx_common.h"
#include "mfx_common_int.h"

class InputSurfaces
{
private:
    bool                    m_bOpaq;
    bool                    m_bSysMemFrames;
    VideoCORE*              m_pCore;
    mfxFrameAllocRequest    m_request;
    mfxFrameAllocResponse   m_response;
    bool                    m_bInitialized;
    mfxFrameInfo            m_Info;


public:
    InputSurfaces(VideoCORE* pCore):
      m_bOpaq(false),
          m_bSysMemFrames(false),
          m_pCore(pCore),
          m_bInitialized (false)
      {
          memset(&m_request,  0, sizeof(mfxFrameAllocRequest));
          memset(&m_response, 0, sizeof (mfxFrameAllocResponse));  
          memset(&m_Info,0,sizeof(mfxFrameInfo));
      }
     virtual ~InputSurfaces()
     {
         Close();     
     }
     mfxStatus Reset(mfxVideoParam *par, mfxU16 numFrameMin);

     mfxStatus Close();
    

     inline bool isOpaq() {return  m_bOpaq;}
     inline bool isSysMemFrames () {return m_bSysMemFrames;}

     inline mfxFrameSurface1 *GetOriginalSurface(mfxFrameSurface1 *surface)
     {
         return m_bOpaq ? m_pCore->GetNativeSurface(surface) : surface;
     }

     inline mfxFrameSurface1 *GetOpaqSurface(mfxFrameSurface1 *surface)
     {
         return m_bOpaq ? m_pCore->GetOpaqSurface(surface->Data.MemId) : surface;
     } 
};

//----MFX data -> UMC data--------------------------------------------
uint8_t  CalculateMAXBFrames (mfxU8 GopRefDist);
uint16_t CalculateUMCGOPLength (mfxU16 GOPSize, mfxU8 targetUsage);

bool SetPROParameters (mfxU8 TargetUsages,uint8_t &MESpeed, bool &UseFB, bool &FastFB,
                       bool &bIntensityCompensation, bool &bChangeInterpolationType,
                       bool &bChangeVLCTables,
                       bool &bTrellisQuantization, bool &bUsePadding,
                       bool &bVSTransform, bool &deblocking, mfxU8 &smoothing, bool &fastUVMC);
bool SetUFParameters(mfxU8 TargetUsages, bool& mixed,uint32_t& twoRef );

uint32_t CalculateUMCBitrate(mfxU16    TargetKbps);

double CalculateUMCFramerate(mfxU32 FrameRateExtN, mfxU32 FrameRateExtD);
void CalculateMFXFramerate(double framerate, mfxU32* FrameRateExtN, mfxU32* FrameRateExtD);
void ConvertFrameRateMPEG2(mfxU32 FrameRateExtD, mfxU32 FrameRateExtN, mfxI32 &frame_rate_code, mfxI32 &frame_rate_extension_n, mfxI32 &frame_rate_extension_d);
//void ConvertFrameRateMPEG2(mfxU32 FrameRateExtD, mfxU32 FrameRateExtN, mfxI32 &frame_rate_code, mfxI32 &frame_rate_extension_n, mfxI32 &frame_rate_extension_d);

mfxStatus CheckFrameRateMPEG2(mfxU32 &FrameRateExtD, mfxU32 &FrameRateExtN);
mfxStatus CheckAspectRatioMPEG2 (mfxU16 &aspectRatioW, mfxU16 &aspectRatioH, mfxU32 frame_width, mfxU32 frame_heigth, mfxU16 cropW, mfxU16 cropH);


bool IsFrameRateMPEG2Supported(mfxU32 FrameRateExtD, mfxU32 FrameRateExtN);
bool IsAspectRatioMPEG2Supported (mfxU32 aspectRatioW, mfxU32 aspectRatioH, mfxU32 frame_width, mfxU32 frame_heigth, mfxU32 cropW, mfxU32 cropH);
mfxU8 GetAspectRatioCode (mfxU32 dispAspectRatioW, mfxU32 dispAspectRatioH);
bool RecalcFrameMPEG2Rate (mfxU32 FrameRateExtD, mfxU32 FrameRateExtN, mfxU32 &OutFrameRateExtD, mfxU32 &OutFrameRateExtN);
mfxU32 TranslateMfxFRCodeMPEG2(mfxFrameInfo *info, mfxU32 *codeN, mfxU32* codeD); // returns mpeg2 fr code

mfxExtBuffer*       GetExtBuffer       (mfxExtBuffer** ebuffers, mfxU32 nbuffers, mfxU32 BufferId);
mfxExtCodingOption* GetExtCodingOptions(mfxExtBuffer** ebuffers, mfxU32 nbuffers);
mfxExtVideoSignalInfo* GetExtVideoSignalInfo(mfxExtBuffer** ebuffers, mfxU32 nbuffers);

mfxStatus CheckExtVideoSignalInfo(mfxExtVideoSignalInfo * videoSignalInfo);

inline mfxI32 min4(mfxI32 a, mfxI32 b,mfxI32 c,mfxI32 d)
{
    if (a>b)
    {
        if (c<d)
        {
            return (b<c)? b:c;
        }
        else
        {
             return (b<d)? b:d;

        }

    }
    else
    {
        if (c<d)
        {
            return (a<c)? a:c;
        }
        else
        {
             return (a<d)? a:d;

        }
    }
}

UMC::FrameType GetFrameType (mfxU16 FrameOrder, mfxInfoMFX* info);

//----UMC data -> MFX data--------------------------------------------
mfxU16 CalculateMFXGOPLength (uint16_t GOPSize);
mfxU8 CalculateGopRefDist(mfxU8 BNum);

inline bool isIntra(mfxU8 FrameType)
{
    return (FrameType & MFX_FRAMETYPE_I);
}
bool CorrectProfileLevelMpeg2(mfxU16 &profile, mfxU16 & level, mfxU32 w, mfxU32 h, mfxF64 frame_rate, mfxU32 bitrate, mfxU32 GopRefDist);


inline mfxI64 CalcDTSForRefFrameMpeg2(mfxI64 PTS, mfxI32 lastRefDist, mfxU32 maxRefDist, mfxF64 frameRate)
{
    return (maxRefDist == 1 || PTS == -1) ? PTS : PTS - (mfxI64)((1.0/frameRate)*(lastRefDist > 0 ? lastRefDist : 1)*90000);
}
inline mfxI64 CalcDTSForNonRefFrameMpeg2(mfxI64 PTS)
{
    return PTS;
}

struct Rational {mfxU64 n, d;};

#define D3DFMT_NV12 (D3DFORMAT)(MFX_MAKEFOURCC('N', 'V', '1', '2'))
#define D3DDDIFMT_NV12 (D3DDDIFORMAT)(MFX_MAKEFOURCC('N', 'V', '1', '2'))
#define D3DDDIFMT_YU12 (D3DDDIFORMAT)(MFX_MAKEFOURCC('Y', 'U', '1', '2'))

#endif //_MFX_ENC_COMMON_H_
