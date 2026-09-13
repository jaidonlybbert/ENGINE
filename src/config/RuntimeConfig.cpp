#include "config/RuntimeConfig.hpp"

#include <fstream>
#include <stdexcept>
#include <string>

#include "EngineConfig.hpp"
#include "logger/Logging.hpp"
#include "nlohmann/json.hpp"

namespace ENG {

RuntimeConfig default_runtime_config() {
    RuntimeConfig config{};
    config.windowWidth = WIDTH;
    config.windowHeight = HEIGHT;
    config.startupScene = "world";
    return config;
}

namespace {

std::string find_config_path(int argc, char** argv) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (std::string(argv[i]) == "--config") {
            return argv[i + 1];
        }
    }
    return {};
}

void overlay_window(RuntimeConfig& config, const nlohmann::json& root) {
    const auto window = root.find("window");
    if (window == root.end()) {
        return;
    }
    if (const auto width = window->find("width"); width != window->end()) {
        config.windowWidth = width->get<std::uint32_t>();
    }
    if (const auto height = window->find("height"); height != window->end()) {
        config.windowHeight = height->get<std::uint32_t>();
    }
}

void overlay_scene(RuntimeConfig& config, const nlohmann::json& root) {
    if (const auto scene = root.find("scene"); scene != root.end()) {
        config.startupScene = scene->get<std::string>();
    }
}

}  // namespace

RuntimeConfig load_runtime_config(int argc, char** argv) {
    RuntimeConfig config = default_runtime_config();

    const std::string path = find_config_path(argc, argv);
    if (path.empty()) {
        return config;
    }

    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Could not open config file: " + path);
    }

    nlohmann::json root;
    try {
        file >> root;
    } catch (const nlohmann::json::exception& e) {
        throw std::runtime_error("Invalid JSON in config file '" + path + "': " + e.what());
    }

    overlay_window(config, root);
    overlay_scene(config, root);
    ENG_LOG_INFO("Loaded configuration from " << path);
    return config;
}

}  // namespace ENG
