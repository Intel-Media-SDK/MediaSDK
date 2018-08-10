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

#include "umc_defs.h"

#if defined (MFX_ENABLE_VC1_VIDEO_DECODE)

#ifndef __UMC_VC1_DEC_EXCEPTION_H_
#define __UMC_VC1_DEC_EXCEPTION_H_

#define VC1_VLD_CHECK

namespace UMC
{
    namespace VC1Exceptions
    {
        typedef enum
        {
            vld = 0,
            internal_pipeline_error = 1,
            invalid_stream          = 2,
            mem_allocation_er       = 3

        } exception_type; // we have one class for VC1 exceptions but several types to simplify design
        typedef enum
        {
            max_decoding,    // try to decode
            smart_recon,     // diffrent schemes for reconstruction
            fast_decoding,  // need for skip frames in cae of invalid streams
            fast_err_detect  // signal about errors
        } robust_profile;

        typedef enum
        {
            mbOneLevel,   // we change only bad MB
            mbGroupLevel, // we change MB quant
            FrameLevel    // we change all frame (copy from reference)
        } SmartLevel;

        class vc1_exception
        {
        public:
            vc1_exception(exception_type type):m_ExceptType(type)
            {
            };
            exception_type get_exception_type()
            {
                return m_ExceptType;
            }
            void set_exception_type(exception_type e_type)
            {
                m_ExceptType = e_type;
            }
            exception_type   m_ExceptType;
        };

        // vc1_except_profiler consist from current profile description
        class vc1_except_profiler
        {
        public:
            vc1_except_profiler(robust_profile profile,
                                SmartLevel     level): m_Profile(profile),
                                m_SmartLevel(level)
            {
            }
            static vc1_except_profiler GetEnvDescript(robust_profile profile = max_decoding,
                                                      SmartLevel     level   = mbOneLevel)
            {
                static vc1_except_profiler profiler(profile,level);
                return profiler;
            }
            robust_profile m_Profile;
            SmartLevel     m_SmartLevel;
        };
    }
}
#endif
#endif

