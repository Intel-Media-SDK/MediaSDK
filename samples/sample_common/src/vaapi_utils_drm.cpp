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

#if defined(LIBVA_DRM_SUPPORT) || defined(LIBVA_WAYLAND_SUPPORT)

#include "vaapi_utils_drm.h"
#include "vaapi_allocator.h"
#include <fcntl.h>

#include <dirent.h>
#include <stdexcept>

#include <drm_fourcc.h>
#include <intel_bufmgr.h>

#define MFX_PCI_DIR "/sys/bus/pci/devices"
#define MFX_DRI_DIR "/dev/dri/"
#define MFX_PCI_DISPLAY_CONTROLLER_CLASS 0x03

struct mfx_disp_adapters
{
    mfxU32 vendor_id;
    mfxU32 device_id;
};

static int mfx_dir_filter(const struct dirent* dir_ent)
{
    if (!dir_ent) return 0;
    if (!strcmp(dir_ent->d_name, ".")) return 0;
    if (!strcmp(dir_ent->d_name, "..")) return 0;
    return 1;
}

typedef int (*fsort)(const struct dirent**, const struct dirent**);

static mfxU32 mfx_init_adapters(struct mfx_disp_adapters** p_adapters)
{
    mfxU32 adapters_num = 0;
    int i = 0;
    struct mfx_disp_adapters* adapters = NULL;
    struct dirent** dir_entries = NULL;
    int entries_num = scandir(MFX_PCI_DIR, &dir_entries, mfx_dir_filter, (fsort)alphasort);

    char file_name[300] = {};
    char str[16] = {0};
    FILE* file = NULL;

    for (i = 0; i < entries_num; ++i)
    {
        long int class_id = 0, vendor_id = 0, device_id = 0;

        if (!dir_entries[i])
            continue;

        // obtaining device class id
        snprintf(file_name, sizeof(file_name)/sizeof(file_name[0]), "%s/%s/%s", MFX_PCI_DIR, dir_entries[i]->d_name, "class");
        file = fopen(file_name, "r");
        if (file)
        {
            if (fgets(str, sizeof(str), file))
            {
                class_id = strtol(str, NULL, 16);
            }
            fclose(file);

            if (MFX_PCI_DISPLAY_CONTROLLER_CLASS == (class_id >> 16))
            {
                // obtaining device vendor id
                snprintf(file_name, sizeof(file_name)/sizeof(file_name[0]), "%s/%s/%s", MFX_PCI_DIR, dir_entries[i]->d_name, "vendor");
                file = fopen(file_name, "r");
                if (file)
                {
                    if (fgets(str, sizeof(str), file))
                    {
                        vendor_id = strtol(str, NULL, 16);
                    }
                    fclose(file);
                }
                // obtaining device id
                snprintf(file_name, sizeof(file_name)/sizeof(file_name[0]), "%s/%s/%s", MFX_PCI_DIR, dir_entries[i]->d_name, "device");
                file = fopen(file_name, "r");
                if (file)
                {
                    if (fgets(str, sizeof(str), file))
                    {
                        device_id = strtol(str, NULL, 16);
                    }
                    fclose(file);
                }
                // adding valid adapter to the list
                if (vendor_id && device_id)
                {
                    struct mfx_disp_adapters* tmp_adapters = NULL;

                    tmp_adapters = (mfx_disp_adapters*)realloc(adapters,
                                                               (adapters_num+1)*sizeof(struct mfx_disp_adapters));

                    if (tmp_adapters)
                    {
                        adapters = tmp_adapters;
                        adapters[adapters_num].vendor_id = vendor_id;
                        adapters[adapters_num].device_id = device_id;

                        ++adapters_num;
                    }
                }
            }
        }
        free(dir_entries[i]);
    }
    if (entries_num) free(dir_entries);
    if (p_adapters) *p_adapters = adapters;

    return adapters_num;
}

DRMLibVA::DRMLibVA(int type)
    : CLibVA(type)
    , m_fd(-1)
{
    const mfxU32 IntelVendorID = 0x8086;
    //the first Intel adapter is only required now, the second - in the future
    const mfxU32 numberOfRequiredIntelAdapter = 1;
    const char nodesNames[][8] = {"renderD", "card"};

    VAStatus va_res = VA_STATUS_SUCCESS;
    mfxStatus sts = MFX_ERR_NONE;
    int major_version = 0, minor_version = 0;

    mfx_disp_adapters* adapters = NULL;
    int adapters_num = mfx_init_adapters(&adapters);

    // Search for the required display adapter
    int i = 0, nFoundAdapters = 0;
    int nodesNumbers[] = {0,0};
    while ((i < adapters_num) && (nFoundAdapters != numberOfRequiredIntelAdapter))
    {
        if (adapters[i].vendor_id == IntelVendorID)
        {
            nFoundAdapters++;
            nodesNumbers[0] = i+128; //for render nodes
            nodesNumbers[1] = i;     //for card
        }
        i++;
    }
    if (adapters_num) free(adapters);
    // If Intel adapter with specified number wasn't found, throws exception
    if (nFoundAdapters != numberOfRequiredIntelAdapter)
        throw std::range_error("The Intel adapter with a specified number wasn't found");

    // Initialization of paths to the device nodes
    char** adapterPaths = new char* [2];
    for (int i=0; i<2; i++)
    {
        if ((i == 0) && (type == MFX_LIBVA_DRM_MODESET)) {
          adapterPaths[i] = NULL;
          continue;
        }
        adapterPaths[i] = new char[sizeof(MFX_DRI_DIR) + sizeof(nodesNames[i]) + 3];
        sprintf(adapterPaths[i], "%s%s%d", MFX_DRI_DIR, nodesNames[i], nodesNumbers[i]);
    }

    // Loading display. At first trying to open render nodes, then card.
    for (int i=0; i<2; i++)
    {
        if (!adapterPaths[i]) {
          sts = MFX_ERR_UNSUPPORTED;
          continue;
        }
        sts = MFX_ERR_NONE;
        m_fd = open(adapterPaths[i], O_RDWR);

        if (m_fd < 0) sts = MFX_ERR_NOT_INITIALIZED;
        if (MFX_ERR_NONE == sts)
        {
            m_va_dpy = m_vadrmlib.vaGetDisplayDRM(m_fd);

            if (!m_va_dpy)
            {
                close(m_fd);
                sts = MFX_ERR_NULL_PTR;
            }
        }

        if (MFX_ERR_NONE == sts)
        {
            va_res = m_libva.vaInitialize(m_va_dpy, &major_version, &minor_version);
            sts = va_to_mfx_status(va_res);
            if (MFX_ERR_NONE != sts)
            {
                close(m_fd);
                m_fd = -1;
            }
        }

        if (MFX_ERR_NONE == sts) break;
    }

    for (int i=0; i<2; i++)
    {
        delete [] adapterPaths[i];
    }
    delete [] adapterPaths;

    if (MFX_ERR_NONE != sts)
    {
        if (m_va_dpy)
        {
            m_libva.vaTerminate(m_va_dpy);
            m_va_dpy=0;
        }
        if (m_fd >= 0)
        {
            close(m_fd);
            m_fd=-1;
        }

        throw std::invalid_argument("Loading of VA display was failed");
    }
}

DRMLibVA::~DRMLibVA(void)
{
    if (m_va_dpy)
    {
        m_libva.vaTerminate(m_va_dpy);
    }
    if (m_fd >= 0)
    {
        close(m_fd);
    }
}

struct drmMonitorsTable {
  mfxI32 mfx_type;
  uint32_t drm_type;
  const msdk_char * type_name;
};

drmMonitorsTable g_drmMonitorsTable[] = {
#define __DECLARE(type) { MFX_MONITOR_ ## type, DRM_MODE_CONNECTOR_ ## type, MSDK_STRING(#type) }
  __DECLARE(Unknown),
  __DECLARE(VGA),
  __DECLARE(DVII),
  __DECLARE(DVID),
  __DECLARE(DVIA),
  __DECLARE(Composite),
  __DECLARE(SVIDEO),
  __DECLARE(LVDS),
  __DECLARE(Component),
  __DECLARE(9PinDIN),
  __DECLARE(HDMIA),
  __DECLARE(HDMIB),
  __DECLARE(eDP),
  __DECLARE(TV),
  __DECLARE(DisplayPort),
#if defined(DRM_MODE_CONNECTOR_VIRTUAL) // from libdrm 2.4.59
  __DECLARE(VIRTUAL),
#endif
#if defined(DRM_MODE_CONNECTOR_DSI) // from libdrm 2.4.59
  __DECLARE(DSI)
#endif
#undef __DECLARE
};

uint32_t drmRenderer::getConnectorType(mfxI32 monitor_type)
{
    for (size_t i=0; i < sizeof(g_drmMonitorsTable)/sizeof(g_drmMonitorsTable[0]); ++i) {
      if (g_drmMonitorsTable[i].mfx_type == monitor_type) {
        return g_drmMonitorsTable[i].drm_type;
      }
    }
    return DRM_MODE_CONNECTOR_Unknown;
}

const msdk_char* drmRenderer::getConnectorName(uint32_t connector_type)
{
    for (size_t i=0; i < sizeof(g_drmMonitorsTable)/sizeof(g_drmMonitorsTable[0]); ++i) {
      if (g_drmMonitorsTable[i].drm_type == connector_type) {
        return g_drmMonitorsTable[i].type_name;
      }
    }
    return MSDK_STRING("Unknown");
}

drmRenderer::drmRenderer(int fd, mfxI32 monitorType)
  : m_fd(fd)
  , m_bufmgr(NULL)
  , m_overlay_wrn(true)
  , m_pCurrentRenderTargetSurface(NULL)
{
    bool res = false;
    uint32_t connectorType = getConnectorType(monitorType);
    uint32_t i;

    if (monitorType == MFX_MONITOR_AUTO) {
      connectorType = DRM_MODE_CONNECTOR_Unknown;
    } else if (connectorType == DRM_MODE_CONNECTOR_Unknown) {
      throw std::invalid_argument("Unsupported monitor type");
    }
    drmModeRes *resource = m_drmlib.drmModeGetResources(m_fd);
    if (resource) {
        if (getConnector(resource, connectorType) &&
            getPlane()) {
            res = true;
        }
        m_drmlib.drmModeFreeResources(resource);
    }
    if (!res) {
      throw std::invalid_argument("Failed to allocate renderer");
    }
    msdk_printf(MSDK_STRING("drmrender: connected via %s to %dx%d@%d capable display\n"),
      getConnectorName(m_connector_type), m_mode.hdisplay, m_mode.vdisplay, m_mode.vrefresh);
}

drmRenderer::~drmRenderer()
{
    m_drmlib.drmModeFreeCrtc(m_crtc);
    if (m_bufmgr)
    {
        m_drmintellib.drm_intel_bufmgr_destroy(m_bufmgr);
        m_bufmgr = NULL;
    }
}

bool drmRenderer::getConnector(drmModeRes *resource, uint32_t connector_type)
{
    bool found = false;
    drmModeConnectorPtr connector = NULL;

    for (int i = 0; i < resource->count_connectors; ++i) {
        connector = m_drmlib.drmModeGetConnector(m_fd, resource->connectors[i]);
        if (connector) {
            if ((connector->connector_type == connector_type) ||
                (connector_type == DRM_MODE_CONNECTOR_Unknown)) {
                if (connector->connection == DRM_MODE_CONNECTED) {
                    msdk_printf(MSDK_STRING("drmrender: trying connection: %s\n"), getConnectorName(connector->connector_type));
                    m_connector_type = connector->connector_type;
                    m_connectorID = connector->connector_id;
                    found = setupConnection(resource, connector);
                    if (found) msdk_printf(MSDK_STRING("drmrender: succeeded...\n"));
                    else msdk_printf(MSDK_STRING("drmrender: failed...\n"));
                } else if ((connector_type != DRM_MODE_CONNECTOR_Unknown)) {
                    msdk_printf(MSDK_STRING("drmrender: error: requested monitor not connected\n"));
                }
            }
            m_drmlib.drmModeFreeConnector(connector);
            if (found) return true;
        }
    }
    msdk_printf(MSDK_STRING("drmrender: error: requested monitor not available\n"));
    return false;
}

bool drmRenderer::setupConnection(drmModeRes *resource, drmModeConnector* connector)
{
    bool ret = false;
    drmModeEncoderPtr encoder;

    if (!connector->count_modes) {
      msdk_printf(MSDK_STRING("drmrender: error: no valid modes for %s connector\n"),
        getConnectorName(connector->connector_type));
      return false;
    }
    // we will use the first available mode - that's always mode with the highest resolution
    m_mode = connector->modes[0];

    // trying encoder+crtc which are currently attached to connector
    m_encoderID = connector->encoder_id;
    encoder = m_drmlib.drmModeGetEncoder(m_fd, m_encoderID);
    if (encoder) {
      m_crtcID = encoder->crtc_id;
      for (int j = 0; j < resource->count_crtcs; ++j)
      {
          if (m_crtcID == resource->crtcs[j])
          {
              m_crtcIndex = j;
              break;
          }
      }
      ret = true;
      msdk_printf(MSDK_STRING("drmrender: selected crtc already attached to connector\n"));
      m_drmlib.drmModeFreeEncoder(encoder);
    }

    // if previous attempt to get crtc failed, let performs global search
    // searching matching encoder+crtc globally
    if (!ret) {
      for (int i = 0; i < connector->count_encoders; ++i) {
        encoder = m_drmlib.drmModeGetEncoder(m_fd, connector->encoders[i]);
        if (encoder) {
          for (int j = 0; j < resource->count_crtcs; ++j) {
            // check whether this CRTC works with the encoder
            if ( !((encoder->possible_crtcs & (1 << j)) &&
                   (encoder->crtc_id == resource->crtcs[j])) )
                continue;

            m_encoderID = connector->encoders[i];
            m_crtcIndex = j;
            m_crtcID = resource->crtcs[j];
            ret = true;
            msdk_printf(MSDK_STRING("drmrender: found crtc with global search\n"));
            break;
          }
          m_drmlib.drmModeFreeEncoder(encoder);
          if (ret)
              break;
        }
      }
    }
    if (ret) {
        m_crtc = m_drmlib.drmModeGetCrtc(m_fd, m_crtcID);
        if (!m_crtc)
            ret = false;
    } else {
        msdk_printf(MSDK_STRING("drmrender: failed to select crtc\n"));
    }
    return ret;
}

bool drmRenderer::getPlane()
{
    drmModePlaneResPtr planes = m_drmlib.drmModeGetPlaneResources(m_fd);
    if (!planes) {
        return false;
    }
    for (uint32_t i = 0; i < planes->count_planes; ++i) {
        drmModePlanePtr plane = m_drmlib.drmModeGetPlane(m_fd, planes->planes[i]);
        if (plane) {
            if (plane->possible_crtcs & (1 << m_crtcIndex)) {
                for (uint32_t j = 0; j < plane->count_formats; ++j) {
                    if (plane->formats[j] == DRM_FORMAT_XRGB8888) {
                        m_planeID = plane->plane_id;
                        m_drmlib.drmModeFreePlane(plane);
                        m_drmlib.drmModeFreePlaneResources(planes);
                        return true;
                    }
                }
            }
            m_drmlib.drmModeFreePlane(plane);
        }
    }
    m_drmlib.drmModeFreePlaneResources(planes);
    return false;
}

bool drmRenderer::setMaster()
{
    int wait_count = 0;
    do {
      if (!m_drmlib.drmSetMaster(m_fd)) return true;
      usleep(100);
      ++wait_count;
    } while(wait_count < 30000);
    msdk_printf(MSDK_STRING("drmrender: error: failed to get drm mastership during 3 seconds - aborting\n"));
    return false;
}

void drmRenderer::dropMaster()
{
    m_drmlib.drmDropMaster(m_fd);
}

bool drmRenderer::restore()
{
  if (!setMaster()) return false;

  int ret = m_drmlib.drmModeSetCrtc(m_fd, m_crtcID, m_crtc->buffer_id, m_crtc->x, m_crtc->y, &m_connectorID, 1, &m_mode);
  if (ret) {
    msdk_printf(MSDK_STRING("drmrender: failed to restore original mode\n"));
    return false;
  }
  dropMaster();
  return true;
}

void* drmRenderer::acquire(mfxMemId mid)
{
    vaapiMemId* vmid = (vaapiMemId*)mid;
    uint32_t fbhandle=0;

    if (vmid->m_buffer_info.mem_type == VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME) {
        if (!m_bufmgr) {
            m_bufmgr = m_drmintellib.drm_intel_bufmgr_gem_init(m_fd, 4096);
            if (!m_bufmgr) return NULL;
        }

        drm_intel_bo* bo = m_drmintellib.drm_intel_bo_gem_create_from_prime(
            m_bufmgr, (int)vmid->m_buffer_info.handle, vmid->m_buffer_info.mem_size);
        if (!bo) return NULL;

        int ret = m_drmlib.drmModeAddFB(m_fd,
              vmid->m_image.width, vmid->m_image.height,
              24, 32, vmid->m_image.pitches[0],
              bo->handle, &fbhandle);
        if (ret) {
          return NULL;
        }
        m_drmintellib.drm_intel_bo_unreference(bo);
    } else if (vmid->m_buffer_info.mem_type == VA_SURFACE_ATTRIB_MEM_TYPE_KERNEL_DRM) {
        struct drm_gem_open flink_open;
        struct drm_gem_close flink_close;

        MSDK_ZERO_MEMORY(flink_open);
        flink_open.name = vmid->m_buffer_info.handle;
        int ret = m_drmlib.drmIoctl(m_fd, DRM_IOCTL_GEM_OPEN, &flink_open);
        if (ret) return NULL;

        ret = m_drmlib.drmModeAddFB(m_fd,
              vmid->m_image.width, vmid->m_image.height,
              24, 32, vmid->m_image.pitches[0],
              flink_open.handle, &fbhandle);
        if (ret) return NULL;

        MSDK_ZERO_MEMORY(flink_close);
        flink_close.handle = flink_open.handle;
        ret = m_drmlib.drmIoctl(m_fd, DRM_IOCTL_GEM_CLOSE, &flink_close);
        if (ret) return NULL;
    } else {
        return NULL;
    }
    try {
        uint32_t* hdl = new uint32_t;
        *hdl = fbhandle;
        return hdl;
    } catch(...) {
        return NULL;
    }
}

void drmRenderer::release(mfxMemId mid, void * mem)
{
    uint32_t* hdl = (uint32_t*)mem;
    if (!hdl) return;
    if (!restore()) {
      msdk_printf(MSDK_STRING("drmrender: warning: failure to restore original mode may lead to application segfault!\n"));
    }
    m_drmlib.drmModeRmFB(m_fd, *hdl);
    delete(hdl);
}

mfxStatus drmRenderer::render(mfxFrameSurface1 * pSurface)
{
    int ret;
    vaapiMemId * memid;
    uint32_t fbhandle;

    if (!pSurface || !pSurface->Data.MemId) return MFX_ERR_INVALID_HANDLE;
    memid = (vaapiMemId*)(pSurface->Data.MemId);
    if (!memid->m_custom) return MFX_ERR_INVALID_HANDLE;
    fbhandle = *(uint32_t*)memid->m_custom;

    // rendering on the screen
    if (!setMaster()) {
      return MFX_ERR_UNKNOWN;
    }
    if ((m_mode.hdisplay == memid->m_image.width) &&
        (m_mode.vdisplay == memid->m_image.height)) {
        // surface in the framebuffer exactly matches crtc scanout port, so we
        // can scanout from this framebuffer for the whole crtc
        ret = m_drmlib.drmModeSetCrtc(m_fd, m_crtcID, fbhandle, 0, 0, &m_connectorID, 1, &m_mode);
        if (ret) {
          return MFX_ERR_UNKNOWN;
        }
    } else {
        if (m_overlay_wrn) {
          m_overlay_wrn = false;
          msdk_printf(MSDK_STRING("drmrender: warning: rendering via OVERLAY plane\n"));
        }
        // surface in the framebuffer exactly does NOT match crtc scanout port,
        // and we can only use overlay technique with possible resize (depending on the driver))
        ret = m_drmlib.drmModeSetPlane(m_fd, m_planeID, m_crtcID, fbhandle, 0,
          0, 0, m_crtc->width, m_crtc->height,
          pSurface->Info.CropX << 16, pSurface->Info.CropY << 16, pSurface->Info.CropW << 16, pSurface->Info.CropH << 16);
        if (ret) {
          return MFX_ERR_UNKNOWN;
        }
    }
    dropMaster();

    /* Unlock previous Render Target Surface (if exists) */
    if (NULL != m_pCurrentRenderTargetSurface)
        m_pCurrentRenderTargetSurface->Data.Locked--;

    /* new Render target */
    m_pCurrentRenderTargetSurface = pSurface;
    /* And lock it */
    m_pCurrentRenderTargetSurface->Data.Locked++;
    return MFX_ERR_NONE;
}

#endif // #if defined(LIBVA_DRM_SUPPORT)
