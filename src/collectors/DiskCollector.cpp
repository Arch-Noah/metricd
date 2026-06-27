#include "metricd/collectors/DiskCollector.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <string_view>
#include <cstdlib>
#include <cstring>
#include <sys/statvfs.h>
#include <unordered_map>

static std::unordered_map<std::string, std::pair<unsigned long long, unsigned long long>> readDiskStats()
{
    std::ifstream stats("/proc/diskstats");
    char buffer[256];
    std::unordered_map<std::string, std::pair<unsigned long long, unsigned long long>> result;

    while (stats.getline(buffer, sizeof(buffer))) {
        char* ptr = buffer;
        std::strtoull(ptr, &ptr, 10); ++ptr;
        std::strtoull(ptr, &ptr, 10); ++ptr;

        char* name_start = ptr;
        while (*ptr && *ptr != ' ') ++ptr;
        std::string dev(name_start, ptr - name_start);
        if (!ptr) continue;
        ++ptr;

        for (int i = 0; i < 3; ++i) {
            std::strtoull(ptr, &ptr, 10); ++ptr;
        }
        const unsigned long long rsect = std::strtoull(ptr, &ptr, 10); ++ptr;

        for (int i = 0; i < 3; ++i) {
            std::strtoull(ptr, &ptr, 10); ++ptr;
        }
        const unsigned long long wsect = std::strtoull(ptr, &ptr, 10);

        result[dev] = {rsect * 512, wsect * 512};
    }
    return result;
}

nlohmann::json DiskCollector::collect()
{
    std::ifstream mounts("/proc/mounts");
    char buffer[512];
    nlohmann::json filesystems = nlohmann::json::array();
    const auto disk_stats = readDiskStats();

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

        nlohmann::json fs_entry;
        fs_entry["mount"] = mnt;
        fs_entry["device"] = dev;
        fs_entry["total_gb"] = total / (1024.0 * 1024.0 * 1024.0);
        fs_entry["used_gb"] = used / (1024.0 * 1024.0 * 1024.0);
        fs_entry["available_gb"] = avail / (1024.0 * 1024.0 * 1024.0);
        fs_entry["used_percent"] = total > 0
            ? (static_cast<double>(used) / static_cast<double>(total)) * 100.0
            : 0.0;

        const char* slash = std::strrchr(dev, '/');
        const char* devname = slash ? slash + 1 : dev;
        auto it = disk_stats.find(devname);
        if (it != disk_stats.end()) {
            fs_entry["disk_read_bytes"] = it->second.first;
            fs_entry["disk_write_bytes"] = it->second.second;
        } else {
            fs_entry["disk_read_bytes"] = 0;
            fs_entry["disk_write_bytes"] = 0;
        }

        filesystems.push_back(fs_entry);
    }

    return {{"filesystems", filesystems}};
}
