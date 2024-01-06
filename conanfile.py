from conan import ConanFile
from conan.tools.files import copy, collect_libs
import os


required_conan_version = ">=2.0"


class DotnetHostFxrConan(ConanFile):
    name = "dotnet-host-fxr"
    version = "8.0.0"
    settings = "os", "arch", "build_type"

    def __os(self):
        if self.settings.os == "Windows":
            return "win"
        else:
            raise ConanInvalidConfiguration("Currently only supported on Windows.")

    def __arch(self):
        if self.settings.arch == "x86_64":
            return "x64"
        else:
            raise ConanInvalidConfiguration("Currently only supported on x86_64.")

    def __config(self):
        if self.settings.build_type == "Debug":
            return "Debug"
        elif self.settings.build_type == "Release":
            return "Release"
        else:
            raise ConanInvalidConfiguration("Currently only supports Debug and Release.")

    def __config_folder_name(self):
        return "{}-{}.{}".format(self.__os(), self.__arch(), self.__config())

    def layout(self):
        self.folders.build = os.path.join(
            "source",
            "artifacts",
            "bin",
            self.__config_folder_name(),
            "corehost"
        )

        self.folders.source = self.folders.build

    def package(self):
        copy(self, "*.h", self.build_folder, os.path.join(self.package_folder, "include"), keep_path=False)
        copy(self, "libhostfxr.lib", self.build_folder, os.path.join(self.package_folder, "lib"), keep_path=False)

    def package_info(self):
        self.cpp_info.set_property("cmake_find_mode", "both")
        self.cpp_info.set_property("cmake_file_name", "HostFxr")
        self.cpp_info.set_property("cmake_target_name", "Dotnet::HostFxr")
        self.cpp_info.set_property("pkg_config_name", "HostFxr")

        self.cpp_info.libs = collect_libs(self)
