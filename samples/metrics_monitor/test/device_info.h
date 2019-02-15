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
 * File Name: device_info.h
 *
 */

/* Source file content was taken from Intel GPU Tools */

#ifndef __DEVICE_INFO_H__
#define __DEVICE_INFO_H__

#include <pciaccess.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BIT(x) (1<<(x))

struct intel_device_info {
    unsigned gen;
    unsigned gt; /* 0 if unknown */
    bool is_mobile : 1;
    bool is_whitney : 1;
    bool is_almador : 1;
    bool is_brookdale : 1;
    bool is_montara : 1;
    bool is_springdale : 1;
    bool is_grantsdale : 1;
    bool is_alviso : 1;
    bool is_lakeport : 1;
    bool is_calistoga : 1;
    bool is_bearlake : 1;
    bool is_pineview : 1;
    bool is_broadwater : 1;
    bool is_crestline : 1;
    bool is_eaglelake : 1;
    bool is_cantiga : 1;
    bool is_ironlake : 1;
    bool is_arrandale : 1;
    bool is_sandybridge : 1;
    bool is_ivybridge : 1;
    bool is_valleyview : 1;
    bool is_haswell : 1;
    bool is_broadwell : 1;
    bool is_cherryview : 1;
    bool is_skylake : 1;
    bool is_broxton : 1;
    bool is_kabylake : 1;
    bool is_geminilake : 1;
    bool is_coffeelake : 1;
    bool is_cannonlake : 1;
    bool is_icelake : 1;
    const char *codename;
};

const struct intel_device_info *intel_get_device_info(uint16_t devid);

unsigned intel_gen(uint16_t devid);
unsigned intel_gt(uint16_t devid);

#define IS_BROXTON(devid)	(intel_get_device_info(devid)->is_broxton)
#define IS_SKYLAKE(devid)	(intel_get_device_info(devid)->is_skylake)

#ifdef __cplusplus
}
#endif

#endif /* __DEVICE_INFO_H__ */
