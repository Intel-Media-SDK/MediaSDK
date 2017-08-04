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

#include <iostream>
#include <exception>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <sys/mman.h>
extern "C" {
#include <drm.h>
#include <xf86drm.h>
#include <intel_bufmgr.h>
}
#include "class_wayland.h"
#include "listener_wayland.h"
#include "wayland-drm-client-protocol.h"

#define BATCH_SIZE 0x80000

static const struct wl_callback_listener frame_listener = {
    handle_done
};

static const struct wl_buffer_listener buffer_listener = {
    buffer_release
};

Wayland::Wayland()
    : m_display(NULL)
    , m_registry(NULL)
    , m_compositor(NULL)
    , m_shell(NULL)
    , m_drm(NULL)
    , m_shm(NULL)
    , m_pool(NULL)
    , m_surface(NULL)
    , m_shell_surface(NULL)
    , m_callback(NULL)
    , m_shm_pool(NULL)
    , m_event_queue(NULL)
    , m_fd(-1)
    , m_display_fd(-1)
    , m_bufmgr(NULL)
    , m_device_name(NULL)
    , m_pending_frame(0)
    , m_x(0), m_y(0)
    , m_perf_mode(false)
{
    std::memset(&m_poll, 0, sizeof(m_poll));
}

bool Wayland::InitDisplay()
{
    static const struct wl_registry_listener registry_listener = {
        .global = registry_handle_global,
        .global_remove = remove_registry_global
    };

    m_display = wl_display_connect(NULL);
    if (NULL == m_display) {
        std::cout << "Error: Cannot connect to wayland display\n";
        return false;
    }
    m_registry = wl_display_get_registry(m_display);
    wl_registry_add_listener(m_registry
        , &registry_listener
        , this);

    m_display_fd = wl_display_get_fd(m_display);
    wl_display_roundtrip(m_display);
    wl_display_roundtrip(m_display);
    m_event_queue = wl_display_create_queue(m_display);
    if(NULL == m_event_queue)
        return false;

    m_poll.fd = m_display_fd;
    m_poll.events = POLLIN;
    return true;
}

int Wayland::DisplayRoundtrip()
{
    return wl_display_roundtrip(m_display);
}

bool Wayland::CreateSurface()
{
    static const struct wl_shell_surface_listener
        shell_surface_listener = {
            shell_surface_ping,
            shell_surface_configure
        };

    m_surface = wl_compositor_create_surface(m_compositor);
    if (NULL == m_surface)
        return false;

    m_shell_surface = wl_shell_get_shell_surface(m_shell
        , m_surface);
    if(NULL == m_shell_surface)
    {
        wl_surface_destroy(m_surface);
        return false;
    }

    wl_shell_surface_add_listener(m_shell_surface
        , &shell_surface_listener
        , 0);
    wl_shell_surface_set_toplevel(m_shell_surface);
    wl_shell_surface_set_user_data(m_shell_surface, m_surface);
    wl_surface_set_user_data(m_surface, NULL);
    return true;
}


void Wayland::FreeSurface()
{
    if(NULL != m_shell_surface)
        wl_shell_surface_destroy(m_shell_surface);
    if(NULL != m_surface)
        wl_surface_destroy(m_surface);
}

void Wayland::Sync()
{
    int ret;
    while(NULL != m_callback)
    {
        while(wl_display_prepare_read_queue(m_display, m_event_queue) < 0)
            wl_display_dispatch_queue_pending(m_display, m_event_queue);

        wl_display_flush(m_display);

        ret = poll(&m_poll,1,-1);
        if(ret < 0 )
            wl_display_cancel_read(m_display);
        else
        wl_display_read_events(m_display);
        wl_display_dispatch_queue_pending(m_display, m_event_queue);
    }
}

void Wayland::SetPerfMode(bool perf_mode)
{
    m_perf_mode = perf_mode;
}

void Wayland::SetRenderWinPos(int x, int y)
{
    m_x = x; m_y = y;
}

void Wayland::RenderBuffer(struct wl_buffer *buffer
     , int32_t width
     , int32_t height)
{
    wl_surface_attach(m_surface, buffer, 0, 0);
    wl_surface_damage(m_surface, m_x, m_y, width, height);

    wl_proxy_set_queue((struct wl_proxy *) buffer, m_event_queue);

    wl_buffer_add_listener(buffer, &buffer_listener, NULL);
    m_pending_frame=1;
    if (m_perf_mode)
        m_callback = wl_display_sync(m_display);
    else
    m_callback = wl_surface_frame(m_surface);
    wl_callback_add_listener(m_callback, &frame_listener, this);
    wl_proxy_set_queue((struct wl_proxy *) m_callback, m_event_queue);
    wl_surface_commit(m_surface);
    wl_display_dispatch_queue(m_display, m_event_queue);
    /* Force a Sync before and after render to ensure client handles
      wayland events in a timely fashion. This also fixes the one time
      flicker issue on wl_shell_surface pointer enter */
    Sync();

}

void Wayland::RenderBufferWinPosSize(struct wl_buffer *buffer
    , int x
    , int y
    , int32_t width
    , int32_t height)
{
    wl_surface_attach(m_surface, buffer, 0, 0);
    wl_surface_damage(m_surface, x, y, width, height);

    wl_proxy_set_queue((struct wl_proxy *) buffer, m_event_queue);

    wl_buffer_add_listener(buffer, &buffer_listener, NULL);
    m_pending_frame=1;
    if (m_perf_mode)
        m_callback = wl_display_sync(m_display);
    else
    m_callback = wl_surface_frame(m_surface);
    wl_callback_add_listener(m_callback, &frame_listener, this);
    wl_proxy_set_queue((struct wl_proxy *) m_callback, m_event_queue);
    wl_surface_commit(m_surface);
    wl_display_dispatch_queue(m_display, m_event_queue);
}


void Wayland::DestroyCallback()
{
    if(m_callback)
    {
        wl_callback_destroy(m_callback);
        m_callback = NULL;
        m_pending_frame=0;
    }
}

//ShmPool
bool  Wayland::CreateShmPool(int fd, int32_t size, int prot)
{
    m_shm_pool = new struct ShmPool;
    if (NULL == m_shm_pool)
        return false;

    m_shm_pool->capacity = size;
    m_shm_pool->size = 0;
    m_shm_pool->fd = fd;

    m_shm_pool->memory = static_cast<uint32_t*>(mmap(0
        , size
        , prot
        , MAP_SHARED
        , m_shm_pool->fd
        , 0));
    if (MAP_FAILED == m_shm_pool->memory)
    {
        delete m_shm_pool;
        return false;
    }

    m_pool = wl_shm_create_pool(m_shm
        , m_shm_pool->fd
        , size);
    if (NULL == m_pool)
    {
        munmap(m_shm_pool->memory, size);
        delete m_shm_pool;
        return false;
    }
    wl_shm_pool_set_user_data(m_pool, m_shm_pool);
    return true;
}


void Wayland::FreeShmPool()
{
    wl_shm_pool_destroy(m_pool);
    munmap(m_shm_pool->memory, m_shm_pool->capacity);
    delete m_shm_pool;
}


struct wl_buffer * Wayland::CreateShmBuffer(unsigned width
    , unsigned height
    , unsigned stride
    , uint32_t PIXEL_FORMAT_ID)
{
    struct wl_buffer *buffer;
    buffer = wl_shm_pool_create_buffer(m_pool
        , m_shm_pool->size*sizeof(uint32_t)
        , width
        , height
        , stride
        , PIXEL_FORMAT_ID);
    if(NULL == buffer)
    return NULL;

    m_shm_pool->size += stride*height;
    return buffer;
}

void Wayland::FreeShmBuffer(struct wl_buffer *buffer)
{
    wl_buffer_destroy(buffer);
}

int Wayland::Dispatch()
{
    return wl_display_dispatch(m_display);
}

struct wl_buffer * Wayland::CreatePlanarBuffer(uint32_t name
    , int32_t width
    , int32_t height
    , uint32_t format
    , int32_t offsets[3]
    , int32_t pitches[3])
{
    struct wl_buffer * buffer = NULL;
    if(NULL == m_drm)
        return NULL;

    buffer = wl_drm_create_planar_buffer(m_drm
        , name
        , width
        , height
        , format
        , offsets[0]
        , pitches[0]
        , offsets[1]
        , pitches[1]
        , offsets[2]
        , pitches[2]);
    return buffer;
}

struct wl_buffer * Wayland::CreatePrimeBuffer(uint32_t name
    , int32_t width
    , int32_t height
    , uint32_t format
    , int32_t offsets[3]
    , int32_t pitches[3])
{
    struct wl_buffer * buffer = NULL;
    if(NULL == m_drm)
        return NULL;

    buffer = wl_drm_create_prime_buffer(m_drm
            , name
            , width
            , height
            , format
            , offsets[0]
            , pitches[0]
            , offsets[1]
            , pitches[1]
            , offsets[2]
            , pitches[2]);
    return buffer;
}

Wayland::~Wayland()
{
    if(NULL != m_shell)
        wl_shell_destroy(m_shell);
    if(NULL != m_shm)
        wl_shm_destroy(m_shm);
    if(NULL != m_bufmgr) {
        drm_intel_bufmgr_destroy(m_bufmgr);
    }
    if(NULL != m_compositor)
        wl_compositor_destroy(m_compositor);
    if(NULL != m_event_queue)
        wl_event_queue_destroy(m_event_queue);
    if(NULL != m_registry)
        wl_registry_destroy(m_registry);
    if(NULL != m_display)
        wl_display_disconnect(m_display);
    if(NULL != m_device_name)
        delete m_device_name;
}

// Registry
void Wayland::RegistryGlobal(struct wl_registry *registry
    , uint32_t name
    , const char *interface
    , uint32_t version)
{
    if(0 == strcmp(interface, "wl_compositor"))
        m_compositor = static_cast<wl_compositor*>
            (wl_registry_bind(registry
            , name
            , &wl_compositor_interface
            , version));
    else if(0 == strcmp(interface, "wl_shell"))
        m_shell = static_cast<wl_shell*>
            (wl_registry_bind(registry
            , name
            , &wl_shell_interface
            , version));
    else if(0 == strcmp(interface, "wl_drm")) {
        static const struct wl_drm_listener drm_listener = {
            drm_handle_device,
            drm_handle_format,
            drm_handle_authenticated,
            drm_handle_capabilities
        };
        m_drm = static_cast<wl_drm*>(wl_registry_bind(registry
            , name
            , &wl_drm_interface
            , 2));
            wl_drm_add_listener(m_drm, &drm_listener, this);
    }
}

void Wayland::DrmHandleDevice(const char *name)
{
    m_device_name = strdup(name);
    if (!m_device_name)
        return;

    drm_magic_t magic;
    m_fd = open(m_device_name, O_RDWR | O_CLOEXEC);
    if (-1 == m_fd) {
        std::cout << "Error: Could not open " <<
            m_device_name << "\n";
        return;
    }
    drmGetMagic(m_fd, &magic);
    wl_drm_authenticate(m_drm, magic);
}

void Wayland::DrmHandleAuthenticated()
{
    m_bufmgr = drm_intel_bufmgr_gem_init(m_fd, BATCH_SIZE);
}

Wayland* WaylandCreate()
{
    return new Wayland;
}

void WaylandDestroy(Wayland *pWld)
{
    delete pWld;
}

