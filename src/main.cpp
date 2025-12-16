#include <stdio.h>
#include <application.hpp>

// Necessary definition for PMP header compilation
#ifndef M_PI
#define M_PI 3.1415926
#endif

#include "interfaces/Logging.hpp"
#include "SocketSessionServer.h"
#include "scenes/SceneWorld.hpp"

#include "asio/post.hpp"
#include "asio/io_context.hpp"

#include <thread>
#include <functional>


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

		// Create server listening on port 8080
		SocketSessionServer server(io_context, 8080);
		std::cout << "Server listening on port 8080..." << std::endl;

		// Run in background thread
		std::thread io_thread([&io_context]() {
			io_context.run();
		});

		app.run(); 

		// Graceful shutdown sequence
		server.stop();           // Stop acceptor and close all sessions
		io_context.stop();       // Stop the io_context

		if (io_thread.joinable()) {
			io_thread.join();    // Wait for io thread to finish
		}


	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
	    	return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
