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

#include "sockets/SocketSessionServer.h"

using asio::ip::tcp;
using asio::awaitable;
using asio::co_spawn;
using asio::detached;
using asio::redirect_error;
using asio::use_awaitable;


typedef std::shared_ptr<chat_participant> chat_participant_ptr;

//----------------------------------------------------------------------

void chat_room::join(chat_participant_ptr participant)
{
	participants_.insert(participant);
	for (auto msg : recent_msgs_)
		participant->deliver(msg);
}

void chat_room::leave(chat_participant_ptr participant)
{
	participants_.erase(participant);
}

void chat_room::deliver(const std::string& msg)
{
	recent_msgs_.push_back(msg);
	while (recent_msgs_.size() > max_recent_msgs)
		recent_msgs_.pop_front();

	for (auto participant : participants_)
		participant->deliver(msg);
}

chat_session::chat_session(tcp::socket socket, chat_room& room)
	: socket_(std::move(socket)),
	timer_(socket_.get_executor()),
	room_(room)
{
	timer_.expires_at(std::chrono::steady_clock::time_point::max());
}

void chat_session::start()
{
	room_.join(shared_from_this());

	co_spawn(socket_.get_executor(),
		[self = shared_from_this()] { return self->reader(); },
		detached);

	co_spawn(socket_.get_executor(),
		[self = shared_from_this()] { return self->writer(); },
		detached);
}

void chat_session::deliver(const std::string& msg)
{
	write_msgs_.push_back(msg);
	timer_.cancel_one();
}

awaitable<void> chat_session::reader()
{
	try
	{
		for (std::string read_msg;;)
		{
			std::size_t n = co_await asio::async_read_until(socket_,
				asio::dynamic_buffer(read_msg, 1024), "\n", use_awaitable);

			room_.deliver(read_msg.substr(0, n));
			read_msg.erase(0, n);
		}
	}
	catch (std::exception&)
	{
		stop();
	}
}

awaitable<void> chat_session::writer()
{
	try
	{
		while (socket_.is_open())
		{
			if (write_msgs_.empty())
			{
				asio::error_code ec;
				co_await timer_.async_wait(redirect_error(use_awaitable, ec));
			}
			else
			{
				co_await asio::async_write(socket_,
					asio::buffer(write_msgs_.front()), use_awaitable);
				write_msgs_.pop_front();
			}
		}
	}
	catch (std::exception&)
	{
		stop();
	}
}

void chat_session::stop()
{
	room_.leave(shared_from_this());
	socket_.close();
	timer_.cancel();
}


awaitable<void> listener(tcp::acceptor acceptor)
{
	chat_room room;

	for (;;)
	{
		std::make_shared<chat_session>(
			co_await acceptor.async_accept(use_awaitable),
			room
		)->start();
	}
}
