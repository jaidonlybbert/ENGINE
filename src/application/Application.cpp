#include "application/Application.hpp"
#include "logger/Logging.hpp"

#include<vector>
#include<functional>
#include<iostream>
#include<thread>

#include "asio/post.hpp"
#include "asio/io_context.hpp"
#include "asio/co_spawn.hpp"
#include "asio/detached.hpp"

Application::~Application() {
	shutdown();
}

void Application::registerInitFunction(const std::string name, std::function<void(void)> fun)
{
	initErrors.emplace_back(nullptr);
	std::exception_ptr& this_error = initErrors.back();
	initFunctions.emplace_back([fun, name, &this_error]() {
		try {
			fun();
		}
		catch (std::exception e) {
			std::stringstream s;
			s << "Caught exception in init thread [" << name << "]: " << e.what() << std::endl;
			this_error = std::make_exception_ptr(std::runtime_error(s.str()));
		}
		});
}

void Application::registerCoroutine(const std::string name, std::function<asio::awaitable<void>(void)> fun)
{
	coroutineErrors.emplace_back(nullptr);
	std::exception_ptr& this_error = coroutineErrors.back();
	coroutineFunctions.emplace_back([name, fun, &this_error]() -> asio::awaitable<void> {
		try {
			co_await fun();
		}
		catch (std::exception& e) {
			std::stringstream s;
			s << "Caught exception in coroutine [" << name << "]: " << e.what() << std::endl;
			this_error = std::make_exception_ptr(std::runtime_error(s.str()));
		}

		co_return;
		});
}

void Application::registerDedicatedThread(const std::string name, std::function<void(void)> fun) {
	dedicatedThreadErrors.emplace_back(nullptr);
	std::exception_ptr& this_error = dedicatedThreadErrors.back();
	dedicatedThreadFunctions.emplace_back([name, fun, &this_error]() {
		try {
			return fun();
		}
		catch (std::exception e) {
			std::stringstream s;
			s << "Caught exception in dedicated thread [" << name << "]: " << e.what() << std::endl;
			this_error = std::make_exception_ptr(std::runtime_error(s.str()));
		}
		});
}


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
	signals.async_wait([&](auto, auto){
		shutdown(); 
		});
}

void Application::startDedicatedThreads() {
	// Add background thread to run the async context
	registerDedicatedThread("io_context.run()", []() { io_ctx.run(); });
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
		// MacOS requires certain calls to be made through the main thread
		// other OSs may have similar requirements
		mainThreadFunction();
	}
	catch (std::exception e) {
		ENG_LOG_ERROR("Exception in main thread: " << e.what());
		shutdown();
	}
}

void Application::shutdown() {
	// only shutdown once, can be triggered manually (e.g. SIGINT) or by destructor
	if (isShutdown) {
		return;
	}
	isShutdown = true;

	for (auto& lst : shutdownListeners) {
		try {
			lst();
		}
		catch (std::exception& e) {
			ENG_LOG_ERROR("Exception in shutdown listener: " << e.what() << std::endl);
		}
	}

	if (!io_ctx.stopped()) {
		try {
			io_ctx.stop();
		}
		catch (std::exception& e) {
			ENG_LOG_ERROR("Exeption in io_ctx.stop(): " << e.what() << std::endl);
		}
	}

	for (auto& thd : dedicatedThreads) {
		if (thd.joinable()) {
			try {
				thd.join();
			}
			catch (std::exception& e) {
				ENG_LOG_ERROR("Exception calling join on a thread: " << e.what() << std::endl);
			}
		}
	}

	printRecordedExceptions(initErrors, "init");
	printRecordedExceptions(coroutineErrors, "coroutine");
	printRecordedExceptions(dedicatedThreadErrors, "dedicated");
}

void Application::printRecordedExceptions(std::vector<std::exception_ptr> errs, const std::string& category)
{
	for (auto& err : errs) {
		if (err) {
			try {
				std::rethrow_exception(err);
			}
			catch (std::exception& e) {
				ENG_LOG_ERROR("Exception was recorded in " << category << " thread: " << e.what() << std::endl);
			}
		}
	}
}
