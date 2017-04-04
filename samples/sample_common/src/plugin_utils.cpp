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

#include "mfx_samples_config.h"

#include "plugin_utils.h"
#include "mfxvp8.h"

bool AreGuidsEqual(const mfxPluginUID& guid1, const mfxPluginUID& guid2)
{
    for(size_t i = 0; i != sizeof(mfxPluginUID); i++)
    {
        if (guid1.Data[i] != guid2.Data[i])
            return false;
    }
    return true;
}

mfxStatus ConvertStringToGuid(const msdk_string & strGuid, mfxPluginUID & mfxGuid)
{
    mfxStatus sts = MFX_ERR_NONE;
    mfxU32 hex = 0;
    for(size_t i = 0; i != sizeof(mfxPluginUID); i++)
    {
        hex = 0;

        if (1 != sscanf(strGuid.c_str() + 2*i, MSDK_STRING("%2x"), &hex))
        {
            sts = MFX_ERR_UNKNOWN;
            break;
        }
        if (hex == 0 && (const msdk_char *)strGuid.c_str() + 2*i != msdk_strstr((const msdk_char *)strGuid.c_str() + 2*i,  MSDK_STRING("00")))
        {
            sts = MFX_ERR_UNKNOWN;
            break;
        }
        mfxGuid.Data[i] = (mfxU8)hex;
    }
    if (sts != MFX_ERR_NONE)
        MSDK_ZERO_MEMORY(mfxGuid);
    return sts;
}

const mfxPluginUID & msdkGetPluginUID(mfxIMPL impl, msdkComponentType type, mfxU32 uCodecid)
{
    if (impl == MFX_IMPL_SOFTWARE)
    {
        switch(type)
        {
        case MSDK_VDECODE:
            switch(uCodecid)
            {
            case MFX_CODEC_HEVC:
                return MFX_PLUGINID_HEVCD_SW;
            }
            break;
        case MSDK_VENCODE:
            switch(uCodecid)
            {
            case MFX_CODEC_HEVC:
                return MFX_PLUGINID_HEVCE_SW;
            }
            break;
        }
    }
    else if (impl |= MFX_IMPL_HARDWARE)
    {
        switch(type)
        {
        case MSDK_VDECODE:
            switch(uCodecid)
            {
            case MFX_CODEC_HEVC:
                return MFX_PLUGINID_HEVCD_HW;
            case MFX_CODEC_VP8:
                return MFX_PLUGINID_VP8D_HW;
            case MFX_CODEC_VP9:
                return MFX_PLUGINID_VP9D_HW;
            }
            break;
        case MSDK_VENCODE:
            switch(uCodecid)
            {
            case MFX_CODEC_HEVC:
                return MFX_PLUGINID_HEVCE_HW;
            case MFX_CODEC_VP8:
                return MFX_PLUGINID_VP8E_HW;
            }
            break;
        case MSDK_VENC:
            switch(uCodecid)
            {
            case MFX_CODEC_HEVC:
                return MFX_PLUGINID_HEVCE_FEI_HW;   // HEVC FEI uses ENC interface
            }
            break;
        }
    }

    return MSDK_PLUGINGUID_NULL;
}
