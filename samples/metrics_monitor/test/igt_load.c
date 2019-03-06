/*
 * Copyright (C) 2017-2019 Intel Corporation.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * File Name: igt_load.c
 *
 */

/* Source file content was taken from Intel GPU Tools */

#include <errno.h>
#include <time.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <asm-generic/int-ll64.h>
#include <asm-generic/errno-base.h>
#include <xf86drm.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include "device_info.h"
#include "igt_load.h"

struct igt_list {
    struct igt_list *prev;
    struct igt_list *next;
};

typedef struct igt_spin {
    unsigned int handle;
    timer_t timer;
    struct igt_list link;
    uint32_t *batch;
} igt_spin_t;

struct local_i915_gem_mmap_v2 {
    uint32_t handle;
    uint32_t pad;
    uint64_t offset;
    uint64_t size;
    uint64_t addr_ptr;
    uint64_t flags;
#define I915_MMAP_WC 0x1
};

const struct intel_execution_engine {
    const char *name;
    const char *full_name;
    unsigned exec_id;
    unsigned flags;
} intel_execution_engines[];

const struct intel_execution_engine intel_execution_engines[] = {
    {"default", NULL, 0, 0},
    {"render", "rcs0", I915_EXEC_RENDER, 0},
    {"bsd", "vcs0", I915_EXEC_BSD, 0},
    {"bsd1", "vcs0", I915_EXEC_BSD, 1 << 13 /*I915_EXEC_BSD_RING1*/},
    {"bsd2", "vcs1", I915_EXEC_BSD, 2 << 13 /*I915_EXEC_BSD_RING2*/},
    {"blt", "bcs0", I915_EXEC_BLT, 0},
    {"vebox", "vecs0", I915_EXEC_VEBOX, 0},
    {NULL, 0, 0}
};

const int BATCH_SIZE = 4096;

#define MI_BATCH_BUFFER_START	(0x31 << 23)
#define MI_BATCH_BUFFER_END	(0xA << 23)
#define MI_ARB_CHK (0x5 << 23)
#define LOCAL_I915_EXEC_BSD_SHIFT      (13)
#define LOCAL_I915_EXEC_BSD_MASK       (3 << LOCAL_I915_EXEC_BSD_SHIFT)
#define ENGINE_MASK  (I915_EXEC_RING_MASK | LOCAL_I915_EXEC_BSD_MASK)
#define LOCAL_I915_PARAM_HAS_BSD2 31
#define LOCAL_IOCTL_I915_GEM_MMAP_v2 DRM_IOWR(DRM_COMMAND_BASE + DRM_I915_GEM_MMAP, struct local_i915_gem_mmap_v2)

#ifdef HAVE_VALGRIND
#include <valgrind/valgrind.h>
#include <valgrind/memcheck.h>

#define VG(x) x
#else
#define VG(x) do {} while (0)
#endif

#define for_if(expr__) if (!(expr__)) {} else

#define for_each_engine(fd__, flags__) \
    for (const struct intel_execution_engine *e__ = intel_execution_engines;\
         e__->name; \
         e__++) \
        for_if (gem_has_ring(fd__, flags__ = e__->exec_id | e__->flags))

#define do_ioctl(fd, ioc, ioc_data) do { \
    igt_ioctl((fd), (ioc), (ioc_data)); \
    errno = 0; \
} while (0)

#define __IGT_INIT_LIST(name) { &(name), &(name) }
#define IGT_LIST(name) struct igt_list name = __IGT_INIT_LIST(name)

static IGT_LIST(spin_list);

static inline uint64_t to_user_pointer(const void *ptr)
{
    return (uintptr_t)ptr;
}

static inline void *from_user_pointer(uint64_t u64)
{
    return (void *)(uintptr_t)u64;
}

static inline void igt_list_add(struct igt_list *elm, struct igt_list *list)
{
    list->next->prev = elm;
    elm->next = list->next;
    elm->prev = list;
    list->next = elm;
}

static inline void igt_list_del(struct igt_list *elm)
{
    elm->next->prev = elm->prev;
    elm->prev->next = elm->next;
}

int(*igt_ioctl)(int fd, unsigned long request, void *arg) = drmIoctl;

uint16_t intel_get_drm_devid()
{
    uint16_t __drm_device_id = 0;

    int fd = -1;
    char buf[8] = {0};
    char *end;

    fd = open("/sys/devices/pci0000:00/0000:00:02.0/device", O_RDONLY);
    if (fd >= 0)
    {
        ssize_t bytes_read = read(fd, buf, sizeof(buf));
        if (bytes_read != -1)
        {
            __drm_device_id = (uint16_t)strtol(buf, &end, 16);
        }
        close(fd);
    }

    return __drm_device_id;
}

static bool has_param(int fd, int param)
{
    drm_i915_getparam_t gp;
    int tmp = 0;

    memset(&gp, 0, sizeof(gp));
    gp.value = &tmp;
    gp.param = param;

    if (igt_ioctl(fd, DRM_IOCTL_I915_GETPARAM, &gp))
        return false;

    errno = 0;
    return tmp > 0;
}

bool gem_has_bsd2(int fd)
{
    static int has_bsd2 = -1;
    if (has_bsd2 < 0)
        has_bsd2 = has_param(fd, LOCAL_I915_PARAM_HAS_BSD2);
    return has_bsd2;
}

int gem_execbuf(int fd, struct drm_i915_gem_execbuffer2 *execbuf)
{
    int err = 0;
    if (igt_ioctl(fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, execbuf))
        err = -errno;
    errno = 0;
    return err;
}

bool gem_has_ring(int fd, unsigned ring)
{
    struct drm_i915_gem_execbuffer2 execbuf;
    struct drm_i915_gem_exec_object2 exec;

        /* silly ABI, the kernel thinks everyone who has BSD also has BSD2 */
    if ((ring & ~(3 << 13)) == I915_EXEC_BSD) {
        if (ring & (3 << 13) && !gem_has_bsd2(fd))
            return false;
    }

    memset(&exec, 0, sizeof(exec));
    memset(&execbuf, 0, sizeof(execbuf));
    execbuf.buffers_ptr = to_user_pointer(&exec);
    execbuf.buffer_count = 1;
    execbuf.flags = ring;
    return gem_execbuf(fd, &execbuf) == -ENOENT;
}

static void fill_reloc(struct drm_i915_gem_relocation_entry *reloc,
    uint32_t gem_handle,
    uint32_t offset,
    uint32_t read_domains,
    uint32_t write_domains)
{
    reloc->target_handle = gem_handle;
    reloc->offset = offset * sizeof(uint32_t);
    reloc->read_domains = read_domains;
    reloc->write_domain = write_domains;
}

int gem_set_domain(int fd, uint32_t handle, uint32_t read, uint32_t write)
{
    struct drm_i915_gem_set_domain set_domain;
    int err;

    memset(&set_domain, 0, sizeof(set_domain));
    set_domain.handle = handle;
    set_domain.read_domains = read;
    set_domain.write_domain = write;

    err = 0;
    if (igt_ioctl(fd, DRM_IOCTL_I915_GEM_SET_DOMAIN, &set_domain))
        err = -errno;

    return err;
}

void gem_close(int fd, uint32_t handle)
{
    struct drm_gem_close close_bo;

    memset(&close_bo, 0, sizeof(close_bo));
    close_bo.handle = handle;
    igt_ioctl(fd, DRM_IOCTL_GEM_CLOSE, &close_bo);
}

uint32_t gem_create(int fd, uint64_t size)
{
    uint32_t handle = 0;

    struct drm_i915_gem_create create = {
        .size = size,
    };

    if (igt_ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create) == 0)
        handle = create.handle;

    return handle;
}

bool gem_mmap__has_wc(int fd)
{
    static int has_wc = -1;

    if (has_wc == -1) {
        struct drm_i915_getparam gp;
        int mmap_version = -1;
        int gtt_version = -1;

        has_wc = 0;

        memset(&gp, 0, sizeof(gp));
        gp.param = 40; /* MMAP_GTT_VERSION */
        gp.value = &gtt_version;
        ioctl(fd, DRM_IOCTL_I915_GETPARAM, &gp);

        memset(&gp, 0, sizeof(gp));
        gp.param = 30; /* MMAP_VERSION */
        gp.value = &mmap_version;
        ioctl(fd, DRM_IOCTL_I915_GETPARAM, &gp);

                /* Do we have the new mmap_ioctl with DOMAIN_WC? */
        if (mmap_version >= 1 && gtt_version >= 2) {
            struct local_i915_gem_mmap_v2 arg;

                        /* Does this device support wc-mmaps ? */
            memset(&arg, 0, sizeof(arg));
            arg.handle = gem_create(fd, 4096);
            arg.offset = 0;
            arg.size = 4096;
            arg.flags = I915_MMAP_WC;
            has_wc = igt_ioctl(fd, LOCAL_IOCTL_I915_GEM_MMAP_v2, &arg) == 0;
            gem_close(fd, arg.handle);
        }
        errno = 0;
    }

    return has_wc > 0;
}

void *gem_mmap__wc(int fd, uint32_t handle, uint64_t offset, uint64_t size, unsigned prot)
{
    struct local_i915_gem_mmap_v2 arg;

    if (!gem_mmap__has_wc(fd)) {
        errno = ENOSYS;
        return NULL;
    }

    memset(&arg, 0, sizeof(arg));
    arg.handle = handle;
    arg.offset = offset;
    arg.size = size;
    arg.flags = I915_MMAP_WC;
    if (igt_ioctl(fd, LOCAL_IOCTL_I915_GEM_MMAP_v2, &arg))
        return NULL;

    VG(VALGRIND_MAKE_MEM_DEFINED(from_user_pointer(arg.addr_ptr), arg.size));

    errno = 0;
    return from_user_pointer(arg.addr_ptr);
}

void *gem_mmap__gtt(int fd, uint32_t handle, uint64_t size, unsigned prot)
{
    struct drm_i915_gem_mmap_gtt mmap_arg;
    void *ptr;

    memset(&mmap_arg, 0, sizeof(mmap_arg));
    mmap_arg.handle = handle;
    if (igt_ioctl(fd, DRM_IOCTL_I915_GEM_MMAP_GTT, &mmap_arg))
        return NULL;

    ptr = mmap(0, size, prot, MAP_SHARED, fd, mmap_arg.offset);
    if (ptr == MAP_FAILED)
        ptr = NULL;
    else
        errno = 0;

    VG(VALGRIND_MAKE_MEM_DEFINED(ptr, size));

    return ptr;
}

bool gem_bo_busy(int fd, uint32_t handle)
{
    struct drm_i915_gem_busy busy;

    memset(&busy, 0, sizeof(busy));
    busy.handle = handle;

    do_ioctl(fd, DRM_IOCTL_I915_GEM_BUSY, &busy);

    return !! busy.busy;
}

int gem_munmap(void *ptr, uint64_t size)
{
    int ret = munmap(ptr, size);

    if (ret == 0)
        VG(VALGRIND_MAKE_MEM_NOACCESS(ptr, size));

    return ret;
}

static void emit_recursive_batch(igt_spin_t *spin,
    int fd,
    uint32_t ctx,
    unsigned engine,
    uint32_t dep)
{
#define SCRATCH 0
#define BATCH 1
    const int gen = intel_gen(intel_get_drm_devid());
    struct drm_i915_gem_exec_object2 obj[2];
    struct drm_i915_gem_relocation_entry relocs[2];
    struct drm_i915_gem_execbuffer2 execbuf;
    unsigned int engines[16];
    unsigned int nengine;
    uint32_t *batch;
    unsigned int i;

    nengine = 0;
    if ((int)engine == -1) {
        for_each_engine(fd, engine)
            if(engine)
                engines[nengine++] = engine;
    }
    else {
        gem_has_ring(fd, engine);
        engines[nengine++] = engine;
    }

    memset(&execbuf, 0, sizeof(execbuf));
    memset(obj, 0, sizeof(obj));
    memset(relocs, 0, sizeof(relocs));

    obj[BATCH].handle = gem_create(fd, BATCH_SIZE);
    batch = (uint32_t*)gem_mmap__wc(fd,
        obj[BATCH].handle,
        0,
        BATCH_SIZE,
        PROT_WRITE);
    if (!batch)
        batch = (uint32_t*)gem_mmap__gtt(fd,
            obj[BATCH].handle,
            BATCH_SIZE,
            PROT_WRITE);
    gem_set_domain(fd,
        obj[BATCH].handle,
        I915_GEM_DOMAIN_GTT,
        I915_GEM_DOMAIN_GTT);
    execbuf.buffer_count++;

    if (dep) {
        /* dummy write to dependency */
        obj[SCRATCH].handle = dep;
        fill_reloc(&relocs[obj[BATCH].relocation_count++],
            dep,
            1020,
            I915_GEM_DOMAIN_RENDER,
            I915_GEM_DOMAIN_RENDER);
        execbuf.buffer_count++;
    }

    spin->batch = batch;
    spin->handle = obj[BATCH].handle;

        /* Allow ourselves to be preempted */
    *batch++ = MI_ARB_CHK;

    batch += 1000;

        /* recurse */
    fill_reloc(&relocs[obj[BATCH].relocation_count],
        obj[BATCH].handle,
        (batch - spin->batch) + 1,
        I915_GEM_DOMAIN_COMMAND,
        0);
    if (gen >= 8) {
        *batch++ = MI_BATCH_BUFFER_START | 1 << 8 | 1;
        *batch++ = 0;
        *batch++ = 0;
    }
    else if (gen >= 6) {
        *batch++ = MI_BATCH_BUFFER_START | 1 << 8;
        *batch++ = 0;
    }
    else {
        *batch++ = MI_BATCH_BUFFER_START | 2 << 6;
        *batch = 0;
        if (gen < 4) {
            *batch |= 1;
            relocs[obj[BATCH].relocation_count].delta = 1;
        }
        batch++;
    }
    obj[BATCH].relocation_count++;
    obj[BATCH].relocs_ptr = to_user_pointer(relocs);

    execbuf.buffers_ptr = to_user_pointer(obj + (2 - execbuf.buffer_count));
    execbuf.rsvd1 = ctx;

    for (i = 0; i < nengine; i++) {
        execbuf.flags &= ~ENGINE_MASK;
        execbuf.flags = engines[i];
        gem_execbuf(fd, &execbuf);
    }
}

igt_spin_t * igt_spin_batch_new(int fd, uint32_t ctx, unsigned engine, uint32_t dep)
{
    igt_spin_t *spin;

    spin = (igt_spin_t*)calloc(1, sizeof(struct igt_spin));

    if (!spin)
        return NULL;

    emit_recursive_batch(spin, fd, ctx, engine, dep);
    gem_bo_busy(fd, spin->handle);

    igt_list_add(&spin->link, &spin_list);

    return spin;
}

void igt_spin_batch_end(igt_spin_t *spin)
{
    if (!spin)
        return;

    *spin->batch = MI_BATCH_BUFFER_END;
    __sync_synchronize();
}

void igt_spin_batch_free(int fd, igt_spin_t *spin)
{
    if (!spin)
        return;

    igt_list_del(&spin->link);

    if (spin->timer)
        timer_delete(spin->timer);

    igt_spin_batch_end(spin);
    gem_munmap(spin->batch, BATCH_SIZE);

    gem_close(fd, spin->handle);
    free(spin);
}