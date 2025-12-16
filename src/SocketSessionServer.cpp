#include <memory>
#include <set>
#include <iostream>

#include "asio.hpp"
#include "SocketSessionServer.h"
#include "interfaces/Logging.hpp"

using asio::ip::tcp;

namespace ENG {

// Represents a single client connection
Session::Session(tcp::socket socket) : socket_(std::move(socket)) {}

void Session::start() {
	do_read();
}

void Session::send(const std::string& message) {
	auto self = shared_from_this();
	asio::async_write(socket_,
		asio::buffer(message),
		[this, self](asio::error_code ec, std::size_t) {
			if (ec) {
				ENG_LOG_ERROR("Write error: " << ec.message() << std::endl);
			}
		});
}

tcp::socket& Session::socket() { return socket_; }

void Session::close() {
	asio::error_code ec;
	ec = socket_.shutdown(tcp::socket::shutdown_both, ec);

	if (ec) {
		ENG_LOG_ERROR("Error shutting down socket!" << std::endl);
	}
	
	ec = socket_.close(ec);
	if (ec) {
		ENG_LOG_ERROR("Error closing socket!" << std::endl);
	}
}

void Session::do_read() {
	auto self = shared_from_this();
	socket_.async_read_some(
		asio::buffer(buffer_),
		[this, self](asio::error_code ec, std::size_t length) {
			if (!ec) {
				// Echo back what we received
				std::string message(buffer_.data(), length);
				ENG_LOG_DEBUG("Received: " << message << std::endl);
				send(message);

				// Continue reading
				do_read();
			} else {
				ENG_LOG_DEBUG("Read error: " << ec.message() << std::endl);
			}
		});
}

// Server that accepts connections and manages sessions
SocketSessionServer::SocketSessionServer(asio::io_context& io_context, short port)
: acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
	do_accept();
}

void SocketSessionServer::broadcast(const std::string& message) {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
	for (auto& session : sessions_) {
		session->send(message);
	}
}

void SocketSessionServer::stop() {
	std::lock_guard<std::mutex> lock(sessions_mutex_);

	// Stop accepting new connections
	asio::error_code ec;
	ec = acceptor_.close(ec);

	if (ec) {
		ENG_LOG_ERROR("Error closing acceptor!" << std::endl);
	}

	// Close all existing sessions
	for (auto& session : sessions_) {
		session->close();
	}
	sessions_.clear();
}

void SocketSessionServer::do_accept() {
	acceptor_.async_accept(
		[this](asio::error_code ec, tcp::socket socket) {
			if (!ec) {
				ENG_LOG_DEBUG("Client connected from: " 
					<< socket.remote_endpoint() << std::endl);

				std::lock_guard<std::mutex> lock(sessions_mutex_);
				auto session = std::make_shared<Session>(std::move(socket));
				sessions_.insert(session);
				session->start();
			}

			// Accept next connection
			do_accept();

			// Only continue accepting if acceptor is still open
			if (acceptor_.is_open()) {
				do_accept();
			}
		});
}

} // end namespace
