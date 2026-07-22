#include "core/network/server.hpp"

namespace papermc::core::network {

ServerEngine::ServerEngine(std::string host, uint16_t port, std::size_t thread_count, bool offline_mode)
    : host_(std::move(host)),
      port_(port),
      thread_count_(thread_count),
      offline_mode_(offline_mode),
      acceptor_(io_context_) {
    asio::ip::tcp::resolver resolver(io_context_);
    asio::ip::tcp::endpoint endpoint = *resolver.resolve(host_, std::to_string(port_)).begin();

    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(asio::ip::tcp::acceptor::reuse_address(true));
    acceptor_.bind(endpoint);
    acceptor_.listen();
}

ServerEngine::~ServerEngine() {
    stop();
}

void ServerEngine::start() {
    if (running_) return;
    running_ = true;

    spdlog::info("Starting PaperMC++ Server Engine on {}:{} (threads: {}, offline-mode: {})...",
                 host_, port_, thread_count_, offline_mode_);

    do_accept();

    for (std::size_t i = 0; i < thread_count_; ++i) {
        worker_threads_.emplace_back([this](std::stop_token st) {
            while (!st.stop_requested()) {
                try {
                    io_context_.run();
                } catch (const std::exception& ex) {
                    spdlog::error("Worker thread exception: {}", ex.what());
                }
            }
        });
    }
}

void ServerEngine::stop() {
    if (!running_) return;
    running_ = false;

    spdlog::info("Stopping PaperMC++ Server Engine...");
    io_context_.stop();

    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.request_stop();
        }
    }
    worker_threads_.clear();
    spdlog::info("Server Engine shut down clean.");
}

void ServerEngine::do_accept() {
    acceptor_.async_accept(
        [this](asio::error_code ec, asio::ip::tcp::socket socket) {
            if (!ec) {
                std::make_shared<ServerConnection>(std::move(socket), offline_mode_)->start();
            } else {
                spdlog::warn("Accept error: {}", ec.message());
            }

            if (running_) {
                do_accept();
            }
        }
    );
}

} // namespace papermc::core::network
