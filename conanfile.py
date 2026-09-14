from conan import ConanFile
from conan.tools.cmake import CMakeDeps, CMakeToolchain


class EngineRecipe(ConanFile):
    name = "engine"
    version = "1.0"
    settings = "os", "arch", "compiler", "build_type"

    def requirements(self):
        self.requires("glm/1.0.1")
        self.requires("stb/cci.20240531")
        self.requires("tinyobjloader/2.0.0-rc10")
        self.requires("nlohmann_json/3.12.0")
        self.requires("pmp/3.0.0")
        self.requires("lua/5.4.7")
        self.requires("flatbuffers/25.9.23")
        self.requires("asio/1.36.0")
        self.requires("vulkan-memory-allocator/3.3.0")
        self.requires("joltphysics/5.2.0")
        self.requires("gtest/1.17.0")
        self.requires("spdlog/1.15.3")

        # GLFW has no Android backend at all (see issue #10/#46) - the Android build
        # excludes it and uses its own AndroidWindow/AndroidInput instead (issues #47/#49).
        if self.settings.os != "Android":
            self.requires("glfw/3.4")

        # Was previously gated on `platform.platform() == "Windows"`, which reflects the
        # machine running this recipe rather than the package's target os - harmless
        # before this project cross-compiled for anything, but wrong in general (e.g. it
        # would've pulled winreg into an Android build made from a Windows host). settings
        # captures the actual target.
        if self.settings.os == "Windows":
            self.requires("winreg/6.2.0")

    def layout(self):
        self.folders.build = "build"
        self.folders.generators = "build/generators"
        self.folders.source = "."

    def build_requirements(self):
        pass  # Optional: add build tools here

    def generate(self):
        CMakeToolchain(self).generate()
        CMakeDeps(self).generate()
