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
 * File Name: igt_load.h
 *
 */

/* Source file content was taken from Intel GPU Tools */

#ifndef __IGT_LOAD_H__
#define __IGT_LOAD_H__

#include <fcntl.h>
#include <i915_drm.h>
#include <pciaccess.h>
#include "i915_pciids.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct igt_spin igt_spin_t;

bool gem_has_bsd2(int fd);
uint16_t intel_get_drm_devid();
igt_spin_t * igt_spin_batch_new(int fd, uint32_t ctx, unsigned engine, uint32_t dep);
void igt_spin_batch_free(int fd, igt_spin_t *spin);

#ifdef __cplusplus
}
#endif

#endif /* __IGT_LOAD_H__ */
