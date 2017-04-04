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

#if defined(LIBVA_X11_SUPPORT)

#include "sample_defs.h"
#include "vaapi_utils_x11.h"

#include <dlfcn.h>
#if defined(X11_DRI3_SUPPORT)
#include <fcntl.h>
#endif

#define VAAPI_X_DEFAULT_DISPLAY ":0.0"

const char* PROCESSING_DRIVER_NAME="iHD";
const char* RENDERING_DRIVER_NAME="iHD";

X11LibVA::X11LibVA(void)
    : CLibVA(MFX_LIBVA_X11)
    , m_display(0)
    , m_contextID(VA_INVALID_ID)
{
    VAStatus va_res = VA_STATUS_SUCCESS;
    mfxStatus sts = MFX_ERR_NONE;
    int major_version = 0, minor_version = 0;
    char* currentDisplay = getenv("DISPLAY");

#if !defined(X11_DRI3_SUPPORT)
    try
    {
        if (currentDisplay)
            m_display = m_x11lib.XOpenDisplay(currentDisplay);
        else
            m_display = m_x11lib.XOpenDisplay(VAAPI_X_DEFAULT_DISPLAY);

        if (NULL == m_display) sts = MFX_ERR_NOT_INITIALIZED;
        if (MFX_ERR_NONE == sts)
        {
            m_va_dpy = m_vax11lib.vaGetDisplay(m_display);
            m_va_dpy_render = m_vax11lib.vaGetDisplay(m_display);

            if (!m_va_dpy || !m_va_dpy_render)
            {
                m_x11lib.XCloseDisplay(m_display);
                sts = MFX_ERR_NULL_PTR;
            }
        }
        if (MFX_ERR_NONE == sts)
        {
            if (!setenv("LIBVA_DRIVER_NAME", PROCESSING_DRIVER_NAME, 1)) {
            va_res = m_libva.vaInitialize(m_va_dpy, &major_version, &minor_version);
            sts = va_to_mfx_status(va_res);
            if (MFX_ERR_NONE != sts) {
                m_x11lib.XCloseDisplay(m_display);
                }
                unsetenv("LIBVA_DRIVER_NAME");
            } else {
                sts = MFX_ERR_NOT_INITIALIZED;
            }
        }
        if (MFX_ERR_NONE == sts)
        {
        if (!msdk_strcmp(PROCESSING_DRIVER_NAME, RENDERING_DRIVER_NAME)) {
            // same driver
            VAStatus        va_res        = VA_STATUS_SUCCESS;
            VAConfigID      vpp_config_id = VA_INVALID_ID;
            VAConfigAttrib  cfgAttrib;

            m_va_dpy_render = m_va_dpy;

            cfgAttrib.type = VAConfigAttribRTFormat;
            m_libva.vaGetConfigAttributes(m_va_dpy_render,
                                          VAProfileNone,
                                          VAEntrypointVideoProc,
                                          &cfgAttrib,
                                          1);

            va_res = m_libva.vaCreateConfig(m_va_dpy_render,
                                            VAProfileNone,
                                            VAEntrypointVideoProc,
                                            &cfgAttrib,
                                            1,
                                            &vpp_config_id);

            /* Create a context for VPP pipe */
            va_res = m_libva.vaCreateContext(m_va_dpy_render,
                                             vpp_config_id,
                                             0,
                                             0,
                                             VA_PROGRESSIVE,
                                             0,
                                             0,
                                             &m_contextID);
        } else {
            m_va_dpy_render = m_vax11lib.vaGetDisplay(m_display);
            if (!m_va_dpy_render)
            {
                m_libva.vaTerminate(m_va_dpy);
                m_x11lib.XCloseDisplay(m_display);
                sts = MFX_ERR_NULL_PTR;
            }
            if (!setenv("LIBVA_DRIVER_NAME", RENDERING_DRIVER_NAME, 1)) {
                va_res = m_libva.vaInitialize(m_va_dpy_render, &major_version, &minor_version);
                sts = va_to_mfx_status(va_res);
                if (MFX_ERR_NONE != sts)
                {
                    m_libva.vaTerminate(m_va_dpy);
                    m_x11lib.XCloseDisplay(m_display);
                }
                unsetenv("LIBVA_DRIVER_NAME");
            }
            }
        }
    }
#else
    try
    {
        if (currentDisplay)
            m_display = m_x11lib.XOpenDisplay(currentDisplay);
        else
            m_display = m_x11lib.XOpenDisplay(VAAPI_X_DEFAULT_DISPLAY);

        if (NULL == m_display) sts = MFX_ERR_NOT_INITIALIZED;
        if (MFX_ERR_NONE == sts)
        {
            m_va_dpy = m_vax11lib.vaGetDisplay(m_display);

            if (!m_va_dpy)
            {
                m_x11lib.XCloseDisplay(m_display);
                sts = MFX_ERR_NULL_PTR;
            }
        }
        if (MFX_ERR_NONE == sts)
        {
            va_res = m_libva.vaInitialize(m_va_dpy, &major_version, &minor_version);
            sts = va_to_mfx_status(va_res);
            if (MFX_ERR_NONE != sts)
            {
                m_x11lib.XCloseDisplay(m_display);
            }
        }
    }
#endif // X11_DRI3_SUPPORT
    catch(std::exception& )
    {
        sts = MFX_ERR_NOT_INITIALIZED;
    }

    if (MFX_ERR_NONE != sts) throw std::bad_alloc();
}

X11LibVA::~X11LibVA(void)
{
    if (!msdk_strcmp(PROCESSING_DRIVER_NAME, RENDERING_DRIVER_NAME))
    {
        //release context
        if (m_contextID != VA_INVALID_ID)
        {
            m_libva.vaDestroyContext(m_va_dpy_render, m_contextID);
            m_contextID = VA_INVALID_ID;
        }
    }

#if !defined(X11_DRI3_SUPPORT)
    if (m_va_dpy_render && (m_va_dpy_render != m_va_dpy))
    {
        m_libva.vaTerminate(m_va_dpy_render);
    }
#endif
    if (m_va_dpy)
    {
        m_libva.vaTerminate(m_va_dpy);
    }
    if (m_display)
    {
        m_x11lib.XCloseDisplay(m_display);
    }
}

#endif // #if defined(LIBVA_X11_SUPPORT)
