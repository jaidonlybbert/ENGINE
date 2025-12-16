//
// chat_server.cpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2022 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <asio/awaitable.hpp>
#include <asio/detached.hpp>
#include <asio/co_spawn.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/read_until.hpp>
#include <asio/redirect_error.hpp>
#include <asio/signal_set.hpp>
#include <asio/steady_timer.hpp>
#include <asio/use_awaitable.hpp>
#include <asio/write.hpp>

using asio::ip::tcp;
using asio::awaitable;
using asio::co_spawn;
using asio::detached;
using asio::redirect_error;
using asio::use_awaitable;


class chat_participant
{
public:
	virtual ~chat_participant() {}
	virtual void deliver(const std::string& msg) = 0;
};

typedef std::shared_ptr<chat_participant> chat_participant_ptr;


class chat_room
{
public:
    void join(chat_participant_ptr participant);
    void leave(chat_participant_ptr participant);
    void deliver(const std::string& msg);

private:
	std::set<chat_participant_ptr> participants_;
	enum { max_recent_msgs = 100 };
	std::deque<std::string> recent_msgs_;
};


class chat_session
  : public chat_participant,
    public std::enable_shared_from_this<chat_session>
{
public:
    chat_session(tcp::socket socket, chat_room& room);
    void start();
    void deliver(const std::string& msg);

private:
    awaitable<void> reader();
    awaitable<void> writer();
    void stop();

	tcp::socket socket_;
	asio::steady_timer timer_;
	chat_room& room_;
	std::deque<std::string> write_msgs_;
};

awaitable<void> listener(tcp::acceptor acceptor);
