#pragma once
#include <memory>
#include <set>
#include <iostream>

#include "asio.hpp"

using asio::ip::tcp;

namespace ENG {
// Represents a single client connection
class Session : public std::enable_shared_from_this<Session> {
public:
	Session(tcp::socket socket);

	void start();
	void send(const std::string& message);
	tcp::socket& socket();
	void close();

private:
	void do_read();

	tcp::socket socket_;
	std::array<char, 1024> buffer_;
};

// Server that accepts connections and manages sessions
class SocketSessionServer {
public:
	SocketSessionServer(asio::io_context& io_context, short port);

	void broadcast(const std::string& message);
	void stop();

private:
	void do_accept();

	tcp::acceptor acceptor_;
	std::set<std::shared_ptr<Session>> sessions_;
	std::mutex sessions_mutex_;
};
}
