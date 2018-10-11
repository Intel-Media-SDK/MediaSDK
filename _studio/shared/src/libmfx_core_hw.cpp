#include "libmfx_core_hw.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <iomanip>
#include <fstream>
#include <string>


constexpr mfxU32 PCI_DISPLAY_CONTROLLER_CLASS = 0x03;
constexpr mfxU32 CLASS_ID_OFFSET = 16;
constexpr mfxU32 INTEL_VENDOR_ID = 0x8086;

std::unique_ptr<HardwareInfoStore> HardwareInfoStore::m_instance;
std::mutex HardwareInfoStore::m_mtx;

#if defined(LINUX32) || defined(LINUX64)
const std::string PCI_DIR = "/sys/bus/pci/devices";
int mfx_dir_filter(const struct dirent* dir_ent)
{
    if (!dir_ent) return 0;
    if (!strcmp(dir_ent->d_name, ".")) return 0;
    if (!strcmp(dir_ent->d_name, "..")) return 0;
    return 1;
}

mfxU32 ReadFirstHexNumInFile(std::string file_name)
{
    mfxU32 retval = 0;
    std::ifstream file(file_name, std::ifstream::in);
    if (file.is_open())
    {
        file >> std::hex >> retval;
        file.close();
    }
    return retval;
}
#endif // #if defined(LINUX32) || defined(LINUX64)


void HardwareInfoStore::LoadPciDisplayDevicesList()
{

#if defined(LINUX32) || defined(LINUX64)
    typedef int (*fsort)(const struct dirent**, const struct dirent**);
    struct mfx_disp_adapters* adapters = NULL;
    struct dirent** dir_entries = NULL;
    mfxU32 entries_num = scandir(PCI_DIR.c_str(), &dir_entries, mfx_dir_filter, (fsort)alphasort);

    for (mfxU32 i = 0; i < entries_num; ++i)
    {
        mfxU32 class_id = 0, vendor_id = 0, device_id = 0;

        if (!dir_entries[i]) continue;
        std::string pci_dev_name(dir_entries[i]->d_name);
        std::string full_pci_dev_path = PCI_DIR + '/' + pci_dev_name;

        class_id = ReadFirstHexNumInFile(full_pci_dev_path + "/class");

        if (PCI_DISPLAY_CONTROLLER_CLASS == (class_id >> CLASS_ID_OFFSET))
        {
            vendor_id = ReadFirstHexNumInFile(full_pci_dev_path + "/vendor");
            device_id = ReadFirstHexNumInFile(full_pci_dev_path + "/device");
            if (vendor_id && device_id)
            {
                m_pci_disp_dev_list.emplace_back(PciDisplayDevice{vendor_id, device_id});
            }
        }
    }

    // Cleanup
    for (mfxU32 i = 0; i < entries_num; ++i)
    {
        free(dir_entries[i]);
    }

    if (entries_num)
    {
        free(dir_entries);
    }
#endif // #if defined(LINUX32) || defined(LINUX64)

    return;
}

// Returns true if the (adapter_num)-th display adapter is present in the system
// and supports the (interface).
bool CheckForIntelHWDisplayDevicePresent(mfxU32 adapter_num, mfxIMPL interface)
{
#if defined(LINUX32) || defined(LINUX64)
    if (!(interface & MFX_IMPL_VIA_VAAPI))
    {
        return false; // Only VAAPI is currently supported
    }

    const std::vector<PciDisplayDevice>& pci_disp_dev_list = HardwareInfoStore::GetInstance().GetPciDisplayDeviceList();

    if (adapter_num >= pci_disp_dev_list.size())
    {
        return false;
    }
    PciDisplayDevice str;

    return (pci_disp_dev_list[adapter_num].vendor_id == INTEL_VENDOR_ID);

#else
    return true; // Not implemented for platforms other than Linux
#endif // #if defined(LINUX32) || defined(LINUX64)
}

bool IsHwMvcEncSupported()
{
    return false;
}

