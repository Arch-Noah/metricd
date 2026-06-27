#include <utility>

#include "metricd/ipc/Server.hpp"
#include "metricd/collectors/MemoryCollector.hpp"
#include "metricd/collectors/CpuCollector.hpp"
#include "metricd/collectors/DiskCollector.hpp"
#include "metricd/collectors/NetworkCollector.hpp"
#include "metricd/collectors/GpuCollector.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/timerfd.h>

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

    if (bind(server_fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(server_fd);
        throw std::runtime_error("Failed to bind socket");
    }
    if (listen(server_fd, SOMAXCONN) < 0) {
        close(server_fd);
        throw std::runtime_error("Failed to listen on socket");
    }
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
    io_uring_prep_send(sqe, client_fd, ctx->buffer.data(), ctx->buffer.size(), 0);
    io_uring_sqe_set_data(sqe, ctx);
}

void ipc::Server::handleCompletion(struct io_uring_cqe* cqe)
{
    const auto* ctx = static_cast<IOContext*>(io_uring_cqe_get_data(cqe));
    if (!ctx) return;

    if (cqe->res < 0) {
        if (ctx->type != OpType::ACCEPT && ctx->type != OpType::TIMER){
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

std::string ipc::Server::collectMetrics()
{
    nlohmann::json all;

    if (config_.enable_cpu)     all[cpu_collector_->name()]     = cpu_collector_->collect();
    if (config_.enable_memory)  all[mem_collector_->name()]     = mem_collector_->collect();
    if (config_.enable_disk)    all[disk_collector_->name()]    = disk_collector_->collect();
    if (config_.enable_network) all[net_collector_->name()]     = net_collector_->collect();
    if (config_.enable_gpu)     all[gpu_collector_->name()]     = gpu_collector_->collect();

    return all.dump();
}

void ipc::Server::handleTimerTrigger()
{
    broadcast(collectMetrics());
}

void ipc::Server::broadcast(const std::string& jsonMsg)
{
    std::vector<char> data(jsonMsg.begin(), jsonMsg.end());
    data.push_back('\n');

    for (const int fd : connectedClients) {
        queueWrite(fd, data);
    }
    io_uring_submit(&ring);
}
