#include <stdio.h>
#include <renderer/Renderer.hpp>

// Necessary definition for PMP header compilation
#ifndef M_PI
#define M_PI 3.1415926
#endif

#include "logger/Logging.hpp"
#include "sockets/SocketSessionServer.h"
#include "scenes/SceneWorld.hpp"

#include "asio/post.hpp"
#include "asio/io_context.hpp"
#include "asio/co_spawn.hpp"
#include "asio/detached.hpp"

#include <thread>
#include <functional>

void stop(asio::io_context& io_context) {
	if (io_context.stopped()) {
		return;
	}

	io_context.stop();
}

int main() {
	
	try {
		printf("Starting app\n");
		// ENG_LOG_TRACE("Application path: " << install_dir.native().c_str() << std::endl);

		VulkanTemplateApp app;
		ENG_LOG_INFO(app);

		asio::io_context io_context;

		asio::post([&app] () {
			app.initializeScene(initializeWorldScene);
		});


		co_spawn(io_context,
			listener(tcp::acceptor(io_context, { tcp::v4(), 8080 })),
			detached);

		std::cout << "Server listening on port 8080..." << std::endl;

		asio::signal_set signals(io_context, SIGINT, SIGTERM);
		signals.async_wait([&](auto, auto){ stop(io_context); });

		// Run in background thread
		std::thread io_thread([&io_context]() {
			io_context.run();
		});

		app.run(); 

		// Graceful shutdown sequence
		stop(io_context);

		if (io_thread.joinable()) {
			io_thread.join();    // Wait for io thread to finish
		}


	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
	    	return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
