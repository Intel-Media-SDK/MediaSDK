/******************************************************************************\
Copyright (c) 2005-2017, Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

This sample was distributed or derived from the Intel's Media Samples package.
The original version of this sample may be obtained from https://software.intel.com/en-us/intel-media-server-studio
or https://software.intel.com/en-us/media-client-solutions-support.
\**********************************************************************************/

#include <math.h>
#include "vm/strings_defs.h"
#include <time.h>
#include <stdlib.h>

#include "vm/time_defs.h"

#include "sample_vpp_utils.h"
#include "sample_vpp_roi.h"

#define VPP_ALIGN2(SZ)  (((SZ + 1) >> 1) << 1) // round up to a multiple of 2
#define VPP_MSDK_ALIGN16(SZ)  (((SZ + 15) >> 4) << 4) // round up to a multiple of 16
#define VPP_MSDK_ALIGN32(SZ)  (((SZ + 31) >> 5) << 5) // round up to a multiple of 32

int ownRandomGenerator_32s( int lowest, int highest )
{
    int random_integer;
    int range=(highest-lowest)+1;

    random_integer = lowest+int(range*rand()/(RAND_MAX + 1.0));

    return random_integer;
}

/* *************************************************************************** */

ROIGenerator::ROIGenerator( void )
{
    m_width = m_height = 0;
    m_seed = 0;
} // ROIGenerator::ROIGenerator( void )


ROIGenerator::~ROIGenerator( void )
{
    Close();

} // ROIGenerator::~ROIGenerator( void )

mfxStatus ROIGenerator::Init(  mfxU16  width, mfxU16  height, int seed)
{
    m_width  = width;
    m_height = height;

    m_seed = seed;

    if( 0 == seed )
    {
        m_seed = (int)time(NULL);
    }

    srand( m_seed );

    //printf("\nroi seed = %i \n", m_seed);

    return MFX_ERR_NONE;

} // mfxStatus ROIGenerator::Init( mfxVideoParam *par )

mfxStatus ROIGenerator::Close( void )
{
    m_width = m_height = 0;

    return MFX_ERR_NONE;

} // mfxStatus ROIGenerator::Close( void )


mfxStatus ROIGenerator:: SetROI(mfxFrameInfo* pInfo)
{
    int lowest, highest;
    int result;

    // roi_width
    lowest = 16;
    highest = (int)(m_width - 16);

    result = ownRandomGenerator_32s(lowest, highest);
    result = (((result + 15) >> 4) << 4);
    pInfo->CropW = (mfxU16)result;

    // roi_height
    lowest = 32;
    highest = (int)(m_height - 32);

    result = (mfxU16)ownRandomGenerator_32s(lowest, highest);
    result = (((result + 31) >> 5) << 5);
    pInfo->CropH = (mfxU16)result;

    // roi_x
    lowest = 0;
    highest = (int)(m_width - pInfo->CropW);

    result = ownRandomGenerator_32s(lowest, highest);
    result = (((result + 1) >> 1) << 1);
    pInfo->CropX = (mfxU16)result;

    // roi_y
    lowest = 0;
    highest = (int)(m_height - pInfo->CropH);

    result = ownRandomGenerator_32s(lowest, highest);
    result = (((result + 1) >> 1) << 1);
    pInfo->CropY = (mfxU16)result;

    //printf("\nroi (x, y, w, h) = (%i, %i, %i, %i)\n", pInfo->CropX, pInfo->CropY, pInfo->CropW, pInfo->CropH);

    return MFX_ERR_NONE;

} // mfxStatus ROIGenerator::SetROI(mfxFrameInfo* pInfo)

mfxI32 ROIGenerator::GetSeed( void )
{
    return m_seed;
}

/* EOF */