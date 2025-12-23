#include<vector>
#include<functional>
#include<iostream>
#include<thread>

#include "asio/post.hpp"
#include "asio/io_context.hpp"
#include "asio/co_spawn.hpp"
#include "asio/detached.hpp"
#include "asio/signal_set.hpp"

class Application {

public:
	std::vector<std::function<void(void)>> initFunctions;
	std::vector<std::function<asio::awaitable<void>(void)>> coroutineFunctions;
	std::vector<std::function<void(void)>> dedicatedThreadFunctions;
	std::vector<std::function<void(void)>> shutdownListeners;
	std::vector<std::thread> dedicatedThreads;
	std::function<void(void)> mainThreadFunction;
	inline static asio::io_context io_ctx;

	Application() = default;

	void queueInitFunctions();
	void spawnCoroutines();
	void setSignalInterruptCallback();
	void startDedicatedThreads();
	void start();
	void shutdown();
};