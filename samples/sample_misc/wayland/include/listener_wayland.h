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

#ifndef LISTENER_WAYLAND_H
#define LISTENER_WAYLAND_H

#include "class_wayland.h"

/* drm listeners */
void drm_handle_device(void *data
    , struct wl_drm *drm
    , const char *device);

void drm_handle_format(void *data
    , struct wl_drm *drm
    , uint32_t format);

void drm_handle_authenticated(void *data
    , struct wl_drm *drm);

void drm_handle_capabilities(void *data
    , struct wl_drm *drm
    , uint32_t value);

/* registry listeners */
void registry_handle_global(void *data
    , struct wl_registry *registry
    , uint32_t name
    , const char *interface
    , uint32_t version);

void remove_registry_global(void *data
    , struct wl_registry *regsitry
    , uint32_t name);

/* surface listener */
void shell_surface_ping(void *data
    , struct wl_shell_surface *shell_surface
    , uint32_t serial);

void shell_surface_configure(void *data
    , struct wl_shell_surface *shell_surface
    , uint32_t edges
    , int32_t width
    , int32_t height);

void handle_done(void *data, struct wl_callback *callback, uint32_t time);

void buffer_release(void *data, struct wl_buffer *buffer);
#endif /* LISTENER_WAYLAND_H */
