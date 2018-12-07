#!/usr/bin/env python
# -*- coding: utf-8 -*-

from conans import ConanFile, CMake, tools
import os


class SkoomaConan(ConanFile):
    name = "skooma"
    version = "0.0.0"
    description = "Utility for speedrunning Oblivion"
    url = "https://github.com/elizagamedev/skooma"
    author = "Eliza Velasquez"
    license = "GPL-3.0+"
    exports_sources = ["CMakeLists.txt", "exe/*", "dll/*", "skooma.ico", "skooma.rc"]
    generators = "cmake"
    settings = {
        "os": ["Windows"],
        "compiler": ["Visual Studio"],
        "arch": ["x86"],
        "build_type": None,
    }

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        self.copy("*.exe", dst="bin", src="bin")
        self.copy("*.dll", dst="bin", src="bin")
