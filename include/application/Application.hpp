#pragma once
#include <asio/awaitable.hpp>
#include <asio/io_context.hpp>
#include <asio/signal_set.hpp>
#include <functional>
#include <thread>
#include <vector>

class Application {
   public:
    inline static asio::io_context io_ctx;
    std::function<void(void)> mainThreadFunction;
    bool isShutdown{false};

    Application() = default;
    ~Application();

    void registerInitFunction(const std::string name, std::function<void(void)> fun);
    void registerCoroutine(const std::string name, std::function<asio::awaitable<void>(void)> fun);
    void registerDedicatedThread(const std::string name, std::function<void(void)> fun);
    void start();
    void shutdown();

   private:
    inline static std::mutex shutdownMutex;
    asio::signal_set signals{io_ctx, SIGINT, SIGTERM};
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