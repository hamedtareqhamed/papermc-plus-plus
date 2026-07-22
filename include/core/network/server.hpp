#ifndef PAPERMC_CORE_NETWORK_SERVER_HPP
#define PAPERMC_CORE_NETWORK_SERVER_HPP

#include <string>
#include <vector>
#include <thread>
#include <asio.hpp>
#include <spdlog/spdlog.h>
#include "core/network/connection.hpp"

namespace papermc::core::network {

class ServerEngine {
public:
    ServerEngine(std::string host, uint16_t port, std::size_t thread_count = 4);
    ~ServerEngine();

    void start();
    void stop();

private:
    void do_accept();

    std::string host_;
    uint16_t port_;
    std::size_t thread_count_;

    asio::io_context io_context_;
    asio::ip::tcp::acceptor acceptor_;
    std::vector<std::jthread> worker_threads_;
    bool running_{false};
};

} // namespace papermc::core::network

#endif // PAPERMC_CORE_NETWORK_SERVER_HPP
