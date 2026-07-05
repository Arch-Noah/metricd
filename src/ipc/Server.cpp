#include <utility>

#include "metricd/ipc/Server.hpp"
#include "metricd/collectors/MemoryCollector.hpp"
#include "metricd/collectors/CpuCollector.hpp"
#include "metricd/collectors/DiskCollector.hpp"
#include "metricd/collectors/NetworkCollector.hpp"
#include "metricd/collectors/GpuCollector.hpp"
#include "metricd/collectors/TemperatureCollector.hpp"
#include "metricd/collectors/BatteryCollector.hpp"
#include "metricd/collectors/SystemCollector.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/timerfd.h>
#include <sys/stat.h>
#include <chrono>

using namespace metricd;

ipc::Server::Server(const metricd::Config& config)  :
    config_(config),
    socketPath(config.socket_path),
    intervalSec(config.interval)
{
    if (socketPath.empty()) throw std::invalid_argument("socketPath cannot be empty");
    if (intervalSec <= 0) throw std::invalid_argument("intervalSec must be greater than 0");
    if (io_uring_queue_init(256, &ring, 0) < 0) {
        throw std::runtime_error("Failed to initialize io_uring queue");
    }

    cpu_collector_ = std::make_unique<CpuCollector>(config.enable_per_core);
    mem_collector_ = std::make_unique<MemoryCollector>();
    disk_collector_ = std::make_unique<DiskCollector>();
    net_collector_ = std::make_unique<NetworkCollector>();
    gpu_collector_ = std::make_unique<GpuCollector>();
    temp_collector_ = std::make_unique<TemperatureCollector>();
    battery_collector_ = std::make_unique<BatteryCollector>();
    system_collector_ = std::make_unique<SystemCollector>();

    initServer();
    initTimer();
}

ipc::Server::~Server() {
    shutdown();
}

void ipc::Server::initServer()
{
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) {
        throw std::runtime_error("Failed to create socket");
    }
    struct sockaddr_un addr{};
    std::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;

    if (socketPath.size() >= sizeof(addr.sun_path)) {
        throw std::invalid_argument("socketPath is too long");
    }

    std::strncpy(addr.sun_path, socketPath.c_str(), sizeof(addr.sun_path) - 1);
    unlink(socketPath.c_str());

    const mode_t old_umask = umask(0077);
    if (bind(server_fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        umask(old_umask);
        close(server_fd);
        throw std::runtime_error("Failed to bind socket");
    }
    umask(old_umask);

    if (listen(server_fd, SOMAXCONN) < 0) {
        close(server_fd);
        throw std::runtime_error("Failed to listen on socket");
    }
}

bool ipc::Server::ensureSocketExists()
{
    struct stat st;
    if (stat(socketPath.c_str(), &st) == 0)
        return true;

    int new_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (new_fd < 0) return false;

    struct sockaddr_un addr{};
    std::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, socketPath.c_str(), sizeof(addr.sun_path) - 1);

    const mode_t old_umask = umask(0077);
    int rc = bind(new_fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));
    umask(old_umask);
    if (rc < 0) { close(new_fd); return false; }

    if (listen(new_fd, SOMAXCONN) < 0) { close(new_fd); return false; }

    int old_fd = server_fd;
    server_fd = new_fd;
    close(old_fd);
    queueAccept();
    return true;
}

void ipc::Server::initTimer()
{
    timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (timer_fd < 0) {
        throw std::runtime_error("Failed to create timerfd");
    }
    struct itimerspec iterspec{};
    std::memset(&iterspec, 0, sizeof(iterspec));

    iterspec.it_value.tv_sec = intervalSec;
    iterspec.it_value.tv_nsec = 0;

    iterspec.it_interval.tv_sec = intervalSec;
    iterspec.it_interval.tv_nsec = 0;

    if (timerfd_settime(timer_fd, 0, &iterspec, nullptr) < 0) {
        close(timer_fd);
        close(server_fd);
        throw std::runtime_error("Failed to set timerfd");
    }
}

void ipc::Server::run()
{
    queueAccept();
    queueTimer();
    io_uring_submit(&ring);

    struct io_uring_cqe *cqe;

    while (running) {
        const int ret = io_uring_wait_cqe(&ring, &cqe);
        if (ret < 0) { continue; }
        handleCompletion(cqe);
        io_uring_cqe_seen(&ring, cqe);
    }
}

void ipc::Server::shutdown() {
    if (running) {
        stop();
    }

    if (timer_fd != -1) {
        close(timer_fd);
        timer_fd = -1;
    }

    if (server_fd != -1) {
        close(server_fd);
        unlink(socketPath.c_str());
        server_fd = -1;
    }
    io_uring_queue_exit(&ring);
}

void ipc::Server::stop() {
    running = false;
}

void ipc::Server::queueAccept()
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
    if (!sqe) {
        return;
    }
    auto* ctx = new IOContext(OpType::ACCEPT, server_fd);
    io_uring_prep_accept(sqe, server_fd, nullptr, nullptr, 0);
    io_uring_sqe_set_data(sqe, ctx);
}

void ipc::Server::queueTimer()
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
    if (!sqe) return;

    auto* ctx = new IOContext(OpType::TIMER, timer_fd);
    io_uring_prep_read(sqe, timer_fd, ctx->buffer.data(), 8, 0);
    io_uring_sqe_set_data(sqe, ctx);
}

void ipc::Server::queueRead(int client_fd)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
    if (!sqe) return;

    auto* ctx = new IOContext(OpType::READ, client_fd);
    io_uring_prep_recv(sqe, client_fd, ctx->buffer.data(), ctx->buffer.size(), 0);
    io_uring_sqe_set_data(sqe, ctx);
}

void ipc::Server::queueWrite(int client_fd, const std::vector<char>& data)
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
    if (!sqe) return;

    auto* ctx = new IOContext(OpType::WRITE, client_fd);
    ctx->buffer = data;
    io_uring_prep_send(sqe, client_fd, ctx->buffer.data(), ctx->buffer.size(), MSG_DONTWAIT);
    io_uring_sqe_set_data(sqe, ctx);
}

void ipc::Server::handleCompletion(struct io_uring_cqe* cqe)
{
    const auto* ctx = static_cast<IOContext*>(io_uring_cqe_get_data(cqe));
    if (!ctx) return;

    if (cqe->res < 0) {
        if (ctx->type == OpType::WRITE) {
            removeClient(ctx->fd);
        } else if (ctx->type != OpType::ACCEPT && ctx->type != OpType::TIMER) {
            removeClient(ctx->fd);
        }
        delete ctx;
        return;
    }
    switch (ctx->type)
    {
        case OpType::ACCEPT: {
                const int client_fd = cqe->res;
                onClientConnected(client_fd);
                queueRead(client_fd);
                queueAccept();
                io_uring_submit(&ring);
                break;
            }
        case OpType::TIMER:
            {
                handleTimerTrigger();
                queueTimer();
                io_uring_submit(&ring);
                break;
            }
        case OpType::READ:
            {
                const int bytes_read = cqe->res;
                if (bytes_read == 0) {
                    onClientDisconnected(ctx->fd);
                    removeClient(ctx->fd);
                } else {
                    const std::string msg(ctx->buffer.data(), bytes_read);
                    onMessage(ctx->fd, msg);
                    queueRead(ctx->fd);
                    io_uring_submit(&ring);
                }
                break;
            }
        case OpType::WRITE: { break; }
    }
    delete ctx;
}

void ipc::Server::onClientConnected(int client_fd) {
    int flags = fcntl(client_fd, F_GETFL, 0);
    fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
    connectedClients.insert(client_fd);
}

void ipc::Server::onClientDisconnected(int client_fd) {
    connectedClients.erase(client_fd);
}

void ipc::Server::removeClient(int client_fd)
{
    close(client_fd);
    connectedClients.erase(client_fd);
}

void ipc::Server::onMessage(int /*client_fd*/, std::string /*msg*/)
{
}

std::vector<std::string> ipc::Server::collectMetrics()
{
    std::vector<std::string> messages;

    const auto now = std::chrono::system_clock::now();
    const long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();

    auto make_msg = [&](const std::string& type, nlohmann::json data) -> std::string {
        data["type"] = type;
        data["timestamp"] = timestamp;
        data["proto_version"] = 1;
        return data.dump();
    };

    if (config_.enable_cpu)         messages.push_back(make_msg(cpu_collector_->name(), cpu_collector_->collect()));
    if (config_.enable_memory)      messages.push_back(make_msg(mem_collector_->name(), mem_collector_->collect()));
    if (config_.enable_disk)        messages.push_back(make_msg(disk_collector_->name(), disk_collector_->collect()));
    if (config_.enable_network)     messages.push_back(make_msg(net_collector_->name(), net_collector_->collect()));
    if (config_.enable_gpu)         messages.push_back(make_msg(gpu_collector_->name(), gpu_collector_->collect()));
    if (config_.enable_temperature) messages.push_back(make_msg(temp_collector_->name(), temp_collector_->collect()));
    if (config_.enable_battery)     messages.push_back(make_msg(battery_collector_->name(), battery_collector_->collect()));
    if (config_.enable_system)      messages.push_back(make_msg(system_collector_->name(), system_collector_->collect()));

    return messages;
}

void ipc::Server::handleTimerTrigger()
{
    ensureSocketExists();
    const auto messages = collectMetrics();
    std::vector<char> data;
    for (const auto& msg : messages) {
        data.insert(data.end(), msg.begin(), msg.end());
        data.push_back('\n');
    }

    broadcast(data);
}

void ipc::Server::broadcast(const std::vector<char>& data)
{
    for (const int fd : connectedClients) {
        queueWrite(fd, data);
    }
    io_uring_submit(&ring);
}
