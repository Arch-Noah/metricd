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

#include "metricd/core/Daemon.hpp"
#include "Logger.hpp"
#include <csignal>
#include <cstring>

namespace metricd {

Daemon* Daemon::instance_ = nullptr;

Daemon::Daemon(const Config& config)
    : config_(config)
    , server_(config.socket_path, config.interval)
{
    instance_ = this;
}

Daemon::~Daemon()
{
    instance_ = nullptr;
}

void Daemon::signalHandler(int sig)
{
    if (instance_) {
        instance_->stop();
    }
}

void Daemon::setupSignals()
{
    struct sigaction sa{};
    std::memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &Daemon::signalHandler;
    sigfillset(&sa.sa_mask);

    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGQUIT, &sa, nullptr);

    signal(SIGPIPE, SIG_IGN);
}

int Daemon::run()
{
    setupSignals();
    LOG(Logger::LogLevel::INFO, "listening on %s (interval=%ds)",
        config_.socket_path.c_str(), config_.interval);
    server_.run();
    LOG(Logger::LogLevel::INFO, "shut down");
    return 0;
}

void Daemon::stop()
{
    server_.stop();
}

}
