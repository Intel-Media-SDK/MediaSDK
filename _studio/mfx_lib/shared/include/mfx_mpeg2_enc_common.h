// Copyright (c) 2019 Intel Corporation
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

#ifndef __MFX_MPEG2_ENC_COMMON_H__
#define __MFX_MPEG2_ENC_COMMON_H__

// functions for VideoEditing

#include "mfx_common.h"
#include "mfxvideo.h"

class SHParametersEx
{
public:
    SHParametersEx()
    {
        m_pBuffer = 0;
        m_bufLen  = 0;
    }
    ~SHParametersEx()
    {
        if (m_pBuffer)
        {
            delete [] m_pBuffer;        
        }    
    }
    void GetSH (mfxU8 **pSH, mfxU32 *len)
    {
        *pSH = m_pBuffer;
        *len = m_bufLen;    
    }
    static bool CheckSHParameters (mfxU8 *pSH, mfxU32 len, mfxU32 &real_len, mfxVideoParam *par, mfxExtCodingOption * extOpt)
    {
        real_len = 0;
        return DecodeSequenceHeader (pSH,len, par, extOpt, real_len);
    }
    mfxStatus FillSHParameters (mfxVideoParam* par, mfxExtCodingOption * extOpt)
    {
        mfxU8 *pSH      = nullptr;
        mfxU32 len      = 0;
        mfxU32 real_len = 0;

        if (GetSPSBuffer(par, &pSH, &len))
        {
            if (!DecodeSequenceHeader (pSH,len, par, extOpt, real_len)) 
                return MFX_ERR_INCOMPATIBLE_VIDEO_PARAM;

            delete [] m_pBuffer;
            m_pBuffer = new mfxU8 [real_len];

            std::copy(pSH, pSH + real_len, m_pBuffer);
            m_bufLen  = real_len;                     
        }
        return MFX_ERR_NONE;
    }
    static bool GetSPSBuffer(mfxVideoParam* par, mfxU8 **ppSH, mfxU32 *pLen)
    {
        mfxExtCodingOptionSPSPPS* pCO = GetExtCodingOptionsSPSPPS(par->ExtParam, par->NumExtParam);
        if (pCO)
        {
            if (pCO->SPSBuffer && pCO->SPSBufSize!=0)
            {
                *ppSH = pCO->SPSBuffer;
                *pLen = pCO->SPSBufSize;
                return true;
            }        
        }
        *ppSH = 0;
        *pLen = 0;
        return false;
    }
    static mfxExtCodingOptionSPSPPS* GetExtCodingOptionsSPSPPS(mfxExtBuffer** ebuffers, mfxU32 nbuffers) 
    {
        if (ebuffers)
        {
          for(mfxU32 i=0; i<nbuffers; i++) 
          {
            if((*(ebuffers+i))->BufferId == MFX_EXTBUFF_CODING_OPTION_SPSPPS) 
            {
              return (mfxExtCodingOptionSPSPPS*)(*(ebuffers+i));
            }
          }
        }
        return 0;
    }
protected:

// VideoEncode methods
#define _GetBits(n,out)   \
    {\
        mfxU32 bitreaded = 0;\
        \
        out = 0;\
        if (ptr - pSH > (mfxI32)len - ((n+7)>>3))\
            return false;\
        \
        while (n > bitreaded)\
        {\
            mfxU32 t = ((*ptr) << bitpos) &0xff;\
            if (n - bitreaded >=  8 - bitpos)\
            {\
                uint32_t num_bits = 8 - bitpos;\
                t = t >> (8-num_bits);\
                out = (out <<num_bits)|t;\
                bitreaded += num_bits;\
                bitpos = 0;\
                ptr ++;\
            }\
            else\
            {\
                uint32_t num_bits = n - bitreaded;\
                t = t >> (8-num_bits);\
                out = (out <<num_bits)|t;\
                bitpos += num_bits;  \
                bitreaded += num_bits; \
            \
            }   \
        }\
    }
    static bool DARtoPAR(mfxU32 width, mfxU32 height, mfxU32 dar_h, mfxU32 dar_v,
                        mfxU16 *par_h, mfxU16 *par_v)
    {
      // (width*par_h) / (height*par_v) == dar_h/dar_v =>
      // par_h / par_v == dar_h * height / (dar_v * width)
      mfxU32 simple_tab[] = {2,3,5,7,11,13,17,19,23,29,31,37,41,43,47,53,59};
      mfxU32 i, denom;

      // suppose no overflow of 32s
      mfxU32 h = dar_h * height;
      mfxU32 v = dar_v * width;
      // remove common multipliers
      while( ((h|v)&1) == 0 ) {
        h>>=1;
        v>>=1;
      }

      for(i=0;i<sizeof(simple_tab)/sizeof(simple_tab[0]);i++) {
        denom = simple_tab[i];
        while(h%denom==0 && v%denom==0) {
          v /= denom;
          h /= denom;
        }
        if(v<=denom || h<=denom)
          break;
      }
      *par_h = (mfxU16)h;
      *par_v = (mfxU16)v;
      // can don't reach minimum, no problem
     // if(i<sizeof(simple_tab)/sizeof(simple_tab[0]))
     //   return false;
      return true;
    }

    static bool DecodeSequenceHeader(
        mfxU8 *              pSH,
        mfxU32               len,
        mfxVideoParam *      par,
        mfxExtCodingOption * extOpt,
        mfxU32 &             real_len)
    {
        mfxU32 bitpos = 0;
        mfxU8* ptr = pSH;
        mfxU32 temp = 0;
        mfxU32 w = 0, h = 0;
        mfxU32 aspect_ratio = 0;
        mfxU32 frame_rate_code = 0;
        mfxU32 frame_rate_code_n = 0;
        mfxU32 frame_rate_code_d = 0;
        mfxU32 vbv_buffer_size = 0;

        static const mfxU32 frame_rate_value_n [9] = {0, 24000, 24,25,30000,30,50,60000,60};
        static const mfxU32 frame_rate_value_d [9] = {0, 1001,  1,  1, 1001, 1, 1, 1001, 1};

        real_len = 0;

        while ( *ptr == 0 && ptr - pSH < (mfxI32)len)
        {
            ptr ++;
        }
        _GetBits (8,temp);
        if (temp != 0x01)
            return false;

        _GetBits (8,temp);  // start code
        if (temp != 0xB3)
            return false;
   
        _GetBits (12, w);  // start code
        _GetBits (12, h);  // start code
        
        if (!w || !h)
            return false;

        _GetBits (4,aspect_ratio);
        if (aspect_ratio > 4 || !aspect_ratio)
            return false;
        
        
       switch(aspect_ratio)
       {
       case 1:
        par->mfx.FrameInfo.AspectRatioH = 1;
        par->mfx.FrameInfo.AspectRatioW = 1;
        break;
       case 2:
        DARtoPAR(w, h, 4, 3, &par->mfx.FrameInfo.AspectRatioW , &par->mfx.FrameInfo.AspectRatioH);
        break;
       case 3:
        DARtoPAR(w, h, 16, 9, &par->mfx.FrameInfo.AspectRatioW , &par->mfx.FrameInfo.AspectRatioH);
        break;
       case 4:
        DARtoPAR(w, h, 221, 100, &par->mfx.FrameInfo.AspectRatioW , &par->mfx.FrameInfo.AspectRatioH);
        break;
       default:
         return false;         
       }
        _GetBits (4,frame_rate_code);
        if (frame_rate_code > 8 || !frame_rate_code)
            return false;

        _GetBits (18,temp); //bitrate
        
        temp = ((temp)*400)/1000;
        if (temp > 0x0000FFFF)
            return false;
        if (temp != 0)
        {
            par->mfx.TargetKbps = (mfxU16)temp;
            par->mfx.RateControlMethod = MFX_RATECONTROL_CBR;
        }

        _GetBits (1,temp);
        if (!temp)
            return false;

        _GetBits (10, vbv_buffer_size);
        
        _GetBits (1,temp);
        if (temp)
            return false;

        _GetBits (1,temp); // intra quant matrix
        if (temp)
            return false;

        _GetBits (1,temp); // inter quant matrix
        if (temp)
            return false;

        if (bitpos!=0)
        {
            ptr ++;
            bitpos = 0;
        
        }
        while ( *ptr == 0 && ptr - pSH < (mfxI32)len)
        {
            ptr ++;
        }
        temp = 0;
        if (ptr - pSH >= (mfxI32)len - 3)
          return false;

        _GetBits (8,temp);
        if (temp != 0x01)
            return false;

        _GetBits (12,temp);  // start code
        if (temp != 0x0B51)
            return false;

       _GetBits (1,temp); // escape bit

       _GetBits (3,temp); // profile

        switch(temp)
        {
        case 5:
            par->mfx.CodecProfile = MFX_PROFILE_MPEG2_SIMPLE;
            break;
        case 4:
            par->mfx.CodecProfile = MFX_PROFILE_MPEG2_MAIN;
            break;
        case 6:
            par->mfx.CodecProfile = MFX_PROFILE_MPEG2_HIGH;
            break;
        default:
             return false;   
        }

       _GetBits (4,temp); // level

        switch(temp)
        {
        case 10:
            par->mfx.CodecLevel = MFX_LEVEL_MPEG2_LOW;
            break;
        case 8:
             par->mfx.CodecLevel = MFX_LEVEL_MPEG2_MAIN;
             break;
        case 6:
             par->mfx.CodecLevel = MFX_LEVEL_MPEG2_HIGH1440;
             break;
        case 4:
             par->mfx.CodecLevel = MFX_LEVEL_MPEG2_HIGH;
             break;
        default:
             return false;   
        }

        _GetBits (1,temp); // progressive
        
        if (temp == 1)
        {
            par->mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
        }
        else
        {
            par->mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_UNKNOWN;

            if (extOpt)
                extOpt->FramePicture = MFX_CODINGOPTION_ON;
        }

        _GetBits (2,temp); //chroma format
        if (temp != 1)
            return false;

        par->mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;

        _GetBits (2,temp); // ext h
         if (temp != 0)
            return false;

        _GetBits (2,temp); // ext w
         if (temp != 0)
            return false;

        _GetBits (12,temp); //bitrate ex
        if (temp != 0)
            return false;

        _GetBits (1,temp);
        if (temp != 1)
            return false;

        _GetBits (8,temp); // vbv_buffer_size_ext
        vbv_buffer_size |= (temp << 10);
        par->mfx.BufferSizeInKB = mfxU16(std::min<mfxU32>(0xffff, 2 * vbv_buffer_size));

        _GetBits (1,temp); //low delay
        if (temp == 1)
            return false;

        _GetBits (2,frame_rate_code_n);
        _GetBits (5,frame_rate_code_d);
         
        if (bitpos!=0)
        {
            ptr ++;  
            bitpos = 0;
        }
        while ( *ptr == 0 && ptr - pSH < (mfxI32)len)
        {
            ptr ++;
        }
        temp = 0;
        if (ptr - pSH < (mfxI32)len - 3)
        {
            _GetBits (8,temp);
            if (temp != 0x01)
                return false;
            _GetBits (12,temp);  // start code            
        }
        
        par->mfx.FrameInfo.CropX = 0;
        par->mfx.FrameInfo.CropY = 0;
        par->mfx.FrameInfo.CropW = (mfxU16) w;
        par->mfx.FrameInfo.CropH = (mfxU16) h;

        par->mfx.FrameInfo.Width  = par->mfx.FrameInfo.Width > (mfxU16)(((w+15)/16)*16)? par->mfx.FrameInfo.Width : (mfxU16)(((w+15)/16)*16);
        if (par->mfx.FrameInfo.PicStruct == MFX_PICSTRUCT_PROGRESSIVE)
        {
            par->mfx.FrameInfo.Height = par->mfx.FrameInfo.Height> (mfxU16)(((h+15)/16)*16)? par->mfx.FrameInfo.Height: (mfxU16)(((h+15)/16)*16);
        }
        else
        {
            par->mfx.FrameInfo.Height = par->mfx.FrameInfo.Height> (mfxU16)(((h+31)/32)*32)? par->mfx.FrameInfo.Height: (mfxU16)(((h+31)/32)*32);
        }
        par->mfx.FrameInfo.FrameRateExtD = (frame_rate_code_d + 1)*frame_rate_value_d[frame_rate_code];
        par->mfx.FrameInfo.FrameRateExtN = (frame_rate_code_n + 1)*frame_rate_value_n[frame_rate_code];

        if (temp == 0x0B52)
        {  
              _GetBits (3,temp); //video format
             if (temp > 5)
                 return false;

             _GetBits (1,temp); //colour_description

             if (temp)
             {
                 _GetBits (24,temp);          
             }
             _GetBits (14,temp);
             if (temp != w)
                 return false;

             _GetBits (1,temp);
             if (temp != 1)
                 return false;

             _GetBits (14,temp);
             if (temp != h)
                 return false;

             if (bitpos!=0)
             {
                ptr ++; 
                bitpos = 0;
             }
        }
        real_len = (mfxU32)(ptr - pSH);        
        return true;   
    
    }
#undef _GetBits


private:
    mfxU8*        m_pBuffer;
    mfxU32        m_bufLen;

    // Declare private copy constructor to avoid accidental assignment
    // and klocwork complaining.
    SHParametersEx (const SHParametersEx &);
    SHParametersEx & operator = (const SHParametersEx &);
};

#endif