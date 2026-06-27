#include "metricd/collectors/DiskCollector.hpp"
#include <nlohmann/json.hpp>
#include <cassert>
#include <iostream>

int main()
{
    DiskCollector collector;
    const auto json = collector.collect();

    assert(json.contains("filesystems"));
    assert(json["filesystems"].is_array());

    if (!json["filesystems"].empty()) {
        const auto& fs = json["filesystems"][0];
        assert(fs.contains("mount"));
        assert(fs.contains("total_gb"));
        assert(fs.contains("used_gb"));
        assert(fs.contains("available_gb"));
        assert(fs.contains("used_percent"));
        assert(fs.contains("disk_read_bytes"));
        assert(fs.contains("disk_write_bytes"));

        const auto total = fs["total_gb"].get<double>();
        assert(total > 0.0);
    }

    std::cout << "PASS: DiskCollector (" << json["filesystems"].size() << " filesystems)" << std::endl;
    return 0;
}
