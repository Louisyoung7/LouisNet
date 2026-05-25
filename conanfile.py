from conan import ConanFile
from conan.tools.cmake import cmake_layout


class LouisNetConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    exports_sources = "CMakeLists.txt", "src/*", "example/*", "tests/*"
    generators = "CMakeToolchain", "CMakeDeps"

    def requirements(self):
        self.requires("spdlog/1.14.1")
        self.requires("gtest/1.14.0")

    def layout(self):
        cmake_layout(self)

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
