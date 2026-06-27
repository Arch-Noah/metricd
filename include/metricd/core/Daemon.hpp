#pragma once

#include "metricd/core/Config.hpp"
#include "metricd/ipc/Server.hpp"

namespace metricd {

class Daemon {
public:
    explicit Daemon(const Config& config);
    ~Daemon();
    int run();
    void stop();

private:
    Config config_;
    ipc::Server server_;
    static Daemon* instance_;

    static void signalHandler(int sig);
    static void setupSignals();
};

}
