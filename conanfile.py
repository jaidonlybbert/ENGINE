from conan import ConanFile
from conan.tools.cmake import CMakeDeps, CMakeToolchain, cmake_layout

class EngineRecipe(ConanFile):
    name = "engine"
    version = "1.0"
    settings = "os", "arch", "compiler", "build_type"
    requires = (
        "glfw/3.4",
        "boost/1.88.0",
        "glm/1.0.1",
        "stb/cci.20240531",
        "tinyobjloader/2.0.0-rc10",
        "nlohmann_json/3.12.0"
    )
    generators = "CMakeDeps", "CMakeToolchain"

    def layout(self):
        cmake_layout(self)

    def build_requirements(self):
        pass  # Optional: add build tools here

    def generate(self):
        CMakeToolchain(self).generate()
        CMakeDeps(self).generate()

