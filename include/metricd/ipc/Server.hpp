#pragma once

#include <string>
#include <vector>
#include <set>
#include <memory>
#include <liburing.h>
#include "metricd/collectors/ICollector.hpp"
#include "metricd/core/Config.hpp"

namespace metricd::ipc
{
    enum class OpType {
        ACCEPT,
        READ,
        WRITE,
        TIMER
    };

    struct IOContext {
        OpType type;
        int fd;
        std::vector<char> buffer;

        explicit IOContext(OpType t, int f = -1) : type(t), fd(f), buffer(4096) {}
    };

    class Server {
        public:
            explicit Server(const metricd::Config& config);
            Server(Server& s) = delete ;
            Server& operator=(Server& s) = delete ;
            ~Server();

            void run();
            void stop();
            void shutdown();

        private:
            metricd::Config config_;
            std::string socketPath;
            int intervalSec{};
            int server_fd = -1;
            int timer_fd = -1;
            bool running = true;

            struct io_uring ring{};
            std::set<int> connectedClients;

            std::unique_ptr<ICollector> cpu_collector_;
            std::unique_ptr<ICollector> mem_collector_;
            std::unique_ptr<ICollector> disk_collector_;
            std::unique_ptr<ICollector> net_collector_;
            std::unique_ptr<ICollector> gpu_collector_;
            std::unique_ptr<ICollector> temp_collector_;
            std::unique_ptr<ICollector> battery_collector_;
            std::unique_ptr<ICollector> system_collector_;

            void initServer();
            bool ensureSocketExists();
            void initTimer();

            void queueAccept();
            void queueRead(int client_fd);
            void queueTimer();
            void queueWrite(int client_fd, const std::vector<char>& data);

            void handleCompletion(struct io_uring_cqe* cqe);
            void handleTimerTrigger();
            void removeClient(int client_fd);
            void broadcast(const std::vector<char>& data);

            void onClientConnected(int client_fd);
            void onClientDisconnected(int client_fd);
            void onMessage(int client_fd, std::string msg);

            std::vector<std::string> collectMetrics();
    };
}
