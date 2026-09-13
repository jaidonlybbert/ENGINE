#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "config/RuntimeConfig.hpp"

namespace {

// Writes `contents` to a uniquely named file for the lifetime of the object.
class TempFile {
   public:
    explicit TempFile(const std::string& contents) {
        path_ = std::filesystem::temp_directory_path() /
                ("engine_runtime_config_test_" + std::to_string(counter_++) + ".json");
        std::ofstream(path_) << contents;
    }
    ~TempFile() {
        std::error_code ec;
        std::filesystem::remove(path_, ec);
    }

    TempFile(const TempFile&) = delete;
    TempFile& operator=(const TempFile&) = delete;

    std::string path() const { return path_.string(); }

   private:
    std::filesystem::path path_;
    static inline int counter_ = 0;
};

// Builds an argv array that stays alive as long as the vector does.
std::vector<char*> make_argv(std::vector<std::string>& storage) {
    std::vector<char*> argv;
    for (auto& arg : storage) {
        argv.push_back(arg.data());
    }
    return argv;
}

}  // namespace

TEST(RuntimeConfigTest, ReturnsDefaultsWithoutConfigFlag) {
    std::vector<std::string> args{"engine"};
    auto argv = make_argv(args);

    const ENG::RuntimeConfig config = ENG::load_runtime_config(static_cast<int>(argv.size()), argv.data());
    const ENG::RuntimeConfig defaults = ENG::default_runtime_config();

    EXPECT_EQ(config.windowWidth, defaults.windowWidth);
    EXPECT_EQ(config.windowHeight, defaults.windowHeight);
    EXPECT_EQ(config.startupScene, defaults.startupScene);
}

TEST(RuntimeConfigTest, OverlaysWindowSizeFromFile) {
    TempFile file(R"({ "window": { "width": 800, "height": 600 } })");
    std::vector<std::string> args{"engine", "--config", file.path()};
    auto argv = make_argv(args);

    const ENG::RuntimeConfig config = ENG::load_runtime_config(static_cast<int>(argv.size()), argv.data());

    EXPECT_EQ(config.windowWidth, 800u);
    EXPECT_EQ(config.windowHeight, 600u);
}

TEST(RuntimeConfigTest, PartialFileKeepsOtherDefaults) {
    TempFile file(R"({ "window": { "width": 1024 } })");
    std::vector<std::string> args{"engine", "--config", file.path()};
    auto argv = make_argv(args);

    const ENG::RuntimeConfig config = ENG::load_runtime_config(static_cast<int>(argv.size()), argv.data());

    EXPECT_EQ(config.windowWidth, 1024u);
    EXPECT_EQ(config.windowHeight, ENG::default_runtime_config().windowHeight);
    EXPECT_EQ(config.startupScene, ENG::default_runtime_config().startupScene);
}

TEST(RuntimeConfigTest, OverlaysStartupSceneFromFile) {
    TempFile file(R"({ "scene": "blue_sky" })");
    std::vector<std::string> args{"engine", "--config", file.path()};
    auto argv = make_argv(args);

    const ENG::RuntimeConfig config = ENG::load_runtime_config(static_cast<int>(argv.size()), argv.data());

    EXPECT_EQ(config.startupScene, "blue_sky");
    EXPECT_EQ(config.windowWidth, ENG::default_runtime_config().windowWidth);
}

TEST(RuntimeConfigTest, ThrowsWhenConfigFileMissing) {
    std::vector<std::string> args{"engine", "--config", "definitely/not/a/real/path.json"};
    auto argv = make_argv(args);

    EXPECT_THROW(ENG::load_runtime_config(static_cast<int>(argv.size()), argv.data()), std::runtime_error);
}

TEST(RuntimeConfigTest, ThrowsOnInvalidJson) {
    TempFile file("this is not json");
    std::vector<std::string> args{"engine", "--config", file.path()};
    auto argv = make_argv(args);

    EXPECT_THROW(ENG::load_runtime_config(static_cast<int>(argv.size()), argv.data()), std::runtime_error);
}
