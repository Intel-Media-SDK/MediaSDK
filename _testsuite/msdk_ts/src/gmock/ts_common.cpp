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

#include "ts_common.h"
#include "ts_struct.h"

#if defined(_WIN32) || defined(_WIN64)
    #include "..\opensource\mfx_dispatch\include\mfx_dxva2_device.h"
#endif

tsTrace      g_tsLog(std::cout.rdbuf());
tsStatus     g_tsStatus;
mfxIMPL      g_tsImpl     = MFX_IMPL_AUTO;
HWType       g_tsHWtype   = MFX_HW_UNKNOWN;
OSFamily     g_tsOSFamily = MFX_OS_FAMILY_UNKNOWN;
OSWinVersion g_tsWinVersion = MFX_WIN_VER_UNKNOWN;
bool         g_tsIsSSW = false;
mfxVersion   g_tsVersion  = {MFX_VERSION_MINOR, MFX_VERSION_MAJOR};
mfxU32       g_tsTrace    = 1;
tsPlugin     g_tsPlugin;
tsStreamPool g_tsStreamPool;
tsConfig     g_tsConfig = {0, false, MFX_GPUCOPY_DEFAULT, ""};

bool operator == (const mfxFrameInfo& v1, const mfxFrameInfo& v2)
{
    return !memcmp(&v1, &v2, sizeof(mfxFrameInfo));
}

bool operator == (const mfxFrameData& v1, const mfxFrameData& v2)
{
    return !memcmp(&v1, &v2, sizeof(mfxFrameData));
}

#pragma warning(disable:4996)
std::string ENV(const char* name, const char* def)
{
    std::string sv = def;
    char*       cv = getenv(name);

    if(cv)
        sv = cv;

    g_tsLog << "ENV: " << name << " = " << sv << "\n";

    return sv;
}

void set_brc_params(tsExtBufType<mfxVideoParam>* p)
{

    if(!p->mfx.FrameInfo.FrameRateExtN && !p->mfx.FrameInfo.FrameRateExtD)
    {
        p->mfx.FrameInfo.FrameRateExtN = 30;
        p->mfx.FrameInfo.FrameRateExtD = 1;
    }

    /*
    BitRate for AVC:
        [image width] x [image height] x [framerate] x [motion rank] x 0.07 = [desired bitrate]
        [motion rank]: from 1 (low motion) to 4 (high motion)
    */
    mfxU32 fr = mfxU32(p->mfx.FrameInfo.FrameRateExtN / p->mfx.FrameInfo.FrameRateExtD);
    mfxU16 br = mfxU16(p->mfx.FrameInfo.Width * p->mfx.FrameInfo.Height * fr * 2 * 0.07 / 1000);

    if (p->mfx.CodecId == MFX_CODEC_MPEG2)
        br = (mfxU16)(br*1.5);
    else if (p->mfx.CodecId == MFX_CODEC_HEVC)
        br >>= 1;

    if (p->mfx.RateControlMethod == MFX_RATECONTROL_CQP)
    {
        p->mfx.QPI = p->mfx.QPP = p->mfx.QPB = 0;
        p->mfx.QPI = 23;
        if (!p->mfx.GopPicSize || p->mfx.GopPicSize > 1)
            p->mfx.QPP = 25;
        if (!p->mfx.GopPicSize || p->mfx.GopRefDist > 1)
            p->mfx.QPB = 25;
    } else if (p->mfx.RateControlMethod == MFX_RATECONTROL_CBR ||
               p->mfx.RateControlMethod == MFX_RATECONTROL_VBR ||
               p->mfx.RateControlMethod == MFX_RATECONTROL_VCM ||
               p->mfx.RateControlMethod == MFX_RATECONTROL_LA ||
               p->mfx.RateControlMethod == MFX_RATECONTROL_LA_HRD ||
               p->mfx.RateControlMethod == MFX_RATECONTROL_LA_EXT)
    {
        p->mfx.TargetKbps = p->mfx.MaxKbps = p->mfx.InitialDelayInKB = 0;

        p->mfx.MaxKbps = p->mfx.TargetKbps = br;
        if (p->mfx.RateControlMethod != MFX_RATECONTROL_CBR)
        {
            p->mfx.MaxKbps = (mfxU16)(p->mfx.TargetKbps * 1.3);
        }
        // buffer = 0.5 sec
        p->mfx.BufferSizeInKB = mfxU16(p->mfx.MaxKbps / fr * mfxU16(fr / 2));
        p->mfx.InitialDelayInKB = mfxU16(p->mfx.BufferSizeInKB / 2);

        if (p->mfx.RateControlMethod == MFX_RATECONTROL_VCM)
            p->mfx.GopRefDist = 1;

        if (p->mfx.RateControlMethod == MFX_RATECONTROL_LA ||
            p->mfx.RateControlMethod == MFX_RATECONTROL_LA_HRD ||
            p->mfx.RateControlMethod == MFX_RATECONTROL_LA_EXT)
        {
            p->mfx.InitialDelayInKB = 0;

            mfxExtCodingOption2* p_cod2 = (mfxExtCodingOption2*)p->GetExtBuffer(MFX_EXTBUFF_CODING_OPTION2);
            if (!p_cod2)
            {
                mfxExtCodingOption2& cod2 = *p;
                p_cod2 = &cod2;
            }

            mfxExtCodingOption3* p_cod3 = (mfxExtCodingOption3*)p->GetExtBuffer(MFX_EXTBUFF_CODING_OPTION3);
            if (p_cod3)
            {
                p_cod3->WinBRCMaxAvgKbps = (mfxU16)(p->mfx.TargetKbps * 1.5);
                p_cod3->WinBRCSize = p->mfx.FrameInfo.FrameRateExtN << 2;
            }
            p_cod2->LookAheadDepth = 20;

            if (p->mfx.RateControlMethod == MFX_RATECONTROL_LA_HRD)
            {
                mfxExtCodingOption* p_cod = (mfxExtCodingOption*)p->GetExtBuffer(MFX_EXTBUFF_CODING_OPTION);
                if (!p_cod)
                {
                    mfxExtCodingOption& cod = *p;
                    p_cod = &cod;
                }
                p_cod->NalHrdConformance = MFX_CODINGOPTION_ON;
            }
        }
    } else if (p->mfx.RateControlMethod == MFX_RATECONTROL_AVBR)
    {
        p->mfx.TargetKbps = p->mfx.Convergence = p->mfx.Accuracy = 0;

        p->mfx.TargetKbps = br;
        p->mfx.Convergence = 1;
        p->mfx.Accuracy = 2;
    } else if (p->mfx.RateControlMethod == MFX_RATECONTROL_ICQ ||
               p->mfx.RateControlMethod == MFX_RATECONTROL_LA_ICQ)
    {
        p->mfx.TargetKbps = p->mfx.Convergence = p->mfx.Accuracy = 0;

        p->mfx.ICQQuality = 20;

        if (p->mfx.RateControlMethod == MFX_RATECONTROL_LA_ICQ)
        {
            mfxExtCodingOption2* p_cod2 = (mfxExtCodingOption2*)p->GetExtBuffer(MFX_EXTBUFF_CODING_OPTION2);
            if (p_cod2)
            {
                p_cod2->LookAheadDepth = 20;
            } else
            {
                mfxExtCodingOption2& cod2 = *p;
                cod2.LookAheadDepth = 20;
            }
        }
    } else if (p->mfx.RateControlMethod == MFX_RATECONTROL_QVBR)
    {
        p->mfx.TargetKbps = p->mfx.Convergence = p->mfx.Accuracy = 0;
        p->mfx.ICQQuality = 0;
        p->mfx.MaxKbps = p->mfx.TargetKbps = br;

        mfxExtCodingOption3* p_cod3 = (mfxExtCodingOption3*)p->GetExtBuffer(MFX_EXTBUFF_CODING_OPTION3);
        if (p_cod3)
        {
            p_cod3->QVBRQuality = 20;
        } else
        {
            mfxExtCodingOption3& cod3 = *p;
            cod3.QVBRQuality = 20;
        }
    }
}

void set_chromaformat(mfxFrameInfo& frameinfo)
{
    const mfxU32& FourCC = frameinfo.FourCC;
    mfxU16& ChromaFormat = frameinfo.ChromaFormat;

    switch( FourCC ) {
        case MFX_FOURCC_P8        :
        case MFX_FOURCC_P8_TEXTURE:
            ChromaFormat = 0;
            break;
        case MFX_FOURCC_NV12      :
        case MFX_FOURCC_YV12      :

        case MFX_FOURCC_P010      :
        case MFX_FOURCC_P016      :
            ChromaFormat = MFX_CHROMAFORMAT_YUV420;
            break;
        case MFX_FOURCC_NV16      :
        case MFX_FOURCC_YUY2      :
        case MFX_FOURCC_UYVY      :

        case MFX_FOURCC_P210      :
        case MFX_FOURCC_Y210      :
        case MFX_FOURCC_Y216      :
            ChromaFormat = MFX_CHROMAFORMAT_YUV422;
            break;
        case MFX_FOURCC_AYUV      :

        case MFX_FOURCC_Y410      :
        case MFX_FOURCC_Y416      :

        case MFX_FOURCC_A2RGB10   :
        case MFX_FOURCC_ARGB16    :
        case MFX_FOURCC_ABGR16    :
        case MFX_FOURCC_R16       :
        case MFX_FOURCC_AYUV_RGB4 :
        case MFX_FOURCC_RGB3      :
        case MFX_FOURCC_RGB4      :
        case MFX_FOURCC_BGR4      :
            ChromaFormat = MFX_CHROMAFORMAT_YUV444;
            break;
        default:
            ChromaFormat = MFX_CHROMAFORMAT_YUV420;
            break;
    }
}
void set_chromaformat_mfx(tsExtBufType<mfxVideoParam>* p)
{
    return set_chromaformat(p->mfx.FrameInfo);
}
void set_chromaformat_vpp(tsExtBufType<mfxVideoParam>* p)
{
    set_chromaformat(p->vpp.In);
    return set_chromaformat(p->vpp.Out);
}

void MFXVideoTest::SetUp()
{
    std::string platform  = ENV("TS_PLATFORM", "auto");
    std::string trace     = ENV("TS_TRACE", "1");
    std::string plugins   = ENV("TS_PLUGINS", "");
    std::string lowpower  = ENV("TS_LOWPOWER", "");
    std::string cfg_file  = ENV("TS_CONFIG_FILE", "");
    std::string copy_mode = ENV("TS_GPUCOPY_MODE", "");

    g_tsConfig.sim = (ENV("TS_SIM", "0") == "1");

    g_tsConfig.lowpower = MFX_CODINGOPTION_UNKNOWN;
    if      (lowpower ==  "ON") { g_tsConfig.lowpower =  MFX_CODINGOPTION_ON; }
    else if (lowpower == "OFF") { g_tsConfig.lowpower = MFX_CODINGOPTION_OFF; }

    g_tsConfig.GPU_copy_mode = MFX_GPUCOPY_DEFAULT;
    if      (copy_mode ==  "ON") { g_tsConfig.GPU_copy_mode =  MFX_GPUCOPY_ON; }
    else if (copy_mode == "OFF") { g_tsConfig.GPU_copy_mode = MFX_GPUCOPY_OFF; }

    g_tsConfig.cfg_filename = cfg_file;
    g_tsStatus.reset();

    if(platform.size())
    {
        if (platform[0] == 'w')
        {
            g_tsOSFamily = MFX_OS_FAMILY_WINDOWS;
            if (platform[1] == '1' && platform[2] == '0')
                g_tsWinVersion = MFX_WIN_VER_W10;
        }
        /* For Linux we have to common names
         * (1): c7.2_bdw_64_server and variants
         * (2): yocto_bxt_64_server */
        else if ((platform[0] == 'c') || (platform[0] == 'y'))
        {
            g_tsOSFamily = MFX_OS_FAMILY_LINUX;
        }
        if (platform.find("_ssw_") != std::string::npos)
        {
            g_tsIsSSW = true;
            g_tsImpl = MFX_IMPL_SOFTWARE;
        }
        else if(platform.find("_sw_") != std::string::npos)
        {
            g_tsImpl = MFX_IMPL_SOFTWARE;
        }
        else if(platform == "auto")
        {
            g_tsImpl = MFX_IMPL_AUTO;
        }
        else
        {
            if(platform.find("snb") != std::string::npos)
                g_tsHWtype = MFX_HW_SNB;
            else if(platform.find("ivb") != std::string::npos)
                g_tsHWtype = MFX_HW_IVB;
            else if(platform.find("vlv") != std::string::npos)
                g_tsHWtype = MFX_HW_VLV;
            else if(platform.find("hsw-ult") != std::string::npos)
                g_tsHWtype = MFX_HW_HSW_ULT;
            else if(platform.find("hsw") != std::string::npos)
                g_tsHWtype = MFX_HW_HSW;
            else if(platform.find("bdw") != std::string::npos)
                g_tsHWtype = MFX_HW_BDW;
            else if(platform.find("skl") != std::string::npos)
                g_tsHWtype = MFX_HW_SKL;
            else if(platform.find("bxt") != std::string::npos)
                g_tsHWtype = MFX_HW_APL;
            else if(platform.find("kbl") != std::string::npos)
                g_tsHWtype = MFX_HW_KBL;
            else if (platform.find("glk") != std::string::npos)
                g_tsHWtype = MFX_HW_GLK;
            else if (platform.find("cfl") != std::string::npos)
                g_tsHWtype = MFX_HW_CFL;
            else if(platform.find("cnl") != std::string::npos)
                g_tsHWtype = MFX_HW_CNL;
            else if(platform.find("icl") != std::string::npos)
                g_tsHWtype = MFX_HW_ICL;
            else if (platform.find("lkf") != std::string::npos)
                g_tsHWtype = MFX_HW_LKF;
            else if(platform.find("tgl") != std::string::npos)
                g_tsHWtype = MFX_HW_TGL;
            else
                g_tsHWtype = MFX_HW_UNKNOWN;

            g_tsImpl = MFX_IMPL_HARDWARE;
        }

        if(platform.find("d3d11") != std::string::npos)
        {
            g_tsImpl |= MFX_IMPL_VIA_D3D11;
        }
    }
    sscanf(trace.c_str(), "%d", &g_tsTrace);

    g_tsPlugin.Init(plugins, platform);

#if defined(_WIN32) || defined(_WIN64)
    for (mfxU32 adapter = 0; adapter < 4 && !g_tsConfig.sim; adapter++)
    {
        MFX::DXVA2Device device;

        if (g_tsImpl & MFX_IMPL_VIA_D3D11)
        {
            if (!device.InitDXGI1(adapter))
                break;
        }
        else if (!device.InitD3D9(adapter))
            break;


        if (device.GetVendorID() != 0x8086)
            continue;

        auto id = device.GetDeviceID();

        switch (id)
        {
        //CNL Simulation:
        case 0xA00: //iCNLGT0
        case 0xA01: //iCNLGT1
        case 0xA02: //iCNLGT2
        //ICL Simulation:
        case 0xFF02: //iICL11HP
        case 0xFF05: //iICL11LP
        case 0xFF15: //iICLCNXG
        //GLV Simulation
        case 0xFF10: //iGLVSIM
        //TGL Simulation
        case 0xFF20: //iTGLSIM
        //LKF Simulation
        case 0x8A40: //iLKFSIM
            g_tsConfig.sim = true;
            g_tsLog << "SIMULATION DETECTED\n";
        default:
            break;
        }

        id = ((id << 8) | (id >> 8));
        g_tsLog << "DeviceID = 0x" << hexstr(&id, 2) << " at adapter #" << adapter << "\n";

        break;
    }
#endif
}
#pragma warning(default:4996)

void MFXVideoTest::TearDown()
{
}

tsStatus::tsStatus()
{
    reset();
}

tsStatus::~tsStatus()
{
}

void tsStatus::reset()
{
    m_status = MFX_ERR_NONE;
    m_expected = MFX_ERR_NONE;
    m_failed = false;
    m_throw_exceptions = true;
    m_last = false;
    m_disable = 0;
}

bool tsStatus::check()
{
    if (m_disable)
    {
        g_tsLog << "CHECK STATUS(disabled, expected " << m_expected << "): " << m_status << "\n";
        if (m_disable == 1)
            m_disable = 0;
        return m_failed = false;
    }

    g_tsLog << "CHECK STATUS(expected " << m_expected << "): " << m_status << " -- ";
    if(m_status != m_expected)
    {
        g_tsLog << "FAILED\n";
        m_expected = MFX_ERR_NONE;
        ADD_FAILURE() << "returned status is wrong";
        if (m_last)
        {
            g_tsLog << "FORCED TEST FINISH!\n";
            disable();
        }
        if(m_throw_exceptions || m_last)
            throw tsFAIL;
        return m_failed = true;
    }
    g_tsLog << "OK\n";
    m_expected = MFX_ERR_NONE;
    if (m_last)
    {
        g_tsLog << "FORCED TEST FINISH!\n";
        disable();
        throw tsOK;
    }
    return m_failed;
}

bool tsStatus::check(mfxStatus status)
{
    m_status = status;
    return check();
}

void GetBufferIdSz(const std::string& name, mfxU32& bufId, mfxU32& bufSz)
{
    //constexpr size_t maxBuffers = g_StringsOfBuffers / g_StringsOfBuffers[0];
    const size_t maxBuffers = sizeof( g_StringsOfBuffers ) / sizeof( g_StringsOfBuffers[0] );

    const std::string& buffer_name = name.substr(0, name.find(":"));

    for(size_t i(0); i < maxBuffers; ++i)
    {
        //if( name.find(g_StringsOfBuffers[i].string) != std::string::npos )
        if( buffer_name == g_StringsOfBuffers[i].string )
        {
            bufId = g_StringsOfBuffers[i].BufferId;
            bufSz = g_StringsOfBuffers[i].BufferSz;

            return;
        }
    }
}

void SetParam(void* base, const std::string name, const mfxU32 offset, const mfxU32 size, mfxU64 value)
{
    memcpy((mfxU8*)base + offset, &value, size);
}

template <typename T>
void SetParam(tsExtBufType<T>& base, const std::string name, const mfxU32 offset, const mfxU32 size, mfxU64 value)
{
    assert(!name.empty() );
    void* ptr = &base;

    if( (typeid(T) == typeid(mfxVideoParam) && ( name.find("mfxVideoParam") == std::string::npos) ) || 
        (typeid(T) == typeid(mfxBitstream)  && ( name.find("mfxBitstream")  == std::string::npos) ) || 
        (typeid(T) == typeid(mfxEncodeCtrl) && ( name.find("mfxEncodeCtrl") == std::string::npos) ) ||
        (typeid(T) == typeid(mfxInitParam) &&  ( name.find("mfxInitParam") == std::string::npos) ) )
    {
        mfxU32 bufId = 0, bufSz = 0;
        GetBufferIdSz(name, bufId, bufSz);
        ptr = base.GetExtBuffer(bufId);
        if(!ptr)
        {
            assert(0 != bufSz);
            base.AddExtBuffer(bufId, bufSz);
            ptr = base.GetExtBuffer(bufId);
        }
    }
    memcpy((mfxU8*)ptr + offset, &value, size);
}

template void SetParam(tsExtBufType<mfxVideoParam>& base, const std::string name, const mfxU32 offset, const mfxU32 size, mfxU64 value);
template void SetParam(tsExtBufType<mfxBitstream>& base, const std::string name, const mfxU32 offset, const mfxU32 size, mfxU64 value);
template void SetParam(tsExtBufType<mfxInitParam>& base, const std::string name, const mfxU32 offset, const mfxU32 size, mfxU64 value);

template <typename T>
bool CompareParam(tsExtBufType<T>& base, const tsStruct::Field& field, mfxU64 value, CompareLog log)
{
    assert(!field.name.empty());
    void* ptr = &base;

    if ((std::is_same<T, mfxVideoParam>::value && (field.name.find("mfxVideoParam") == std::string::npos)) ||
        (std::is_same<T, mfxBitstream>::value  && (field.name.find("mfxBitstream") == std::string::npos)) ||
        (std::is_same<T, mfxEncodeCtrl>::value && (field.name.find("mfxEncodeCtrl") == std::string::npos)) ||
        (std::is_same<T, mfxInitParam>::value  && (field.name.find("mfxInitParam") == std::string::npos)))
    {
        mfxU32 bufId = 0, bufSz = 0;
        GetBufferIdSz(field.name, bufId, bufSz);
        ptr = base.GetExtBuffer(bufId);
        if (ptr==NULL)
        {
            if (log != LOG_NOTHING)
                g_tsLog << "WARNING: CompareParams expectedto find ExtBuf[" << field.name <<"] attached but it is missed\n";
            return false;
        }
    }
    return CompareParam(ptr, field, value);
}

template bool CompareParam(tsExtBufType<mfxVideoParam>& base, const tsStruct::Field& field, mfxU64 value, CompareLog log);
template bool CompareParam(tsExtBufType<mfxBitstream>& base, const tsStruct::Field& field, mfxU64 value, CompareLog log);
template bool CompareParam(tsExtBufType<mfxInitParam>& base, const tsStruct::Field& field, mfxU64 value, CompareLog log);
template bool CompareParam(tsExtBufType<mfxEncodeCtrl>& base, const tsStruct::Field& field, mfxU64 value, CompareLog log);

bool CompareParam(void* base, const tsStruct::Field& field, mfxU64 value)
{
    return memcmp((mfxU8*)base + field.offset, &value, field.size) == 0;
}

template <typename T>
bool CompareParam(tsExtBufType<T> *base, const tsStruct::Field& field, mfxU64 value, CompareLog log)
{
    assert(0 != base);
    if (base)    return CompareParam(*base, field, value, log);
    else return false;
}
