// Copyright (c) 2017 Intel Corporation
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

#ifndef __LIBMFX_CORE__HW_H__
#define __LIBMFX_CORE__HW_H__

#include "mfx_common.h"
#include "umc_va_base.h"
#include <memory>
#include <mutex>
#include <vector>

struct PciDisplayDevice
{
    mfxU32 vendor_id;
    mfxU32 device_id;
};

class HardwareInfoStore
{
public:
    static HardwareInfoStore& GetInstance()
    {
        std::lock_guard<std::mutex> lock(m_mtx);
        if (!m_instance)
        {
            m_instance.reset(new HardwareInfoStore());
        }
        return (*m_instance);
    }
    const std::vector<PciDisplayDevice>& GetPciDisplayDeviceList()
    {
        {
            std::lock_guard<std::mutex> lock(m_mtx);
            if (m_pci_disp_dev_list.empty())
            {
                LoadPciDisplayDevicesList();
            }
        }
        return m_pci_disp_dev_list;
    };

private:
    HardwareInfoStore() = default;
    void LoadPciDisplayDevicesList();
    std::vector<PciDisplayDevice> m_pci_disp_dev_list;
    static std::unique_ptr<HardwareInfoStore> m_instance;
    static std::mutex m_mtx;
};

bool CheckForIntelHWDisplayDevicePresent(mfxU32 adapter_num, mfxIMPL interface);
bool IsHwMvcEncSupported();

#endif


