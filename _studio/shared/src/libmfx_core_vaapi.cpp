// Copyright (c) 2017-2018 Intel Corporation
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <iostream>

#include "mfx_common.h"

#if defined (MFX_VA_LINUX)

#include "umc_va_linux.h"

#include "libmfx_core_vaapi.h"
#include "mfx_utils.h"
#include "mfx_session.h"
#include "mfx_common_decode_int.h"
#include "mfx_enc_common.h"
#include "mfxfei.h"
#include "umc_va_fei.h"

#include "cm_mem_copy.h"

#include <sys/ioctl.h>

#include "va/va.h"
#include <va/va_backend.h>

#define MFX_CHECK_HDL(hdl) {if (!hdl) MFX_RETURN(MFX_ERR_INVALID_HANDLE);}

typedef struct drm_i915_getparam {
    int param;
    int *value;
} drm_i915_getparam_t;

#define I915_PARAM_CHIPSET_ID            4
#define DRM_I915_GETPARAM   0x06
#define DRM_IOCTL_BASE          'd'
#define DRM_COMMAND_BASE                0x40
#define DRM_IOWR(nr,type)       _IOWR(DRM_IOCTL_BASE,nr,type)
#define DRM_IOCTL_I915_GETPARAM         DRM_IOWR(DRM_COMMAND_BASE + DRM_I915_GETPARAM, drm_i915_getparam_t)

typedef struct {
    int device_id;
    eMFXHWType platform;
    eMFXGTConfig config;
} mfx_device_item;

// list of legal dev ID for Intel's graphics
 const mfx_device_item listLegalDevIDs[] = {
    /*IVB*/
    { 0x0156, MFX_HW_IVB, MFX_GT1 },   /* GT1 mobile */
    { 0x0166, MFX_HW_IVB, MFX_GT2 },   /* GT2 mobile */
    { 0x0152, MFX_HW_IVB, MFX_GT1 },   /* GT1 desktop */
    { 0x0162, MFX_HW_IVB, MFX_GT2 },   /* GT2 desktop */
    { 0x015a, MFX_HW_IVB, MFX_GT1 },   /* GT1 server */
    { 0x016a, MFX_HW_IVB, MFX_GT2 },   /* GT2 server */
    /*HSW*/
    { 0x0402, MFX_HW_HSW, MFX_GT1 },   /* GT1 desktop */
    { 0x0412, MFX_HW_HSW, MFX_GT2 },   /* GT2 desktop */
    { 0x0422, MFX_HW_HSW, MFX_GT2 },   /* GT2 desktop */
    { 0x041e, MFX_HW_HSW, MFX_GT2 },   /* Core i3-4130 */
    { 0x040a, MFX_HW_HSW, MFX_GT1 },   /* GT1 server */
    { 0x041a, MFX_HW_HSW, MFX_GT2 },   /* GT2 server */
    { 0x042a, MFX_HW_HSW, MFX_GT2 },   /* GT2 server */
    { 0x0406, MFX_HW_HSW, MFX_GT1 },   /* GT1 mobile */
    { 0x0416, MFX_HW_HSW, MFX_GT2 },   /* GT2 mobile */
    { 0x0426, MFX_HW_HSW, MFX_GT2 },   /* GT2 mobile */
    { 0x0C02, MFX_HW_HSW, MFX_GT1 },   /* SDV GT1 desktop */
    { 0x0C12, MFX_HW_HSW, MFX_GT2 },   /* SDV GT2 desktop */
    { 0x0C22, MFX_HW_HSW, MFX_GT2 },   /* SDV GT2 desktop */
    { 0x0C0A, MFX_HW_HSW, MFX_GT1 },   /* SDV GT1 server */
    { 0x0C1A, MFX_HW_HSW, MFX_GT2 },   /* SDV GT2 server */
    { 0x0C2A, MFX_HW_HSW, MFX_GT2 },   /* SDV GT2 server */
    { 0x0C06, MFX_HW_HSW, MFX_GT1 },   /* SDV GT1 mobile */
    { 0x0C16, MFX_HW_HSW, MFX_GT2 },   /* SDV GT2 mobile */
    { 0x0C26, MFX_HW_HSW, MFX_GT2 },   /* SDV GT2 mobile */
    { 0x0A02, MFX_HW_HSW, MFX_GT1 },   /* ULT GT1 desktop */
    { 0x0A12, MFX_HW_HSW, MFX_GT2 },   /* ULT GT2 desktop */
    { 0x0A22, MFX_HW_HSW, MFX_GT2 },   /* ULT GT2 desktop */
    { 0x0A0A, MFX_HW_HSW, MFX_GT1 },   /* ULT GT1 server */
    { 0x0A1A, MFX_HW_HSW, MFX_GT2 },   /* ULT GT2 server */
    { 0x0A2A, MFX_HW_HSW, MFX_GT2 },   /* ULT GT2 server */
    { 0x0A06, MFX_HW_HSW, MFX_GT1 },   /* ULT GT1 mobile */
    { 0x0A16, MFX_HW_HSW, MFX_GT2 },   /* ULT GT2 mobile */
    { 0x0A26, MFX_HW_HSW, MFX_GT2 },   /* ULT GT2 mobile */
    { 0x0D02, MFX_HW_HSW, MFX_GT1 },   /* CRW GT1 desktop */
    { 0x0D12, MFX_HW_HSW, MFX_GT2 },   /* CRW GT2 desktop */
    { 0x0D22, MFX_HW_HSW, MFX_GT2 },   /* CRW GT2 desktop */
    { 0x0D0A, MFX_HW_HSW, MFX_GT1 },   /* CRW GT1 server */
    { 0x0D1A, MFX_HW_HSW, MFX_GT2 },   /* CRW GT2 server */
    { 0x0D2A, MFX_HW_HSW, MFX_GT2 },   /* CRW GT2 server */
    { 0x0D06, MFX_HW_HSW, MFX_GT1 },   /* CRW GT1 mobile */
    { 0x0D16, MFX_HW_HSW, MFX_GT2 },   /* CRW GT2 mobile */
    { 0x0D26, MFX_HW_HSW, MFX_GT2 },   /* CRW GT2 mobile */
    /* this dev IDs added per HSD 5264859 request  */
    { 0x040B, MFX_HW_HSW, MFX_GT1 }, /*HASWELL_B_GT1 *//* Reserved */
    { 0x041B, MFX_HW_HSW, MFX_GT2 }, /*HASWELL_B_GT2*/
    { 0x042B, MFX_HW_HSW, MFX_GT3 }, /*HASWELL_B_GT3*/
    { 0x040E, MFX_HW_HSW, MFX_GT1 }, /*HASWELL_E_GT1*//* Reserved */
    { 0x041E, MFX_HW_HSW, MFX_GT2 }, /*HASWELL_E_GT2*/
    { 0x042E, MFX_HW_HSW, MFX_GT3 }, /*HASWELL_E_GT3*/

    { 0x0C0B, MFX_HW_HSW, MFX_GT1 }, /*HASWELL_SDV_B_GT1*/ /* Reserved */
    { 0x0C1B, MFX_HW_HSW, MFX_GT2 }, /*HASWELL_SDV_B_GT2*/
    { 0x0C2B, MFX_HW_HSW, MFX_GT3 }, /*HASWELL_SDV_B_GT3*/
    { 0x0C0E, MFX_HW_HSW, MFX_GT1 }, /*HASWELL_SDV_B_GT1*//* Reserved */
    { 0x0C1E, MFX_HW_HSW, MFX_GT2 }, /*HASWELL_SDV_B_GT2*/
    { 0x0C2E, MFX_HW_HSW, MFX_GT3 }, /*HASWELL_SDV_B_GT3*/

    { 0x0A0B, MFX_HW_HSW, MFX_GT1 }, /*HASWELL_ULT_B_GT1*/ /* Reserved */
    { 0x0A1B, MFX_HW_HSW, MFX_GT2 }, /*HASWELL_ULT_B_GT2*/
    { 0x0A2B, MFX_HW_HSW, MFX_GT3 }, /*HASWELL_ULT_B_GT3*/
    { 0x0A0E, MFX_HW_HSW, MFX_GT1 }, /*HASWELL_ULT_E_GT1*/ /* Reserved */
    { 0x0A1E, MFX_HW_HSW, MFX_GT2 }, /*HASWELL_ULT_E_GT2*/
    { 0x0A2E, MFX_HW_HSW, MFX_GT3 }, /*HASWELL_ULT_E_GT3*/

    { 0x0D0B, MFX_HW_HSW, MFX_GT1 }, /*HASWELL_CRW_B_GT1*/ /* Reserved */
    { 0x0D1B, MFX_HW_HSW, MFX_GT2 }, /*HASWELL_CRW_B_GT2*/
    { 0x0D2B, MFX_HW_HSW, MFX_GT3 }, /*HASWELL_CRW_B_GT3*/
    { 0x0D0E, MFX_HW_HSW, MFX_GT1 }, /*HASWELL_CRW_E_GT1*/ /* Reserved */
    { 0x0D1E, MFX_HW_HSW, MFX_GT2 }, /*HASWELL_CRW_E_GT2*/
    { 0x0D2E, MFX_HW_HSW, MFX_GT3 }, /*HASWELL_CRW_E_GT3*/

    /* VLV */
    { 0x0f30, MFX_HW_VLV, MFX_GT1 },   /* VLV mobile */
    { 0x0f31, MFX_HW_VLV, MFX_GT1 },   /* VLV mobile */
    { 0x0f32, MFX_HW_VLV, MFX_GT1 },   /* VLV mobile */
    { 0x0f33, MFX_HW_VLV, MFX_GT1 },   /* VLV mobile */
    { 0x0157, MFX_HW_VLV, MFX_GT1 },
    { 0x0155, MFX_HW_VLV, MFX_GT1 },

    /* BDW */
    /*GT3: */
    { 0x162D, MFX_HW_BDW, MFX_GT3},
    { 0x162A, MFX_HW_BDW, MFX_GT3},
    /*GT2: */
    { 0x161D, MFX_HW_BDW, MFX_GT2 },
    { 0x161A, MFX_HW_BDW, MFX_GT2 },
    /* GT1: */
    { 0x160D, MFX_HW_BDW, MFX_GT1 },
    { 0x160A, MFX_HW_BDW, MFX_GT1 },
    /* BDW-ULT */
    /* (16x2 - ULT, 16x6 - ULT, 16xB - Iris, 16xE - ULX) */
    /*GT3: */
    { 0x162E, MFX_HW_BDW, MFX_GT3},
    { 0x162B, MFX_HW_BDW, MFX_GT3},
    { 0x1626, MFX_HW_BDW, MFX_GT3},
    { 0x1622, MFX_HW_BDW, MFX_GT3},
    /* GT2: */
    { 0x161E, MFX_HW_BDW, MFX_GT2 },
    { 0x161B, MFX_HW_BDW, MFX_GT2 },
    { 0x1616, MFX_HW_BDW, MFX_GT2 },
    { 0x1612, MFX_HW_BDW, MFX_GT2 },
    /* GT1: */
    { 0x160E, MFX_HW_BDW, MFX_GT1 },
    { 0x160B, MFX_HW_BDW, MFX_GT1 },
    { 0x1606, MFX_HW_BDW, MFX_GT1 },
    { 0x1602, MFX_HW_BDW, MFX_GT1 },

    /* CHT */
    { 0x22b0, MFX_HW_CHT, MFX_GT1 },
    { 0x22b1, MFX_HW_CHT, MFX_GT1 },
    { 0x22b2, MFX_HW_CHT, MFX_GT1 },
    { 0x22b3, MFX_HW_CHT, MFX_GT1 },

    /* SCL */
    /* GT1F */
    { 0x1902, MFX_HW_SCL, MFX_GT1 }, // DT, 2x1F, 510
    { 0x1906, MFX_HW_SCL, MFX_GT1 }, // U-ULT, 2x1F, 510
    { 0x190A, MFX_HW_SCL, MFX_GT1 }, // Server, 4x1F
    { 0x190B, MFX_HW_SCL, MFX_GT1 },
    { 0x190E, MFX_HW_SCL, MFX_GT1 }, // Y-ULX 2x1F
    /*GT1.5*/
    { 0x1913, MFX_HW_SCL, MFX_GT1 }, // U-ULT, 2x1.5
    { 0x1915, MFX_HW_SCL, MFX_GT1 }, // Y-ULX, 2x1.5
    { 0x1917, MFX_HW_SCL, MFX_GT1 }, // DT, 2x1.5
    /* GT2 */
    { 0x1912, MFX_HW_SCL, MFX_GT2 }, // DT, 2x2, 530
    { 0x1916, MFX_HW_SCL, MFX_GT2 }, // U-ULD 2x2, 520
    { 0x191A, MFX_HW_SCL, MFX_GT2 }, // 2x2,4x2, Server
    { 0x191B, MFX_HW_SCL, MFX_GT2 }, // DT, 2x2, 530
    { 0x191D, MFX_HW_SCL, MFX_GT2 }, // 4x2, WKS, P530
    { 0x191E, MFX_HW_SCL, MFX_GT2 }, // Y-ULX, 2x2, P510,515
    { 0x1921, MFX_HW_SCL, MFX_GT2 }, // U-ULT, 2x2F, 540
    /* GT3 */
    { 0x1923, MFX_HW_SCL, MFX_GT3 }, // U-ULT, 2x3, 535
    { 0x1926, MFX_HW_SCL, MFX_GT3 }, // U-ULT, 2x3, 540 (15W)
    { 0x1927, MFX_HW_SCL, MFX_GT3 }, // U-ULT, 2x3e, 550 (28W)
    { 0x192A, MFX_HW_SCL, MFX_GT3 }, // Server, 2x3
    { 0x192B, MFX_HW_SCL, MFX_GT3 }, // Halo 3e
    { 0x192D, MFX_HW_SCL, MFX_GT3 },
    /* GT4e*/
    { 0x1932, MFX_HW_SCL, MFX_GT4 }, // DT
    { 0x193A, MFX_HW_SCL, MFX_GT4 }, // SRV
    { 0x193B, MFX_HW_SCL, MFX_GT4 }, // Halo
    { 0x193D, MFX_HW_SCL, MFX_GT4 }, // WKS

    /* APL */
    { 0x0A84, MFX_HW_APL, MFX_GT1 },
    { 0x0A85, MFX_HW_APL, MFX_GT1 },
    { 0x0A86, MFX_HW_APL, MFX_GT1 },
    { 0x0A87, MFX_HW_APL, MFX_GT1 },
    { 0x1A84, MFX_HW_APL, MFX_GT1 },
    { 0x5A84, MFX_HW_APL, MFX_GT1 },
    { 0x5A85, MFX_HW_APL, MFX_GT1 },

    /* KBL */
    { 0x5902, MFX_HW_KBL, MFX_GT1 }, // DT GT1
    { 0x5906, MFX_HW_KBL, MFX_GT1 }, // ULT GT1
    { 0x5908, MFX_HW_KBL, MFX_GT1 }, // HALO GT1F
    { 0x590A, MFX_HW_KBL, MFX_GT1 }, // SERV GT1
    { 0x590B, MFX_HW_KBL, MFX_GT1 }, // HALO GT1
    { 0x590E, MFX_HW_KBL, MFX_GT1 }, // ULX GT1
    { 0x5912, MFX_HW_KBL, MFX_GT2 }, // DT GT2
    { 0x5913, MFX_HW_KBL, MFX_GT1 }, // ULT GT1 5
    { 0x5915, MFX_HW_KBL, MFX_GT1 }, // ULX GT1 5
    { 0x5916, MFX_HW_KBL, MFX_GT2 }, // ULT GT2
    { 0x5917, MFX_HW_KBL, MFX_GT2 }, // ULT GT2 R
    { 0x591A, MFX_HW_KBL, MFX_GT2 }, // SERV GT2
    { 0x591B, MFX_HW_KBL, MFX_GT2 }, // HALO GT2
    { 0x591D, MFX_HW_KBL, MFX_GT2 }, // WRK GT2
    { 0x591E, MFX_HW_KBL, MFX_GT2 }, // ULX GT2
    { 0x5921, MFX_HW_KBL, MFX_GT2 }, // ULT GT2F
    { 0x5923, MFX_HW_KBL, MFX_GT3 }, // ULT GT3
    { 0x5926, MFX_HW_KBL, MFX_GT3 }, // ULT GT3 15W
    { 0x5927, MFX_HW_KBL, MFX_GT3 }, // ULT GT3 28W
    { 0x592A, MFX_HW_KBL, MFX_GT3 }, // SERV GT3
    { 0x592B, MFX_HW_KBL, MFX_GT3 }, // HALO GT3
    { 0x5932, MFX_HW_KBL, MFX_GT4 }, // DT GT4
    { 0x593A, MFX_HW_KBL, MFX_GT4 }, // SERV GT4
    { 0x593B, MFX_HW_KBL, MFX_GT4 }, // HALO GT4
    { 0x593D, MFX_HW_KBL, MFX_GT4 }, // WRK GT4

    /* CFL, GT information can be wrong, need to verify */
    { 0x3E90, MFX_HW_CFL, MFX_GT1 },
    { 0x3E91, MFX_HW_CFL, MFX_GT2 },
    { 0x3E92, MFX_HW_CFL, MFX_GT2 },
    { 0x3E93, MFX_HW_CFL, MFX_GT1 },
    { 0x3E94, MFX_HW_CFL, MFX_GT2 },
    { 0x3E96, MFX_HW_CFL, MFX_GT2 },
    { 0x3E9B, MFX_HW_CFL, MFX_GT2 },
    { 0x3EA5, MFX_HW_CFL, MFX_GT3 },
    { 0x3EA6, MFX_HW_CFL, MFX_GT3 },
    { 0x3EA7, MFX_HW_CFL, MFX_GT3 },
    { 0x3EA8, MFX_HW_CFL, MFX_GT3 },

    /* WHL */
    { 0x3EA0, MFX_HW_CFL, MFX_GT2 },

    /* CNL */
    { 0x5A51, MFX_HW_CNL, MFX_GT2 },
    { 0x5A52, MFX_HW_CNL, MFX_GT2 },
    { 0x5A5A, MFX_HW_CNL, MFX_GT2 },
    { 0x5A42, MFX_HW_CNL, MFX_GT2 },
    { 0x5A4A, MFX_HW_CNL, MFX_GT2 },
    { 0x5A59, MFX_HW_CNL, MFX_GT2 },
    { 0x5A41, MFX_HW_CNL, MFX_GT2 },
    { 0x5A49, MFX_HW_CNL, MFX_GT2 },

    /* ICL LP */
    { 0x8A50, MFX_HW_ICL_LP, MFX_GT2 },
    { 0x8A51, MFX_HW_ICL_LP, MFX_GT2 },
    { 0x8A5C, MFX_HW_ICL_LP, MFX_GT2 },
    { 0x8A5D, MFX_HW_ICL_LP, MFX_GT2 },
    { 0x8A52, MFX_HW_ICL_LP, MFX_GT2 },
    { 0x8A5A, MFX_HW_ICL_LP, MFX_GT2 },
    { 0x8A5B, MFX_HW_ICL_LP, MFX_GT2 },
    { 0x8A71, MFX_HW_ICL_LP, MFX_GT1 },
    { 0x8A70, MFX_HW_ICL_LP, MFX_GT1 }

 };

/* END: IOCTLs definitions */

#define TMP_DEBUG

using namespace std;
using namespace UMC;

#ifdef _MSVC_LANG
#pragma warning(disable: 4311) // in HWVideoCORE::TraceFrames(): pointer truncation from 'void*' to 'int'
#endif

static
mfx_device_item getDeviceItem(VADisplay pVaDisplay)
{
    /* This is value by default */
    mfx_device_item retDeviceItem = { 0x0000, MFX_HW_UNKNOWN, MFX_GT_UNKNOWN };
    int fd = 0, i = 0, listSize = 0;
    int devID = 0;
    int ret = 0;
    drm_i915_getparam_t gp;
    VADisplayContextP pDisplayContext_test = NULL;
    VADriverContextP  pDriverContext_test = NULL;

    pDisplayContext_test = (VADisplayContextP) pVaDisplay;
    pDriverContext_test  = pDisplayContext_test->pDriverContext;
    fd = *(int*) pDriverContext_test->drm_state;

    /* Now as we know real authenticated fd of VAAPI library,
     * we can call ioctl() to kernel mode driver,
     * get device ID and find out platform type
     * */
    gp.param = I915_PARAM_CHIPSET_ID;
    gp.value = &devID;

    ret = ioctl(fd, DRM_IOCTL_I915_GETPARAM, &gp);
    if (!ret)
    {
        listSize = (sizeof(listLegalDevIDs) / sizeof(mfx_device_item));
        for (i = 0; i < listSize; ++i)
        {
            if (listLegalDevIDs[i].device_id == devID)
            {
                retDeviceItem = listLegalDevIDs[i];
                break;
            }
        }
    }

    return retDeviceItem;
} // eMFXHWType getDeviceItem (VADisplay pVaDisplay)


VAAPIVideoCORE::VAAPIVideoCORE(
    const mfxU32 adapterNum,
    const mfxU32 numThreadsAvailable,
    const mfxSession session) :
            CommonCORE(numThreadsAvailable, session)
          , m_Display(0)
          , m_VAConfigHandle((mfxHDL)VA_INVALID_ID)
          , m_VAContextHandle((mfxHDL)VA_INVALID_ID)
          , m_KeepVAState(false)
          , m_adapterNum(adapterNum)
          , m_bUseExtAllocForHWFrames(false)
          , m_HWType(MFX_HW_IVB)
          , m_GTConfig(MFX_GT_UNKNOWN)
#if !defined(ANDROID)
          , m_bCmCopy(false)
          , m_bCmCopyAllowed(true)
#else
          , m_bCmCopy(false)
          , m_bCmCopyAllowed(false)
#endif
          , m_bHEVCFEIEnabled(false)
{
} // VAAPIVideoCORE::VAAPIVideoCORE(...)


VAAPIVideoCORE::~VAAPIVideoCORE()
{
    if (m_bCmCopy)
    {
        m_pCmCopy.get()->Release();
        m_bCmCopy = false;
    }

    Close();

} // VAAPIVideoCORE::~VAAPIVideoCORE()


void VAAPIVideoCORE::Close()
{
    m_KeepVAState = false;
    m_pVA.reset();
} // void VAAPIVideoCORE::Close()


mfxStatus
VAAPIVideoCORE::GetHandle(
    mfxHandleType type,
    mfxHDL *handle)
{
    MFX_CHECK_NULL_PTR1(handle);
    UMC::AutomaticUMCMutex guard(m_guard);

        return CommonCORE::GetHandle(type, handle);

} // mfxStatus VAAPIVideoCORE::GetHandle(mfxHandleType type, mfxHDL *handle)

mfxStatus
VAAPIVideoCORE::SetHandle(
    mfxHandleType type,
    mfxHDL hdl)
{
    MFX_CHECK_NULL_PTR1(hdl);
    UMC::AutomaticUMCMutex guard(m_guard);
    try
    {
        switch ((mfxU32)type)
        {
        default:
            mfxStatus sts = CommonCORE::SetHandle(type, hdl);
            MFX_CHECK_STS(sts);
            m_Display = (VADisplay)m_hdl;

            /* As we know right VA handle (pointer),
            * we can get real authenticated fd of VAAPI library(display),
            * and can call ioctl() to kernel mode driver,
            * to get device ID and find out platform type
            */
            mfx_device_item devItem= getDeviceItem(m_Display);
            m_HWType = devItem.platform;
            m_GTConfig = devItem.config;
        }
        return MFX_ERR_NONE;
    }
    catch (...)
    {
        ReleaseHandle();
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }
}// mfxStatus VAAPIVideoCORE::SetHandle(mfxHandleType type, mfxHDL handle)


mfxStatus
VAAPIVideoCORE::TraceFrames(
    mfxFrameAllocRequest* request,
    mfxFrameAllocResponse* response,
    mfxStatus sts)
{
    (void)request;
    (void)response;

    return sts;
}

mfxStatus
VAAPIVideoCORE::AllocFrames(
    mfxFrameAllocRequest* request,
    mfxFrameAllocResponse* response,
    bool isNeedCopy)
{
    UMC::AutomaticUMCMutex guard(m_guard);
    try
    {
        MFX_CHECK_NULL_PTR2(request, response);
        mfxStatus sts = MFX_ERR_NONE;
        mfxFrameAllocRequest temp_request = *request;

        // external allocator doesn't know how to allocate opaque surfaces
        // we can treat opaque as internal
        if (temp_request.Type & MFX_MEMTYPE_OPAQUE_FRAME)
        {
            temp_request.Type -= MFX_MEMTYPE_OPAQUE_FRAME;
            temp_request.Type |= MFX_MEMTYPE_INTERNAL_FRAME;
        }

        if (!m_bCmCopy && m_bCmCopyAllowed && isNeedCopy && m_Display)
        {
            m_pCmCopy.reset(new CmCopyWrapper);
            if (!m_pCmCopy.get()->GetCmDevice(m_Display)){
                m_bCmCopy = false;
                m_bCmCopyAllowed = false;
                m_pCmCopy.get()->Release();
                m_pCmCopy.reset();
            }else{
                sts = m_pCmCopy.get()->Initialize(GetHWType());
                MFX_CHECK_STS(sts);
                m_bCmCopy = true;
            }
        }else if(m_bCmCopy){
            if(m_pCmCopy.get())
                m_pCmCopy.get()->ReleaseCmSurfaces();
            else
                m_bCmCopy = false;
        }


        // use common core for sw surface allocation
        if (request->Type & MFX_MEMTYPE_SYSTEM_MEMORY)
        {
            sts = CommonCORE::AllocFrames(request, response);
            return TraceFrames(request, response, sts);
        } else
        {
            // external allocator
            if (m_bSetExtFrameAlloc && request->Info.FourCC != MFX_FOURCC_P8)
            {
                // Default allocator should be used if D3D manager is not set and internal D3D buffers are required
                if (!m_Display && request->Type & MFX_MEMTYPE_INTERNAL_FRAME)
                {
                    m_bUseExtAllocForHWFrames = false;
                    sts = DefaultAllocFrames(request, response);
                    MFX_CHECK_STS(sts);

                    return TraceFrames(request, response, sts);
                }

                sts = (*m_FrameAllocator.frameAllocator.Alloc)(m_FrameAllocator.frameAllocator.pthis, &temp_request, response);

                // if external allocator cannot allocate d3d frames - use default memory allocator
                if (MFX_ERR_UNSUPPORTED == sts || MFX_ERR_MEMORY_ALLOC == sts)
                {
                    // Default Allocator is used for internal memory allocation only
                    if (request->Type & MFX_MEMTYPE_EXTERNAL_FRAME)
                        return sts;
                    m_bUseExtAllocForHWFrames = false;
                    sts = DefaultAllocFrames(request, response);
                    MFX_CHECK_STS(sts);

                    return TraceFrames(request, response, sts);
                }
                // let's create video accelerator
                else if (MFX_ERR_NONE == sts)
                {
                    // Checking for unsupported mode - external allocator exist but Device handle doesn't set
                    if (!m_Display)
                        return MFX_ERR_UNSUPPORTED;

                    if (response->NumFrameActual < request->NumFrameMin)
                    {
                        (*m_FrameAllocator.frameAllocator.Free)(m_FrameAllocator.frameAllocator.pthis, response);
                        return MFX_ERR_MEMORY_ALLOC;
                    }

                    m_bUseExtAllocForHWFrames = true;
                    sts = ProcessRenderTargets(request, response, &m_FrameAllocator);
                    MFX_CHECK_STS(sts);

                    return TraceFrames(request, response, sts);
                }
                // error situation
                else
                {
                    m_bUseExtAllocForHWFrames = false;
                    return sts;
                }
            }
            else
            {
                // Default Allocator is used for internal memory allocation and all coded buffers allocation
                if (request->Type & MFX_MEMTYPE_EXTERNAL_FRAME)
                    return MFX_ERR_MEMORY_ALLOC;

                m_bUseExtAllocForHWFrames = false;
                sts = DefaultAllocFrames(request, response);
                MFX_CHECK_STS(sts);

                return TraceFrames(request, response, sts);
            }
        }
    }
    catch(...)
    {
        return MFX_ERR_MEMORY_ALLOC;
    }

} // mfxStatus VAAPIVideoCORE::AllocFrames(...)


mfxStatus VAAPIVideoCORE::ReallocFrame(mfxFrameSurface1 *surf)
{
    MFX_CHECK_NULL_PTR1(surf);

    mfxMemId memid = surf->Data.MemId;

    if (!(surf->Data.MemType & MFX_MEMTYPE_INTERNAL_FRAME &&
        ((surf->Data.MemType & MFX_MEMTYPE_DXVA2_DECODER_TARGET)||
         (surf->Data.MemType & MFX_MEMTYPE_DXVA2_PROCESSOR_TARGET))))
        return MFX_ERR_MEMORY_ALLOC;

    mfxFrameAllocator *pFrameAlloc = GetAllocatorAndMid(memid);
   if (!pFrameAlloc)
       return MFX_ERR_MEMORY_ALLOC;

   mfxHDL srcHandle;
   if (MFX_ERR_NONE == GetFrameHDL(surf->Data.MemId, &srcHandle))
   {
       VASurfaceID *va_surf = (VASurfaceID*)srcHandle;
       return mfxDefaultAllocatorVAAPI::ReallocFrameHW(pFrameAlloc->pthis, surf, va_surf);
   }

    return MFX_ERR_MEMORY_ALLOC;
}

mfxStatus
VAAPIVideoCORE::DefaultAllocFrames(
    mfxFrameAllocRequest* request,
    mfxFrameAllocResponse* response)
{
    mfxStatus sts = MFX_ERR_NONE;

    if ((request->Type & MFX_MEMTYPE_DXVA2_DECODER_TARGET)||
        (request->Type & MFX_MEMTYPE_DXVA2_PROCESSOR_TARGET)) // SW - TBD !!!!!!!!!!!!!!
    {
        if (!m_Display)
            return MFX_ERR_NOT_INITIALIZED;

        mfxBaseWideFrameAllocator* pAlloc = GetAllocatorByReq(request->Type);
        // VPP, ENC, PAK can request frames for several times
        if (pAlloc && (request->Type & MFX_MEMTYPE_FROM_DECODE))
            return MFX_ERR_MEMORY_ALLOC;

        if (!pAlloc)
        {
            m_pcHWAlloc.reset(new mfxDefaultAllocatorVAAPI::mfxWideHWFrameAllocator(request->Type, m_Display));
            pAlloc = m_pcHWAlloc.get();
        }
        // else ???

        pAlloc->frameAllocator.pthis = pAlloc;
        sts = (*pAlloc->frameAllocator.Alloc)(pAlloc->frameAllocator.pthis,request, response);
        MFX_CHECK_STS(sts);
        sts = ProcessRenderTargets(request, response, pAlloc);
        MFX_CHECK_STS(sts);

    }
    else
    {
        return CommonCORE::DefaultAllocFrames(request, response);
    }
    ++m_NumAllocators;

    return sts;

} // mfxStatus VAAPIVideoCORE::DefaultAllocFrames(...)


mfxStatus
VAAPIVideoCORE::CreateVA(
    mfxVideoParam* param,
    mfxFrameAllocRequest* request,
    mfxFrameAllocResponse* response,
    UMC::FrameAllocator *allocator)
{
    mfxStatus sts = MFX_ERR_NONE;

    if (!(request->Type & MFX_MEMTYPE_FROM_DECODE) ||
        !(request->Type & MFX_MEMTYPE_DXVA2_DECODER_TARGET))
        return MFX_ERR_NONE;

    int profile = UMC::VA_VLD;

    // video accelerator is needed for decoders only
    switch (param->mfx.CodecId)
    {
    case MFX_CODEC_VC1:
        profile |= VA_VC1;
        break;
    case MFX_CODEC_MPEG2:
        profile |= VA_MPEG2;
        break;
    case MFX_CODEC_AVC:
        profile |= VA_H264;
        break;
    case MFX_CODEC_HEVC:
        profile |= VA_H265;
        if (MFX_PROFILE_HEVC_REXT == param->mfx.CodecProfile)
        {
            profile |= VA_PROFILE_REXT;
        }
        if (param->mfx.FrameInfo.FourCC == MFX_FOURCC_P010
#if (MFX_VERSION >= 1027)
            || param->mfx.FrameInfo.FourCC == MFX_FOURCC_Y210
            || param->mfx.FrameInfo.FourCC == MFX_FOURCC_Y410
#endif
        )
        {
            profile |= VA_PROFILE_10;
        }

        if (MFX_CHROMAFORMAT_YUV422 == param->mfx.FrameInfo.ChromaFormat)
            profile |= (VA_PROFILE_422 | VA_PROFILE_REXT);
        else if (MFX_CHROMAFORMAT_YUV444 == param->mfx.FrameInfo.ChromaFormat)
            profile |= (VA_PROFILE_444 |VA_PROFILE_REXT);

        break;
    case MFX_CODEC_VP8:
        profile |= VA_VP8;
        break;
    case MFX_CODEC_VP9:
        profile |= VA_VP9;
        switch (param->mfx.FrameInfo.FourCC)
        {
        case MFX_FOURCC_P010:
            profile |= VA_PROFILE_10;
            break;
        case MFX_FOURCC_AYUV:
            profile |= VA_PROFILE_444;
            break;
#if (MFX_VERSION >= 1027)
        case MFX_FOURCC_Y410:
            profile |= VA_PROFILE_10 | VA_PROFILE_444;
            break;
#endif
        }
        break;
    case MFX_CODEC_JPEG:
        profile |= VA_JPEG;
        break;
    default:
        return MFX_ERR_UNSUPPORTED;
    }

    bool init_render_targets =
#if defined(ANDROID)
        true
#else
        param->mfx.CodecId != MFX_CODEC_MPEG2 &&
        param->mfx.CodecId != MFX_CODEC_AVC   &&
        param->mfx.CodecId != MFX_CODEC_HEVC
#endif
        ;

    VASurfaceID* RenderTargets = NULL;
    std::vector<VASurfaceID> rt_pool;
    if (init_render_targets)
    {
        rt_pool.resize(response->NumFrameActual);
        RenderTargets = &rt_pool[0];

        for (mfxU32 i = 0; i < response->NumFrameActual; i++)
        {
            mfxMemId InternalMid = response->mids[i];
            mfxFrameAllocator* pAlloc = GetAllocatorAndMid(InternalMid);
            VASurfaceID *pSurface = NULL;
            if (pAlloc)
                pAlloc->GetHDL(pAlloc->pthis, InternalMid, (mfxHDL*)&pSurface);
            else
                return MFX_ERR_UNDEFINED_BEHAVIOR;

            rt_pool[i] = *pSurface;
        }
    }

    if(GetExtBuffer(param->ExtParam, param->NumExtParam, MFX_EXTBUFF_DEC_ADAPTIVE_PLAYBACK))
        m_KeepVAState = true;
    else
        m_KeepVAState = false;

    sts = CreateVideoAccelerator(param, profile, response->NumFrameActual, RenderTargets, allocator);

    return sts;

} // mfxStatus VAAPIVideoCORE::CreateVA(...)

mfxStatus VAAPIVideoCORE::CreateVideoProcessing(mfxVideoParam * param)
{
    (void)param;

    mfxStatus sts = MFX_ERR_NONE;
#if defined (MFX_ENABLE_VPP)
    if (!m_vpp_hw_resmng.GetDevice()){
        sts = m_vpp_hw_resmng.CreateDevice(this);
    }
#else
    sts = MFX_ERR_UNSUPPORTED;
#endif
    return sts;
}

mfxStatus
VAAPIVideoCORE::ProcessRenderTargets(
    mfxFrameAllocRequest* request,
    mfxFrameAllocResponse* response,
    mfxBaseWideFrameAllocator* pAlloc)
{
#if defined(ANDROID)
    if (response->NumFrameActual > 128)
        return MFX_ERR_UNSUPPORTED;
#endif

    RegisterMids(response, request->Type, !m_bUseExtAllocForHWFrames, pAlloc);
    m_pcHWAlloc.pop();

    return MFX_ERR_NONE;

} // mfxStatus VAAPIVideoCORE::ProcessRenderTargets(

mfxStatus
VAAPIVideoCORE::GetVAService(
    VADisplay*  pVADisplay)
{
    // check if created already
    if (m_Display)
    {
        if (pVADisplay)
        {
            *pVADisplay = m_Display;
        }
        return MFX_ERR_NONE;
    }

    return MFX_ERR_NOT_INITIALIZED;

} // mfxStatus VAAPIVideoCORE::GetVAService(...)

mfxStatus
VAAPIVideoCORE::SetCmCopyStatus(bool enable)
{
    m_bCmCopyAllowed = enable;
    if (!enable)
    {
        if (m_pCmCopy.get())
        {
            m_pCmCopy.get()->Release();
        }
        m_bCmCopy = false;
    }
    return MFX_ERR_NONE;
} // mfxStatus VAAPIVideoCORE::SetCmCopyStatus(...)

mfxStatus
VAAPIVideoCORE::CreateVideoAccelerator(
    mfxVideoParam* param,
    int profile,
    int NumOfRenderTarget,
    VASurfaceID* RenderTargets,
    UMC::FrameAllocator *allocator)
{
    mfxStatus sts = MFX_ERR_NONE;

    Status st;
    UMC::LinuxVideoAcceleratorParams params;
    mfxFrameInfo *pInfo = &(param->mfx.FrameInfo);

    if (!m_Display)
        return MFX_ERR_NOT_INITIALIZED;

    UMC::VideoStreamInfo VideoInfo;
    VideoInfo.clip_info.width = pInfo->Width;
    VideoInfo.clip_info.height = pInfo->Height;

    // Init Accelerator
    params.m_Display = m_Display;
    params.m_pConfigId = (VAConfigID*)&m_VAConfigHandle;
    params.m_pContext = (VAContextID*)&m_VAContextHandle;
    params.m_pKeepVAState = &m_KeepVAState;
    params.m_pVideoStreamInfo = &VideoInfo;
    params.m_iNumberSurfaces = NumOfRenderTarget;
    params.m_allocator = allocator;
    params.m_surf = (void **)RenderTargets;

    params.m_protectedVA = param->Protected;

    /* There are following conditions for post processing via HW fixed function engine:
     * (1): AVC
     * (2): Progressive only
     * (3): Supported on SKL (Core) and APL (Atom) platforms and above
     * (4): Only video memory supported (so, OPAQ memory does not supported!)
     * */
    if ( (GetExtBuffer(param->ExtParam, param->NumExtParam, MFX_EXTBUFF_DEC_VIDEO_PROCESSING)) &&
         (MFX_PICSTRUCT_PROGRESSIVE == param->mfx.FrameInfo.PicStruct) &&
         (MFX_HW_SCL <= GetHWType()) &&
         (param->IOPattern & MFX_IOPATTERN_OUT_VIDEO_MEMORY))
    {
        params.m_needVideoProcessingVA = true;
    }

    //check 'StreamOut' feature is requested
    {
        mfxExtBuffer* ext = GetExtBuffer(param->ExtParam, param->NumExtParam, MFX_EXTBUFF_FEI_PARAM);
        if (ext)
            params.m_CreateFlags |= reinterpret_cast<mfxExtFeiParam*>(ext)->Func == MFX_FEI_FUNCTION_DEC ? VA_DECODE_STREAM_OUT_ENABLE : 0;
    }

    m_pVA.reset((params.m_CreateFlags & VA_DECODE_STREAM_OUT_ENABLE) ? new FEIVideoAccelerator() : new LinuxVideoAccelerator());
    m_pVA.get()->m_Platform = UMC::VA_LINUX;
    m_pVA.get()->m_Profile = (VideoAccelerationProfile)profile;
    m_pVA.get()->m_HWPlatform = m_HWType;

    st = m_pVA.get()->Init(&params);

    if(UMC_OK != st)
    {
        return MFX_ERR_UNSUPPORTED;
    }

    return sts;

} // mfxStatus VAAPIVideoCORE::CreateVideoAccelerator(...)


mfxStatus
VAAPIVideoCORE::DoFastCopyWrapper(
    mfxFrameSurface1* pDst,
    mfxU16 dstMemType,
    mfxFrameSurface1* pSrc,
    mfxU16 srcMemType)
{
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "VAAPIVideoCORE::DoFastCopyWrapper");
    mfxStatus sts;

    mfxHDL srcHandle, dstHandle;
    mfxMemId srcMemId, dstMemId;

    mfxFrameSurface1 srcTempSurface, dstTempSurface;

    memset(&srcTempSurface, 0, sizeof(mfxFrameSurface1));
    memset(&dstTempSurface, 0, sizeof(mfxFrameSurface1));

    // save original mem ids
    srcMemId = pSrc->Data.MemId;
    dstMemId = pDst->Data.MemId;

    mfxU8* srcPtr = GetFramePointer(pSrc->Info.FourCC, pSrc->Data);
    mfxU8* dstPtr = GetFramePointer(pDst->Info.FourCC, pDst->Data);

    srcTempSurface.Info = pSrc->Info;
    dstTempSurface.Info = pDst->Info;

    bool isSrcLocked = false;
    bool isDstLocked = false;

    if (srcMemType & MFX_MEMTYPE_EXTERNAL_FRAME)
    {
        if (srcMemType & MFX_MEMTYPE_SYSTEM_MEMORY)
        {
            if (nullptr == srcPtr)
            {
                sts = LockExternalFrame(srcMemId, &srcTempSurface.Data);
                MFX_CHECK_STS(sts);

                isSrcLocked = true;
            }
            else
            {
                srcTempSurface.Data = pSrc->Data;
                srcTempSurface.Data.MemId = 0;
            }
        }
        else if (srcMemType & MFX_MEMTYPE_DXVA2_DECODER_TARGET)
        {
            sts = GetExternalFrameHDL(srcMemId, &srcHandle);
            MFX_CHECK_STS(sts);

            srcTempSurface.Data.MemId = srcHandle;
        }
    }
    else if (srcMemType & MFX_MEMTYPE_INTERNAL_FRAME)
    {
        if (srcMemType & MFX_MEMTYPE_SYSTEM_MEMORY)
        {
            if (nullptr == srcPtr)
            {
                sts = LockFrame(srcMemId, &srcTempSurface.Data);
                MFX_CHECK_STS(sts);

                isSrcLocked = true;
            }
            else
            {
                srcTempSurface.Data = pSrc->Data;
                srcTempSurface.Data.MemId = 0;
            }
        }
        else if (srcMemType & MFX_MEMTYPE_DXVA2_DECODER_TARGET)
        {
            sts = GetFrameHDL(srcMemId, &srcHandle);
            MFX_CHECK_STS(sts);

            srcTempSurface.Data.MemId = srcHandle;
        }
    }

    if (dstMemType & MFX_MEMTYPE_EXTERNAL_FRAME)
    {
        if (dstMemType & MFX_MEMTYPE_SYSTEM_MEMORY)
        {
            if (nullptr == dstPtr)
            {
                sts = LockExternalFrame(dstMemId, &dstTempSurface.Data);
                MFX_CHECK_STS(sts);

                isDstLocked = true;
            }
            else
            {
                dstTempSurface.Data = pDst->Data;
                dstTempSurface.Data.MemId = 0;
            }
        }
        else if (dstMemType & MFX_MEMTYPE_DXVA2_DECODER_TARGET)
        {
            sts = GetExternalFrameHDL(dstMemId, &dstHandle);
            MFX_CHECK_STS(sts);

            dstTempSurface.Data.MemId = dstHandle;
        }
    }
    else if (dstMemType & MFX_MEMTYPE_INTERNAL_FRAME)
    {
        if (dstMemType & MFX_MEMTYPE_SYSTEM_MEMORY)
        {
            if (nullptr == dstPtr)
            {
                sts = LockFrame(dstMemId, &dstTempSurface.Data);
                MFX_CHECK_STS(sts);

                isDstLocked = true;
            }
            else
            {
                dstTempSurface.Data = pDst->Data;
                dstTempSurface.Data.MemId = 0;
            }
        }
        else if (dstMemType & MFX_MEMTYPE_DXVA2_DECODER_TARGET)
        {
            sts = GetFrameHDL(dstMemId, &dstHandle);
            MFX_CHECK_STS(sts);

            dstTempSurface.Data.MemId = dstHandle;
        }
    }

    sts = DoFastCopyExtended(&dstTempSurface, &srcTempSurface);

    if (MFX_ERR_DEVICE_FAILED == sts && 0 != dstTempSurface.Data.Corrupted)
    {
        // complete task even if frame corrupted
        pDst->Data.Corrupted = dstTempSurface.Data.Corrupted;
        sts = MFX_ERR_NONE;
    }

    MFX_CHECK_STS(sts);

    if (true == isSrcLocked)
    {
        if (srcMemType & MFX_MEMTYPE_EXTERNAL_FRAME)
        {
            sts = UnlockExternalFrame(srcMemId, &srcTempSurface.Data);
            MFX_CHECK_STS(sts);
        }
        else if (srcMemType & MFX_MEMTYPE_INTERNAL_FRAME)
        {
            sts = UnlockFrame(srcMemId, &srcTempSurface.Data);
            MFX_CHECK_STS(sts);
        }
    }

    if (true == isDstLocked)
    {
        if (dstMemType & MFX_MEMTYPE_EXTERNAL_FRAME)
        {
            sts = UnlockExternalFrame(dstMemId, &dstTempSurface.Data);
            MFX_CHECK_STS(sts);
        }
        else if (dstMemType & MFX_MEMTYPE_INTERNAL_FRAME)
        {
            sts = UnlockFrame(dstMemId, &dstTempSurface.Data);
            MFX_CHECK_STS(sts);
        }
    }

    return MFX_ERR_NONE;

} // mfxStatus VAAPIVideoCORE::DoFastCopyWrapper(...)


mfxStatus
VAAPIVideoCORE::DoFastCopyExtended(
    mfxFrameSurface1* pDst,
    mfxFrameSurface1* pSrc)
{
    mfxStatus sts;
    mfxU8* srcPtr;
    mfxU8* dstPtr;

    sts = GetFramePointerChecked(pSrc->Info, pSrc->Data, &srcPtr);
    MFX_CHECK(MFX_SUCCEEDED(sts), MFX_ERR_UNDEFINED_BEHAVIOR);
    sts = GetFramePointerChecked(pDst->Info, pDst->Data, &dstPtr);
    MFX_CHECK(MFX_SUCCEEDED(sts), MFX_ERR_UNDEFINED_BEHAVIOR);

    // check that only memId or pointer are passed
    // otherwise don't know which type of memory copying is requested
    if (
        (nullptr != dstPtr && nullptr != pDst->Data.MemId) ||
        (nullptr != srcPtr && nullptr != pSrc->Data.MemId)
        )
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    mfxSize roi = {MFX_MIN(pSrc->Info.Width, pDst->Info.Width), MFX_MIN(pSrc->Info.Height, pDst->Info.Height)};

    // check that region of interest is valid
    if (0 == roi.width || 0 == roi.height)
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    CmCopyWrapper *pCmCopy = m_pCmCopy.get();

    bool canUseCMCopy = m_bCmCopy ? CmCopyWrapper::CanUseCmCopy(pDst, pSrc) : false;

    if (NULL != pSrc->Data.MemId && NULL != pDst->Data.MemId)
    {
        if (canUseCMCopy)
        {
            sts = pCmCopy->CopyVideoToVideo(pDst, pSrc);
            MFX_CHECK_STS(sts);
        }
        else
        {
            MFX_CHECK(m_Display, MFX_ERR_NOT_INITIALIZED);

            VASurfaceID *va_surf_src = (VASurfaceID*)(pSrc->Data.MemId);
            VASurfaceID *va_surf_dst = (VASurfaceID*)(pDst->Data.MemId);
            MFX_CHECK(va_surf_src != va_surf_dst, MFX_ERR_UNDEFINED_BEHAVIOR);

            VAImage va_img_src = {};
            VAStatus va_sts;

            va_sts = vaDeriveImage(m_Display, *va_surf_src, &va_img_src);
            MFX_CHECK(VA_STATUS_SUCCESS == va_sts, MFX_ERR_DEVICE_FAILED);

            {
                MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaPutImage");
                va_sts = vaPutImage(m_Display, *va_surf_dst, va_img_src.image_id,
                                    0, 0, roi.width, roi.height,
                                    0, 0, roi.width, roi.height);
            }
            MFX_CHECK(VA_STATUS_SUCCESS == va_sts, MFX_ERR_DEVICE_FAILED);

            va_sts = vaDestroyImage(m_Display, va_img_src.image_id);
            MFX_CHECK(VA_STATUS_SUCCESS == va_sts, MFX_ERR_DEVICE_FAILED);
        }
    }
    else if (nullptr != pSrc->Data.MemId && nullptr != dstPtr)
    {
        MFX_CHECK(m_Display,MFX_ERR_NOT_INITIALIZED);

        // copy data
        {
            if (canUseCMCopy)
            {
                sts = pCmCopy->CopyVideoToSys(pDst, pSrc);
                MFX_CHECK_STS(sts);
            }
            else
            {
                VASurfaceID *va_surface = (VASurfaceID*)(pSrc->Data.MemId);
                VAImage va_image;
                VAStatus va_sts;
                void *pBits = NULL;

                va_sts = vaDeriveImage(m_Display, *va_surface, &va_image);
                MFX_CHECK(VA_STATUS_SUCCESS == va_sts, MFX_ERR_DEVICE_FAILED);

                {
                    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaMapBuffer");
                    va_sts = vaMapBuffer(m_Display, va_image.buf, (void **) &pBits);
                }
                MFX_CHECK(VA_STATUS_SUCCESS == va_sts, MFX_ERR_DEVICE_FAILED);

                {
                    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "FastCopy_vid2sys");
                    mfxStatus sts = mfxDefaultAllocatorVAAPI::SetFrameData(va_image, pDst->Info.FourCC, (mfxU8*)pBits, &pSrc->Data);
                    MFX_CHECK_STS(sts);

                    mfxMemId saveMemId = pSrc->Data.MemId;
                    pSrc->Data.MemId = 0;

                    sts = CoreDoSWFastCopy(pDst, pSrc, COPY_VIDEO_TO_SYS); // sw copy
                    MFX_CHECK_STS(sts);

                    pSrc->Data.MemId = saveMemId;
                    MFX_CHECK_STS(sts);

                }

                {
                    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaUnmapBuffer");
                    va_sts = vaUnmapBuffer(m_Display, va_image.buf);
                }
                MFX_CHECK(VA_STATUS_SUCCESS == va_sts, MFX_ERR_DEVICE_FAILED);

                va_sts = vaDestroyImage(m_Display, va_image.image_id);
                MFX_CHECK(VA_STATUS_SUCCESS == va_sts, MFX_ERR_DEVICE_FAILED);

            }
        }

    }
    else if (nullptr != srcPtr && nullptr != dstPtr)
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "FastCopy_sys2sys");
        // system memories were passed
        // use common way to copy frames
        sts = CoreDoSWFastCopy(pDst, pSrc, COPY_SYS_TO_SYS); // sw copy
        MFX_CHECK_STS(sts);
    }
    else if (nullptr != srcPtr && nullptr != pDst->Data.MemId)
    {
        if (canUseCMCopy)
        {
            sts = pCmCopy->CopySysToVideo(pDst, pSrc);
            MFX_CHECK_STS(sts);
        }
        else
        {
            VAStatus va_sts = VA_STATUS_SUCCESS;
            VASurfaceID *va_surface = (VASurfaceID*)(size_t)pDst->Data.MemId;
            VAImage va_image;
            void *pBits = NULL;

            MFX_CHECK(m_Display, MFX_ERR_NOT_INITIALIZED);

            va_sts = vaDeriveImage(m_Display, *va_surface, &va_image);
            MFX_CHECK(VA_STATUS_SUCCESS == va_sts, MFX_ERR_DEVICE_FAILED);

            {
                MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaMapBuffer");
                va_sts = vaMapBuffer(m_Display, va_image.buf, (void **) &pBits);
            }
            MFX_CHECK(VA_STATUS_SUCCESS == va_sts, MFX_ERR_DEVICE_FAILED);

            {
                MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "FastCopy_sys2vid");

                mfxStatus sts = mfxDefaultAllocatorVAAPI::SetFrameData(va_image, pDst->Info.FourCC, (mfxU8*)pBits, &pDst->Data);
                MFX_CHECK_STS(sts);

                mfxMemId saveMemId = pDst->Data.MemId;
                pDst->Data.MemId = 0;

                sts = CoreDoSWFastCopy(pDst, pSrc, COPY_SYS_TO_VIDEO); // sw copy
                MFX_CHECK_STS(sts);

                pDst->Data.MemId = saveMemId;
                MFX_CHECK_STS(sts);

            }

            {
                MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_EXTCALL, "vaUnmapBuffer");
                va_sts = vaUnmapBuffer(m_Display, va_image.buf);
            }
            MFX_CHECK(VA_STATUS_SUCCESS == va_sts, MFX_ERR_DEVICE_FAILED);

            // vaDestroyImage
            va_sts = vaDestroyImage(m_Display, va_image.image_id);
            MFX_CHECK(VA_STATUS_SUCCESS == va_sts, MFX_ERR_DEVICE_FAILED);
        }
    }
    else
    {
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    }

    return MFX_ERR_NONE;

} // mfxStatus VAAPIVideoCORE::DoFastCopyExtended(mfxFrameSurface1 *pDst, mfxFrameSurface1 *pSrc)


void VAAPIVideoCORE::ReleaseHandle()
{

} // void VAAPIVideoCORE::ReleaseHandle()

//function checks profile and entrypoint and video resolution support
//On linux specific function!
mfxStatus VAAPIVideoCORE::IsGuidSupported(const GUID guid,
                                         mfxVideoParam *par, bool /* isEncoder */)
{
    MFX_CHECK(par, MFX_WRN_PARTIAL_ACCELERATION);
    MFX_CHECK(!IsMVCProfile(par->mfx.CodecProfile), MFX_WRN_PARTIAL_ACCELERATION);

    MFX_CHECK(m_Display, MFX_ERR_DEVICE_FAILED);

    VaGuidMapper mapper(guid);
    VAProfile req_profile = mapper.profile;
    VAEntrypoint req_entrypoint = mapper.entrypoint;
    mfxI32 va_max_num_entrypoints = vaMaxNumEntrypoints(m_Display);
    mfxI32 va_max_num_profiles = vaMaxNumProfiles(m_Display);
    MFX_CHECK_COND(va_max_num_entrypoints && va_max_num_profiles);

    //driver always support VAProfileNone
    if (req_profile != VAProfileNone)
    {
        vector <VAProfile> va_profiles (va_max_num_profiles, VAProfileNone);

        //ask driver about profile support
        VAStatus va_sts = vaQueryConfigProfiles(m_Display,
                            va_profiles.data(), &va_max_num_profiles);
        MFX_CHECK(va_sts == VA_STATUS_SUCCESS, MFX_ERR_UNSUPPORTED);

        //check profile support
        auto it_profile = find(va_profiles.begin(), va_profiles.end(), req_profile);
        MFX_CHECK(it_profile != va_profiles.end(), MFX_ERR_UNSUPPORTED);
    }

    vector <VAEntrypoint> va_entrypoints (va_max_num_entrypoints, static_cast<VAEntrypoint> (0));

    //ask driver about entrypoint support
    VAStatus va_sts = vaQueryConfigEntrypoints(m_Display, req_profile,
                    va_entrypoints.data(), &va_max_num_entrypoints);
    MFX_CHECK(va_sts == VA_STATUS_SUCCESS, MFX_ERR_UNSUPPORTED);

    //check entrypoint support
    auto it_entrypoint = find(va_entrypoints.begin(), va_entrypoints.end(), req_entrypoint);
    MFX_CHECK(it_entrypoint != va_entrypoints.end(), MFX_ERR_UNSUPPORTED);

    VAConfigAttrib attr[] = {{VAConfigAttribMaxPictureWidth,  0},
                             {VAConfigAttribMaxPictureHeight, 0}};

    //ask driver about support
    va_sts = vaGetConfigAttributes(m_Display, mapper.profile,
                                   mapper.entrypoint,
                                   attr, sizeof(attr)/sizeof(*attr));

    MFX_CHECK(va_sts == VA_STATUS_SUCCESS, MFX_ERR_UNSUPPORTED);

    //check resolution video
    MFX_CHECK(attr[0].value != VA_ATTRIB_NOT_SUPPORTED, MFX_ERR_UNSUPPORTED);
    MFX_CHECK(attr[1].value != VA_ATTRIB_NOT_SUPPORTED, MFX_ERR_UNSUPPORTED);
    MFX_CHECK_COND(attr[0].value && attr[1].value);
    MFX_CHECK(attr[0].value >= par->mfx.FrameInfo.Width, MFX_ERR_UNSUPPORTED);
    MFX_CHECK(attr[1].value >= par->mfx.FrameInfo.Height, MFX_ERR_UNSUPPORTED);

    return MFX_ERR_NONE;
}

void* VAAPIVideoCORE::QueryCoreInterface(const MFX_GUID &guid)
{
    if(MFXIVideoCORE_GUID == guid)
    {
        return (void*) this;
    }
    else if( MFXICOREVAAPI_GUID == guid )
    {
        return (void*) m_pAdapter.get();
    }
    else if (MFXICORE_GT_CONFIG_GUID == guid)
    {
        return (void*)&m_GTConfig;
    }
    else if (MFXIHWCAPS_GUID == guid)
    {
        return (void*) &m_encode_caps;
    }
#ifdef MFX_ENABLE_MFE
    else if (MFXMFEDDIENCODER_SEARCH_GUID == guid)
    {
        if (!m_mfe.get())
        {
            m_mfe = (MFEVAAPIEncoder*)m_session->m_pOperatorCore->QueryGUID<ComPtrCore<MFEVAAPIEncoder> >(&VideoCORE::QueryCoreInterface, MFXMFEDDIENCODER_GUID);
            if (m_mfe.get())
                m_mfe.get()->AddRef();
        }
        return (void*)&m_mfe;
    }
    else if (MFXMFEDDIENCODER_GUID == guid)
    {
        return (void*)&m_mfe;
    }
#endif
    else if (MFXICORECM_GUID == guid)
    {
        CmDevice* pCmDevice = NULL;
        if (!m_bCmCopy)
        {
            m_pCmCopy.reset(new CmCopyWrapper);
            pCmDevice = m_pCmCopy.get()->GetCmDevice(m_Display);
            if (!pCmDevice)
                return NULL;
            if (MFX_ERR_NONE != m_pCmCopy.get()->Initialize(GetHWType()))
                return NULL;
            m_bCmCopy = true;
        }
        else
        {
            pCmDevice =  m_pCmCopy.get()->GetCmDevice(m_Display);
        }
        return (void*)pCmDevice;
    }
    else if (MFXICORECMCOPYWRAPPER_GUID == guid)
    {
        if (!m_pCmCopy.get())
        {
            m_pCmCopy.reset(new CmCopyWrapper);
            if (!m_pCmCopy.get()->GetCmDevice(m_Display))
            {
                m_bCmCopy = false;
                m_bCmCopyAllowed = false;
                m_pCmCopy.get()->Release();
                m_pCmCopy.reset();
                return NULL;
            }
            else
            {
                if (MFX_ERR_NONE != m_pCmCopy.get()->Initialize(GetHWType()))
                    return NULL;
                else
                    m_bCmCopy = true;
            }
        }
        return (void*)m_pCmCopy.get();
    }
    else if (MFXICMEnabledCore_GUID == guid)
    {
        if (!m_pCmAdapter.get())
        {
            m_pCmAdapter.reset(new CMEnabledCoreAdapter(this));
        }
        return (void*)m_pCmAdapter.get();
    }
    else if (MFXIHWMBPROCRATE_GUID == guid)
    {
        return (void*) &m_encode_mbprocrate;
    }
    else if (MFXIEXTERNALLOC_GUID == guid && m_bSetExtFrameAlloc)
    {
        return &m_FrameAllocator.frameAllocator;
    }
    else if (MFXICORE_API_1_19_GUID == guid)
    {
        return &m_API_1_19;
    }
    else if (MFXIFEIEnabled_GUID == guid)
    {
        return &m_bHEVCFEIEnabled;
    }
    else
    {
        return NULL;
    }

} // void* VAAPIVideoCORE::QueryCoreInterface(const MFX_GUID &guid)

bool IsHwMvcEncSupported()
{
    return false;
}


#endif
/* EOF */
