from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout

class ClientServer(ConanFile):
    name: str = "client_server"
    version: str = "1.0"
    settings: tuple = "os", "build_type", "arch"
    
    exports_sources: list[str] = ["*"]
        
    def layout(self) -> None:
        cmake_layout(self)
        
    def build_requirements(self) -> None:
        self.tool_requires("cmake/4.2.0")
        self.test_requires("gtest/1.17.0")

    def requirements(self) -> None:
        self.requires("boost/1.90.0")   

    def generate(self) -> None:
        dp: CMakeDeps = CMakeDeps(self)
        dp.generate()

        tc: CMakeToolchain = CMakeToolchain(self)
        tc.generator = "Ninja"
        tc.generate()

    def build(self) -> None:
        cmake: CMake = CMake(self)
        cmake.configure()
        cmake.build()
    
    def package(self) -> None:
        cmake: CMake = CMake(self)
        cmake.install()
    