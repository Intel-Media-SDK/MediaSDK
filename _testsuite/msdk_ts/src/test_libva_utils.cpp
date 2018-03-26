/*#include "test_common.h"

#if defined(LINUX32) || defined (LINUX64)
#if defined(LIBVA_SUPPORT)
LinDisplayHolder* LinDisplayHolder::single_instance = 0;

LinDisplayHolder & LinDisplayHolder::get_instance()
{
    assert(single_instance);
    return *single_instance;
}
#if defined(LIBVA_X11_SUPPORT)
LinDisplayHolder::LinDisplayHolder()
{
    assert(!single_instance);
    single_instance = this;

    va_display = 0;
    x_display = 0;
    GetVADisplayHandle();
}

LinDisplayHolder::~LinDisplayHolder()
{
    single_instance = 0;

    CloseVADisplay();
    if (va_display)
        ;//G_OSTRM2("ERR", "Failed to terminate VA", "\n");
    if (x_display)
        ;//G_OSTRM2("ERR", "Failed to close X display", "\n");
}

mfxHDL* LinDisplayHolder::GetVADisplayHandle()
{
    mfxI8* currentDisplay = getenv("DISPLAY");

    if (va_display)
    {
        if (!x_display)    ;//G_OSTRM2("ERR", "Warning: VA is initialised, but X display was not opened", "\n");
        return (mfxHDL*) va_display;
    }
    if (!x_display)
    {
        if (currentDisplay)
            x_display = XOpenDisplay(currentDisplay);
        else
            x_display = XOpenDisplay(VAAPI_X_DEFAULT_DISPLAY);
        if (NULL == x_display)
        {
            ;//G_OSTRM2("ERR", "Failed to XOpenDisplay, NULL pointer was returned", "\n");
            return NULL;
        }
    ;//G_OSTRM2("ERR", "XOpenDisplay: Display opened", "\n");
    ;//G_OSTRM3("ERR", "x_display = ", x_display, "\n");
    }
    va_display = vaGetDisplay(VAAPI_GET_X_DISPLAY(x_display));

    if (NULL == va_display)
    {
        ;//G_OSTRM2("ERR", "Warning: vaGetDisplay (X11) returned NULL pointer", "\n");
    }
    ;//G_OSTRM3("ERR", "LinDisplayHolder::GetVADisplayHandle() got display ", va_display, "\n");
    mfxI32 v_minor, v_major;
    VAStatus va_res = VA_STATUS_SUCCESS;
    va_res = vaInitialize(va_display, &v_minor, &v_major);
    if (0 != va_res)
    {
        ;//G_OSTRM3("ERR", "Failed to vaInitialize, returned error ", va_res, "\n");
        return NULL;
    }
    ;//G_OSTRM5("ERR", "libVA X11 version ", v_major, ".", v_minor," was initialised\n");
    return (mfxHDL*) va_display;
}

mfxStatus LinDisplayHolder::CloseVADisplay()
{
    VAStatus va_res = VA_STATUS_SUCCESS;
    if (NULL == va_display)
    {
        ;//G_OSTRM2("ERR", "Failed to find opened VA display", "\n");
        if (x_display)
        {
            ;//G_OSTRM3("ERR", "LinDisplayHolder::CloseVADisplay(): X Display = ", x_display, "\n");
            XCloseDisplay ((Display*) x_display);
            x_display = NULL;
            ;//G_OSTRM4("ERR", "However, X display ",x_display," had been opened and now it is closed", "\n");
        }
        return MFX_ERR_NOT_INITIALIZED;
    }

    ;//G_OSTRM3("ERR", "LinDisplayHolder::CloseVADisplay(): VA_Display = ", va_display, "\n");
    va_res = vaTerminate(va_display);
    va_display = NULL;
    if (0 != va_res)
    {
        if (!x_display)
        {
            ;//G_OSTRM2("ERR", "VA display was found and terminated with error\nX display was not found!", "\n");
        }
        else
        {
            ;//G_OSTRM3("ERR", "LinDisplayHolder::CloseVADisplay(): X Display = ", x_display, "\n");
            XCloseDisplay ((Display*) x_display);
        }
        x_display = NULL;
        ;//G_OSTRM3("ERR", "vaTerminate returns error ", va_res, "\n");
        return MFX_ERR_NOT_INITIALIZED;
    }
    if (!x_display)
    {
        ;//G_OSTRM2("ERR", "VA display was found and closed, but x display was not found", "\n");
    }
    else
    {
        ;//G_OSTRM3("ERR", "LinDisplayHolder::CloseVADisplay(): X Display = ", x_display, "\n");
        XCloseDisplay ((Display*) x_display);
    }
    x_display = NULL;

    return MFX_ERR_NONE;
}
#endif //defined(LIBVA_X11_SUPPORT)

#if defined(LIBVA_DRM_SUPPORT)
LinDisplayHolder::LinDisplayHolder()
{
    assert(!single_instance);
    single_instance = this;

    va_display = 0;
    m_fd = -1;
    GetVADisplayHandle();
}

LinDisplayHolder::~LinDisplayHolder()
{
    single_instance = 0;

    CloseVADisplay();
    if (va_display)
        ;//G_OSTRM2("ERR", "Failed to terminate VA", "\n");
}

mfxHDL* LinDisplayHolder::GetVADisplayHandle()
{
    if (va_display)
    {
        return (mfxHDL*) va_display;
    }
     
    if ( m_fd < 0 )
    {
        m_fd = open("/dev/dri/card0", O_RDWR);
        if ( m_fd < 0 )
        {
            ;//G_OSTRM2("ERR", "Failed to open /dev/dri/card0", "\n");
            return NULL;
        }
    }
    va_display = vaGetDisplayDRM(m_fd);

    if (NULL == va_display)
    {
        ;//G_OSTRM2("ERR", "Warning: vaGetDisplay(DRM) returned NULL pointer", "\n");
    }
    ;//G_OSTRM3("ERR", "LinDisplayHolder::GetVADisplayHandle() got display ", va_display, "\n");
    mfxI32 v_minor, v_major;
    VAStatus va_res = VA_STATUS_SUCCESS;
    va_res = vaInitialize(va_display, &v_minor, &v_major);
    if (0 != va_res)
    {
        ;//G_OSTRM3("ERR", "Failed to vaInitialize, returned error ", va_res, "\n");
        return NULL;
    }
    ;//G_OSTRM5("ERR", "libVA DRM version ", v_major, ".", v_minor," was initialised\n");
    return (mfxHDL*) va_display;
}

mfxStatus LinDisplayHolder::CloseVADisplay()
{
    VAStatus va_res = VA_STATUS_SUCCESS;
    if (NULL == va_display)
    {
        ;//G_OSTRM2("ERR", "Failed to find opened VA display", "\n");
        return MFX_ERR_NOT_INITIALIZED;
    }

    ;//G_OSTRM3("ERR", "LinDisplayHolder::CloseVADisplay(): VA_Display = ", va_display, "\n");
    va_res = vaTerminate(va_display);
    va_display = NULL;
    if (0 != va_res)
    {
        if (m_fd >= 0)
        {
            close(m_fd);
            m_fd = -1;
        }
        ;//G_OSTRM3("ERR", "vaTerminate returns error ", va_res, "\n");
        return MFX_ERR_NOT_INITIALIZED;
    }
    if (m_fd >= 0)
    {
        close(m_fd);
        m_fd = -1;
    }
    return MFX_ERR_NONE;
}
#endif //defined(LIBVA_DRM_SUPPORT)
#endif //defined(LIBVA_SUPPORT)
#endif //defined(LINUX32) || defined (LINUX64)
*/