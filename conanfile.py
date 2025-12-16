import platform
from conan import ConanFile
from conan.tools.cmake import CMakeDeps, CMakeToolchain


class EngineRecipe(ConanFile):
    name = "engine"
    version = "1.0"
    settings = "os", "arch", "compiler", "build_type"
    requires = (
        "glfw/3.4",
        "glm/1.0.1",
        "stb/cci.20240531",
        "tinyobjloader/2.0.0-rc10",
        "nlohmann_json/3.12.0",
        "pmp/3.0.0",
        "lua/5.4.7",
        "flatbuffers/25.9.23",
        "asio/1.36.0",
    )

    if platform.platform() == "Windows":
        requires = requires + ("winreg/6.2.0",)

    def layout(self):
        self.folders.build = "build"
        self.folders.generators = "build/generators"
        self.folders.source = "."

    def build_requirements(self):
        pass  # Optional: add build tools here

    def generate(self):
        CMakeToolchain(self).generate()
        CMakeDeps(self).generate()
