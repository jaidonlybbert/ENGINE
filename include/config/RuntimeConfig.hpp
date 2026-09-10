#pragma once
#include <cstdint>

namespace ENG {

// Runtime-tunable application settings. Currently only the initial window size;
// this is intended to grow (render adapter, startup scene, keybinds - see issue #3).
struct RuntimeConfig {
    std::uint32_t windowWidth;
    std::uint32_t windowHeight;
};

// The compiled-in defaults (from EngineConfig.hpp).
RuntimeConfig default_runtime_config();

// Start from the compiled-in defaults, then, if the command line contains
// `--config <path>`, overlay the values present in that JSON file.
// Throws std::runtime_error if an explicitly requested file cannot be read or parsed.
//
// Expected file shape (all keys optional):
//   { "window": { "width": 1600, "height": 1200 } }
RuntimeConfig load_runtime_config(int argc, char** argv);

}  // namespace ENG
