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
 * File Name: device_info.c
 *
 */

/* Source file content was taken from Intel GPU Tools */

#include "device_info.h"
#include "i915_pciids.h"

#include <strings.h> /* ffs() */

#define BIT(x) (1<<(x))

static const struct intel_device_info intel_generic_info = {
	.gen = 0,
};

static const struct intel_device_info intel_i810_info = {
	.gen = BIT(0),
	.is_whitney = true,
	.codename = "solano" /* 815 == "whitney" ? or vice versa? */
};

static const struct intel_device_info intel_i815_info = {
	.gen = BIT(0),
	.is_whitney = true,
	.codename = "whitney"
};

static const struct intel_device_info intel_i830_info = {
	.gen = BIT(1),
	.is_almador = true,
	.codename = "almador"
};
static const struct intel_device_info intel_i845_info = {
	.gen = BIT(1),
	.is_brookdale = true,
	.codename = "brookdale"
};
static const struct intel_device_info intel_i855_info = {
	.gen = BIT(1),
	.is_mobile = true,
	.is_montara = true,
	.codename = "montara"
};
static const struct intel_device_info intel_i865_info = {
	.gen = BIT(1),
	.is_springdale = true,
	.codename = "spingdale"
};

static const struct intel_device_info intel_i915_info = {
	.gen = BIT(2),
	.is_grantsdale = true,
	.codename = "grantsdale"
};
static const struct intel_device_info intel_i915m_info = {
	.gen = BIT(2),
	.is_mobile = true,
	.is_alviso = true,
	.codename = "alviso"
};
static const struct intel_device_info intel_i945_info = {
	.gen = BIT(2),
	.is_lakeport = true,
	.codename = "lakeport"
};
static const struct intel_device_info intel_i945m_info = {
	.gen = BIT(2),
	.is_mobile = true,
	.is_calistoga = true,
	.codename = "calistoga"
};

static const struct intel_device_info intel_g33_info = {
	.gen = BIT(2),
	.is_bearlake = true,
	.codename = "bearlake"
};
static const struct intel_device_info intel_pineview_info = {
	.gen = BIT(2),
	.is_mobile = true,
	.is_pineview = true,
	.codename = "pineview"
};

static const struct intel_device_info intel_i965_info = {
	.gen = BIT(3),
	.is_broadwater = true,
	.codename = "broadwater"
};

static const struct intel_device_info intel_i965m_info = {
	.gen = BIT(3),
	.is_mobile = true,
	.is_crestline = true,
	.codename = "crestline"
};

static const struct intel_device_info intel_g45_info = {
	.gen = BIT(3),
	.is_eaglelake = true,
	.codename = "eaglelake"
};
static const struct intel_device_info intel_gm45_info = {
	.gen = BIT(3),
	.is_mobile = true,
	.is_cantiga = true,
	.codename = "cantiga"
};

static const struct intel_device_info intel_ironlake_info = {
	.gen = BIT(4),
	.is_ironlake = true,
	.codename = "ironlake" /* clarkdale? */
};
static const struct intel_device_info intel_ironlake_m_info = {
	.gen = BIT(4),
	.is_mobile = true,
	.is_arrandale = true,
	.codename = "arrandale"
};

static const struct intel_device_info intel_sandybridge_info = {
	.gen = BIT(5),
	.is_sandybridge = true,
	.codename = "sandybridge"
};
static const struct intel_device_info intel_sandybridge_m_info = {
	.gen = BIT(5),
	.is_mobile = true,
	.is_sandybridge = true,
	.codename = "sandybridge"
};

static const struct intel_device_info intel_ivybridge_info = {
	.gen = BIT(6),
	.is_ivybridge = true,
	.codename = "ivybridge"
};
static const struct intel_device_info intel_ivybridge_m_info = {
	.gen = BIT(6),
	.is_mobile = true,
	.is_ivybridge = true,
	.codename = "ivybridge"
};

static const struct intel_device_info intel_valleyview_info = {
	.gen = BIT(6),
	.is_valleyview = true,
	.codename = "valleyview"
};

#define HASWELL_FIELDS \
	.gen = BIT(6), \
	.is_haswell = true, \
	.codename = "haswell"

static const struct intel_device_info intel_haswell_gt1_info = {
	HASWELL_FIELDS,
	.gt = 1,
};

static const struct intel_device_info intel_haswell_gt2_info = {
	HASWELL_FIELDS,
	.gt = 2,
};

static const struct intel_device_info intel_haswell_gt3_info = {
	HASWELL_FIELDS,
	.gt = 3,
};

#define BROADWELL_FIELDS \
	.gen = BIT(7), \
	.is_broadwell = true, \
	.codename = "broadwell"

static const struct intel_device_info intel_broadwell_gt1_info = {
	BROADWELL_FIELDS,
	.gt = 1,
};

static const struct intel_device_info intel_broadwell_gt2_info = {
	BROADWELL_FIELDS,
	.gt = 2,
};

static const struct intel_device_info intel_broadwell_gt3_info = {
	BROADWELL_FIELDS,
	.gt = 3,
};

static const struct intel_device_info intel_broadwell_unknown_info = {
	BROADWELL_FIELDS,
};

static const struct intel_device_info intel_cherryview_info = {
	.gen = BIT(7),
	.is_cherryview = true,
	.codename = "cherryview"
};

#define SKYLAKE_FIELDS \
	.gen = BIT(8), \
	.codename = "skylake", \
	.is_skylake = true

static const struct intel_device_info intel_skylake_gt1_info = {
	SKYLAKE_FIELDS,
	.gt = 1,
};

static const struct intel_device_info intel_skylake_gt2_info = {
	SKYLAKE_FIELDS,
	.gt = 2,
};

static const struct intel_device_info intel_skylake_gt3_info = {
	SKYLAKE_FIELDS,
	.gt = 3,
};

static const struct intel_device_info intel_skylake_gt4_info = {
	SKYLAKE_FIELDS,
	.gt = 4,
};

static const struct intel_device_info intel_broxton_info = {
	.gen = BIT(8),
	.is_broxton = true,
	.codename = "broxton"
};

#define KABYLAKE_FIELDS \
	.gen = BIT(8), \
	.is_kabylake = true, \
	.codename = "kabylake"

static const struct intel_device_info intel_kabylake_gt1_info = {
	KABYLAKE_FIELDS,
	.gt = 1,
};

static const struct intel_device_info intel_kabylake_gt2_info = {
	KABYLAKE_FIELDS,
	.gt = 2,
};

static const struct intel_device_info intel_kabylake_gt3_info = {
	KABYLAKE_FIELDS,
	.gt = 3,
};

static const struct intel_device_info intel_kabylake_gt4_info = {
	KABYLAKE_FIELDS,
	.gt = 4,
};

static const struct intel_device_info intel_geminilake_info = {
	.gen = BIT(8),
	.is_geminilake = true,
	.codename = "geminilake"
};

#define COFFEELAKE_FIELDS \
	.gen = BIT(8), \
	.is_coffeelake = true, \
	.codename = "coffeelake"

static const struct intel_device_info intel_coffeelake_gt1_info = {
	COFFEELAKE_FIELDS,
	.gt = 1,
};

static const struct intel_device_info intel_coffeelake_gt2_info = {
	COFFEELAKE_FIELDS,
	.gt = 2,
};

static const struct intel_device_info intel_coffeelake_gt3_info = {
	COFFEELAKE_FIELDS,
	.gt = 3,
};

static const struct intel_device_info intel_cannonlake_info = {
	.gen = BIT(9),
	.is_cannonlake = true,
	.codename = "cannonlake"
};

static const struct intel_device_info intel_icelake_info = {
	.gen = BIT(10),
	.is_icelake = true,
	.codename = "icelake"
};

const struct pci_id_match intel_device_match [] = {
    INTEL_I810_IDS(&intel_i810_info),
    INTEL_I815_IDS(&intel_i815_info),

    INTEL_I830_IDS(&intel_i830_info),
    INTEL_I845G_IDS(&intel_i845_info),
    INTEL_I85X_IDS(&intel_i855_info),
    INTEL_I865G_IDS(&intel_i865_info),

    INTEL_I915G_IDS(&intel_i915_info),
    INTEL_I915GM_IDS(&intel_i915m_info),
    INTEL_I945G_IDS(&intel_i945_info),
    INTEL_I945GM_IDS(&intel_i945m_info),

    INTEL_G33_IDS(&intel_g33_info),
    INTEL_PINEVIEW_IDS(&intel_pineview_info),

    INTEL_I965G_IDS(&intel_i965_info),
    INTEL_I965GM_IDS(&intel_i965m_info),

    INTEL_G45_IDS(&intel_g45_info),
    INTEL_GM45_IDS(&intel_gm45_info),

    INTEL_IRONLAKE_D_IDS(&intel_ironlake_info),
    INTEL_IRONLAKE_M_IDS(&intel_ironlake_m_info),

    INTEL_SNB_D_IDS(&intel_sandybridge_info),
    INTEL_SNB_M_IDS(&intel_sandybridge_m_info),

    INTEL_IVB_D_IDS(&intel_ivybridge_info),
    INTEL_IVB_M_IDS(&intel_ivybridge_m_info),

    INTEL_HSW_GT1_IDS(&intel_haswell_gt1_info),
    INTEL_HSW_GT2_IDS(&intel_haswell_gt2_info),
    INTEL_HSW_GT3_IDS(&intel_haswell_gt3_info),

    INTEL_VLV_IDS(&intel_valleyview_info),

    INTEL_BDW_GT1_IDS(&intel_broadwell_gt1_info),
    INTEL_BDW_GT2_IDS(&intel_broadwell_gt2_info),
    INTEL_BDW_GT3_IDS(&intel_broadwell_gt3_info),
    INTEL_BDW_RSVD_IDS(&intel_broadwell_unknown_info),

    INTEL_CHV_IDS(&intel_cherryview_info),

    INTEL_SKL_GT1_IDS(&intel_skylake_gt1_info),
    INTEL_SKL_GT2_IDS(&intel_skylake_gt2_info),
    INTEL_SKL_GT3_IDS(&intel_skylake_gt3_info),
    INTEL_SKL_GT4_IDS(&intel_skylake_gt4_info),

    INTEL_BXT_IDS(&intel_broxton_info),

    INTEL_KBL_GT1_IDS(&intel_kabylake_gt1_info),
    INTEL_KBL_GT2_IDS(&intel_kabylake_gt2_info),
    INTEL_KBL_GT3_IDS(&intel_kabylake_gt3_info),
    INTEL_KBL_GT4_IDS(&intel_kabylake_gt4_info),
    INTEL_AML_KBL_GT2_IDS(&intel_kabylake_gt2_info),

    INTEL_GLK_IDS(&intel_geminilake_info),

    INTEL_CFL_S_GT1_IDS(&intel_coffeelake_gt1_info),
    INTEL_CFL_S_GT2_IDS(&intel_coffeelake_gt2_info),
    INTEL_CFL_H_GT2_IDS(&intel_coffeelake_gt2_info),
    INTEL_CFL_U_GT2_IDS(&intel_coffeelake_gt2_info),
    INTEL_CFL_U_GT3_IDS(&intel_coffeelake_gt3_info),
    INTEL_WHL_U_GT1_IDS(&intel_coffeelake_gt1_info),
    INTEL_WHL_U_GT2_IDS(&intel_coffeelake_gt2_info),
    INTEL_WHL_U_GT3_IDS(&intel_coffeelake_gt3_info),
    INTEL_AML_CFL_GT2_IDS(&intel_coffeelake_gt2_info),

    INTEL_CNL_IDS(&intel_cannonlake_info),

    INTEL_ICL_11_IDS(&intel_icelake_info),

    INTEL_VGA_DEVICE(PCI_MATCH_ANY, &intel_generic_info),
};

const struct intel_device_info *intel_get_device_info(uint16_t devid)
{
    static const struct intel_device_info *cache = &intel_generic_info;
    static uint16_t cached_devid;
    int i;

    if (cached_devid == devid)
        goto out;

            /* XXX Presort table and bsearch! */
    for (i = 0; intel_device_match[i].device_id != PCI_MATCH_ANY; i++) {
        if (devid == intel_device_match[i].device_id)
            break;
    }

    cached_devid = devid;
    cache = (void *)intel_device_match[i].match_data;

out:
    return cache;
}

unsigned intel_gen(uint16_t devid)
{
    return ffs(intel_get_device_info(devid)->gen);
}

unsigned intel_gt(uint16_t devid)
{
    const struct intel_device_info *devinfo = intel_get_device_info(devid);

    /* If in the database, just use that information. */
    if (devinfo->gt != 0)
        return devinfo->gt - 1;

    /*
     * This scheme doesn't work on Coffeelake, we should probably
     * not rely on this anymore.
    */

    unsigned mask = intel_gen(devid);

    if (mask >= 8)
        mask = 0xf;
    else if (mask >= 6)
        mask = 0x3;
    else
        mask = 0;

    return (devid >> 4) & mask;
}
