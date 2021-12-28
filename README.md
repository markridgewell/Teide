
Teide &#127755;
===============
A 3D renderer based on Vulkan

[![Build-and-Test](https://github.com/markridgewell/Teide/actions/workflows/Build-and-Test.yml/badge.svg)](https://github.com/markridgewell/Teide/actions/workflows/Build-and-Test.yml)
[![Format-Check](https://github.com/markridgewell/Teide/actions/workflows/Format-Check.yml/badge.svg)](https://github.com/markridgewell/Teide/actions/workflows/Format-CheStatic-Analysisck.yml)
[![Static-Analysis](https://github.com/markridgewell/Teide/actions/workflows/Static-Analysis.yml/badge.svg)](https://github.com/markridgewell/Teide/actions/workflows/Static-Analysis.yml)
[![license](https://img.shields.io/badge/license-MIT-blue)](LICENSE)
[![pre-commit](https://img.shields.io/badge/pre--commit-enabled-brightgreen?logo=pre-commit&logoColor=white)](https://github.com/pre-commit/pre-commit)

Requirements
------------

 * [CMake](https://cmake.org/) version 3.19 or greater
 * [vcpkg](https://github.com/Microsoft/vcpkg) - VCPKG_ROOT must be defined
 * [Vulkan SDK](https://www.vulkan.org/tools#download-these-essential-development-tools)

Building
--------

**Windows**: To generate a Visual Studio solution, run the command:

    cmake --preset msvc

To build, either build the sln file in Visual Studio, or run one of the following commands:

    cmake --build --preset msvc-debug
    cmake --build --preset msvc-release

**Linux**: To generate makefiles, run the command:

    cmake --preset unix

To build, run the command:

    cmake --build --preset unix
