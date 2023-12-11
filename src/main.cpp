#include<iostream>
#include<stdexcept>
#include<cstdlib>
#include "GLFW/glfw3.h"
#include "vulkan/vulkan.hpp"
#include "EngineConfig.h"

using namespace std;

class VulkanTemplateApp {
public:
	void run() {
		initVulkan();
		mainLoop();
		cleanup();
	}

private:
	void initVulkan() {

	}

	void mainLoop() {

	}

	void cleanup() {

	}
};

int main() {
    cout << "Engine Version: " << Engine_VERSION_MAJOR << "." 
         << Engine_VERSION_MINOR << endl;

    VulkanTemplateApp app;

    try {
	    app.run();
    } catch (const std::exception& e) {
	    std::cerr << e.what() << std::endl;
	    return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
