// Copyright (c) 2018 Intel Corporation
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

#include "ts_plugin.h"
#include <unordered_set>

mfxPluginUID readUID(char* s)
{
    mfxPluginUID uid = {};
    mfxU32 b = 0;

    for(mfxU32 i = 0; i < 16; i ++)
    {
#pragma warning(disable:4996)
        sscanf(s+(i<<1), "%2x", &b);
#pragma warning(default:4996)
        uid.Data[i] = (mfxU8)b;
    }
    return uid;
}

void tsPlugin::Init(std::string env, std::string platform)
{
    bool isHW = (std::string::npos == platform.find("_sw_"));

    Reg(MFX_PLUGINTYPE_VIDEO_VPP, MFX_MAKEFOURCC('P','T','I','R'), MFX_PLUGINID_ITELECINE_HW);
    Reg(MFX_PLUGINTYPE_VIDEO_VPP, MFX_MAKEFOURCC('C','A','M','R'), MFX_PLUGINID_CAMERA_HW);
    Reg(MFX_PLUGINTYPE_VIDEO_DECODE, MFX_MAKEFOURCC('C','A','P','T'), MFX_PLUGINID_CAPTURE_HW);
    Reg(MFX_PLUGINTYPE_VIDEO_ENCODE, MFX_CODEC_VP8, MFX_PLUGINID_VP8E_HW);
    Reg(MFX_PLUGINTYPE_VIDEO_DECODE, MFX_CODEC_VP8, MFX_PLUGINID_VP8D_HW);
    Reg(MFX_PLUGINTYPE_VIDEO_DECODE, MFX_CODEC_VP9, MFX_PLUGINID_VP9D_HW);

    if(isHW)
    {
        Reg(MFX_PLUGINTYPE_VIDEO_ENCODE, MFX_CODEC_VP8, MFX_PLUGINID_VP8E_HW);
        Reg(MFX_PLUGINTYPE_VIDEO_DECODE, MFX_CODEC_VP8, MFX_PLUGINID_VP8D_HW);
        Reg(MFX_PLUGINTYPE_VIDEO_DECODE, MFX_CODEC_HEVC, MFX_PLUGINID_HEVCD_HW);
        Reg(MFX_PLUGINTYPE_VIDEO_ENCODE, MFX_CODEC_HEVC, MFX_PLUGINID_HEVCE_HW);
        Reg(MFX_PLUGINTYPE_VIDEO_DECODE, MFX_CODEC_VP9, MFX_PLUGINID_VP9D_HW);
        Reg(MFX_PLUGINTYPE_VIDEO_ENCODE, MFX_CODEC_VP9, MFX_PLUGINID_VP9E_HW);
        Reg(MFX_PLUGINTYPE_VIDEO_ENCODE, MFX_CODEC_HEVC, MFX_PLUGINID_HEVC_FEI_ENCODE, MSDK_PLUGIN_TYPE_FEI);
    }
    else
    {
        Reg(MFX_PLUGINTYPE_VIDEO_ENCODE, MFX_CODEC_HEVC, MFX_PLUGINID_HEVCE_SW);
        Reg(MFX_PLUGINTYPE_VIDEO_DECODE, MFX_CODEC_HEVC, MFX_PLUGINID_HEVCD_SW);
    }

    // Example: TS_PLUGINS=HEVC,1,15dd936825ad475ea34e35f3f54217a6;...
    while(env.size())
    {
        std::string::size_type pos = env.find_first_of(';');

        if(pos == std::string::npos)
        {
            pos = env.size();
        }

        if(    pos >= sizeof(mfxPluginUID) + 6
            && env[4] == ',' && env[6] == ',')
        {
            mfxU32 id   = MFX_MAKEFOURCC(env[0], env[1], env[2], env[3]);
            mfxU32 type = env[5] - '0';
            Reg(type, id, readUID(&env[7]));
            m_list.push_back(readUID(&env[7]));
        }

        env.erase(0, pos+1);
    }
}

mfxU32 tsPlugin::GetType(const mfxPluginUID& uid) {

    typedef std::tuple<mfxU32, mfxU32, MsdkPluginType> Type;

    auto predicate = [&](const std::pair<Type, mfxPluginUID*> &par) {
        return memcmp(uid.Data, par.second->Data, sizeof(par.second->Data)) == 0;
    };

    auto res = std::find_if(std::begin(m_puid), std::end(m_puid), predicate);

    return std::get<0>(res->first);
}

void func_name(std::unordered_multiset<mfxU32>::const_iterator it) {

    switch(*it)
    {
        //common
        case func_list::Init:
        {
            std::cout<<"Init"<<std:: endl;
            break;
        }
        case func_list::QueryIOSurf:
        {
            std::cout<<"QueryIOSurf"<<std:: endl;
            break;
        }
        case func_list::Query:
        {
            std::cout<<"Query"<<std:: endl;
            break;
        }
        case func_list::Reset:
        {
            std::cout<<"Reset"<<std:: endl;
            break;
        }
        case func_list::GetVideoParam:
        {
            std::cout<<"GetVideoParam"<<std:: endl;
            break;
        }
        case func_list::Close:
        {
            std::cout<<"Close"<<std:: endl;
            break;
        }
        case func_list::PluginInit:
        {
            std::cout<<"PluginInit"<<std:: endl;
            break;
        }
        case func_list::PluginClose:
        {
            std::cout<<"PluginClose"<<std:: endl;
            break;
        }
        case func_list::GetPluginParam:
        {
            std::cout<<"GetPluginParam"<<std:: endl;
            break;
        }
        case func_list::Execute:
        {
            std::cout<<"Execute"<<std:: endl;
            break;
        }
        case func_list::FreeResources:
        {
            std::cout<<"FreeResources"<<std:: endl;
            break;
        }
        case func_list::Release:
        {
            std::cout<<"Release"<<std:: endl;
            break;
        }
        case func_list::SetAuxParams:
        {
            std::cout<<"SetAuxParams"<<std:: endl;
            break;
        }

        //VPP
        case func_list::VPPFrameSubmit:
        {
            std::cout<<"VPPFrameSubmit"<<std:: endl;
            break;
        }
        case func_list::VPPFrameSubmitEx:
        {
            std::cout<<"VPPFrameSubmitEx"<<std::endl;
            break;
        }

        //ENCODER
        case func_list::EncodeFrameSubmit:
        {
            std::cout<<"EncodeFrameSubmit"<<std:: endl;
            break;
        }

        //DECODER
        case func_list::DecodeHeader:
        {
            std::cout<<"DecodeHeader"<<std:: endl;
            break;
        }
        case func_list::GetPayload:
        {
            std::cout<<"GetPayload"<<std:: endl;
            break;
        }
        case func_list::DecodeFrameSubmit:
        {
            std::cout<<"DecodeFrameSubmit"<<std:: endl;
            break;
        }
    }
}

bool check_calls(std::vector<mfxU32> real_calls, std::vector<mfxU32> expected_calls)
{
    bool result = true;
    std::unordered_multiset<mfxU32> _calls(real_calls.begin(), real_calls.end());
    std::unordered_multiset<mfxU32> _m_calls(expected_calls.begin(),expected_calls.end());

    for (auto cls=_calls.cbegin();cls!=_calls.cend();)
    {
        auto mcls=_m_calls.find(*cls);
        // equal elements exist
        if (mcls!=_m_calls.end())
        {
            // remove equal from both
            cls=_calls.erase(cls);
            _m_calls.erase(mcls);
        }
        else
        {
            ++cls;
        }
    }
    if(_m_calls.empty() && !_calls.empty())
    {
        std::cout <<"ERROR: Invalid calls detected"<<std:: endl;
        for (std::unordered_multiset<mfxU32>::const_iterator i(_calls.begin()), end(_calls.end()); i != end; ++i)
        {
            std::cout <<"The following functions were not called: " << std:: endl;
            func_name(i);
        }
         result = false;
    }
    else if(!_m_calls.empty() && _calls.empty())
    {
        std::cout <<"ERROR: Invalid calls detected"<<std:: endl;
        for (std::unordered_multiset<mfxU32>::const_iterator i(_m_calls.begin()), end(_m_calls.end()); i != end; ++i)
        {
            std::cout <<"The following functions were unexpected called: " << std:: endl;
            func_name(i);
        }
        result = false;
    }
    if(!_calls.empty() && !_m_calls.empty())
    {
        std::cout <<"ERROR: Invalid calls detected"<<std:: endl;
        for (std::unordered_multiset<mfxU32>::const_iterator i(_calls.begin()), end(_calls.end()); i != end; ++i)
        {
            std::cout <<"The following functions were not called: " << std:: endl;
            func_name(i);
        }
        for (std::unordered_multiset<mfxU32>::const_iterator i(_m_calls.begin()), end(_m_calls.end()); i != end; ++i)
        {
                  std::cout <<"The following functions were unexpected called: " << std:: endl;
                  func_name(i);
        }
        result = false;
    }
    return result;
}
