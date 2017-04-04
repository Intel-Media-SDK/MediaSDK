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

#ifndef WAYLAND_CLASS_H
#define WAYLAND_CLASS_H

#if defined(LIBVA_WAYLAND_SUPPORT)

extern "C"
{
    #include <intel_bufmgr.h>
}
#include <poll.h>
#include <wayland-client.h>
#include "wayland-drm-client-protocol.h"

/* ShmPool Struct */
struct ShmPool {
    int fd;
    uint32_t *memory;
    unsigned capacity;
    unsigned size;
};

class Wayland {
    public:
        Wayland();
        virtual ~Wayland();
        virtual bool InitDisplay();
        virtual bool CreateSurface();
        virtual void FreeSurface();
        virtual void SetRenderWinPos(int x, int y);
        virtual void RenderBuffer(struct wl_buffer *buffer
            , int32_t width
            , int32_t height);
        virtual void RenderBufferWinPosSize(struct wl_buffer *buffer
            , int x
            , int y
            , int32_t width
            , int32_t height);
        bool CreateShmPool(int fd, int32_t size, int prot);
        void FreeShmPool();
        /* Create wl_buffer from shm_pool */
        struct wl_buffer * CreateShmBuffer(unsigned width
            , unsigned height
            , unsigned stride
            , uint32_t PIXEL_FORMAT_ID);
        void FreeShmBuffer(struct wl_buffer *buffer);
        int Dispatch();
        struct wl_buffer * CreatePlanarBuffer(uint32_t name
            , int32_t width
            , int32_t height
            , uint32_t format
            , int32_t offsets[3]
            , int32_t pitches[3]);
        virtual struct wl_buffer * CreatePrimeBuffer(uint32_t name
            , int32_t width
            , int32_t height
            , uint32_t format
            , int32_t offsets[3]
            , int32_t pitches[3]);
        struct wl_display * GetDisplay() { return m_display; }
        struct wl_registry * GetRegistry() { return m_registry; }
        struct wl_compositor * GetCompositor() { return m_compositor; }
        struct wl_shell * GetShell() { return m_shell; }
        struct wl_drm * GetDrm() { return m_drm; }
        struct wl_shm * GetShm() { return m_shm; };
        struct wl_shm_pool * GetShmPool() { return m_pool; }
        struct wl_surface * GetSurface() { return m_surface; }
        struct wl_shell_surface * GetShellSurface() { return m_shell_surface; }
        struct wl_callback * GetCallback() { return m_callback; }
        void SetCompositor(struct wl_compositor *compositor)
        {
            m_compositor = compositor;
        }
        void SetShell(struct wl_shell *shell)
        {
            m_shell = shell;
        }
        void SetShm(struct wl_shm *shm)
        {
            m_shm = shm;
        }
        void SetDrm(struct wl_drm *drm)
        {
            m_drm = drm;
        }
        void DrmHandleDevice(const char* device);
        void DrmHandleAuthenticated();
        void RegistryGlobal(struct wl_registry *registry
            , uint32_t name
            , const char *interface
            , uint32_t version);
        void DisplayFlush()
        {
            wl_display_flush(m_display);
        }
        virtual int DisplayRoundtrip();
        void DestroyCallback();
        virtual void Sync();
        virtual void SetPerfMode(bool perf_mode);
    private:
        //no copies allowed
        Wayland(const Wayland &);
        void operator = (const Wayland&);

        struct wl_display *m_display;
        struct wl_registry *m_registry;
        struct wl_compositor *m_compositor;
        struct wl_shell *m_shell;
        struct wl_drm *m_drm;
        struct wl_shm *m_shm;
        struct wl_shm_pool *m_pool;
        struct wl_surface *m_surface;
        struct wl_shell_surface *m_shell_surface;
        struct wl_callback *m_callback;
        struct wl_event_queue *m_event_queue;
        volatile int m_pending_frame;
        struct ShmPool *m_shm_pool;
        int m_display_fd;
        int m_fd;
        struct pollfd m_poll;
        drm_intel_bufmgr *m_bufmgr;
        char *m_device_name;
        int m_x, m_y;
        bool m_perf_mode;
};

extern "C" Wayland* WaylandCreate();
extern "C" void WaylandDestroy(Wayland *pWld);

#endif //LIBVA_WAYLAND_SUPPORT

#endif /* WAYLAND_CLASS_H */
