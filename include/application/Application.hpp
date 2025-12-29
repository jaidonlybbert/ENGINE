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
	inline static asio::io_context io_ctx;
	std::function<void(void)> mainThreadFunction;

	Application() = default;

	void registerInitFunction(const std::string name, std::function<void(void)> fun);
	void registerCoroutineFunction(const std::string name, std::function<asio::awaitable<void>(void)> fun);
	void registerDedicatedThread(const std::string name, std::function<void(void)> fun);
	void start();
	void shutdown();

private:
	asio::signal_set signals{ io_ctx, SIGINT, SIGTERM };
	std::vector<std::function<void(void)>> initFunctions;
	std::vector<std::exception_ptr> initErrors;
	std::vector<std::function<asio::awaitable<void>(void)>> coroutineFunctions;
	std::vector<std::exception_ptr> coroutineErrors;
	std::vector<std::function<void(void)>> dedicatedThreadFunctions;
	std::vector<std::exception_ptr> dedicatedThreadErrors;
	std::vector<std::function<void(void)>> shutdownListeners;
	std::vector<std::thread> dedicatedThreads;

	void queueInitFunctions();
	void spawnCoroutines();
	void setSignalInterruptCallback();
	void startDedicatedThreads();
	void printRecordedExceptions(std::vector<std::exception_ptr> errs, const std::string& category);
};