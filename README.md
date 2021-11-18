
Teide &#127755;
===============
A 3D renderer based on Vulkan

[![Build-and-Test](https://github.com/markridgewell/Teide/actions/workflows/Build-and-Test.yml/badge.svg)](https://github.com/markridgewell/Teide/actions/workflows/Build-and-Test.yml)
[![Format-Check](https://github.com/markridgewell/Teide/actions/workflows/Format-Check.yml/badge.svg)](https://github.com/markridgewell/Teide/actions/workflows/Format-Check.yml)

Requirements
------------

 * [CMake](https://cmake.org/) version 3.19 or greater
 * [vcpkg](https://github.com/Microsoft/vcpkg) - VCPKG_ROOT must be defined
 * [Vulkan SDK](https://www.vulkan.org/tools#download-these-essential-development-tools)

Building
--------

To generate a Visual Studio solution, run the command:

    cmake --preset=msvc

To build, either build the sln file in Visual Studio, or run the command:

    cmake --build build
