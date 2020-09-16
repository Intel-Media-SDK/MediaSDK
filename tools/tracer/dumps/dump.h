/* ****************************************************************************** *\

Copyright (C) 2012-2020 Intel Corporation.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
- Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
- Neither the name of Intel Corporation nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY INTEL CORPORATION "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL INTEL CORPORATION BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

File Name: dump.h

\* ****************************************************************************** */

#ifndef DUMP_H_
#define DUMP_H_

#include <string>
#include <sstream>
#include <iomanip>
#include <iterator>
#include <typeinfo>
#include "mfxstructures.h"
#include "mfxvideo.h"
#include "mfxplugin.h"
#include "mfxenc.h"
#include "mfxfei.h"
#include "mfxla.h"
#include "mfxvp8.h"
#include "mfxcamera.h"
#include "mfxjpeg.h"
#include "mfxmvc.h"
#include "mfxbrc.h"



std::string pVoidToHexString(void* x);
std::string GetStatusString(mfxStatus sts);
std::string GetmfxIMPL(mfxIMPL impl);
std::string GetFourCC(mfxU32 fourcc);
std::string GetCodecIdString (mfxU32 id);
std::string GetIOPattern (mfxU32 io);
bool _IsBadReadPtr(void *ptr, size_t size);

#define GET_ARRAY_SIZE(x) sizeof(x)/sizeof(x[0])
#define DUMP_RESERVED_ARRAY(r) dump_reserved_array(&(r[0]), GET_ARRAY_SIZE(r))

#define DUMP_FIELD(_field) \
    str += structName + "." #_field "=" + ToString(_struct._field) + "\n";

#define DUMP_FIELD_RESERVED(_field) \
    str += structName + "." #_field "[]=" + DUMP_RESERVED_ARRAY(_struct._field) + "\n";

    #define ToString( x )  static_cast< std::ostringstream const & >( \
        ( std::ostringstream() << std::dec << x ) ).str()

    #define TimeToString( x ) static_cast< std::ostringstream const & >( \
            ( std::ostringstream() << std::left << std::setw(4) << std::dec << x <<" msec") ).str()

    /*
    #define ToHexFormatString( x ) static_cast< std::ostringstream const & >( \
            ( std::ostringstream() << std::hex << x ) ).str()
    */

    #define ToHexFormatString( x ) (static_cast< std::ostringstream const & >( \
            ( std::ostringstream() << std::hex << pVoidToHexString((void*)x) ) ).str() )
/*
#define DEFINE_GET_TYPE(type) \
    template<> \
    inline const char* get_type<type>(){ return #type; };
*/

#define DEFINE_GET_TYPE(type) \
    template<> \
    const char* get_type<type>();

#define DEFINE_GET_TYPE_DEF(type) \
    template<> const char* DumpContext::get_type<type>(){ return #type; }

#define DEFINE_DUMP_FUNCTION(type) \
    std::string dump(const std::string structName, const type & var);

enum eDumpContect {
    DUMPCONTEXT_MFX,
    DUMPCONTEXT_VPP,
    DUMPCONTEXT_ALL,
};

enum eDumpFormat {
    DUMP_DEC,
    DUMP_HEX
};

class DumpContext
{
public:
    eDumpContect context;

    DumpContext(void) {
        context = DUMPCONTEXT_ALL;
    };
    ~DumpContext(void) {};

    template<typename T>
    inline std::string toString( T x, eDumpFormat format = DUMP_DEC){
        return static_cast< std::ostringstream const & >(
            ( std::ostringstream() << ((format == DUMP_DEC) ? std::dec : std::hex) << x )).str();
    }

    const char* get_bufferid_str(mfxU32 bufferid);

    template<typename T>
    std::string dump_reserved_array(T* data, size_t size)
    {
        std::stringstream result;

        result << "{ ";
        for (size_t i = 0; i < size; ++i) {
            result << data[i];
            if (i < (size-1)) result << ", ";
        }
        result << " }";
        return result.str();
    }

    template<typename T0, typename T1>
    std::string dump_array_with_cast(T0* data, size_t size)
    {
        std::stringstream result;

        result << "{ ";
        for (size_t i = 0; i < size; ++i) {
            result << (T1)data[i];
            if (i < (size - 1)) result << ", ";
        }
        result << " }";
        return result.str();
    }

    template<typename T>
    std::string dump_hex_array(T* data, size_t size)
    {
        std::stringstream result;
        result << "{ "  << std::hex << std::uppercase;
        for (size_t i = 0; i < size; ++i)
            result << std::setw(sizeof(T) * 2) << std::setfill('0') << (mfxU64)data[i];
        result << " }";
        return result.str();
    }
    template<typename T, size_t N>
    inline std::string dump_hex_array(T (&data)[N]) { return dump_hex_array(data, N); }


    template<typename T>
    inline const char* get_type(){ return typeid(T).name(); }

    template<typename T>
    std::string dump(const std::string structName, const T *_struct)
    {
        std::string str = get_type<T>();
        str += "* " + structName + "=" + ToHexFormatString(_struct) + "\n";
        if (_struct) str += dump("  " + structName, *_struct);
        return str;
    }
    std::string dump(const std::string structName, const void *_struct)
    {
        std::string str = "void";
        str += "* " + structName + "=" + ToHexFormatString(_struct) + "\n";
        return str;
    }

    template<typename T>
    inline std::string dump_mfxExtParams(const std::string structName, const T& _struct)
    {
        std::string str;
        std::string name;

        str += structName + ".NumExtParam=" + ToString(_struct.NumExtParam) + "\n";
        str += structName + ".ExtParam=" + ToString(_struct.ExtParam) + "\n";

        if (_struct.ExtParam) {
            for (mfxU16 i = 0; i < _struct.NumExtParam; ++i)
            {
                if ((!_IsBadReadPtr(_struct.ExtParam, sizeof(mfxExtBuffer**))) && (!_IsBadReadPtr(_struct.ExtParam[i], sizeof(mfxExtBuffer*))))
                {
                    name = structName + ".ExtParam[" + ToString(i) + "]";
                    str += name + "=" + ToString(_struct.ExtParam[i]) + "\n";
                    switch (_struct.ExtParam[i]->BufferId)
                    {
                        case MFX_EXTBUFF_CODING_OPTION:
                            str += dump(name, *((mfxExtCodingOption*)_struct.ExtParam[i])) + "\n";
                            break;
                        case MFX_EXTBUFF_CODING_OPTION2:
                            str += dump(name, *((mfxExtCodingOption2*)_struct.ExtParam[i])) + "\n";
                            break;
                        case MFX_EXTBUFF_CODING_OPTION3:
                            str += dump(name, *((mfxExtCodingOption3*)_struct.ExtParam[i])) + "\n";
                            break;
                        case MFX_EXTBUFF_ENCODER_RESET_OPTION:
                            str += dump(name, *((mfxExtEncoderResetOption*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUF_CAM_GAMMA_CORRECTION:
                            str += dump(name, *((mfxExtCamGammaCorrection*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUF_CAM_WHITE_BALANCE:
                            str += dump(name, *((mfxExtCamWhiteBalance*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUF_CAM_HOT_PIXEL_REMOVAL:
                            str += dump(name, *((mfxExtCamHotPixelRemoval*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUF_CAM_BLACK_LEVEL_CORRECTION:
                            str += dump(name, *((mfxExtCamBlackLevelCorrection*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUF_CAM_VIGNETTE_CORRECTION:
                            str += dump(name, *((mfxExtCamVignetteCorrection*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUF_CAM_BAYER_DENOISE:
                            str += dump(name, *((mfxExtCamBayerDenoise*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUF_CAM_COLOR_CORRECTION_3X3:
                            str += dump(name, *((mfxExtCamColorCorrection3x3*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUF_CAM_PADDING:
                            str += dump(name, *((mfxExtCamPadding*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUF_CAM_PIPECONTROL:
                            str += dump(name, *((mfxExtCamPipeControl*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUF_CAM_FORWARD_GAMMA_CORRECTION:
                            str += dump(name, *((mfxExtCamFwdGamma*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUF_CAM_LENS_GEOM_DIST_CORRECTION:
                            str += dump(name, *((mfxExtCamLensGeomDistCorrection*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUF_CAM_TOTAL_COLOR_CONTROL:
                            str += dump(name, *((mfxExtCamTotalColorControl*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUF_CAM_CSC_YUV_RGB:
                            str += dump(name, *((mfxExtCamCscYuvRgb*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_LOOKAHEAD_CTRL:
                            str += dump(name, *((mfxExtLAControl*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_LOOKAHEAD_STAT:
                            str += dump(name, *((mfxExtLAFrameStatistics*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_AVC_REFLIST_CTRL:
                            str += dump(name, *((mfxExtAVCRefListCtrl*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_AVC_TEMPORAL_LAYERS:
                            str += dump(name, *((mfxExtAvcTemporalLayers*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_ENCODED_FRAME_INFO:
                            str += dump(name, *((mfxExtAVCEncodedFrameInfo*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_AVC_REFLISTS:
                            str += dump(name, *((mfxExtAVCRefLists*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_JPEG_QT:
                            str += dump(name, *((mfxExtJPEGQuantTables*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_JPEG_HUFFMAN:
                            str += dump(name, *((mfxExtJPEGHuffmanTables*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_MVC_SEQ_DESC:
                            str += dump(name, *((mfxExtMVCSeqDesc*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_MVC_TARGET_VIEWS:
                            str += dump(name, *((mfxExtMVCTargetViews*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_OPAQUE_SURFACE_ALLOCATION:
                            str += dump(name, *((mfxExtOpaqueSurfaceAlloc*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_VPP_DENOISE:
                            str += dump(name, *((mfxExtVPPDenoise*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_VPP_DETAIL:
                            str += dump(name, *((mfxExtVPPDetail*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_VPP_PROCAMP:
                            str += dump(name, *((mfxExtVPPProcAmp*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_CODING_OPTION_SPSPPS:
                            str += dump(name, *((mfxExtCodingOptionSPSPPS*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_VIDEO_SIGNAL_INFO:
                            str += dump(name, *((mfxExtVideoSignalInfo*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_VPP_DOUSE:
                            str += dump(name, *((mfxExtVPPDoUse*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_PICTURE_TIMING_SEI:
                            str += dump(name, *((mfxExtPictureTimingSEI*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_VPP_COMPOSITE:
                            str += dump(name, *((mfxExtVPPComposite*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_VPP_VIDEO_SIGNAL_INFO:
                            str += dump(name, *((mfxExtVPPVideoSignalInfo*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_VPP_DEINTERLACING:
                            str += dump(name, *((mfxExtVPPDeinterlacing*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_HEVC_TILES:
                            str += dump(name, *((mfxExtHEVCTiles*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_HEVC_PARAM:
                            str += dump(name, *((mfxExtHEVCParam*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_HEVC_REGION:
                            str += dump(name, *((mfxExtHEVCRegion*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_DECODED_FRAME_INFO:
                            str += dump(name, *((mfxExtDecodedFrameInfo*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_TIME_CODE:
                            str += dump(name, *((mfxExtTimeCode*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_PRED_WEIGHT_TABLE:
                            str += dump(name, *((mfxExtPredWeightTable*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_ENCODER_CAPABILITY:
                            str += dump(name, *((mfxExtEncoderCapability*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_DIRTY_RECTANGLES:
                            str += dump(name, *((mfxExtDirtyRect*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_MOVING_RECTANGLES:
                            str += dump(name, *((mfxExtMoveRect*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_VPP_FRAME_RATE_CONVERSION:
                            str += dump(name, *((mfxExtVPPFrameRateConversion*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_VPP_IMAGE_STABILIZATION:
                            str += dump(name, *((mfxExtVPPImageStab*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_ENCODER_ROI:
                            str += dump(name, *((mfxExtEncoderROI*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_FEI_PARAM:
                            str += dump(name, *((mfxExtFeiParam*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_FEI_PREENC_CTRL:
                            str += dump(name, *((mfxExtFeiPreEncCtrl*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_FEI_PREENC_MV_PRED:
                            str += dump(name, *((mfxExtFeiPreEncMVPredictors*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_FEI_ENC_QP:
                            str += dump(name, *((mfxExtFeiEncQP*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_FEI_PREENC_MV:
                            str += dump(name, *((mfxExtFeiPreEncMV*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_FEI_PREENC_MB:
                            str += dump(name, *((mfxExtFeiPreEncMBStat*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_FEI_ENC_CTRL:
                            str += dump(name, *((mfxExtFeiEncFrameCtrl*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_FEI_ENC_MV_PRED:
                            str += dump(name, *((mfxExtFeiEncMVPredictors*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_FEI_ENC_MB:
                            str += dump(name, *((mfxExtFeiEncMBCtrl*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_FEI_ENC_MV:
                            str += dump(name, *((mfxExtFeiEncMV*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_FEI_ENC_MB_STAT:
                            str += dump(name, *((mfxExtFeiEncMBStat*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_FEI_PAK_CTRL:
                            str += dump(name, *((mfxFeiPakMBCtrl*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_FEI_SPS:
                            str += dump(name, *((mfxExtFeiSPS*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_FEI_PPS:
                            str += dump(name, *((mfxExtFeiPPS*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_FEI_SLICE:
                            str += dump(name, *((mfxExtFeiSliceHeader*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_CODING_OPTION_VPS:
                            str += dump(name, *((mfxExtCodingOptionVPS*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_VPP_ROTATION:
                            str += dump(name, *(( mfxExtVPPRotation*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_ENCODED_SLICES_INFO:
                            str += dump(name, *((mfxExtEncodedSlicesInfo*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_VPP_SCALING:
                            str += dump(name, *((mfxExtVPPScaling*)_struct.ExtParam[i])) + "\n";
                            break;
                        case  MFX_EXTBUFF_FEI_CODING_OPTION:
                            str += dump(name, *((mfxExtFeiCodingOption*)_struct.ExtParam[i])) + "\n";
                            break;
                        case MFX_EXTBUFF_FEI_DEC_STREAM_OUT:
                            str += dump(name, *((mfxExtFeiDecStreamOut*)_struct.ExtParam[i])) + "\n";
                            break;
                        case MFX_EXTBUFF_FEI_REPACK_CTRL:
                            str += dump(name, *((mfxExtFeiRepackCtrl*)_struct.ExtParam[i])) + "\n";
                            break;
#if (MFX_VERSION >= 1025)
                        case MFX_EXTBUFF_FEI_REPACK_STAT:
                            str += dump(name, *((mfxExtFeiRepackStat*)_struct.ExtParam[i])) + "\n";
                            break;
#endif
                        case MFX_EXTBUFF_VPP_MIRRORING:
                            str += dump(name, *((mfxExtVPPMirroring*)_struct.ExtParam[i])) + "\n";
                            break;
                        case MFX_EXTBUFF_MV_OVER_PIC_BOUNDARIES:
                            str += dump(name, *((mfxExtMVOverPicBoundaries*)_struct.ExtParam[i])) + "\n";
                            break;
                        case MFX_EXTBUFF_VPP_COLORFILL:
                            str += dump(name, *((mfxExtVPPColorFill*)_struct.ExtParam[i])) + "\n";
                            break;
                        case MFX_EXTBUFF_DEC_VIDEO_PROCESSING:
                            str += dump(name, *((mfxExtDecVideoProcessing*)_struct.ExtParam[i])) + "\n";
                            break;
                        case MFX_EXTBUFF_BRC:
                            str += dump(name, *((mfxExtBRC*)_struct.ExtParam[i])) + "\n";
                            break;
                        case MFX_EXTBUFF_MBQP:
                            str += dump(name, *((mfxExtMBQP*)_struct.ExtParam[i])) + "\n";
                            break;
                        case MFX_EXTBUFF_ENCODER_IPCM_AREA:
                            str += dump(name, *((mfxExtEncoderIPCMArea*)_struct.ExtParam[i])) + "\n";
                            break;
                        case MFX_EXTBUFF_INSERT_HEADERS:
                            str += dump(name, *((mfxExtInsertHeaders*)_struct.ExtParam[i])) + "\n";
                            break;
#if (MFX_VERSION >= 1025)
                        case  MFX_EXTBUFF_DECODE_ERROR_REPORT:
                            str += dump(name, *((mfxExtDecodeErrorReport*)_struct.ExtParam[i])) + "\n";
                            break;
                        case MFX_EXTBUFF_MASTERING_DISPLAY_COLOUR_VOLUME:
                            str += dump(name, *((mfxExtMasteringDisplayColourVolume*)_struct.ExtParam[i])) + "\n";
                            break;
                        case MFX_EXTBUFF_CONTENT_LIGHT_LEVEL_INFO:
                            str += dump(name, *((mfxExtContentLightLevelInfo*)_struct.ExtParam[i])) + "\n";
                            break;
                        case MFX_EXTBUFF_MULTI_FRAME_PARAM:
                            str += dump(name, *((mfxExtMultiFrameParam*)_struct.ExtParam[i])) + "\n";
                            break;
                        case MFX_EXTBUFF_MULTI_FRAME_CONTROL:
                            str += dump(name, *((mfxExtMultiFrameControl*)_struct.ExtParam[i])) + "\n";
                            break;
                        case MFX_EXTBUFF_ENCODED_UNITS_INFO:
                            str += dump(name, *((mfxExtEncodedUnitsInfo*)_struct.ExtParam[i])) + "\n";
                            break;
                        case MFX_EXTBUFF_VPP_COLOR_CONVERSION:
                            str += dump(name, *((mfxExtColorConversion*)_struct.ExtParam[i])) + "\n";
                            break;
#endif
#if (MFX_VERSION >= 1026)
                        case MFX_EXTBUFF_VPP_MCTF:
                            str += dump(name, *((mfxExtVppMctf*)_struct.ExtParam[i])) + "\n";
                            break;
#endif

#if (MFX_VERSION >= 1026)
                        case MFX_EXTBUFF_VP9_SEGMENTATION:
                            str += dump(name, *((mfxExtVP9Segmentation*)_struct.ExtParam[i])) + "\n";
                            break;
                        case MFX_EXTBUFF_VP9_TEMPORAL_LAYERS:
                            str += dump(name, *((mfxExtVP9TemporalLayers*)_struct.ExtParam[i])) + "\n";
                            break;
                        case MFX_EXTBUFF_VP9_PARAM:
                            str += dump(name, *((mfxExtVP9Param*)_struct.ExtParam[i])) + "\n";
                            break;
#endif
                        default:
                            str += dump(name, *(_struct.ExtParam[i])) + "\n";
                            break;
                    };
                }
                else
                {
                    str += "WARNING: Can't read from ExtParam[" + ToString(i) + "]!\n";
                    return str;
                }

            }
        }
        return str;
    }

    //mfxdefs
    std::string dump_mfxU32(const std::string structName, mfxU32 u32);
    std::string dump_mfxU64(const std::string structName, mfxU64 u64);
    std::string dump_mfxHDL(const std::string structName, const mfxHDL *hdl);
    std::string dump_mfxStatus(const std::string structName, mfxStatus status);

    //mfxcommon
    DEFINE_DUMP_FUNCTION(mfxBitstream);
    DEFINE_DUMP_FUNCTION(mfxExtBuffer);
    DEFINE_DUMP_FUNCTION(mfxIMPL);
    DEFINE_DUMP_FUNCTION(mfxInitParam);
    DEFINE_DUMP_FUNCTION(mfxPriority);
    DEFINE_DUMP_FUNCTION(mfxVersion);
    DEFINE_DUMP_FUNCTION(mfxSyncPoint);
    DEFINE_DUMP_FUNCTION(mfxExtThreadsParam);
    DEFINE_DUMP_FUNCTION(mfxPlatform);

    //mfxenc
    DEFINE_DUMP_FUNCTION(mfxENCInput);
    DEFINE_DUMP_FUNCTION(mfxENCOutput);

    //mfxplugin
    DEFINE_DUMP_FUNCTION(mfxPlugin);
    DEFINE_DUMP_FUNCTION(mfxCoreParam);
    DEFINE_DUMP_FUNCTION(mfxPluginParam);
    DEFINE_DUMP_FUNCTION(mfxCoreInterface);

    //mfxstructures
    DEFINE_DUMP_FUNCTION(mfxDecodeStat);
    DEFINE_DUMP_FUNCTION(mfxEncodeCtrl);
    DEFINE_DUMP_FUNCTION(mfxEncodeStat);
    DEFINE_DUMP_FUNCTION(mfxExtCodingOption);
    DEFINE_DUMP_FUNCTION(mfxExtCodingOption2);
    DEFINE_DUMP_FUNCTION(mfxExtCodingOption3);
    DEFINE_DUMP_FUNCTION(mfxExtEncoderResetOption);
    DEFINE_DUMP_FUNCTION(mfxExtVppAuxData);
    DEFINE_DUMP_FUNCTION(mfxFrameAllocRequest);
    DEFINE_DUMP_FUNCTION(mfxFrameAllocResponse);
    DEFINE_DUMP_FUNCTION(mfxFrameData);
    DEFINE_DUMP_FUNCTION(mfxFrameId);
    DEFINE_DUMP_FUNCTION(mfxFrameInfo);
    DEFINE_DUMP_FUNCTION(mfxFrameSurface1);
    DEFINE_DUMP_FUNCTION(mfxHandleType);
    DEFINE_DUMP_FUNCTION(mfxInfoMFX);
    DEFINE_DUMP_FUNCTION(mfxInfoVPP);
    DEFINE_DUMP_FUNCTION(mfxPayload);
    DEFINE_DUMP_FUNCTION(mfxSkipMode);
    DEFINE_DUMP_FUNCTION(mfxVideoParam);
    DEFINE_DUMP_FUNCTION(mfxVPPStat);
    DEFINE_DUMP_FUNCTION(mfxExtVPPDoNotUse);
    DEFINE_DUMP_FUNCTION(mfxExtAVCRefListCtrl);
    DEFINE_DUMP_FUNCTION(mfxExtAvcTemporalLayers);
    DEFINE_DUMP_FUNCTION(mfxExtAVCEncodedFrameInfo);
    DEFINE_DUMP_FUNCTION(mfxExtAVCRefLists);
    DEFINE_DUMP_FUNCTION(mfxExtOpaqueSurfaceAlloc);
    DEFINE_DUMP_FUNCTION(mfxExtVPPDenoise);
    DEFINE_DUMP_FUNCTION(mfxExtVPPDetail);
    DEFINE_DUMP_FUNCTION(mfxExtVPPProcAmp);
    DEFINE_DUMP_FUNCTION(mfxExtCodingOptionSPSPPS);
    DEFINE_DUMP_FUNCTION(mfxExtVideoSignalInfo);
    DEFINE_DUMP_FUNCTION(mfxExtVPPDoUse);
    DEFINE_DUMP_FUNCTION(mfxExtPictureTimingSEI);
    DEFINE_DUMP_FUNCTION(mfxVPPCompInputStream);
    DEFINE_DUMP_FUNCTION(mfxExtVPPComposite);
    DEFINE_DUMP_FUNCTION(mfxExtVPPVideoSignalInfo);
    DEFINE_DUMP_FUNCTION(mfxExtVPPDeinterlacing);
    DEFINE_DUMP_FUNCTION(mfxExtHEVCTiles);
    DEFINE_DUMP_FUNCTION(mfxExtHEVCParam);
    DEFINE_DUMP_FUNCTION(mfxExtHEVCRegion);
    DEFINE_DUMP_FUNCTION(mfxExtDecodedFrameInfo);
    DEFINE_DUMP_FUNCTION(mfxExtTimeCode);
    DEFINE_DUMP_FUNCTION(mfxExtPredWeightTable);
    DEFINE_DUMP_FUNCTION(mfxExtEncoderCapability);
    DEFINE_DUMP_FUNCTION(mfxExtDirtyRect);
    DEFINE_DUMP_FUNCTION(mfxExtMoveRect);
    DEFINE_DUMP_FUNCTION(mfxExtVPPFrameRateConversion);
    DEFINE_DUMP_FUNCTION(mfxExtVPPImageStab);
    DEFINE_DUMP_FUNCTION(mfxExtEncoderROI);
    DEFINE_DUMP_FUNCTION(mfxExtCodingOptionVPS);
    DEFINE_DUMP_FUNCTION(mfxExtVPPRotation);
    DEFINE_DUMP_FUNCTION(mfxExtEncodedSlicesInfo);
    DEFINE_DUMP_FUNCTION(mfxExtVPPScaling);
    DEFINE_DUMP_FUNCTION(mfxExtVPPMirroring);
    DEFINE_DUMP_FUNCTION(mfxExtMVOverPicBoundaries);
    DEFINE_DUMP_FUNCTION(mfxExtVPPColorFill);
    DEFINE_DUMP_FUNCTION(mfxExtDecVideoProcessing);
    DEFINE_DUMP_FUNCTION(mfxExtMBQP);
    DEFINE_DUMP_FUNCTION(mfxExtEncoderIPCMArea);
    DEFINE_DUMP_FUNCTION(mfxExtInsertHeaders);
#if (MFX_VERSION >= 1025)
    DEFINE_DUMP_FUNCTION(mfxExtDecodeErrorReport);
    DEFINE_DUMP_FUNCTION(mfxExtMasteringDisplayColourVolume);
    DEFINE_DUMP_FUNCTION(mfxExtContentLightLevelInfo);
    DEFINE_DUMP_FUNCTION(mfxExtMultiFrameParam);
    DEFINE_DUMP_FUNCTION(mfxExtMultiFrameControl);
    DEFINE_DUMP_FUNCTION(mfxExtEncodedUnitsInfo);
    DEFINE_DUMP_FUNCTION(mfxExtColorConversion);
#endif
#if (MFX_VERSION >= 1026)
    DEFINE_DUMP_FUNCTION(mfxExtVppMctf);
#endif

#if (MFX_VERSION >= 1026)

    DEFINE_DUMP_FUNCTION(mfxVP9SegmentParam);
    DEFINE_DUMP_FUNCTION(mfxExtVP9Segmentation);
    DEFINE_DUMP_FUNCTION(mfxVP9TemporalLayer);
    DEFINE_DUMP_FUNCTION(mfxExtVP9TemporalLayers);
    DEFINE_DUMP_FUNCTION(mfxExtVP9Param);
#endif

#if (MFX_VERSION >= 1034)
    DEFINE_DUMP_FUNCTION(mfxExtAV1FilmGrainParam);
#endif

    //mfxsession
    DEFINE_DUMP_FUNCTION(mfxSession);

    //mfxvideo
    DEFINE_DUMP_FUNCTION(mfxBufferAllocator);
    DEFINE_DUMP_FUNCTION(mfxFrameAllocator);

    //mfxla
    DEFINE_DUMP_FUNCTION(mfxExtLAControl);
    DEFINE_DUMP_FUNCTION(mfxExtLAControl::mfxStream);
    DEFINE_DUMP_FUNCTION(mfxLAFrameInfo);
    DEFINE_DUMP_FUNCTION(mfxExtLAFrameStatistics);

    //mfxfei
    DEFINE_DUMP_FUNCTION(mfxExtFeiPreEncCtrl);
    DEFINE_DUMP_FUNCTION(mfxExtFeiPreEncMVPredictors);
    DEFINE_DUMP_FUNCTION(mfxExtFeiPreEncMV);
    DEFINE_DUMP_FUNCTION(mfxExtFeiPreEncMBStat);
    DEFINE_DUMP_FUNCTION(mfxExtFeiEncFrameCtrl);
    DEFINE_DUMP_FUNCTION(mfxExtFeiEncMVPredictors);
    DEFINE_DUMP_FUNCTION(mfxExtFeiEncMVPredictors::mfxExtFeiEncMVPredictorsMB);
    DEFINE_DUMP_FUNCTION(mfxExtFeiEncQP);
    DEFINE_DUMP_FUNCTION(mfxExtFeiEncMBCtrl);
    DEFINE_DUMP_FUNCTION(mfxExtFeiEncMBCtrl::mfxExtFeiEncMBCtrlMB);
    DEFINE_DUMP_FUNCTION(mfxExtFeiEncMV);
    DEFINE_DUMP_FUNCTION(mfxExtFeiEncMBStat);
    DEFINE_DUMP_FUNCTION(mfxExtFeiEncMBStat::mfxExtFeiEncMBStatMB);
    DEFINE_DUMP_FUNCTION(mfxFeiPakMBCtrl);
    DEFINE_DUMP_FUNCTION(mfxExtFeiPakMBCtrl);
    DEFINE_DUMP_FUNCTION(mfxExtFeiRepackCtrl);
#if (MFX_VERSION >= 1025)
    DEFINE_DUMP_FUNCTION(mfxExtFeiRepackStat);
#endif
    DEFINE_DUMP_FUNCTION(mfxExtFeiParam);
    DEFINE_DUMP_FUNCTION(mfxPAKInput);
    DEFINE_DUMP_FUNCTION(mfxPAKOutput);
    DEFINE_DUMP_FUNCTION(mfxExtFeiSPS);
    DEFINE_DUMP_FUNCTION(mfxExtFeiPPS);
    DEFINE_DUMP_FUNCTION(mfxExtFeiSliceHeader);
    DEFINE_DUMP_FUNCTION(mfxExtFeiSliceHeader::mfxSlice);
    DEFINE_DUMP_FUNCTION(mfxExtFeiCodingOption);
    DEFINE_DUMP_FUNCTION(mfxExtFeiDecStreamOut);
    DEFINE_DUMP_FUNCTION(mfxFeiDecStreamOutMBCtrl);


    //mfxvp8
    DEFINE_DUMP_FUNCTION(mfxExtVP8CodingOption)

    //mfxcamera
    DEFINE_DUMP_FUNCTION(mfxExtCamGammaCorrection);
    DEFINE_DUMP_FUNCTION(mfxExtCamWhiteBalance);
    DEFINE_DUMP_FUNCTION(mfxExtCamHotPixelRemoval);
    DEFINE_DUMP_FUNCTION(mfxExtCamBlackLevelCorrection);
    DEFINE_DUMP_FUNCTION(mfxCamVignetteCorrectionParam);
    DEFINE_DUMP_FUNCTION(mfxExtCamVignetteCorrection);
    DEFINE_DUMP_FUNCTION(mfxExtCamBayerDenoise);
    DEFINE_DUMP_FUNCTION(mfxExtCamColorCorrection3x3);
    DEFINE_DUMP_FUNCTION(mfxExtCamPadding);
    DEFINE_DUMP_FUNCTION(mfxExtCamPipeControl);
    DEFINE_DUMP_FUNCTION(mfxCamFwdGammaSegment);
    DEFINE_DUMP_FUNCTION(mfxExtCamFwdGamma);
    DEFINE_DUMP_FUNCTION(mfxExtCamLensGeomDistCorrection);
    DEFINE_DUMP_FUNCTION(mfxExtCamTotalColorControl);
    DEFINE_DUMP_FUNCTION(mfxExtCamCscYuvRgb);

    //mfxjpeg
    DEFINE_DUMP_FUNCTION(mfxExtJPEGQuantTables);
    DEFINE_DUMP_FUNCTION(mfxExtJPEGHuffmanTables);

    //mfxmvc
    DEFINE_DUMP_FUNCTION(mfxMVCViewDependency);
    DEFINE_DUMP_FUNCTION(mfxMVCOperationPoint);
    DEFINE_DUMP_FUNCTION(mfxExtMVCSeqDesc);
    DEFINE_DUMP_FUNCTION(mfxExtMVCTargetViews);

    //mfxbrc
    DEFINE_DUMP_FUNCTION(mfxExtBRC);
    DEFINE_DUMP_FUNCTION(mfxBRCFrameParam);
    DEFINE_DUMP_FUNCTION(mfxBRCFrameCtrl);
    DEFINE_DUMP_FUNCTION(mfxBRCFrameStatus);

#if (MFX_VERSION >= MFX_VERSION_NEXT)
    //custom scaling matrices
    DEFINE_DUMP_FUNCTION(mfxExtAVCScalingMatrix);
#endif

#if (MFX_VERSION >= MFX_VERSION_NEXT)
    //partial output
    DEFINE_DUMP_FUNCTION(mfxExtPartialBitstreamParam);
#endif
};
#endif //DUMP_H_

