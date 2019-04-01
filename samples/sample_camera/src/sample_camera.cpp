/******************************************************************************\
Copyright (c) 2005-2019, Intel Corporation
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
#include "pipeline_camera.h"
#include <sstream>
#include "version.h"

void PrintHelp(msdk_char *strAppName, const msdk_char *strErrorMessage)
{
    msdk_printf(MSDK_STRING("Intel(R) Camera Expert SDK Sample Version %s\n\n"), GetMSDKSampleVersion().c_str());

    if (strErrorMessage)
    {
        msdk_printf(MSDK_STRING("Error: %s\n"), strErrorMessage);
    }

    msdk_printf(MSDK_STRING("Usage: %s [Options] -i InputFileNameBase -o OutputFileNameBase [numberOfFilesToDump]\n"), strAppName);
    msdk_printf(MSDK_STRING("Options: \n"));
    msdk_printf(MSDK_STRING("   [-plugin_version ver]                                - define camera plugin version\n"));
    msdk_printf(MSDK_STRING("   [-a asyncDepth] / [-asyncDepth asyncDepth]          - set async depth, default %d \n"), CAM_SAMPLE_ASYNC_DEPTH);
    msdk_printf(MSDK_STRING("   [-b bitDepth] / [-bitDepth bitDepth]                - set bit depth, default 10 \n"));
    msdk_printf(MSDK_STRING("   [-f format] / [-format format]                      - input Bayer format: rggb, bggr, grbg or gbrg\n"));
    msdk_printf(MSDK_STRING("   [-of format] / [-outFormat format]                  - output format: of argb16 or 16 meaning 16 bit ARGB, 8 bit ARGB, NV12\n"));
    msdk_printf(MSDK_STRING("   [-offset pre1 pre2 pre3 post1 post2 post3]          - In case of NV12 output, offset set color pre and post offsets\n"));
    msdk_printf(MSDK_STRING("   [-3DLUT_gamma]                                      - use 3D LUT gamma correction\n"));
    msdk_printf(MSDK_STRING("   [-ng] / [-noGamma]                                  - no gamma correction\n"));
    msdk_printf(MSDK_STRING("   [-gamma_points]                                     - set specific gamma points (64 points expected)\n"));
    msdk_printf(MSDK_STRING("   [-gamma_corrected]                                  - set specific gamma corrected values (64 values expected)\n"));
    msdk_printf(MSDK_STRING("                                                           -gamma_points and -gamma_corrected options must be used together\n"));
    msdk_printf(MSDK_STRING("   [-bdn threshold] / [-bayerDenoise threshold]        - bayer denoise on\n"));
    msdk_printf(MSDK_STRING("   [-hot_pixel Diff Num]                               - bayer hot pixel removal\n"));
    msdk_printf(MSDK_STRING("   [-bbl B G0 G1 R] / [-bayerBlackLevel B G0 G1 R]     - bayer black level correction\n"));
    msdk_printf(MSDK_STRING("   [-tcc R G B C M Y] / [-totalColorControl R G B C M Y]  - total color control \n"));
    msdk_printf(MSDK_STRING("   [-bwb B G0 G1 R] / [-bayerWhiteBalance B G0 G1 R]   - bayer white balance\n"));
    msdk_printf(MSDK_STRING("   [-ccm n00 n01 ... n33 ]                             - color correction 3x3 matrix\n"));
    msdk_printf(MSDK_STRING("   [-vignette maskfile ]                               - enable vignette correction using mask from specified file\n"));
    msdk_printf(MSDK_STRING("   [-lens a b c d ]                                    - enable lens geometry distortion correction\n"));
    msdk_printf(MSDK_STRING("   [-chroma_aberration aR bR cR dR aG bG cG dG aB bB cB dB ] - enable chroma aberration correction\n"));
    msdk_printf(MSDK_STRING("   [-w width] / [-width width]                         - input width, default 4096\n"));
    msdk_printf(MSDK_STRING("   [-h height] / [-height height]                      - input height, default 2160\n"));
    msdk_printf(MSDK_STRING("   [-n numFrames] / [-numFramesToProcess numFrames]    - number of frames to process\n"));
    msdk_printf(MSDK_STRING("   [-alpha alpha]                                      - write value to alpha channel of output surface \n"));
    msdk_printf(MSDK_STRING("   [-pd] / [-padding]                                  - do input surface padding \n"));
    msdk_printf(MSDK_STRING("   [-perf_opt]                                         - buffered reading of input (support: Bayer sequence \n"));
    msdk_printf(MSDK_STRING("   [-resetInterval resetInterval]                      - reset interval in frames, default 7 \n"));
    msdk_printf(MSDK_STRING("   [-reset -i ... -o ... -f ... -w ... -h ... -bbl ... -bwb ... -ccm ...]     -  params to be used after next reset.\n"));
    msdk_printf(MSDK_STRING("       Only params listed above are supported, if a param is not set here, the originally set value is used. \n"));
    msdk_printf(MSDK_STRING("       There can be any number of resets, applied in order of appearance in the command line, \n"));
    msdk_printf(MSDK_STRING("           after resetInterval frames are processed with the current params  \n"));
    msdk_printf(MSDK_STRING("   [-imem]                                              - input memory type (sys|video). Default is system\n"));
#if D3D_SURFACES_SUPPORT
    msdk_printf(MSDK_STRING("   [-omem]                                              - output memory type (sys|video). Default is system\n"));
    msdk_printf(MSDK_STRING("   [-accel]                                             - type of acceleration device (d3d9|d3d11). Default is d3d9\n"));
    msdk_printf(MSDK_STRING("   [-r]  / [-render]                                    - render output in a separate window \n"));
    msdk_printf(MSDK_STRING("   [-wall w h n m f t tmo]                              - same as -r, and positioned rendering window in a particular cell on specific monitor \n"));
    msdk_printf(MSDK_STRING("       w               - number of columns of video windows on selected monitor\n"));
    msdk_printf(MSDK_STRING("       h               - number of rows of video windows on selected monitor\n"));
    msdk_printf(MSDK_STRING("       n(0,.,w*h-1)    - order of video window in table that will be rendered\n"));
    msdk_printf(MSDK_STRING("       m(0,1..)        - monitor id \n"));
    msdk_printf(MSDK_STRING("       f               - rendering framerate\n"));
    msdk_printf(MSDK_STRING("       t(0/1)          - enable/disable window's title\n"));
    msdk_printf(MSDK_STRING("       tmo             - timeout for -wall option\n"));
#endif
#if defined(_WIN32) || defined(_WIN64)
    msdk_printf(MSDK_STRING("\nFeatures: \n"));
    msdk_printf(MSDK_STRING("   Press 1 to toggle fullscreen rendering on/off\n"));
#endif
    msdk_printf(MSDK_STRING("\n"));
}

#define GET_OPTION_POINTER(PTR)        \
{                                      \
    if (2 == msdk_strlen(strInput[i]))     \
    {                                  \
        i++;                           \
        if (strInput[i][0] == MSDK_CHAR('-')) \
        {                              \
            i = i - 1;                 \
        }                              \
        else                           \
        {                              \
            PTR = strInput[i];         \
        }                              \
    }                                  \
    else                               \
    {                                  \
        PTR = strInput[i] + 2;         \
    }                                  \
}

mfxStatus ParseInputString(msdk_char* strInput[], mfxU8 nArgNum, sInputParams* pParams)
{
    if (1 == nArgNum)
    {
        PrintHelp(strInput[0], NULL);
        return MFX_ERR_UNSUPPORTED;
    }

    sResetParams resPar;

    MSDK_CHECK_POINTER(pParams, MFX_ERR_NULL_PTR);

    for (mfxU8 i = 1; i < nArgNum; i++)
    {
        // multi-character options
        if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-plugin_version")))
        {
            msdk_opt_read(strInput[++i], pParams->CameraPluginVersion);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-accel")))
        {
            if(i + 1 >= nArgNum)
            {
                PrintHelp(strInput[0], MSDK_STRING("Not enough parameters for -accel key"));
                return MFX_ERR_UNSUPPORTED;
        }

            if (0 == msdk_strcmp(strInput[i+1], MSDK_STRING("d3d9")))
        {
                pParams->accelType = D3D9;
        }
            else if (0 == msdk_strcmp(strInput[i+1], MSDK_STRING("d3d11")))
        {
                pParams->accelType = D3D11;
            }
            else
            {
                PrintHelp(strInput[0], MSDK_STRING("Unsupported value for -accel key"));
                return MFX_ERR_UNSUPPORTED;
            }
            i++;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-perf_opt")))
        {
            pParams->bPerf_opt = true;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-imem")))
        {
            if(i + 1 >= nArgNum)
            {
                PrintHelp(strInput[0], MSDK_STRING("Not enough parameters for -imem key"));
                return MFX_ERR_UNSUPPORTED;
            }
            if (0 == msdk_strcmp(strInput[i+1], MSDK_STRING("system")))
            {
                pParams->memTypeIn = SYSTEM;
        }
            else if (0 == msdk_strcmp(strInput[i+1], MSDK_STRING("video")))
        {
                pParams->memTypeIn = VIDEO;
            }
            else
            {
                PrintHelp(strInput[0], MSDK_STRING("Unsupported value for -imem key"));
                return MFX_ERR_UNSUPPORTED;
            }
            i++;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-omem")))
        {
            if(i + 1 >= nArgNum)
            {
                PrintHelp(strInput[0], MSDK_STRING("Not enough parameters for -omem key"));
                return MFX_ERR_UNSUPPORTED;
            }
            if (0 == msdk_strcmp(strInput[i+1], MSDK_STRING("system")))
            {
                pParams->memTypeOut = SYSTEM;
        }
            else if (0 == msdk_strcmp(strInput[i+1], MSDK_STRING("video")))
        {
                pParams->memTypeOut = VIDEO;
            }
            else
            {
                PrintHelp(strInput[0], MSDK_STRING("Unsupported value for -omem key"));
                return MFX_ERR_UNSUPPORTED;
            }
            i++;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-r")) || 0 == msdk_strcmp(strInput[i], MSDK_STRING("-render")))
        {
            pParams->bRendering = true;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-a")) || 0 == msdk_strcmp(strInput[i], MSDK_STRING("-asyncDepth")))
        {
            msdk_opt_read(strInput[++i], pParams->asyncDepth);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-n")) || 0 == msdk_strcmp(strInput[i], MSDK_STRING("-numFramesToProcess")))
        {
            msdk_opt_read(strInput[++i], pParams->nFramesToProceed);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-ng")) || 0 == msdk_strcmp(strInput[i], MSDK_STRING("-noGamma")))
        {
            pParams->bGamma = false;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-offset")))
        {

            if (i + 6>= nArgNum)
            {
                PrintHelp(strInput[0], MSDK_STRING("Not enough parameters for offset key"));
                return MFX_ERR_UNSUPPORTED;
            }
            pParams->offset = true;

            msdk_opt_read(strInput[++i], pParams->pre[0]);
            msdk_opt_read(strInput[++i], pParams->pre[1]);
            msdk_opt_read(strInput[++i], pParams->pre[2]);
            msdk_opt_read(strInput[++i], pParams->post[0]);
            msdk_opt_read(strInput[++i], pParams->post[1]);
            msdk_opt_read(strInput[++i], pParams->post[2]);

        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-3DLUT_gamma")))
        {
            pParams->b3DLUTGamma = false;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-bdn")) || 0 == msdk_strcmp(strInput[i], MSDK_STRING("-bayerDenoise")))
        {
            if(i + 1 >= nArgNum)
            {
                PrintHelp(strInput[0], MSDK_STRING("Not enough parameters for -bdn key"));
                return MFX_ERR_UNSUPPORTED;
            }
            pParams->bBayerDenoise = true;
            msdk_opt_read(strInput[++i], pParams->denoiseThreshold);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-bbl")) || 0 == msdk_strcmp(strInput[i], MSDK_STRING("-bayerBlackLevel")))
        {
            if(i + 4 >= nArgNum)
            {
                PrintHelp(strInput[0], MSDK_STRING("Not enough parameters for -bbl key"));
                return MFX_ERR_UNSUPPORTED;
            }
            pParams->bBlackLevel = true;
            msdk_opt_read(strInput[++i], pParams->black_level_B);
            msdk_opt_read(strInput[++i], pParams->black_level_G0);
            msdk_opt_read(strInput[++i], pParams->black_level_G1);
            msdk_opt_read(strInput[++i], pParams->black_level_R);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-tcc")) || 0 == msdk_strcmp(strInput[i], MSDK_STRING("-totalColorControl"))) {
            if (i + 6 >= nArgNum)
            {
                PrintHelp(strInput[0], MSDK_STRING("Not enough parameters for -tcc key"));
                return MFX_ERR_UNSUPPORTED;
            }
            pParams->bTCC = true;
            msdk_opt_read(strInput[++i], pParams->tcc_red);
            msdk_opt_read(strInput[++i], pParams->tcc_green);
            msdk_opt_read(strInput[++i], pParams->tcc_blue);
            msdk_opt_read(strInput[++i], pParams->tcc_cyan);
            msdk_opt_read(strInput[++i], pParams->tcc_magenta);
            msdk_opt_read(strInput[++i], pParams->tcc_yellow);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-hot_pixel")))
        {
            if(i + 2 >= nArgNum)
            {
                PrintHelp(strInput[0], MSDK_STRING("Not enough parameters for -hot_pixel key"));
                return MFX_ERR_UNSUPPORTED;
            }
            pParams->bHP = true;
            msdk_opt_read(strInput[++i], pParams->hp_diff);
            msdk_opt_read(strInput[++i], pParams->hp_num);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-bwb")) || 0 == msdk_strcmp(strInput[i], MSDK_STRING("-bayerWhiteBalance")))
        {
            if(i + 4 >= nArgNum)
            {
                PrintHelp(strInput[0], MSDK_STRING("Not enough parameters for -bwb key"));
                return MFX_ERR_UNSUPPORTED;
            }
            pParams->bWhiteBalance = true;
            msdk_opt_read(strInput[++i], pParams->white_balance_B);
            msdk_opt_read(strInput[++i], pParams->white_balance_G0);
            msdk_opt_read(strInput[++i], pParams->white_balance_G1);
            msdk_opt_read(strInput[++i], pParams->white_balance_R);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-lens")) )
        {
            if(i + 4 >= nArgNum)
            {
                PrintHelp(strInput[0], MSDK_STRING("Not enough parameters for -lens key"));
                return MFX_ERR_UNSUPPORTED;
            }
            pParams->bLens = true;
            msdk_opt_read(strInput[++i], pParams->lens_aR);
            msdk_opt_read(strInput[++i], pParams->lens_bR);
            msdk_opt_read(strInput[++i], pParams->lens_cR);
            msdk_opt_read(strInput[++i], pParams->lens_dR);
            pParams->lens_aB = pParams->lens_aG = pParams->lens_aR;
            pParams->lens_bB = pParams->lens_bG = pParams->lens_bR;
            pParams->lens_cB = pParams->lens_cG = pParams->lens_cR;
            pParams->lens_dB = pParams->lens_dG = pParams->lens_dR;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-chroma_aberration")) )
        {
            if(i + 12 >= nArgNum)
            {
                PrintHelp(strInput[0], MSDK_STRING("Not enough parameters for -chroma_aberration key"));
                return MFX_ERR_UNSUPPORTED;
            }
            pParams->bLens = true;
            msdk_opt_read(strInput[++i], pParams->lens_aR);
            msdk_opt_read(strInput[++i], pParams->lens_bR);
            msdk_opt_read(strInput[++i], pParams->lens_cR);
            msdk_opt_read(strInput[++i], pParams->lens_dR);
            msdk_opt_read(strInput[++i], pParams->lens_aB);
            msdk_opt_read(strInput[++i], pParams->lens_bB);
            msdk_opt_read(strInput[++i], pParams->lens_cB);
            msdk_opt_read(strInput[++i], pParams->lens_dB);
            msdk_opt_read(strInput[++i], pParams->lens_aG);
            msdk_opt_read(strInput[++i], pParams->lens_bG);
            msdk_opt_read(strInput[++i], pParams->lens_cG);
            msdk_opt_read(strInput[++i], pParams->lens_dG);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-ccm")))
        {
            if(i + 9 >= nArgNum)
            {
                PrintHelp(strInput[0], MSDK_STRING("Not enough parameters for -ccm key."));
                return MFX_ERR_UNSUPPORTED;
            }
            pParams->bCCM = true;
            for(int k = 0; k < 3; k++)
                for (int z = 0; z < 3; z++)
                    msdk_opt_read(strInput[++i], pParams->CCM[k][z]);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-gamma_points")))
        {
            if(i + 64 >= nArgNum)
            {
                PrintHelp(strInput[0], MSDK_STRING("There should be 64 points provided."));
                return MFX_ERR_UNSUPPORTED;
            }
            for(int k = 0; k < 64; k++)
                msdk_opt_read(strInput[++i],  pParams->gamma_point[k]);

            pParams->bExternalGammaLUT = true;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-gamma_corrected")))
        {
            if(i + 64 >= nArgNum)
            {
                PrintHelp(strInput[0], MSDK_STRING("There should be 64 points provided."));
                return MFX_ERR_UNSUPPORTED;
            }
            for(int k = 0; k < 64; k++)
                msdk_opt_read(strInput[++i],  pParams->gamma_corrected[k]);

            pParams->bExternalGammaLUT = true;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-pd")) || 0 == msdk_strcmp(strInput[i], MSDK_STRING("-padding")))
        {
            pParams->bDoPadding = true;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-vignette")))
        {
            pParams->bVignette = true;
            msdk_strcopy(pParams->strVignetteMaskFile, strInput[++i]);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-i")))
        {
            msdk_strcopy(pParams->strSrcFile, strInput[++i]);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-o")))
        {
            msdk_strcopy(pParams->strDstFile, strInput[++i]);
            pParams->bOutput = true;
            if (i + 1 < nArgNum)  {
                int n;
                if (msdk_opt_read(strInput[++i], n) == MFX_ERR_NONE) {
                    pParams->maxNumBmpFiles = n;
                }
            }
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-f")) || 0 == msdk_strcmp(strInput[i], MSDK_STRING("-format")))
        {
            i++;
            if (0 == msdk_strcmp(strInput[i], MSDK_STRING("bggr")))
                pParams->inputType     = MFX_CAM_BAYER_BGGR;
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("rggb")))
                pParams->inputType     = MFX_CAM_BAYER_RGGB;
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("grbg")))
                pParams->inputType     = MFX_CAM_BAYER_GRBG;
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("gbrg")))
                pParams->inputType     = MFX_CAM_BAYER_GBRG;
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("argb16")))
                pParams->inputType     = MFX_FOURCC_ARGB16;
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("abgr16")))
                pParams->inputType     = MFX_FOURCC_ABGR16;
            else
            {
                PrintHelp(strInput[0], MSDK_STRING("Format %s is unknown."));
                return MFX_ERR_UNSUPPORTED;
            }

        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-b")) || 0 == msdk_strcmp(strInput[i], MSDK_STRING("-bitDepth")))
        {
            msdk_opt_read(strInput[++i], pParams->bitDepth);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-of")) || 0 == msdk_strcmp(strInput[i], MSDK_STRING("-outFormat")))
        {
            i++;
            if (0 == msdk_strcmp(strInput[i], MSDK_STRING("argb16")) || 0 == msdk_strcmp(strInput[i], MSDK_STRING("16")))
                pParams->frameInfo[VPP_OUT].FourCC = MFX_FOURCC_ARGB16;
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("abgr16")))
                pParams->frameInfo[VPP_OUT].FourCC = MFX_FOURCC_ABGR16;
#if MFX_VERSION >= 1023
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("nv12"))) {
                pParams->frameInfo[VPP_OUT].FourCC = MFX_FOURCC_NV12;
                pParams->bRGBToYUV = true;
            }
#endif
            else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("rgb4")))
                pParams->frameInfo[VPP_OUT].FourCC = MFX_FOURCC_RGB4;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-w")) || 0 == msdk_strcmp(strInput[i], MSDK_STRING("-width")))
        {
            msdk_opt_read(strInput[++i], pParams->frameInfo[VPP_IN].nWidth);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-3dlut")))
        {
            pParams->b3DLUT = true;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-h")) || 0 == msdk_strcmp(strInput[i], MSDK_STRING("-height")))
        {
            msdk_opt_read(strInput[++i], pParams->frameInfo[VPP_IN].nHeight);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-cropW")))
        {
            msdk_opt_read(strInput[++i], pParams->frameInfo[VPP_IN].CropW);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-cropH")))
        {
            msdk_opt_read(strInput[++i], pParams->frameInfo[VPP_IN].CropH);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-cropX")))
        {
            msdk_opt_read(strInput[++i], pParams->frameInfo[VPP_IN].CropX);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-cropY")))
        {
            msdk_opt_read(strInput[++i], pParams->frameInfo[VPP_IN].CropY);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-alpha")))
        {
            msdk_opt_read(strInput[++i], pParams->alphaValue);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-reset")))
        {
            resPar.inputType  = pParams->inputType;
            msdk_strcopy(resPar.strSrcFile, pParams->strSrcFile);
            msdk_strcopy(resPar.strDstFile, pParams->strDstFile);
            resPar.width = pParams->frameInfo[VPP_IN].nWidth;
            resPar.height = pParams->frameInfo[VPP_IN].nHeight;
            resPar.bHP     = pParams->bHP;
            resPar.hp_diff = pParams->hp_diff;
            resPar.hp_num  = pParams->hp_num;

            resPar.bBlackLevel    = pParams->bBlackLevel;
            resPar.black_level_B  = pParams->black_level_B;
            resPar.black_level_G0 = pParams->black_level_G0;
            resPar.black_level_G1 = pParams->black_level_G1;
            resPar.black_level_R  = pParams->black_level_R;
            i++;
            for (;i < nArgNum; i++)
            {
                if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-f")))
                {
                    i++;
                    if (0 == msdk_strcmp(strInput[i], MSDK_STRING("bggr")))
                        resPar.inputType     = MFX_CAM_BAYER_BGGR;
                    else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("rggb")))
                        resPar.inputType     = MFX_CAM_BAYER_RGGB;
                    else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("grbg")))
                        resPar.inputType     = MFX_CAM_BAYER_GRBG;
                    else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("gbrg")))
                        resPar.inputType     = MFX_CAM_BAYER_GBRG;
                }
                else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-bbl")))
                {
                    if(i + 4 >= nArgNum)
                    {
                        PrintHelp(strInput[0], MSDK_STRING("Not enough parameters for -bbl key"));
                        return MFX_ERR_UNSUPPORTED;
                    }
                    resPar.bBlackLevel = true;
                    msdk_opt_read(strInput[++i], resPar.black_level_B);
                    msdk_opt_read(strInput[++i], resPar.black_level_G0);
                    msdk_opt_read(strInput[++i], resPar.black_level_G1);
                    msdk_opt_read(strInput[++i], resPar.black_level_R);
                } else if(0 == msdk_strcmp(strInput[i], MSDK_STRING("-tcc")) || 0 == msdk_strcmp(strInput[i], MSDK_STRING("-totalColorControl"))) {
                    if (i + 6 >= nArgNum)
                    {
                        PrintHelp(strInput[0], MSDK_STRING("Not enough parameters for -tcc key"));
                        return MFX_ERR_UNSUPPORTED;
                    }
                    pParams->bTCC = true;
                    msdk_opt_read(strInput[++i], pParams->tcc_red);
                    msdk_opt_read(strInput[++i], pParams->tcc_green);
                    msdk_opt_read(strInput[++i], pParams->tcc_blue);
                    msdk_opt_read(strInput[++i], pParams->tcc_cyan);
                    msdk_opt_read(strInput[++i], pParams->tcc_magenta);
                    msdk_opt_read(strInput[++i], pParams->tcc_yellow);
                }
                else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-hot_pixel")))
                {
                    if(i + 2 >= nArgNum)
                    {
                        PrintHelp(strInput[0], MSDK_STRING("Not enough parameters for -hot_pixel key"));
                        return MFX_ERR_UNSUPPORTED;
                    }
                    resPar.bHP = true;
                    msdk_opt_read(strInput[++i], resPar.hp_diff);
                    msdk_opt_read(strInput[++i], resPar.hp_num);
                }
                else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-bdn")))
                {
                    if(i + 1 >= nArgNum)
                    {
                        PrintHelp(strInput[0], MSDK_STRING("Not enough parameters for -bdn key"));
                        return MFX_ERR_UNSUPPORTED;
                    }
                    resPar.bDenoise = true;
                    msdk_opt_read(strInput[++i], resPar.denoiseThreshold);
                }
                else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-bwb")))
                {
                    if(i + 4 >= nArgNum)
                    {
                        PrintHelp(strInput[0], MSDK_STRING("Not enough parameters for -bwb key"));
                        return MFX_ERR_UNSUPPORTED;
                    }
                    resPar.bWhiteBalance = true;
                    msdk_opt_read(strInput[++i], resPar.white_balance_B);
                    msdk_opt_read(strInput[++i], resPar.white_balance_G0);
                    msdk_opt_read(strInput[++i], resPar.white_balance_G1);
                    msdk_opt_read(strInput[++i], resPar.white_balance_R);
                }
                else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-ccm")))
                {
                    if(i + 9 >= nArgNum)
                    {
                        PrintHelp(strInput[0], MSDK_STRING("Not enough parameters for -ccm key."));
                        return MFX_ERR_UNSUPPORTED;
                    }
                    resPar.bCCM = true;
                    for(int k = 0; k < 3; k++)
                        for (int z = 0; z < 3; z++)
                            msdk_opt_read(strInput[++i], resPar.CCM[k][z]);
                }
                else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-w")))
                {
                    msdk_opt_read(strInput[++i], resPar.width);
                }
                else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-h")))
                {
                    msdk_opt_read(strInput[++i], resPar.height);
                }
                else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-cropW")))
                {
                    msdk_opt_read(strInput[++i], resPar.cropW);
                }
                else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-cropH")))
                {
                    msdk_opt_read(strInput[++i], resPar.cropH);
                }
                else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-cropX")))
                {
                    msdk_opt_read(strInput[++i], resPar.cropX);
                }
                else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-cropY")))
                {
                    msdk_opt_read(strInput[++i], resPar.cropY);
                }
                else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-vignette")))
                {
                    resPar.bVignette = true;
                    msdk_strcopy(resPar.strVignetteMaskFile, strInput[++i]);
                }
                else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-i")))
                {
                    msdk_strcopy(resPar.strSrcFile, strInput[++i]);
                }
                else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-o")))
                {
                    msdk_strcopy(resPar.strDstFile, strInput[++i]);
                }
                else
                {
                    i--;
                    break;
                }
            }
            pParams->resetParams.push_back(resPar);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-resetInterval")))
        {
            msdk_opt_read(strInput[++i], pParams->resetInterval);
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-?")))
        {
            PrintHelp(strInput[0], NULL);
            return MFX_ERR_UNSUPPORTED;
        }
        else if (0 == msdk_strcmp(strInput[i], MSDK_STRING("-wall")))
        {
            if(i + 7 >= nArgNum)
            {
                PrintHelp(strInput[0], MSDK_STRING("Not enough parameters for -wall key"));
                return MFX_ERR_UNSUPPORTED;
            }
            pParams->bRendering = true;

            msdk_opt_read(strInput[++i], pParams->nWallW);
            msdk_opt_read(strInput[++i], pParams->nWallH);
            msdk_opt_read(strInput[++i], pParams->nWallCell);
            msdk_opt_read(strInput[++i], pParams->nWallMonitor);
            msdk_opt_read(strInput[++i], pParams->nWallFPS);

            int nTitle;
            msdk_opt_read(strInput[++i], nTitle);

            pParams->bWallNoTitle = 0 == nTitle;

           msdk_opt_read(strInput[++i], pParams->nWallTimeout);
        }
        else // 1-character options
        {
            std::basic_stringstream<msdk_char> stream;
            stream << MSDK_STRING("Unknown option: ") << strInput[i];
            PrintHelp(strInput[0], stream.str().c_str());
            return MFX_ERR_UNSUPPORTED;
        }
    }

    if (0 == msdk_strlen(pParams->strSrcFile))
    {
        PrintHelp(strInput[0], MSDK_STRING("Source file name not found"));
        return MFX_ERR_UNSUPPORTED;
    }

    if (0 == msdk_strlen(pParams->strDstFile))
    {
        pParams->bOutput = false;
    }

    return MFX_ERR_NONE;
}

#if defined(_WIN32) || defined(_WIN64)
int _tmain(int argc, TCHAR *argv[])
#else
int main(int argc, char *argv[])
#endif
{
    sInputParams      Params;   // input parameters from command line
    CCameraPipeline   Pipeline; // pipeline for decoding, includes input file reader, decoder and output file writer

    mfxStatus sts = MFX_ERR_NONE; // return value check

    sts = ParseInputString(argv, (mfxU8)argc, &Params);
    MSDK_CHECK_PARSE_RESULT(sts, MFX_ERR_NONE, 1);

    sts = Pipeline.Init(&Params);
    MSDK_CHECK_STATUS(sts, "Pipeline.Init failed");

    // print stream info
    Pipeline.PrintInfo();

    msdk_printf(MSDK_STRING("Camera pipe started\n"));
#if defined(_WIN32) || defined(_WIN64)
    LARGE_INTEGER timeBegin, timeEnd, m_Freq;
    QueryPerformanceFrequency(&m_Freq);

    QueryPerformanceCounter(&timeBegin);
#endif
    int resetNum = 0;
    for (;;) {
        sts = Pipeline.Run();
        if (MFX_WRN_VIDEO_PARAM_CHANGED == sts) {
            sInputParams *pParams = &Params;
            if (resetNum >= (int)Params.resetParams.size())
                break;
            msdk_strcopy(Params.strSrcFile, Params.resetParams[resetNum].strSrcFile);
            msdk_strcopy(pParams->strDstFile, Params.resetParams[resetNum].strDstFile);
            pParams->frameInfo[VPP_IN].nWidth   = (mfxU16)Params.resetParams[resetNum].width;
            pParams->frameInfo[VPP_IN].nHeight  = (mfxU16)Params.resetParams[resetNum].height;
            pParams->frameInfo[VPP_IN].CropW = pParams->frameInfo[VPP_IN].nWidth;
            if ( Params.resetParams[resetNum].cropW )
                pParams->frameInfo[VPP_IN].CropW = Params.resetParams[resetNum].cropW;
            pParams->frameInfo[VPP_IN].CropH = pParams->frameInfo[VPP_IN].nHeight;
            if ( Params.resetParams[resetNum].cropH )
                pParams->frameInfo[VPP_IN].CropH = Params.resetParams[resetNum].cropH;
            pParams->frameInfo[VPP_IN].CropX = align(Params.resetParams[resetNum].cropX);
            pParams->frameInfo[VPP_IN].CropY = Params.resetParams[resetNum].cropY;

            if ( ! pParams->frameInfo[VPP_IN].CropW )
            {
                pParams->frameInfo[VPP_IN].CropW = pParams->frameInfo[VPP_IN].nWidth;
            }

            if ( ! pParams->frameInfo[VPP_IN].CropH )
            {
                pParams->frameInfo[VPP_IN].CropH = pParams->frameInfo[VPP_IN].nHeight;
            }

            pParams->frameInfo[VPP_OUT].nWidth  = pParams->frameInfo[VPP_IN].CropW;
            pParams->frameInfo[VPP_OUT].nHeight = pParams->frameInfo[VPP_IN].CropH;
            pParams->frameInfo[VPP_OUT].CropW = pParams->frameInfo[VPP_IN].CropW;
            pParams->frameInfo[VPP_OUT].CropH = pParams->frameInfo[VPP_IN].CropH;

            pParams->inputType     = Params.resetParams[resetNum].inputType;
            pParams->bHP           = Params.resetParams[resetNum].bHP;
            pParams->hp_diff       = Params.resetParams[resetNum].hp_diff;
            pParams->hp_num        = Params.resetParams[resetNum].hp_num;

            pParams->bBayerDenoise    = Params.resetParams[resetNum].bDenoise;
            pParams->denoiseThreshold = Params.resetParams[resetNum].denoiseThreshold;

            pParams->bBlackLevel   = Params.resetParams[resetNum].bBlackLevel;
            pParams->black_level_B = Params.resetParams[resetNum].black_level_B;
            pParams->black_level_G0= Params.resetParams[resetNum].black_level_G0;
            pParams->black_level_G1= Params.resetParams[resetNum].black_level_G1;
            pParams->black_level_R = Params.resetParams[resetNum].black_level_R;
            pParams->bWhiteBalance   = Params.resetParams[resetNum].bWhiteBalance;
            pParams->white_balance_B = Params.resetParams[resetNum].white_balance_B;
            pParams->white_balance_G0= Params.resetParams[resetNum].white_balance_G0;
            pParams->white_balance_G1= Params.resetParams[resetNum].white_balance_G1;
            pParams->white_balance_R = Params.resetParams[resetNum].white_balance_R;
            pParams->tcc_red = Params.resetParams[resetNum].tcc_red;
            pParams->tcc_blue = Params.resetParams[resetNum].tcc_blue;
            pParams->tcc_green = Params.resetParams[resetNum].tcc_green;
            pParams->tcc_cyan = Params.resetParams[resetNum].tcc_cyan;
            pParams->tcc_magenta = Params.resetParams[resetNum].tcc_magenta;
            pParams->tcc_yellow = Params.resetParams[resetNum].tcc_yellow;
            pParams->bCCM = Params.resetParams[resetNum].bCCM;
            for (int k = 0; k < 3; k++)
                for (int z = 0; z < 3; z++)
                    pParams->CCM[k][z] = Params.resetParams[resetNum].CCM[k][z];

            //pParams->frameInfo[VPP_OUT].FourCC = MFX_FOURCC_ARGB16;
            //pParams->memTypeIn = pParams->memTypeOut = SYSTEM_MEMORY;

            sts = Pipeline.Reset(&Params);
            if (sts == MFX_ERR_INCOMPATIBLE_VIDEO_PARAM)
            {
                Pipeline.Close();
                sts = Pipeline.Init(&Params);
            }
            if (sts != MFX_ERR_NONE)
                break;
            resetNum++;
        } else
            break;
    }

#if defined(_WIN32) || defined(_WIN64)
    QueryPerformanceCounter(&timeEnd);


    double time = ((double)timeEnd.QuadPart - (double)timeBegin.QuadPart)/ (double)m_Freq.QuadPart;

    int frames = Pipeline.GetNumberProcessedFrames();
    _tprintf(_T("Total frames %d \n"), frames);
    _tprintf(_T("Total time   %.2lf sec\n"), time);
    _tprintf(_T("Total FPS    %.2lf fps\n"), frames/time);
#endif
    //Pipeline.Close();

    if(MFX_ERR_ABORTED != sts)
        MSDK_CHECK_STATUS(sts, "Unexpected error!!");

    msdk_printf(MSDK_STRING("\nCamera pipe finished\n"));

    return 0;
}
