#pragma once
#include <asio/awaitable.hpp>
#include <asio/io_context.hpp>
#include <asio/signal_set.hpp>
#include <functional>
#include <mutex>
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

    // Lifecycle hooks for platforms where the OS can tear down and later restore the
    // rendering surface out from under a running app (Android: APP_CMD_TERM_WINDOW/
    // APP_CMD_INIT_WINDOW/APP_CMD_PAUSE/APP_CMD_RESUME - see issue #48 and
    // src/main_android.cpp). Desktop backends (GLFW) never call the notify* methods -
    // the window/surface never gets torn down by the OS underneath them the way it does
    // on Android - so callbacks registered here simply stay dormant there.
    void registerSurfaceDestroyedCallback(std::function<void(void)> fun);
    void registerSurfaceCreatedCallback(std::function<void(void)> fun);
    void registerPauseCallback(std::function<void(void)> fun);
    void registerResumeCallback(std::function<void(void)> fun);

    // Invoked by the platform-specific main loop when the OS actually fires the
    // corresponding event. Not called automatically by start()/shutdown() - something
    // platform-specific (e.g. android_main's command dispatch) is expected to call these.
    void notifySurfaceDestroyed();
    void notifySurfaceCreated();
    void notifyPause();
    void notifyResume();

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

    std::vector<std::function<void(void)>> surfaceDestroyedListeners;
    std::vector<std::function<void(void)>> surfaceCreatedListeners;
    std::vector<std::function<void(void)>> pauseListeners;
    std::vector<std::function<void(void)>> resumeListeners;
    void notifyListeners(std::vector<std::function<void(void)>>& listeners, const std::string& category);

    void queueInitFunctions();
    void spawnCoroutines();
    void setSignalInterruptCallback();
    void startDedicatedThreads();
    void printRecordedExceptions(std::vector<std::exception_ptr> errs, const std::string& category);
};