/*
**  _                                              _      ___    ___  
** | |                                            | |    |__ \  / _ \
** | |_Created _       _ __   _ __    ___    __ _ | |__     ) || (_) |
** | '_ \ | | | |     | '_ \ | '_ \  / _ \  / _` || '_ \   / /  \__, |
** | |_) || |_| |     | | | || | | || (_) || (_| || | | | / /_    / /
** |_.__/  \__, |     |_| |_||_| |_| \___/  \__,_||_| |_||____|  /_/
**          __/ |     on 25/06/2026.
**         |___/
*/


#include "metricd/collectors/DiskCollector.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <string_view>
#include <cstdlib>
#include <sys/statvfs.h>
#include <vector>

nlohmann::json DiskCollector::collect()
{
    std::ifstream mounts("/proc/mounts");
    char buffer[512];
    nlohmann::json filesystems = nlohmann::json::array();

    while (mounts.getline(buffer, sizeof(buffer))) {
        char dev[256], mnt[256], type[64];
        if (std::sscanf(buffer, "%255s %255s %63s", dev, mnt, type) < 3)
            continue;

        std::string_view fs_type(type);
        if (fs_type.rfind("ext", 0) != 0 &&
            fs_type.rfind("btrfs", 0) != 0 &&
            fs_type.rfind("xfs", 0) != 0 &&
            fs_type.rfind("ntfs", 0) != 0 &&
            fs_type.rfind("vfat", 0) != 0 &&
            fs_type != "zfs")
            continue;

        struct statvfs vfs{};
        if (statvfs(mnt, &vfs) != 0)
            continue;

        unsigned long long total  = vfs.f_blocks * vfs.f_frsize;
        unsigned long long avail  = vfs.f_bavail * vfs.f_frsize;
        unsigned long long used   = total - vfs.f_bfree * vfs.f_frsize;

        filesystems.push_back({
            {"mount", mnt},
            {"device", dev},
            {"total_gb", total / (1024.0 * 1024.0 * 1024.0)},
            {"used_gb", used / (1024.0 * 1024.0 * 1024.0)},
            {"available_gb", avail / (1024.0 * 1024.0 * 1024.0)},
            {"used_percent", total > 0
                ? (static_cast<double>(used) / static_cast<double>(total)) * 100.0
                : 0.0}
        });
    }

    return {{"filesystems", filesystems}};
}
