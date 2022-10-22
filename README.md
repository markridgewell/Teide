
Teide &#127755;
===============
A 3D renderer based on Vulkan

[![Build-and-Test](https://github.com/markridgewell/Teide/actions/workflows/Build-and-Test.yml/badge.svg)](https://github.com/markridgewell/Teide/actions/workflows/Build-and-Test.yml)
[![Format-Check](https://github.com/markridgewell/Teide/actions/workflows/Format-Check.yml/badge.svg)](https://github.com/markridgewell/Teide/actions/workflows/Format-CheStatic-Analysisck.yml)
[![Static-Analysis](https://github.com/markridgewell/Teide/actions/workflows/Static-Analysis.yml/badge.svg)](https://github.com/markridgewell/Teide/actions/workflows/Static-Analysis.yml)

[![license](https://img.shields.io/badge/license-MIT-blue)](LICENSE)
[![codecov](https://codecov.io/gh/markridgewell/Teide/branch/main/graph/badge.svg?token=49D5P0U2C4)](https://codecov.io/gh/markridgewell/Teide)
[![pre-commit](https://img.shields.io/badge/pre--commit-enabled-brightgreen?logo=pre-commit&logoColor=white)](https://github.com/pre-commit/pre-commit)

Requirements
------------

 * [CMake](https://cmake.org/) version 3.19 or greater
 * [vcpkg](https://github.com/Microsoft/vcpkg) - VCPKG_ROOT must be defined
 * [Vulkan SDK](https://www.vulkan.org/tools#download-these-essential-development-tools)

Building
--------

**Windows**: To generate a Visual Studio solution, run the command:

    $ cmake --preset msvc2022

(msvc2019 is also supported)

To build, either build the sln file in Visual Studio, or run one of the following commands:

    $ cmake --build --preset msvc2022-debug
    $ cmake --build --preset msvc2022-release

**Linux**: To generate makefiles, run the command:

    $ cmake --preset linux -DCMAKE_BUILD_TYPE=Debug

The selected build type may be Debug or Release

To build with the selected build type, run the command:

    $ cmake --build --preset linux
