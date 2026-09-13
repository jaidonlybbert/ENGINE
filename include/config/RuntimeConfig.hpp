#pragma once
#include <cstdint>
#include <string>

namespace ENG {

// Runtime-tunable application settings. Currently the initial window size and which
// scene to load at startup; this is intended to grow further (render adapter, keybinds
// - see issue #3).
struct RuntimeConfig {
    std::uint32_t windowWidth;
    std::uint32_t windowHeight;
    // Which scene main() loads at startup. Recognized values: "world", "blue_sky".
    // Unrecognized values fall back to "world".
    std::string startupScene;
};

// The compiled-in defaults (from EngineConfig.hpp).
RuntimeConfig default_runtime_config();

// Start from the compiled-in defaults, then, if the command line contains
// `--config <path>`, overlay the values present in that JSON file.
// Throws std::runtime_error if an explicitly requested file cannot be read or parsed.
//
// Expected file shape (all keys optional):
//   { "window": { "width": 1600, "height": 1200 }, "scene": "world" }
RuntimeConfig load_runtime_config(int argc, char** argv);

}  // namespace ENG
