/******************************************************************************\
Copyright (c) 2005-2018, Intel Corporation
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

#if defined(_WIN32) || defined(_WIN64)

#include <windowsx.h>
#include <dwmapi.h>
#include <mmsystem.h>

#include "sample_defs.h"
#include "camera_render.h"
#pragma warning(disable : 4100)

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
#ifdef WIN64
    CCameraD3DRender* pRender = (CCameraD3DRender*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
#else
    CCameraD3DRender* pRender = (CCameraD3DRender*)LongToPtr(GetWindowLongPtr(hWnd, GWL_USERDATA));
#endif
    if (pRender)
    {
        switch(message)
        {
            HANDLE_MSG(hWnd, WM_DESTROY, pRender->OnDestroy);
            HANDLE_MSG(hWnd, WM_KEYUP,   pRender->OnKey);
        }
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

CCameraD3DRender::CCameraD3DRender()
{
    m_bDwmEnabled = false;
    QueryPerformanceFrequency(&m_Freq);
    MSDK_ZERO_MEMORY(m_LastInputTime);
    m_nFrames = 0;
    m_nMonitorCurrent = 0;

    m_hwdev = NULL;
    MSDK_ZERO_MEMORY(m_sWindowParams);
    m_Hwnd = 0;
    MSDK_ZERO_MEMORY(m_rect);
    MSDK_ZERO_MEMORY(m_RectWindow);
    m_style = 0;
    MSDK_ZERO_MEMORY(m_RectWindow);
}

BOOL CALLBACK CCameraD3DRender::MonitorEnumProc(HMONITOR /*hMonitor*/,
                                           HDC /*hdcMonitor*/,
                                           LPRECT lprcMonitor,
                                           LPARAM dwData)
{
    CCameraD3DRender * pRender = reinterpret_cast<CCameraD3DRender *>(dwData);
    RECT r = {0};
    if (NULL == lprcMonitor)
        lprcMonitor = &r;

    if (pRender->m_nMonitorCurrent++ == pRender->m_sWindowParams.nAdapter)
    {
        pRender->m_RectWindow = *lprcMonitor;
    }
    return TRUE;
}

CCameraD3DRender::~CCameraD3DRender()
{
    Close();

    //DestroyTimer();
}

void CCameraD3DRender::Close()
{
    if (m_Hwnd)
        DestroyWindow(m_Hwnd);
    //DestroyTimer();
}

mfxStatus CCameraD3DRender::Init(sWindowParams pWParams)
{
    // window part
    m_sWindowParams = pWParams;

    WNDCLASS window;
    MSDK_ZERO_MEMORY(window);

    window.lpfnWndProc= (WNDPROC)WindowProc;
    window.hInstance= GetModuleHandle(NULL);;
    window.hCursor= LoadCursor(NULL, IDC_ARROW);
    window.lpszClassName= m_sWindowParams.lpClassName;

    UnregisterClass(m_sWindowParams.lpClassName, m_sWindowParams.hInstance);

    if (!RegisterClass(&window))
        return MFX_ERR_UNKNOWN;

    ::RECT displayRegion = {CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT};

    //right and bottom fields consist of width and height values of displayed reqion
    if (0 != m_sWindowParams.nx )
    {
        EnumDisplayMonitors(NULL, NULL, &CCameraD3DRender::MonitorEnumProc, (LPARAM)this);

        displayRegion.right  = (m_RectWindow.right - m_RectWindow.left)  / m_sWindowParams.nx;
        displayRegion.bottom = (m_RectWindow.bottom - m_RectWindow.top)  / m_sWindowParams.ny;
        displayRegion.left   = displayRegion.right * (m_sWindowParams.ncell % m_sWindowParams.nx) + m_RectWindow.left;
        displayRegion.top    = displayRegion.bottom * (m_sWindowParams.ncell / m_sWindowParams.nx) + m_RectWindow.top;
    }
    else
    {
        m_sWindowParams.nMaxFPS = 10000;//hypotetical maximum
    }

    //no title window style if required
    DWORD dwStyle = NULL == m_sWindowParams.lpWindowName ?  WS_POPUP|WS_BORDER|WS_MAXIMIZE : WS_OVERLAPPEDWINDOW;

    m_Hwnd = CreateWindowEx(NULL,
        m_sWindowParams.lpClassName,
        m_sWindowParams.lpWindowName,
        !m_sWindowParams.bFullScreen ? dwStyle : (WS_POPUP),
        !m_sWindowParams.bFullScreen ? displayRegion.left : 0,
        !m_sWindowParams.bFullScreen ? displayRegion.top : 0,
        !m_sWindowParams.bFullScreen ? displayRegion.right : GetSystemMetrics(SM_CXSCREEN),
        !m_sWindowParams.bFullScreen ? displayRegion.bottom : GetSystemMetrics(SM_CYSCREEN),
        m_sWindowParams.hWndParent,
        m_sWindowParams.hMenu,
        m_sWindowParams.hInstance,
        m_sWindowParams.lpParam);

    if (!m_Hwnd)
        return MFX_ERR_UNKNOWN;

    ShowWindow(m_Hwnd, SW_SHOWDEFAULT);
    UpdateWindow(m_Hwnd);

#ifdef WIN64
    SetWindowLongPtr(m_Hwnd, GWLP_USERDATA, (LONG_PTR)this);
#else
    SetWindowLong(m_Hwnd, GWL_USERDATA, PtrToLong(this));
#endif
    return MFX_ERR_NONE;
}


mfxStatus CCameraD3DRender::RenderFrame(mfxFrameSurface1 *pSurface, mfxFrameAllocator *pmfxAlloc)
{
    MSG msg;
    MSDK_ZERO_MEMORY(msg);
    while (msg.message != WM_QUIT && PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    RECT rect;
    GetClientRect(m_Hwnd, &rect);
    if (IsRectEmpty(&rect))
        return MFX_ERR_UNKNOWN;

    //EnableDwmQueuing();

    mfxStatus sts = m_hwdev->RenderFrame(pSurface, pmfxAlloc);
    MSDK_CHECK_STATUS(sts, "m_hwdev->RenderFrame failed");

    if (NULL != m_sWindowParams.lpWindowName)
    {
        double dfps = 0.;

        if (0 == m_LastInputTime.QuadPart)
        {
            QueryPerformanceCounter(&m_LastInputTime);
        }
        else
        {
            LARGE_INTEGER timeEnd;
            QueryPerformanceCounter(&timeEnd);
            dfps = ++m_nFrames * (double)m_Freq.QuadPart / ((double)timeEnd.QuadPart - (double)m_LastInputTime.QuadPart);
        }

        TCHAR str[40];
        _stprintf_s(str, 40, _T("fps=%.2lf    frame %d"), dfps, m_nFrames);
        SetWindowText(m_Hwnd, str);
    }

    return sts;
}

VOID CCameraD3DRender::OnDestroy(HWND /*hwnd*/)
{
    PostQuitMessage(0);
}

VOID CCameraD3DRender::OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
    if (TRUE == fDown)
        return;

    if ('1' == vk && false == m_sWindowParams.bFullScreen)
        ChangeWindowSize(true);
    else if (true == m_sWindowParams.bFullScreen)
        ChangeWindowSize(false);
}

void CCameraD3DRender::AdjustWindowRect(RECT *rect)
{
    int cxmax = GetSystemMetrics(SM_CXMAXIMIZED);
    int cymax = GetSystemMetrics(SM_CYMAXIMIZED);
    int cxmin = GetSystemMetrics(SM_CXMINTRACK);
    int cymin = GetSystemMetrics(SM_CYMINTRACK);
    int leftmax = cxmax - cxmin;
    int topmax = cymax - cxmin;
    if (rect->left < 0)
        rect->left = 0;
    if (rect->left > leftmax)
        rect->left = leftmax;
    if (rect->top < 0)
        rect->top = 0;
    if (rect->top > topmax)
        rect->top = topmax;

    if (rect->right < rect->left + cxmin)
        rect->right = rect->left + cxmin;
    if (rect->right - rect->left > cxmax)
        rect->right = rect->left + cxmax;

    if (rect->bottom < rect->top + cymin)
        rect->bottom = rect->top + cymin;
    if (rect->bottom - rect->top > cymax)
        rect->bottom = rect->top + cymax;
}

VOID CCameraD3DRender::ChangeWindowSize(bool bFullScreen)
{
    HMONITOR hMonitor = MonitorFromWindow(m_Hwnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFOEX mi;
    mi.cbSize = sizeof(mi);
    GetMonitorInfo(hMonitor, &mi);

    WINDOWINFO wndInfo;
    wndInfo.cbSize = sizeof(WINDOWINFO);
    GetWindowInfo(m_Hwnd, &wndInfo);

    if(!m_sWindowParams.bFullScreen)
    {
        m_rect = wndInfo.rcWindow;
        m_style = wndInfo.dwStyle;
    }

    m_sWindowParams.bFullScreen = bFullScreen;

    if(!bFullScreen)
    {
        AdjustWindowRectEx(&m_rect,0,0,0);
        SetWindowLong(m_Hwnd, GWL_STYLE, m_style);
        SetWindowPos(m_Hwnd, HWND_NOTOPMOST,
            m_rect.left , m_rect.top ,
            abs(m_rect.right - m_rect.left), abs(m_rect.bottom - m_rect.top),
            SWP_SHOWWINDOW);
    }
    else
    {
        SetWindowLong(m_Hwnd, GWL_STYLE, WS_POPUP);
        SetWindowPos(m_Hwnd, HWND_NOTOPMOST,mi.rcMonitor.left , mi.rcMonitor.top,
            abs(mi.rcMonitor.left - mi.rcMonitor.right), abs(mi.rcMonitor.top - mi.rcMonitor.bottom), SWP_SHOWWINDOW);
    }
}

bool CCameraD3DRender::EnableDwmQueuing()
{
    HRESULT hr;

    // DWM queuing is enabled already.
    if (m_bDwmEnabled)
    {
        return true;
    }

    // Check to see if DWM is currently enabled.
    BOOL bDWM = FALSE;

    hr = DwmIsCompositionEnabled(&bDWM);

    if (FAILED(hr))
    {
        _tprintf(_T("DwmIsCompositionEnabled failed with error 0x%x.\n"), (unsigned int)hr);
        return false;
    }

    // DWM queuing is disabled when DWM is disabled.
    if (!bDWM)
    {
        m_bDwmEnabled = false;
        return false;
    }

    // Retrieve DWM refresh count of the last vsync.
    DWM_TIMING_INFO dwmti = {0};

    dwmti.cbSize = sizeof(dwmti);

    hr = DwmGetCompositionTimingInfo(NULL, &dwmti);

    if (FAILED(hr))
    {
        _tprintf(_T("DwmGetCompositionTimingInfo failed with error 0x%x.\n"), (unsigned int)hr);
        return false;
    }

    // Enable DWM queuing from the next refresh.
    DWM_PRESENT_PARAMETERS dwmpp = {0};

    dwmpp.cbSize                    = sizeof(dwmpp);
    dwmpp.fQueue                    = TRUE;
    dwmpp.cRefreshStart             = dwmti.cRefresh + 1;
    dwmpp.cBuffer                   = 8; //maximum depth of DWM queue
    dwmpp.fUseSourceRate            = TRUE;
    dwmpp.cRefreshesPerFrame        = 1;
    dwmpp.eSampling                 = DWM_SOURCE_FRAME_SAMPLING_POINT;
    dwmpp.rateSource.uiDenominator  = 1;
    dwmpp.rateSource.uiNumerator    = m_sWindowParams.nMaxFPS;


    hr = DwmSetPresentParameters(m_Hwnd, &dwmpp);

    if (FAILED(hr))
    {
        _tprintf(_T("DwmSetPresentParameters failed with error 0x%x.\n"), (unsigned int)hr);
        return false;
    }

    // DWM queuing is enabled.
    m_bDwmEnabled = true;

    return true;
}
#endif // #if defined(_WIN32) || defined(_WIN64)
