#include "application/Application.hpp"

#include<vector>
#include<functional>
#include<iostream>
#include<thread>

#include "asio/post.hpp"
#include "asio/io_context.hpp"
#include "asio/co_spawn.hpp"
#include "asio/detached.hpp"
#include "asio/signal_set.hpp"

void Application::queueInitFunctions() {
	for (auto& fun : initFunctions) {
		asio::post(io_ctx, fun);
	}
}

void Application::spawnCoroutines() {
	for (auto& fun : coroutineFunctions) {
		asio::co_spawn(io_ctx, fun, asio::detached);
	}
}

void Application::setSignalInterruptCallback() {
	asio::signal_set signals(io_ctx, SIGINT, SIGTERM);
	signals.async_wait([&](auto, auto){ shutdown(); });
}

void Application::startDedicatedThreads() {
	// Add background thread to run the async context
	dedicatedThreadFunctions.emplace_back([]() { io_ctx.run(); });
	dedicatedThreads.reserve(dedicatedThreadFunctions.size());
	for (auto& fun : dedicatedThreadFunctions) {
		dedicatedThreads.emplace_back(fun);
	}
}

void Application::start() {
	setSignalInterruptCallback();
	queueInitFunctions();
	spawnCoroutines();
	startDedicatedThreads();

	try {
		mainThreadFunction();
	}
	catch (std::exception e) {
		shutdown();
	}
}

void Application::shutdown() {
	for (auto& lst : shutdownListeners) {
		lst();
	}

	if (!io_ctx.stopped()) {
		io_ctx.stop();
	}
	
	for (auto& thd : dedicatedThreads) {
		if (thd.joinable()) {
			thd.join();
		}
	}

}
